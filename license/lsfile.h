#ifndef INCLUDE_LICENSE_LSFILE_H
#define INCLUDE_LICENSE_LSFILE_H


#include <general.h>
#include <stdio.h>


struct license_data;

      struct license_data *ls_rdentry P((FILE *,       struct license_data *));
const struct license_data *ls_wrentry P((FILE *, const struct license_data *));


#endif /* !INCLUDE_LICENSE_LSFILE_H */
