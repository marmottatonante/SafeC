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
    SAFCOD_SUCCESS = 0,

    _SAFCOD_WARNING_START, // internal
    SAFCOD_WARNING_MUTEX_UNLOCK_FAILED,
    SAFCOD_WARNING_REALLOC_FAILED,
    _SAFCOD_WARNING_END, // internal
    
    _SAFCOD_ERROR_START, // internal
    SAFCOD_ERROR_INVALID_ARGUMENT,
    SAFCOD_ERROR_MUTEX_INIT_FAILED,
    SAFCOD_ERROR_MUTEX_LOCK_FAILED,
    SAFCOD_ERROR_MUTEX_IS_INVALID,
    SAFCOD_ERROR_MALLOC_FAILED,
    _SAFCOD_ERROR_END, // internal
};

typedef enum safcod_e safcod;

inline bool safcod_is_success(safcod code);
inline bool safcod_is_warning(safcod code);
inline bool safcod_is_error(safcod code);
inline bool safcod_is_ok(safcod code);

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

safcod safmem_create(safmem* new_safmem, const size_t size, const bool is_managed, const bool is_concurrent);
safcod safmem_destroy(safmem* ptr);
safcod safmem_resize(safmem *ptr, const size_t new_size);
safcod safmem_read(safmem* ptr, void* out_data);
safcod safmem_write(safmem* ptr, const void* new_data, const size_t new_size);

#endif