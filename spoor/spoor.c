/*
 * $RCSfile: spoor.c,v $
 * $Revision: 2.37 $
 * $Date: 1995/09/20 06:39:32 $
 * $Author: liblit $
 */

#include <config.h>

#include <stdio.h>

#include <glist.h>
#include <dlist.h>
#include <hashtab.h>
#include <spoor.h>
#include <zstrings.h>

#ifndef lint
static const char spoor_rcsid[] =
    "$Id: spoor.c,v 2.37 1995/09/20 06:39:32 liblit Exp $";
#endif /* lint */

#define methodTable_Entry(mt,n) \
    ((struct methodTableEntry *)(glist_Nth((mt),(n))))
#define methodTable_Length(mt) (glist_Length(mt))

#define methodTable_Function(mt,sel) \
    ((methodTable_Entry((mt),(sel)))->fn)
#define methodTable_Name(mt,sel) \
    ((methodTable_Entry((mt),(sel)))->name)
#define methodTable_Description(mt,sel) \
    ((methodTable_Entry((mt),(sel)))->description)

#define methodTable_Replace(mt,sel,entry) \
    (glist_Set((mt),(sel),((char *)(entry))))

struct methodTableEntry {
    char *name, *description;
    void (*fn) NP((struct spoor *, spArgList_t));
};

struct spClass *spoor_class, *spClass_class;

static int classNumberingIsValid = 0;

static int protectlevel = 0, expunging = 0;

static struct glist classList;
static struct hashtab instanceRegistry;
static struct dlist protected;

struct hashtab *spoor_InstanceRegistry = &instanceRegistry;

char spoor_SubclassResponsibility[] = "subclass responsibility";

int m_spoor_subclassResponsibility;
int m_spoor_setInstanceName;

int m_spClass_addMethod;
int m_spClass_addOverride;
int m_spClass_findMethod;
int m_spClass_methodDescription;
int m_spClass_initializeInstance;
int m_spClass_newInstance;

/* These two macros are for when we have to declare self or obj as
 * GENERIC_POINTER_TYPE *
 */
#define Self ((struct spoor *) self)
#define Obj  ((struct spoor *) obj)

int
spoor_IsClassMember(self, Class)
    GENERIC_POINTER_TYPE *self;
    struct spClass *Class;
{
    struct spClass *c = Self->Class;
    int onum;

    if (!classNumberingIsValid) {
	spoor_NumberClasses();
    }
    onum = c->myNum;
    return ((onum >= Class->myNum) && (onum < Class->subClassCutoff));
}

static void
spoor_setInstanceName(self, arg)
    struct spoor *self;
    spArgList_t arg;
{
    const char *name = spArg(arg, const char *);

    if (self->name) {
	struct spoor_RegistryElt probe;

	probe.name = self->name;
	hashtab_Remove(&instanceRegistry, &probe);
	free(self->name);
    }
    if (name) {
	struct spoor_RegistryElt tmp;

	self->name = emalloc(1 + strlen(name), "setInstanceName");
	strcpy(self->name, name);
	tmp.name = self->name;
	tmp.obj = self;
	hashtab_Add(&instanceRegistry, &tmp);
    } else {
	self->name = NULL;
    }
}

static void
spoor_subclassResponsibility(self, arg)
    struct spoor *self;
    spArgList_t arg;
{
    char *Class, *method;
    static char buf[128];

    Class = spArg(arg, char *);
    method = spArg(arg, char *);

    sprintf(buf, "%s:%s", Class, method);
    RAISE(spoor_SubclassResponsibility, buf);
}

static void
methodTableEntry_Init(entry, name, description, fn)
    struct methodTableEntry *entry;
    char *name, *description;
    void (*fn) NP((struct spoor *, spArgList_t));
{
    entry->name = name;
    entry->description = description;
    entry->fn = fn;
}

static void
methodTable_Init(mt)
    struct glist *mt;
{
    glist_Init(mt, (sizeof (struct methodTableEntry)), 8);
}

static int
methodTable_AddEntry(mt, entry)
    struct glist *mt;
    struct methodTableEntry *entry;
{
    int selector = methodTable_Length(mt);

    glist_Add(mt, (char *) entry);
    return (selector);
}

static int
methodTable_Find(mt, name)
    struct glist *mt;
    char *name;
{
    int i;

    for (i = 0; i < methodTable_Length(mt); ++i)
	if (!strcmp(name, methodTable_Name(mt, i)))
	    return (i);
    return (-1);
}

static void
recursivelyInitializeInstance(obj, Class)
    struct spoor *obj;
    struct spClass *Class;
{
    if (Class->superClass)
	recursivelyInitializeInstance(obj, Class->superClass);
    if (Class->initialize)
	(*(Class->initialize))(obj);
}

static void
recursivelyFinalizeInstance(obj, Class)
    struct spoor *obj;
    struct spClass *Class;
{
    if (Class->finalize)
	(*(Class->finalize))(obj);
    if (Class->superClass)
	recursivelyFinalizeInstance(obj, Class->superClass);
}

static void
root_init(self)
    struct spoor *self;
{
    self->name = NULL;
}

static void
root_final(self)
    struct spoor *self;
{
    if (self->name) {
	free(self->name);
	self->name = NULL;
    }
}

static int registry_cmp P((CVPTR, CVPTR));

static int
registry_cmp(a, b)
    CVPTR a, b;
{
    return (strcmp(((struct spoor_RegistryElt *) a)->name,
                   ((struct spoor_RegistryElt *) b)->name));
}

static unsigned int registry_hash P((CVPTR));

static unsigned int
registry_hash(e)
    CVPTR e;
{
    return (hashtab_StringHash(((const struct spoor_RegistryElt *) e)->name));
}

static void
spClass_init(self)
    struct spClass *self;
{
    methodTable_Init(&(self->methods));
    glist_Init(&(self->children), (sizeof (struct spClass *)), 4);
    self->superClass = (struct spClass *) 0;
    self->name = (char *) 0;
    self->description = (char *) 0;
    self->initialize = (void (*) ()) 0;
    self->finalize = (void (*) ()) 0;
}

static int
spClass_addMethod(self, arg)
    struct spClass *self;
    spArgList_t arg;
{
    char *name, *description;
    spoor_method_t fn;
    struct methodTableEntry entry;

    name = spArg(arg, char *);
    description = spArg(arg, char *);
    fn = spArg(arg, spoor_method_t);

    methodTableEntry_Init(&entry, name, description, fn);
    return (methodTable_AddEntry(&(self->methods), &entry));
}

static void
spClass_addOverride(self, arg)
    struct spClass *self;
    spArgList_t arg;
{
    int selector;
    char *description;
    spoor_method_t fn;
    struct methodTableEntry new, *old;

    selector = spArg(arg, int);
    description = spArg(arg, char *);
    fn = spArg(arg, spoor_method_t);

    old = methodTable_Entry(&(self->methods), selector);
    methodTableEntry_Init(&new, old->name,
			  description ? description : old->description,
			  fn);
    methodTable_Replace(&(self->methods), selector, &new);
}

static int
spClass_findMethod(self, arg)
    struct spClass *self;
    spArgList_t arg;
{
    char *name = spArg(arg, char *);

    return (methodTable_Find(&(self->methods), name));
}

static char *
spClass_methodDescription(self, arg)
    struct spClass *self;
    spArgList_t arg;
{
    int selector = spArg(arg, int);

    return (methodTable_Description(&(self->methods), selector));
}

static void
spClass_initializeInstance(self, arg)
    struct spClass *self;
    spArgList_t arg;
{
    struct spoor *obj = spArg(arg, struct spoor *);

    obj->Class = self;
    recursivelyInitializeInstance(obj, self);
}

static struct spoor *
spClass_newInstance(self, arg)
    struct spClass *self;
    spArgList_t arg;
{
    struct spoor *result = 0;

    TRY {
	spoor_InitializeInstance(self,
				 result = ((struct spoor *)
					   emalloc(self->instanceSize,
						   "spoor_NewInstance")));
    } EXCEPT(ANY) {
	if (result)
	    free(result);
	PROPAGATE();
    } ENDTRY;

    return (result);
}

void
spoor_Initialize()
{
    static int initialized = 0;
    struct methodTableEntry entry;
    int i;

    if (initialized)
	return;
    initialized = 1;

    spoor_class = (struct spClass *) emalloc(sizeof (struct spClass),
					     "spoor_Initialize");
    spClass_class = (struct spClass *) emalloc(sizeof (struct spClass),
					       "spoor_Initialize");

    /* Initialize the spoor class */
    spoor_class->superClass = (struct spClass *) 0;
    spoor_Class(spoor_class) = spClass_class;
    methodTable_Init(&(spoor_class->methods));
    glist_Init(&(spoor_class->children), (sizeof (struct spClass *)), 4);
    spoor_class->name = "spoor";
    spoor_class->description = "root of the SPOOR class hierarchy";
    spoor_class->initialize = root_init;
    spoor_class->finalize = root_final;
    spoor_class->instanceSize = (sizeof (struct spoor));
    ((struct spoor *) spoor_class)->name = (char *) 0;

    /* Initialize the spClass class */
    spClass_class->superClass = spoor_class;
    spoor_Class(spClass_class) = spClass_class;
    methodTable_Init(&(spClass_class->methods));
    glist_Init(&(spClass_class->children), (sizeof (struct spClass *)), 4);
    spClass_class->name = "spClass";
    spClass_class->description = "class of SPOOR classes";
    spClass_class->initialize = spClass_init;
    spClass_class->finalize = (void (*) ()) 0;
				/* classes aren't designed to be
				 * deallocated */
    spClass_class->instanceSize = (sizeof (struct spClass));
    ((struct spoor *) spClass_class)->name = (char *) 0;
    /* spClass is a child of spoor */
    glist_Add(&(spoor_class->children), &spClass_class);

    /* Add methods to spoor */
    methodTableEntry_Init(&entry, "subclassResponsibility",
			  "abstract supermethod erroneously selected",
			  spoor_subclassResponsibility);
    m_spoor_subclassResponsibility =
	methodTable_AddEntry(&(spoor_class->methods), &entry);
    methodTableEntry_Init(&entry, "setInstanceName",
			  "name an instance and place it in the registry",
			  spoor_setInstanceName);
    m_spoor_setInstanceName =
	methodTable_AddEntry(&(spoor_class->methods), &entry);

    /* Copy spoor's methodtable, then add methods to spClass */
    for (i = 0; i < methodTable_Length(&(spoor_class->methods)); ++i) {
	methodTable_AddEntry(&(spClass_class->methods),
			     methodTable_Entry(&(spoor_class->methods), i));
    }
    methodTableEntry_Init(&entry, "addMethod",
			  "add a method to a class's methodtable",
			  spClass_addMethod);
    m_spClass_addMethod = methodTable_AddEntry(&(spClass_class->methods),
					       &entry);
    methodTableEntry_Init(&entry, "addOverride",
			  "add an override to a class's methodtable",
			  spClass_addOverride);
    m_spClass_addOverride = methodTable_AddEntry(&(spClass_class->methods),
						 &entry);
    methodTableEntry_Init(&entry, "findMethod",
			  "find a method selector by name",
			  spClass_findMethod);
    m_spClass_findMethod = methodTable_AddEntry(&(spClass_class->methods),
						&entry);
    methodTableEntry_Init(&entry, "methodDescription",
			  "get a method description by selector",
			  spClass_methodDescription);
    m_spClass_methodDescription =
	methodTable_AddEntry(&(spClass_class->methods), &entry);
    methodTableEntry_Init(&entry, "initializeInstance",
			  "initialize an instance of a class",
			  spClass_initializeInstance);
    m_spClass_initializeInstance =
	methodTable_AddEntry(&(spClass_class->methods), &entry);
    methodTableEntry_Init(&entry, "newInstance",
			  "allocate a new instance of a class",
			  spClass_newInstance);
    m_spClass_newInstance = methodTable_AddEntry(&(spClass_class->methods),
						 &entry);

    /* Register the new classes */
    glist_Init(&classList, (sizeof (struct spClass *)), 8);
    glist_Add(&classList, &spoor_class);
    glist_Add(&classList, &spClass_class);

    /* Miscellaneous additional initialization */
    hashtab_Init(&instanceRegistry, registry_hash,
		 registry_cmp, (sizeof (struct spoor_RegistryElt)), 37);
    dlist_Init(&protected, (sizeof (struct spoor *)), 32);
}

void
spClass_setup(self, name, descr, super, size, init, final)
    struct spClass *self;
    char *name, *descr;
    struct spClass *super;
    int size;
    void (*init) NP((GENERIC_POINTER_TYPE *));
    void (*final) NP((GENERIC_POINTER_TYPE *));
{
    struct methodTableEntry *mte;
    int i;

    classNumberingIsValid = 0;
    self->superClass = super ? super : spoor_class;
    glist_Add(&(self->superClass->children), &self);
    self->name = name;
    self->description = descr;
    self->initialize = init;
    self->finalize = final;
    self->instanceSize = size;

    for (i = 0; i < methodTable_Length(&(self->superClass->methods)); ++i) {
	mte = methodTable_Entry(&(self->superClass->methods), i);
	methodTable_AddEntry(&(self->methods), mte);
    }

    glist_Add(&classList, &self);
}

struct spClass *
spoor_CreateClass(name, descr, super, size, init, final)
    char *name, *descr;
    struct spClass *super;
    int size;
    void (*init)  NP((GENERIC_POINTER_TYPE *));
    void (*final) NP((GENERIC_POINTER_TYPE *));
{
    struct spClass *result;

    spClass_setup(result = spClass_NEW(),
		  name, descr, super, size, init, final);
    return (result);
}

struct spClass *
spoor_FindClass(name)
    char *name;
{
    int i;

    for (i = 0; i < glist_Length(&classList); ++i)
	if (!strcmp(name, (*((struct spClass **)
			     glist_Nth(&classList, i)))->name))
	    return (*((struct spClass **)
		      glist_Nth(&classList, i)));
    return (0);
}

struct spoor *
spoor_FindInstance(name)
    const char *name;
{
    struct spoor_RegistryElt *result, probe;

    /* XXX casting away const */
    probe.name = (char *) name;
    result = (struct spoor_RegistryElt *) hashtab_Find(&instanceRegistry,
						  &probe);
    return (result ? result->obj : (struct spoor *) 0);
}

void
spoor_FinalizeInstance(obj)
    GENERIC_POINTER_TYPE *obj;
{
    if (Obj->name) {
	struct spoor_RegistryElt probe;

	probe.name = Obj->name;
	hashtab_Remove(&instanceRegistry, &probe);
    }
    recursivelyFinalizeInstance(Obj, Obj->Class);
}

void
spoor_Protect()
{
    ++protectlevel;
}

void
spoor_Unprotect()
{
    if ((--protectlevel <= 0) && !expunging) {
	int i;
	struct spoor *obj;

	expunging = 1;

	TRY {
	    while ((i = dlist_Head(&protected)) >= 0) {
		obj = *((struct spoor **) dlist_Nth(&protected, i));
		/* Now, don't call spoor_DestroyInstance, because a finalize
		 * routine might reprotect objects!
		 */
		spoor_FinalizeInstance(obj);
		free(obj);
		dlist_Remove(&protected, i);
	    }
	} FINALLY {
	    expunging = 0;
	} ENDTRY;
    }
}

void
spoor_DestroyInstance(obj)
    GENERIC_POINTER_TYPE *obj;
{
    if (protectlevel > 0) {
	dlist_Append(&protected, &obj);
    } else {
	spoor_FinalizeInstance(obj);
	free(obj);
    }
}

static int
numberClasses(Class, num)
    struct spClass *Class;
    int num;
{
    int i, newNum;

    Class->myNum = num;
    newNum = num + 1;
    for (i = 0; i < glist_Length(&(Class->children)); ++i) {
	newNum = numberClasses(*((struct spClass **)
				 glist_Nth(&(Class->children), i)),
			       newNum);
    }
    return (Class->subClassCutoff = newNum);
}

int
spoor_NumberClasses()
{
    if (!classNumberingIsValid) {
	classNumberingIsValid = 1;
	return (numberClasses(spoor_class, 0));
    } else
	return (spoor_class->subClassCutoff);
}

void
spSend(VA_ALIST(GENERIC_POINTER_TYPE *obj))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(GENERIC_POINTER_TYPE *obj);
    int sel;
    void (*fn) NP((struct spoor *, spArgList_t));

    VA_START(ap, GENERIC_POINTER_TYPE *, obj);
    sel = VA_ARG(ap, int);
    if (!(fn = ((void (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(Obj->Class->methods), sel)))) {
	spSend(obj, m_spoor_subclassResponsibility,
	       spClass_Name(spoor_Class(Obj)),
	       methodTable_Name(&(Obj->Class->methods), sel));
	return;
    }
    TRY {
	(*fn)(Obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
}

char
spSend_c(VA_ALIST(GENERIC_POINTER_TYPE *obj))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(GENERIC_POINTER_TYPE *obj);
    int sel;
    char (*fn) NP((struct spoor *, spArgList_t)), retval;

    VA_START(ap, GENERIC_POINTER_TYPE *, obj);
    sel = VA_ARG(ap, int);
    if (!(fn = ((char (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(Obj->Class->methods), sel)))) {
	spSend(obj, m_spoor_subclassResponsibility,
	       spClass_Name(spoor_Class(Obj)),
	       methodTable_Name(&(Obj->Class->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(Obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

double
spSend_d(VA_ALIST(GENERIC_POINTER_TYPE *obj))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(GENERIC_POINTER_TYPE *obj);
    int sel;
    double (*fn) NP((struct spoor *, spArgList_t)), retval;

    VA_START(ap, GENERIC_POINTER_TYPE *, obj);
    sel = VA_ARG(ap, int);
    if (!(fn = ((double (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(Obj->Class->methods), sel)))) {
	spSend(obj, m_spoor_subclassResponsibility,
	       spClass_Name(spoor_Class(Obj)),
	       methodTable_Name(&(Obj->Class->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(Obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

float
spSend_f(VA_ALIST(GENERIC_POINTER_TYPE *obj))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(GENERIC_POINTER_TYPE *obj);
    int sel;
    float (*fn) NP((struct spoor *, spArgList_t)), retval;

    VA_START(ap, GENERIC_POINTER_TYPE *, obj);
    sel = VA_ARG(ap, int);
    if (!(fn = ((float (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(Obj->Class->methods), sel)))) {
	spSend(obj, m_spoor_subclassResponsibility,
	       spClass_Name(spoor_Class(Obj)),
	       methodTable_Name(&(Obj->Class->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(Obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

int
spSend_i(VA_ALIST(GENERIC_POINTER_TYPE *obj))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(GENERIC_POINTER_TYPE *obj);
    int sel;
    int (*fn) NP((struct spoor *, spArgList_t)), retval;

    VA_START(ap, GENERIC_POINTER_TYPE *, obj);
    sel = VA_ARG(ap, int);
    if (!(fn = ((int (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(Obj->Class->methods), sel)))) {
	spSend(obj, m_spoor_subclassResponsibility,
	       spClass_Name(spoor_Class(Obj)),
	       methodTable_Name(&(Obj->Class->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(Obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

long
spSend_l(VA_ALIST(GENERIC_POINTER_TYPE *obj))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(GENERIC_POINTER_TYPE *obj);
    int sel;
    long (*fn) NP((struct spoor *, spArgList_t)), retval;

    VA_START(ap, GENERIC_POINTER_TYPE *, obj);
    sel = VA_ARG(ap, int);
    if (!(fn = ((long (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(Obj->Class->methods), sel)))) {
	spSend(obj, m_spoor_subclassResponsibility,
	       spClass_Name(spoor_Class(Obj)),
	       methodTable_Name(&(Obj->Class->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(Obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

GENERIC_POINTER_TYPE *
spSend_p(VA_ALIST(GENERIC_POINTER_TYPE *obj))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(GENERIC_POINTER_TYPE *obj);
    int sel;
    GENERIC_POINTER_TYPE *(*fn) NP((struct spoor *, spArgList_t)), *retval;

    VA_START(ap, GENERIC_POINTER_TYPE *, obj);
    sel = VA_ARG(ap, int);
    if (!(fn = ((GENERIC_POINTER_TYPE *(*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(Obj->Class->methods), sel)))) {
	spSend(obj, m_spoor_subclassResponsibility,
	       spClass_Name(spoor_Class(Obj)),
	       methodTable_Name(&(Obj->Class->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(Obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

short
spSend_s(VA_ALIST(GENERIC_POINTER_TYPE *obj))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(GENERIC_POINTER_TYPE *obj);
    int sel;
    short (*fn) NP((struct spoor *, spArgList_t)), retval;

    VA_START(ap, GENERIC_POINTER_TYPE *, obj);
    sel = VA_ARG(ap, int);
    if (!(fn = ((short (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(Obj->Class->methods), sel)))) {
	spSend(obj, m_spoor_subclassResponsibility,
	       spClass_Name(spoor_Class(Obj)),
	       methodTable_Name(&(Obj->Class->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(Obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

unsigned char
spSend_uc(VA_ALIST(GENERIC_POINTER_TYPE *obj))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(GENERIC_POINTER_TYPE *obj);
    int sel;
    unsigned char (*fn) NP((struct spoor *, spArgList_t)), retval;

    VA_START(ap, GENERIC_POINTER_TYPE *, obj);
    sel = VA_ARG(ap, int);
    if (!(fn = ((unsigned char (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(Obj->Class->methods), sel)))) {
	spSend(obj, m_spoor_subclassResponsibility,
	       spClass_Name(spoor_Class(Obj)),
	       methodTable_Name(&(Obj->Class->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(Obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

unsigned int
spSend_ui(VA_ALIST(GENERIC_POINTER_TYPE *obj))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(GENERIC_POINTER_TYPE *obj);
    int sel;
    unsigned int (*fn) NP((struct spoor *, spArgList_t)), retval;

    VA_START(ap, GENERIC_POINTER_TYPE *, obj);
    sel = VA_ARG(ap, int);
    if (!(fn = ((unsigned int (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(Obj->Class->methods), sel)))) {
	spSend(obj, m_spoor_subclassResponsibility,
	       spClass_Name(spoor_Class(Obj)),
	       methodTable_Name(&(Obj->Class->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(Obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

unsigned long
spSend_ul(VA_ALIST(GENERIC_POINTER_TYPE *obj))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(GENERIC_POINTER_TYPE *obj);
    int sel;
    unsigned long (*fn) NP((struct spoor *, spArgList_t)), retval;

    VA_START(ap, GENERIC_POINTER_TYPE *, obj);
    sel = VA_ARG(ap, int);
    if (!(fn = ((unsigned long (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(Obj->Class->methods), sel)))) {
	spSend(obj, m_spoor_subclassResponsibility,
	       spClass_Name(spoor_Class(Obj)),
	       methodTable_Name(&(Obj->Class->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(Obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

unsigned short
spSend_us(VA_ALIST(GENERIC_POINTER_TYPE *obj))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(GENERIC_POINTER_TYPE *obj);
    int sel;
    unsigned short (*fn) NP((struct spoor *, spArgList_t)), retval;

    VA_START(ap, GENERIC_POINTER_TYPE *, obj);
    sel = VA_ARG(ap, int);
    if (!(fn = ((unsigned short (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(Obj->Class->methods), sel)))) {
	spSend(obj, m_spoor_subclassResponsibility,
	       spClass_Name(spoor_Class(Obj)),
	       methodTable_Name(&(Obj->Class->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(Obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

void
spSuper(VA_ALIST(VPTR x))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(VPTR x);
    struct spClass *presumedClass;
    struct spoor *obj;
    int sel;
    void (*fn) NP((struct spoor *, spArgList_t));
    
    VA_START(ap, VPTR, x);
    presumedClass = (struct spClass *) x;
    obj = VA_ARG(ap, struct spoor *);
    sel = VA_ARG(ap, int);
    
    if (!(fn = ((void (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(presumedClass->superClass->methods),
				     sel)))) {
	spSuper(presumedClass, obj,
		m_spoor_subclassResponsibility,
		spClass_Name(spoor_Class(obj)),
		methodTable_Name(&(presumedClass->methods), sel));
	return;
    }
    TRY {
	(*fn)(obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
}

char
spSuper_c(VA_ALIST(VPTR x))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(VPTR x);
    struct spClass *presumedClass;
    struct spoor *obj;
    int sel;
    char (*fn) NP((struct spoor *, spArgList_t)), retval;

    VA_START(ap, VPTR, x);
    presumedClass = (struct spClass *) x;
    obj = VA_ARG(ap, struct spoor *);
    sel = VA_ARG(ap, int);

    if (!(fn = ((char (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(presumedClass->superClass->methods),
				     sel)))) {
	spSuper(presumedClass, obj, m_spoor_subclassResponsibility,
		spClass_Name(spoor_Class(obj)),
		methodTable_Name(&(presumedClass->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

double
spSuper_d(VA_ALIST(VPTR x))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(VPTR x);
    struct spClass *presumedClass;
    struct spoor *obj;
    int sel;
    double (*fn) NP((struct spoor *, spArgList_t)), retval;

    VA_START(ap, VPTR, x);
    presumedClass = (struct spClass *) x;
    obj = VA_ARG(ap, struct spoor *);
    sel = VA_ARG(ap, int);

    if (!(fn = ((double (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(presumedClass->superClass->methods),
				     sel)))) {
	spSuper(presumedClass, obj, m_spoor_subclassResponsibility,
		spClass_Name(spoor_Class(obj)),
		methodTable_Name(&(presumedClass->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

float
spSuper_f(VA_ALIST(VPTR x))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(VPTR x);
    struct spClass *presumedClass;
    struct spoor *obj;
    int sel;
    float (*fn) NP((struct spoor *, spArgList_t)), retval;

    VA_START(ap, VPTR, x);
    presumedClass = (struct spClass *) x;
    obj = VA_ARG(ap, struct spoor *);
    sel = VA_ARG(ap, int);

    if (!(fn = ((float (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(presumedClass->superClass->methods),
				     sel)))) {
	spSuper(presumedClass, obj, m_spoor_subclassResponsibility,
		spClass_Name(spoor_Class(obj)),
		methodTable_Name(&(presumedClass->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

int
spSuper_i(VA_ALIST(VPTR x))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(VPTR x);
    struct spClass *presumedClass;
    struct spoor *obj;
    int sel;
    int (*fn) NP((struct spoor *, spArgList_t)), retval;

    VA_START(ap, VPTR, x);
    presumedClass = (struct spClass *) x;
    obj = VA_ARG(ap, struct spoor *);
    sel = VA_ARG(ap, int);

    if (!(fn = ((int (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(presumedClass->superClass->methods),
				     sel)))) {
	spSuper(presumedClass, obj, m_spoor_subclassResponsibility,
		spClass_Name(spoor_Class(obj)),
		methodTable_Name(&(presumedClass->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

long
spSuper_l(VA_ALIST(VPTR x))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(VPTR x);
    struct spClass *presumedClass;
    struct spoor *obj;
    int sel;
    long (*fn) NP((struct spoor *, spArgList_t)), retval;

    VA_START(ap, VPTR, x);
    presumedClass = (struct spClass *) x;
    obj = VA_ARG(ap, struct spoor *);
    sel = VA_ARG(ap, int);

    if (!(fn = ((long (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(presumedClass->superClass->methods),
				     sel)))) {
	spSuper(presumedClass, obj, m_spoor_subclassResponsibility,
		spClass_Name(spoor_Class(obj)),
		methodTable_Name(&(presumedClass->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

GENERIC_POINTER_TYPE *
spSuper_p(VA_ALIST(VPTR x))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(VPTR x);
    struct spClass *presumedClass;
    struct spoor *obj;
    int sel;
    GENERIC_POINTER_TYPE *(*fn) NP((struct spoor *, spArgList_t)), *retval;

    VA_START(ap, VPTR, x);
    presumedClass = (struct spClass *) x;
    obj = VA_ARG(ap, struct spoor *);
    sel = VA_ARG(ap, int);

    if (!(fn = ((GENERIC_POINTER_TYPE *(*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(presumedClass->superClass->methods),
				     sel)))) {
	spSuper(presumedClass, obj, m_spoor_subclassResponsibility,
		spClass_Name(spoor_Class(obj)),
		methodTable_Name(&(presumedClass->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

short
spSuper_s(VA_ALIST(VPTR x))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(VPTR x);
    struct spClass *presumedClass;
    struct spoor *obj;
    int sel;
    short (*fn) NP((struct spoor *, spArgList_t)), retval;

    VA_START(ap, VPTR, x);
    presumedClass = (struct spClass *) x;
    obj = VA_ARG(ap, struct spoor *);
    sel = VA_ARG(ap, int);

    if (!(fn = ((short (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(presumedClass->superClass->methods),
				     sel)))) {
	spSuper(presumedClass, obj, m_spoor_subclassResponsibility,
		spClass_Name(spoor_Class(obj)),
		methodTable_Name(&(presumedClass->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

unsigned char
spSuper_uc(VA_ALIST(VPTR x))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(VPTR x);
    struct spClass *presumedClass;
    struct spoor *obj;
    int sel;
    unsigned char (*fn) NP((struct spoor *, spArgList_t)), retval;

    VA_START(ap, VPTR, x);
    presumedClass = (struct spClass *) x;
    obj = VA_ARG(ap, struct spoor *);
    sel = VA_ARG(ap, int);

    if (!(fn = ((unsigned char (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(presumedClass->superClass->methods),
				     sel)))) {
	spSuper(presumedClass, obj, m_spoor_subclassResponsibility,
		spClass_Name(spoor_Class(obj)),
		methodTable_Name(&(presumedClass->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

unsigned int
spSuper_ui(VA_ALIST(VPTR x))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(VPTR x);
    struct spClass *presumedClass;
    struct spoor *obj;
    int sel;
    unsigned int (*fn) NP((struct spoor *, spArgList_t)), retval;

    VA_START(ap, VPTR, x);
    presumedClass = (struct spClass *) x;
    obj = VA_ARG(ap, struct spoor *);
    sel = VA_ARG(ap, int);

    if (!(fn = ((unsigned int (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(presumedClass->superClass->methods),
				     sel)))) {
	spSuper(presumedClass, obj, m_spoor_subclassResponsibility,
		spClass_Name(spoor_Class(obj)),
		methodTable_Name(&(presumedClass->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

unsigned long
spSuper_ul(VA_ALIST(VPTR x))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(VPTR x);
    struct spClass *presumedClass;
    struct spoor *obj;
    int sel;
    unsigned long (*fn) NP((struct spoor *, spArgList_t)), retval;

    VA_START(ap, VPTR, x);
    presumedClass = (struct spClass *) x;
    obj = VA_ARG(ap, struct spoor *);
    sel = VA_ARG(ap, int);

    if (!(fn = ((unsigned long (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(presumedClass->superClass->methods),
				     sel)))) {
	spSuper(presumedClass, obj, m_spoor_subclassResponsibility,
		spClass_Name(spoor_Class(obj)),
		methodTable_Name(&(presumedClass->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}

unsigned short
spSuper_us(VA_ALIST(VPTR x))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(VPTR x);
    struct spClass *presumedClass;
    struct spoor *obj;
    int sel;
    unsigned short (*fn) NP((struct spoor *, spArgList_t)), retval;

    VA_START(ap, VPTR, x);
    presumedClass = (struct spClass *) x;
    obj = VA_ARG(ap, struct spoor *);
    sel = VA_ARG(ap, int);

    if (!(fn = ((unsigned short (*) NP((struct spoor *, spArgList_t)))
		methodTable_Function(&(presumedClass->superClass->methods),
				     sel)))) {
	spSuper(presumedClass, obj, m_spoor_subclassResponsibility,
		spClass_Name(spoor_Class(obj)),
		methodTable_Name(&(presumedClass->methods), sel));
	return (0);
    }
    TRY {
	retval = (*fn)(obj, ap);
    } FINALLY {
	VA_END(ap);
    } ENDTRY;
    return (retval);
}
