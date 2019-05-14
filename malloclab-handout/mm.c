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
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE((HDRP(bp))))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static char *heap_listp;// pointer to the prologue block

static int mm_check(void) {
    // check for inconsistencies 

    //look for overlapping

    /*char* p = heap_listp;
    size_t totalsize = 0;
    //Check coalescing 
    while((char *)mem_heap_hi()>p){
        if (p> heap_listp){
            if (!GET_ALLOC(HDRP(p)) && (!GET_ALLOC(NEXT_BLKP(HDRP(p))) || !GET_ALLOC(PREV_BLKP(HDRP(p))))){ //check that a free block has no free block neighboors
            printf("\nproblem with coalesce()\n");
            return 0;
            }
        }

        else{
            if (!GET_ALLOC(HDRP(p)) && !GET_ALLOC(NEXT_BLKP(p))){
                printf("\nproblem with coalesce()\n");
                return 0;
            }
        }

        totalsize += GET_SIZE(HDRP(p));
        p = NEXT_BLKP(p);
    }

    if (mem_heapsize()!=totalsize){
        printf("\nthe size of the heap is not coherent\n");
        return 0;
    }
    return 1;*/

    // check for no 0 size block in the middle

    char* p = heap_listp;
    char* stop = mem_heap_hi();

    printf("\n the start is at %p and the end at %p", p, stop);

    while (p<= stop){
        if (GET_SIZE(HDRP(p))==0){
            printf("\nget a block of size 0 at %p\n", p);
            return 0;
        }
        p = NEXT_BLKP(p);
    }

    return  1;
}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    // Previous and next blocks are allocated
    if (prev_alloc && next_alloc){
        return bp;
    }

    // Only previous block is allocated
    else if (prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size,0));
        PUT(FTRP(bp), PACK(size,0));
    }

    // Only next block is allocated
    else if (!prev_alloc && next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        PUT(FTRP(bp), PACK(size,0));
        bp = PREV_BLKP(bp);
    }

    // Previous and next blocks are free
    else{
        size += GET_SIZE(HDRP(PREV_BLKP(bp)))+ GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
        bp = PREV_BLKP(bp);
    }
    
    if (!mm_check()){
        printf("pb in coalesce \n");
    }
    return bp;
}


static void *extend_heap(size_t words){
    char *bp;
    size_t size;
    printf("try to extend the heap with size %d", words);
    if (words == 0){
        return NULL;
    }
    size = (words % 2) ? (words+1)*WSIZE : words * WSIZE; // make sure size is divisible by DSIZE
    bp = mem_sbrk(size);
    if ((long)(bp) == -1){
        return NULL;
    }

    PUT(HDRP(bp), PACK(size,0));            // This HDR overwrites old epilogue
    PUT(FTRP(bp), PACK(size,0));            // This FTR is 2 WSIZE before the new brk pointer
    PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1));     // Create new Epilogue as next block to newly free space, placed WSIZE before brk pointer 
    printf("actually extend heap with size %d", size);
    if (!mm_check()){
        printf("pb in extend heap\n");
    }
    return coalesce(bp);
}

static char *find_fit(size_t size){
    // Implemented using first fit 

    /*void *bp = heap_listp;
    void *end = mem_heap_hi();

    if (bp >= end) {
        // no fit. need to extend heap.
        return NULL;
    }

    while ((bp < end) && (GET_ALLOC(HDRP(bp)) || size > GET_SIZE(HDRP(bp)))){
        //printf("Trying to fit %d bytes in block of size %d at %p...\n", size, GET_SIZE(HDRP(bp)), bp);
        //printf("Prologue at %p, size of prologue %d Epilogue at %p\n", heap_listp, GET_SIZE(HDRP(heap_listp)), end);
        if (GET_ALLOC(HDRP(bp))) {
            //printf("Already allocated.\n");
        }
        else {
            //printf("Size insufficient.\n");
        }
        bp = NEXT_BLKP(bp);
    }

    if (bp >= end) {
        // no fit. need to extend heap.
        return NULL;
    }

    return bp;*/

    void *bp;

    for(bp = heap_listp; GET_SIZE(HDRP(bp))>0; bp = NEXT_BLKP(bp)){
        if (!GET_ALLOC(HDRP(bp)) && (size <= GET_SIZE(HDRP(bp)))){
            return bp;
        }
    }
    return NULL;
}

static void place(char* bp, size_t size){
    size_t sizeblock = GET_SIZE(HDRP(bp));
    size_t sizesplit = sizeblock - size;

    if (sizesplit >= 2*DSIZE){
        PUT(HDRP(bp), PACK(size,1));
        PUT(FTRP(bp), PACK(size,1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(sizesplit,0));
        PUT(FTRP(bp), PACK(sizesplit,0));
        printf("with splitting\n");

    }

    else {
        PUT(HDRP(bp), PACK(sizeblock,1));
        PUT(FTRP(bp), PACK(sizeblock,1));
        printf("without splitting\n");
    }

    printf("Placing %d bytes in %d bytes of free space at %p ", size, sizeblock, bp);

    if (!mm_check()){
        printf("pb in place\n");
    }
   /* if (sizeblock >= size + 2*DSIZE){ // Split iff remaining free block has space for FTR, HDR, and 1 DSIZE (to respect alignement requirements) of payload
        size_t sizesplit = sizeblock - size;

        //put allocated block before free block

        char* splitp = (char*) (bp) + size;

        PUT(HDRP(bp), PACK(size,1));
        PUT(FTRP(bp), PACK(size,1));
        PUT(HDRP(splitp), PACK(sizesplit,0));
        PUT(FTRP(splitp), PACK(sizesplit,0));
        printf("with splitting\n");
        //bp = NEXT_BLKP(bp); // bp no points 
    }

    // Do not split
    else {
        PUT(HDRP(bp), PACK(sizeblock,1));
        PUT(FTRP(bp), PACK(sizeblock,1));
        printf("without splitting\n");
    }*/

    /*PUT(HDRP(bp), PACK(sizeblock,1));
    PUT(FTRP(bp), PACK(sizeblock,1));*/
}


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    heap_listp = mem_sbrk(4*WSIZE); // 4 WSIZE for alignement, Prologue HDR, FTR and Epilogue
    
    if (heap_listp == (void *)-1){
        return -1;
    }

    PUT(heap_listp, 0);                             // Alignment is unallocated and said to be of size 0 by convention
    PUT(heap_listp + 1*WSIZE, PACK(DSIZE,1));       // Prologue HDR
    PUT(heap_listp + 2*WSIZE, PACK(DSIZE,1));       // Prologue FDR
    PUT(heap_listp + 3*WSIZE, PACK(0,1));           // Epilogue 

    heap_listp += 2*WSIZE;                          // Point to Prologue 

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
    printf("\nTrying to allocate memory - end block at %p:\n", mem_heap_hi());
    
    asize = ALIGN(size) + DSIZE; //align size and add space for HDR and FTR

    bp = find_fit(asize); // find free block which can fit asize

    if (bp != NULL){ 
        printf("Fit found at %p, now placing.\n", bp);
        place(bp, asize);
        /*
        if (mm_check()) {
            // printf("malloc success\n");
        }
        else {
            // printf("check fails in mm_malloc\n");
        }
        return bp;
        */
    }

    // no place left to fit a size

    extendsize = MAX(asize, CHUNKSIZE);
    printf("No fit found in heap. Extending heap by %d\n", extendsize);
    bp = extend_heap(extendsize/WSIZE);
    if (bp  == NULL){
        printf("sbrk failed\n");
        return NULL;
    }

    if (bp == mem_heap_hi()+1){
        printf("\nlittle problem maybe...\n");
    }
    place(bp, asize);
    /*
    if (mm_check()) {
            // printf("malloc success with extented heap\n");
        }
    else {
            // printf("check fails in mm_malloc and try to extend heap\n");
        }
        */
    if (!mm_check()){
        printf("pb in mm_malloc\n");
    }
    return bp;

}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{   
    // printf("try to free block\n");
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








