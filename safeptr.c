#include "safeptr.h"
#include <stdlib.h>
#include <string.h>

static const safeptr DEFAULT_SAFEPTR =
{
    .data = NULL,
    .size = 0
};

safeptr_result safeptr_alloc(safeptr* p, void* data, size_t size)
{
    if(p == NULL || data == NULL || size == 0)
        return SAFEPTR_ERROR_INVALID_ARGUMENT;
    
    *p = DEFAULT_SAFEPTR;

    void* temp_data = malloc(size);
    if(temp_data == NULL)
        return SAFEPTR_ERROR_ALLOC_FAIL;

    p->data = temp_data;
    p->size = size;

    memcpy(p->data, data, p->size);

    return SAFEPTR_SUCCESS;
}

safeptr_result safeptr_realloc(safeptr *p, size_t new_size)
{
    if(p == NULL || new_size == 0)
        return SAFEPTR_ERROR_INVALID_ARGUMENT;
    
    void* new_data = realloc(p->data, new_size);
    if(new_data == NULL)
        return SAFEPTR_ERROR_REALLOC_FAIL;
    
    p->data = new_data;
    p->size = new_size;

    return SAFEPTR_SUCCESS;
}

safeptr_result safeptr_free(safeptr* p)
{
    free(p->data);
    *p = DEFAULT_SAFEPTR;
    return SAFEPTR_SUCCESS;
}