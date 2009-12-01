/* pager.c     Copyright 1993 Z-Code Software Corp. */

#ifndef lint
static char	pager_rcsid[] =
    "$Id: pager.c,v 2.36 1996/07/09 06:29:36 schaefer Exp $";
#endif

#include "zmail.h"

#include <general.h>
#include "pager.h"
#include "catalog.h"
#include "strcase.h"
#include "child.h"

static int pgfirst_write P ((ZmPager, char *));
static void pgfirst_end P ((ZmPager));
static void pgline_init P ((ZmPager));
static int pgline_pipe_write P ((ZmPager, char *));
static int pgline_write P ((ZmPager, char *));
static void pgline_end P ((ZmPager));
static void pgline_pipe_end P ((ZmPager));

int max_text_length;

ZmPager cur_pager;

struct zmPagerDevice pager_devices[(int)PgTotalTypes] = {
    { PgHelp,      pgline_init,      pgline_write,      pgline_end, },
    { PgHelpIndex, pgline_init,      pgline_write,  	pgline_end, },
    { PgText,  	   pgline_init,      pgline_write,  	pgline_end, },
    { PgMessage,   pgline_init,      pgline_write,  	pgline_end, },
    { PgNormal,    pgline_init,      pgline_write,  	pgline_end, },
    { PgInternal,  pgline_init,      pgline_write,  	pgline_end, PGD_IGNORE_VARPAGER },
    { PgPrint,	   pgline_init,      pgline_write,  	pgline_end, },
    { PgOutput,    pgline_init,      pgline_write,      pgline_end  }
};

#ifdef GUI
extern struct zmPagerDevice gui_pager_devices[(int)PgTotalTypes];
#endif /* GUI */

ZmPager
ZmPagerStart(type)
ZmPagerType type;
{
    ZmPager pager;

    pager = zmNew(zmPager);
    cur_pager = pager;
    ZmPagerSetType(pager, type);
    return pager;
}

void
ZmPagerSetType(pager, type)
ZmPager pager;
ZmPagerType type;
{
    pager->device = &pager_devices[(int)type];
    if (pager->device->type != type)
	error(ZmErrFatal, catgets(catalog, CAT_SHELL, 866, "pager device type mismatch (%d)"), type);
    pager->write_func = pgfirst_write;
    pager->end_func   = pgfirst_end;
}

void
ZmPagerStop(pager)
ZmPager pager;
{
    pager->end_func(pager);
    xfree(pager->title);
    xfree(pager->file);
    xfree(pager->prog);
    xfree(pager->printer);
    xfree(pager);
    if (pager == cur_pager) cur_pager = (ZmPager) 0;
}

void
ZmPagerWrite(pager, str)
ZmPager pager;
char *str;
{
    int ret;

    if (ison(pager->flags, PG_DONE|PG_ERROR)) return;
    ret = (pager->write_func)(pager, str);
    if (ret == EOF) turnon(pager->flags, PG_DONE);
}

void
ZmPagerInitialize(pager)
ZmPager pager;
{
    int used_gui = False;
    
#ifdef GUI
    if (istool > 1 && isoff(pager->flags, PG_NO_GUI)) {
	ZmPagerType type = pager->device->type;

	pager->device = &gui_pager_devices[(int)type];	
	if (pager->device->type != type)
	    error(ZmErrFatal,
		catgets(catalog, CAT_SHELL, 866, "pager device type mismatch (%d)"), type);
	used_gui = True;
    }
#endif /* GUI */
    pager->write_func = pager->device->write_func;
    pager->end_func = pager->device->end_func;
    pager->device->init_func(pager);
    /* if the GUI init func decided not to do anything, call the regular
     * one.
     */
    if (used_gui && ison(pager->flags, PG_NO_GUI)) {
	pager->device = &pager_devices[(int)pager->device->type];	
	ZmPagerInitialize(pager);
    }
}

static int 
pgfirst_write(pager, str)
ZmPager pager;
char *str;
{
    ZmPagerInitialize(pager);
    if (ZmPagerCheckCancel(pager))
        return EOF;
    else
	return pager->write_func(pager, str);
}

static void
pgfirst_end(pager)
ZmPager pager;
{
    ZmPagerInitialize(pager);
    pager->end_func(pager);
}

#if defined(UNIX) && defined(GUI)
/*
 * In UNIX GUIs, capture the output and error of pagers that we pipe
 * to, and display it after the pager finishes.  This has to use a
 * pair of tempfiles because writing/reading to/from the same process
 * is a potential deadlock.
 */
static void
pggui_pipe_init(pager, prog)
ZmPager pager;
const char *prog;
{
    FILE *out = 0, *err = 0;
    char *outname = 0, *errname = 0;
    int save_errno;

    if ((out = open_tempfile("out", &outname)) &&
	    (err = open_tempfile("err", &errname))) {
	if (pager->pid = popensh(&pager->fp, &out, &err, prog)) {
	    pager->outfile = outname;
	    pager->errfile = errname;
	    pager->write_func = pgline_pipe_write;
	    pager->end_func = pgline_pipe_end;
	    fclose(out);
	    fclose(err);
	    return;
	}
    }
    save_errno = errno;
    if (out) {
	fclose(out);
	(void) unlink(outname);
	xfree(outname);
    }
    if (err) {
	fclose(err);
	(void) unlink(errname);
	xfree(errname);
    }
    errno = save_errno;
}
#else /* !(UNIX && GUI) */
#define pggui_pipe_init(pager, prog)	pgline_pipe_init(pager, prog)
#endif /* !(UNIX && GUI) */

static void
pgline_pipe_init(pager, prog)
ZmPager pager;
const char *prog;
{
#ifdef UNIX
    if (ison(pager->flags, PG_EDITABLE)) {
	pager->pid = popensh((FILE **)0, (FILE **)0, (FILE **)0, prog);
	pager->write_func = 0;
	pager->fp = stdout;
    } else {
	pager->pid = popensh(&pager->fp, (FILE **)0, (FILE **)0, prog);
	pager->write_func = pgline_pipe_write;
    }
    pager->end_func = pgline_pipe_end;
#else /* !UNIX */
    pager->fp = popen(prog, "w");
#endif /* !UNIX */
}

static void
pgline_init(pager)
ZmPager pager;
{
    char *prog = NULL;

    turnon(glob_flags, IGN_SIGS);
#ifndef MAC_OS
    if (isoff(pager->flags, PG_NO_PAGING))
	prog = pager->prog;
    if (isoff(pager->device->flags, PGD_IGNORE_VARPAGER) &&
	  isoff(pager->flags, PG_NO_PAGING)) {
	if (!prog && !(prog = value_of(VarPager)))
	    prog = DEF_PAGER;
	if (!strcmp(prog, "NONE"))
	    turnon(pager->flags, PG_NO_PAGING);
	if (!strcmp(prog, "internal") || !strcmp(prog, "NONE"))
	    prog = NULL; /* internal pager */
    }
    if (prog) {
	pager->fp = 0;
	echo_on();			/* XXX Why is this here? */
	if (istool == 2)
	    pggui_pipe_init(pager, prog);
	else
	    pgline_pipe_init(pager, prog);
	if (pager->fp)
	    return;			/* XXX Maybe this is why */
	ZmPagerSetError(pager);
	error(SysErrWarning, prog);
	echo_off();
    }
#endif /* !MAC_OS */
    pager->fp = stdout;
    if (ison(glob_flags, ECHO_FLAG)) {
	turnon(pager->flags, PG_RESTORE_ECHO);
	turnoff(glob_flags, ECHO_FLAG);
	echo_off();
    }
#ifdef apollo
    if (apollo_ispad()) turnon(pager->flags, PG_NO_PAGING);
#endif /* apollo */
    if (ison(pager->flags, PG_NO_PAGING))
	pager->write_func = pgline_pipe_write;
}

static int
pgline_pipe_write(pager, str)
ZmPager pager;
char *str;
{
    return !check_intr() ? fputs(str, pager->fp) : EOF;
}

static int
pgline_write(pager, buf)
ZmPager pager;
char *buf;
{
    register char c = 0, *cr = index(buf, '\n');
    
    if (cr && (c = *++cr) != '\0')
	*cr = 0; /* send one line to stdout and prompt for more */
    pager->line_chct += strlen(buf);
    if (cr) {
	int maxlen =
#ifdef CURSES
	    iscurses ? COLS :
#endif /* CURSES */
		80;
	if (pager->line_chct > maxlen)
	    pager->line_ct += pager->line_chct / maxlen;
	pager->line_chct = 0;
    }
    (void) fputs(buf, stdout);
    if (cr && (++pager->line_ct / (crt-1))) {
	int n = c_more(NULL);
	if (n == '\n' || n == '\r')
	    pager->line_ct--; /* go line by line */
	else if (n == Ctrl('D') || lower(n) == 'd')
	    pager->line_ct = ((crt-1)/2);
	else if (n < 0 || lower(n) == 'q') {
	    /* could check if "c" is set, but... see warning above */
	    clearerr(stdin); /* for n < 0 */
	    return EOF;
	} else
	    pager->line_ct = 1;
    }
    if (c) {
	*cr = c;
	return pgline_write(pager, cr);
    }
    return 0;
}

static void
pgline_end(pager)
ZmPager pager;
{
    if (ison(pager->flags, PG_RESTORE_ECHO)) {
	echo_on();
	turnon(glob_flags, ECHO_FLAG);
    } else
	echo_off();
    turnoff(glob_flags, IGN_SIGS);
}

static void
pgline_pipe_end(pager)
ZmPager pager;
{
    int status;
#ifdef UNIX
    FILE *fp;
#endif /* UNIX */

#if !defined(ZM_CHILD_MANAGER) && !defined(MAC_OS)
#ifdef _WINDOWS                                        
    RETSIGTYPE (*oldchld)(int) = signal(SIGCHLD, SIG_DFL);
#else    
    RETSIGTYPE (*oldchld)() = signal(SIGCHLD, SIG_DFL);
#endif /* _WINDOWS */    
#endif /* !ZM_CHILD_MANAGER && !MAC_OS */

#ifdef UNIX
    if (pager->fp != stdout)
	fclose(pager->fp);
    status = pclosev(pager->pid);
# ifdef GUI
    if (pager->outfile) {
	if (fp = fopen(pager->outfile, "r")) {
	    ZmPager outpager;

	    outpager = ZmPagerStart(pager->device->type == PgPrint ? PgOutput : PgNormal);
	    fioxlate(fp, -1, -1, 0, fiopager, outpager);
	    ZmPagerStop(outpager);
	    fclose(fp);
	}
	(void) unlink(pager->outfile);
	xfree(pager->outfile);
    }
    if (pager->errfile) {
	struct stat statbuf;

	if ((status
	     || (!stat(pager->errfile, &statbuf)
		 && statbuf.st_size > 0))
	    && (fp = fopen(pager->errfile, "r"))) {
	    ZmPager errpager;

	    errpager = ZmPagerStart(status ? PgText : PgNormal);
	    if (status) {
		ZmPagerSetTitle(errpager, "Error");
		ZmPagerWrite(errpager,
			     zmVaStr("Command exited with status %d.\n\n",
				     status));
	    }
	    fioxlate(fp, -1, -1, 0, fiopager, errpager);
	    ZmPagerStop(errpager);
	    fclose(fp);
	}
	(void) unlink(pager->errfile);
	xfree(pager->errfile);
    }
# endif /* GUI */
#else /* !UNIX */
    status = pclose(pager->fp);
#endif /* !UNIX */
#ifndef MAC_OS
    if (WEXITSTATUS(*((WAITSTATUS *) &status)) != 0)
	ZmPagerSetError(pager);
#if !defined(ZM_CHILD_MANAGER) && !defined(WIN16)
    (void) signal(SIGCHLD, oldchld);
#endif /* !ZM_CHILD_MANAGER && !WIN16 */
#endif /* !MAC_OS */
    pgline_end(pager);
}

/* curses based "more" like option */
int
c_more(p)
register const char *p;
{
    register int c;

    if (ison(glob_flags, NO_INTERACT))
	return ' '; /* pretend user hit spacebar */

    if (!p)
	p = catgets( catalog, CAT_SHELL, 523, "--more--" );
    print_more(p);

    do {
	while ((c = getchar()) >= 0 && c != Ctrl('D') && !isspace(c) &&
		c != '\n' && c != '\r' && lower(c) != 'q' && lower(c) != 'd')
	    bell();
    } while (c == EOF && !feof(stdin));
    if (ison(glob_flags, ECHO_FLAG) && c != '\n' && c != '\r')
	while (getchar() != '\n');
    (void) printf("\r%*c\r", strlen(p), ' '); /* remove the prompt */
    (void) fflush(stdout);
    return c;
}

/* fioxlate()-compatible interface to zm_pager */
long
fiopager(input, len, output, state)
char *input, **output, *state;
long len;
{
    ZmPager pager = (ZmPager) state;
    
    ZmPagerWrite(pager, input);
    return (ZmPagerIsDone(pager) ? -1 : 0);
}

#ifdef GUI
int
pg_check_interrupt(p, len)
     ZmPager p;
     size_t len;
{
    /* Check interrupts every 5000 bytes ... integer division */
    if ((p->tot_len + len) / 5000 > p->tot_len / 5000) {
	char intrmsg[200];

	(void) sprintf(intrmsg, catgets(catalog, CAT_SHELL, 923, "%d bytes"), p->tot_len + len);
	return check_intr_msg(intrmsg);
    }
    return 0;
}

int
pg_check_max_text_length(p)
     ZmPager p;
{
    extern int max_text_length;

    if (isoff(p->flags, PG_ALREADY_ASKED) &&
	max_text_length > 0 &&
	p->tot_len > max_text_length) {
	char aborted[128];
	AskAnswer answer;
	
	turnon(p->flags, PG_ALREADY_ASKED);
	answer = ask(
#ifdef PAGER_SIZE_CANCEL
		     AskNo,
#else /* !PAGER_SIZE_CANCEL */
		     WarnNo,
#endif /* !PAGER_SIZE_CANCEL */
		     p->device->type == PgMessage?
		     catgets(catalog, CAT_SHELL, 924, "Message exceeds %d bytes.  Read it all?") :
		     catgets(catalog, CAT_SHELL, 925, "Text exceeds %d bytes.  Read it all?"),
		     max_text_length);
	if (answer != AskYes) {
#ifdef PAGER_SIZE_CANCEL
	    if (answer == AskCancel) turnon(p->flags, PG_CANCELLED);
#endif /* PAGER_SIZE_CANCEL */
	    return 1;
	}
    }
    return 0;
}
#endif /* GUI */
