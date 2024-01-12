//
// Created by macha on 03.11.2023.
//

#include "heap.h"

size_t SUM_CECK(struct memory_chunk_t *node) {
    size_t sum = 0;
    sum += node->size;
    sum += (size_t) node->next;
    sum += (size_t) node->prev;
    return sum;
}

void *block_init(char *wsk, size_t size) {
    wsk += sizeof(struct memory_chunk_t);
    for (size_t i = 0; i < FENCES_SIZE; ++i) {
        *(wsk + i) = FENCES;
    }
    wsk += FENCES_SIZE;
    char *return_pointer = wsk;
    wsk += size;
    for (size_t i = 0; i < FENCES_SIZE; ++i) {
        *(wsk + i) = FENCES;
    }
    return (void *) return_pointer;
}

int heap_setup(void) {
    void *mem = custom_sbrk(PAGE_SIZE);
    if (!mem) {
        return -1;
    }
    memory_manager.first_memory_chunk = NULL;
    memory_manager.memory_size = PAGE_SIZE;
    memory_manager.memory_start = mem;
    memory_manager.used_memory = 0;
    return 0;
}

void heap_clean(void) {
    custom_sbrk((intptr_t) memory_manager.memory_size * (-1));
    memory_manager.memory_size = 0;
    memory_manager.used_memory = 0;
    memory_manager.memory_start = NULL;
    memory_manager.first_memory_chunk = NULL;
}

size_t heap_get_largest_used_block_size(void) {
    if (heap_validate() != 0) {
        return 0;
    }
    size_t largest_size = 0;
    if (memory_manager.memory_start == NULL) {
        return 0;
    }
    struct memory_chunk_t *check = memory_manager.first_memory_chunk;
    while (check) {
        if (check->size > largest_size) {
            largest_size = check->size;
        }
        check = check->next;
    }
    return largest_size;
}

enum pointer_type_t get_pointer_type(const void *const pointer) {
    if (pointer == NULL) {
        return pointer_null;
    }
    if (heap_validate() != 0) {
        return pointer_heap_corrupted;
    }
    for (struct memory_chunk_t *start = memory_manager.first_memory_chunk; start != NULL; start = start->next) {
        char *move = (char *) start;
        for (size_t i = 0; i < sizeof(struct memory_chunk_t); i++) {
            if ((move + i) == ((char *) pointer)) {
                return pointer_control_block;
            }
        }
        move += sizeof(struct memory_chunk_t);
        for (size_t i = 0; i < FENCES_SIZE; i++) {
            if ((move + i) == ((char *) pointer)) {
                return pointer_inside_fences;
            }
        }
        move += FENCES_SIZE;
        for (size_t i = 0; i < start->size; i++) {
            if (i == 0 && (move + i) == (char *) pointer) {
                return pointer_valid;
            }
            if (move + i == (char *) pointer) {
                return pointer_inside_data_block;
            }
        }
        move += start->size;
        for (size_t i = 0; i < FENCES_SIZE; i++) {
            if ((move + i) == ((char *) pointer)) {
                return pointer_inside_fences;
            }
        }
    }
    return pointer_unallocated;
}

int heap_validate(void) {
    if (memory_manager.memory_start == NULL || memory_manager.memory_size == 0) {
        return 2;
    }
    for (struct memory_chunk_t *start = memory_manager.first_memory_chunk; start != NULL; start = start->next) {
        char *move = (char *) start;
        if (SUM_CECK(start) != start->sum_size) {
            return 3;
        }
        move += sizeof(struct memory_chunk_t);
        for (size_t i = 0; i < FENCES_SIZE; i++) {
            if (*(move + i) != FENCES) {
                return 1;
            }
        }
        move += FENCES_SIZE;
        move += start->size;
        for (size_t i = 0; i < FENCES_SIZE; i++) {
            if (*(move + i) != FENCES) {
                return 1;
            }
        }
    }
    return 0;
}


void *heap_malloc(size_t size) {
    if (size == 0 || heap_validate() != 0) {
        return NULL;
    }
    size_t BLOCK_SIZE = size + sizeof(struct memory_chunk_t) + 2 * FENCES_SIZE;

    struct memory_chunk_t *start = memory_manager.first_memory_chunk;
    if (start != NULL && memory_manager.memory_start != start) {
        char *wsk = (char *) memory_manager.memory_start;
        size_t free_memory = (char *) start - wsk;
        if (free_memory >= BLOCK_SIZE) {
            struct memory_chunk_t *new_head_node = (struct memory_chunk_t *) wsk;
            new_head_node->size = size;
            new_head_node->prev = NULL;
            new_head_node->next = memory_manager.first_memory_chunk;
            memory_manager.first_memory_chunk->prev = new_head_node;
            memory_manager.used_memory += BLOCK_SIZE;
            memory_manager.first_memory_chunk = new_head_node;
            void *return_pointer = block_init(wsk, size);
            new_head_node->sum_size = SUM_CECK(new_head_node);
            memory_manager.first_memory_chunk->next->sum_size = SUM_CECK(memory_manager.first_memory_chunk->next);
            return (void *) return_pointer;
        }

    }
    if (start == NULL) {
        if (memory_manager.memory_size < BLOCK_SIZE) {
            size_t new_p_size = BLOCK_SIZE / PAGE_SIZE;
            new_p_size++;
            void *ptr = custom_sbrk((intptr_t) new_p_size * PAGE_SIZE);
            if (ptr == ((void *) -1)) {
                return NULL;
            }
            memory_manager.memory_size += new_p_size * PAGE_SIZE;
        }
        char *wsk1 = (char *) memory_manager.memory_start;
        struct memory_chunk_t *head_node = (struct memory_chunk_t *) wsk1;
        head_node->size = size;
        head_node->prev = NULL;
        head_node->next = NULL;
        memory_manager.first_memory_chunk = head_node;
        memory_manager.used_memory += BLOCK_SIZE;
        void *return_pointer = block_init(wsk1, size);
        head_node->sum_size = SUM_CECK(head_node);
        return return_pointer;

    }
    for (; start->next != NULL; start = start->next) {
        size_t free_memory =
                (char *) start->next - ((char *) start + sizeof(struct memory_chunk_t) + start->size + 2 * FENCES_SIZE);
        if (free_memory >= BLOCK_SIZE) {
            char *wsk = (char *) start;
            wsk += sizeof(struct memory_chunk_t) + start->size + 2 * FENCES_SIZE;
            struct memory_chunk_t *new_node = (struct memory_chunk_t *) wsk;
            new_node->size = size;
            new_node->prev = start;
            new_node->next = new_node->prev->next;
            new_node->prev->next = new_node;
            new_node->next->prev = new_node;
            memory_manager.used_memory += BLOCK_SIZE;
            void *return_pointer = block_init(wsk, size);
            new_node->sum_size = SUM_CECK(new_node);
            new_node->next->sum_size = SUM_CECK(new_node->next);
            new_node->prev->sum_size = SUM_CECK(new_node->prev);
            return return_pointer;
        }
    }

    size_t bytes = (int8_t *) start - (int8_t *) memory_manager.memory_start + start->size + FENCES_SIZE * 2 +
                   sizeof(struct memory_chunk_t) + BLOCK_SIZE;
    if (bytes > memory_manager.memory_size) {
        size_t missing_bytes = bytes - (memory_manager.memory_size - memory_manager.used_memory);
        size_t new_p_size = missing_bytes / PAGE_SIZE;
        new_p_size++;
        void *ptr = custom_sbrk((intptr_t) new_p_size * PAGE_SIZE);
        if (ptr == ((void *) -1)) {
            return NULL;
        }
        memory_manager.memory_size += new_p_size * PAGE_SIZE;
    }

    char *wsk = (char *) start;
    wsk += (start->size + sizeof(struct memory_chunk_t) + 2 * FENCES_SIZE);
    struct memory_chunk_t *new_node = (struct memory_chunk_t *) wsk;
    new_node->size = size;
    new_node->next = NULL;
    start->next = new_node;
    new_node->prev = start;
    memory_manager.used_memory += BLOCK_SIZE;
    void *return_pointer = block_init(wsk, size);
    new_node->sum_size = SUM_CECK(new_node);
    start->sum_size = SUM_CECK(start);
    return return_pointer;
}

void *heap_calloc(size_t number, size_t size) {
    if (number < 1 || size < 1) {
        return NULL;
    }
    void *new_mem = heap_malloc(size * number);
    if (!new_mem) {
        return NULL;
    }
    char *wsk = (char *) new_mem;
    for (size_t i = 0; i < number * size; i++) {
        *(wsk + i) = 0;
    }
    return new_mem;
}

void *heap_realloc(void *memblock, size_t count) {
    if (memblock != NULL && count == 0) {
        heap_free(memblock);
        return NULL;
    }
    if (heap_validate() != 0 || count < 1) {
        return NULL;
    }
    if (!memblock) {
        return heap_malloc(count);
    }
    struct memory_chunk_t *chunk = memory_manager.first_memory_chunk;
    for (; chunk != NULL; chunk = chunk->next) {
        if ((void *) ((char *) chunk + sizeof(struct memory_chunk_t) + FENCES_SIZE) == memblock) {
            break;
        }
    }
    if (chunk == NULL) {
        return NULL;
    }
    if (chunk->size == count) {
        return memblock;
    }
    if (chunk->size > count) {
        chunk->size = count;
        chunk->sum_size = SUM_CECK(chunk);
        char *wsk = (char *) chunk;
        block_init(wsk, count);
        return memblock;
    }
    if (chunk->next == NULL) {
        size_t bytes = (int8_t *) chunk - (int8_t *) memory_manager.memory_start + chunk->size + 2 * FENCES_SIZE +
                       sizeof(struct memory_chunk_t);
        size_t free_space = memory_manager.memory_size - bytes;
        if (free_space < count - chunk->size) {
            size_t missing = count - chunk->size - free_space;
            size_t new_p_size = missing / PAGE_SIZE;
            new_p_size++;
            void *ptr = custom_sbrk((intptr_t) new_p_size * PAGE_SIZE);
            if (ptr == ((void *) -1)) {
                return NULL;
            }
            memory_manager.memory_size += new_p_size * PAGE_SIZE;
        } else {
            chunk->size = count;
            chunk->sum_size = SUM_CECK(chunk);
            char *wsk = (char *) chunk;
            block_init(wsk, count);
            return memblock;
        }
    }
    size_t free_space =
            (int8_t *) chunk->next - (int8_t *) chunk - chunk->size - 2 * FENCES_SIZE - sizeof(struct memory_chunk_t);
    if (free_space >= count - chunk->size) {
        chunk->size = count;
        chunk->sum_size = SUM_CECK(chunk);
        char *wsk = (char *) chunk;
        block_init(wsk, count);
        return memblock;
    }
    char *new_mem = heap_malloc(count);
    if (new_mem == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < chunk->size; ++i) {
        *(new_mem + i) = *((char *) memblock + i);
    }
    heap_free(memblock);
    chunk->sum_size = SUM_CECK(chunk);
    return new_mem;
}

void heap_free(void *memblock) {
    if (!memblock || heap_validate() != 0) {
        return;
    }

    if (get_pointer_type(memblock) != pointer_valid) {
        return;
    }

    struct memory_chunk_t *starter = memory_manager.first_memory_chunk;
    for (; starter != NULL; starter = starter->next) {
        if ((void *) ((char *) starter + sizeof(struct memory_chunk_t) + FENCES_SIZE) == memblock) {
            break;
        }
    }
    if (starter == NULL) {
        return;
    }
    memory_manager.used_memory -= (sizeof(struct memory_chunk_t) + starter->size + 2 * FENCES_SIZE);
    if (starter->next != NULL && starter->prev != NULL) {
        starter->next->prev = starter->prev;
        starter->prev->next = starter->next;
        starter->prev->sum_size = SUM_CECK(starter->prev);
        starter->next->sum_size = SUM_CECK(starter->next);
        return;
    }
    if (starter->next == NULL && starter->prev == NULL) {
        memory_manager.first_memory_chunk = NULL;
        return;
    }
    if (starter->next == NULL && starter->prev != NULL) {
        starter->prev->next = NULL;
        starter->prev->sum_size = SUM_CECK(starter->prev);
        return;
    }
    if (starter->prev == NULL && starter->next != NULL) {
        memory_manager.first_memory_chunk = starter->next;
        memory_manager.first_memory_chunk->prev = NULL;
        memory_manager.first_memory_chunk->sum_size = SUM_CECK(starter->next);
        return;
    }
}

void *heap_malloc_aligned(size_t count) {
    if (count == 0 || heap_validate() != 0) {
        return NULL;
    }
    struct memory_chunk_t *start = memory_manager.first_memory_chunk;
    if (start == NULL) {
        size_t new_p = (count + FENCES_SIZE) / PAGE_SIZE;
        new_p++;
        void *ptr = custom_sbrk((intptr_t) new_p * PAGE_SIZE);
        if (ptr == (void *) -1) {
            return NULL;
        }
        memory_manager.memory_size += new_p * PAGE_SIZE;
        char *wsk = (char *) memory_manager.memory_start + PAGE_SIZE;
        wsk -= FENCES_SIZE;
        wsk -= sizeof(struct memory_chunk_t);
        struct memory_chunk_t *head = (struct memory_chunk_t *) wsk;
        head->next = NULL;
        head->prev = NULL;
        head->size = count;
        memory_manager.first_memory_chunk = head;
        void *return_pointer = block_init(wsk, count);
        head->sum_size = SUM_CECK(head);
        return return_pointer;
    }
    for (; start->next != NULL; start = start->next) {
        char *p2 = (char *) start;
        char *p3 = (char *) start->next;
        p2 += sizeof(struct memory_chunk_t) + 2 * FENCES_SIZE + start->size;
        size_t struct_space = PAGE_SIZE - (((size_t) p2 % PAGE_SIZE) % PAGE_SIZE);
        p2 += struct_space;
        size_t block_space = count + FENCES_SIZE;
        if (struct_space < sizeof(struct memory_chunk_t) + FENCES_SIZE) {
            if (p2 + PAGE_SIZE >= p3) {
                continue;
            }
            p2 += PAGE_SIZE;
        }
        if (p2 + block_space >= p3) {
            continue;
        }
        p2 -= FENCES_SIZE;
        p2 -= sizeof(struct memory_chunk_t);
        struct memory_chunk_t *new = (struct memory_chunk_t *) p2;
        new->size = count;
        new->prev = start;
        new->next = new->prev->next;
        new->prev->next = new;
        new->next->prev = new;
        void *r = block_init(p2, count);
        new->next->sum_size = SUM_CECK(new->next);
        new->prev->sum_size = SUM_CECK(new->prev);
        new->sum_size = SUM_CECK(new);
        return r;
    }
    char *p1 = (char *) start;
    p1 += sizeof(struct memory_chunk_t) + 2 * FENCES_SIZE + start->size;
    size_t struct_space = PAGE_SIZE - (((size_t)p1 % PAGE_SIZE) % PAGE_SIZE);
    p1 += struct_space;
    size_t block_bytes = count + FENCES_SIZE;
    size_t memory_to_p1 = (p1 - (char *) memory_manager.memory_start);

    if (sizeof(struct memory_chunk_t) + FENCES_SIZE > struct_space) {
        if (memory_to_p1 + PAGE_SIZE > memory_manager.memory_size) {
            size_t new_p = 1;
            void *ptr = custom_sbrk((intptr_t) new_p * PAGE_SIZE);
            if (ptr == (void *) -1) {
                return NULL;
            }
            memory_manager.memory_size += new_p * PAGE_SIZE;
        }
        p1 += PAGE_SIZE;
        memory_to_p1 += PAGE_SIZE;
    }
    size_t space_available = memory_manager.memory_size - memory_to_p1;
    if (space_available < block_bytes) {
        size_t missing_bytes = block_bytes - space_available;
        size_t new_p = (missing_bytes / PAGE_SIZE);
        new_p++;
        void *ptr = custom_sbrk((intptr_t) new_p * PAGE_SIZE);
        if (ptr == (void *) -1) {
            return NULL;
        }
        memory_manager.memory_size += new_p * PAGE_SIZE;
    }
    p1 -= FENCES_SIZE;
    p1 -= sizeof(struct memory_chunk_t);
    struct memory_chunk_t *new_block = (struct memory_chunk_t *) p1;
    new_block->next = NULL;
    new_block->size = count;
    new_block->prev = start;
    start->next = new_block;
    void *r = block_init(p1, count);
    start->sum_size = SUM_CECK(start);
    new_block->sum_size = SUM_CECK(new_block);
    return r;
}

void *heap_calloc_aligned(size_t number, size_t size) {
    if (number < 1 || size < 1) {
        return NULL;
    }
    void *new_mem = heap_malloc_aligned(size * number);
    if (!new_mem) {
        return NULL;
    }
    char *wsk = (char *) new_mem;
    for (size_t i = 0; i < number * size; i++) {
        *(wsk + i) = 0;
    }
    return new_mem;
}

void *heap_realloc_aligned(void *memblock, size_t size) {
    if (memblock != NULL && size == 0) {
        heap_free(memblock);
        return NULL;
    }
    if (heap_validate() != 0 || size < 1) {
        return NULL;
    }
    if (!memblock) {
        return heap_malloc_aligned(size);
    }
    struct memory_chunk_t *chunk = memory_manager.first_memory_chunk;
    for (; chunk != NULL; chunk = chunk->next) {
        if ((void *) ((char *) chunk + sizeof(struct memory_chunk_t) + FENCES_SIZE) == memblock) {
            break;
        }
    }
    if (chunk == NULL) {
        return NULL;
    }
    if (chunk->size == size) {
        return memblock;
    }
    if (chunk->size > size) {
        chunk->size = size;
        chunk->sum_size = SUM_CECK(chunk);
        char *wsk = (char *) chunk;
        block_init(wsk, size);
        return memblock;
    }
    if (chunk->next == NULL) {
        size_t bytes = (int8_t *) chunk - (int8_t *) memory_manager.memory_start + chunk->size + 2 * FENCES_SIZE +
                       sizeof(struct memory_chunk_t);
        size_t free_space = memory_manager.memory_size - bytes;
        if (free_space < size - chunk->size) {
            size_t missing = size - chunk->size - free_space;
            size_t new_p_size = missing / PAGE_SIZE;
            new_p_size++;
            void *ptr = custom_sbrk((intptr_t) new_p_size * PAGE_SIZE);
            if (ptr == ((void *) -1)) {
                return NULL;
            }
            memory_manager.memory_size += new_p_size * PAGE_SIZE;
        } else {
            chunk->size = size;
            chunk->sum_size = SUM_CECK(chunk);
            char *wsk = (char *) chunk;
            block_init(wsk, size);
            return memblock;
        }
    }
    size_t free_space =
            (int8_t *) chunk->next - (int8_t *) chunk - chunk->size - 2 * FENCES_SIZE - sizeof(struct memory_chunk_t);
    if (free_space >= size - chunk->size) {
        chunk->size = size;
        chunk->sum_size = SUM_CECK(chunk);
        char *wsk = (char *) chunk;
        block_init(wsk, size);
        return memblock;
    }
    char *new_mem = heap_malloc_aligned(size);
    if (new_mem == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < chunk->size; ++i) {
        *(new_mem + i) = *((char *) memblock + i);
    }
    heap_free(memblock);
    chunk->sum_size = SUM_CECK(chunk);
    return new_mem;
}
