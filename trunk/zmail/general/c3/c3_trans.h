#ifndef _C3_TRANS_H_
#define _C3_TRANS_H_

#include "mime.h"
extern int c3_is_known_to_c3 P(( mimeCharSet ));
extern mimeCharSet c3_ccs_num_list P((int pos));
extern int c3_cs_list_length P((void));
extern mimeCharSet c3_add_to_table P((const char *, const char *));
extern int IsKnownMimeCharSet P((mimeCharSet));
extern int IsKnownMimeCharSetName P((const char *, mimeCharSet *));
extern mimeCharSet c3_cs_list P((int));
extern const char * c3_nicename_from_cs P((mimeCharSet));
extern const char * c3_mimename_from_cs P((mimeCharSet));
extern int c3_translate P((char *, const char *, int , int *, int, mimeCharSet,
			mimeCharSet, int *));
#endif
