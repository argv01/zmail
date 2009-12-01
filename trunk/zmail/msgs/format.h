#ifndef INCLUDE_MSGS_FORMAT_H
#define INCLUDE_MSGS_FORMAT_H


#include <general.h>

/* format.c 29/09/94 17.04.50 */
char *format_unindent_lines P((char **lines, int plen));
int format_prefix_length P((char **lines, int all_sp_ok));
char *format_indent_lines P((char **lines, char *pfix));
char *format_fill_lines P((char **lines, int wrap));
char *format_pipe_str P((char *in, char *cmd));


#endif /* !INCLUDE_MSGS_FORMAT_H */
