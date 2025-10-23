#include "safeptr.h"
#include <stdlib.h>
#include <string.h>
#include <threads.h>

safeptr_result safeptr_unsafe_init(safeptr* out_ptr)
{
    if(out_ptr == NULL)
        return SAFEPTR_ERROR_INVALID_ARGUMENT;
    
    out_ptr->data = NULL;
    out_ptr->size = 0;
    out_ptr->state = SAFEPTR_STATE_UNALLOCATED;

    return SAFEPTR_SUCCESS;
}

safeptr_result safeptr_unsafe_lock(safeptr* ptr);
safeptr_result safeptr_unsafe_unlock(safeptr *ptr);
safeptr_result safeptr_unsafe_alloc(safeptr* ptr, const size_t size);
safeptr_result safeptr_unsafe_realloc(safeptr *ptr, const size_t new_size);
safeptr_result safeptr_unsafe_get(const safeptr* ptr, void* out_data);
safeptr_result safeptr_unsafe_set(safeptr *ptr, const void* data);
safeptr_result safeptr_unsafe_free(safeptr* ptr);

safeptr_result safeptr_create(safeptr* out_ptr)
{
    if(out_ptr == NULL)
        return SAFEPTR_ERROR_INVALID_ARGUMENT;

    out_ptr->data = NULL;
    out_ptr->size = 0;
    out_ptr->state = SAFEPTR_STATE_UNALLOCATED;
    if(mtx_init(&out_ptr->lock, mtx_plain) != thrd_success)
        return SAFEPTR_ERROR_MUTEX_INIT_FAIL;

    return SAFEPTR_SUCCESS;
}

safeptr_result safeptr_alloc(safeptr* p, const size_t size)
{
    if(p == NULL || size == 0)
        return SAFEPTR_ERROR_INVALID_ARGUMENT;
    
    if(p->state != SAFEPTR_STATE_UNALLOCATED)
        return SAFEPTR_ERROR_INVALID_STATE;
    
    void* data = malloc(size);
    if(data == NULL)
        return SAFEPTR_ERROR_ALLOCATION_FAIL;

    p->state = SAFEPTR_STATE_UNINITIALIZED;
    p->data = data;
    p->size = size;

    return SAFEPTR_SUCCESS;
}

safeptr_result safeptr_free(safeptr* p)
{
    if(p == NULL)
        return SAFEPTR_ERROR_INVALID_ARGUMENT;
    
    if(p->state == SAFEPTR_STATE_UNALLOCATED)
        return SAFEPTR_ERROR_INVALID_STATE;

    free(p->data);
    safeptr_unsafe_init(p);

    return SAFEPTR_SUCCESS;
}

safeptr_result safeptr_realloc(safeptr *p, const size_t new_size)
{
    if(p == NULL)
        return SAFEPTR_ERROR_INVALID_ARGUMENT;
        
    if(p->state == SAFEPTR_STATE_UNALLOCATED)
        return SAFEPTR_ERROR_INVALID_STATE;
    
    if(new_size == 0)
        return safeptr_free(p);

    void* new_data = realloc(p->data, new_size);
    if(new_data == NULL)
        return SAFEPTR_ERROR_SIZE_UNCHANGED;
        
    size_t old_size = p->size;
    if(new_size > old_size)
        p->state = SAFEPTR_STATE_UNINITIALIZED;
    p->data = new_data;
    p->size = new_size;

    return SAFEPTR_SUCCESS;
}

safeptr_result safeptr_get(const safeptr* p, void* out_data)
{
    if(p == NULL || out_data == NULL)
        return SAFEPTR_ERROR_INVALID_ARGUMENT;
    
    if(p->state != SAFEPTR_STATE_INITIALIZED)
        return SAFEPTR_ERROR_INVALID_STATE;
    
    out_data = p->data;
    return SAFEPTR_SUCCESS;
}

safeptr_result safeptr_set(safeptr* p, const void* in_data)
{
    if(p == NULL || in_data == NULL || p->state == SAFEPTR_STATE_UNALLOCATED)
        return SAFEPTR_ERROR_INVALID_ARGUMENT;
    
    if(p->state == SAFEPTR_STATE_UNALLOCATED)
        return SAFEPTR_ERROR_INVALID_STATE;
    
    memcpy(p->data, in_data, p->size);
    p->state = SAFEPTR_STATE_INITIALIZED;

    return SAFEPTR_SUCCESS;
}