#ifndef SAFEC_H
#define SAFEC_H

#include <stddef.h>
#include <stdbool.h>
#include <threads.h>

/*
    safcod (Error Management)
*/

enum safcod_e
{
    /* internal use only */
    _SAFCOD_SUCCESS = 0,
    /* Operation completed successfully. */
    SAFCOD_SUCCESS_COMPLETED,
    /* Operation skipped as concurrency was disabled. */
    SAFCOD_SUCCESS_CONCURRENCY_DISABLED,

    /* internal use only */
    _SAFCOD_WARNING = 100,
    /* Operation completed anyway, but a sync failed to unlock. */
    SAFCOD_WARNING_SYNC_UNLOCK_FAILED,
    /* Operation completed anyway, but a sync failed to destroy. */
    SAFCOD_WARNING_SYNC_DESTROY_FAILED,
    /* Realloc failed to resize, but memory block is still valid. */
    SAFCOD_WARNING_REALLOC_FAILED,
    /* Buffer size was different from block size. Data was truncated. */
    SAFCOD_WARNING_TRUNCATED,
    
    /* internal use only */
    _SAFCOD_ERROR = 200,
    /* Argument passed to a function was invalid. */
    SAFCOD_ERROR_INVALID_ARGUMENT,
    /* Mutex failed to initialized. */
    SAFCOD_ERROR_MUTEX_INIT_FAILED,
    /* Mutex was in an invalid state. */
    SAFCOD_ERROR_MUTEX_IS_INVALID,
    /* Failed to acquire lock on mutex. */
    SAFCOD_ERROR_MUTEX_LOCK_FAILED,
    /* Failed to release lock on mutex. */
    SAFCOD_ERROR_MUTEX_UNLOCK_FAILED,
    /* Failed to allocate the memory block. */
    SAFCOD_ERROR_MALLOC_FAILED,
};

typedef enum safcod_e safcod;

bool safcod_ok(safcod code);
bool safcod_warn(safcod code);
bool safcod_err(safcod code);

bool safcod_ok_or_warn(safcod code);
bool safcod_err_or_warn(safcod code);

/*
    safsyn (Thread safty)
*/

struct safsyn_s
{
    mtx_t lock;
    bool is_valid;
    bool is_enabled;
};

typedef struct safsyn_s safsyn;

safcod safsyn_create(safsyn* sync, bool is_enabled);
safcod safsyn_lock(safsyn* sync);
safcod safsyn_unlock(safsyn* sync);
safcod safsyn_destroy(safsyn* sync);

/*
    safmem (Memory Block)
*/

struct safmem_s
{
    safsyn sync;

    void* data;
    size_t size;
    bool is_managed;
};

typedef struct safmem_s safmem;

safcod safmem_create(safmem* mem, const size_t size, const bool is_managed, const bool is_concurrent);
safcod safmem_destroy(safmem* mem);
safcod safmem_resize(safmem *mem, const size_t new_size);
safcod safmem_read(safmem* mem, void* buffer, const size_t buffer_size);
safcod safmem_write(safmem* mem, const void* buffer, const size_t buffer_size);

#endif