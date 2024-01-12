//
// Created by macha on 03.11.2023.
//

#ifndef ALOKATOR_HEAP_H
#define ALOKATOR_HEAP_H
#include <stdio.h>
#include "custom_unistd.h"
#define PAGE_SIZE 4096
#define FENCES '$'
#define FENCES_SIZE 8
struct memory_manager_t
{
    void *memory_start;
    size_t memory_size;
    size_t used_memory;
    struct memory_chunk_t *first_memory_chunk;
}memory_manager;

struct memory_chunk_t
{
    struct memory_chunk_t* prev;
    struct memory_chunk_t* next;
    size_t size;
    size_t sum_size;
};
enum pointer_type_t
{
    pointer_null,
    pointer_heap_corrupted,
    pointer_control_block,
    pointer_inside_fences,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};
size_t SUM_CECK(struct memory_chunk_t*node);
enum pointer_type_t get_pointer_type(const void *const pointer);
int heap_validate(void);
size_t heap_get_largest_used_block_size(void);
int heap_setup(void);
void heap_clean(void);
void* heap_malloc(size_t size);
void* heap_calloc(size_t number, size_t size);
void* heap_realloc(void* memblock, size_t count);
void  heap_free(void* memblock);
void* heap_malloc_aligned(size_t count);
void* heap_calloc_aligned(size_t number, size_t size);
void* heap_realloc_aligned(void* memblock, size_t size);
#endif //ALOKATOR_HEAP_H
