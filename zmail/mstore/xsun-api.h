#include "mime-api.h"

struct xsun_stackelt {
    long length, lines;
    void (*cleanup) NP((VPTR));
    VPTR cleanup_data;
};

#define xsun_Header(D, X, Y, Z) mime_Header(D, X, Y, Z)
#define xsun_ContinueHeader(D, X, Y) mime_ContinueHeader(D, X, Y)

extern int xsun_Headers P((struct dpipe *, struct glist *, const char *,
			   long *, long *));
extern void xsun_AttachmentStart P((struct glist *, long, long,
				    void (*) NP((GENERIC_POINTER_TYPE *)),
				    GENERIC_POINTER_TYPE *));
extern void xsun_Unwind P((struct glist *, int));
extern int xsun_NextBoundary P((struct dpipe *, struct dpipe *,
				struct glist *, const char *));
extern char *xsun_ParseEncodingInfo P((char *, struct glist *));
extern void xsun_AnalyzeHeaders P((struct glist *, char **, char **,
				   long *, long *));
