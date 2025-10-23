#ifndef SAFEPTR_H
#define SAFEPTR_H

#include <stddef.h>
#include <threads.h>
#include "safec.h"

enum safeptr_state_e
{
    SAFEPTR_STATE_UNALLOCATED = 0, // Memory has not been allocated. Invalid pointer.
    SAFEPTR_STATE_UNINITIALIZED, // Memory has not been initialized and may contain garbage.
    SAFEPTR_STATE_INITIALIZED, // Memory has been initialized and pointer is readable.
    SAFEPTR_STATE_DESTROYED, // Memory has been freed and mutex has been destroyed.
};

typedef enum safeptr_state_e safeptr_state;

struct safeptr_s
{
    void* data;
    size_t size;
    safeptr_state state;
    mtx_t lock;
};

typedef struct safeptr_s safeptr;


safec_result safeptr_manual_lock(safeptr* ptr);
safec_result safeptr_manual_unlock(safeptr *ptr);
safec_result safeptr_manual_alloc(safeptr* ptr, const size_t size);
safec_result safeptr_manual_realloc(safeptr *ptr, const size_t new_size);
safec_result safeptr_manual_read(const safeptr* ptr, void* out_data);
safec_result safeptr_manual_write(safeptr* ptr, const void* data);
safec_result safeptr_manual_free(safeptr* ptr);

safec_result safeptr_create(safeptr* out_ptr);
safec_result safeptr_destroy(safeptr* ptr);
safec_result safeptr_get(const safeptr* ptr, void* out_data);
safec_result safeptr_set(safeptr* ptr, const void* new_data, const size_t new_size);

#endif