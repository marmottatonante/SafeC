#ifndef SAFEPTR_H
#define SAFEPTR_H

#include <stddef.h>

struct safeptr_s
{
    void* data;
    size_t size;
};

typedef struct safeptr_s safeptr;

enum safeptr_result_e
{
    SAFEPTR_SUCCESS = 0, // Success, pointer is valid
    SAFEPTR_ERROR_INVALID_ARGUMENT, // Invalid argument, pointer is invalid
    SAFEPTR_ERROR_ALLOC_FAIL, // Alloc failed, pointer is invalid
    SAFEPTR_ERROR_REALLOC_FAIL // Realloc failed, but pointer is still valid
};

typedef enum safeptr_result_e safeptr_result;

safeptr_result safeptr_alloc(safeptr* p, void* data, size_t size);
safeptr_result safeptr_realloc(safeptr *p);
safeptr_result safeptr_free(safeptr* p);

#endif