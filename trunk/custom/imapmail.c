
#include "config.h"

#if defined( IMAP )

#if defined( SUN414 )
#if !defined( const )
#define const
#endif
#endif

#ifdef NOT_ZMAIL
#define lock_fopen              fopen
#define close_lock(x, y)        fclose(y)
#define cleant char *));
#ifdef MSDOS
#include <conio.h>
#endif /* MSDOS */
#ifdef LANWP
#include <pwd.h>
#endif /* LANWP */
#else /* !NOT_ZMAIL */

#include "zmail.h"
#include "fsfix.h"
#include "refresh.h"
extern Ftrack *real_spoolfile;
#define DELIVERYFILE    \
                (real_spoolfile ? ftrack_Name(real_spoolfile) : spoolfile)
#define LOGINNAME               zlogin

#endif /* NOT_ZMAIL */

#undef msg
#include "pop.h"
#include "dputil.h"
#include "dynstr.h"
#include "excfns.h"
#include "strcase.h"
#include "mailserv.h"
#include "error.h"

#include "c-clientmail.h"

#ifdef MOTIF
#include "zm_motif.h"   /* for hdr_list_w */
#endif /* MOTIF */

static char pass[64] = { '\0' }; /* RJL ** 6.14.93 -these need to be inited */
static char logn[64] = { '\0' }; /* since they are passed to imap_open */

#include <grp.h>  /* for getgrpnam() */

extern MAILSTREAM *gIMAPh;

static int syncing = 0;

void ImapSyncFolder();
int zimap_update_deleted_local();
int zimap_update_flags_local();

extern msg_folder *current_folder;

MAILSTREAM *
zimap_connect( imaplogin, imappword )
char *imaplogin;
char *imappword;
{
	return( gIMAPh );
}

char prevHost[MAXPATHLEN] = "";

static int isLocalOpen = 0;

int
IsLocalOpen()
{
	return( isLocalOpen );
}

static int allowDeletes = 0;

int
SetAllowDeletes( what )
int what;
{
	allowDeletes = what;
}

int
GetAllowDeletes()
{
	if ( boolean_val( VarImapShared ) ) return( allowDeletes );
	return( 1 );
}

int
OpenAndDrainIMAP( file )
char *file;
{
	static char buf[MAXPATHLEN];	/* must be static */
    turnoff(folder_flags, CONTEXT_RESET|CONTEXT_IN_USE);

#if defined(GUI) || defined(VUI) 
	gui_close_folder(current_folder, 0);
    flush_msgs();
#endif
#if defined(GUI) || defined(VUI) 
	gui_refresh(current_folder, REDRAW_SUMMARIES);
#endif
	current_folder = open_folders[0];
#if defined(GUI) || defined(VUI)
	gui_close_folder(current_folder, 0);
    flush_msgs();
#endif
	zimap_shutdown( (char *) NULL, 0 );
    ClearPrev();
	strcpy( prevHost, getenv( "MAILHOST" ) );
	sprintf( buf, "MAILHOST=%s", file );
    putenv( buf );
	isLocalOpen = 1;
    turnon(folder_flags, CONTEXT_IN_USE);

	fetch_via_imap(); 

#if defined(GUI) || defined(VUI) 
    gui_clear_hdrs(current_folder);
	gui_redraw_hdrs(current_folder, NULL_GRP);
	gui_refresh(current_folder, REDRAW_SUMMARIES);
#endif
	isLocalOpen = 0;
}

char *
GetImapPasswordGlobal()
{
        return ( pass );
}

int
loadmail(master, preserve, imaplogin, imappword)
int master, preserve;
char *imaplogin, *imappword;
{
    MAILSTREAM *imapH;
    struct dpipe *dp;
    int i, flags = 0, n;
    char mailbox[MAXPATHLEN], *tmp, *msgptr, *dateline, *fromline;
    FILE *mfstream, *fp;
    struct stat mfstat, stat1, stat2;
    unsigned first_new = 1, err = 0;
    off_t pre_retr_offset;
    unsigned long uidval = 0L, uid, readUID = 0L;
    char buf[ 128 ];
    char fldr[ 128 ];
    char usr[ 128 ];
    char junk;
    int reopen = 0, fd, truncate = 0, recent, new, isspool;
    char savepath[MAXPATHLEN], *path;
    AskAnswer answer = AskYes;
    int synchronize;
    int	wasshell;

    truncate = synchronize = 0;

    errno = 0;
    
#ifndef NOT_ZMAIL
    if (!spoolfile)
	    spoolfile = savestr(getenv("MAIL"));
    if (!zlogin)
	    imaplogin = value_of(VarUser);
#endif /* !NOT_ZMAIL */
    
    if (tmp = DELIVERYFILE)
	    strcpy(mailbox, tmp);
    else {
	    error(ZmErrWarning, catgets( catalog, CAT_CUSTOM, 27, "Error reading environment: no MAIL file.\n" ));
	    return;
    }

    if (imappword)
	    strcpy(pass, imappword);
    if (pass[0])
	    flags = POP_NO_GETPASS;
    if (imaplogin)
	    strcpy(logn, imaplogin);
    if (logn[0] == 0)
	    strcpy(logn, LOGINNAME);

    sprintf( buf, "%s/%s", MAILDIR, logn );

    isspool = 0;
    if (stat(buf, &stat1) != -1) 
	    if (stat(spoolfile, &stat2) != -1) 
			if (stat1.st_dev==stat2.st_dev && stat1.st_ino==stat2.st_ino)
				isspool = 1;

    if (!(imapH = zimap_open(NULL, logn, pass, flags, &new, isspool))) {
	    error(ZmErrWarning, catgets( catalog, CAT_CUSTOM, 320, "Error opening connection with imap server: %s\n" ),
	      imap_error);
	    imap_initialized = 0;
	    return;
    }

    imap_initialized = 1;

    mail_ping( imapH );

    if ( !new && !IsLocalOpen() )
	    HandleFlagsAndDeletes( 3 );

    /* if a non-IMAP folder is currently in front, return. Notice we ping'd
       first, to keep the background connection alive */

    if ( !new && !current_folder->imap_path )
	    return;

    if ( imapH->rdonly ) {
        error(ZmErrWarning, catgets( catalog, CAT_CUSTOM, 321, "IMAP mailbox is read-only\n" ));
#if 0
	zimap_close(imapH, 0);
	using_imap = 0;
	return;
#endif
    }

    if (!imapH->nmsgs && !new) 
	    return;

    if ( new ) {
	    uidval = zimap_getuidval( imapH );
	    fp = lock_fopen(mailbox, FLDR_READ_MODE);
	    if ( fp != (FILE *) NULL ) {

		if ( stat( mailbox, &mfstat ) == -1 ) {
	
			/* unlikely to fail, but ought to be safe */	
#ifdef NOT_ZMAIL
			perror(catgets( catalog, CAT_CUSTOM, 333, "Error reading mailbox in loadmail" ));
#else /* !NOT_ZMAIL */
			error(SysErrWarning, catgets( catalog, CAT_CUSTOM, 333, "Error reading mailbox in loadmail" ));
#endif /* !NOT_ZMAIL */
			zimap_close(imapH, 0);
			close_lock(mailbox, fp);
			return;
		}
			
		if ( mfstat.st_size == 0 ) {
			close_lock(mailbox, fp);
			truncate++;		/* write UIDVAL clause */ 
		}
		else {
			fscanf( fp, "UIDVAL=%08lx%s %s %c\n", &readUID, fldr, usr, &junk );
	        	close_lock(mailbox, fp);

    if ( current_folder ) 
	if ( current_folder->uidval == readUID ) 
		if ( getremotefolder() && !strcmp( fldr, getremotefolder() ) )
			if ( getremoteuser() && !strcmp( usr, getremoteuser() ) ) {
				/* same mailbox */

				current_folder->mf_last_size = 0L;
				reopen = 1;
			}

			if ( readUID != uidval || 
				strcmp( fldr, getremotefolder() ) ||
				strcmp( usr, getremoteuser() ) || reopen ) {
				msg_group list;
				char *argv[2];
				char *path;

				/* would you like to save??? */

askcopy:
				answer = AskYes;
				answer = ask(AskYes, catgets( catalog, CAT_MSGS, 1100, "Save a copy of %s?\n\nSelect 'Yes' to save a copy of %s,\n'No' to overwrite %s,\nor 'Cancel' to cancel the IMAP session and use the mailbox in disconnected mode." ), mailbox, mailbox, mailbox );
again:
				if ( answer == AskYes ) {
					strcpy( savepath, value_of(VarHome) );
					strcat( savepath, "/Mail" );
					path = tempnam( savepath, "ZmSave" );
					if ( CopyIMAPFolder( mailbox, path ) == 0 ) {
						using_imap = 0;	
						zimap_close( imapH, 0 );
						return;
					}
				} else if ( answer == AskCancel ) {
					using_imap = 0;	
					zimap_close( imapH, 0 );
					return;
				} else {
					answer = AskYes;
					answer = ask(AskYes, catgets( catalog, CAT_MSGS, 1101, "Contents of %s will be overwritten. Please verify your selection.\n\nSelect 'Yes' to save a copy of %s.\nSelect 'No' to overwrite %s.\nSelect 'Cancel' to cancel the IMAP session and operate in disconnected mode." ), mailbox, mailbox, mailbox );
					if ( answer == AskYes || answer == AskCancel ) {
						goto again;
					} 
				}
				truncate++;
			}
			else if ( junk == 'D' || MessageFlagsDirty() ) {
				char *path;

				answer = AskYes;
				if ( !boolean_val(VarImapSynchronize) ) 
					answer = ask(AskYes, catgets( catalog, CAT_MSGS, 1102, "Synchronize %s and remote folder %s?" ), mailbox, getremotefolder() );
				if ( answer == AskNo ) {
					goto askcopy;
				}
				else {
					synchronize++;
				}
			}

			if ( !truncate )  {
				static char *argv[3] = { "folder", "", "" };

				argv[1] = mailbox;
				wasshell = is_shell;
				if ( !wasshell )
					turnon(glob_flags, DO_SHELL|IS_SHELL);
				syncing = 1; /* supress headers */
				folder( 2, argv, 0 );
				syncing = 0;
				if ( !wasshell )
					turnoff(glob_flags, DO_SHELL|IS_SHELL);
			}
		}
	    }
	    else 
		truncate++;
     }

     if ( truncate ) {
	struct group *foo;

	fd = open( mailbox, O_CREAT | O_RDWR | O_TRUNC,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
	if ( fd == -1 ) {
#ifdef NOT_ZMAIL
		perror(catgets( catalog, CAT_CUSTOM, 334, "Could not create mailbox in loadmail" ));
#else /* !NOT_ZMAIL */
		error(SysErrWarning, catgets( catalog, CAT_CUSTOM, 334, "Could not create mailbox in loadmail" ));
#endif /* !NOT_ZMAIL */
		zimap_close(imapH, 0);
		return;
	}
	foo = getgrnam( "mail" );
#ifdef SCO32V4
	chown( mailbox, geteuid(), (foo ? foo->gr_gid : getegid()) );
#else
	fchown( fd, geteuid(), (foo ? foo->gr_gid : getegid()) );	
#endif
	sprintf( buf, "UIDVAL=%08lx%s %s C\n", uidval, getremotefolder(),
		getremoteuser() );
	write( fd, buf, strlen( buf ) );
	close( fd );
#if 0
	chmod( mailbox, S_IRUSR | S_IWUSR | S_IRGRP );
	if ( tempfile && tmpf ) {
		fd = open( tempfile, O_CREAT | O_RDWR | O_TRUNC );
		close( fd );
	}
#endif
     }
     else { 
	if ( synchronize || imapH->nmsgs != msg_cnt )
		first_new = 1;
	else
		first_new = imapH->nmsgs - imapH->recent;
	if ( first_new == 0 )
		first_new = 1;
    }

    if (first_new <= 0 || first_new > imapH->nmsgs) {

	/* this isn't fatal - it just means there is nothing to read */

	if ( imapH->nmsgs < msg_cnt )
		ImapSyncFolder( imapH, 1 /* want expunge */, 1, 0 );
	return;
    }

    if ( synchronize ) {

	/* Make a pass through the messages, and send deleted, read,
	   answered flags up to the host. If any deletes, silently 
	   EXPUNGE */

	ImapSyncFolder( imapH, 1 /* want expunge */, 0, 0 );
    } else if ( new ) {

	/* Just see if any messages deleted on host, if so nuke em */

	ImapSyncFolder( imapH, 1 /* want expunge */, 1, 0 );
    }

    if ( new )
	    mail_size(current_folder->mf_name, FALSE);

    /* Begin loading mailbox */

    if (!(mfstream = lock_fopen(mailbox, FLDR_APPEND_MODE))) {
#ifdef NOT_ZMAIL
	perror(catgets( catalog, CAT_CUSTOM, 335, "Error opening mailbox in loadmail" ));
#else /* !NOT_ZMAIL */
	error(SysErrWarning, catgets( catalog, CAT_CUSTOM, 335, "Error opening mailbox in loadmail" ));
#endif /* !NOT_ZMAIL */
	zimap_close(imapH, 0);
	return;
    }
#ifndef NOT_ZMAIL
    if (imapH->nmsgs > 0) {
	char *gotfrom = value_of(VarMailhost);
	if ( synchronize ) 
		init_intr_mnr(zmVaStr(catgets( catalog, CAT_MSGS, 1103, "Synchronizing %s. Please wait" ), gotfrom), 100);
	else if ( new )
		init_intr_mnr(zmVaStr(imapH->nmsgs - first_new ? catgets( catalog, CAT_CUSTOM, 32, "Retrieving %d messages from %s" )
			      : catgets( catalog, CAT_CUSTOM, 31, "Retrieving %d message from %s" ),
			      1 + imapH->nmsgs - first_new, gotfrom), 100);
    }
#endif /* NOT_ZMAIL */
    err = 0;

    if ( new && IsLocalOpen() ) {
	flush_msgs();
    }

    for (i = first_new; i <= imapH->nmsgs && !err; i++) {	/* Load new messages */
	if ( !gIMAPh ) {
		err = TRUE;
		break;
	}
 
#ifndef NOT_ZMAIL
	if ( synchronize )
	zmVaStr(catgets( catalog, CAT_CUSTOM, 322, "Processing %d ..." ), 1 + i - first_new);
	else
	zmVaStr(catgets( catalog, CAT_CUSTOM, 33, "Retrieving %d ..." ), 1 + i - first_new);
	if (new && check_intr_mnr(zmVaStr(NULL), ((1 + i - first_new) * 100)/(1 + imapH->nmsgs - first_new)))
	    break;
	if ( zimap_uid_match( zimap_UID( imapH, (unsigned long) i ) ) != -1 )
		continue;
	if (!istool)
	    print("%s\n", zmVaStr(NULL));
#endif /* NOT_ZMAIL */
	TRY {
		dp = zimap_retrieve(imapH, i);
	} EXCEPT(ANY) {
	    error(ZmErrWarning, catgets( catalog, CAT_CUSTOM, 336, "Out of memory in zimap_retrieve" ));
	    err = TRUE;	    
	} FINALLY {
	    if (!dp)
	    	err = TRUE;
	} ENDTRY;
	if (err)
	    continue;
	pre_retr_offset = ftell(mfstream);
	if (FolderDelimited == def_fldr_type)
	    efprintf(mfstream, "%s", msg_separator);
	TRY {
	    msgptr = NULL;
	    n = dpipe_Get(dp, &msgptr);
	    if (n && strncmp(msgptr, "From ", 5) != 0) {
		struct dynstr ds;

		dynstr_Init(&ds);
		TRY {
		    dynstr_AppendN(&ds, msgptr, n);
		    free(msgptr);
		    msgptr = NULL;
		    /* Now we need to get ALL the headers to phony one up */
		    while (!strstr(dynstr_Str(&ds), "\n\n") &&
			   (n = dpipe_Get(dp, &msgptr))) {
			dynstr_AppendN(&ds, msgptr, n);
			free(msgptr);
		    }
		    msgptr = NULL;
		    dateline = date(dynstr_Str(&ds));
		    fromline = from_line(dynstr_Str(&ds));
		    TRY {
			efprintf(mfstream,
				 (FolderDelimited == def_fldr_type) ?
				 "From %s %s\n%s" : "\nFrom %s %s\n%s",
				 fromline, dateline, dynstr_Str(&ds));
		    } FINALLY {
			free(dateline);
			free(fromline);
		    } ENDTRY;
		} FINALLY {
		    dynstr_Destroy(&ds);
		} ENDTRY;
	    } else if (n) {
		efwrite(msgptr, sizeof(char), n, mfstream, "loadmail");
		free(msgptr);
		msgptr = NULL;
	    }
	    while (n = dpipe_Get(dp, &msgptr)) {
		efwrite(msgptr, sizeof(char), n, mfstream, "loadmail");
		free(msgptr);
		msgptr = NULL;
	    }
	} EXCEPT(ANY) {
	    xfree(msgptr);
	    ftruncate(fileno(mfstream), pre_retr_offset);
	    error(SysErrWarning, catgets(catalog, CAT_CUSTOM, 34, "Error writing mailbox file\n"));
	    err = TRUE;
	} FINALLY {
	    dputil_Destroy(dp);
	} ENDTRY;

	if (!err) {
	    if (FolderDelimited == def_fldr_type)
		efprintf(mfstream, "%s", msg_separator);
	}
    }
#ifndef NOT_ZMAIL
    if (imapH->nmsgs > 0) {
	i -= first_new;
	if ( new ) {
	end_intr_mnr(zmVaStr(ison(glob_flags, WAS_INTR) ?
			     i == 1 ?
			     catgets(catalog, CAT_CUSTOM, 37, "Stopped, retrieved %d messages.")
			     : catgets(catalog, CAT_CUSTOM, 42, "Stopped, retrieved %d message.")
			     : i == 1 ?
			     catgets(catalog, CAT_CUSTOM, 43, "Done, retrieved %d message.")
			     : catgets(catalog, CAT_CUSTOM, 44, "Done, retrieved %d messages."),
			     i),
		     100);
	end_intr_msg( NULL );
        }
     }
#endif /* NOT_ZMAIL */
    if (close_lock(mailbox, mfstream) == EOF) {
#ifdef NOT_ZMAIL
	perror(catgets(catalog, CAT_CUSTOM, 38, "Error closing mailbox file in loadmail"));
#else /* !NOT_ZMAIL */
	error(SysErrWarning, catgets(catalog, CAT_CUSTOM, 38, "Error closing mailbox file in loadmail"));
#endif /* !NOT_ZMAIL */
	zimap_close(imapH, 0);
	return;
    }

    if ( IsLocalOpen() )
	last_msg_cnt = msg_cnt + imapH->nmsgs;

    if ( !imapH->rdonly ) 
	    ValidateMessageOrder();
    return;
}

int
HandleFlagsAndDeletes( which )
int	which;
{
    unsigned long number;
    extern unsigned long hasDeleted, hasFlags;
    int needRefresh = 0, idx;
    MAILSTREAM *stream;
    static char *argv[3] = { "update", "", NULL };

    if ( IsLocalOpen() )
	return;

    if ( !current_folder || !current_folder->mf_msgs )
        return;

    if ( hasFlags && ( which & 1 ) ) {

        hasFlags = 0;
        while ( ( number = GetFlags() ) ) {
                idx = zimap_update_flags_local( number );
		needRefresh = 1; 
        }
    }

    if ( hasDeleted && ( which & 2 ) ) {

	if ( GetAllowDeletes() ) /* If 1, then we are exiting Z-Mail */
		return;

	SetAllowDeletes( 1 );
        hasDeleted = 0;
        while ( ( number = GetDeleted() ) ) 
        	needRefresh += zimap_update_deleted_local( number );
	if ( needRefresh ) {
		argv[1] = current_folder->mf_name;
		folder( 2, argv, 0 );
		turnoff(folder_flags, DO_UPDATE);
		needRefresh = 0;
	}
	SetAllowDeletes( 0 );

    }

    if ( needRefresh ) {
#if defined(GUI) || defined(VUI)
        gui_redraw_hdrs(current_folder, NULL_GRP);
        gui_refresh(current_folder, REDRAW_SUMMARIES);
#endif
    }
}

int
MessageFlagsDirty()
{
	int	retval = 0;
	int 	i;

	for ( i = 0; i < msg_cnt; i++ ) 
                if ( current_folder->mf_msgs[i]->m_flags & ZMF_DELETE ) 
			retval = 1;
		else if (current_folder->mf_msgs[i]->m_flags & ZMF_REPLIED ) 
			retval = 1;
	return( retval );
}

int
zimap_syncing()
{
	return( syncing );
}

int
ValidateMessageOrder()
{
	int	i;
	int retval = 0;
	unsigned long uid;
	MAILSTREAM *stream;

    	stream = zimap_connect( NULL, NULL );
	if ( !stream ) return( retval );
	if ( !current_folder ) return( retval );
	if ( stream->uid_validity != current_folder->uidval ) return( retval );

	for ( i = 0; i < msg_cnt && i < stream->nmsgs; i++ ) {
        	uid = zimap_UID( stream, (unsigned long) i + 1 );
		if ( uid && uid != current_folder->mf_msgs[i]->uid ) {
			error(UserErrWarning, catgets( catalog, CAT_SHELL, 322, "Message sequence (%d) is out of order" ), i + 1 );
			retval = 1;
			break;
		}
	}
	return( retval );
}

void
ImapSyncFolder( imapH, wantExpunge, onlyDeletes, force )
MAILSTREAM *imapH;
int	wantExpunge;
int	onlyDeletes;
int 	force;
{
	int 	i, expunge = 0;
	unsigned long uidval;
	unsigned long ccuid, uid;
	char	buf[ 16 ];	/* enough to hold a UID value */
	char	*hdr;
	int	wasshell;

	/* just in case */

	if ( !imapH )
		return;

	if ( force ) goto justupdate;

        syncing = 1;

	uidval = imapH->uid_validity;

	if ( onlyDeletes ) 
		init_intr_mnr(zmVaStr(catgets( catalog, CAT_CUSTOM, 323, "Looking for deleted messages in remote folder. Please wait" )), 100);
	else
		init_intr_mnr(zmVaStr(catgets( catalog, CAT_CUSTOM, 324, "Updating status changes to remote folder. Please wait" )), 100);

	/* send all delete and answered flags up to the server */

	for ( i = 0; i < msg_cnt; i++ ) {

		/* first, send our flags up */ 

		if ( onlyDeletes ) {
			if ( check_intr_mnr(zmVaStr( catgets( catalog, CAT_CUSTOM, 325, "Checking for message %d ..." ), i + 1 ), ((i + 1) * 100)/(msg_cnt)))
				break;
		}
		else {
			if ( check_intr_mnr(zmVaStr( catgets( catalog, CAT_CUSTOM, 326, "Processing status changes for message %d ..." ), i + 1 ), ((i + 1) * 100)/(msg_cnt)))
				break;
		}

		if ( !onlyDeletes ) {
		if ( current_folder->mf_msgs[i]->m_flags & ZMF_DELETE ) {
			zimap_turnon( uidval, 
				current_folder->mf_msgs[i]->uid, ZMF_DELETE );            
			turnon(current_folder->mf_msgs[i]->m_flags, DO_UPDATE |
				ZMF_DELETE );
			expunge++;
		}

		if (current_folder->mf_msgs[i]->m_flags & ZMF_REPLIED ) 
			zimap_turnon( uidval, 
				current_folder->mf_msgs[i]->uid, ZMF_REPLIED );
		}

		/* next, force flag updates - the remote mailbox may have
		   changed. because we made our change first, ours will 
		   persist */

		if ( ( uid = current_folder->mf_msgs[i]->uid ) ) {
			if ( ( hdr = mail_fetchheader_full( imapH, 
				uid, NIL, NIL, FT_UID ) ) && strlen( hdr ) ) {
				sprintf( buf, "%d", uid );
				mail_fetchflags_full( imapH, buf, FT_UID ); 
			}
			else {
#if 0
				zimap_turnon( uidval, 
					current_folder->mf_msgs[i]->uid, 
					ZMF_DELETE );            
#endif
				turnon(current_folder->mf_msgs[i]->m_flags, 
					DO_UPDATE | ZMF_DELETE );
				expunge++;
			}
		}
	}	

	end_intr_mnr( NULL, 100L );

	/* send an expunge to the server, then force an update of the
	   local folder, if needed */

justupdate:
	if ( force || expunge && wantExpunge ) {

                static char *argv[3] = { "update", "", NULL };

		if ( !force )
			zimap_expunge( uidval );

                /* call mail_size to update the folder size
                   and mod dates. if we don't, we might be
                   bitten later in copyback with a corrupted
                   mail folder message. */

                mail_size(current_folder->mf_name, TRUE);

		argv[1] = current_folder->mf_name;
		wasshell = is_shell;
		if ( !wasshell )
			turnon(glob_flags, DO_SHELL|IS_SHELL);
		turnon(folder_flags, DO_UPDATE);
        	SetAllowDeletes( 1 );
                folder( 2, argv, 0 );
        	SetAllowDeletes( 0 );
		turnoff(folder_flags, DO_UPDATE);
		if ( !wasshell )
			turnoff(glob_flags, DO_SHELL|IS_SHELL);
	}
        HandleFlagsAndDeletes( 3 );
	syncing = 0;

#if 0
#if defined( GUI ) 		
        gui_redraw_hdrs(current_folder, NULL_GRP);
#if defined( MOTIF )
	if ( istool )
		gui_redraw_hdr_items(hdr_list_w, &current_folder->mf_group, False);
#endif
#if defined( VUI ) 
	{
		static char *argv[3] = { "redraw", "-a", NULL };

		gui_redraw( 2, argv, &current_folder->mf_group );
	}
#  else /* VUI */
#if defined( MOTIF )
	if ( istool )
		gui_redraw_hdr_items(hdr_list_w, &current_folder->mf_group, TRUE);
#endif
#  endif /* VUI */
        gui_refresh(current_folder, REDRAW_SUMMARIES); 
#endif
#endif
}

/* only do this for the spool folder */

int
TruncIfNoCache( mailbox )
char 	*mailbox;
{
	int 	fd;

	if ( !boolean_val( VarImapCache ) && mailbox && spoolfile &&
		!strcmp( spoolfile, mailbox ) && using_imap && imap_initialized ) {
		fd = open( mailbox, O_CREAT | O_RDWR | O_TRUNC, 0600 );
		if ( fd >= 0 )
			close( fd );
	}
}

int
CopyIMAPFolder( src, dest )
char *src;
char *dest;
{
	int	fd1, fd2;
	char	buf[ BUFSIZ ];
	int	n, retval = 1;

	if ( dest ) {
		fd1 = open( dest, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP );
		fd2 = open( src, O_RDONLY );	
		if ( fd1 == -1 || fd2 == -1 ) {
			error(SysErrWarning, catgets( catalog, CAT_CUSTOM, 327, "Error opening mailbox or save file in loadmail.\nOpening mail file in disconnected mode" ));
			if ( fd1 != -1 ) { 
				unlink( dest );
				close( fd1 );
			}
			else
				close( fd2 );
			retval = 0;
		}
		else {
			while ( ( n = read( fd2, buf, BUFSIZ ) ) > 0 ) {
				if ( write( fd1, buf, n ) <= 0 ) {
					error(SysErrWarning, catgets( catalog, CAT_CUSTOM, 328, "Error saving mailbox in loadmail.\nOpening mail file in disconnected mode" ));
					retval = 0;
					break;
				}
			}
			if ( retval == 1 )
				error(UserErrWarning, catgets( catalog, CAT_CUSTOM, 329, "Mailbox saved as %s" ), dest );
			else
				unlink( dest );
			close(fd1); close(fd2);
		}
	}
	else
		retval = 0;
	return( retval );
}

int
zimap_uid_match( uid )
unsigned long uid;
{
	int i, retval = -1;

      	for (i = 0; i < msg_cnt; i++)
		if ( current_folder->mf_msgs[i]->uid == uid ) {
			retval = i;
			break;
		}
  	return( retval ); 
}

int
zimap_update_deleted_local( message )
unsigned long message;
{
	unsigned long uid, uidval;
	MAILSTREAM *imapH;
	int index;

	index = zimap_uid_match( message );

	if ( index >= 0 ) {
		if ( boolean_val(VarImapShared) )
			current_folder->mf_msgs[index]->m_flags |= ZMF_EXPUNGE;
		current_folder->mf_msgs[index]->m_flags |= ZMF_DELETE;
		turnon(current_folder->mf_msgs[index]->m_flags, DO_UPDATE);
		turnon(folder_flags, DO_UPDATE);
		return( 1 );
	}
	return( 0 );
}

int
zimap_update_flags_local( message )
unsigned long message;
{
	unsigned long uid, uidval;
	MAILSTREAM *imapH;
	MESSAGECACHE *elt;
	int	index, i, which;

        imapH = (MAILSTREAM *) zimap_connect( NULL, NULL );

	if ( imapH == (MAILSTREAM *) NULL )
		return ( 0 );

#if 0
    	uidval = zimap_getuidval( imapH );

	uid = zimap_UID( imapH, (unsigned long) message );
#endif

	index = zimap_uid_match( message );

	if ( index >= 0 ) {

		for ( i = 1; i <= imapH->nmsgs; i++ ) {
			elt = mail_elt( imapH, i );
			if ( elt->private.uid == message ) {
				which = i;
				break; 
			}
			elt = (MESSAGECACHE *) NULL;
		}

		if ( elt == ( MESSAGECACHE * ) NULL )
			return ( 0 );

		if ( elt->answered == 1 ) {
			if ( !(current_folder->mf_msgs[index]->m_flags & ZMF_REPLIED) ) {
				current_folder->mf_msgs[index]->m_flags |= ZMF_REPLIED;
				current_folder->mf_msgs[index]->m_flags &= ~ZMF_UNREAD;
				current_folder->mf_msgs[index]->m_flags |= ZMF_OLD;
				turnon(current_folder->mf_msgs[index]->m_flags, DO_UPDATE);
				turnon(folder_flags, DO_UPDATE);
			}
		}
		else 
			current_folder->mf_msgs[index]->m_flags &= ~ZMF_REPLIED;
		if ( elt->deleted == 1 ) {
			if ( !(current_folder->mf_msgs[index]->m_flags & ZMF_DELETE) ) {
				current_folder->mf_msgs[index]->m_flags |= ZMF_DELETE;
				turnon(current_folder->mf_msgs[index]->m_flags, DO_UPDATE);
				turnon(folder_flags, DO_UPDATE);
			}
		}
		else if ( current_folder->mf_msgs[index]->m_flags & ZMF_DELETE ) {
			current_folder->mf_msgs[index]->m_flags &= ~ZMF_DELETE;
			turnon(current_folder->mf_msgs[index]->m_flags, DO_UPDATE);
			turnon(folder_flags, DO_UPDATE);
		}
	}
	return( which );
}

void
WriteUIDVAL( fp )
FILE *fp;
{
	unsigned long uid;
	char	*buf, *buf2;

	if ( fp && (uid = current_folder->uidval) && (buf = current_folder->imap_path) && ( buf2 = current_folder->imap_user ) )
		fprintf( fp, "UIDVAL=%08lx%s %s %c\n", uid, buf, buf2, (using_imap ? 'C' : 'D') );
}

/* This function calls loadmail, after first forking, releasing stdin,
 * stdout and stderr and ignoring any signals that might be generated.
 *
 * The above behavior is now controlled by the POP_BACKGROUND item in
 * the flags parameter.  If it is false, the check is carried on in the
 * foreground and can be interrupted (this may be undesirable).
 */

void 
imapchkmail(flags, imaplogin, imappword)
int flags;
char *imaplogin, *imappword;
{
#ifdef POP3_FORK_OK

    static int cpid = 0, numtries = 0;

    if ((flags & POP_BACKGROUND) == 0) {
	loadmail(TRUE, flags & POP_PRESERVE, imaplogin, imappword);
	return;
    }

    if (cpid > 0) {
	if (!kill(cpid, 0)) {
	    numtries++;
	    if (numtries > 10) {
		kill(cpid, SIGKILL);
	    }
	    return;
	} else {
	    numtries = 0;
	}
    }
    if (!(cpid = fork())) {
	/* Ignore several signals for workability */
	signal(SIGCONT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGSTOP, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGIO, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	loadmail(FALSE, flags & POP_PRESERVE, imaplogin, imappword);
	_exit(0);
    } else {
	if (cpid == -1) {
#ifdef NOT_ZMAIL
	    perror(catgets( catalog, CAT_CUSTOM, 330, "Unable to fork in imapchkmail\n" ));
#else /* !NOT_ZMAIL */
	    error(SysErrWarning, catgets( catalog, CAT_CUSTOM, 330, "Unable to fork in imapchkmail\n" ));
#endif /* !NOT_ZMAIL */
	}
	return;
    }
#else /* !POP3_FORK_OK */
#ifdef MAC_OS
    gui_mailcheck_feedback(TRUE);
#endif
    loadmail(TRUE, flags & POP_PRESERVE, imaplogin, imappword);
#ifdef MAC_OS
    gui_mailcheck_feedback(FALSE);
#endif
#endif /* POP3_FORK_OK */
}

/* Function called if c-client timesout. If returns non-zero, then tcp 
   connection is aborted */

int
zimap_timeout( seconds ) 
time_t	seconds;
{
	int retval = 1;
    	AskAnswer answer;

	answer = ask(AskYes, catgets( catalog, CAT_CUSTOM, 331, 
		"Connection to IMAP server timed out. Continue trying?" ));
	if ( answer == AskYes ) 
		retval = 0;
	else
		error(UserErrWarning, catgets( catalog, CAT_CUSTOM, 332, 
		"Connection timed out out after %d seconds" ), seconds );
	return( retval );
}

#endif /* IMAP */
