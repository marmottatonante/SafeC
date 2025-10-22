#include "safeptr.h"
#include <stdlib.h>
#include <string.h>

static const safeptr DEFAULT_SAFEPTR =
{
    .state = SAFEPTR_STATE_UNALLOCATED,
    .data = NULL,
    .size = 0,
};

safeptr safeptr_create()
{
    return DEFAULT_SAFEPTR;
}

safeptr_result safeptr_alloc(safeptr* p, size_t size)
{
    if(p == NULL || size == 0)
        return SAFEPTR_ERROR_INVALID_ARGUMENT;
    
    if(p->state != SAFEPTR_STATE_UNALLOCATED)
        return SAFEPTR_ERROR_INVALID_STATE;
    
    void* data = malloc(size);
    if(data == NULL)
        return SAFEPTR_ERROR_ALLOCATION_FAILED;

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
    *p = DEFAULT_SAFEPTR;

    return SAFEPTR_SUCCESS;
}

safeptr_result safeptr_realloc(safeptr *p, size_t new_size)
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

safeptr_result safeptr_copy(safeptr* p, void* data)
{
    if(p == NULL || data == NULL)
        return SAFEPTR_ERROR_INVALID_ARGUMENT;
    
    if(p->state == SAFEPTR_STATE_UNALLOCATED)
        return SAFEPTR_ERROR_INVALID_STATE;
    
    memcpy(p->data, data, p->size);
    p->state = SAFEPTR_STATE_INITIALIZED;
    
    return SAFEPTR_SUCCESS;
}

bool safeptr_is_initialized(safeptr* p)
{
    return p != NULL && p->state == SAFEPTR_STATE_INITIALIZED;
}