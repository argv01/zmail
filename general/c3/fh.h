/* $Id: fh.h,v 1.7 1995/08/03 17:58:39 spencer Exp $ */

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
 *  can be found on the first line.
 *
 */

#ifndef __fh_included__
#define __fh_included__

#ifndef FALSE
#define FALSE	(0)
#endif
#ifndef TRUE
#define TRUE	(1)
#endif

#define FH_UNIX_STDIN	0
#define FH_UNIX_STDOUT	1
#define FH_UNIX_STDERR	2
#define FH_UNIX_OTHER	3

#define FH_OPEN_READ		0
#define FH_OPEN_BINREAD		3
#define FH_OPEN_WRITE		1
#define FH_OPEN_BINWRITE	4
#define FH_OPEN_READ_WRITE	2

#define FH_BACKUP_METHOD_NOKEEP		0
#define FH_BACKUP_METHOD_RENAMESOURCE	1
#define FH_BACKUP_METHOD_RENAMETARGET   2

#define FH_BACKUP_RENAME_DESTROY	0
#define FH_BACKUP_RENAME_OVERWRITE	1
#define FH_BACKUP_RENAME_ASK		2
#define FH_BACKUP_RENAME_PROTECT	3

#define FH_TYPE_APPROX		0
#define FH_TYPE_DEF		1
#define FH_TYPE_BIN		2
#define FH_TYPE_FACTOR		3
#define FH_TYPE_NAMES 		4

#ifndef _c3bool_defined
typedef short int c3bool;
#define _c3bool_defined
#endif

typedef int		fhandle;

extern
fhandle
fh_unix_std_hdl P((
    int stream_indication));
/*
 * Give a handle to a (UNIX conventional) standard I/O file -
 * FH_UNIX_STDIN, FH_UNIX_STDOUT or FH_UNIX_STDERR.
 */

extern
fhandle
fh_opened_temporary_hdl P((
    char *root,
    char *path
));
/*
 * Open temporary file for write access with given ROOT name in
 * same directory as PATH
 * 
 */


extern
fhandle
fh_opened_by_name_hdl P((
    char *path,
    int io_mode,
    fhandle other_file,
    int exist_handling
));
/*
 * Open file with given path and I/O mode
 * If PATH is relative:
 *     if OTHER_FILE > 0
 *         use same directory as OTHER_FILE,
 *     else
 *         use default/current directory.
 * 
 * EXIST_HANDLING specifies how to handle name
 * collisions. Possible values:
 *   FH_BACKUP_RENAME_DESTROY - always remove existing file
 *   FH_BACKUP_RENAME_OVERWRITE - remove a file only if it is writable
 *   FH_BACKUP_RENAME_ASK - ask the user if the file is writable
 *   and stdin is a terminal device, keep it otherwise
 *   FH_BACKUP_RENAME_PROTECT - always keep an existing file
 */

c3bool
fh_handle_updated_file_OK P((
    fhandle old_fh,
    fhandle new_fh,
    int backup_method,
    char *prefix,
    char *suffix,
    int rename_existing
));
/* 
 * Handle updated version NEW_FH of previous file OLD_FH.
 * Change file system info or file contents in this way:
 *
 *  State at entrance:
 *      File referenced by OLD_FH has contents OFC and name OFN
 *      File referenced by NEW_FH  has contents NFC and name NFN
 *      Both files should exist and be closed
 *
 *  State at exit:
 *      OLD_FH and NEW_FH are illegal file handles
 *
 *      backup_method == FH_BACKUP_METHOD_NOKEEP
 *          The file with name OFN has contents NFC
 *          File contents OFC is not kept
 *          
 *      backup_method == FH_BACKUP_METHOD_RENAMESOURCE
 *          The file with name OFN has contents NFC
 *          The file with name prefix + OFN + suffix has contents OFC
 *
 *      backup_method == FH_BACKUP_METHOD_RENAMETARGET
 *          The file with name prefix + OFN + suffix has contents NFC
 *          The file with name OFN has (still) contents OFC
 *
 *  When renaming to an existing filname:
 *
 *	rename_existing == FH_BACKUP_RENAME_DESTROY
 *	    Allways remove the existing file
 *	rename_existing == FH_BACKUP_RENAME_OVERWRITE
 *	    Remove the existing file if it is writeable
 *	rename_existing == FH_BACKUP_RENAME_ASK
 *	    Ask the user if the file is writable and stdin
 *          is a terminal device, keep it otherwise
 *	rename_existing == FH_BACKUP_RENAME_PROTECT
 *	    Allways keep the existing file
 *
 * Returns FALSE on error.
 */

extern
c3bool
fh_close_OK P((
    fhandle fh
));
/*
 * Close file FH
 * Returns FALSE on error.
 *
 */

extern
c3bool
fh_is_eof P((
    fhandle fh
));
/*
 * Return TRUE if FH is at end-of-file.
 *
 */

extern
int
fh_byte_read P((
    fhandle fh
));
/*
 * Read one byte, return EOF if end-of-file.
 *
 */

extern
void
fh_read_bytes P((
    fhandle fh,
    int bytes_to_read,
    char *buffer,
    int *bytes_read
));
/*
 * Read the amount BYTES_TO_READ bytes from file FH to BUFFER
 * Return number of bytes read in BYTES_READ
 */

extern
void
fh_byte_write P((fhandle fh, char byte));
/*
 * Write one byte
 *
 */

extern
void
fh_write_bytes P((
    fhandle fh,
    int bytes_to_write,
    char *buffer,
    int *bytes_written
));
/*
 * Write the amount BYTES_TO_READ bytes from BUFFER to file FH
 * Return number of bytes written in BYTES_WRITTEN
 * and TRUE as result if all bytes where written
 */

extern
c3bool
fh_position_OK P((
    fhandle fh,
    int pos
));
/*
 * Absolute position file pointer
 *
 */

extern
int
fh_latest_error P((
    fhandle fh
));
/*
 * Return latest error
 *
 */

extern
fhandle
fh_opened_through_dialog_hdl P((
    char *prompt,
    char *suggested__name,
    int io_mode
));
/*
 * Open a file in given IO_MODE through a dialog
 * with the user.
 * PROMPT is the promt to the user, specifying what file is
 * requested.
 * SUGGESTED_NAME is a suggested name
 */

void
fh_remove P((
    fhandle fh
));
/*
 * Remove file
 * 
 */

/*
 *************************
 * C3 specific functions *
 *************************
 */

extern
fhandle
fh_c3_open_hdl P((
    int file_type,
    int io_mode,
    const char *conv_syst,
    int which
));
/*
 * Open a C3 file of type FILE_TYPE in given IO_MODE
 * for conversion system CONV_SYST.
 * WHICH is a further specification:
 *
 * FILE_TYPE          description          WHICH
 * =========          ===========          =====
 * FH_TYPE_APPROX     approximation file   [unused]
 * FH_TYPE_DEF        definition file      ccs number
 * FH_TYPE_BIN        binary file          [unused]
 * FH_TYPE_FACTOR     factor file          factor number
 *
 */

extern
c3bool
fh_c3_exist_convsyst P((
    const char *conv_syst
));
/*
 * Check if this conversion system exist
 */

extern
c3bool
fh_c3_def_list_OK P((
    const char *conv_syst,
    char ***ccs_list,
    int *listlength
));
/*
 * Get the list of coded character sets in given conversion
 * system for which CCS data exist. Returns list in
 * CCS_LIST and it's length in LISTLENGTH.
 * Returns FALSE on error.
 * 
 */

extern
struct ccs_data *
fh_c3_bin_ccsdatai P((
    fhandle fh,
    int ccs_num,
    int ctype
));
/*
 * Read from the C3 binary file specified by FH
 * data for coded character set CCS_NUM,
 * conversion type CTYPE.
 *
 */

extern
struct factor_data *
fh_c3_bin_factordata P((
    fhandle fh,
    int parameter,
    int value
));
/*
 * Read from the C3 binary file specified by FH
 * data for factor PARAMETER, with value VALUE.
 *
 */

extern
c3bool
fh_c3_bin_add_ccsdata_OK P((
    fhandle fh,
    struct ccs_data *ccs_data
));
/*
 * Store in the C3 binary file specified by FH the CCS
 * data given through CCS_DATA.
 */

extern
c3bool
fh_c3_bin_add_factordata_OK P((
    fhandle fh,
    struct factor_data *factor_data
));
/*
 * Store in the C3 binary file specified by FH the factor
 * data given through CCS_DATA.
 */

extern
void
fh_c3_set_backup_table_path P((const char *pathname));

#endif /* __fh_included__ */
