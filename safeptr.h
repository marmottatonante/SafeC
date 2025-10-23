#ifndef SAFEPTR_H
#define SAFEPTR_H

#include <stddef.h>
#include <stdbool.h>
#include <threads.h>

enum safeptr_state_e
{
    SAFEPTR_STATE_UNALLOCATED = 0, // Memory has not been allocated. Invalid pointer.
    SAFEPTR_STATE_UNINITIALIZED, // Memory has not been initialized and may contain garbage.
    SAFEPTR_STATE_INITIALIZED, // Memory has been initialized and pointer is readable.
};

typedef enum safeptr_state_e safeptr_state;

struct safeptr_s
{
    mtx_t lock;

    void* data;
    size_t size;
    safeptr_state state;
};

typedef struct safeptr_s safeptr;

enum safeptr_result_e
{
    SAFEPTR_SUCCESS = 0, // Operation succeded
    SAFEPTR_ERROR_INVALID_ARGUMENT, // An invalid argument was passed to the function
    SAFEPTR_ERROR_INVALID_STATE, // Pointer is in an invalid state for the operation
    SAFEPTR_ERROR_MUTEX_INIT_FAIL, // Initializing the mutex for thread safety failed
    SAFEPTR_ERROR_ALLOCATION_FAIL, // Memory allocation failed, malloc returned NULL
    SAFEPTR_ERROR_SIZE_UNCHANGED, // Memory reallocation failed, but pointer is still valid
};

typedef enum safeptr_result_e safeptr_result;

safeptr_result safeptr_lock_acquire(safeptr* ptr);
safeptr_result safeptr_lock_release(safeptr *ptr);

safeptr_result safeptr_unthreaded_alloc(safeptr* ptr, const size_t size);
safeptr_result safeptr_unthreaded_realloc(safeptr *ptr, const size_t new_size);
safeptr_result safeptr_unthreaded_get(const safeptr* ptr, void* out_data);
safeptr_result safeptr_unthreaded_set(safeptr *ptr, const void* data);
safeptr_result safeptr_unthreaded_free(safeptr* ptr);

safeptr_result safeptr_create(safeptr* out_ptr);
safeptr_result safeptr_get(const safeptr* ptr, void* out_data);
safeptr_result safeptr_set(safeptr* ptr, const void* data);
safeptr_result safeptr_destroy(safeptr* out_ptr);

#endif