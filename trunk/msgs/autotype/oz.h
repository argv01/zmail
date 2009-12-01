#pragma once
#ifdef __cplusplus
extern "C" {
#endif /* C++ */
#include <stdio.h>

extern const char * const IrixTypeParam;

struct Attach;

extern void parse_attach_irix2mime(int, char **, int);
extern void parse_attach_mime2irix(int, char **, int);
extern void free_attach_oz();
extern void save_attach_oz(FILE * const);

extern const char *autotype_via_oz(const char *);
#ifdef __cplusplus
}

extern const char *guess_desk_type(const struct Attach &);
#endif /* C++ */
