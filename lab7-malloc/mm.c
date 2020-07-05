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

inline void put_val(void* ptr, unsigned int p)
{
    *(unsigned int *) ptr = p;
}

inline unsigned int get_val(void* ptr)
{
    return *(unsigned int *) ptr;
}

inline void* get_array_elem_ptr(void* base_ptr, int offset)
{
    return base_ptr + offset;
}

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
    if((heap_list_ptr = mem_sbrk(HEADER_WRITE_SIZE << 2)) == (void*) -1) return -1;
    int idx = 0;
    for(; idx < MAX_HEAP; ++idx) heap_block[idx] = NULL;
    put_val(get_array_elem_ptr(heap_list_ptr, 0), 0); // padding
    put_val(get_array_elem_ptr(heap_list_ptr, HEADER_WRITE_SIZE), 9);
    put_val(get_array_elem_ptr(heap_list_ptr, HEADER_WRITE_SIZE << 1), 9);
    put_val(get_array_elem_ptr(heap_list_ptr, (HEADER_WRITE_SIZE << 1) + HEADER_WRITE_SIZE), 1);
    heap_list_ptr = heap_list_ptr + (HEADER_WRITE_SIZE << 1);

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
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














