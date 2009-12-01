#include "dynstr.h"
#include "glist.h"
#include "mime-api.h"
#include "reassver.h"

void
add_version_header(struct glist *headers)
{
    struct mime_pair *mp;

    glist_Add(headers, (VPTR)0);
    mp = (struct mime_pair *)glist_Last(headers);
    dynstr_Init(&mp->name);
    dynstr_Set(&mp->name, REASSEMBLER_VERSION_HEADER);
    dynstr_Init(&mp->value);
    dynstr_Set(&mp->value, " ");
    dynstr_Append(&mp->value, REASSEMBLER_VERSION_STRING);
    dynstr_AppendChar(&mp->value, '\n');
}

