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
    "Team A",
    /* First member's full name */
    "Guillaume Laine",
    /* First member's email address */
    "guillaume.laine.2020@polytechnique.edu",
    /* Second member's full name (leave blank if none) */
    "Marine Hoche",
    /* Second member's email address (leave blank if none) */
    "marine.hoche@polytechnique.edu"
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define WSIZE 4                 // Word and header/footer size
#define DSIZE 8                 // Double word size
#define CHUNKSIZE (1<<12)       // Extend heap by this amount

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define MAX(size1, size2) ((size1) > (size2) ? (size1) : (size2))
#define GET(p) (*(unsigned int *)(p))
#define PUT(p,val) (*(unsigned int *)(p)= (val))
#define PACK(size, alloc) ((size) | (alloc))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HDRP(bp) ((char *)(bp) - 4)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp))-8)

#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static char *heap_listp;// pointer to the prologue block

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc){
        return bp;
    }

    else if (prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size,0));
        PUT(FTRP(bp), PACK(size,0));

    }

    else if (!prev_alloc && next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        PUT(FTRP(bp), PACK(size,0));
        bp = PREV_BLKP(bp);
    }

    else{
        size += GET_SIZE(HDRP(PREV_BLKP(bp)))+ GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
        bp = PREV_BLKP(bp);
    }

    return bp;
}


static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    size = (words % 2) ? (words+1)*WSIZE : words * WSIZE;
    bp = mem_sbrk(size);
    if ((long)(bp) == -1){
        printf("NOOOOOOOO\n");
        return NULL;
    }

    PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));
    PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1));

    return coalesce(bp);
}

char *find_fit(size_t size){
    // Implemented using first fit 
    //void *bp = mem_heap_lo() + 2*WSIZE; // start right after Prologue HDR
    void *bp = heap_listp;
    void *end = mem_heap_hi();

    while ((bp < end) && (GET_ALLOC(HDRP(bp)) || size >= GET_SIZE(HDRP(bp)))){
        printf("\nRunning loop for size %d. Current block of size %d at %p\n", size, GET_SIZE(HDRP(bp)), bp);
        printf("Next block pointer %p\n", NEXT_BLKP(bp));
        printf("End pointer is %p\n", end);
        bp = NEXT_BLKP(bp);
    }

    if (bp == end) {
        return NULL;
    }

    return bp;
}

void place(char* bp, size_t size){
    size_t sizeblock = GET_SIZE(HDRP(bp));


    if (sizeblock >= size + 2*DSIZE){ // Split iff remaining free block has space for FTR, HDR, and 1 DSIZE (to respect alignement requirements) of payload
        size_t sizesplit = sizeblock - size;

        //put allocated block before free block

        /*char* splitp = (char*) (bp) + size;

        PUT(HDRP(bp), PACK(size,1));
        PUT(FTRP(bp), PACK(size,1));
        PUT(HDRP(splitp), PACK(sizesplit,0));
        PUT(FTRP(splitp), PACK(sizesplit,0));*/

        //put the allocated block at the end of the free block

        PUT(HDRP(bp), PACK(sizesplit,0));
        PUT(FTRP(bp), PACK(sizesplit,0));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(size, 1));
        PUT(FTRP(PREV_BLKP(bp)), PACK(size, 1));

        bp = NEXT_BLKP(bp);
    }

    // Do not split
    else {
        PUT(HDRP(bp), PACK(sizeblock,1));
        PUT(FTRP(bp), PACK(sizeblock,1));
    }
}


int mm_check(void) {
    // check for inconsistencies 

    //look for overlapping

    char* p = heap_listp;

    while((char *)mem_heap_hi()>p){
        if (p + GET_SIZE(HDRP(p)) == NEXT_BLKP(p)){
            p = NEXT_BLKP(p);
        }
        else {
            return 0;
        }
    }
    return 1;
}



/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /*
    void *p = mem_sbrk(4*WSIZE);
    
    if (p == (void *)-1){
        return -1;
    }

    PUT(p, 0);
    PUT(p + 1*WSIZE, PACK(DSIZE,1));
    PUT(p + 2*WSIZE, PACK(DSIZE,1));
    PUT(p + 3*WSIZE, PACK(0,1));
    // p += 2*WSIZE;

    // heap_listp = p;

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL){ //cannot extend heap
        return -1;
    }

    return 0;//
    */
    heap_listp = mem_sbrk(4*WSIZE);
    
    if (heap_listp == (void *)-1){
        return -1;
    }

    PUT(heap_listp, 0);
    PUT(heap_listp + 1*WSIZE, PACK(DSIZE,1));
    PUT(heap_listp + 2*WSIZE, PACK(DSIZE,1));
    PUT(heap_listp + 3*WSIZE, PACK(0,1));
    heap_listp += 2*WSIZE;

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL){ //cannot extend heap
        return -1;
    }
    printf("\ninit success\n");
    return 0;
}


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    /* ###initial code###
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }*/
    
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size == 0){
        return NULL;
    }
    printf("try to allocate memory\n");
    if (size <= DSIZE) {
        asize = 2 * DSIZE;
    }

    else{
        asize = DSIZE * ((size + DSIZE + DSIZE - 1)/DSIZE);
    }

    bp = find_fit(asize);

    if (bp != NULL){
        place(bp,asize);
        if (mm_check()) {
            printf("malloc success\n");
        }
        else {
            printf("check fails in mm_malloc\n");
        }
        return bp;
    }

    extendsize = MAX(asize,CHUNKSIZE);
    bp = extend_heap(extendsize/WSIZE);
    if (bp  == NULL){
        printf("malloc fail");
        return NULL;
    }

    place(bp, asize);
    if (mm_check()) {
            printf("malloc success with extented heap\n");
        }
    else {
            printf("check fails in mm_malloc and try to extend heap\n");
        }
    return bp;

}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{   
    printf("try to free block\n");
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size,0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}








