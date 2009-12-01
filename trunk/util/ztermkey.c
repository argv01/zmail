#include <zcunix.h>
#include <zcerr.h>
#include <zcsig.h>
#include <zcstr.h>
#include <zctime.h>
#include <zcfctl.h>
#include <except.h>

#include "ztermkey.h"

#define ZTERMKEYVERSION "ZTermKey (3.2.9 26may94)\n"

/* The following are stolen from Bob.  What more can I say. */

#if !defined(HAVE_PUTP) && !defined(putp)
# ifdef putchar
static int
putcharfn(c)
    int c;
{
    return (putchar(c));
}
# else /* !putchar */
#  define putcharfn putchar
# endif /* !putchar */

# define putp(s) (tputs((s),1,putcharfn))
#endif /* !HAVE_PUTP && !putp */

#ifndef HAVE_TIGETSTR
extern char *tgetstr();

static char *
tigetstr(tiname)		/* this fn only covers calls to tigetstr */
    char *tiname;		/* used in this file */
{
    static char pointless[128];
    char *pointlessp = pointless;
    char *tcname;

    if (!strcmp(tiname, "smkx")) {
	tcname = "ks";
    } else if (!strcmp(tiname, "smln")) {
	tcname = "LO";
    } else if (!strcmp(tiname, "rmln")) {
	tcname = "LF";
    } else if (!strcmp(tiname, "pln")) {
	tcname = "pn";
    } else if (!strcmp(tiname, "enacs")) {
	tcname = "eA";
    } else if (!strcmp(tiname, "rmcup")) {
	tcname = "te";
    } else if (!strcmp(tiname, "rmkx")) {
	tcname = "ke";
    } else if (!strcmp(tiname, "smcup")) {
	tcname = "ti";
    } else if (!strcmp(tiname, "kcud1")) {
	tcname = "kd";
    } else if (!strcmp(tiname, "kcuu1")) {
	tcname = "ku";
    } else if (!strcmp(tiname, "kcub1")) {
	tcname = "kl";
    } else if (!strcmp(tiname, "kcuf1")) {
	tcname = "kr";
    } else if (!strcmp(tiname, "kcbt")) {
	tcname = "kB";
    } else if (!strcmp(tiname, "khome")) {
	tcname = "kh";
    } else if (!strcmp(tiname, "kend")) {
	tcname = "@7";
    } else if (!strcmp(tiname, "kpp")) {
	tcname = "kP";
    } else if (!strcmp(tiname, "knp")) {
	tcname = "kN";
    } else if (!strcmp(tiname, "kdch1")) {
	tcname = "kD";
    } else if (!strcmp(tiname, "kich1")) {
	tcname = "kI";
    } else if (!strncmp(tiname, "kf", 2)) {
	int i = atoi(tiname +2);
	static char buf[16];

	if ((i < 0) || (i > 63))
	    return (NULL);
	if (i < 10) {
	    sprintf(buf, "k%c", i + '0');
	} else if (i == 10) {
	    strcpy(buf, "k;");
	} else if (i < 20) {
	    sprintf(buf, "F%c", i - 10 + '0');
	} else if (i < 46) {
	    sprintf(buf, "F%c", i - 20 + 'A');
	} else {
	    sprintf(buf, "F%c", i - 46 + 'a');
	}
	tcname = buf;
    } else {
	RAISE(strerror(EINVAL), "tigetstr");
	return (NULL);
    }
    return (tgetstr(tcname, &pointlessp));
}
#else  /* HAVE_TIGETSTR */
extern char *tigetstr P((char *));
#endif /* !HAVE_TIGETSTR */

/* Comments are the main reason we should never sell source licenses. */
/* I swear to God, this is not something I wrote in high school Pascal */
/* and hacked to work here.  Really */

main(argc, argv)
    int argc;
    char *argv[];
{
    KeyHolderType keys;
    char *envterm;
    char termin[80];

    if ((argc > 1) && strcmp(argv[0], "-version")) {
	fprintf(stdout, ZTERMKEYVERSION);
	fprintf(stdout, "Copyright 1990-1998 NetManage, Inc.  All rights reserved.\n" );
	exit(1);
    }

    keys.NumFunctionKeys = 0;
    keys.NumOurKeys = 0;
    keys.TheirKeys = NULL;

    envterm = getenv("LITETERM");
    if (!envterm)
	envterm = getenv("TERM");
    fprintf(stdout,"Terminal type? ");
    if (envterm)
	fprintf(stdout,"[%s] ", envterm);
    fgets(termin, 80, stdin);
    if ((!*termin) || (*termin == '\r') || (*termin == '\n')) {
	if (envterm) {
	    strcpy(termin, envterm);
	} else {
	    strcpy(termin, "unknown");
	}
    } else {
	termin[strlen(termin) -1] = '\0'; /* Zorch \n from fgets. */
    }

    fprintf(stdout, "Terminal type set to %s.\n", termin);

#ifdef HAVE_NCURSES
    newterm( termin, stdout, stdin );
    endwin();
    setbuf( stdout, (char *) NULL );
#endif
    fprintf(stdout, "ztermkey will now ask you to press certain keys on your keyboard\nso it can record the hardware dependant keysequences.\nIf you DO NOT have the named key, then PRESS the SPACEBAR to go\non to the next question.\n");

    /* get the output of the function keys */
    getFunctionKeys(&keys);

    /* get the output of any special keys mentioned in the multikey dialog */
    getDialogKeys(&keys);

    /* get any other keys the user wants to define. */
    getOtherKeys(&keys);

    /* spit out the multikey entries into a file */
    dumpKeybindings(&keys, termin);

}

char *
duplicateKey(new, keys)
    KeyEntryType *new;
    KeyHolderType *keys;
{
    int i, j, match;
    KeyEntryLLType *walker;

    /* This is something of a pain if the new key is already */	
    /* in the structure in which we are checking for duplicates */
    /* so instead we need to make sure that we are always passed */
    /* the new key in its own piece of memory. */

    /* check against other function keys defined so far */
    /* it would be good to know how many functions keys we really have */
    /* Oh what do you know - NumFunctionKeys is accurate */

    for (i = 0; i < keys->NumFunctionKeys; i++) {
	if (new->NumKeys == keys->FunctionKeys[i].NumKeys) {
	    match = 1;
	    for (j = 0; j < new->NumKeys; j++) {
		if (new->Keys[j] != keys->FunctionKeys[i].Keys[j]) {
		    match = 0;
		    j = new->NumKeys;
		} 
	    }
	    if (match) {
		return(keys->FunctionKeys[i].Name);
	    }
	}
    }

    /* check against dialog keys defined so far */
    /* it would be good to know how many are defined for real */
    /* Cool - NumOurKeys is accurate */

    for (i = 0; i < keys->NumOurKeys; i++) {
	if (new->NumKeys == keys->OurKeys[i].NumKeys) {
	    match = 1;
	    for (j = 0; j < new->NumKeys; j++) {
		if (new->Keys[j] != keys->OurKeys[i].Keys[j]) {
		    match = 0;
		    j = new->NumKeys;
		} 
	    }
	    if (match) {
		return(keys->OurKeys[i].Name);
	    }
	}
    }

    /* check against user defined keys */
    /* we should just be able to step down the linked list, right? */
    /* As long as we make sure that we allocate an element for the */
    /* key after deciding the user isn't going to hit space */
    
    for (walker = keys->TheirKeys; walker; walker = walker->next) {
	if (new->NumKeys == walker->entry.NumKeys) {
	    match = 1;
	    for (j = 0; j < new->NumKeys; j++) {
		if (new->Keys[j] != walker->entry.Keys[j]) {
		    match = 0;
		    j = new->NumKeys;
		} 
	    }
	    if (match) {
		return(walker->entry.Name);
	    }
	}
    }


    return (char *)NULL;
}


void
getOtherKeys(keys)
    KeyHolderType *keys;
{
    char dialog[80];
    char *dupname;
    KeyEntryLLType* walker;
    KeyEntryType tempkey;
    int duplicate, j, done;

    done = 0;
    walker = NULL;
    keys->TheirKeys = NULL;
    
    fprintf(stdout, "\nThis section only applies if you wish to use bindkey to bind\nspecial keys on your keyboard to actions in Z-Mail Lite.\nDo you want to define any other keys that are particular\nto your keyboard? [no] ");
    fgets(dialog, 80, stdin);
    if ((*dialog) && ((*dialog == 'y') || (*dialog == 'Y'))) {
	fprintf(stdout, "Hit return by itself if you have no more keys to define.\n");

	/* Loop here until they don't have any more keys to define */

	while (!done) {
	    fprintf(stdout, "What is the name of the key you want to define? ");
	    fgets(dialog, 80, stdin);
	    if ((*dialog) && (*dialog != '\r') && (*dialog != '\n')) {
		dialog[strlen(dialog) -1] = '\0';
		strcpy(tempkey.Name, dialog);
		duplicate = 1;
		while (duplicate) {
		    fprintf(stdout, "Press %s now:  ", tempkey.Name);
		    fflush(stdout);
		    readkey(&tempkey);
		    fprintf(stdout, "OK.\n");
		    if ((tempkey.NumKeys != 1) || (tempkey.Keys[0] != ' ')) {
			/* Didn't hit space */
			if (!(dupname = duplicateKey(&tempkey, keys))) {
			    /* Allocate another struct for this key */
			    if (!walker) {
				/* First time through the loop */
				keys->TheirKeys = (KeyEntryLLType *) malloc(sizeof(KeyEntryType));
				walker = keys->TheirKeys;
				walker->next = NULL;
			    } else {
				/* Every other time */
				walker->next = (KeyEntryLLType *) malloc(sizeof(KeyEntryType));
				walker = walker->next;
				walker->next = NULL;
			    }
			    if (walker == NULL) {
				fprintf(stderr, 
					"Not enough memory for your additional key.\n");
				exit(-1);
			    }
#ifdef DEBUG
			    fprintf(stderr, "Got this far.\n");
#endif
			    strcpy(walker->entry.Name, tempkey.Name);
			    /* copy temp key into real spot */
			    walker->entry.NumKeys = tempkey.NumKeys;
			    for (j = 0; j < tempkey.NumKeys; j++)
				walker->entry.Keys[j] = tempkey.Keys[j];
			    duplicate = 0;
			} else {
			    /* Duplicate of another key */
			    fprintf(stdout, "Warning, that key generated a keysequence that was a\nduplicate of the '%s' key.  It has been ignored.\nYou may attempt to have a different key represent\n\"%s\", or press SPACEBAR to go on to next question.\n", dupname, tempkey.Name);
			}
		    } else {
			/* Hit space - aborted this key */
			/* We need some way of marking this block as bad */
			/* and removing it from the linked list */
			fprintf(stdout, "Note: No %s key recorded.\n", tempkey.Name);
			duplicate = 0;
		    }
		}
	    } else {
		done = 1;
	    }
	}
    }
}

void 
getFunctionKeys(keys)
    KeyHolderType *keys;
{
    char dialog[80];
    char *dupname;
    KeyEntryType tempkey;
    int i, j, requested, duplicate;

    fprintf(stdout, "Does your keyboard have function keys? [yes] ");
    fgets(dialog, 80, stdin);
    if ((!*dialog) || (*dialog == '\r') || (*dialog == '\n') 
	|| (*dialog == 'y') || (*dialog == 'Y')) {	
	fprintf(stdout, "How many function keys does your keyboard have? [12] ");
	fgets(dialog, 80, stdin);
	if ((!*dialog) || (*dialog == '\r') || (*dialog == '\n')) {
	    requested = 12;
	} else {
	    requested = atoi(dialog);
	}
    } else {
	requested = 0;
    }
    
    fprintf(stdout, "%d function keys noted.\n", requested);

    if (requested > 0) {
	keys->FunctionKeys = (KeyEntryType *) malloc(requested * sizeof(KeyEntryType));
	if (keys->FunctionKeys == NULL) {
	    fprintf(stderr, "Not enough memory for all your function keys.\n");
	    exit(-1);
	}
    }

    fprintf(stdout, "When prompted, please press each key, one at a time.\nDo not type ahead as it may be read as a single key.\nAlso, if your terminal has keyrepeat, you may wish to disable it.\nThe keystrokes will be recorded and a multikey entry will be\ngenerated for each key.\n");

    fflush(stdout);
    keys->NumFunctionKeys = 0;
    for (i = 0; i < requested; i++) {
	duplicate = 1;
	while (duplicate) {
	    fprintf(stdout, "Press F%d now:  ", i+1);
	    fflush(stdout);
	    readkey(&tempkey);
	    fprintf(stdout, "OK.\n");
	    if ((tempkey.NumKeys != 1) || (tempkey.Keys[0] != ' ')) {
		/* Didn't hit space */
		if (!(dupname = duplicateKey(&tempkey, keys))) {
		    /* copy temp key into real spot */
		    keys->FunctionKeys[keys->NumFunctionKeys].NumKeys = tempkey.NumKeys;
		    for (j = 0; j < tempkey.NumKeys; j++)
			keys->FunctionKeys[keys->NumFunctionKeys].Keys[j] = 
			    tempkey.Keys[j];
		    sprintf(keys->FunctionKeys[keys->NumFunctionKeys].Name, 
			    "f%d", i+1);
		    keys->NumFunctionKeys++;
		    duplicate = 0;
		} else {
		    fprintf(stdout, "Warning, that key generated a keysequence that was a\nduplicate of the '%s' key.  It has been ignored.\nYou may attempt to have a different key represent\n\"F%d\", or press SPACEBAR to go on to next question.\n", dupname, i+1);
		    /* Duplicate of another key */
		}
	    } else {
		fprintf(stdout, "Note: no F%d key recorded.\n", i+1);
		duplicate = 0;
	    }
	}
    }
    fprintf(stdout, "\n");

}

void
getDialogKeys(keys)
    KeyHolderType *keys;
{
    KeyEntryType tempkey;
    char dialog[80];
    char *dupname;
    int i, j, same, duplicate;
    KeyEntryType tab, backtab;

    /* NUMDIALOGKEYS -1 because tab and backtab are special cased */
    /* {"name multikey uses",  */
    /* "name person will see on keyboard", */
    /* "description of key"} */
    /* As the deadline gets closer, this is betting messier and messier. */
    static char *dialogkeys[NUMDIALOGKEYS -1][3] = 
	{{"up", "up arrow", "Up or that shows an arrow pointing up"}, 
	     {"down", "down arrow", "Down or that shows an arrow pointing down"},
	     {"right", "right arrow", "Right or that shows an arrow pointing right"},
	     {"left", "left arrow", "Left or that shows an arrow pointing left"},
	     {"home", "Home", "Home"},
	     {"end", "End", "End"},
	     {"pageup", "Page Up", "Page Up"},
	     {"pagedown", "Page Down", "Page Down"},
	     {"delete", "Delete", "Delete"},
	     {"insert", "Insert", "Insert"}
     };

    keys->NumOurKeys = 0;
    for (i = 0; i < (NUMDIALOGKEYS -1); i++) {
	duplicate = 1;
	while (duplicate) {
	    fprintf(stdout, "Press %s now:  ", dialogkeys[i][1]);
	    fflush(stdout);
	    readkey(&tempkey);
	    fprintf(stdout, "OK.\n");
	    fflush(stdout);
	    if ((tempkey.NumKeys != 1) || (tempkey.Keys[0] != ' ')) {
		/* Didn't hit space */
		if (!(dupname = duplicateKey(&tempkey, keys))) {
		    /* copy temp key into real spot */
		    keys->OurKeys[keys->NumOurKeys].NumKeys = tempkey.NumKeys;
		    strcpy(keys->OurKeys[keys->NumOurKeys].Name, 
			   dialogkeys[i][0]);
		    for (j = 0; j < tempkey.NumKeys; j++)
			keys->OurKeys[keys->NumOurKeys].Keys[j] 
			    = tempkey.Keys[j];
		    duplicate = 0;
		    keys->NumOurKeys++;
		} else {
		    /* Duplicate of another key */
		    fprintf(stdout, "Warning, that key generated a keysequence that was a\nduplicate of the '%s' key.  It has been ignored.\nYou may attempt to have a different key represent\n\"%s\", or press SPACEBAR to go on to next question.\n", dupname, dialogkeys[i][0]); 
		}
	    } else {
		fprintf(stdout, "Note: no %s key recorded.\n", dialogkeys[i][1]);
		duplicate = 0;
	    }	
	}
    }

    /* special case for tab/backtab */
    /* I hate special cases */
    /* I wish I had known what was really to be expected from this program */
    /* when I started writing it.  Oh well, at least it hasn't been that */
    /* hard to extend, though it's looking like it could use an overhaul. */
    fprintf(stdout, "Press Tab now:  ");
    fflush(stdout);
    readkey(&tab);
    fprintf(stdout, "OK.\n");
    /* If they type space, we shouldn't worry about backtab */
    if ((tab.NumKeys == 1) && (tab.Keys[0] == ' ')) {
	fprintf(stdout, "Note: no tab key recorded.\n");
	return;
    }
	
    duplicate = 1;
    while (duplicate) {
 	fprintf(stdout, "Press Shift-Tab now:  ");
	fflush(stdout);
	readkey(&backtab);
	fprintf(stdout, "OK.\n");
	fflush(stdout);	
	/* Compare it against everything first, then against tab.*/
	if ((backtab.NumKeys != 1) || (backtab.Keys[0] != ' ')) {
	    /* Didn't hit space */
	    same = 1;
	    if (!(dupname = duplicateKey(&backtab, keys))) {
		/* Not a duplicate of the other keys */
		/* Compare tab and backtab */
		if (tab.NumKeys == backtab.NumKeys) {
		    for (i = 0; i < tab.NumKeys; i++)
			if (tab.Keys[i] != backtab.Keys[i]) {
			    same = 0;
			    i = tab.NumKeys;
			}
		} else {
		    /* different number of keys */
		    same = 0; 
		}
		duplicate = !same;
		if (same) {
		    /* This is only where backtab = tab */
		    fprintf(stdout, "Warning, that key generated a keysequence that was a\nduplicate of the 'tab' key.  It has been ignored.\nYou may attempt to have a different key represent\n\"Backtab\", or press SPACEBAR to go on to next question.\n");
		    duplicate = 1;
		} else {
		    /* This is where backtab is its own key, for real */
		    /* fprintf(stdout, "Tab and Shift+Tab generate different output from your keyboard.\n"); */
		    /* fprintf(stdout, "You may use Shift+Tab as a backtab in Z-Mail Lite.\n"); */
		    for (i = 0; i < backtab.NumKeys; i++) {
			keys->OurKeys[keys->NumOurKeys].Keys[i] = backtab.Keys[i];
		    }
		    keys->OurKeys[keys->NumOurKeys].NumKeys = backtab.NumKeys;
		    strcpy(keys->OurKeys[keys->NumOurKeys].Name, "backtab");
		    keys->NumOurKeys++;
		    duplicate = 0;
		}
	    } else {
		/* was a duplicate of one of the pre read keys */
		fprintf(stdout, "Warning, that key generated a keysequence that was a\nduplicate of the '%s' key.  It has been ignored.\nYou may attempt to have a different key represent\n\"Backtab\", or press SPACEBAR to go on to next question.\n", dupname);
	    }
	} else {
	    fprintf(stdout, "Note: no backtab key recorded.\n");
	    duplicate = 0;
	    /* it was a space */
	}
    }
}

FILE *
tryHomeDir(term, where)
    char *term;
    char *where;
{
    FILE *dest;
    struct stat stbuf;
    char *home;
    char dialog[MAXPATHLEN];

    /* This is only it's own procedure because it's called in two */
    /* places that I can't bring any closer logically.  */

    /* OK, it's not going into the system area, so how about */
    /* $HOME/.multikey/$TERM */
    home = getenv("HOME");
    if (home) {
	sprintf(where, "%s/.multikey", home);
    } else {
	/* No home directory?  What will they think of next? */
	where[0] = (char)NULL;
    }

    fprintf(stdout, 
	    "Where should the multikey information for %s go?\n",
	    term);
    if (where[0])
	fprintf(stdout, "[%s/%s] ", where, term);
    fgets(dialog, 80, stdin);
    if ((!*dialog) || (*dialog == '\r') || (*dialog == '\n')) {
	if (where[0]) {
	    /* Using the value we gave as a default */
	    /* look for the directory - if it doesn't exist, create it */
	    if (stat(where, &stbuf) == -1) {
		if (mkdir(where, (mode_t)00755) != 0) {
		    fprintf(stderr, 
			    "Cannot create directory %s.\n", 
			    where);
		    exit(1);
		}
	    }
	    sprintf(dialog, "%s/%s", where, term);
	    if (stat(dialog, &stbuf) != -1) {
		fprintf(stdout, "Overwrite existing file [yes] ");
		fgets(where, 80, stdin);
		if ((!*where) || (*where == '\r') || (*where == '\n') 
		    || (*where == 'y') || (*where == 'Y')) {
		    dest = fopen(dialog, "w");
		} else {
		    fprintf(stdout, "Print to standard output [yes] ");
		    fgets(where, 80, stdin);
		    if ((!*where) || (*where == '\r') || (*where == '\n') 
			|| (*where == 'y') || (*where == 'Y')) {
			dest = stdout;
		    } else {
			exit(1);
		    }
		} 
	    } else {
		dest = fopen(dialog, "w");
	    }
	    if (!dest) {
		fprintf(stderr,
			"Cannot open %s: writing to standard out.\n", 
			dialog);
		dest = stdout;
		
	    } 
	} else {
	    /* No filename - bleh.  I hate error conditions. */
	    fprintf(stderr, 
		    "No filename, writing to standard out.\n");
	    dest = stdout;
	}	
    } else {
	/* Not using the default, using the typed in value */
	dialog[strlen(dialog) -1] = '\0'; /* Zorch \n from fgets. */
	dest = fopen(dialog, "w");
	if (!dest) {
	    fprintf(stderr,
		    "Cannot open %s: writing to standard out.\n", 
		    dialog);
	    dest = stdout;
	} 
    }

    strcpy(where, dialog);
    return(dest);
}


void
dumpKeybindings(keys, term)
    KeyHolderType *keys;
    char *term;
{
    struct stat stbuf;
    char dialog[MAXPATHLEN];
    char where[MAXPATHLEN];
    char *zmlib;
    FILE *dest;
    KeyEntryLLType *walker;
    int remember;
    int i;

    
    /* I should flowchart this.  After I have some more sake. */
    /* No, really, I bet the first bug reported will be right */
    /* here. */
    /* (a day later) In fact, the first pr was filed right here */


    dialog[0] = (char)NULL;
    fprintf(stdout, "Do you want to save your changes? [yes] ");
    fgets(dialog, 80, stdin);
    if ((*dialog) && (*dialog != '\r') && (*dialog != '\n') 
	&& (*dialog != 'y') && (*dialog != 'Y')) {
	/* OK, fine.  I'll try not to be hurt.  Just be that way. */
	exit(1);
    }

    /* First, see if we have write permission on the ZMLIB directory */

    remember = 0;
    dialog[0] = (char)NULL;
    zmlib = getenv("ZMLIB");
    if (!zmlib) {
	sprintf(where, "/usr/lib/Zmlite/multikey/%s", term);
    } else {
	sprintf(where, "%s/multikey/%s", zmlib, term);
    }

    if (stat(where, &stbuf) == -1) {
	/* file doesn't already exist , remember to unlink it. */
	remember = 1;
    }
    
    dest = fopen(where, "a");

    if (dest) {
	/* We opened the system file ok */
	fprintf(stdout, 
		"Install the multikey entry for %s in the system directory\n%s? [yes] ", 
		term, 
		where);
	fgets(dialog, 80, stdin);  /* Leaves that annoying return on the end */
	if ((*dialog) && (*dialog != '\r') && (*dialog != '\n') 
	    && (*dialog != 'y') && (*dialog != 'Y')) {
	    /* They don't want to use the system file */
	    if (remember) {
		fclose(dest);
		remove(where);
	    }
	    dest = tryHomeDir(term, where);
	} else {
	    /* They do want to use the system file */
	    fclose(dest);
	    dest = fopen(where, "w+");
	}
    } else {
	/* We couldn't open the system file */
	dest = tryHomeDir(term, where);
    }

    fprintf(dest, "# Terminal-specific \"multikey\" definitions\n");
    fprintf(dest, "# This file generated automatically by ZTermKey\n# ");
    fprintf(dest, ZTERMKEYVERSION);

    for (i = 0; i < keys->NumFunctionKeys; i++) {
	printkey(&keys->FunctionKeys[i], dest);
    }
    for (i = 0; i < keys->NumOurKeys; i++) {
	printkey(&keys->OurKeys[i], dest);
    }
    for (walker = keys->TheirKeys; walker; walker = walker->next) {
	printkey(&walker->entry, dest);
    }
    if (dest != stdout)
	fprintf(stdout, 
		"\nKeybindings put into %s\n", 
		where);
}


void
ttySetup(t)
    TTY_t *t;
{
    char *tistr;
    /* I stole all this from Bob in spoor/cursim.c or something like that. */
    /* I have no pride. */

#if defined(TERM_USE_TERMIO) || defined(TERM_USE_TERMIOS)

# if defined(VDISCARD) && (VDISCARD < VMAX)
    t->c_cc[VDISCARD] = 0377;
# endif

# if defined(VDSUSP) && (VDSUSP < VMAX)
    t->c_cc[VDSUSP] = 0377;
# endif

# if defined(VEOL2) && (VEOL2 < VMAX)
    t->c_cc[VEOL2]  =  0377;
# endif

# if defined(VERASE) && (VERASE < VMAX)
    t->c_cc[VERASE] =  0377;
# endif

# if defined(VFLUSHO) && (VFLUSHO < VMAX)
    t->c_cc[VFLUSHO] = 0377;
# endif

# if defined(VKILL) && (VKILL < VMAX)
    t->c_cc[VKILL]  =  0377;
# endif

# if defined(VLNEXT) && (VLNEXT < VMAX)
    t->c_cc[VLNEXT] = 0377;
# endif

# if defined(VMIN) && (VMIN < VMAX)
    t->c_cc[VMIN] = 1;
# endif

# if defined(VQUIT) && (VQUIT < VMAX)
    t->c_cc[VQUIT]  =  0377;
# endif

# if defined(VREPRINT) && (VREPRINT < VMAX)
    t->c_cc[VREPRINT] = 0377;
# endif

# if defined(VRPRNT) && (VRPRNT < VMAX)
    t->c_cc[VRPRNT] =  0377;
# endif

# if 0
#  if defined(VSTART) && (VSTART < VMAX)
    t->c_cc[VSTART] =  0377;
#  endif

#  if defined(VSTOP) && (VSTOP < VMAX)
    t->c_cc[VSTOP] =   0377;
#  endif
# endif

# if defined(VSUSP) && (VSUSP < VMAX)
    t->c_cc[VSUSP] = 0377;
# endif

# if defined(VSWTCH) && (VSWTCH < VMAX)
    t->c_cc[VSWTCH] =  0377;
# endif

# if defined(VTIME) && (VTIME < VMAX)
    t->c_cc[VTIME] = 0;
# endif

# if defined(VWERASE) && (VWERASE < VMAX)
    t->c_cc[VWERASE] = 0377;
# endif

# if defined(VWERSE) && (VWERSE < VMAX)
    t->c_cc[VWERSE] = 0377;
# endif

# if defined(VEOF) && (VEOF < VMAX) && !defined( __DGUX__ ) 
    t->c_cc[VEOF]   =  0;
# endif

# if defined(VEOL) && (VEOL < VMAX)
    t->c_cc[VEOL]   =  0;
# endif

    t->c_lflag &= ~(ISIG);
    t->c_lflag &= ~(ICANON);
    t->c_lflag &= ~(ECHO);

    Stty(0, t);

#endif /* TERM_USE_TERMIO || TERM_USE_TERMIOS */
    tistr = tigetstr("smkx");	/* keypad_xmit */
    if (tistr && (tistr != (char *) -1) && *tistr)
	putp(tistr);
    tistr = tigetstr("enacs"); /* enable alternate character set */
    if (tistr && (tistr != (char *) -1) && *tistr)
	putp(tistr);
}

void
ttyRevert(t)
    TTY_t *t;
{
    char *tistr;

    Stty(0, t);
/*    if (tistr && (tistr != (char *) -1) && *tistr)
	putp(tistr); */
    tistr = tigetstr("rmkx");
    if (tistr && (tistr != (char *) -1) && *tistr)
	putp(tistr);
}

void
readkey(key)
    KeyEntryType *key;
{
    int done, nfound, numread;
    TTY_t t, origt;
    struct timeval timeout;
    fd_set rbits;

    Gtty(0, &origt);
    Gtty(0, &t);
    ttySetup(&t);

    key->NumKeys = 0;
    FD_ZERO(&rbits);
    FD_SET(0, &rbits);

    /* wait to read the first character */
    numread = 0;
    nfound = select(1, &rbits, (fd_set *)NULL, (fd_set *)NULL,
		    (struct timeval *)NULL);
    if (nfound > 0) {
	if (FD_ISSET(0, &rbits)) {
	    numread = read(0, &key->Keys[key->NumKeys], 1);
	    key->NumKeys += numread;
#ifdef DEBUG
	    fprintf(stderr, "The first read returned %d!\n", numread);
#endif
	}		
    } else {
#ifdef DEBUG
	fprintf(stderr, "The first select returned %d!\n", nfound);
#endif
    }
    if (numread <= 0) { 
	/* when interrupted or when we get an error */
	if (errno == EINTR) {
	    errno = 0;
	    ttyRevert(&origt);
	    fprintf(stderr, "EINTR while reading\n");
	    exit(-1);
	} else {
	    /*  some other weirdness  */
	    ttyRevert(&origt);
	    fprintf(stderr,"error: read returned %d\n", numread);
	    exit(-1);
	}    
    } else {
	/* Got a character - loop on the next characters */
	/* or time out after a half second or so. */
	/* key->NumKeys += numread; */
#ifdef DEBUG
	fprintf(stderr, "So far, we've got %d characters.\n", numread);
#endif
	done = 0;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;
	while (!done) {
	    nfound = select(1, &rbits, (fd_set *)NULL, (fd_set *)NULL,
			    &timeout);
	    if (nfound > 0) {
		if (FD_ISSET(0, &rbits)) {
		    read(0, &key->Keys[key->NumKeys], 1);
#ifdef DEBUG
		    fprintf(stderr, "select returned %d!\n", nfound);
#endif
		    key->NumKeys++;	
		}		
	    } else {
#ifdef DEBUG
		fprintf(stderr, "select returned %d!\n", nfound);
#endif
		done = 1;
	    }
	}
    }	
    ttyRevert(&origt);
}

void 
printkey(key, where)
    KeyEntryType *key;
    FILE *where;
{
    int i;
    static char *ascii[128] = 
	{ "nul","ctrl+a","ctrl+b","ctrl+c",
	      "ctrl+d","ctrl+e","ctrl+f","ctrl+g",
	      "ctrl+h","tab","newline","ctrl+k",
	      "ctrl+l","return","ctrl+n","ctrl+o",
	      "ctrl+p","ctrl+q","ctrl+r","ctrl+s",
	      "ctrl+t","ctrl+u","ctrl+v","ctrl+w",
	      "ctrl+x","ctrl+y","ctrl+z","esc",
	      "ctrl+\\","ctrl+]","ctrl+^","ctrl+_",
	      "space","!","\"","#","$","%","&","\'",
	      "(",")","*","+",",","-",".","/",
	      "0","1","2","3","4","5","6","7",
	      "8","9",":",";","<","=",">","?",
	      "@","A","B","C","D","E","F","G",
	      "H","I","J","K","L","M","N","O",
	      "P","Q","R","S","T","U","V","W",
	      "X","Y","Z","[","\\","]","^","_",
	      "`","a","b","c","d","e","f","g",
	      "h","i","j","k","l","m","n","o",
	      "p","q","r","s","t","u","v","w",
	      "x","y","z","{","|","}","~","del"};

/*    fprintf(stderr, "Got %d keystrokes.\n", key->NumKeys); */
    fprintf(where, "multikey  ");
    for (i = 0; i < key->NumKeys ; i++) {
	if ((/* (key->Keys[i] >= 0) && */ (key->Keys[i] <= 32)) 
	    || (key->Keys[i] == 127)) {
	    fprintf(where, "\\<%s>", ascii[key->Keys[i]]);
	} else {
	    if ((key->Keys[i] >= 33) && (key->Keys[i] <= 126)) {
		fprintf(where, "%s", ascii[key->Keys[i]]);
	    } else {
		fprintf(where, "\\%3o", key->Keys[i]);
	    }
	} 
    }
    fprintf(where, "\t\\<%s>\n", key->Name);
}


