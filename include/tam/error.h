#ifndef TAM_ERROR_H__
#define TAM_ERROR_H__

#include <tam/tam.h>

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

static const char *errorMessage(TamError Err) {
    switch (Err) {
    case OK:
        return "";
    case ErrFileNotFound:
        return "input file not found";
    case ErrFileLength:
        return "input file was too long or contained incomplete instructions";
    case ErrFileRead:
        return "there was a problem while reading the input file";
    case ErrCodeAccessViolation:
        return "code access violation";
    case ErrDataAccessViolation:
        return "data access violation";
    case ErrStackOverflow:
        return "stack overflow";
    case ErrStackUnderflow:
        return "stack underflow";
    }
}

#endif
