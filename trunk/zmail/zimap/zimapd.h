/*
 * Copyright 1994 by Z-Code Software Corp., an NCD company.
 */

#ifndef ZYNCD_H
#define ZYNCD_H

#include "config.h"

#define ZYNC_VERSION "1.0dev"

#define DEBUG_ERRORS    1
#define DEBUG_WARNINGS  2
#define DEBUG_CONFIG    4
#define DEBUG_LIBRARY   8
#define DEBUG_EXEC      16
#define DEBUG_CCLIENT   32

#include <syslog.h>
#define ZYNC_ERROR LOG_ERR
#define ZYNC_WARNING LOG_WARNING
#define ZYNC_DEBUG LOG_DEBUG
#define ZYNC_SYSLOG_FACILITY LOG_LOCAL0

extern void zync_init();
extern void mm_dlog();

#endif /* ZYNCD_H */
