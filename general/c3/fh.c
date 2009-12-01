
#ifdef MAC_OS
#define PATH_SEP 	':'
#define PATH_SEP_STR 	":"
#define PATH_STR	"%s:%s"
#define APPROX_TABLE	"%s:%s:approx-table.txta"
#define DEF_TABLE	"%s:%s:def-%s.txta"
#define BIN_TABLE	"%s:%s:c3tables.bin"
#define NAMES_TABLE	"%s:%s:c3names.txt"
#define FACTOR_TABLE	"%s:%s:factor-%d.txta"

#else /* !MAC_OS */

#define PATH_SEP 	'/'
#define PATH_SEP_STR 	"/"
#define PATH_STR	"%s/%s"
#define APPROX_TABLE	"%s/%s/approx-table.txta"
#define DEF_TABLE	"%s/%s/def-%s.txta"
#define BIN_TABLE	"%s/%s/c3tables.bin"
#define NAMES_TABLE	"%s/%s/c3names.txt"
#define FACTOR_TABLE	"%s/%s/factor-%d.txta"
#endif /* MAC_OS */

static char file_id[] =
    "$Id: fh.c,v 1.19 1995/10/05 05:00:38 liblit Exp $";

/*
 *  WARNING: THIS SOURCE CODE IS PROVISIONAL. ITS FUNCTIONALITY
 *           AND BEHAVIOR IS AT ALFA TEST LEVEL. IT IS NOT
 *           RECOMMENDED FOR PRODUCTION USE.
 *
 *  This code has been produced for the C3 Task Force within
 *  TERENA (formerly RARE) by:
 *  
 *  ++ Peter Svanberg <psv@nada.kth.se>, fax: +46-8-790 09 30
 *     Royal Institute of Technology, Stockholm, Sweden
 *
 *  Use of this provisional source code is permitted and
 *  encouraged for testing and evaluation of the principles,
 *  software, and tableware of the C3 system.
 *
 *  More information about the C3 system in general can be
 *  found e.g. at
 *      <URL:http://www.nada.kth.se/i18n/c3/> and
 *      <URL:ftp://ftp.nada.kth.se/pub/i18n/c3/>
 *
 *  Questions, comments, bug reports etc. can be sent to the
 *  email address
 *      <c3-questions@nada.kth.se>
 *
 *  The version of this file and the date when it was last changed
 *  can be found on (or after) the line starting "static char file_id".
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef MAC_OS
#include <folders.h>
#include <Errors.h>
#endif


/*
 *    Missing from include files
 */



#include "c3_string.h"
#include "euseq.h"
#include "c3.h"
#include "lib.h"
#include "util.h"
#include "fh.h"
#include "parse_util.h"

extern char *
_ccs_id_from_ccs_no P(( const int ));

extern
char *
mktemp P(( char * ));

extern int opt_verbosity;

/*
 *    Limits
 */

#define FH_MAX_FILE_HANDLES	16
#define FH_MAX_PATH_LENGTH	1024

/*
 *    Macros
 */

#define ZERO_MEMORY(mem, length)	bzero((char *) (mem), (length))


/*
 *    The struct containing all info about a file
 */

struct fh_handle {
    FILE   *filep;
    char   *name;
};

typedef struct fh_handle fh_handle;

/*
 * The private data structures
 */

static fh_handle *fhtab[FH_MAX_FILE_HANDLES];
static const char *backup_path = NULL;
 
void
fh_c3_set_backup_table_path(pathname)
    const char *pathname;
{
    backup_path = pathname;
}

/*
 *    Internal functions
 */

char *
_fh_c3_table_path()
{
    char *res;

    res = getenv("C3_TABLE_PATH");
    if (res == NULL) {
	if (backup_path != NULL) 
	    res = getenv(backup_path);
#ifndef MAC_OS
	if (res == NULL) {
	    res = TPATH_DEF; /* From -D in Makefile */
	}
#endif
    }
    return(res);
}

c3bool
_fh_exists(fh)
    fhandle fh;
{
    return(fh < FH_MAX_FILE_HANDLES && fh >= 0 && fhtab[fh] != NULL);
}


c3bool
_fh_user_answer(prompt, str)
    char *prompt;
    char *str;
{
    int answerch, ch;

    fprintf(stderr, prompt, str);
    for (answerch = ch = getchar();
	 ch != '\n' && ch != EOF;
	 ch = getchar())
	;
    return(answerch == 'y' || answerch == 'Y');
}


c3bool
_fh_should_use(path, existing_handling)
    char *path;
    int existing_handling;
{
    int ftype;

    ftype = _c3_file_type(path);
    if (ftype != C3PLATF_FILE_NONE) /* File exists */
    {
	if (ftype == C3PLATF_FILE_REGULAR)
	{
	    if (existing_handling == FH_BACKUP_RENAME_ASK)
	    {
		if (access (path, W_OK) == 0
		    && isatty(fileno(stdin))
		    && _fh_user_answer("Overwrite existing file \"%s\"? ",
				       path))
		{
		    return(TRUE);
		}
	    }
	    else if (existing_handling == FH_BACKUP_RENAME_DESTROY
		     || (existing_handling == FH_BACKUP_RENAME_OVERWRITE
			 && access (path, W_OK) == 0))
	    {
		return(TRUE);
	    }
	}
    }
    else
    {
	return(TRUE);
    }
    return(FALSE);
}


c3bool
_fh_rename(path1, path2, rename_existing)
    char *path1;
    char *path2;
    int rename_existing;
{
    int ftype;
    c3bool do_rename = FALSE;

    ftype = _c3_file_type(path2);
    if (ftype != C3PLATF_FILE_NONE) /* File exists */
    {
	if (ftype == C3PLATF_FILE_REGULAR)
	{
	    if (rename_existing == FH_BACKUP_RENAME_ASK)
	    {
		if (access (path2, W_OK) == 0
		    && isatty(fileno(stdin))
		    && _fh_user_answer("Overwrite existing file \"%s\"? ",
				       path2))
		{
		    do_rename = TRUE;
		}
	    }
	    else if (rename_existing == FH_BACKUP_RENAME_DESTROY
		     || (rename_existing == FH_BACKUP_RENAME_OVERWRITE
			 && access (path2, W_OK) == 0))
	    {
		do_rename = TRUE;
	    }
	}
    }
    else
    {
	do_rename = TRUE;
    }
    if (do_rename)
    {
	return(rename(path1, path2) == 0);
    }
    else
    {
	return(FALSE);
    }
}



fhandle
_fh_new_hdl()
{
    int fh;

    for (fh = 0; fh < FH_MAX_FILE_HANDLES && fhtab[fh] != NULL; fh++)
	;
    if (fh == FH_MAX_FILE_HANDLES)
    {
/*	_fh_register_outcome(CURRFUNC, ...NOSHANDL); TODO */
	return(-1);
    }
    /*
     * Allocate handle memory
     */
    fhtab[fh] = (fh_handle *) malloc (sizeof (fh_handle));
    if (fhtab[fh] == NULL)
    {
/*	_fh_register_outcome(CURRFUNC, ...NOMEM); TODO */
	return(-1);
    }
    ZERO_MEMORY(fhtab[fh], sizeof (fh_handle));
    return(fh);
}

void
_fh_dispose_hdl(fh)
	fhandle fh;
{
    if (_fh_exists(fh)) {
    	/* Deallocate handle memory. */
	if (fhtab[fh]->name) free(fhtab[fh]->name);
	free(fhtab[fh]);
	fhtab[fh] = NULL;
    }
    else {
	/* _fh_register_outcome(CURRFUNC, ...NOSHANDL); TODO */
    }
}

void
_fh_error(function_name, error_msg)
    char *function_name;
    char *error_msg;
{
    fprintf(stderr, "%s: %s\n", function_name, error_msg);
}

void
_fh_inform(function_name, verbosity_level, msg, str)
    char *function_name;
    int verbosity_level;
    char *msg;
    char *str;
{
    if (opt_verbosity >= verbosity_level)
    {
	fprintf(stderr, "%s: %s", function_name, msg);
	if (str != NULL)
	{
	    fprintf(stderr, " %s", str);
	}
	fprintf(stderr, "\n");
    }
}


/*
 *    External functions
 */

#undef CURRFUNC
#define CURRFUNC "fh_unix_std_hdl"

fhandle
fh_unix_std_hdl(stream_indication)
    int stream_indication;
{
    fhandle fh;

    fh = _fh_new_hdl();
    if (fh >= 0) {
	switch(stream_indication)
	{
	case FH_UNIX_STDIN:
	    fhtab[fh]->filep = stdin;
	    break;

	case FH_UNIX_STDOUT:
	    fhtab[fh]->filep = stdout;
	    break;

	case FH_UNIX_STDERR:
	    fhtab[fh]->filep = stderr;
	    break;

	default:
	    _fh_dispose_hdl(fh);
	    return(-1);
	    break;
	}
    }
    return(fh);
}

#undef CURRFUNC
#define CURRFUNC "fh_opened_temporary_hdl"

extern
fhandle
fh_opened_temporary_hdl(root,path)
    char *root;
    char *path;
{
    int fh, slen, ftype;
    char *dirEnd, *path_to_use;
    const char *x_string = "-XXXXXX";

    ftype = _c3_file_type(path);

    switch(ftype)
    {
    case C3PLATF_FILE_REGULAR:
    case C3PLATF_FILE_SYMLINK:
	if ((dirEnd = rindex(path, PATH_SEP)) == NULL)
	{
#ifndef MAC_OS
	    path_to_use = ".";
#endif
	}
	else
	{
	    *dirEnd = '\0';
	    path_to_use = strdup(path);
	    *dirEnd = PATH_SEP;
	}
	break;

    case C3PLATF_FILE_DIRECTORY:
	path_to_use = path;
	break;

    default:
	/*	_fh_register_outcome(CURRFUNC, ...?); TODO */
	return(-1);
	break;
    }
    fh = _fh_new_hdl();
    if (fh < 0)
	return(-1);

    slen = strlen(root) + strlen(path_to_use) + strlen(x_string) + 1;
    fhtab[fh]->name =
	(char *) malloc((slen + 1) * sizeof (char));
    if (fhtab[fh] == NULL)
    {
	/*	_fh_register_outcome(CURRFUNC, ...NOMEM); TODO */
	return(-1);
    }
    ZERO_MEMORY(fhtab[fh]->name, (slen + 1) * sizeof (char));
    strcpy(fhtab[fh]->name, path_to_use);
    strcat(fhtab[fh]->name, PATH_SEP_STR);
    strcat(fhtab[fh]->name, root);
    strcat(fhtab[fh]->name, x_string);
    _fh_inform(CURRFUNC, 4,
	       "Temporary file", fhtab[fh]->name);
    mktemp(fhtab[fh]->name);
    if (fhtab[fh]->name[0] == '\0')
    {
	_fh_inform(CURRFUNC, 1,
		   "Couldn't create temporary file name", NULL);
	_fh_dispose_hdl(fh);
	return(-1);
    }
    else
    {
	_fh_inform(CURRFUNC, 4,
		   "Temporary file name", fhtab[fh]->name);
	fhtab[fh]->filep = fopen(fhtab[fh]->name, "w");
	if (fhtab[fh]->filep == NULL)
	{
	    _fh_dispose_hdl(fh);
	    return(-1);
	}
#if defined(MAC_OS) && defined(USE_SETVBUF)
	(void) setvbuf(fhtab[fh]->filep, NULL, _IOFBF, BUFSIZ * 8);
#endif /* MAC_OS && USE_SETVBUF */

    }
    return(fh);
}

#undef CURRFUNC
#define CURRFUNC "fh_opened_by_name_hdl"

fhandle
fh_opened_by_name_hdl(path, io_mode, other_file,exist_handling)
    char *path;
    int io_mode;
    fhandle other_file;
    int exist_handling;
{
    int fh;
    char *type;

    fh = _fh_new_hdl();
    if (fh >= 0) {
	/* TODO Check directory of OTHER_FILE etc. */

	/* Convert io_mode to a fopen type. */
	switch (io_mode) {
	case FH_OPEN_READ:
	    type = "r";
	    break;
	case FH_OPEN_BINREAD:
	    type = "r";
	    break;
	case FH_OPEN_WRITE:
	    type = "w";
	    break;
	case FH_OPEN_BINWRITE:
	    type = "w";
	    break;
	case FH_OPEN_READ_WRITE:
	    type = "rw";
	    break;
	default:
	    /* TODO Error */
	    _fh_dispose_hdl(fh);
	    return(-1);
	    break;
	}
	if (io_mode == FH_OPEN_READ || io_mode == FH_OPEN_BINREAD
	    || _fh_should_use(path, exist_handling))
	{
	    fhtab[fh]->filep = fopen(path, type);
	    if (fhtab[fh]->filep == NULL)
	    {
		_fh_dispose_hdl(fh);
		return(-1);
	    }
#if defined(MAC_OS) && defined(USE_SETVBUF)
	    (void) setvbuf(fhtab[fh]->filep, NULL, _IOFBF, BUFSIZ * 8);
#endif /* MAC_OS && USE_SETVBUF */

	    fhtab[fh]->name = strdup(path);
	}
	else
	{
	    _fh_dispose_hdl(fh);
	    return(-1);
	}
    }
    return(fh);
}


#undef CURRFUNC
#define CURRFUNC "fh_handle_updated_file_OK"

c3bool
fh_handle_updated_file_OK(old_fh, new_fh, backup_method, prefix,
	suffix, rename_existing)
    fhandle old_fh;
    fhandle new_fh;
    int backup_method;
    char *prefix;
    char *suffix;
    int rename_existing;
{
    char other_fname[FH_MAX_PATH_LENGTH];

    if (!_fh_exists(old_fh))
    {
	_fh_error(CURRFUNC, "old_fh doesn't exist");
	return(FALSE);
    }
    else if (!_fh_exists(new_fh))
    {
	_fh_error(CURRFUNC, "new_fh doesn't exist");
	return(FALSE);
    }


    switch(backup_method)
    {
    case FH_BACKUP_METHOD_NOKEEP:	/* Leave no backup */
	_fh_inform(CURRFUNC, 4, "Rename from", fhtab[new_fh]->name);
	_fh_inform(CURRFUNC, 4, "...to", fhtab[old_fh]->name);
	if (! _fh_rename(fhtab[new_fh]->name, fhtab[old_fh]->name,
		       rename_existing))
	{
	    /* Rename error */
	    /* TODO Remove new_file, or give it a better name? */
	    _fh_inform(CURRFUNC, 1,
		       "File update error on", fhtab[old_fh]->name);
	    return(FALSE);
	}
	break;

    case FH_BACKUP_METHOD_RENAMESOURCE:
	if (strlen(prefix) + strlen(fhtab[old_fh]->name) 
	    + strlen(suffix) + 1 > FH_MAX_PATH_LENGTH)
	{
	    _fh_inform(CURRFUNC, 1,
		       "Temporary file name storage overflow", NULL);
	    return(FALSE);
	}
	strcpy(other_fname, prefix);
	strcat(other_fname, fhtab[old_fh]->name);
	strcat(other_fname, suffix);

	_fh_inform(CURRFUNC, 4, "Rename from", fhtab[old_fh]->name);
	_fh_inform(CURRFUNC, 4, "...to", other_fname);
	if (! _fh_rename(fhtab[old_fh]->name, other_fname,
		       rename_existing))
	{
	    /* Rename-of-old error */
	    /* TODO Remove new_file, or give it a better name? */
	    return(FALSE);
	}
	_fh_inform(CURRFUNC, 4, "Rename from", fhtab[new_fh]->name);
	_fh_inform(CURRFUNC, 4, "...to", fhtab[old_fh]->name);
	if (! _fh_rename(fhtab[new_fh]->name, fhtab[old_fh]->name,
		       rename_existing))
	{
	    /* Rename-of-new error */
	    /* TODO Remove new_file, or give it a better name? */
	    return(FALSE);
	}
	break;
       
    case FH_BACKUP_METHOD_RENAMETARGET:
	if (strlen(prefix) + strlen(fhtab[old_fh]->name) 
	    + strlen(suffix) + 1 < FH_MAX_PATH_LENGTH)
	{
	    _fh_inform(CURRFUNC, 1,
		       "Temporary file name storage overflow", NULL);
	    return(FALSE);
	}
	strcpy(other_fname, prefix);
	strcat(other_fname, fhtab[old_fh]->name);
	strcat(other_fname, suffix);

	_fh_inform(CURRFUNC, 4, "Rename from", fhtab[new_fh]->name);
	_fh_inform(CURRFUNC, 4, "... to", other_fname);
	if (! _fh_rename(fhtab[new_fh]->name, other_fname,
		       rename_existing))
	{
	    /* Rename-of-new error */
	    /* TODO Remove new_file, or give it a better name? */
	    return(FALSE);
	}
	break;
       
    default:
	_fh_error(CURRFUNC, "Unknown BACKUP_METHOD");
	return(FALSE);
	break;
    }
    return(TRUE);
}

#undef CURRFUNC
#define CURRFUNC "fh_close_OK"

c3bool
fh_close_OK(fh)
    fhandle fh;
{
    if (_fh_exists(fh))
    {
	fclose(fhtab[fh]->filep);
	free(fhtab[fh]->name);
	free(fhtab[fh]);	/* free the memory and zero out  --tcj */
	fhtab[fh] = NULL;
	return(TRUE);
    }
    else
    {
	/* TODO */
	return(FALSE);
    }
}

#undef CURRFUNC
#define CURRFUNC "fh_is_eof"

c3bool
fh_is_eof(fh)
    fhandle fh;
{
    if (_fh_exists(fh))
    {
	return(feof(fhtab[fh]->filep));
    }
    else
    {
	return(FALSE);
	/* TODO */
    }
}


#undef CURRFUNC
#define CURRFUNC "fh_byte_read"

int  
fh_byte_read(fh)
    fhandle fh;
{
    if (_fh_exists(fh))
    {
	return(getc(fhtab[fh]->filep));
    }
    else
    {
	/* TODO */
	return(EOF);
    }
}


#undef CURRFUNC
#define CURRFUNC "fh_read_bytes"

void
fh_read_line(fh, max_bytes_to_read, buffer, bytes_read)
    fhandle fh;
    int max_bytes_to_read;
    char *buffer;
    int *bytes_read;
{

#ifdef MAC_OS
    Boolean flag = FALSE;

    if (gusi_UpdateRoutineInstalled()) {
        gusi_InstallUpdateRoutine(FALSE);
        flag = TRUE;
    }
#endif

    buffer[0] = '\0';
    if (_fh_exists(fh))
    {
	fgets(buffer, max_bytes_to_read, fhtab[fh]->filep);
	*bytes_read = strlen(buffer);
    }
    else
    {
	/* TODO */
	*bytes_read = 0;
    }

#ifdef MAC_OS
    if (flag)
	gusi_InstallUpdateRoutine(TRUE);
#endif
}

void
fh_read_bytes(fh, bytes_to_read, buffer, bytes_read)
    fhandle fh;
    int bytes_to_read;
    char *buffer;
    int *bytes_read;
{

#ifdef MAC_OS
    Boolean flag = FALSE;

    if (gusi_UpdateRoutineInstalled()) {
        gusi_InstallUpdateRoutine(FALSE);
        flag = TRUE;
    }
#endif

    if (_fh_exists(fh))
    {
	*bytes_read = fread(buffer, 1, bytes_to_read, fhtab[fh]->filep);
    }
    else
    {
	/* TODO */
	*bytes_read = 0;
    }

#ifdef MAC_OS
    if (flag)
	gusi_InstallUpdateRoutine(TRUE);
#endif
}


#undef CURRFUNC
#define CURRFUNC "fh_write_byte"

void
#ifdef HAVE_PROTOTYPES
fh_byte_write(fhandle fh, char byte)
#else /* !HAVE_PROTOTYPES */
fh_byte_write(fh, byte)
    fhandle fh;
    char byte;
#endif /* HAVE_PROTOTYPES */
{
    if (_fh_exists(fh))
    {
	putc(byte, fhtab[fh]->filep);
	return;
    }
    else
    {
	return;
    }
}

#undef CURRFUNC
#define CURRFUNC "fh_write_bytes_OK"

void
fh_write_bytes(fh, bytes_to_write, buffer, bytes_written)
    fhandle fh;
    int bytes_to_write;
    char *buffer;
    int *bytes_written;
{
    if (_fh_exists(fh))
    {
	*bytes_written = fwrite(buffer, 1, bytes_to_write, fhtab[fh]->filep);
    }
    else
    {
	*bytes_written = 0;
    }
    return;
}


c3bool
fh_position_OK(fh, pos)
/*
 * Absolute position file pointer
 *
 */
    fhandle fh;
    int pos;
{
    int i1;

    if (_fh_exists(fh))
    {
	i1 = fseek(fhtab[fh]->filep, (long) pos, 0);
    }
    else
    {
	i1 = -1;
    }
    return(i1 >= 0);
}

/* TODO */

int
fh_latest_error(fh)
    fhandle fh;
{
    return(ferror(fhtab[fh]->filep));
}

/* TODO
 * extern
 * fhandle
 * fh_opened_through_dialog_hdl(
 *    char *prompt,
 *    char *suggested__name,
 *    int io_mode
 * );
 */

#undef CURRFUNC
#define CURRFUNC "fh_remove"

void
fh_remove(fh)
    fhandle fh;
{
    if (_fh_exists(fh))
    {
	unlink(fhtab[fh]->name);
    }
    else
    {
	/* TODO */
    }
}


#undef CURRFUNC
#define CURRFUNC "fh_c3_open_hdl"

fhandle
fh_c3_open_hdl(file_type, io_mode, conv_syst, which)
    int file_type;
    int io_mode;
    const char *conv_syst;
    int which;
{
    char path[256], inform_msg[512];
    fhandle fh;

    switch(file_type)
    {
    case FH_TYPE_APPROX:

	/* TODO  Check for overflow */
	sprintf(path, APPROX_TABLE, _fh_c3_table_path(),
		conv_syst);
	sprintf(inform_msg, "Using approximation table in file %s", path);
	break;

    case FH_TYPE_DEF:

	/* TODO  Check for overflow */
	sprintf(path, DEF_TABLE, _fh_c3_table_path(),
		conv_syst, _ccs_id_from_ccs_no(which));
	sprintf(inform_msg, "Using definition table in file %s", path);
	break;

    case FH_TYPE_BIN:

	/* TODO  Check for overflow */
	sprintf(path, BIN_TABLE, _fh_c3_table_path(), conv_syst);
	sprintf(inform_msg, "Using binary table(s) in file %s", path);

	break;

    case FH_TYPE_NAMES:

	/* TODO  Check for overflow */
	sprintf(path, NAMES_TABLE, _fh_c3_table_path(), conv_syst);
	sprintf(inform_msg, "Using table names in file %s", path);

	break;

    case FH_TYPE_FACTOR:

	/* TODO  Check for overflow */
	sprintf(path, FACTOR_TABLE, _fh_c3_table_path(),
		conv_syst, which);
	sprintf(inform_msg, "Using factor table in file %s", path);
	break;

    default:
	sprintf(inform_msg, "Illegal file type %d", file_type);
	_fh_inform(CURRFUNC, 1, inform_msg, NULL);
	break;

    }

    fh = fh_opened_by_name_hdl(path, io_mode, -1, FH_BACKUP_RENAME_ASK);

    _fh_inform(CURRFUNC, 4, inform_msg, NULL);
    return(fh);
}

#undef CURRFUNC
#define CURRFUNC "fh_c3_exist_convsyst"

#include <dirent.h>

c3bool
fh_c3_exist_convsyst(conv_syst)	/* Check if this conversion system exist */
    const char *conv_syst;
{
    char dir[FH_MAX_PATH_LENGTH];

    sprintf(dir, PATH_STR, _fh_c3_table_path(), conv_syst);
    return(_c3_file_type(dir) == C3PLATF_FILE_DIRECTORY);
}

#undef CURRFUNC
#define CURRFUNC "fh_c3_def_list_OK"

c3bool
fh_c3_def_list_OK(conv_syst, ccs_list, listlength)
    const char *conv_syst;
    char ***ccs_list;
    int *listlength;
{
    DIR *dirp;
    struct dirent *dp;
    char path[256];
    char **listp, **lp;
    const int MAXLIST = 256;
    int i, len, listlen;
    char *name;

    lp = listp = (char **) malloc(MAXLIST * sizeof(char *));
    if (listp == NULL)
    {
/*	_fh_register_outcome(CURRFUNC, ...NOMEM); TODO */
	return(FALSE);
    }
    sprintf(path, PATH_STR, _fh_c3_table_path(), conv_syst);
    dirp = opendir(path);
    if (dirp == NULL) 	/* add check for no path error */
    { 
/*	_fh_register_outcome(CURRFUNC, ...NODIR); TODO */
	return(FALSE);
    }
    listlen = 0;
    for (dp = readdir(dirp), i=0; dp != NULL && i < MAXLIST; 
	 dp = readdir(dirp), i++)
    {
	name = (char *)dp->d_name;
	len = strlen(name);
	if (strncasecmp(name, "def-", 4) == 0
	    && strncasecmp(name+len-5, ".txta", 5) == 0)
	{
	    *lp = strdup(name);
	    lp++;
	    listlen++;
	}
    }
    *lp = NULL;
    closedir (dirp);
    *ccs_list = listp;
    *listlength = listlen;
    return(TRUE);
}
#undef CURRFUNC
#define CURRFUNC "fh_c3_names_list_OK"

#define BUF_SIZE 256
#define MAXLIST 256

void 
fh_c3_build_table(conv_syst, ccs_table, tablelength)
    const char *conv_syst;
    struct ccs_no_id_mime **ccs_table;
    int *tablelength;
{
    DIR *dirp;
    struct dirent *dp;
    char path[256];
    struct ccs_no_id_mime *tablep, *tp;
    int i, len, bytes_read;
    char *buffer, *csid, *mimename;
    int cs;
    fhandle fh;
    static int tablelen = 0;
    static firsttime = TRUE;

    *tablelength = tablelen;
    if (!firsttime) 
	return;
    firsttime = FALSE;

    fh = fh_c3_open_hdl(FH_TYPE_NAMES, FH_OPEN_READ, C3_SYSNAME, 0);
    if (fh < 0) 	/* add check for no path error */
    { 
/*	_fh_register_outcome(CURRFUNC, ...NOFILE); TODO */
	return;
    }

    SAFE_ZALLOCATE(tablep, (struct ccs_no_id_mime *),
		      (MAXLIST * sizeof(struct ccs_no_id_mime)), 
		      return, CURRFUNC);
    SAFE_ZALLOCATE(buffer, (char *),
		      (BUF_SIZE * sizeof(char *)), return, CURRFUNC);
    SAFE_ZALLOCATE(csid, (char *),
		      (BUF_SIZE * sizeof(char *)), return, CURRFUNC);
    SAFE_ZALLOCATE(mimename, (char *),
		      (BUF_SIZE * sizeof(char *)), return, CURRFUNC);
    if (tablep == NULL || buffer == NULL || csid == NULL || mimename == NULL)
    {
        fh_close_OK(fh);
	return;
    }

    tp = tablep;
    i = 0;
    fh_read_line(fh, BUF_SIZE, buffer, &bytes_read);
    while (!feof(fhtab[fh]->filep) && i < MAXLIST) {
	mimename[0] = csid[0] = '\0';
	sscanf(buffer, "%d %s %s", &cs, csid, mimename);
	if ((cs > 0) && (cs < 99999)) {
	    tp->num = cs;
	    tp->id = strdup(csid);
	    if (!mimename || *mimename == '\0')
	    	tp->mime_name = tp->id;
	    else
	    	tp->mime_name = strdup(mimename);
	    tp++;
	    tablelen++;
	    i++;
	}
        fh_read_line(fh, BUF_SIZE, buffer, &bytes_read);
    }
    tp->id = NULL;
    *ccs_table = tablep;
    *tablelength = tablelen;
    free(buffer);
    free(csid);
    free(mimename);
    fh_close_OK(fh);
    return;
}
