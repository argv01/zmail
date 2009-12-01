#ifndef INCLUDE_SHELL_LPR_H
#define INCLUDE_SHELL_LPR_H

#include <general.h>

struct mgroup;
struct printdata;
struct zmPager;

int zm_lpr P((int argc, char **argv, struct mgroup *list));
char *printer_choose_one P((const char *allPrinters));
struct zmPager *printer_setup P((const struct printdata *pdata,
				 int use_same_printer));

#endif /* INCLUDE_SHELL_LPR_H */
