#ifndef SAFEC_H
#define SAFEC_H

enum safec_result_e
{
    SAFEC_SUCCESS = 0, // Operation succeded

    SAFEC_ERROR_INVALID_ARGUMENT, // An invalid argument was passed to the function
    SAFEC_ERROR_INVALID_STATE, // Pointer is in an invalid state for the operation

    SAFEC_ERROR_CONCURRENCY_FAIL, // Initializing or locking/unlocking the mutex failed
    SAFEC_ERROR_ALLOCATION_FAILED, // Memory allocation failed, malloc returned NULL
};

typedef enum safec_result_e safec_result;

#endif