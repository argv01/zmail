#ifndef INCLUDE_SHELL_EXECUTE_H
#define INCLUDE_SHELL_EXECUTE_H


#include "osconfig.h"
#include <general.h>

int execute P((char * const *argv));
/* account for terminated child processes */
RETSIGTYPE sigchldcatcher P((int));


#endif /* !INCLUDE_SHELL_EXECUTE_H */
