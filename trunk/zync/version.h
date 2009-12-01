/*
 * Copyright (c) 1993-1998 Z-Code Software Corp.
 */

#ifndef VERSION_H
#define VERSION_H

enum release_level {
  dev,
  alpha,
  beta,
  Beta,  /* groan */
  released
};

struct version {
  unsigned int majr;            /* Major release number */
  unsigned int minr;            /* Minor release number */
  enum release_level level;     /* Release level */
  unsigned int patch;           /* Patch level */
};

extern int parseVersion();
extern int versionCmp();
char *unparseVersion();

#endif /* VERSION_H */
