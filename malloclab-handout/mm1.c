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

// define the maximum size for the segregated lists
#define MAXSIZE_L1  3*DSIZE
#define MAXSIZE_L2  4*DSIZE
#define MAXSIZE_L3  5*DSIZE
#define MEXSIZE_L4  9*DSIZE

// define the offset from the start of theheap of each segregated list
#define OFF_L1  0
#define OFF_L2  DSIZE
#define OFF_L3  2*DSIZE
#define OFF_L4  3*DSIZE
#define OFF_L5  4*DSIZE

//total nb of lists

#define NB_LIST 5 

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

#define NEXT_FREE(bp)  ((char *) ((char *)(bp)))    //the adress of the next block is contained in a free block
#define PREV_FREE(bp)  ((char *)) ((char *)(bp) - DSIZE) // Not sure...
static char *heap_listp;// pointer to the prologue block

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
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
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
    return bp;  
}


static void *extend_heap(size_t words){

    char *bp;
    size_t size;
    
    printf("try to extend the heap with size %d", words);

    if (words == 0){
        return NULL;
    }
agre
    size = (words % 2) ? (words+1)*WSIZE : words * WSIZE; // make sure size is divisible by DSIZE
    bp = mem_sbrk(size);
    
    if ((long)(bp) == -1){
        return NULL;
    }

    PUT(HDRP(bp), PACK(size,0));            // This HDR overwrites old epilogue
    PUT(FTRP(bp), PACK(size,0));            // This FTR is 2 WSIZE before the new brk pointer
    PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1));     // Create new Epilogue as next block to newly free space, placed WSIZE before brk pointer 
    //printf("actually extend heap with size %d", size);
    return coalesce(bp);
}

static char* find_inlist(size_t blocksize, size_t offset){

    char* bp = GET(heap_listp + offset); //initialize bp at the beginning of the suited segregated list
    while (bp != NULL){
        if (blocksize <= GET_SIZE(HDRP(bp))){
            break;
        }
        bp = (char *) GET(NEXT_FREE(bp));
    }
    return bp;
}
static char *bestfit(size_t size){
    size_t good_offL;

    if (size <= MAXSIZE_L1){
        good_offL = OFF_L1;
    }

    else if (size <= MAXSIZE_L2){
        good_offL = OFF_L2;
    }

    else if (size <= MAXSIZE_L3){
        good_offL = OFF_L3;
    }

    else if (size <= MAXSIZE_L4){
        good_offL = OFF_L4;
    }

    else {
        good_offL = OFF_L5
    }

    char* bp = NULL;

    for (size_t offset = good_offL; offset<= OFF_L5; offset += 8){
        if ((bp=find_inlist(size, offset)) != NULL){
            return bp;
        }
    }
    return NULL;
}

static void place(void *bp, size_t size){
    size_t sizeblock = GET_SIZE(HDRP(bp));
    removefrom_L(ptr,sizeblock); //since we are allocating this block need to remove it from its segregated list
    PUT(HDRP(bp), PACK(sizeblock,1));
    PUT(FTRP(bp), PACK(sizeblock,1));
}


int mm_init(void){

    heap_listp = mem_sbrk(NB_LIST * DSIZE);

    if (heap_listp == NULL){
        printf("mem_sbrk fails\n");
        return -1;
    }

    //Initialize the starting blocks of the segregated lists
    
    PUT(heap_listp + OFF_L1, (size_t) NULL);   //for segregated lists we store in the footer (only for free blocks) the address of the next block of the segregated list 
    PUT(heap_listp + OFF_L2, (size_t) NULL);
    PUT(heap_listp + OFF_L3, (size_t) NULL);
    PUT(heap_listp + OFF_L4, (size_t) NULL);
    PUT(heap_listp + OFF_L5, (size_t) NULL);

    //create prologue and epilogue
    heap_pro = mem_sbrk(4*WSIZE);

    if (heap_pro == NULL){
        printf("mem_sbrk fails\n");
        return -1;
    }

    PUT(heap_pro,0);                        //Alignment padding
    PUT(heap_pro + WSIZE, PACK(DSIZE,1));   //Prologue HDRP
    PUT(heap_pro + 2*WSIZE, PACK(DSIZE,1)); //Prologue FTRP
    PUT(heap_pro + 3*WSIZE, PACK(0,1));     //Epilogue

    //add extra space to the heap

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL){ //cannot extend heap
        return -1;
    }

    printf("mm_init() success\n");
    
    return 0;
}

void *mm_malloc(size_t size) {
    
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size == 0){
        return NULL;
    }

    printf("\nTrying to allocate memory - end block at %p:\n", mem_heap_hi());
    
    asize = ALIGN(size) + DSIZE; //align size and add space for HDR and FTR

    bp = bestfit(asize); // find free block which can fit asize

    if (bp != NULL){ 
        printf("Fit found at %p, now placing.\n", bp);
        place(bp, asize);
        return bp;
    }

    // no place left to fit a size

    extendsize = MAX(asize, CHUNKSIZE);
    printf("No fit found in heap. Extending heap by %d\n", extendsize);
    bp = extend_heap(extendsize/WSIZE);
    if (bp  == NULL){
        printf("extend_heap() failed\n");
        return NULL;
    }

    place(bp, asize);

    return bp;
}

void mm_free(void *ptr){

    printf("try to free block\n");
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size,0));
    addto_segL(ptr,size);  //since we have freed, need to add it to its suited segregated list
    coalesce(ptr);
}