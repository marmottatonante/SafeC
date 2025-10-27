#include "safec.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <threads.h>

/*
    safcod (Error Management)
*/

/* Returns whether the code is strictly SUCCESS. */
inline bool safcod_ok(safcod code) {
    return code > _SAFCOD_SUCCESS && code < _SAFCOD_WARNING;
}

/* Returns whether the code is strictly WARNING. */
inline bool safcod_warn(safcod code) {
    return code > _SAFCOD_WARNING && code < _SAFCOD_ERROR;
}

/* Returns whether the code is strictly ERROR. */
inline bool safcod_err(safcod code) {
    return code > _SAFCOD_ERROR;
}

/* Returns whether the code is either SUCCESS or WARNING. */
inline bool safcod_ok_or_warn(safcod code) {
    return safcod_ok(code) || safcod_warn(code);
}

/* Returns whether the code is either ERROR or WARNING. */
inline bool safcod_err_or_warn(safcod code) {
    return safcod_err(code) || safcod_warn(code);
}

/*
    safsyn section
*/

/* Creates a safsyn object wrapping a mutex. It can be disabled. */
safcod safsyn_create(safsyn* sync, bool is_enabled)
{
    if(sync == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;

    /* Zeroes the struct, flags are 0/false now. */
    memset(sync, 0, sizeof(safsyn));
    sync->is_enabled = is_enabled;

    /* Checks if safsyn has been disabled and immediately returns if it is. */
    if(!sync->is_enabled) return SAFCOD_SUCCESS_CONCURRENCY_DISABLED;

    /* Initializes the wrapped mtx_t. */
    sync->is_valid = mtx_init(&sync->lock, mtx_plain) == thrd_success;
    if(!sync->is_valid) return SAFCOD_ERROR_MUTEX_INIT_FAILED;
    
    return SAFCOD_SUCCESS_COMPLETED;
}

/* Acquires lock on a safsyn. It returns immediately if safsyn is disabled. */
safcod safsyn_lock(safsyn* sync)
{
    if(sync == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;

    /* Immediately skips and returns if safsyn is disabled.  */
    if(!sync->is_enabled) return SAFCOD_SUCCESS_CONCURRENCY_DISABLED;
    if(!sync->is_valid) return SAFCOD_ERROR_MUTEX_IS_INVALID;

    if(mtx_lock(&sync->lock) != thrd_success)
        return SAFCOD_ERROR_MUTEX_LOCK_FAILED;

    return SAFCOD_SUCCESS_COMPLETED;
}

/* Releases lock on a safsyn. It returns immediately if safsyn is disabled. */
safcod safsyn_unlock(safsyn* sync)
{
    if(sync == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;

    /* Immediately skips and returns if safsyn is disabled.  */
    if(!sync->is_enabled) return SAFCOD_SUCCESS_CONCURRENCY_DISABLED;
    if(!sync->is_valid) return SAFCOD_ERROR_MUTEX_IS_INVALID;

    if(mtx_unlock(&sync->lock) != thrd_success)
        return SAFCOD_ERROR_MUTEX_UNLOCK_FAILED;

    return SAFCOD_SUCCESS_COMPLETED;
}

/* Destroys a safsyn object. */
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

/* Creates a safmem object wrapping a memory block and offering safety features.
   is_managed flag registers the safmem into an internal registry for easy cleanup.
   is_concurrent flag makes the safmem acquire and release locks on every operation. */
safcod safmem_create(safmem* mem, const size_t size, const bool is_managed, const bool is_concurrent)
{
    if(mem == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;

    /* Zeroes the struct, flags are 0/false now. */
    memset(mem, 0, sizeof(safmem));
    mem->is_managed = is_managed;
    
    /* Tries to malloc the requested size and returns an error it it couldn't. */
    mem->size = size;
    mem->data = malloc(mem->size);
    if(mem->data == NULL) return SAFCOD_ERROR_MALLOC_FAILED;
    
    /* Tries to create a safsyn. If it fails, it cleans everything up and returns an error. */
    const safcod SAFCOD_SYNC = safsyn_create(&mem->sync, is_concurrent);
    if(safcod_err(SAFCOD_SYNC)) { free(mem->data); mem->data = NULL; return SAFCOD_SYNC; }

    return SAFCOD_SUCCESS_COMPLETED;
}

/* Destroys the safemem object. */
safcod safmem_destroy(safmem* mem)
{
    if(mem == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;

    /* Tries to acquire lock and immediately returns if it fails. */
    const safcod SAFCOD_LOCK = safsyn_lock(&mem->sync);
    if(safcod_err(SAFCOD_LOCK)) return SAFCOD_LOCK;

    /* Lock is acquired, memory is freeable.
       After this point, it would make no sense
       to stop the destruction process. */

    free(mem->data);
    mem->data = NULL;

    /* Releases the lock and stores the outcome for later. */
    const safcod SAFCOD_UNLOCK = safsyn_unlock(&mem->sync);

    /* Destroys the safsyn and stores the outcome for later. */
    const safcod SAFCOD_DESTROY = safsyn_destroy(&mem->sync);
    /* Zeroes the struct, for good hygiene.  */
    memset(mem, 0, sizeof(safmem)); 

    /* Reports completion, but with a warning if unlock or destroy failed. */
    if(safcod_err(SAFCOD_UNLOCK)) return SAFCOD_WARNING_SYNC_UNLOCK_FAILED;
    if(safcod_err(SAFCOD_DESTROY)) return SAFCOD_WARNING_SYNC_DESTROY_FAILED;
    
    return SAFCOD_SUCCESS_COMPLETED;
}

safcod safmem_resize(safmem *mem, const size_t new_size)
{
    if(mem == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;
    
    /* Resizing to 0 means calling destroy on the safmem. */
    if(new_size == 0) return safmem_destroy(mem);

    /* Tries to acquire lock and immediately returns if it fails. */
    const safcod SAFCOD_LOCK = safsyn_lock(&mem->sync);
    if(safcod_err(SAFCOD_LOCK)) return SAFCOD_LOCK;

    /* Tries to resize block.
       If it fails, it releases the lock and returns a warning.
       Warnings signal non-destruction, safmem is still usable. */
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

    /* Calculates the smaller size between the buffer's and the memory block's
       and uses it as the number of bytes to copy from the memory block to the buffer. */
    size_t least_size = buffer_size > mem->size ? mem->size : buffer_size;
    memcpy(buffer, mem->data, least_size);

    const safcod SAFCOD_UNLOCK = safsyn_unlock(&mem->sync);
    if(safcod_err(SAFCOD_UNLOCK)) return SAFCOD_WARNING_SYNC_UNLOCK_FAILED;

    /* Reports completion, but also informs truncation happened, if the two sizes mismatched. */
    if(mem->size != buffer_size) return SAFCOD_WARNING_TRUNCATED;

    return SAFCOD_SUCCESS_COMPLETED;
}

safcod safmem_write(safmem* mem, const void* buffer, const size_t buffer_size)
{
    if(mem == NULL || buffer == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;

    const safcod SAFCOD_LOCK = safsyn_lock(&mem->sync);
    if(safcod_err(SAFCOD_LOCK)) return SAFCOD_LOCK;

    /* Calculates the smaller size between the buffer's and the memory block's
       and uses it as the number of bytes to copy from the buffer to the memory block. */
    size_t least_size = buffer_size > mem->size ? mem->size : buffer_size;
    memcpy(mem->data, buffer, least_size);

    const safcod SAFCOD_UNLOCK = safsyn_unlock(&mem->sync);
    if(safcod_err(SAFCOD_UNLOCK)) return SAFCOD_WARNING_SYNC_UNLOCK_FAILED;

    /* Reports completion, but also informs truncation happened, if the two sizes mismatched. */
    if(mem->size != buffer_size) return SAFCOD_WARNING_TRUNCATED;

    return SAFCOD_SUCCESS_COMPLETED;
}