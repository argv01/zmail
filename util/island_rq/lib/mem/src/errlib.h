#ifndef _ERRLIB_H
#define _ERRLIB_H

#include "compiler.h"

typedef short		ERR_ID;
typedef unsigned long	ERR_NO;

typedef enum
{
    ERR_FAILURE = 0,
    ERR_SUCCESS = 1
} ERR_BOOL;

typedef enum
{
    ERR_IN_MEMORY,
    ERR_IN_FILE
} ERR_LOC;

Header(extern ERR_ID err_register, (char *module_name, ERR_LOC err_loc, char *err_string_path, unsigned short num_errors));
Header(extern ERR_BOOL err_push, (/*varargs*/));
Header(extern ERR_BOOL err_clear, (void));
Header(extern ERR_BOOL err_report, (ERR_BOOL (far * err_func) ()));
Header(extern ERR_NO err_last, (ERR_ID err_id));
Header(extern ERR_BOOL err_uninit, (void));

#endif /* _ERRLIB_H */
