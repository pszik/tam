#ifndef TAM_ERROR_H__
#define TAM_ERROR_H__

typedef enum TamError {
    OK,
    ErrFileNotFound,
    ErrFileLength,
    ErrFileRead,
    ErrCodeAccessViolation,
    ErrDataAccessViolation,
    ErrStackOverflow,
    ErrStackUnderflow,
} TamError;

#endif
