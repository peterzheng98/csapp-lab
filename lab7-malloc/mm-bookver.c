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

team_t team = {
    /* Team name */
    "XXXXXXX",
    /* First member's full name */
    "123",
    /* First member's email address */
    "234",
    /* Second member's full name (leave blank if none) */
    "345",
    /* Second member's email address (leave blank if none) */
    "456"
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
#define MAX_HEAP 15

#define BASIC_WORD_SIZE 4
#define BASIC_DWORD_SIZE 8

#define HEADER_WRITE_SIZE 4
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

void* heap_list_ptr;
void* heap_block[MAX_HEAP];

inline void put_val(void* ptr, unsigned int p){ *(unsigned int *) ptr = p; }
inline unsigned int get_val(void* ptr){ return *(unsigned int *) ptr; }

inline void* get_array_elem_ptr(void* base_ptr, int offset){ return base_ptr + offset; }
inline unsigned int get_size(void* p){ return get_val(p) & ~0x7; }
inline unsigned int get_allocation_size(void* p){ return get_val(p) & 0x1; }
inline void* header_addr(void* bp){ return ((char*)bp - BASIC_WORD_SIZE); }
inline void* footer_addr(void* bp){ return ((char*)bp + get_size(header_addr(bp)) - BASIC_DWORD_SIZE); }
inline void* next_blkp(bp){ return (char *)(bp)+get_size(((char *)(bp)-BASIC_WORD_SIZE)); }
inline void* prev_blkp(bp){ return (char *)(bp)-get_size(((char *)(bp)-BASIC_DWORD_SIZE)); }

#define GET(p)  (*(unsigned int *)(p))
#define PUT(p,val)  (*(unsigned int *)(p) = (val))
#define PACK(size,alloc)    ((size) | (alloc))

#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p)    (GET(p) & 0x1)

#define HDRP(bp)    ((char *)(bp)-BASIC_WORD_SIZE)
#define FTRP(bp)    ((char *)(bp)+GET_SIZE(HDRP(bp))-BASIC_DWORD_SIZE)

#define NEXT_BLKP(bp)   ((char *)(bp)+GET_SIZE(((char *)(bp)-BASIC_WORD_SIZE)))
#define PREV_BLKP(bp)   ((char *)(bp)-GET_SIZE(((char *)(bp)-BASIC_DWORD_SIZE)))

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t size);
static void place(void *bp,size_t asize);

int calculate_bit_size(int a)
{
    int count = 0;
    while(a){
        a = a & (a - 1);
        count++;
    }
    return count;
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if((heap_list_ptr = mem_sbrk(HEADER_WRITE_SIZE*4)) == (void*) -1) return -1;
    int idx = 0;
    // for(; idx < MAX_HEAP; ++idx) heap_block[idx] = NULL;
    // put_val(get_array_elem_ptr(heap_list_ptr, 0), 0); // padding
    // put_val(get_array_elem_ptr(heap_list_ptr, HEADER_WRITE_SIZE), 9);
    // put_val(get_array_elem_ptr(heap_list_ptr, HEADER_WRITE_SIZE << 1), 9);
    // put_val(get_array_elem_ptr(heap_list_ptr, (HEADER_WRITE_SIZE << 1) + HEADER_WRITE_SIZE), 1);
    PUT(heap_list_ptr,0);
    PUT(heap_list_ptr+(1*BASIC_WORD_SIZE),9);
    PUT(heap_list_ptr+(2*BASIC_WORD_SIZE),9);
    PUT(heap_list_ptr+(3*BASIC_WORD_SIZE),1);
    heap_list_ptr = heap_list_ptr + (HEADER_WRITE_SIZE << 1);
    if(extend_heap(1 << 10) == NULL) return -1;
    return 0;
}

static void* coalesce(void* ptr){
    // size_t prev_alloc_size = get_allocation_size(footer_addr(prev_blkp(ptr)));
    // size_t next_alloc_size = get_allocation_size(header_addr(next_blkp(ptr)));
    size_t prev_alloc_size = GET_ALLOC(FTRP(PREV_BLKP(ptr)));
    size_t next_alloc_size = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t len = GET_SIZE(HDRP(ptr));

    if(prev_alloc_size && next_alloc_size){
        return ptr;
    } else if(prev_alloc_size && !next_alloc_size){
        len += GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(ptr), PACK(len,0));
        PUT(FTRP(ptr), PACK(len,0));
    } else if(!prev_alloc_size && next_alloc_size){
        len += GET_SIZE(HDRP(PREV_BLKP(ptr)));
        PUT(FTRP(ptr),PACK(len,0));
        PUT(HDRP(PREV_BLKP(ptr)),PACK(len,0));
        ptr = PREV_BLKP(ptr);        
    } else {
        len +=GET_SIZE(FTRP(NEXT_BLKP(ptr)))+ GET_SIZE(HDRP(PREV_BLKP(ptr)));
        PUT(FTRP(NEXT_BLKP(ptr)),PACK(len,0));
        PUT(HDRP(PREV_BLKP(ptr)),PACK(len,0));
        ptr = PREV_BLKP(ptr);
    }
    return ptr;
}

static inline void *find_fit(size_t size){
    for(void* bp = heap_list_ptr; GET_SIZE(HDRP(bp))>0; bp = NEXT_BLKP(bp)){
        if(!GET_ALLOC(HDRP(bp)) && (size <= GET_SIZE(HDRP(bp)))){
            return bp;
        }
    }
    return NULL;
}

static inline void *extend_heap(size_t words){
    char *ptr;
    size_t size = (words % 2) ? (words + 1)* BASIC_WORD_SIZE : words * BASIC_WORD_SIZE;
    if((long)(ptr=mem_sbrk(size)) == (void *)-1)
        return NULL;

    PUT(HDRP(ptr),PACK(size,0));
    PUT(FTRP(ptr),PACK(size,0));
    PUT(HDRP(NEXT_BLKP(ptr)),PACK(0,1));

    return coalesce(ptr);
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t len, new_len;
    if(size == 0) return NULL;
    if(size <= BASIC_DWORD_SIZE){
        len = BASIC_DWORD_SIZE << 1;
    } else {
        len = (BASIC_DWORD_SIZE) * ((size + (BASIC_DWORD_SIZE << 1) - 1) / BASIC_DWORD_SIZE);
    }

    char* ptr = find_fit(len);
    if(ptr != NULL){
        place(ptr, len);
        return ptr;
    }

    new_len = len > (1 << 12) ? len : (1 << 12);
    ptr = extend_heap(new_len >> 2);
    if(ptr == NULL) return NULL;
    place(ptr, len);
    return ptr;
    
}

static inline void place(void *ptr,size_t asize){
    size_t ptr_size = GET_SIZE(HDRP(ptr));
    if((ptr_size-asize)>=(BASIC_DWORD_SIZE << 1)){
        PUT(HDRP(ptr),PACK(asize,1));
        PUT(FTRP(ptr),PACK(asize,1));
        ptr = NEXT_BLKP(ptr);
        PUT(HDRP(ptr),PACK(ptr_size-asize,0));
        PUT(FTRP(ptr),PACK(ptr_size-asize,0));
    }else{
        PUT(HDRP(ptr),PACK(ptr_size,1));
        PUT(FTRP(ptr),PACK(ptr_size,1));
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
inline void mm_free(void *ptr)
{
    if(ptr == NULL) return;
    size_t len = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(len, 0));
    PUT(FTRP(ptr), PACK(len, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
inline void *mm_realloc(void *ptr, size_t size)
{
    size_t oldsize; void* nptr;
    if(size == 0){
        mm_free(ptr);
        return 0;
    }
    if(ptr == NULL) return mm_malloc(size);
    nptr = mm_malloc(size);
    if(!nptr) return 0;
    oldsize = GET_SIZE(HDRP(ptr));;
    if(size < oldsize) oldsize = size;
    memcpy(nptr, ptr, oldsize);
    mm_free(ptr);
    return nptr;
}