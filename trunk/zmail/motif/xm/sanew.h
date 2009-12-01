#ifdef SANE_WINDOW
#ifndef _XmSaneW_h
#define _XmSaneW_h

#include <Xm/Xm.h>

#define ZmNextResizable "extResizable"
#define ZmNhasSash "hasSash"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/* Class record constant */
externalref WidgetClass zmSaneWindowWidgetClass;

#ifndef XmIsSaneWindow
#define XmIsSaneWindow(w)	XtIsSubclass(w, zmSaneWindowWidgetClass)
#endif /* XmIsSaneWindow */

typedef struct _XmSaneWindowClassRec  *XmSaneWindowWidgetClass;
typedef struct _XmSaneWindowRec	*XmSaneWindowWidget;


#ifdef _NO_PROTO
extern Widget XmCreateSaneWindow() ;
#else
extern Widget XmCreateSaneWindow( 
                        Widget parent,
                        char *name,
                        ArgList args,
                        int argCount) ;
#endif /* _NO_PROTO */


#if defined(__cplusplus) || defined(c_plusplus)
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmSaneWindow_h */
#endif /* SANE_WINDOW */
/* DON'T ADD ANYTHING AFTER THIS #endif */

