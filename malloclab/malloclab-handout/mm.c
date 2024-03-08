/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "solo mission",
    /* First member's full name */
    "yidong ding",
    /* First member's email address */
    "ydding2001@sjtu.edu.cn",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
/*private global variables */
static char* heap_listp; 
static char* last_bp;
static void* next_fitp;
// static char *mem_heap; /*points to first byte of heap */
// static char *mem_brk ; /*points to last byte of heap plus 1 */
// static char *mem_max_addr; /*max legal heap addr plus 1*/

/*Basic constants and macros */
#define WSIZE 4 /* word and header/footer size (bytes) */
#define DSIZE 8 /* double word size */
#define CHUNKSIZE (1<<12) /* extend heap by this amount */

#define MAX(x,y) ((x) > (y)? (x):(y))

/* pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size)|(alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int*)(p)) /* sizeof(unsigned int) = 4 = WSIZE */
#define PUT(p, val) (*(unsigned int*)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer (bytes)*/
#define HDRP(bp) ((char*)(bp) - WSIZE)  /* sizeof(char) = 1 */
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/*given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE((char*)(bp) - WSIZE))
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE((char*)(bp) - DSIZE))

static void place(void *bp, size_t asize);
static void* extend_heap(size_t words);
static void* coalesce(void* bp);
static void place(void *bp, size_t asize);
static void* first_fit(size_t asize);
static void* best_fit(size_t asize);
static void* next_fit(size_t asize);
static void* next_fit2(size_t asize);
int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *bp);
void *mm_realloc(void *ptr, size_t size);
void checkblock(void* bp);
void mm_printblock();


void mm_printblock() {
    char *curbp;
    printf("\n=========================== malloc heap ===========================\n");
    for (curbp = heap_listp; GET_SIZE(HDRP(curbp)) > 0; curbp = NEXT_BLKP(curbp)) {
        printf("address = %p\n", curbp);
        printf("hsize = %x, fsize = %x\n", GET_SIZE(HDRP(curbp)), GET_SIZE(FTRP(curbp)));
        printf("halloc = %x, falloc = %x\n", GET_ALLOC(HDRP(curbp)), GET_ALLOC(FTRP(curbp)));
        printf("\n");
    }
    //epilogue blocks
    printf("address = %p\n", curbp);
    printf("hsize = %x\n", GET_SIZE(HDRP(curbp)));
    printf("halloc = %x\n", GET_ALLOC(HDRP(curbp)));
    printf("=========================== malloc heap ===========================\n");
}


void checkblock(void* bp) {
    printf("next_blkp:%x\n", NEXT_BLKP(bp));
    printf("prev_blkp:%x\n", PREV_BLKP(bp));
    printf("size:%x\n", GET_SIZE(HDRP(bp)));
    // prinf("highest pointer:%x\n")
}


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void*)-1)
    {
        return -1;
    }
    PUT(heap_listp, 0); /*padding*/
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));  /* Prologue header */
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));  /* Prologue footer */
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));   /* epilogue header */
    heap_listp += (2*WSIZE);
    last_bp = heap_listp;
    next_fitp = heap_listp;

    /* extend the empty heap with a free block of CHUNKSIZE */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{

    size_t asize;  /* adjusted block size */
    size_t extendsize; /* amount to extend heap if no fit */
    char *bp;  /* byte */

    /* Ignore spurious requests */
    if(size == 0){
        return NULL;
    }

    if (size <= DSIZE) {
        asize = 2 * DSIZE;
    }
    else {
        /* Rounding up */
        asize = DSIZE * ((size + DSIZE + DSIZE - 1) / DSIZE);
    }

    /* search the free list for a fit */
    if ((bp = next_fit2(asize)) != NULL) {
        place(bp, asize);
        // mm_printblock();
        return bp;
    }

    /* no fit found. Get more memory and place the block */
    // curr_fit_bp = heap_listp; /* no fit found */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL) {
        return NULL;
    }
    if (bp == 0x7ffff68a6228) {
            printf("%s", "dkas");
        }
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block and merge the empty blocks.
 */
void mm_free(void *bp)
{
    if (bp == 0x7ffff68a6228) {
        printf("%s", "skdjfl");
    }
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));  /* set to zero */
    PUT(FTRP(bp), PACK(size, 0));  /* set to zero */

    coalesce(bp); /* merge free adjacent blocks */
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == 0x7ffff68a6228) {
        printf("%s", "skdjfl");
    }
    if ((ptr == NULL) && (size != 0)) {
        return mm_malloc(size);
    }
    else if ((ptr != NULL) && (size == 0)) {
        mm_free(ptr);
        return NULL;
    }
    else if ((ptr == NULL) && (size == 0)) {
        return NULL;
    }
    else {
        void *oldptr = ptr;
        void *newptr;
        size_t copySize;
    
        newptr = mm_malloc(size);
        if (newptr == NULL)
            return NULL;

        
        size = GET_SIZE(HDRP(oldptr));
        copySize = GET_SIZE(HDRP(newptr));
        if (size < copySize)
            copySize = size;
        memcpy(newptr, oldptr, copySize-DSIZE);
        mm_free(oldptr);
        return newptr;
    }
}
/* 
 * extend_heap - extend the heap. it will be called in initialization and
 * mm_malloc can not find a proper block.
 */
static void* extend_heap(size_t words) {
    char *bp;
    size_t size;
    size = (words % 2) ? (words+1) * WSIZE : (words) * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    /* coalesce if the previous block was free */
    return coalesce(bp);
}

/* 
 * coalesce - merge two empty blocks, return a pointer to the merged block.
 */
static void* coalesce(void* bp) {
    if (bp == 0x7ffff68a6228) {
        printf("%s", "skdjfl");
    }
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    
    if (prev_alloc && next_alloc) {
        return bp;
    }
    else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        return bp;
    }
    else if (!prev_alloc && next_alloc){
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        if (bp == last_bp) {
            last_bp = PREV_BLKP(bp);
            printf("coalesce:%x\n", last_bp);
        }
        bp = PREV_BLKP(bp);
        return bp;
    }

    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
            GET_SIZE(HDRP(NEXT_BLKP(bp)));

        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        
        if (bp == last_bp) {
            last_bp = PREV_BLKP(bp);
            printf("coalesce2:%x\n", last_bp);
        }
        bp = PREV_BLKP(bp);
        return bp;
    }
}

/* 
 * find_fit - find block, First-fit search.
 */
static void* first_fit(size_t asize) {
    char* ptr;
    for(ptr = heap_listp; GET_SIZE(HDRP(ptr)) > 0; ptr=NEXT_BLKP(ptr)) {
        printf("bp:%x\n", ptr);
        if((!GET_ALLOC(HDRP(ptr))) && (GET_SIZE(HDRP(ptr)) >= asize))
            return ptr;
    }
    return NULL;
}

static void* best_fit(size_t asize) {
    char* bp;
    char* best_bp;
    best_bp = NULL;
    size_t min_rest_size = mem_heapsize();
    for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp=NEXT_BLKP(bp)) {
        if((!GET_ALLOC(HDRP(bp))) && (GET_SIZE(HDRP(bp)) >= asize)) {
            if (min_rest_size > GET_SIZE(HDRP(bp)) - asize) {
                min_rest_size = GET_SIZE(HDRP(bp)) - asize;
                best_bp = bp;
            }
        }
    }
    return best_bp;
}


static void* next_fit(size_t asize) {
    char* ptr; // 0x7ffff68bf098
    for(ptr = last_bp; GET_SIZE(HDRP(ptr)) > 0; ptr=NEXT_BLKP(ptr)) {
        if((!GET_ALLOC(HDRP(ptr))) && (GET_SIZE(HDRP(ptr)) >= asize)) {
            last_bp = ptr;
            if (ptr == 0x7ffff68a6228) {
                printf("next fit bp:%x\n", ptr);
            }
            return ptr;
        }   
    }
    for (ptr = heap_listp; ptr != NEXT_BLKP(last_bp); ptr=NEXT_BLKP(ptr)) {
        if((!GET_ALLOC(HDRP(ptr))) && (GET_SIZE(HDRP(ptr)) >= asize)) {
            last_bp = ptr;
            if (ptr == 0x7ffff68a6228) {
                printf("%s", "skdjfl");
            }
            // printf("next fit bp:%x\n", ptr);
            return ptr;
        }  
    }
    return NULL;
}

static void place(void *bp, size_t asize) {
    if (bp == 0x7ffff68a6228) {
        printf("%s", "skdjfl");
    }
    size_t csize = GET_SIZE(HDRP(bp)); // original block size.
    size_t rest_size = csize - asize;
    if (rest_size < 2 * DSIZE) {
        /* do not split block, do not change the size. */
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
    else {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        /*
        char* new_footer = HDRP(bp) + asize - WSIZE; 
        PUT(new_footer, PACK(asize, 1));
        char* new_header = HDRP(bp) + asize; 
        */
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(rest_size, 0));
        PUT(FTRP(bp), PACK(rest_size, 0));
    }
}

static void *next_fit2(size_t size) {
    char *lastp;
    if (next_fitp != heap_listp) {
        next_fitp = NEXT_BLKP(next_fitp);  //此次搜索开始的位置
    }
        
    lastp = next_fitp;
    for (;GET_SIZE(HDRP(next_fitp)) > 0; next_fitp = NEXT_BLKP(next_fitp)) {
        if (!GET_ALLOC(HDRP(next_fitp)) && (GET_SIZE(HDRP(next_fitp)) >= size)) {
            return next_fitp;
        }
    }
    next_fitp = NEXT_BLKP(heap_listp);
    for (;next_fitp != lastp; next_fitp = NEXT_BLKP(next_fitp)) {
        if (!GET_ALLOC(HDRP(next_fitp)) && (GET_SIZE(HDRP(next_fitp)) >= size)) {
            return next_fitp;
        }
    }
    return NULL;
}



















