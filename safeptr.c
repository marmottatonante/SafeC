#include "safeptr.h"
#include <stdlib.h>
#include <string.h>
#include <threads.h>

/******************** MANUAL **********************/

safec_result safeptr_manual_lock(safeptr* ptr)
{
    if(ptr == NULL)
        return SAFEC_ERROR_INVALID_ARGUMENT;
    
    if(ptr->state == SAFEPTR_STATE_DESTROYED)
        return SAFEC_ERROR_INVALID_STATE;
    
    if(mtx_lock(&ptr->lock) != thrd_success)
        return SAFEC_ERROR_CONCURRENCY_FAIL;

    return SAFEC_SUCCESS;
}

safec_result safeptr_manual_unlock(safeptr *ptr)
{
    if(ptr == NULL)
        return SAFEC_ERROR_INVALID_ARGUMENT;
    
    if(ptr->state == SAFEPTR_STATE_DESTROYED)
        return SAFEC_ERROR_INVALID_STATE;
    
    if(mtx_unlock(&ptr->lock) != thrd_success)
        return SAFEC_ERROR_CONCURRENCY_FAIL;
    
    return SAFEC_SUCCESS;
}

safec_result safeptr_manual_alloc(safeptr* ptr, const size_t size)
{
    if(ptr == NULL || size == 0)
        return SAFEC_ERROR_INVALID_ARGUMENT;

    if(ptr->state != SAFEPTR_STATE_UNALLOCATED)
        return SAFEC_ERROR_INVALID_STATE;
    
    void* data = malloc(size);
    if(data == NULL)
        return SAFEC_ERROR_ALLOCATION_FAILED;

    ptr->data = data;
    ptr->size = size;
    ptr->state = SAFEPTR_STATE_UNINITIALIZED;
        
    return SAFEC_SUCCESS;
}

safec_result safeptr_manual_realloc(safeptr *ptr, const size_t new_size)
{
    if(ptr == NULL)
        return SAFEC_ERROR_INVALID_ARGUMENT;
        
    if(ptr->state == SAFEPTR_STATE_DESTROYED)
        return SAFEC_ERROR_INVALID_STATE;
    
    if(ptr->state == SAFEPTR_STATE_UNALLOCATED)
        return SAFEC_ERROR_INVALID_STATE;
    
    if(new_size == 0)
        return safeptr_manual_free(ptr);

    void* new_data = realloc(ptr->data, new_size);
    if(new_data == NULL)
        return SAFEC_ERROR_ALLOCATION_FAILED;
        
    size_t old_size = ptr->size;
    ptr->data = new_data;
    ptr->size = new_size;
    if(new_size > old_size)
        ptr->state = SAFEPTR_STATE_UNINITIALIZED;

    return SAFEC_SUCCESS;
}

safec_result safeptr_manual_free(safeptr* ptr)
{
    if(ptr == NULL)
        return SAFEC_ERROR_INVALID_ARGUMENT;
    
    if(ptr->state == SAFEPTR_STATE_UNALLOCATED)
        return SAFEC_ERROR_INVALID_STATE;
    
    if(ptr->state == SAFEPTR_STATE_DESTROYED)
        return SAFEC_ERROR_INVALID_STATE;

    free(ptr->data);
    ptr->data = NULL;
    ptr->size = 0;
    ptr->state = SAFEPTR_STATE_UNALLOCATED;

    return SAFEC_SUCCESS;
}

safec_result safeptr_manual_read(const safeptr* ptr, void* out_data)
{
    if(ptr == NULL)
        return SAFEC_ERROR_INVALID_ARGUMENT;
    
    if(ptr->state == SAFEPTR_STATE_UNALLOCATED)
        return SAFEC_ERROR_INVALID_STATE;
    
    if(ptr->state == SAFEPTR_STATE_DESTROYED)
        return SAFEC_ERROR_INVALID_STATE;
    
    if(ptr->state == SAFEPTR_STATE_UNINITIALIZED)
        return SAFEC_ERROR_INVALID_STATE;

    memcpy(out_data, ptr->data, ptr->size);
    
    return SAFEC_SUCCESS;
}

safec_result safeptr_manual_write(safeptr* ptr, const void* data)
{
    if(ptr == NULL)
        return SAFEC_ERROR_INVALID_ARGUMENT;
    
    if(ptr->state == SAFEPTR_STATE_UNALLOCATED)
        return SAFEC_ERROR_INVALID_STATE;
    
    if(ptr->state == SAFEPTR_STATE_DESTROYED)
        return SAFEC_ERROR_INVALID_STATE;
    
    memcpy(ptr->data, data, ptr->size);
    ptr->state = SAFEPTR_STATE_INITIALIZED;

    return SAFEC_SUCCESS;
}

/******************** PRIMARY **********************/

safec_result safeptr_create(safeptr* out_ptr)
{
    if(out_ptr == NULL)
        return SAFEC_ERROR_INVALID_ARGUMENT;
    
    memset(out_ptr, 0, sizeof(safeptr));
    if(mtx_init(&out_ptr->lock, mtx_plain) != thrd_success)
        return SAFEC_ERROR_CONCURRENCY_FAIL;
    
    return SAFEC_SUCCESS;
}

safec_result safeptr_destroy(safeptr* ptr)
{
    if(ptr == NULL)
        return SAFEC_ERROR_INVALID_ARGUMENT;
    
    safec_result lock_result = safeptr_manual_lock(ptr);
    if(lock_result != SAFEC_SUCCESS)
        return lock_result;

    safec_result free_result = safeptr_manual_free(ptr);
    safec_result unlock_result = safeptr_manual_unlock(ptr);
    if(unlock_result != SAFEC_SUCCESS)
        return unlock_result;
    if(free_result != SAFEC_SUCCESS)
        return free_result;

    mtx_destroy(&ptr->lock);
    memset(ptr, 0, sizeof(safeptr));
    ptr->state = SAFEPTR_STATE_DESTROYED;

    return SAFEC_SUCCESS;
}

safec_result safeptr_get(safeptr* ptr, void* out_data)
{
    safec_result lock_result = safeptr_manual_lock(ptr);
    if(lock_result != SAFEC_SUCCESS)
        return lock_result;
    
    safec_result read_result = safeptr_manual_read(ptr, out_data);
    safec_result unlock_result = safeptr_manual_unlock(ptr);
    if(unlock_result != SAFEC_SUCCESS)
        return unlock_result;
    if(read_result != SAFEC_SUCCESS)
        return read_result;
    
    return SAFEC_SUCCESS;
}

safec_result safeptr_set(safeptr* ptr, const void* new_data, const size_t new_size)
{
    safec_result operation_result;

    operation_result = safeptr_manual_lock(ptr);
    if(operation_result != SAFEC_SUCCESS)
        return operation_result;
    
    if(ptr->state == SAFEPTR_STATE_UNALLOCATED)
        operation_result = safeptr_manual_alloc(ptr, new_size);
    else if (ptr->size != new_size)
        operation_result = safeptr_manual_realloc(ptr, new_size);
    else
        operation_result = SAFEC_SUCCESS;
    
    if(operation_result != SAFEC_SUCCESS)
    {
        safeptr_manual_unlock(ptr);
        return operation_result;
    }

    operation_result = safeptr_manual_write(ptr, new_data);
    safeptr_manual_unlock(ptr);

    return operation_result;
}