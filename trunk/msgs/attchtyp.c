/* attchtyp.c     Copyright 1995 Z-Code Software Corp. */

#ifndef lint
static char	attchtyp_rcsid[] = "$Id: attchtyp.c,v 2.7 1996/07/09 06:28:24 schaefer Exp $";
#endif

#include "zmail.h"
#include "catalog.h"
#include "general.h"
#include "glob.h"
#include "linklist.h"
#include "pager.h"
#include "strcase.h"
#include "zcstr.h"
#include "zmflag.h"
#include "zmopt.h"
#include "fsfix.h"

/*
 * See the attach.base file in lib/attach for further details on the 
 * attach.types file.  Some info follows ...
 *
 * Format of the attach_types file is:
 *
 * # in column 1 only means a comment line
 * PATH    dir1:dir2:...
 * TYPE    type_key  "viewer program"    "editor program"    "comment"
 * CODE    code_key  "encoding program"  "decoding program"  "comment"
 * ENCODE  type_key  code_key
 * DEFAULT code_key
 * ALIAS   alias_name type_key
 * INLINE  type_key1 type_key2 type_key3 ...
 *
 * WINTYPE	type_key	extension	"comment"
 * MACTYPE	mactype/creator	type_key	"comment"
 * MIME2MACTYPE	type_key	mactype/creator
 *
 * IRIX2MIME	IRIXtype	type_key
 * MIME2IRIX	type_key	IRIXtype
 *
 * Keys and aliases are strings.  They may be enclosed in quotes to include 
 * spaces.
 * The type_key is placed (or found) in the X-Zm-Data-Type: header.
 * The code_key is placed (or found) in the X-Zm-Encoding-Algorithm:.
 *
 * Aliases are used to specify that messages labeled as type
 * alias_name should be treated as type type_key. 
 *
 * The PATH specifies the search path used to find TYPE and CODE programs.
 * Second and succeeding PATH entries are appended to the first entry.
 *
 * The programs may be /bin/sh commands or pipes.  If the program
 * begins with a "|" character, then it will be fed its data on the
 * standard input.  If it ends with a "|" character, then it produces
 * data on standard output.  If it contains a %s (printf string-format)
 * anywhere, then a file name is inserted at that point for processing
 * of that file.  CODE programs must have either both a leading and a
 * trailing "|", or a %s and one "|", or only a "%s" (in which case it
 * is assumed the file is modified in place); they may not have all three.
 * TYPE editors and viewers that lack a leading/trailing "|" or a "%s"
 * will simply be executed, and can find their input files themselves.
 *
 * The ENCODE pairs are used for sending only, and specify that files of
 * the given type_key should automatically be encoded using the code_key.
 * The DEFAULT entry specifies which CODE key should be used when a file
 * is a non-ASCII file, but the user has not specified an encoding.
 *
 * The INLINE directive lists types that Z-Mail should attempt to display
 * (or edit) in-line in the message display window (or compose window).
 * This presently applies only to types that can reasonably be represented
 * as text.  Type alias names may also appear in the INLINE directive; if
 * the true name of the aliased type is inlined, then all aliases for it
 * are also; but inlining an alias name does not automatically inline the
 * corresponding true type.
 */

typedef struct link AttachTag;

AttachTag *inline_types;
AttachKey *type_keys, *code_keys;
AttachTypeAlias	*type_aliases;		/* for incoming body parts */
#if defined (MAC_OS) || defined (MSDOS) /* Ugh. This is unfortunate. -ABS 12/30/93 */
AttachTypeAlias	*type_mactypes;		/* for outgoing body parts */
#endif /* MAC_OS || MSDOS */
static int keys_loaded;

/* This should be initialized with a function call */
static char dflt_encode_init[] = "base64";
char *attach_path, *dflt_encode = dflt_encode_init;

/* XXX link_to_argv  comes from shell/cursmenu.c, and should really be 
 * in some include file
 */
extern char **link_to_argv();

static void
free_attach_keys(keys)
AttachKey **keys;
{
    AttachKey *tmp;

    if (tmp = *keys)
	do {
	    if (tmp->k_link.l_next)
		remove_link(keys, tmp);
	    else
		*keys = 0;
	    xfree(tmp->key_string);
	    xfree(tmp->encode.program);
	    xfree(tmp->decode.program);
	    xfree(tmp->use_code);
	    xfree(tmp->description);
#ifdef HAS_ATTACH_BITMAP
	    unload_icon(&tmp->bitmap);
#endif /* HAS_ATTACH_BITMAP */
	    xfree(tmp->bitmap_filename);
	    xfree(tmp);
	} while (tmp = *keys);
}

static void
free_attach_type_aliases(ta)
AttachTypeAlias **ta;
{
    AttachTypeAlias *tmp;
    
    if (tmp = *ta)
	do {
	    if (tmp->a_link.l_next)
		remove_link(ta, tmp);
	    else
		*ta = 0;
	    xfree(tmp->type_key);
	    xfree(tmp);
	} while (tmp = *ta);
}

static void
free_attach_tags(ta)
AttachTag **ta;
{
    AttachTag *tmp;
    
    if (tmp = *ta)
	do {
	    if (tmp->l_next)
		remove_link(ta, tmp);
	    else
		*ta = 0;
	    xfree(tmp->l_name);
	    xfree(tmp);
	} while (tmp = *ta);
}

static void
coloncat(dst, s1, s2)
char *dst, *s1, *s2;
{
    int i;

    if (!dst)
	return;

    if (s1) {
	i = Strcpy(dst, s1+(s1[0]==PJOIN));
	if (i > 0 && dst[i-1] != PJOIN)
	    dst[i++] = PJOIN;
    } else
	dst[(i = 0)] = 0;
    if (s2) {
	i += Strcpy(&dst[i], s2+(s2[0]==PJOIN));
	if (i > 0 && dst[--i] == PJOIN)
	dst[i] = '\0';
    }
}

static void
parse_attach_path(n, fields, override)
int n;		/* argc */
char **fields;	/* argv */
int override;
{
    char *str;
    int i;

    xfree(fields[0]);
    if (n < 2)
	return;
    Debug("Setting attachment program path\n" );
    i = 2 + strlen(fields[1]) + (attach_path? strlen(attach_path) : 0);
    if (str = (char *) malloc(i)) {
	if (override) /* Prepend the new path */
	    coloncat(str, fields[1], attach_path);
	else
	    coloncat(str, attach_path, fields[1]);
	xfree(attach_path);
	attach_path = str;
    } else {
	i = -1;
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 95, "Cannot store attach-types PATH" ));
    }
    free_elems(&fields[1]);
    if (i < 0)
	return;
}

static void
parse_attach_default(n, fields, override)
int n;		/* argc */
char **fields;	/* argv */
int override;
{
    /* Silently ignore, we're using MIME defaults, now */
    free_elems(fields);
#ifdef NOT_NOW
    if (n < 2) {
	free_elems(fields);
	return;
    }    Debug("Setting attachment default encoding\n" );
    xfree(fields[0]);
    /* ci_strcpy(fields[1], fields[1]); */
    if (dflt_encode != dflt_encode_init)
	xfree(dflt_encode);
    dflt_encode = fields[1];
    if (n > 2)
	free_elems(&fields[2]);
#endif
}

static void
parse_attach_encode(n, fields, override)
int n;		/* argc */
char **fields;	/* argv */
int override;
{
    AttachKey *tmp;
    int x = FALSE;

    if (n < 3) {
	free_elems(fields);
	return;
    }
    Debug("Setting attachment type-encoding map\n" );
    /* ci_strcpy(fields[1], fields[1]); */
    if (tmp = (AttachKey *)retrieve_link(type_keys, fields[1], ci_strcmp)) {
	x = TRUE;
	if (override) {
	    xfree(tmp->use_code);
	    tmp->use_code = NULL;
	}
    }
    if (!tmp && !(x = FALSE, tmp = zmNew(AttachKey))) {
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 98, "Cannot allocate type_key" ));
	free_elems(fields);
	return;
    }
    if (!tmp->use_code)
	tmp->use_code = fields[2]; /* ci_strcpy(fields[2], fields[2]); */
    if (!x) {
	tmp->key_string = fields[1];
	insert_link(&type_keys, tmp);
    } else
	xfree(fields[1]);
    xfree(fields[0]);
    if (n > 3)
	free_elems(&fields[3]);
}

static void
parse_attach_alias(n, fields, override)
int n;		/* argc */
char **fields;	/* argv */
int override;
{
    char buf[MAXPATHLEN];
    AttachKey *tmpKey;
    AttachTypeAlias *tmpTypeAlias;
    int x = FALSE;

    if (n < 3) {
	free_elems(fields);
	return;
    }
    Debug("Setting alias for type %s to %s\n" , fields[2], fields[1]);
    /*
    ci_strcpy(fields[1], fields[1]);
    ci_strcpy(fields[2], fields[2]);
    */
    if (tmpTypeAlias = 
	(AttachTypeAlias *)retrieve_link(type_aliases, fields[1], ci_strcmp)) {
	x = TRUE;
	if (override) {
	    xfree(tmpTypeAlias->type_key);
	    tmpTypeAlias->type_key = NULL;
	}
    }
    if (!tmpTypeAlias && !(x = FALSE, tmpTypeAlias = zmNew(AttachTypeAlias))) {
	error(SysErrWarning, 
	      catgets(catalog, CAT_MSGS, 843, "Cannot allocate AttachTypeAlias.\n" ));
	free_elems(fields);
	return;
    }
    (void) strcpy(buf, fields[2]);

    if (tmpKey = (AttachKey *)retrieve_link(type_keys, fields[1], ci_strcmp))
	print(catgets( catalog, CAT_MSGS, 815, "Warning: Using the name %s as an alias for type %s overrides the definition for viewing incoming message parts of type %s.\n" ), 
		  fields[1], fields[2], fields[1]);
    if (!tmpTypeAlias->type_key) {
	tmpTypeAlias->type_key = savestr(fields[2]);
    }
    if (!x) {
	tmpTypeAlias->alias_string = fields[1];
	insert_link(&type_aliases, tmpTypeAlias);
    } else
	xfree(fields[1]);
    xfree(fields[0]);
    if (n > 2)
	free_elems(&fields[2]);
}

static void
parse_attach_bitmap(n, fields, override)
int n;		/* argc */
char **fields;	/* argv */
int override;
{
    char buf[MAXPATHLEN];
    AttachKey *tmp;
    int x = FALSE;

    if (n < 3) {
	free_elems(fields);
	return;
    }
    Debug("Setting attachment bitmap for type %s\n" , fields[1]);
    /* ci_strcpy(fields[1], fields[1]); */
    if (tmp = (AttachKey *)retrieve_link(type_keys, fields[1], ci_strcmp)) {
	x = TRUE;
	if (override) {
	    xfree(tmp->bitmap_filename);
	    tmp->bitmap_filename = NULL;
	}
    }
    if (!tmp && !(tmp = zmNew(AttachKey))) {
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 98, "Cannot allocate type_key" ));
	free_elems(fields);
	return;
    }
    (void) strcpy(buf, fields[2]);
#ifndef MAC_OS
    if (!tmp->bitmap_filename) {
	switch (dvarstat(zmlibdir, fields[2], buf, NULL)) {
	  case -2:
	    if (istool) {
#ifndef VUI
		error(UserErrWarning, "%s: %s", fields[2], buf);
#endif /* VUI */
	        break;
	    }
	    /* else fall through */
	  case -1:		/* Bart says this case should be impossible,
				 * but I know better -- bg */
	    tmp->bitmap_filename = savestr(fields[2]);
	    break;
	  default:
	    tmp->bitmap_filename = savestr(buf);
	    break;
	}
    }
#else /* MAC_OS */
    tmp->bitmap_filename = savestr(fields[2]);
#endif /* !MAC_OS */

    if (!x) {
	tmp->key_string = fields[1];
	insert_link(&type_keys, tmp);
    } else
	xfree(fields[1]);
    xfree(fields[0]);
    if (n > 2)
	free_elems(&fields[2]);
}

#if defined (MAC_OS) || defined (MSDOS)
/* 
 * This is currently only used for mapping MAC type/creator pairs to
 * MIME types.  Eventually a similar scheme should also be used for nicknames 
 * for outgoing types which are presented in the UI
 */

/*
 * This is included, though poorly named, as a mapping from Window's 
 * extensions to MIME types. Ugh.  -ABS 12/30/93
 */

static void
parse_attach_mactype(n, fields, override)
int n;		/* argc */
char **fields;	/* argv */
int override;
{
    char buf[MAXPATHLEN];
    AttachKey *tmpKey;
    AttachTypeAlias *tmpTypeAlias;
    int x = FALSE;

    if (n < 3) {
	free_elems(fields);
	return;
    }
    Debug("%s maps %s to %s\n", fields[0], fields[1], fields[2]);
    if (tmpTypeAlias = 
	(AttachTypeAlias *)retrieve_link(type_mactypes, fields[1],
#ifdef MAC_OS
	    /* Macintosh type/creator values are case sensitive */
	    (ci_strcmp(fields[0], "MIME2MACTYPE") != 0)? strcmp :
#endif /* MAC_OS */
	    ci_strcmp)) {

/*
the only case in which we don't allocate a new attach type is if it is
completely identical.
-ABS 2/15/93
*/

#if defined (MSDOS)
    if (strcmp (fields[2], tmpTypeAlias->type_key) == 0) {
#endif

	x = TRUE;
	if (override) {
	    xfree(tmpTypeAlias->type_key);
	    tmpTypeAlias->type_key = NULL;
	}
#if defined (MSDOS)
    } else {
        tmpTypeAlias = (AttachTypeAlias *) NULL;
    }
#endif    
    }
    if (!tmpTypeAlias && !(x = FALSE, tmpTypeAlias = zmNew(AttachTypeAlias))) {
	error(SysErrWarning, 
	      catgets(catalog, CAT_MSGS, 843, "Cannot allocate AttachTypeAlias.\n" ));
	free_elems(fields);
	return;
    }
    (void) strcpy(buf, fields[2]);

/* 
since in DOS, we can map a 1:n mime type to extension, don't 
complain about it.
-ABS 2/15/93
*/

#if !defined (MSDOS)
    if (tmpKey = (AttachKey *)retrieve_link(type_mactypes, fields[1], ci_strcmp))
	print(catgets(catalog, CAT_MSGS, 846, "Warning: Mapping Macintosh type/creator %s to type %s overrides the previous mapping for Macintosh type/creator %s.\n"), 
		  fields[1], fields[2], fields[1]);
#endif

    if (!tmpTypeAlias->type_key) {
	tmpTypeAlias->type_key = savestr(fields[2]);
    }
    if (!x) {
	tmpTypeAlias->alias_string = fields[1];
	insert_link(&type_mactypes, tmpTypeAlias);
    } else
	xfree(fields[1]);
    xfree(fields[0]);
    if (n > 2)
	free_elems(&fields[2]);
}
#endif /* MAC_OS || MSDOS */

static void
parse_attach_inline(n, fields, override)
int n;		/* argc */
char **fields;	/* argv */
int override;
{
    AttachTag *tmp;
    int x = FALSE;
    char *nextfield;

    if (n < 2) {
	free_elems(fields);
	return;
    }

    for (nextfield = fields[--n]; n > 0; nextfield = fields[--n]) {
	const char *key = get_type_from_alias(nextfield);

	Debug("Setting inline display for type %s\n", nextfield);

	if (key != nextfield) {
	    print(catgets(catalog, CAT_MSGS, 1006, "Warning: %s is an alias for type %s.\nSetting inline display for %s.\n"), 
		      nextfield, key, key);
	}
	if (tmp = (AttachTag *)retrieve_link(inline_types, key, ci_strcmp)) {
	    x = TRUE;
	}
	if (!tmp && !(tmp = zmNew(AttachTag))) {
	    error(SysErrWarning, catgets( catalog, CAT_MSGS, 1011, "Cannot allocate INLINE tag" ));
	    free_elems(fields);
	    return;
	}
	if (!x) {
	    tmp->l_name = savestr(nextfield);
	    insert_link(&inline_types, tmp);
	}
    }

    free_elems(fields);
}

static void
parse_attach_keys(n, fields, override)
int n;		/* argc */
char **fields;	/* argv */
int override;
{
    AttachKey *tmp, **klist;
    int x = FALSE;
    const char	*newTypeStr;

    if (!ci_strcmp(fields[0], "PATH")) {
	parse_attach_path(n, fields, !override);
    } else if (!ci_strcmp(fields[0], "DEFAULT")) {
	parse_attach_default(n, fields, override);
    } else if ((klist = &type_keys, !ci_strcmp(fields[0], "TYPE")) ||
		(klist = &code_keys, !ci_strcmp(fields[0], "CODE"))) {
	if (n < 4) {
	    free_elems(fields);
	    return;
	}
	ci_strcpy(fields[0], fields[0]);
	Debug("Setting new attachment %s programs\n" , fields[0]);
	if ((klist == &type_keys) &&
	    ((newTypeStr = get_type_from_alias(fields[1])) != fields[1]))
	    ZSTRDUP(fields[1], newTypeStr);
	/* XXX should remove or complain about spaces here */
	/* ci_strcpy(fields[1], fields[1]); */
	if (tmp = 
	    (AttachKey *)retrieve_link(*klist, fields[1], ci_strcmp)) {
	    Debug("Found existing attachment %s %s\n" , fields[0], fields[1]);
	    if (tmp->encode.program && bool_option(VarWarning, "attach"))
		print(catgets( catalog, CAT_MSGS, 103, "Warning: Attachment %s %s multiply defined.\n" ),
		    klist == &type_keys? catgets( catalog, CAT_MSGS, 104, "type" ) : catgets( catalog, CAT_MSGS, 105, "encoding" ),
		    tmp->key_string);
	    x = TRUE;
	    if (override) {
		xfree(tmp->decode.program);
		tmp->decode.program = NULL;
		tmp->decode.checked = 0;
		xfree(tmp->encode.program);
		tmp->encode.program = NULL;
		tmp->encode.checked = 0;
		xfree(tmp->description);
		tmp->description = NULL;
	    }
	} else if (!(tmp = zmNew(AttachKey))) {
	    error(SysErrWarning, catgets( catalog, CAT_MSGS, 106, "Cannot allocate %s" ),
		klist == &type_keys? "type_key" : "code_key");
	    free_elems(fields);
	    return;
	} else
	    Debug("Allocated new attachment %s %s\n" , fields[0], fields[1]);
	if (!tmp->key_string)
	    tmp->key_string = fields[1];
	else
	    xfree(fields[1]);
	/* Positions of the encoding/decoding strings are reversed
	 * depending on whether we're adding a type or a code key.
	 *  Type:
	 *	Viewer == fields[2] --> decode.program
	 *	Editor == fields[3] --> encode.program
	 *  Code:
	 *	Encoding == fields[2] --> encode.program
	 *	Decoding == fields[3] --> decode.program
	 */
	if (!tmp->decode.program)
	    tmp->decode.program = fields[klist == &type_keys ? 2 : 3];
	else
	    xfree(fields[klist == &type_keys ? 2 : 3]);
	if (!tmp->encode.program)
	    tmp->encode.program = fields[klist == &code_keys ? 2 : 3];
	else
	    xfree(fields[klist == &code_keys ? 2 : 3]);
	if (!tmp->description)
	    tmp->description = fields[4];
	else
	    xfree(fields[4]);
	if (!x)
	    insert_link(klist, tmp);
	if (n > 4)
	    free_elems(&fields[5]);
	/* Set a default encoding if there isn't one. This will be 
	 * overridden if an ENCODE command for the type is found later */
	if (!ci_strcmp(fields[0], "TYPE") && (!tmp->use_code)) {
	    char	*encodingStr;
	    encodingStr = GetDefaultEncodingStr(tmp->key_string);
	    if (!encodingStr)
		encodingStr = "none";
	    tmp->use_code = savestr(encodingStr);
	}
	xfree(fields[0]);
    } else if (!ci_strcmp(fields[0], "ALIAS")) {
	parse_attach_alias(n, fields, override);
    } else if (!ci_strcmp(fields[0], "ENCODE")) {
	parse_attach_encode(n, fields, override);
    } else if (!ci_strcmp(fields[0], "BITMAP")) {
	parse_attach_bitmap(n, fields, override);
    } else if (!ci_strcmp(fields[0], "INLINE")) {
	parse_attach_inline(n, fields, override);
#if defined (MSDOS)
   } else if (!ci_strcmp(fields[0], "WINTYPE")) {
	parse_attach_mactype(n, fields, override);
#endif /* MSDOS */
#if defined (MAC_OS)
    } else if (!ci_strcmp(fields[0], "MACTYPE")) {
	parse_attach_mactype(n, fields, override);
    } else if (!ci_strcmp(fields[0], "MIME2MACTYPE")) {
	parse_attach_mactype(n, fields, override);   
#endif /* MAC_OS */
#ifdef OZ_DATABASE
    } else if (!ci_strcmp(fields[0], "irix2mime")) {
	parse_attach_irix2mime(n, fields, override);
    } else if (!ci_strcmp(fields[0], "mime2irix")) {
	parse_attach_mime2irix(n, fields, override);
#endif /* OZ_DATABASE */
    } else /* How did we get here? */
	free_elems(fields);
}

static void
add_default_type_keys()
{
    if (!type_keys) {
	AttachKey *tmp;
	if (!(tmp = zmNew(AttachKey))) {
	    error(SysErrWarning, catgets( catalog, CAT_MSGS, 98, "Cannot allocate type_key" ));
	    return;
	}
	bzero((char *) tmp, sizeof(struct AttachKey));
	tmp->key_string = savestr(MimeTypeStr(TextPlain));
	tmp->use_code = savestr("None");
	insert_link(&type_keys, tmp);
#ifdef NOT_NOW
	if (!(tmp = zmNew(AttachKey))) {
	    error(SysErrWarning, catgets( catalog, CAT_MSGS, 98, "Cannot allocate type_key" ));
	    return;
	}
	bzero((char *) tmp, sizeof(struct AttachKey));
	tmp->key_string = savestr(X_ZM_MIME);
	/* (void) ci_strcpy(tmp->key_string, tmp->key_string); */
	tmp->use_code = savestr("none");
	insert_link(&type_keys, tmp);
#endif /* NOT_NOW */
    }

    if (!type_aliases) {
	AttachTypeAlias *tmp;
	if (!(tmp = zmNew(AttachTypeAlias))) {
	    error(SysErrWarning, catgets( catalog, CAT_MSGS, 816, "Cannot allocate type_alias" ));
	    return;
	}
	bzero((char *) tmp, sizeof(struct AttachTypeAlias));
	tmp->alias_string = savestr("text");
	tmp->type_key = savestr(MimeTypeStr(TextPlain));
	insert_link(&type_aliases, tmp);
    }
}

/*
 *  Insert some default types and load the rest from the specified file
 *  If override is non-zero, override any old entries
 *				CML Wed May 26 17:20:16 1993
 */
static void
load_attach_keys(file, override)
char *file;
int override;
{
    FILE *fp;

    if (file && (fp = fopen(file, "r"))) {

	int line_no = 0;
	char buf[MAXPATHLEN];

	while (next_line(buf, sizeof buf, fp, &line_no)) {
	    /* Use mk_argv() to preserve quoting where present */
	    int n;
	    char ** const fields = mk_argv(buf, &n, FALSE);
	    
	    if (fields) {
		n = stripq(fields); /* n from mk_argv() may be -1, reset it */
		parse_attach_keys(n, fields, override); /* Frees or saves elements */
		free(fields);
	    }
	}
	(void) fclose(fp);
    }
    
    add_default_type_keys();
}

void
list_attach_keys(akey, brief, compose_only)
AttachKey *akey;
int brief;
int compose_only;
{
    AttachKey *tmp = akey;
    char *buf1, *buf2, **names = DUBL_NULL, *what;

    if (!tmp)
	return;

    if (brief) {
	int n = 0;

	if ((compose_only) && (akey == code_keys))
	    n = get_compose_code_keys(&names);
	else if ((compose_only && akey == type_keys))
	    n = get_compose_type_keys(&names);
	else
	    do {
		if ((n = catv(n, &names, 1, unitv(tmp->key_string))) <= 0)
		    break;
	    } while ((tmp = (AttachKey *)tmp->k_link.l_next) != akey);

	if (names) {
	    if (n > 0)
		columnate(n, names, 0, TRPL_NULL);
	    free_vec(names);
	}
    } else {
	int len0, len1, adj1, len2, adj2;

	if (akey == type_keys)
	    what = "TYPE";
	else if (akey == code_keys)
	    what = "CODE";
	else
	    return;
	do {
	    char *pi1, *po1, *pi2, *po2;
	    /* Hack!  Need to reverse fields depending on type or code */
	    if (akey == type_keys) {
		if (tmp->decode.program &&
			check_attach_prog(&tmp->decode, FALSE) != 0)
		    continue;
		if (tmp->encode.program &&
			check_attach_prog(&tmp->encode, FALSE) != 0)
		    continue;
		buf1 = tmp->decode.program;
		pi1 = buf1? (tmp->decode.pipein? "|" :
			    (tmp->decode.zmcmd? ":" : "")) : "";
		po1 = buf1 && tmp->decode.pipeout? "|" : "";
		buf2 = tmp->encode.program;
		pi2 = buf2? (tmp->encode.pipein? "|" :
			    (tmp->encode.zmcmd? ":" : "")) : "";
		po2 = buf2 && tmp->encode.pipeout? "|" : "";
	    } else {
		if (tmp->decode.program &&
			check_attach_prog(&tmp->decode, TRUE) != 0)
		    continue;
		if (tmp->encode.program &&
			check_attach_prog(&tmp->encode, TRUE) != 0)
		    continue;
		buf1 = tmp->encode.program;
		pi1 = buf1? (tmp->encode.pipein? "|" :
			    (tmp->encode.zmcmd? ":" : "")) : "";
		po1 = buf1 && tmp->encode.pipeout? "|" : "";
		buf2 = tmp->decode.program;
		pi2 = buf2? (tmp->decode.pipein? "|" :
			    (tmp->decode.zmcmd? ":" : "")) : "";
		po2 = buf2 && tmp->decode.pipeout? "|" : "";
	    }
	    if (buf1) {
		buf1 = savestr(zmVaStr("\"%s%s%s\"", pi1, buf1, po1));
	    } else
		buf1 = savestr("None");
	    if (!buf1)
		break;
	    if (buf2) {
		buf2 = savestr(zmVaStr("\"%s%s%s\"", pi2, buf2, po2));
	    } else
		buf2 = savestr("None");
	    if (!buf2) {
		xfree(buf1);
		break;
	    }
	    len0 = strlen(tmp->key_string);
	    len1 = strlen(buf1);
	    len2 = strlen(buf2);
	    adj1 = adj2 = -23;	/* Left-justify in 23 columns by default */
	    if (len0 > 10)
		len0 -= 10;
	    else
		len0 = 0;
	    /* len0 is always the amount of encroachment from the left
	     * upon the next field whose width is to be computed.
	     */
	    if (len0 + len1 < 23) {
		adj1 += len0;
		len0 = 0;
	    } else {
		adj1 = -len1;
		len0 = (len0 + len1) - 23;
	    }
	    if (len0 + len2 < 23) {
		adj2 += len0;
		len0 = 0;
	    } else {
		adj2 = -len2;
		len0 = (len0 + len2) - 23;
	    }
	    if (len0) {
		/* We're still encroaching ... try to back off leftward */
		if (len0 + len1 + adj1 < 0)
		    adj1 += len0;
		else
		    adj1 = -len1;
	    }

	    ZmPagerWrite(cur_pager,
		zmVaStr("%s %-10s ", what, tmp->key_string));
	    ZmPagerWrite(cur_pager, zmVaStr("%*s ", adj1, buf1));
	    ZmPagerWrite(cur_pager, zmVaStr("%*s ", adj2, buf2));
	    ZmPagerWrite(cur_pager,
		zmVaStr("\"%s\"\n",
		    tmp->description? tmp->description : tmp->key_string));

	    if (tmp->use_code)
		ZmPagerWrite(cur_pager,
		    zmVaStr("%s %s %s\n",
			catgets(catalog, CAT_MSGS, 117, "ENCODE"),
			tmp->key_string,
			tmp->use_code));
	    if (tmp->bitmap_filename)
		ZmPagerWrite(cur_pager,
		    zmVaStr("BITMAP\t%s\t%s\n",
			tmp->key_string, tmp->bitmap_filename));
	    if (retrieve_link(inline_types, tmp->key_string, ci_strcmp))
		ZmPagerWrite(cur_pager,
		    zmVaStr("INLINE\t%s\n", tmp->key_string));

	    xfree(buf1);
	    xfree(buf2);
	} while ((tmp = (AttachKey *)tmp->k_link.l_next) != akey);
    }
}

void
list_attach_type_aliases(atypeAlias, brief, directive)
AttachTypeAlias *atypeAlias;
int brief;
char *directive;
{
    AttachTypeAlias *tmp = atypeAlias;
    char **names = DUBL_NULL;
    
    if (!tmp || !directive)
	return;
    
    if (brief) {
	int n = 0;
	
	do {
	    if ((n = catv(n, &names, 1, unitv(tmp->alias_string))) <= 0)
		break;
	} while ((tmp = (AttachTypeAlias *)tmp->a_link.l_next) != atypeAlias);
	if (names) {
	    if (n > 0)
		columnate(n, names, 0, TRPL_NULL);
	    free_vec(names);
	}
    } else {
#if defined (MAC_OS) || defined (MSDOS)
	if ((atypeAlias != type_aliases) && (atypeAlias != type_mactypes))
#else
	if (atypeAlias != type_aliases)
#endif /* MAC_OS || MSDOS */
	    return;
	do {
#ifdef NOT_NOW
	    /* What the heck was this supposed to accomplish, anyway? */
	    if (tmp->type_key &&
		!retrieve_link(type_keys, tmp->type_key, ci_strcmp))
		continue;
#endif /* NOT_NOW */
	    ZmPagerWrite(cur_pager,
		zmVaStr("%s %s %s\n", directive, tmp->alias_string,
		   tmp->type_key ? tmp->type_key : "None"));
	} while ((tmp = (AttachTypeAlias *)tmp->a_link.l_next) != atypeAlias);
    }
}

/*
 * Retrieve attachment mappings from the attach_types files.  On the first
 * call to this routine, the file is loaded into internal data structures.
 * Thereafter those structures are looked up and re-used.  The file is not
 * re-parsed unless this function is called again with want_encoding < 0.
 */
AttachKey *
get_attach_keys(want_encoding, attach, keyword)
int want_encoding;
Attach *attach;
const char *keyword;
{
    char buf[MAXPATHLEN], *p, **names = DUBL_NULL;
    int i, x;
    AttachKey *akey = 0;
    AttachKey *returnKey;

    if (!keys_loaded || want_encoding < 0) {
	if (want_encoding < 0) {
	    xfree(attach_path);
	    attach_path = NULL;
	    free_attach_keys(&type_keys);
	    free_attach_keys(&code_keys);
	    free_attach_type_aliases(&type_aliases);
#if defined (MAC_OS) || defined (MSDOS)
	    free_attach_type_aliases(&type_mactypes);
#endif /* MAC_OS || MSDOS */
#ifdef OZ_DATABASE
	    free_attach_oz();
#endif /* OZ_DATABASE */
	    free_attach_tags(&inline_types);
	}
/* 11/14/93 GF  want to update rsrc map to grab new icons, if any */
#ifdef MAC_OS
	gui_OpenIconResFile();
#endif /* MAC_OS */
	if (p = value_of(VarAttachTypes)) {
	    skipspaces(0);
	    (void)no_newln(p); /* Strips trailing spaces */
	    for (i = 0; buf[i] = p[i]; i++) {
		if (isspace(buf[i])) {
		    buf[i] = ',';
		    while (p[i+1] && isspace(p[i+1]))
			i++;
		}
	    }
	    (void)strcpy(buf, zmVaStr("{%s,%s}", encodings_file, buf));
	} else
	    (void)strcpy(buf, encodings_file);
	if ((x = filexp(buf, &names)) < 0) {
	    error(SysErrWarning, catgets( catalog, CAT_MSGS, 119, "Cannot find \"%s\"" ), buf);
	    return 0;
	}
	/* Initialize default path */
	if (!attach_path) {
	    p = buf + Strcpy(buf, zmlibdir);
	    (void) strcpy(p, "/bin");
	    attach_path = savestr(p = buf);
	} else
	    p = NULL;
	if (x == 0)
	    load_attach_keys(NULL, TRUE);
	else
	    for (i = 0; i < x; i++)
		load_attach_keys(names[i], TRUE);
	/* If no path loaded, append the environment PATH */
	if (p && strcmp(attach_path, p) == 0 && (p = getenv("PATH"))) {
	    char *atp = (char *) malloc(strlen(attach_path)+strlen(p)+2);
	    if (atp) {
		coloncat(atp, attach_path, p);
		xfree(attach_path);
		attach_path = atp;
	    }
	}
	keys_loaded = 1;
	free_vec(names), names = DUBL_NULL;
	ZmCallbackCallAll("", ZCBTYPE_ATTACH, 0, NULL);
    }
    if (want_encoding < 0 || (!keyword && !attach))
	return 0;
    if (want_encoding) {
	const char *cp;
	akey = code_keys;
	if (!(cp = keyword? keyword : 
	      (attach ? attach->encoding_algorithm: (char *) NULL)))
	    return 0;
	strcpy(buf, cp);
    } else {
	const char *cp;
	akey = type_keys;
	if (!(cp = keyword? get_type_from_alias(keyword) : 
	      (attach ? attach_data_type(attach) : (char *) NULL)))
	    return 0;
	strcpy(buf, cp);
    }
    if (strcmp(buf, "?") == 0 &&
	    none_p(glob_flags, REDIRECT|NO_INTERACT|IS_FILTER)) {
	list_attach_keys(akey, TRUE, TRUE);
	return 0;
    }
    returnKey = (AttachKey *)retrieve_link(akey, buf, ci_strcmp);
    /*
     * Just in case ... try stripping prefixes
     */
    if (!returnKey && !ci_strncmp(buf, "x-zm-", 5))
	returnKey = (AttachKey *)retrieve_link(akey, buf+5, ci_strcmp);
    if (!returnKey && !ci_strncmp(buf, "x-", 2))
	returnKey = (AttachKey *)retrieve_link(akey, buf+2, ci_strcmp);
    if (!returnKey && (akey==type_keys))
	returnKey = 
	    (AttachKey *)retrieve_link(akey, 
				       MimeTypeStr(GetClosestMimeType(buf)), 
				       ci_strcmp);
    return returnKey;
}

static int
save_attach_keys(file)
char *file;
{
    AttachKey *tmp;
    AttachTypeAlias *tmpTypeAlias;
    AttachTag *tmpTag;
    FILE *fp;
    int x = 1;
    char *path = getpath(file, &x);

    if (x == -1) {
	error(UserErrWarning, "%s: %s", file, path);
	return -1;
    } else if (x != 0) {
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 142, "\"%s\" is a directory." ), path);
	return -1;
    }
    if (!Access(path, F_OK) &&
	    ask(WarnNo, catgets( catalog, CAT_MSGS, 121, "Overwrite \"%s\"?" ), file) != AskYes)
	return -1;
    if (!(fp = fopen(path, "w"))) {
        error(SysErrWarning, catgets( catalog, CAT_SHELL, 398, "Cannot open \"%s\"" ), file);
        return -1;
    }
#if defined(MAC_OS)
    gui_set_filetype(AttachTypeFile, path, NULL);
#endif
    if (attach_path)
	(void) fprintf(fp, "PATH %s\n\n", attach_path);
    if (tmp = type_keys)
	do {
	    char *p, *pi1, *po1, *pi2, *po2;
	    if (tmp->decode.program &&
		    (p = check_attach_prog(&tmp->decode, FALSE))) {
		print(p, tmp->key_string);
		print_more("\n");
		continue;
	    }
	    pi1 = tmp->decode.program? (tmp->decode.pipein? "|" :
			(tmp->decode.zmcmd? ":" : "")) : "";
	    po1 = tmp->decode.program && tmp->decode.pipeout? "|" : "";
	    if (tmp->encode.program &&
		    (p = check_attach_prog(&tmp->encode, FALSE))) {
		print(p, tmp->key_string);
		print_more("\n");
		continue;
	    }
	    pi2 = tmp->encode.program? (tmp->encode.pipein? "|" :
			(tmp->encode.zmcmd? ":" : "")) : "";
	    po2 = tmp->encode.program && tmp->encode.pipeout? "|" : "";
	    if (tmp->decode.program || tmp->encode.program) {
		/* Declaration and name */
		(void) fprintf(fp, "TYPE\t%s\t", tmp->key_string);
		/* Viewer */
		(void) fprintf(fp, "\"%s%s%s\"\t",
		    pi1,
		    tmp->decode.program? tmp->decode.program : "None",
		    po1);
		/* Editor */
		(void) fprintf(fp, "\"%s%s%s\"",
		    pi2,
		    tmp->encode.program? tmp->encode.program : "None",
		    po2);
		/* Comment (Automatch string) */
		if (tmp->description && *tmp->description)
		    (void) fprintf(fp, "\t\"%s\"", tmp->description);
		(void) fputc('\n', fp);
	    }
	    if (tmp->use_code)
		(void) fprintf(fp, "ENCODE\t%s\t%s\n",
		    tmp->key_string, tmp->use_code);
	    if (tmp->bitmap_filename)
		(void) fprintf(fp, "BITMAP\t%s\t%s\n",
		    tmp->key_string, tmp->bitmap_filename);
	} while ((tmp = (AttachKey *)tmp->k_link.l_next) != type_keys);
    if (tmpTypeAlias = type_aliases) {
	(void) fputc('\n', fp);
	do {
	    if (!tmpTypeAlias->alias_string)
		continue;
	    fprintf(fp, "%s %s %s\n", "ALIAS", tmpTypeAlias->alias_string,
		    tmpTypeAlias->type_key);
	} while ((tmpTypeAlias = 
		  (AttachTypeAlias *)tmpTypeAlias->a_link.l_next) != 
		 type_aliases);
    }
#if defined(MAC_OS) || defined (MSDOS)
    if (tmpTypeAlias = type_mactypes) {
	(void) fputc('\n', fp);
	do {
	    if (!tmpTypeAlias->alias_string)
		continue;
#if defined (MAC_OS)
	    fprintf(fp, "%s %s %s\n", "MACTYPE", tmpTypeAlias->alias_string,
		    tmpTypeAlias->type_key);
#endif /* MAC_OS */
#if defined (MSDOS)
	    fprintf(fp, "%s %s %s\n", "WINTYPE", tmpTypeAlias->alias_string,
		    tmpTypeAlias->type_key);
#endif /* MSDOS */

	} while ((tmpTypeAlias = 
		  (AttachTypeAlias *)tmpTypeAlias->a_link.l_next) != 
		 type_mactypes);
    }
#endif /* MAC_OS || MSDOS */
    if (tmpTag = inline_types) {
	(void) fputc('\n', fp);
	do {
	    (void) fprintf(fp, "INLINE\t%s\n", tmpTag->l_name);
	} while ((tmpTag = tmpTag->l_next) != inline_types);
    }
    if (tmp = code_keys) {
	(void) fputc('\n', fp);
	do {
	    char *p, *pi1, *po1, *pi2, *po2;
	    if (tmp->decode.program &&
		    (p = check_attach_prog(&tmp->decode, TRUE))) {
		print(p, tmp->key_string);
		print_more("\n");
		continue;
	    }
	    pi1 = tmp->decode.program? (tmp->decode.pipein? "|" :
			(tmp->decode.zmcmd? ":" : "")) : "";
	    po1 = tmp->decode.program && tmp->decode.pipeout? "|" : "";
	    if (tmp->encode.program &&
		    (p = check_attach_prog(&tmp->encode, TRUE))) {
		print(p, tmp->key_string);
		print_more("\n");
		continue;
	    }
	    pi2 = tmp->encode.program? (tmp->encode.pipein? "|" :
			(tmp->encode.zmcmd? ":" : "")) : "";
	    po2 = tmp->encode.program && tmp->encode.pipeout? "|" : "";
	    /* Declaration and name */
	    (void) fprintf(fp, "CODE\t%s", tmp->key_string);
	    /* Encoder */
	    (void) fprintf(fp, "\t\"%s%s%s\"",
		pi2,
		tmp->encode.program? tmp->encode.program : "None",
		po2);
	    /* Decoder */
	    (void) fprintf(fp, "\t\"%s%s%s\"",
		pi1,
		tmp->decode.program? tmp->decode.program : "None",
		po1);
	    /* Comment (Automatch string) */
	    if (tmp->description && *tmp->description)
		(void) fprintf(fp, "\t\"%s\"", tmp->description);
	    (void) fputc('\n', fp);
	} while ((tmp = (AttachKey *)tmp->k_link.l_next) != code_keys);
    }
    (void) fprintf(fp, "\nDEFAULT %s\n", dflt_encode);
#ifdef OZ_DATABASE
    save_attach_oz(fp);
#endif /* OZ_DATABASE */
    return fclose(fp) != EOF;
}

/*
 *	Dereference an alias. Return the appropriate type name or
 *	the pointer which was passed in if the alias is unknown.
 */
const char *
dereference_alias(aliasName, aliasList, caseSensitiveFlag)
    const char		*aliasName;
    AttachTypeAlias	*aliasList;
    int			caseSensitiveFlag;
{
    AttachTypeAlias *tmpTypeAlias;
    
    if (aliasName && aliasList) {
	/* DON'T initialize keys here.  If we do so after testing
	 * for keys_loaded, we loop:
	 * load_attach_keys->parse_attach_keys->get_type_from_alias->
	 * get_attach_keys->load_attach_keys    
	 * Also, it makes sense semantically to just reference only the
	 * aliases that have already been loaded, if we are midway 
	 * through loading.  CML Wed Dec 15 14:31:45 1993
	 */
	if (tmpTypeAlias = 
	    (AttachTypeAlias *)retrieve_link(aliasList, aliasName, 
					     caseSensitiveFlag ?
					     strcmp :
					     ci_strcmp))
	    return(tmpTypeAlias->type_key);
    }
    return aliasName;
}

/*	Dereference an alias name for the type of an incoming body part */
const char *
get_type_from_alias(aliasName)
    const char	*aliasName;
{
    return(dereference_alias(aliasName, type_aliases, 0));
}

#if defined (MAC_OS) || defined (MSDOS)
/*	Dereference a mactype to get the type for an outgoing body part */
char *
get_type_from_mactype(mactype)
    const char	*mactype;
{
    return(dereference_alias(mactype, type_mactypes, 1));
}
#endif /* MAC_OS || MSDOS */

/* 
 * Generate a list of the type names which can be used for composing messages.
 * Place it in the passed in address.
 * Return a count of the number of values provided.
 */

int
get_compose_type_keys(keysPtr)
    char	***keysPtr;
{
    char	**keys, **xmime, **temp;
    int		cnt = 0;
    
    if (keysPtr) {
	/* Make sure keys have been initialized */
	if (!keys_loaded)
	    (void) get_attach_keys(0, (Attach *)0, NULL);
	keys = link_to_argv(type_keys);    
	/* Bart: Thu Mar 25 18:41:43 PST 1993
	 * Don't allow creation of X-Zm-Mime attachments.
	 * Carlyn: Thu Jun 10 17:00:13 1993
	 * or nonstandard MIME ones, or multipart ones
	 */
	if (boolean_val("sun_attachment")) {
	    cnt = vlen(keys);
	    qsort((char *) keys, cnt, sizeof(char *),
		    (int (*)NP((CVPTR, CVPTR))) strptrcmp);
	} else {
	    for (xmime = keys; xmime && *xmime; ) {
		if ((ci_strcmp(*xmime, X_ZM_MIME) == 0) ||
		    (GetMimeType(*xmime) < ExtendedMimeType) ||
		    (GetMimeType(*xmime) == MessagePartial) ||
		    (GetMimeType(*xmime) == MessageExternalBody) ||
		    (GetMimePrimaryType(*xmime) == MimeMultipart)) {
		    xfree(*xmime);
		    temp = xmime;
		    while (temp[0] = temp[1])
			temp++;
		} else {
		    xmime++;
		    cnt++;
		}
	    }
	    SortMimeTypes(keys);
	}
	*keysPtr = keys;
    }
    return cnt;
}    

/* 
 * Generate a list of the type names which can be used for creating messages.
 * These are the types for which there are compose rules.
 * Place the list in the passed in address.
 * Return a count of the number of values provided.
 */

int
get_create_type_keys(keysPtr)
     char	***keysPtr;
{
    char	**keys, **xmime, **temp;
    int		cnt = 0;
    AttachKey	*akey = 0;
  
    if (keysPtr) {
	cnt = get_compose_type_keys(&keys);
	for (xmime = keys; xmime && *xmime; ) {
	    /* remove if no compose rule */
	    if (!(akey = get_attach_keys(FALSE, (Attach *) 0, *xmime)) ||
		    !akey->encode.program ||
		    check_attach_prog(&akey->encode, FALSE) != 0) {
		xfree(*xmime);
		temp = xmime;
		while (temp[0] = temp[1])
		    temp++;
		cnt--;
	    } else {
		xmime++;
	    }
	}
	*keysPtr = keys;
    }
    return cnt;
}    

/* 
 * Generate a list of the type names which can be used for displaying messages.
 * Place it in the passed in address.
 * Returns a count of the number of values provided.
 */

int
get_display_type_keys(keysPtr) 
    char	***keysPtr;
{
    char	**keys;
    
    if (keysPtr) {
	/* Make sure keys have been initialized */
	if (!keys_loaded)
	    (void) get_attach_keys(0, (Attach *)0, NULL);
	keys = link_to_argv(type_keys);    
	SortMimeTypes(keys);
	(void) vcat(&keys, unitv(catgets(catalog, CAT_MSGS, 826, "Unknown")));
	*keysPtr = keys;
    }
    return vlen(keys);
}

/* 
 * Generate a list of the encoding names which can be used for composing 
 * messages.  Place it in the passed in address.
 * Return a count of the number of values provided.
 */
int
get_compose_code_keys(keysPtr)
    char	***keysPtr;
{
    char	**keys, **xmime, **temp;
    int		cnt = 0, sunstyle = boolean_val("sun_attachment");
    
    if (keysPtr) {
	/* Make sure keys have been initialized */
	if (!keys_loaded)
	    (void) get_attach_keys(0, (Attach *)0, NULL);
	keys = link_to_argv(code_keys);    
	/* There may not be any code keys defined if there was no
	 * attach.types found
	 */
	if (keys) {
	    for (xmime = keys; *xmime; ) {
		AttachKey *ak;
		if (!sunstyle &&
		    GetMimeEncoding(*xmime) < ExtendedMimeEncoding ||
			(ak = get_attach_keys(1, (Attach *)0, *xmime)) &&
			(!ak->encode.program ||
			check_attach_prog(&ak->encode, TRUE) != 0)) {
		    xfree(*xmime);
		    temp = xmime;
		    while (temp[0] = temp[1])
			temp++;
		} else {
		    xmime++;
		    cnt++;
		}
	    }
	    if (sunstyle)
		qsort((char *) keys, cnt, sizeof(char *),
			(int (*)NP((CVPTR, CVPTR))) strptrcmp);
	    else
		SortMimeEncodings(keys);
	}
	(void) vcat(&keys, unitv("None"));
	cnt++;
	*keysPtr = keys;
    }
    return cnt;
}

/* 
 * Generate a list of the encoding names which can be used for displaying 
 * messages.  Place it in the passed in address.
 * Returns a count of the number of values provided.
 */
int
get_display_code_keys(keysPtr)
    char	***keysPtr;
{
    char	**keys;
    
    if (keysPtr) {
	/* Make sure keys have been initialized */
	if (!keys_loaded)
	    (void) get_attach_keys(0, (Attach *)0, NULL);
	keys = link_to_argv(code_keys);    
	SortMimeEncodings(keys);
	(void) vcat(&keys, unitv("None"));
	(void) vcat(&keys, unitv(catgets(catalog, CAT_MSGS, 826, "Unknown")));
	*keysPtr = keys;
    }
    return vlen(keys);
}

/*
 * Given a type or alias name, return true if it is a known type.
 * Return false otherwise.
 */
int 
is_known_type(name)
    const char *name;
{
    char	buf[256];
    const char	*split, *key;
    int		returnVal = 0;
    
    if (name) {
	/* Make sure keys have been initialized */
	if (!keys_loaded)
	    (void) get_attach_keys(0, (Attach *)0, NULL);
	split = index(name, ';');
	if (split) {
	    (void) strncpy(buf, name, split-name);
	    buf[split-name] = 0;
	    key = buf;
	} else
	    key = name;
	
	returnVal = ((GetMimeType(key) > ExtendedMimeType) ||
		     retrieve_link(type_keys, get_type_from_alias(key),
				   ci_strcmp));
    }
    return(returnVal);
}

/*
 * Given a type or alias name, decide whether we should inline it.
 */
int
is_inline_type(name)
    const char *name;
{
    get_attach_keys(0, 0, 0);	/* make sure config has been read in */
    
    return !(!retrieve_link(inline_types, name, ci_strcmp) &&
	 !retrieve_link(inline_types, get_type_from_alias(name), ci_strcmp));
}

/*
 * Dynamically add or modify attachment types.
 * Usage:
 *	attach [-load file] [-merge file] [-save file] [-rehash]
 *	attach [-code | -type | -path | -encode | -default ] args ...
 *	attach [-help | -?]
 *
 * In the latter case, the args are the same as those in the attach.types
 * file entries, e.g. attach -code encoder decoder comment.
 *
 * Options -load, -merge, -save, and -rehash may be specified multiple
 * times and are processed in the order given.  Only one -code, -type,
 * -path, -encode, or -default option may be used at a time.
 *
 * If no options are specified, the current attachment types and codes
 * are listed.
 */
int
zm_attach(argc, argv, list)
int argc;
char **argv;
struct mgroup *list;
{
    char buf[MAXPATHLEN], *p, **fields = DUBL_NULL;

    if (!keys_loaded)
	get_attach_keys(0, (Attach *)0, NULL);

    while (*++argv && **argv == '-') {
	p = NULL;
	switch (lower(argv[0][1])) {
	    case 'l':	/* load */
		if (argv[1]) {
		    if (keys_loaded) {
			xfree(attach_path);
			p = buf + Strcpy(buf, zmlibdir);
			(void) strcpy(p, "/bin");
			p = attach_path = savestr(buf);
			free_attach_keys(&type_keys);
			free_attach_keys(&code_keys);
			free_attach_type_aliases(&type_aliases);
#if defined (MAC_OS) || defined (MSDOS)
			free_attach_type_aliases(&type_mactypes);
#endif /* MAC_OS || MSDOS */
#ifdef OZ_DATABASE
			free_attach_oz();
#endif /* OZ_DATABASE */
		    }
		}
		/* Fall through */
	    case 'm':	/* merge */
		if (*++argv) {
		    load_attach_keys(*argv, TRUE);
		} else {
		    error(UserErrWarning, catgets( catalog, CAT_MSGS, 182, "attach: no types file specified." ));
		    return -1;
		}
		/* If no path loaded, append the environment PATH */
		if (attach_path == p && (p = getenv("PATH"))) {
		    char *atp = (char *) malloc(strlen(attach_path)+strlen(p)+1);
		    if (atp) {
			coloncat(atp, attach_path, p);
			xfree(attach_path);
			attach_path = atp;
		    }
		}
		ZmCallbackCallAll("", ZCBTYPE_ATTACH, 0, NULL);
	    when 'r':	/* rehash or reset */
		(void) get_attach_keys(-1, (Attach *)0, NULL);
	    when 's':	/* save */
		if (*++argv) {
		    return save_attach_keys(*argv) - in_pipe();
		} else {
		    error(UserErrWarning, catgets( catalog, CAT_MSGS, 183, "attach: no file specified." ));
		    return -1;
		}
	    when 'c': case 't':	case 'p':	/* Create a new entry */
	    case 'a': case 'd': case 'e':	/* or modify entries */
	    case 'i':				/* or tag as inlined */
		if ((argc = vcpy(&fields, argv)) > 1) {
		    Debug("Passing %d args to parse_attach_keys()\n" , argc);
		    if (!keys_loaded)
			get_attach_keys(0, (Attach *)0, NULL);
		    ZSTRDUP(fields[0], &argv[0][1]);
		    parse_attach_keys(argc, fields, TRUE); /* Consumes elements */
		    xfree(fields);
		    ZmCallbackCallAll("", ZCBTYPE_ATTACH, 0, NULL);
		    return 0;
		} else if (argc > 0) {
		    if (lower(argv[0][1]) == 'p')
			wprint("PATH=%s\n", attach_path);
		    else
			error(UserErrWarning, catgets( catalog, CAT_MSGS, 185, "attach: not enough arguments." ));
		} else
		    error(SysErrWarning, catgets( catalog, CAT_MSGS, 186, "attach: couldn't copy arguments" ));
		return -1;
	    when 'h':
		return help(0, "attach", cmd_help);
	    otherwise:
		error(UserErrWarning,
		    catgets( catalog, CAT_MSGS, 187, "attach: unrecognized option \"%s\"" ), argv[0]);
		return -1;
	}
    }
    if (!keys_loaded)
	(void) get_attach_keys(0, (Attach *)0, NULL);
    if (argc == 1) {
	ZmPagerStart(PgText);
	ZmPagerWrite(cur_pager, "PATH=");
	ZmPagerWrite(cur_pager, attach_path);
	ZmPagerWrite(cur_pager, "\n");
	ZmPagerWrite(cur_pager,
	    catgets(catalog, CAT_MSGS, 188, "\nAttachment types:\n"));
	list_attach_keys(type_keys, FALSE, FALSE);
 	ZmPagerWrite(cur_pager,
	    catgets(catalog, CAT_MSGS, 835, "\nAttachment type aliases:\n"));
 	list_attach_type_aliases(type_aliases, FALSE, "ALIAS");

#if defined(MAC_OS)
 	ZmPagerWrite(cur_pager,
	    catgets(catalog, CAT_MSGS, 905, "\nMacintosh type to MIME type mappings:\n"));
 	list_attach_type_aliases(type_mactypes, FALSE, "MACTYPE");
#endif /* MAC_OS */

#if defined (MSDOS)
 	ZmPagerWrite(cur_pager,
	    catgets(catalog, CAT_MSGS, 906, "\nWindows type to MIME type mappings:\n"));
 	list_attach_type_aliases(type_mactypes, FALSE, "WINTYPE");
#endif /* MSDOS */

	ZmPagerWrite(cur_pager,
	    catgets(catalog, CAT_MSGS, 189, "\nAttachment encodings:\n"));
	list_attach_keys(code_keys, FALSE, FALSE);

	ZmPagerStop(cur_pager);
    }
    return 0 - in_pipe();
}
