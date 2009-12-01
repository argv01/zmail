#include "osconfig.h"
#include <general.h>

const char *autotype_via_analysis P((const char *));

#ifdef MAC_OS
const char *autotype_via_mactype P((const char *));
#endif /* MAC_OS */
