/*
 *  $RCSfile: syncfldr.c,v $
 *  $Revision: 2.19 $
 *  $Date: 1998/12/07 22:43:05 $
 *  $Author: schaefer $
 */

#include "config.h"

#ifdef ZPOP_SYNC

# include "zmail.h"
# include "dlist.h"
# include "zfolder.h"
# include "mfolder.h"
# include "message.h"
# include "syncfldr.h"

# ifndef lint
static char rcsid[] =
"$Header: /usr/cvsroot/zmail/custom/syncfldr.c,v 2.19 1998/12/07 22:43:05 schaefer Exp $";
# endif

extern void pop_init P((void));
extern char pop_initialized;

# ifdef _WINDOWS
extern char *gUserPass;
# endif /* _WINDOWS */

# ifdef MAC_OS
#  include "mac-stuff.h"
extern MacGlobalsPtr gMacGlobalsPtr;
# endif /* MAC_OS */

/*
 *  Function to return the age (in days) of a given message.
 */

int
message_age(p_msg)
    struct mmsg *p_msg;
{
    struct tm cur_buf, msg_buf;
    time_t now, cur_time, msg_time;
    char zone[4];

    zone[0] = 0;

    time(&now);
    bcopy(localtime(&now), &cur_buf, (sizeof (struct tm)));
    mmsg_Date(p_msg, &msg_buf);

# ifdef _WINDOWS
    sprintf(zone, "%.3s", (_daylight ? _tzname[1] : _tzname[0]));
# endif /* _WINDOWS */

    cur_time = time2gmt(&cur_buf, zone, 1);
    msg_time = time2gmt(&msg_buf, NULL, 0);

    return ((int)((cur_time - msg_time) / (60L * 60L * 24L)));
}

/*
 *  Function to call the synchronization engine to get a dlist of
 *      differences between the specified folders.
 */

void
sync_get_differences(folder1, folder2,
		     p_diff_list_A, p_diff_list_B, p_diff_list_D)
    struct mfldr *folder1, *folder2;
    struct dlist *p_diff_list_A, *p_diff_list_B, *p_diff_list_D;
{
    TRY {
	print("%s", catgets(catalog, CAT_CUSTOM, 225, "Determining folder differences...\n"));
	mfldr_Diff(folder1, folder2, p_diff_list_A, p_diff_list_B);
	mfldr_RefineDiff(folder1, folder2, p_diff_list_A, p_diff_list_B,
	    p_diff_list_D);
	mfldr_DiffStati(folder1, p_diff_list_A);
	mfldr_DiffStati(folder2, p_diff_list_B);
    }
    EXCEPT(ANY) {
# ifdef _WINDOWS
	gui_bell();
# endif /* _WINDOWS */
	error(ZmErrWarning, catgets(catalog, CAT_CUSTOM, 226, "Error during synchronization:  %s"),
	    except_GetExceptionValue());
	while (!dlist_EmptyP(p_diff_list_A))
	    dlist_Remove(p_diff_list_A, dlist_Head(p_diff_list_A));
	while (!dlist_EmptyP(p_diff_list_B))
	    dlist_Remove(p_diff_list_B, dlist_Head(p_diff_list_B));
	while (!dlist_EmptyP(p_diff_list_D))
	    dlist_Remove(p_diff_list_D, dlist_Head(p_diff_list_D));
    }
    FINALLY {
# ifdef _WINDOWS
	gui_print_status(" ");
# endif /* _WINDOWS */
    }
    ENDTRY;
}

/*
 *  Function to filter the dlist of folder differences based on the
 *  specified scenario, removing the appropriate entries.
 */

void
sync_filter_differences(folder1, folder2,
			p_diff_list_A, p_diff_list_B, p_diff_list_D,
			p_scenario)
    struct mfldr *folder1, *folder2;
    struct dlist *p_diff_list_A, *p_diff_list_B, *p_diff_list_D;
    Sync_scenario *p_scenario;
{
    int current, next;
    struct mfldr_diff *p_diff;
    struct mfldr_refinedDiff *p_refinedDiff;
    struct mmsg *p_msg;

    /*
     *  Say what we're up to.
     */
    print("%s", catgets(catalog, CAT_CUSTOM, 227, "Filtering differences...\n"));

    /*
     *  Filter dlist A by type.
     */
    dlist_FOREACH2(p_diff_list_A, struct mfldr_diff, p_diff, current, next) {
	/*
	 *  Check for delete flag.
	 */
	if (p_diff->remove) {
	    /*
	     *  Message is present in folder 1 and deleted from folder 2.
	     */
	    switch (p_scenario->folder1_deleted_handling) {
		case SYNC_DELETED_IGNORE:
		    /*
		     *  Disallow it.
		     */
		    dlist_Remove(p_diff_list_A, current);
		    break;
		case SYNC_DELETED_REMOVE:
		    /*
		     *  Allow it.
		     */
		    p_diff->apply = TRUE;
		    break;
		case SYNC_DELETED_RESTORE:
		    /*
		     *  Modify the entry to force the message to be restored
		     *  to folder 2.
		     */
		    p_diff->remove = FALSE;
		    p_diff->apply = TRUE;
		    break;
		default:
		    /*
		     *  Disallow it.
		     */
		    dlist_Remove(p_diff_list_A, current);
		    break;
	    }
	} else if (p_diff->status & (mmsg_status_NEW | mmsg_status_UNREAD)) {
	    /*
	     *  Message is present and unread in folder 1 and absent in
	     *  folder 2.
	     */
	    switch (p_scenario->folder1_unread_handling) {
		case SYNC_UNIQUE_IGNORE:
		    /*
		     *  Disallow it.
		     */
		    dlist_Remove(p_diff_list_A, current);
		    break;
		case SYNC_UNIQUE_COPY:
		    /*
		     *  Allow it.
		     */
		    p_diff->apply = TRUE;
		    break;
		case SYNC_UNIQUE_DELETE:
		    /*
		     *  Modify the entry to force the message to be deleted.
		     */
		    p_diff->remove = TRUE;
		    p_diff->apply = TRUE;
		    break;
		default:
		    /*
		     *  Disallow it.
		     */
		    dlist_Remove(p_diff_list_A, current);
		    break;
	    }
	} else {
	    /*
	     *  Message is present and read in folder 1 and absent in
	     *  folder 2.
	     */
	    switch (p_scenario->folder1_read_handling) {
		case SYNC_UNIQUE_IGNORE:
		    /*
		     *  Disallow it.
		     */
		    dlist_Remove(p_diff_list_A, current);
		    break;
		case SYNC_UNIQUE_COPY:
		    /*
		     *  Allow it.
		     */
		    p_diff->apply = TRUE;
		    break;
		case SYNC_UNIQUE_DELETE:
		    /*
		     *  Modify the entry to force the message to be deleted.
		     */
		    p_diff->remove = TRUE;
		    p_diff->apply = TRUE;
		    break;
		default:
		    /*
		     *  Disallow it.
		     */
		    dlist_Remove(p_diff_list_A, current);
		    break;
	    }
	}
    }

    /*
     *  Filter dlist B by type.
     */
    dlist_FOREACH2(p_diff_list_B, struct mfldr_diff, p_diff, current, next) {
	/*
	 *  Check for delete flag.
	 */
	if (p_diff->remove) {
	    /*
	     *  Message is present in folder 2 and deleted from folder 1.
	     */
	    switch (p_scenario->folder2_deleted_handling) {
		case SYNC_DELETED_IGNORE:
		    /*
		     *  Disallow it.
		     */
		    dlist_Remove(p_diff_list_B, current);
		    break;
		case SYNC_DELETED_REMOVE:
		    /*
		     *  Allow it.
		     */
		    p_diff->apply = TRUE;
		    break;
		case SYNC_DELETED_RESTORE:
		    /*
		     *  Modify the entry to force the message to be restored
		     *  to folder 1.
		     */
		    p_diff->remove = FALSE;
		    p_diff->apply = TRUE;
		    break;
		default:
		    /*
		     *  Disallow it.
		     */
		    dlist_Remove(p_diff_list_B, current);
		    break;
	    }
	} else if (p_diff->status & (mmsg_status_NEW | mmsg_status_UNREAD)) {
	    /*
	     *  Message is present and unread in folder 2 and absent in
	     *  folder 1.
	     */
	    switch (p_scenario->folder2_unread_handling) {
		case SYNC_UNIQUE_IGNORE:
		    /*
		     *  Disallow it.
		     */
		    dlist_Remove(p_diff_list_B, current);
		    break;
		case SYNC_UNIQUE_COPY:
		    /*
		     *  Allow it.
		     */
		    p_diff->apply = TRUE;
		    break;
		case SYNC_UNIQUE_DELETE:
		    /*
		     *  Modify the entry to force the message to be deleted.
		     */
		    p_diff->remove = TRUE;
		    p_diff->apply = TRUE;
		    break;
		default:
		    /*
		     *  Disallow it.
		     */
		    dlist_Remove(p_diff_list_B, current);
		    break;
	    }
	} else {
	    /*
	     *  Message is present and read in folder 2 and absent in
	     *  folder 1.
	     */
	    switch (p_scenario->folder2_read_handling) {
		case SYNC_UNIQUE_IGNORE:
		    /*
		     *  Disallow it.
		     */
		    dlist_Remove(p_diff_list_B, current);
		    break;
		case SYNC_UNIQUE_COPY:
		    /*
		     *  Allow it.
		     */
		    p_diff->apply = TRUE;
		    break;
		case SYNC_UNIQUE_DELETE:
		    /*
		     *  Modify the entry to force the message to be deleted.
		     */
		    p_diff->remove = TRUE;
		    p_diff->apply = TRUE;
		    break;
		default:
		    /*
		     *  Disallow it.
		     */
		    dlist_Remove(p_diff_list_B, current);
		    break;
	    }
	}
    }

    /*
     *  Filter dlist D by type.
     */
    dlist_FOREACH2(p_diff_list_D, struct mfldr_refinedDiff, p_refinedDiff,
	current, next) {
	/*
	 *  Message is present in both folders, but has header differences.
	 */
	switch (p_scenario->common_handling) {
	    case SYNC_COMMON_IGNORE:
		/*
		 *  Disallow it.
		 */
		dlist_Remove(p_diff_list_D, current);
		break;
	    case SYNC_COMMON_COPY_TO_FOLDER1:
		/*
		 *  Force copy from folder 2 to folder 1.
		 */
		p_refinedDiff->apply1 = TRUE;
		break;
	    case SYNC_COMMON_COPY_TO_FOLDER2:
		/*
		 *  Force copy from folder 1 to folder 2.
		 */
		p_refinedDiff->apply2 = TRUE;
		break;
	    default:
		/*
		 *  Disallow it.
		 */
		dlist_Remove(p_diff_list_D, current);
		break;
	}
    }

    /*
     *  Filter dlist A by size and age.
     */
    dlist_FOREACH2(p_diff_list_A, struct mfldr_diff, p_diff, current, next) {
	/*
	 *  Don't process deleted messages.
	 */
	if (p_diff->remove)
	    continue;

	p_msg = mfldr_NthMessage(folder1, p_diff->num);

	if (p_diff->status & (mmsg_status_NEW | mmsg_status_UNREAD)) {
	    /*
	     *  Message is unread.  Check for a size limit.
	     */
	    if (p_scenario->folder1_unread_size_filter) {
		if ((int)(mmsg_Size(p_msg) / 1024) >
		    p_scenario->folder1_unread_size_limit) {
		    dlist_Remove(p_diff_list_A, current);
		    continue;
		}
	    }
	    /*
	     *  Check for an age limit as well.
	     */
	    if (p_scenario->folder1_unread_age_filter) {
		if (message_age(p_msg) >
		    p_scenario->folder1_unread_age_limit) {
		    dlist_Remove(p_diff_list_A, current);
		}
	    }
	} else {
	    /*
	     *  Message is read.  Check for a size limit.
	     */
	    if (p_scenario->folder1_read_size_filter) {
		if ((int)(mmsg_Size(p_msg) / 1024) >
		    p_scenario->folder1_read_size_limit) {
		    dlist_Remove(p_diff_list_A, current);
		    continue;
		}
	    }
	    /*
	     *  Check for an age limit as well.
	     */
	    if (p_scenario->folder1_read_age_filter) {
		if (message_age(p_msg) >
		    p_scenario->folder1_read_age_limit) {
		    dlist_Remove(p_diff_list_A, current);
		}
	    }
	}
    }

    /*
     *  Filter dlist B by size and age.
     */
    dlist_FOREACH2(p_diff_list_B, struct mfldr_diff, p_diff, current, next) {
	/*
	 *  Don't process deleted messages.
	 */
	if (p_diff->remove)
	    continue;

	p_msg = mfldr_NthMessage(folder2, p_diff->num);

	if (p_diff->status & (mmsg_status_NEW | mmsg_status_UNREAD)) {
	    /*
	     *  Message is unread.  Check for a size limit.
	     */
	    if (p_scenario->folder2_unread_size_filter) {
		if ((int)(mmsg_Size(p_msg) / 1024) >
		    p_scenario->folder2_unread_size_limit) {
		    dlist_Remove(p_diff_list_B, current);
		    continue;
		}
	    }
	    /*
	     *  Check for an age limit as well.
	     */
	    if (p_scenario->folder2_unread_age_filter) {
		if (message_age(p_msg) >
		    p_scenario->folder2_unread_age_limit) {
		    dlist_Remove(p_diff_list_B, current);
		}
	    }
	} else {
	    /*
	     *  Message is read.  Check for a size limit.
	     */
	    if (p_scenario->folder2_read_size_filter) {
		if ((int)(mmsg_Size(p_msg) / 1024) >
		    p_scenario->folder2_read_size_limit) {
		    dlist_Remove(p_diff_list_B, current);
		    continue;
		}
	    }
	    /*
	     *  Check for an age limit as well.
	     */
	    if (p_scenario->folder2_read_age_filter) {
		if (message_age(p_msg) >
		    p_scenario->folder2_read_age_limit) {
		    dlist_Remove(p_diff_list_B, current);
		}
	    }
	}
    }
# ifdef _WINDOWS
    gui_print_status(" ");
# endif /* _WINDOWS */
}

/*
 *  Function to pass the filtered dlist of folder differences back
 *      to the synchronization engine, which will then make the implied
 *      folder changes.
 */
void
sync_apply_differences(folder1, folder2,
		       p_diff_list_A, p_diff_list_B, p_diff_list_D)
    struct mfldr *folder1, *folder2;
    struct dlist *p_diff_list_A, *p_diff_list_B, *p_diff_list_D;
{
    TRY {
	print("%s", catgets(catalog, CAT_CUSTOM, 228, "Updating folders...\n"));
	mfldr_Sync(folder1, folder2, p_diff_list_A, p_diff_list_B,
	    p_diff_list_D, (VPTR)0, (VPTR)0);
    }
    EXCEPT(ANY) {
# ifdef _WINDOWS
	gui_bell();
# endif /* _WINDOWS */
	error(ZmErrWarning, catgets(catalog, CAT_CUSTOM, 229, "Error during synchronization:  %s"),
	    except_GetExceptionValue());
    }
    FINALLY {
# ifdef _WINDOWS
	gui_print_status(" ");
# endif /* _WINDOWS */
    }
    ENDTRY;
}

/*
 *  Flags table for the Z-Script zpop_sync function.
 */
# ifdef WIN16
static const char * __based(__segname("_CODE")) zpop_sync_flags[][2] =
{
# else
static char *zpop_sync_flags[][2] =
{
# endif				/* WIN16 */
    {"-folders", "-f"},
    {"-spools", "-s"},
    {NULL, NULL}
};

/*
 *  Invoked by the Z-Script zpop_sync function.
 */
int
zm_zpop_sync(argc, argv)
int argc;
char **argv;
{
    int status;
    int spools = TRUE;
    PopServer ps = NULL;
    Sync_scenario *pSyncScenario = NULL;
#ifdef UNIX
    static char passwd[64];
#else /* !UNIX */
    char passwd[64];
#endif /* !UNIX */
    msg_folder *folder1, *folder2;
    struct mfldr *mf;
    char *plogin = value_of(VarPopUser);

    if (!plogin || !*plogin)
	plogin = zlogin;

    /*
     *  Parse the -folders & -spools arguments.
     */
    while (*++argv && **argv == '-') {
	fix_word_flag(argv, zpop_sync_flags);
	switch (argv[0][1]) {
	    case 'f':
		spools = FALSE;
		break;
	    case 's':
		spools = TRUE;
		break;
	    default:
# ifdef _WINDOWS
		gui_bell();
# endif /* _WINDOWS */
		error(UserErrWarning,
		    catgets(catalog, CAT_CUSTOM, 230, "zpop_sync: unrecognized option: %s"),
		    *argv);
		return -1;
	}
    }

    /*
     *  Initialize the SPOOR objects.
     */
    zpop_initialize();

    /*
     *  Synchronizing spools.
     */
    if (spools) {
	int current_count = open_folders[0]->mf_count;

	/*
	 * Make sure the folder is open!
	 */
	if (isoff(open_folders[0]->mf_flags, CONTEXT_IN_USE)) {
# ifdef _WINDOWS
	    gui_bell();
# endif /* _WINDOWS */
	    error(UserErrWarning,
		catgets(catalog, CAT_CUSTOM, 231, "Cannot synchronize when mailbox is closed"));
	    return -1;
	}

	/*
	 *  Make sure that we're connected via TCP/IP.
	 */
# ifdef _WINDOWS
	if ((!boolean_val(VarConnected)) || (ZmUsingUUCP()) || (!ZmUsingPOP()))
# else /* !_WINDOWS */
#  ifdef MAC_OS
	if (!boolean_val(VarConnected) || !boolean_val(VarUsePop))
#  else /* !MAC_OS */
	if (!boolean_val(VarUsePop))
#  endif /* !MAC_OS */
# endif /* !_WINDOWS */
	{
# ifdef _WINDOWS
	    gui_bell();
# endif /* _WINDOWS */
	    error(UserErrWarning,
		catgets(catalog, CAT_CUSTOM, 232, "Cannot synchronize mailboxes without a Z-POP connection"));
	    return -1;
	}

	/*
	 *  Make sure that we've got a POP password first.
	 */
# ifndef UNIX
	if (!pop_initialized) {
	    pop_init();
	    if (!pop_initialized)
		return -1;
	}
# else /* UNIX */
	if (!passwd[0] &&
	    choose_one(passwd,
		       catgets(catalog, CAT_CUSTOM, 233, "Enter Z-POP password:"),
		       NULL, NULL, 0, PB_NO_ECHO))
	    return -1;
# endif /* UNIX */
# ifdef _WINDOWS
	strcpy(passwd, gUserPass);
# endif /* _WINDOWS */
# ifdef MAC_OS
	strcpy(passwd, gMacGlobalsPtr->pass);
# endif /* MAC_OS */

	/*
	 *  Open the (Z-)POP connection.
	 */
	print("%s", catgets(catalog, CAT_CUSTOM, 234, "Connecting to mail host..."));
	if (!(ps = pop_open(value_of(VarMailhost), plogin, passwd,
		    POP_NO_GETPASS))) {
# ifdef _WINDOWS
	    gui_bell();
# endif /* _WINDOWS */
	    error(ZmErrWarning,
		catgets(catalog, CAT_CUSTOM, 235, "Error opening connection with post office: %s\n"),
		pop_error);
	    passwd[0] = 0;
	    print("%s", catgets(catalog, CAT_CUSTOM, 236, " failed.\n"));
	    return -1;
	}
	print("%s", " \n");

	/*
	 *  Make sure that the server is capable of sync (Z-POP).
	 */
	if (!pop_services(ps, POP3_ZPOP)) {
# ifdef _WINDOWS
	    gui_bell();
# endif /* _WINDOWS */
	    error(UserErrWarning,
		catgets(catalog, CAT_CUSTOM, 237, "Your mail server doesn't support synchronization"));
	    pop_close(ps);
	    return -1;
	}

	/*
         *  Call the synchronizer.
	 */

	/*
	 * Init -- select scenario and update folders
	 */
	pSyncScenario = init_sync_scenario(*argv, spools);

	if (pSyncScenario) {
	    struct mfldr *corefolder =
		(struct mfldr *) core_to_mfldr(open_folders[0]);

	    /*
	     *  Start the POP server keepalive timer.
	     */
	    sync_start_keepalive(ps);

	    TRY {

		/*
		 * Since we found a scenario, do the sync
		 */
		status = sync_folders(corefolder,
				      (struct mfldr *) zpop_to_mfldr(ps),
				      pSyncScenario);

	    } EXCEPT(ANY) {

		error(ZmErrWarning,
		      catgets(catalog, CAT_CUSTOM, 226, "Error during synchronization:  %s"),
		      except_GetExceptionValue());

	    } FINALLY {

		/*
		 *  Stop the keepalive timer.
		 */
		sync_stop_keepalive();

	    } ENDTRY;
	}

	/*
	 *  Close the POP connection.
	 */
	pop_quit(ps);

	/*
	 *  Filter the new messages.
	 */
	if (msg_cnt >= 0 && current_count < msg_cnt) {
	    int i;
	    msg_group new_list;

	    init_msg_group(&new_list, msg_cnt, 1);
	    clear_msg_group(&new_list);
	    for (i = current_count; i < msg_cnt; i++) {
		if (ison(msg[i]->m_flags, UNREAD))
		    add_msg_to_group(&new_list, i);
		if (current_msg < 0)
		    current_msg = i;
	    }
	    if (count_msg_list(&new_list) > 0) {
		if (folder_filters)
		    filter_msg(NULL, &new_list, folder_filters);
		if (new_filters)
		    filter_msg(NULL, &new_list, new_filters);
		/* PRs 2710 and 3941, mail deleted by filters isn't new */
		for (i = current_count; i < msg_cnt; i++) {
		    if (msg_is_in_group(&new_list, i) &&
			    isoff(msg[i]->m_flags, DELETE)) {
			turnon(folder_flags, NEW_MAIL);
			break;
		    }
		}
	    }
	    destroy_msg_group(&new_list);
	}

# ifdef GUI
	/*
	 *  Refresh the folder windows.
	 */
	gui_refresh(open_folders[0], REDRAW_SUMMARIES);
# endif /* GUI */

        /*
	 * Cleanup
	 */
        cleanup_sync_scenario();
    } else {
	/*
	 *  Synchronizing (open) folders.  Get the folder pointers.
	 */
	if (*argv) {
	    folder1 = lookup_folder(*argv, -1, NO_FLAGS);
	    if (folder1 == NULL_FLDR) {
# ifdef _WINDOWS
		gui_bell();
# endif /* _WINDOWS */
		error(UserErrWarning,
		    catgets(catalog, CAT_CUSTOM, 238, "zpop_sync: invalid folder specification: %s"),
		    *argv);
		return -1;
	    }
	} else {
# ifdef _WINDOWS
	    gui_bell();
# endif /* _WINDOWS */
	    error(UserErrWarning,
		catgets(catalog, CAT_CUSTOM, 239, "zpop_sync: missing folder specification"));
	    return -1;
	}

	if (*++argv) {
	    folder2 = lookup_folder(*argv, -1, NO_FLAGS);
	    if (folder2 == NULL_FLDR) {
# ifdef _WINDOWS
		gui_bell();
# endif /* _WINDOWS */
		error(UserErrWarning,
		    catgets(catalog, CAT_CUSTOM, 240, "zpop_sync: invalid folder specification: %s"),
		    *argv);
		return -1;
	    }
	} else {
# ifdef _WINDOWS
	    gui_bell();
# endif /* _WINDOWS */
	    error(UserErrWarning,
		catgets(catalog, CAT_CUSTOM, 241, "zpop_sync: missing folder specification"));
	    return -1;
	}

	/*
	 *  Call the synchronizer.
	 */

	/*
	 * Init -- select scenario and update folders
	 */
	pSyncScenario = init_sync_scenario(*++argv, spools);

	if (pSyncScenario) {
	    /*
	     * Since we found a scenario, do the sync
	     */
	    status = sync_folders((struct mfldr *)core_to_mfldr(folder1),
				  (struct mfldr *)core_to_mfldr(folder2),
                              	  pSyncScenario);
	}

# ifdef GUI
	/*
	 *  Refresh the folder windows.
	 */
	gui_refresh(folder1, REDRAW_SUMMARIES);
	gui_refresh(folder2, REDRAW_SUMMARIES);
# endif /* GUI */

	/*
	 * Cleanup
	 */
        cleanup_sync_scenario();
    }
    print("%s", catgets(catalog, CAT_CUSTOM, 242, "Sync complete.\n"));

    /*
     *  Return the appropriate status.
     */
    return status;
}

# ifndef _WINDOWS

#include "except.h"
#include "dynstr.h"

/* This array has to correspond to the array of scenarios below.
 * Choice of dynstrs for the name and description fields was not
 * ideal from the standpoint of statically predefining scenarios.
 * (If we had C++ constructors and dynstrs were a class ....)
 */
static struct sync_name {
    char *name;
    catalog_ref desc;
} fixed_scenario_names[] = {
    { "Default Scenario",	catref(CAT_CUSTOM, 243, "Default Scenario")	},
    { "Quick Check In",		catref(CAT_CUSTOM, 244, "Quick Check In")	},
    { "Quick Check Out",	catref(CAT_CUSTOM, 245, "Quick Check Out")	},
    { "Quick Check Mail",	catref(CAT_CUSTOM, 246, "Quick Check Mail")	},
    { 0 }
};

static Sync_scenario fixed_scenarios[] = {
    {
	{ 0 },	/* Default Scenario */
	{ 0 },

	/* Both folders */
	SYNC_COMMON_IGNORE,

	/* Folder 1 read */
	SYNC_UNIQUE_COPY,
	0, /* No size filter */
	100, /* SYNC_DEFAULT_SIZE_LIMIT */
	0, /* No age filter */
	7, /* SYNC_DEFAULT_AGE_LIMIT */

	/* Folder 2 read */
	SYNC_UNIQUE_COPY,
	0, /* No size filter */
	100, /* SYNC_DEFAULT_SIZE_LIMIT */
	0, /* No age filter */
	7, /* SYNC_DEFAULT_AGE_LIMIT */

	/* Folder 1 unread */
	SYNC_UNIQUE_COPY,
	0, /* No size filter */
	100, /* SYNC_DEFAULT_SIZE_LIMIT */
	0, /* No age filter */
	7, /* SYNC_DEFAULT_AGE_LIMIT */

	/* Folder 2 unread */
	SYNC_UNIQUE_COPY,
	0, /* No size filter */
	100, /* SYNC_DEFAULT_SIZE_LIMIT */
	0, /* No age filter */
	7, /* SYNC_DEFAULT_AGE_LIMIT */

	/* Misc. */
	SYNC_DELETED_IGNORE,	/* Folder 1 */
	SYNC_DELETED_IGNORE,	/* Folder 2 */
	SYNC_PREVIEW_NEVER,
	0 /* Not read-only */
    },
    {
	{ 0 },	/* Quick Check In */
	{ 0 },

	/* Both folders */
	SYNC_COMMON_COPY_TO_FOLDER2,

	/* Folder 1 read */
	SYNC_UNIQUE_COPY,
	0, /* No size filter */
	100, /* SYNC_DEFAULT_SIZE_LIMIT */
	0, /* No age filter */
	7, /* SYNC_DEFAULT_AGE_LIMIT */

	/* Folder 2 read */
	SYNC_UNIQUE_IGNORE,
	0, /* No size filter */
	100, /* SYNC_DEFAULT_SIZE_LIMIT */
	0, /* No age filter */
	7, /* SYNC_DEFAULT_AGE_LIMIT */

	/* Folder 1 unread */
	SYNC_UNIQUE_COPY,
	0, /* No size filter */
	100, /* SYNC_DEFAULT_SIZE_LIMIT */
	0, /* No age filter */
	7, /* SYNC_DEFAULT_AGE_LIMIT */

	/* Folder 2 unread */
	SYNC_UNIQUE_IGNORE,
	0, /* No size filter */
	100, /* SYNC_DEFAULT_SIZE_LIMIT */
	0, /* No age filter */
	7, /* SYNC_DEFAULT_AGE_LIMIT */

	/* Misc. */
	SYNC_DELETED_IGNORE,	/* Folder 1 */
	SYNC_DELETED_REMOVE,	/* Folder 2 */
	SYNC_PREVIEW_NEVER,	/* Ideally, INQUIRE */
	0 /* Not read-only */
    },
    {
	{ 0 },	/* Quick Check Out */
	{ 0 },

	/* Both folders */
	SYNC_COMMON_COPY_TO_FOLDER1,

	/* Folder 1 read */
	SYNC_UNIQUE_DELETE,
	0, /* No size filter */
	100, /* SYNC_DEFAULT_SIZE_LIMIT */
	0, /* No age filter */
	7, /* SYNC_DEFAULT_AGE_LIMIT */

	/* Folder 2 read */
	SYNC_UNIQUE_COPY,
	0, /* No size filter */
	100, /* SYNC_DEFAULT_SIZE_LIMIT */
	0, /* No age filter */
	7, /* SYNC_DEFAULT_AGE_LIMIT */

	/* Folder 1 unread */
	SYNC_UNIQUE_DELETE,
	0, /* No size filter */
	100, /* SYNC_DEFAULT_SIZE_LIMIT */
	0, /* No age filter */
	7, /* SYNC_DEFAULT_AGE_LIMIT */

	/* Folder 2 unread */
	SYNC_UNIQUE_COPY,
	0, /* No size filter */
	100, /* SYNC_DEFAULT_SIZE_LIMIT */
	0, /* No age filter */
	7, /* SYNC_DEFAULT_AGE_LIMIT */

	/* Misc. */
	SYNC_DELETED_IGNORE,	/* Folder 1 */
	SYNC_DELETED_RESTORE,	/* Folder 2 */
	SYNC_PREVIEW_NEVER,	/* Ideally, INQUIRE */
	0 /* Not read-only */
    },
    {
	{ 0 },	/* Quick Check Mail */
	{ 0 },

	/* Both folders */
	SYNC_COMMON_IGNORE,

	/* Folder 1 read */
	SYNC_UNIQUE_IGNORE,
	0, /* No size filter */
	100, /* SYNC_DEFAULT_SIZE_LIMIT */
	0, /* No age filter */
	7, /* SYNC_DEFAULT_AGE_LIMIT */

	/* Folder 2 read */
	SYNC_UNIQUE_IGNORE,
	0, /* No size filter */
	100, /* SYNC_DEFAULT_SIZE_LIMIT */
	0, /* No age filter */
	7, /* SYNC_DEFAULT_AGE_LIMIT */

	/* Folder 1 unread */
	SYNC_UNIQUE_IGNORE,
	0, /* No size filter */
	100, /* SYNC_DEFAULT_SIZE_LIMIT */
	0, /* No age filter */
	7, /* SYNC_DEFAULT_AGE_LIMIT */

	/* Folder 2 unread */
	SYNC_UNIQUE_COPY,
	0, /* No size filter */
	100, /* SYNC_DEFAULT_SIZE_LIMIT */
	0, /* No age filter */
	7, /* SYNC_DEFAULT_AGE_LIMIT */

	/* Misc. */
	SYNC_DELETED_IGNORE,	/* Folder 1 */
	SYNC_DELETED_IGNORE,	/* Folder 2 */
	SYNC_PREVIEW_NEVER,
	0 /* Not read-only */
    }
};

#define dummy_scenario fixed_scenarios[0]

static msg_folder *orig_folder;
static int keeping_alive;
static PopServer saved_ps;

static Sync_scenario *
lookup_scenario(scenario_name)
char *scenario_name;
{
    static int initialized = 0;
    Sync_scenario *found = 0;
    int next;

    for (next = 0; fixed_scenario_names[next].name; next++) {
	if (!initialized) {
	    dynstr_InitFrom(&(fixed_scenarios[next].name),
			    fixed_scenario_names[next].name);
	    dynstr_InitFrom(&(fixed_scenarios[next].description),
			    catgetref(fixed_scenario_names[next].desc));
	}
	if (ci_strcmp(fixed_scenario_names[next].name, scenario_name) == 0) {
	    found = &fixed_scenarios[next];
	    if (initialized)
		break;
	}
    }

    return found;
}

Sync_scenario *
init_sync_scenario(scenario_name, spools)
char *scenario_name;
int spools;
{
    Sync_scenario *desired = lookup_scenario(scenario_name);
    char buf[128];

    saved_ps = 0;
    orig_folder = current_folder;

    if (!is_shell || !desired)
	return 0;
    if (spools)
	current_folder = open_folders[0];

    /*
     * Figure out whether we need to update before applying this scenario.
     * If not, return it and go ahead; otherwise force an update.
     */
    if (spools &&
	    desired->common_handling == SYNC_COMMON_IGNORE &&
	    desired->folder1_read_handling == SYNC_UNIQUE_IGNORE &&
	    desired->folder1_unread_handling == SYNC_UNIQUE_IGNORE &&
	    desired->folder1_deleted_handling == SYNC_DELETED_IGNORE &&
	    desired->folder2_deleted_handling == SYNC_DELETED_IGNORE)
	return desired;

    sprintf(buf, "update %s", spools? "%" : get_var_value("openfolders"));

    if (ask(WarnOk,
	    spools?
	    catgets(catalog, CAT_CUSTOM, 247, "This operation updates your mail spool. Continue?"):
	    catgets(catalog, CAT_CUSTOM, 248, "This operation updates ALL OPEN FOLDERS. Continue?")
	   ) == AskYes &&
	cmd_line(buf, NULL_GRP) == 0)
	    return desired;
    return 0;
}

void
cleanup_sync_scenario()
{
#  ifdef GUI
    if (current_folder != orig_folder)
	gui_refresh(current_folder = orig_folder, REDRAW_SUMMARIES);
#  endif /* GUI */
    orig_folder = 0;
    saved_ps = 0;
}

void
sync_start_keepalive(ps)
PopServer ps;
{
    if (ps) {
	if (keeping_alive)
	    return;
	saved_ps = ps;
    } else {
	if (!keeping_alive)
	    return;
    }
    keeping_alive = 1;

    /* ADD TIMER CODE HERE */
}

void
sync_stop_keepalive()
{
    keeping_alive = 0;

    /* ADD TIMER CODE HERE */
}

int
sync_folders(mfolder1, mfolder2, scenario)
struct mfldr *mfolder1, *mfolder2;
Sync_scenario *scenario;
{
    int preview = 1;
    struct dlist diff_list_A, diff_list_B, diff_list_D;

    switch (scenario->preview_handling) {
	case SYNC_PREVIEW_ALWAYS:
	    preview = 1;
	    break;
	case SYNC_PREVIEW_NEVER:
	    preview = 0;
	    break;
	case SYNC_PREVIEW_INQUIRE:
#ifdef GUI
	    switch (gui_sync_ask()) {
		case AskYes:
		    preview = 1;
		    break;
		case AskNo:
#endif /* GUI */
		    preview = 0;
#ifdef GUI
		    break;
		case AskCancel:
		    return -1;
	    }
#endif /* GUI */
	    break;
    }

    dlist_Init(&diff_list_A, sizeof (struct mfldr_diff), SYNC_DLIST_GROWSIZE);
    dlist_Init(&diff_list_B, sizeof (struct mfldr_diff), SYNC_DLIST_GROWSIZE);
    dlist_Init(&diff_list_D, sizeof (struct mfldr_refinedDiff),
	       SYNC_DLIST_GROWSIZE);

#ifdef GUI
    if (istool)
	timeout_cursors(1);	/* Disable mail check, turn on wait cursor */
#endif /* GUI */

    TRY {

    /* Get the lists of messages involved */
    sync_stop_keepalive();
    TRY {
	sync_get_differences(mfolder1, mfolder2,
			     &diff_list_A, &diff_list_B, &diff_list_D);
    } FINALLY {
	sync_start_keepalive((PopServer)0);
    } ENDTRY;

    sync_filter_differences(mfolder1, mfolder2,
			    &diff_list_A, &diff_list_B, &diff_list_D,
			    scenario);

    if ((!dlist_EmptyP(&diff_list_A)) || (!dlist_EmptyP(&diff_list_B)) ||
	    (!dlist_EmptyP(&diff_list_D))) {
	int apply = 1;

#ifdef GUI
	/* Preview if necessary */
	if (preview)
	    apply = gui_sync_preview(mfolder1, mfolder2,
				     &diff_list_A, &diff_list_B,
				     &diff_list_D);
#endif /* GUI */

	if (apply) {
	    sync_stop_keepalive();
	    TRY {
		sync_apply_differences(mfolder1, mfolder2,
				       &diff_list_A, &diff_list_B,
				       &diff_list_D);
	    } FINALLY {
		sync_start_keepalive((PopServer)0);
	    } ENDTRY;
	}
    } else {
	if (preview)
	    error(Message,
		catgets(catalog, CAT_CUSTOM, 249, "Based on scenario \"%s\"\nno action was required."),
		dynstr_Str(&scenario->name));
    }

    } FINALLY {
	dlist_Destroy(&diff_list_A);
	dlist_Destroy(&diff_list_B);
	dlist_Destroy(&diff_list_D);

#ifdef GUI
	if (istool)
	    timeout_cursors(0);	/* Restore cursor, restart timers */
#endif /* GUI */
    } ENDTRY;

    return 0;
}

# endif /* !_WINDOWS */

#endif /* ZPOP_SYNC */
