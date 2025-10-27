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
    SAFCOD_SUCCESS_COMPLETED,
    SAFCOD_SUCCESS_CONCURRENCY_DISABLED,

    /* internal use only */
    _SAFCOD_WARNING = 100,
    SAFCOD_WARNING_COMPLETED_BUT_UNLOCK_FAILED,
    SAFCOD_WARNING_COMPLETED_BUT_SYNC_DESTROY_FAILED,
    SAFCOD_WARNING_REALLOC_FAILED,
    
    /* internal use only */
    _SAFCOD_ERROR = 200,
    SAFCOD_ERROR_INVALID_ARGUMENT,
    SAFCOD_ERROR_MUTEX_INIT_FAILED,
    SAFCOD_ERROR_MUTEX_IS_INVALID,
    SAFCOD_ERROR_MUTEX_LOCK_FAILED,
    SAFCOD_ERROR_MUTEX_UNLOCK_FAILED,
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