#include "safec.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <threads.h>

/*
    safcod (Error Management)
*/

// Returns whether the code is strictly SUCCESS.
inline bool safcod_ok(safcod code) {
    return code > _SAFCOD_SUCCESS && code < _SAFCOD_WARNING;
}

// Returns whether the code is strictly WARNING.
inline bool safcod_warn(safcod code) {
    return code > _SAFCOD_WARNING && code < _SAFCOD_ERROR;
}

// Returns whether the code is strictly ERROR.
inline bool safcod_err(safcod code) {
    return code > _SAFCOD_ERROR;
}

// Returns whether the code is either SUCCESS or WARNING.
inline bool safcod_ok_or_warn(safcod code) {
    return safcod_ok(code) || safcod_warn(code);
}

// Returns whether the code is either ERROR or WARNING.
inline bool safcod_err_or_warn(safcod code) {
    return safcod_err(code) || safcod_warn(code);
}

/*
    safsyn section
*/

safcod safsyn_create(safsyn* sync, bool is_enabled)
{
    if(sync == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;

    memset(sync, 0, sizeof(safsyn));
    sync->is_enabled = is_enabled;

    if(!sync->is_enabled) return SAFCOD_SUCCESS_CONCURRENCY_DISABLED;

    sync->is_valid = mtx_init(&sync->lock, mtx_plain) == thrd_success;
    if(!sync->is_valid) return SAFCOD_ERROR_MUTEX_INIT_FAILED;
    
    return SAFCOD_SUCCESS_COMPLETED;
}

safcod safsyn_lock(safsyn* sync)
{
    if(sync == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;

    if(!sync->is_enabled) return SAFCOD_SUCCESS_CONCURRENCY_DISABLED;
    if(!sync->is_valid) return SAFCOD_ERROR_MUTEX_IS_INVALID;

    if(mtx_lock(&sync->lock) != thrd_success)
        return SAFCOD_ERROR_MUTEX_LOCK_FAILED;

    return SAFCOD_SUCCESS_COMPLETED;
}

safcod safsyn_unlock(safsyn* sync)
{
    if(sync == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;

    if(!sync->is_enabled) return SAFCOD_SUCCESS_CONCURRENCY_DISABLED;
    if(!sync->is_valid) return SAFCOD_ERROR_MUTEX_IS_INVALID;

    if(mtx_unlock(&sync->lock) != thrd_success)
        return SAFCOD_ERROR_MUTEX_UNLOCK_FAILED;

    return SAFCOD_SUCCESS_COMPLETED;
}

safcod safsyn_destroy(safsyn* sync)
{
    if(sync == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;

    if(sync->is_enabled && sync->is_valid)
        mtx_destroy(&sync->lock);

    /* Zeroes the struct, setting is_valid to 0/false. */
    memset(sync, 0, sizeof(safsyn));

    return SAFCOD_SUCCESS_COMPLETED;
}

/*
    safmem section
*/

safcod safmem_create(safmem* mem, const size_t size, const bool is_managed, const bool is_concurrent)
{
    if(mem == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;

    memset(mem, 0, sizeof(safmem));
    mem->is_managed = is_managed;
    
    mem->size = size;
    mem->data = malloc(mem->size);
    if(mem->data == NULL) return SAFCOD_ERROR_MALLOC_FAILED;
    
    const safcod SAFCOD_SYNC = safsyn_create(&mem->sync, is_concurrent);
    if(safcod_err(SAFCOD_SYNC)) { free(mem->data); mem->data = NULL; return SAFCOD_SYNC; }

    return SAFCOD_SUCCESS_COMPLETED;
}

safcod safmem_destroy(safmem* mem)
{
    if(mem == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;

    const safcod SAFCOD_LOCK = safsyn_lock(&mem->sync);
    if(safcod_err(SAFCOD_LOCK)) return SAFCOD_LOCK;

    free(mem->data);
    mem->data = NULL;

    const safcod SAFCOD_UNLOCK = safsyn_unlock(&mem->sync);
    // There's no point in returning for unlock failure.
    // Free has already been called, so it's best to keep destroying.

    const safcod SAFCOD_DESTROY = safsyn_destroy(&mem->sync);
    memset(mem, 0, sizeof(safmem)); 

    if(safcod_err(SAFCOD_UNLOCK)) return SAFCOD_WARNING_SYNC_UNLOCK_FAILED;
    if(safcod_err(SAFCOD_DESTROY)) return SAFCOD_WARNING_SYNC_DESTROY_FAILED;
    
    return SAFCOD_SUCCESS_COMPLETED;
}

safcod safmem_resize(safmem *mem, const size_t new_size)
{
    if(mem == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;
    
    if(new_size == 0) return safmem_destroy(mem);

    const safcod SAFCOD_LOCK = safsyn_lock(&mem->sync);
    if(safcod_err(SAFCOD_LOCK)) return SAFCOD_LOCK;

    void* new_data = realloc(mem->data, new_size);
    if(new_data == NULL) { safsyn_unlock(&mem->sync); return SAFCOD_WARNING_REALLOC_FAILED; }

    mem->data = new_data;
    mem->size = new_size;

    const safcod SAFCOD_UNLOCK = safsyn_unlock(&mem->sync);
    if(safcod_err(SAFCOD_UNLOCK)) return SAFCOD_WARNING_SYNC_UNLOCK_FAILED;

    return SAFCOD_SUCCESS_COMPLETED;
}

safcod safmem_read(safmem* mem, void* buffer, const size_t buffer_size)
{
    if(mem == NULL || buffer == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;

    const safcod SAFCOD_LOCK = safsyn_lock(&mem->sync);
    if(safcod_err(SAFCOD_LOCK)) return SAFCOD_LOCK;

    size_t least_size = buffer_size > mem->size ? mem->size : buffer_size;
    memcpy(buffer, mem->data, least_size);

    const safcod SAFCOD_UNLOCK = safsyn_unlock(&mem->sync);
    if(safcod_err(SAFCOD_UNLOCK)) return SAFCOD_WARNING_SYNC_UNLOCK_FAILED;

    if(mem->size != buffer_size) return SAFCOD_WARNING_TRUNCATED;

    return SAFCOD_SUCCESS_COMPLETED;
}

safcod safmem_write(safmem* mem, const void* buffer, const size_t buffer_size)
{
    if(mem == NULL || buffer == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;

    const safcod SAFCOD_LOCK = safsyn_lock(&mem->sync);
    if(safcod_err(SAFCOD_LOCK)) return SAFCOD_LOCK;

    size_t least_size = buffer_size > mem->size ? mem->size : buffer_size;
    memcpy(mem->data, buffer, least_size);

    const safcod SAFCOD_UNLOCK = safsyn_unlock(&mem->sync);
    if(safcod_err(SAFCOD_UNLOCK)) return SAFCOD_WARNING_SYNC_UNLOCK_FAILED;

    if(mem->size != buffer_size) return SAFCOD_WARNING_TRUNCATED;

    return SAFCOD_SUCCESS_COMPLETED;
}