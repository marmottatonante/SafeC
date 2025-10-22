#ifndef SAFEPTR_H
#define SAFEPTR_H

#include <stddef.h>
#include <stdbool.h>

enum safeptr_state_e
{
    SAFEPTR_STATE_UNALLOCATED = 0, // Memory has not been allocated. Invalid pointer.
    SAFEPTR_STATE_UNINITIALIZED, // Memory has not been initialized and may contain garbage.
    SAFEPTR_STATE_INITIALIZED, // Memory has been initialized and pointer is readable.
};

typedef enum safeptr_state_e safeptr_state;

struct safeptr_s
{
    safeptr_state state;
    void* data;
    size_t size;
};

typedef struct safeptr_s safeptr;

enum safeptr_result_e
{
    SAFEPTR_SUCCESS = 0, // Operation succeded
    SAFEPTR_ERROR_INVALID_ARGUMENT, // An invalid argument was passed to the function
    SAFEPTR_ERROR_INVALID_STATE, // Pointer is in an invalid state for the operation
    SAFEPTR_ERROR_ALLOCATION_FAILED, // Memory allocation failed 
    SAFEPTR_ERROR_SIZE_UNCHANGED, // Memory reallocation failed, but pointer is still valid
};

typedef enum safeptr_result_e safeptr_result;

safeptr safeptr_create();
safeptr_result safeptr_alloc(safeptr* p, size_t size);
safeptr_result safeptr_free(safeptr* p);
safeptr_result safeptr_realloc(safeptr *p, size_t new_size);
safeptr_result safeptr_copy(safeptr* p, void* data);
bool safeptr_is_initialized(safeptr* p);

#endif