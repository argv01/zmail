/*
 * $RCSfile: dpipe.h,v $
 * $Revision: 2.22 $
 * $Date: 1995/07/14 04:11:50 $
 * $Author: schaefer $
 */

#ifndef DPIPE_H
# define DPIPE_H

# include <general.h>
# include <dlist.h>
# include <except.h>

struct dpipe {
    struct dlist segments;
    void (*writer) NP((struct dpipe *, GENERIC_POINTER_TYPE *));
    void (*reader) NP((struct dpipe *, GENERIC_POINTER_TYPE *));
    GENERIC_POINTER_TYPE *wrdata, *rddata;
    int closed, autoflush, readpending, ready;
    long rdcount, wrcount; 
};

struct dpipeline {
    struct dlist nodes;
    void (*writer) NP((struct dpipe *, GENERIC_POINTER_TYPE *));
    void (*reader) NP((struct dpipe *, GENERIC_POINTER_TYPE *));
    GENERIC_POINTER_TYPE *rddata, *wrdata;
    struct {
	struct {
	    int installed;
	    void (*fn) NP((struct dpipe *, GENERIC_POINTER_TYPE *));
	    GENERIC_POINTER_TYPE *data;
	} source, sink;
    } foreign;
};

typedef void (*dpipe_Callback_t) NP((struct dpipe *, GENERIC_POINTER_TYPE *));
typedef void (*dpipeline_Filter_t) NP((struct dpipe *, struct dpipe *,
				       GENERIC_POINTER_TYPE *));
typedef void (*dpipeline_Finalize_t) NP((dpipeline_Filter_t,
					 GENERIC_POINTER_TYPE *));

# define dpipe_wrdata(d) ((d)->wrdata)
# define dpipe_rddata(d) ((d)->rddata)

# define dpipe_Ready(d) ((d)->ready)

# define dpipeline_wrdata(d) ((d)->wrdata)
# define dpipeline_rddata(d) ((d)->rddata)

# define dpipeline_Length(d) (dlist_Length(&((d)->nodes)))

extern void dpipe_Init    P((struct dpipe *,
			     dpipe_Callback_t,
			     GENERIC_POINTER_TYPE *,
			     dpipe_Callback_t,
			     GENERIC_POINTER_TYPE *,
			     int));
extern int  dpipe_Read      P((struct dpipe *, char *, int));
extern void dpipe_Write     P((struct dpipe *, const char *, int));
extern void dpipe_Flush     P((struct dpipe *));
extern int  dpipe_Eof       P((struct dpipe *));
extern int  dpipe_StrictEof P((struct dpipe *));
extern void dpipe_Close     P((struct dpipe *));
extern void dpipe_Destroy   P((struct dpipe *));
extern void dpipe_Unread    P((struct dpipe *, const char *, int));

extern int dpipe_Getchar    P((struct dpipe *));
extern void dpipe_Putchar   P((struct dpipe *, int));
extern void dpipe_Ungetchar P((struct dpipe *, int));
extern int dpipe_Peekchar   P((struct dpipe *));

extern void dpipe_Pump      P((struct dpipe *));

extern void dpipe_Put       P((struct dpipe *, char *, int));
extern int dpipe_Get        P((struct dpipe *, char **));
extern void dpipe_Unget     P((struct dpipe *, char *, int));

extern void dpipeline_Init    P((struct dpipeline *,
				 dpipe_Callback_t, GENERIC_POINTER_TYPE *,
				 dpipe_Callback_t, GENERIC_POINTER_TYPE *));
extern void dpipeline_Prepend P((struct dpipeline *,
				 dpipeline_Filter_t, GENERIC_POINTER_TYPE *,
				 dpipeline_Finalize_t));
extern void dpipeline_Append  P((struct dpipeline *,
				 dpipeline_Filter_t, GENERIC_POINTER_TYPE *,
				 dpipeline_Finalize_t));
extern void dpipeline_PrependDpipe P((struct dpipeline *,
				      struct dpipe *));
extern void dpipeline_AppendDpipe P((struct dpipeline *,
				     struct dpipe *));
extern struct dpipe *dpipeline_UnprependDpipe P((struct dpipeline *));
extern struct dpipe *dpipeline_UnappendDpipe P((struct dpipeline *));
extern void dpipeline_Destroy P((struct dpipeline *));

extern struct dpipe *dpipeline_wrEnd P((struct dpipeline *));
extern struct dpipe *dpipeline_rdEnd P((struct dpipeline *));

DECLARE_EXCEPTION(dpipe_err_NoReader);
DECLARE_EXCEPTION(dpipe_err_NoWriter);
DECLARE_EXCEPTION(dpipe_err_Closed);
DECLARE_EXCEPTION(dpipe_err_Pipeline);

# define dpipe_EOF (-1)

#endif /* DPIPE_H */
