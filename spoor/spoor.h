/*
 * $RCSfile: spoor.h,v $
 * $Revision: 2.24 $
 * $Date: 1995/09/20 06:39:34 $
 * $Author: liblit $
 */

#ifndef SPOOR_H
#define SPOOR_H

#include <general.h>

#include <excfns.h>
#include <glist.h>

#define SUPERCLASS(x) struct x _Spoor_Superclass_Info_
#define spArgList_t VA_LIST
#define spArg(arglist, type) VA_ARG(arglist, type)

#define spoor_Class(obj) (((struct spoor *) (obj))->Class)
#define spoor_InstanceName(obj) (((struct spoor *) (obj))->name)

#define spClass_Name(c) (((struct spClass *) (c))->name)
#define spClass_superClass(c) (((struct spClass *) (c))->superClass)

struct spoor {
    struct spClass *Class;
    char *name;
};

struct spClass {
    SUPERCLASS(spoor);
    struct spClass *superClass;
    struct glist methods, children;
    char *name, *description;
    void (*initialize) (), (*finalize) ();
    int instanceSize, myNum, subClassCutoff;
};

typedef (*spoor_method_t) NP((struct spoor *, spArgList_t));

extern struct spClass *spoor_class, *spClass_class;

extern int m_spoor_subclassResponsibility;
extern int m_spoor_setInstanceName;

extern int m_spClass_addMethod;
extern int m_spClass_addOverride;
extern int m_spClass_findMethod;
extern int m_spClass_methodDescription;
extern int m_spClass_initializeInstance;
extern int m_spClass_newInstance;

extern int spoor_IsClassMember P((VPTR,
				  struct spClass *));
extern int spoor_NumberClasses P((void));
extern struct spoor *spoor_FindInstance P((const char *));
extern struct spClass *
    spoor_CreateClass P((char *,
			 char *,
			 struct spClass *,
			 int,
			 void (*) NP((VPTR)),
			 void (*) NP((VPTR))));
extern void spClass_setup P((struct spClass *,
			     char *,
			     char *,
			     struct spClass *,
			     int,
			     void (*) NP((VPTR)),
			     void (*) NP((VPTR))));
extern struct spClass *spoor_FindClass P((char *));
extern void spoor_DestroyInstance P((VPTR));
extern void spoor_Initialize P((void));
extern void spoor_FinalizeInstance P((VPTR));

extern void spoor_Protect P((void));
extern void spoor_Unprotect P((void));

extern void                  spSend(VA_PROTO(VPTR));
extern char                  spSend_c(VA_PROTO(VPTR));
extern double                spSend_d(VA_PROTO(VPTR));
extern float                 spSend_f(VA_PROTO(VPTR));
extern int                   spSend_i(VA_PROTO(VPTR));
extern long                  spSend_l(VA_PROTO(VPTR));
extern VPTR		     spSend_p(VA_PROTO(VPTR));
extern short                 spSend_s(VA_PROTO(VPTR));
extern unsigned char        spSend_uc(VA_PROTO(VPTR));
extern unsigned int         spSend_ui(VA_PROTO(VPTR));
extern unsigned long        spSend_ul(VA_PROTO(VPTR));
extern unsigned short       spSend_us(VA_PROTO(VPTR));

extern void                  spSuper(VA_PROTO(VPTR));
extern char                  spSuper_c(VA_PROTO(VPTR));
extern double                spSuper_d(VA_PROTO(VPTR));
extern float                 spSuper_f(VA_PROTO(VPTR));
extern int                   spSuper_i(VA_PROTO(VPTR));
extern long                  spSuper_l(VA_PROTO(VPTR));
extern VPTR		     spSuper_p(VA_PROTO(VPTR));
extern short                 spSuper_s(VA_PROTO(VPTR));
unsigned char               spSuper_uc(VA_PROTO(VPTR));
unsigned int                spSuper_ui(VA_PROTO(VPTR));
unsigned long               spSuper_ul(VA_PROTO(VPTR));
unsigned short              spSuper_us(VA_PROTO(VPTR));

#define spoor_AddMethod(c,n,d,f) \
    (spSend_i((c),m_spClass_addMethod,(n),(d),(f)))
#define spoor_AddOverride(c,s,d,f) \
    (spSend((c),m_spClass_addOverride,(s),(d),(f)))
#define spoor_FindMethod(c,n) \
    (spSend_i((c),m_spClass_findMethod,(n)))
#define spoor_MethodDescription(c,s) \
    ((char *)spSend_p((c),m_spClass_methodDescription,(s)))
#define spoor_InitializeInstance(c,o) \
    (spSend((c),m_spClass_initializeInstance,(o)))
#define spoor_NewInstance(c) \
    ((struct spoor *)spSend_p((c),m_spClass_newInstance))

#define spoor_NEW() \
    ((struct spoor *) spoor_NewInstance(spoor_class))
#define spClass_NEW() \
    ((struct spClass *) spoor_NewInstance(spClass_class))

#define SPOOR_PROTECT \
    do { \
        spoor_Protect(); \
        TRY

#define SPOOR_ENDPROTECT \
        FINALLY { \
            spoor_Unprotect(); \
        } ENDTRY; \
    } while (0)

struct spoor_RegistryElt {
    char *name;
    struct spoor *obj;
};

extern struct hashtab *spoor_InstanceRegistry;
extern char spoor_SubclassResponsibility[]; /* exception name */

#endif /* SPOOR_H */
