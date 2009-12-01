/* attach.h	Copyright 1992 Z-Code Software Corp. */

/*
 * $Revision: 2.59 $
 * $Date: 1996/08/07 05:21:57 $
 * $Author: schaefer $
 */

#ifndef _ATTACH_H_
#define _ATTACH_H_

#include "zcunix.h"
#include "gui_def.h"
#include "zctype.h"	/* Has to come after gui_def.h (sigh) */
#include "zccmac.h"	/* Has to come after gui_def.h (sigh) */
#include "linklist.h"
#include "dpipe.h"
#if defined( MSDOS )
# ifndef ZC_INCLUDED_STDDEF_H
#  include <stddef.h>
#  define ZC_INCLUDED_STDDEF_H
# endif /* ZC_INCLUDED_STDDEF_H */
#endif /* MSDOS */
#include "mime.h"
#include <stdio.h>


typedef struct Attach {
    /* These fields are used for processing the attachment (display, etc.) */
    struct link a_link;
#define    a_name a_link.l_name		/* Working file name */
    u_long a_flags;			/* flag bits for internal state */
    /* These fields hold the data transmitted with the attachment */
    u_long content_offset;		/* offset where attachment begins */
    u_long content_length;		/* required length in bytes */
    u_long content_lines;		/* optional length in lines */
    char  *content_type;		/* required type as transported */
    char  *content_name;		/* optional name */
    char  *content_abstract;		/* optional summary */
    char  *encoding_algorithm;		/* required if encoded */
    /* mime stuff */
    /* When reading a message part, raw data is placed in these strs */
    char	*mime_encoding_str; 	
    char	*mime_content_id;	/* optional content id */
    /* enum values, filled in from raw data in above strings */
    mimeType	mime_type;		/* MIME content type */
    mimeEncoding mime_encoding;
    mimeCharSet	mime_char_set;		/* character set number */	
    /* parameters of content-type, filled in from raw data in above strings */
    char	*mime_content_type_str;	/* may not really need this one,
					   unless we make sure we fill it
					   in properly right away */
    char	*orig_mime_content_type_str; /* We keep these around so we 
						can put them in the index. 
						Yes, this code will be 
						restructured real soon now */
    char	*orig_content_name;
    mimeContentParams	content_params;
    /* These two fields are tentative until we complete the specs */
    char  *decoding_hint;
    char  *data_type;			/* required type of original file
					 * NOTE - the attach_data_type
					 * macro below should normally be 
					 * used to reference this, as it will 
					 * dereference aliases */
} Attach;

#define attach_data_type(x)	(get_type_from_alias((x)->data_type))

#define AT_TEMPORARY	ULBIT(0)	/* Using a temporary file as a_name */
#define AT_NAMELESS	ULBIT(1)	/* Original attachment had no name */
#define AT_PRIMARY	ULBIT(2)	/* This is the "primary" body part */
#define AT_TRY_INLINE	ULBIT(3)	/* Show inline, assuming we know how */
#define AT_NOT_INLINE	ULBIT(4)	/* Never show inline, even if we can */
#define AT_DELETE	ULBIT(5)	/* Marked for removal on next update */
#define AT_OMIT		ULBIT(6)	/* Temporarily marked for omission in next copy_msg */
#define AT_BYTECOUNTED	ULBIT(7)	/* A byte-counted bodypart, e.g. SVR4 */
#define AT_UUCODECOMPAT	ULBIT(8)	/* Nevermind all that open standard crap, be compatible */
#define AT_MACBINCOMPAT	ULBIT(9)	/* Nevermind all that open standard crap, be compatible */
#define AT_CHARSET_OK	ULBIT(10)	/* No C3 charset conversion needed */

#define AT_EXPLODED	ULBIT(11)	/* Multipart recursively expanded */

struct Compose;

extern const char *add_attachment P((struct Compose *compose, const char *file,
                       const char *type, const char *desc, const char *encode,
				     u_long flags, Attach **newAttachPtrPtr));

extern char *parse_encoding();
extern void sign_letter P((register char *, unsigned long, FILE *));

extern void
    preview_attachment P((struct Compose *, const char *)),
    del_attachment P((struct Compose *, char *)),
    free_attachments P((Attach **, int)),
    free_attach P((Attach*, int));

extern void copy_attachments();

typedef struct AttachProg {
    char *program;
    struct dpipe *stream;
    int checked, pipein, pipeout, givename, zmcmd, processp, exitStatus;
} AttachProg;

typedef struct AttachKey {
    struct link k_link;
#define key_string k_link.l_name
    AttachProg encode, decode;
    char *use_code;
#if defined(GUI) && defined(MOTIF)
    ZcIcon bitmap;
#define bitmap_filename bitmap.filename
#define HAS_ATTACH_BITMAP
#else /* !GUI || !MOTIF */
    char *bitmap_filename;
#endif /* !GUI || !MOTIF */
    char *description;
} AttachKey;

typedef struct AttachTypeAlias {
    struct link a_link;
#define alias_string a_link.l_name
    char	*type_key;
} AttachTypeAlias;

extern int	has_simple_encoding P((const Attach *));
extern int	can_show_inline P((const Attach *));
extern int	is_plaintext P((const Attach *));
extern int	is_multipart P((const Attach *));
extern int  	is_readabletext P ((const Attach *));

extern int	is_known_type P((const char *));
extern int	is_inline_type P((const char *));
extern AttachKey *get_attach_keys P((int, Attach *, const char *));
extern void	list_attach_keys P((AttachKey *, int, int));
extern const char *get_type_from_alias P((const char *));
#if defined (MAC_OS) || defined (MSDOS)
extern char	*get_type_from_mactype P((const char *));
#endif /* MAC_OS || MSDOS */
extern int	get_create_type_keys P((char ***));
extern int	get_compose_type_keys P((char ***));
extern int	get_display_type_keys P((char ***));
#define get_create_code_keys(x) (get_compose_code_keys(x))
extern int	get_compose_code_keys P((char ***));
extern int	get_display_code_keys P((char ***));

extern int attach_files P ((struct Compose *));
extern int list_attachments P ((Attach *, Attach *, char ***, int, int));
extern int get_attachments_list P ((Attach *, const char *));
extern int get_comp_attachments_info P ((struct Compose *, char **));
extern int get_attachments_info P ((const Attach *, const char *, const char *));
extern int list_templates P((char ***, const char *, int));

extern char *check_attach_prog P((AttachProg *program, int is_coder));

extern int internal_coder P ((char *));
extern AttachProg *coder_prog P ((int, Attach *, const char *, char *, const char *, int));
extern int pclose_coder P ((const AttachProg *));
extern Attach *lookup_part P ((const Attach *, int, const char *));
extern void check_detach_dir P((void));
/* front end to getdir() that gets the detach_dir */
extern char *get_detach_dir P((void));
extern int test_binary P((const char *));
extern char *new_attach_filename P((struct Compose *, char *));
extern struct dpipe *popen_coder P ((AttachProg *, const char *, const char *, const char *));
extern int content_header P ((char *, Attach **));
extern int handle_coder_err P((int, char *, char *));

enum DetachFlags {
    DetachDisplay   = 1<<0,
    DetachSave      = 1<<1,
    DetachAll       = 1<<2,
    DetachOverwrite = 1<<3,
    DetachCharsetOk = 1<<4,
    DetachExplode   = 1<<5,
    DetachCopy      = 1<<6
};
extern int detach_parts P ((int, int, const char *, const char *, const char *, const char *, unsigned long));

typedef enum {
    HT_ulong, HT_string, HT_encoded, HT_magic, HT_end
} HeaderType;

typedef struct DescribeAttach {
    HeaderType header_type;
    char *header_name;
    int header_length;
    int header_offset;
} DescribeAttach;

#define RFC1154	"X-RFC1154"	/* In case we think of a better keyword */
#define X_ZM_MIME "X-ZM-Mime"	/* Another built-in type */

#undef OffOf
#ifdef XtOffsetOf
# define OffOf(f) XtOffsetOf(Attach,f)
#else /* XtOffsetOf */
# define OffOf(f) (int)(&(((Attach *)0)->f))
#endif /* XtOffsetOf */

#ifdef GUI
extern const char *get_attach_label P((Attach *, int));
#endif /* GUI */

#if defined(GUI) && defined(MOTIF)

typedef struct AttachInfo {
    GuiItem  shell;
    GuiItem  list_w;
    /* text widgets carrying assorted values */
    GuiItem  file_w;     /* Name of file -- supplies Content-Name: header */
    GuiItem  datatype_w; /* Type of file -- supplies Data-Type: header */
    GuiItem  autotype_w; /* Automatically determine file type somehow */
    GuiItem  encoding_w; /* Encoding for shipment -- Encoding-Algorithm: */
    GuiItem  comment_w;  /* Brief description -- Content-Abstract: header */
    int      is_compose; /* Boolean: Is this dialog attached to a compose */
    int	     count;	 /* number of attachments in list */
    GuiItem  action_area;
    int	     comment_modified;  /* Boolean: Was the comment field modified */
} AttachInfo;

#endif /* GUI && MOTIF */

#endif /* _ATTACH_H_ */
