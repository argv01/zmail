/* 
 * $RCSfile: mime-api.h,v $
 * $Revision: 1.11 $
 * $Date: 1996/01/30 06:06:11 $
 * $Author: spencer $
 */

#ifndef MIME_API_H
# define MIME_API_H

# include <general.h>
# include <dynstr.h>
# include <except.h>
# include <glist.h>
# include <dpipe.h>

struct mime_pair {
    struct dynstr name, value;
    int offset_adjust;
};

struct mime_stackelt {
    struct dynstr boundary;
    void (*cleanup) NP((VPTR));
    VPTR cleanup_data;
};

extern int mime_SpecialToken;
extern const char mime_LF[], mime_CR[], mime_CRLF[];

extern const char *mime_Readline P((struct dpipe *, struct dynstr *));
extern void mime_Header P((struct dpipe *,
			   struct dynstr *,
			   struct dynstr *,
			   const char *));
extern void mime_ContinueHeader P((struct dpipe *,
				   struct dynstr *,
				   const char *));
extern void mime_Unfold P((struct dynstr *, int));
extern int mime_Headers P((struct dpipe *,
			   struct glist *,
			   const char *));
extern int less_than_mime_Headers P((struct dpipe *,
				     struct glist *,
				     const char *));
extern char *rfc822_NextToken P((char *, char **));
extern char *mime_NextToken P((char *, char **, int));
extern void mime_MultipartStart P((struct glist *,
				   const char *,
				   void (*) NP((GENERIC_POINTER_TYPE *)),
				   GENERIC_POINTER_TYPE *));
extern void mime_Unwind P((struct glist *, int));
extern int mime_NextBoundary P((struct dpipe *,
				struct dpipe *,
				struct glist *,
				const char *));
extern char *mime_ParseContentType P((char *,
				      char **,
				      struct glist *));
extern char *mime_ParseContentDisposition P((char *,
					     struct glist *));
extern void mime_AnalyzeHeaders P((struct glist *,
				   struct glist **,
				   char **, char **, char **, char **));

extern void mime_pair_init P((struct mime_pair *));
extern void mime_pair_destroy P((struct mime_pair *));

extern void mime_GenMultipart P((struct dpipe *,
				 const char *,
				 struct glist *));

DECLARE_EXCEPTION(mime_err_Header);
DECLARE_EXCEPTION(mime_err_String);
DECLARE_EXCEPTION(mime_err_Comment);

#endif /* MIME_API_H */
