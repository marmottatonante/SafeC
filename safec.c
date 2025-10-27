#include "safec.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <threads.h>

/*
    safcod (Error Management)
*/

inline bool safcod_is_success(safcod code) {
    return code == SAFCOD_SUCCESS;
}
inline bool safcod_is_warning(safcod code) {
    return code > _SAFCOD_WARNING_START && code < _SAFCOD_WARNING_END;
}
inline bool safcod_is_error(safcod code) {
    return code > _SAFCOD_ERROR_START && code < _SAFCOD_ERROR_END;
}
inline bool safcod_is_ok(safcod code) {
    return safcod_is_success(code) || safcod_is_warning(code);
}

/*
    safsyn section
*/

safcod safsyn_create(safsyn* new_safsyn, bool is_enabled)
{
    if(new_safsyn == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;
    
    memset(new_safsyn, 0, sizeof(safsyn));
    new_safsyn->is_enabled = is_enabled;
    
    if(!new_safsyn->is_enabled) return SAFCOD_SUCCESS;
    new_safsyn->is_valid = mtx_init(&new_safsyn->lock, mtx_plain) == thrd_success;
    return new_safsyn->is_valid ? SAFCOD_SUCCESS : SAFCOD_ERROR_MUTEX_INIT_FAILED;
}

safcod safsyn_lock(safsyn* sync)
{
    if(sync == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;

    if(!sync->is_enabled) return SAFCOD_SUCCESS;
    if(!sync->is_valid) return SAFCOD_ERROR_MUTEX_IS_INVALID;

    if(mtx_lock(&sync->lock) != thrd_success)
        return SAFCOD_ERROR_MUTEX_LOCK_FAILED;
    else
        return SAFCOD_SUCCESS;
}

safcod safsyn_unlock(safsyn* sync)
{
    if(sync == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;

    if(!sync->is_enabled) return SAFCOD_SUCCESS;
    if(!sync->is_valid) return SAFCOD_ERROR_MUTEX_IS_INVALID;

    if(mtx_unlock(&sync->lock) != thrd_success)
        return SAFCOD_WARNING_MUTEX_UNLOCK_FAILED;
    else
        return SAFCOD_SUCCESS;
}

safcod safsyn_destroy(safsyn* sync)
{
    if(sync == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;

    if(sync->is_enabled && sync->is_valid)
        mtx_destroy(&sync->lock);

    memset(sync, 0, sizeof(safsyn));
    return SAFCOD_SUCCESS;
}

/*
    safmem section
*/

safcod safmem_create(safmem* new_safmem, const size_t size, const bool is_managed, const bool is_concurrent)
{
    if(new_safmem == NULL || size == 0) return SAFCOD_ERROR_INVALID_ARGUMENT;

    memset(new_safmem, 0, sizeof(safmem));
    new_safmem->is_managed = is_managed;
    
    new_safmem->size = size;
    new_safmem->data = malloc(new_safmem->size);
    if(new_safmem->data == NULL) return SAFCOD_ERROR_MALLOC_FAILED;
    
    safcod sync_result = safsyn_create(&new_safmem->sync, is_concurrent);
    if(safcod_is_error(sync_result)) return free(new_safmem->data), new_safmem->data = NULL, sync_result;

    return SAFCOD_SUCCESS;
}

safcod safmem_destroy(safmem* mem)
{
    if(mem == NULL) return SAFCOD_ERROR_INVALID_ARGUMENT;

    safcod lock_result = safsyn_lock(&mem->sync);
    if(safcod_is_error(lock_result)) return lock_result;

    free(mem->data);
    mem->data = NULL;

    safcod unlock_result = safsyn_unlock(&mem->sync);

    safsyn_destroy(&mem->sync);
    memset(mem, 0, sizeof(safmem));

    return unlock_result;
}