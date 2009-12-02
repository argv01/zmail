/*
 * pop.h: Header file for the "pop.c" client POP3 protocol
 * implementation.
 */

#ifdef POP3_SUPPORT

#include <stdio.h>

#define GETLINE_MAX 1024	/* a pretty arbitrary value */

extern char pop_error[];
extern int pop_debug;

typedef struct _PopServer {
     int file, data;
     char buffer[GETLINE_MAX], *dp;
} *PopServer;

/*
 * Valid flags for the pop_open function.
 */

#define POP_NO_KERBEROS	(1<<0)
#define POP_NO_HESIOD	(1<<1)
#define POP_NO_GETPASS 	(1<<2)

#define POP_NO_OPEN	(1<<7)

extern PopServer pop_open();
extern int pop_stat();
extern int pop_list();
extern char *pop_retrieve();
extern int pop_delete();
extern int pop_noop();
extern int pop_last();
extern int pop_reset();
extern int pop_quit();
extern void pop_close();

#endif /* POP3_SUPPORT */
