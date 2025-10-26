#ifndef SAFEC_H
#define SAFEC_H

#include <stddef.h>
#include <stdbool.h>
#include <threads.h>

/*
    safeerr (Error Management)
*/

enum safeerr_e
{
    SAFEERR_SUCCESS = 0, // Operation succeded

    SAFEERR_INVALID_ARGUMENT,

    SAFEERR_MUTEX_INIT_FAILED,
    SAFEERR_MUTEX_LOCK_FAILED,
    SAFEERR_MUTEX_UNLOCK_FAILED,
    SAFEERR_MUTEX_IS_INVALID,

    SAFEERR_MALLOC_FAILED,
    SAFEERR_REALLOC_FAILED,
};

typedef enum safeerr_e safeerr;

safeerr safeerr_read(void);

/*
    safesyn (Thread Safety)
*/

struct safesyn_s
{
    mtx_t lock;
    bool is_valid;
    bool is_enabled;
};

typedef struct safesyn_s safesyn;

bool safesyn_create(safesyn* sync, bool is_enabled);
bool safesyn_lock(safesyn* sync);
bool safesyn_unlock(safesyn* sync);
bool safesyn_destroy(safesyn* sync);

/*
    safemem (Safe Memory Block)
*/

struct safemem_s
{
    safesyn sync;

    void* data;
    size_t size;
    bool is_managed;
};

typedef struct safemem_s safemem;

bool safemem_create(safemem* new_safemem, const size_t size, const bool is_managed, const bool is_concurrent);
bool safemem_destroy(safemem* ptr);
bool safemem_resize(safemem *ptr, const size_t new_size);
bool safemem_read(safemem* ptr, void* out_data);
bool safemem_write(safemem* ptr, const void* new_data, const size_t new_size);

#endif