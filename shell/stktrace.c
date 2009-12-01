/* stacktrace.c		Copyright 1991 Z-Code Software Corp. */

#include "config.h"
#include <general.h>

/*
 * Don't expect this file to compile anywhere except
 * on a sun4.  Also, it doesn't seem to work if this file is compiled with -O
 * (and it might need -g as well).
 * 
 *
 * Externally visible functions:
 * 	get_stacktrace(skip, count, addrs)
 *      symbolize_stacktrace(n, addrs, functions, files, lines,
 *							charbuf, charbufend)
 *	cleanup_symbolize_stacktrace()
 *	print_stacktrace(skip, count)		(a very cheap version).
 *	get_main_argv()		(doesn't work in zmail).
 *
 * FIX THIS-- Figure out how to get symbols from the shared libraries.
 *
 * FIX THIS-- core dumps if compiled with -O.
 *
 */
#ifdef STACKTRACE

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "stktrace.h"


static void
dump(loc, count)
int *loc;
int count;
{
    int i;
    *(int *)&loc &= ~15;
    for (i = 0; i < count; ++i) {
	if (i && loc+i == (int *)0xf8000000)
	    break;	/* don't try to read past stack bottom */
	if (i && i % 4 == 0)
	    printf("\n");
	if (i % 4 == 0)
	    printf("%x:  ", loc+i);
	printf("%08x ", loc[i]);
    }
    printf("\n");
}

char *getenv(), *strcpy(), *strcat();

#define DEF_PATH	".:/bin:/usr/bin:/usr/ucb:/etc:/usr/etc"
static char *
get_execvp_name(name, fullname)
char *name, *fullname;
{
    register char *p, *dir;
    register int found = 0;

    if (!(p = getenv("PATH")))
	p = DEF_PATH;
    if (index(name, '/')) {
	strcpy(fullname, name);
	found = !access(name, 1);
    } else
	for (dir = p; *dir; dir = p) {
	    while (*p && *p != ':')
		p++;
	    (void) sprintf(fullname, "%.*s/%s", p-dir, dir, name);
	    if (found = !access(fullname, 1)) {
		name = fullname;
		break;
	    }
	    if (*p)
		p++;
	}
    if (!found)
	return (char *)0;
	/* errno is set to whatever the last call to "access" set it to. */
    return fullname;
}

#ifdef TOSHIBA
struct frame {
    struct frame *next;
    char *addr;
};
#define firstarg_to_frame(argp) \
	((struct frame *)((char *)(argp) - sizeof(struct frame)))
#define frame_to_firstarg(frame) ((frame)+1)
#endif /* TOSHIBA */

#ifdef sun

#include <a.out.h>

#include <sparc/asm_linkage.h>
#include <sparc/frame.h>
#include <sparc/pcb.h>
#define next fr_savfp
#define addr fr_savpc

#ifdef sparc
/* #define DOWN (MAXWIN+2) */
#define DOWN 6		/* this seems to work */
#else /* !sparc */
#define ARGPUSH 8
#define DOWN 0
#endif /* !sparc */

#define firstarg_to_frame(argp) \
	((struct frame *)((char *)(argp) - ARGPUSH))
#define frame_to_firstarg(frame) \
	((char *)frame + ARGPUSH)

#endif /* sun */

/*
 * Gratuitous recursion function.
 */
static void recurse(n)
{
    if (n > 0)
	recurse(n-1);
}

static void function_after_get_stacktrace();
#define in_get_stacktrace(pc) ((char *)get_stacktrace <= (char *)(pc) \
		       && (char *)(pc) <= (char *)function_after_get_stacktrace)

extern int		/* returns number of levels obtained, at most "count" */
get_stacktrace(skip, count, addrs)
int skip;  /* where to start.
	    * 0 means start with return address from get_stacktrace. */
int count;
char *addrs[/* count */];
{
    int i;
    struct frame *frame;
    static int depth_in_stacktraces = 0;

    /*
     * On the sparc, the information dumped to the stack is from
     * a few levels up in the recursion.  So to get the
     * desired information, we recurse down to 3*DOWN, come back up to 2*DOWN,
     * and look back up to DOWN.
     */
    if (depth_in_stacktraces < 2*DOWN) {
	++depth_in_stacktraces;
	i = get_stacktrace(skip, count, addrs);
	--depth_in_stacktraces;
	return i;
    }
    recurse(DOWN);

    frame = firstarg_to_frame(&skip);
    for (i = 0; i < DOWN; ++i)
	if (!(frame = frame->next)) {
	    /* if the following happens on the sparc, make DOWN bigger. */
	    fprintf(stderr, "stacktrace: got lost.\n");
	    return 0;
	}

    if (!in_get_stacktrace(frame->addr)) {
	fprintf(stderr, "stacktrace: not in stacktrace??\n");
	return 0;
    }
	
    while (frame && in_get_stacktrace(frame->addr))
	frame = frame->next;

    for (i = -skip; i < count && frame; ++i, frame = frame->next) {
	if (i < 0)
	    continue;

	if (addrs)
	    addrs[i] = (char *)frame->addr;
    }
    return i;
}

static void function_after_get_stacktrace()
{
}

/* VARARGS */
char **
get_main_argv(dummy)
{
    struct frame *frame = firstarg_to_frame(&dummy);
    recurse(DOWN);
    while (frame->next)
	frame = frame->next;

    /*
     * The following would work, except that main can (and often does)
     * change argv.
     */
    /* return ((char ***)frame_to_firstarg(frame))[1][0]; */

    return ((char **)frame_to_firstarg(frame)) + 8;
}

static char *
addr_to_fun_file_line(filename, fp, addr, fun, file, line, charbuf, charbufend)
char *filename;	/* name of executable, for error reporting */
FILE *fp;	/* file pointer to the symbol table file */
char *addr;
char **fun, **file;
int *line;
char *charbuf, *charbufend;
{
    int c, ngot;
    struct stat statbuf;
    long hi, lo, mid;
    long size;
    char *addrbuf;
    char funbuf[1024], filebuf[1024], buf[1024];
    long linebuf;

    /*
     * Binary search the file.  The first line of the file is the
     * inode number of the executable.
     */
    if (fstat(fileno(fp), &statbuf) == -1) {
	fprintf(stderr, "cannot fstat ");
	perror(filename);
	goto error;
    }

    size = statbuf.st_size;
    if (!size)
	goto error;
	

    clearerr(fp);
    rewind(fp);
    hi = size;
    lo = 0;
    while ((c = getc(fp)) != EOF && c != '\n')
	lo++;
    lo++;


    if (lo >= hi)
	goto error;

    while (1) {
	mid = (lo + hi) / 2;
	if (fseek(fp, mid, 0) == -1) {
	    fprintf(stderr, "Cannot fseek to %ld in ", mid);
	    perror(filename);
	    goto error;
	}
	while ((c = getc(fp)) != EOF && c != '\n')
	    mid++;
	mid++;
	if (mid >= hi) {
	    mid = lo;
	    if (fseek(fp, mid, 0) == -1) {
		fprintf(stderr, "Cannot fseek to %ld in ", mid);
		perror(filename);
		goto error;
	    }
	    while ((c = getc(fp)) != EOF && c != '\n')
		mid++;
	    mid++;
	    if (mid >= hi)
		break;
	}
	if (fscanf(fp, "%lx", &addrbuf) < 1) {
	    fprintf(stderr, "Cannot read address from %s\n", filename);
	    goto error;
	}
	if (addr >= addrbuf) {
	    lo = mid;
	    continue;
	} else {
	    hi = mid;
	    continue;
	}
    }

    if (fseek(fp, lo, 0) == -1) {
	fprintf(stderr, "Cannot fseek to %ld in ", lo);
	perror(filename);
	goto error;
    }

    if (!fgets(buf, sizeof(buf), fp)) {
	if (feof(fp))
	    fprintf(stderr, "%s: premature EOF\n", filename);
	else {
	    fprintf(stderr, "Cannot fgets in");
	    perror(filename);
	}
	goto error;
    }
    funbuf[0] = 0;
    filebuf[0] = 0;
    ngot = sscanf(buf, "%lx %s %s %x", &addrbuf, funbuf, filebuf, &linebuf);

    /* printf("%lx mapped to %lx %s %s %d\n", addr,addrbuf,funbuf,filebuf,linebuf); */
    if (fun)
	if (ngot >= 2) {
	    if (strlen(funbuf)+1 > charbufend - charbuf)
		goto error;
	    *fun = charbuf;
	    strcpy(charbuf, funbuf);
	    charbuf += strlen(charbuf)+1;
	} else
	    *fun = "";
    if (file)
	if (ngot >= 3) {
	    if (strlen(filebuf)+1 > charbufend - charbuf)
		goto error;
	    *file = charbuf;
	    strcpy(charbuf, filebuf);
	    charbuf += strlen(charbuf)+1;
	} else
	    *file = "";
    if (line)
	if (ngot >= 4) {
	    *line = linebuf;
	} else
	    *line = 0;
    return charbuf;


error:
    clearerr(fp);
    if (fun)
	*fun = "";
    if (file)
	*file = "";
    if (line)
	*line = 0;
    return charbuf;
}

static char *script[] = {
    /* "nm -a $* |", */
    "egrep \" SLINE |   FUN |    SO | t | T \" |",
    "sort +.0 -.8     +.9r -.10   +.19 |",
    "awk '",
    "    /^[0-9a-f]* - 00 0000    SO .*$/ {",
    "            src = $6;",
    "            print $1, src;",
    "            next",
    "    }",
    "    /\\.o$/ {",
    "            next",
    "    }",
    "    /^[0-9a-f]* [tT] .*$/ {",
    "            fun = $3;",
    "            print $1, fun;",
    "            next",
    "    }",
    "    /^[0-9a-f]* - 00 .... SLINE $/ {",
    "            print $1, fun, src, $4",
    "    }",
    "'",
    ">>",
    0
};

char *rindex();
static FILE *fp;
cleanup_symbolize_stacktrace()
{
    if (fp)
	fclose(fp);
    fp = (FILE *)0;
}

/*
 * Value returned is the beginning of the unused area of charbuf,
 * suitable for passing to subsequent calls to symbolize_stacktrace.
 */
extern char *
symbolize_stacktrace(n, addrs, functions, files, lines, charbuf, charbufend)
int n;
char *addrs[];
char *functions[];
char *files[];
int lines[];
char *charbuf, *charbufend;
{
    int i, inode_from_file;
    char **argv;
    char exfilename[1024], symfilename[1024], *p;
    struct stat estatbuf, sstatbuf;

    if (!functions && !files && !lines)
	return 0;

    if (!fp) {
	argv = get_main_argv();
	if (!argv) {
	    fprintf(stderr, "Couldn't find argv\n");
	    return 0;
	}
	if (!argv[0]) {
	    fprintf(stderr, "No argv[0]\n");
	    return 0;
	}
	if (!get_execvp_name(argv[0], exfilename)) {
	    fprintf(stderr, "Couldn't find executable for \"%s\"\n", argv[0]);
	    return 0;
	}

	/* printf("argv[0] = %s\n", argv[0]); */
	/* printf("filename = %s\n", exfilename); */

	if (stat(exfilename, &estatbuf) == -1) {
	    fprintf(stderr, "Cannot stat executable \"%s", exfilename);
	    perror("\"");
	    return 0;
	}

	if (p = rindex(exfilename, '/'))
	    sprintf(symfilename, "/tmp/%s.syms", p+1);
	else {
	    fprintf(stderr, "symbolize_stacktrace: cannot find '/' in %s\n",
								    exfilename);
	    return 0;
	}

	/* printf("symbol filename = %s\n", symfilename); */

again:
	if (stat(symfilename, &sstatbuf) == -1
	 || estatbuf.st_mtime > sstatbuf.st_mtime) {
	    char command[2048];
	    fprintf(stderr, "Generating symbol table lookup file %s... ",
								   symfilename);
	    fflush(stderr);

	    sprintf(command, "echo %d > %s; nm -a %s |", estatbuf.st_ino,
						symfilename, exfilename);
	    for (i = 0; script[i]; ++i)
		strcat(command, script[i]);
	    strcat(command, symfilename);
	    system(command);

	    fprintf(stderr, "done.\n");
	}

	if (!(fp = fopen(symfilename, "r"))) {
	    fprintf(stderr, "Cannot read symbol table lookup file \"%s\" ",
								   symfilename);
	    perror("for reading");
	    return 0;
	}

	if (fscanf(fp, "%d\n", &inode_from_file) != 1
	 || inode_from_file != estatbuf.st_ino) {
	    /*
	     * Oops! The symbol table was from a different file
	     * with the same name.  Remove the symbol table
	     * file and try again.
	     */
	    fclose(fp);
	    if (unlink(symfilename) == -1) {
		fprintf(stderr, "Cannot unlink ");
		perror(symfilename);
	    }
	    goto again;
	}

    }

    for (i = 0; i < n; ++i) {
	charbuf = addr_to_fun_file_line(symfilename, fp, addrs[i],
					functions ? functions+i : (char **)0,
					files ? files+i : (char **)0,
					lines ? lines+i : (int *)0,
					charbuf, charbufend);
	if (!charbuf) {
	    fprintf(stderr, "symbolize_stacktrace: can only do %d levels\n", i);
	    break;
	}
    }
    for (; i < n; ++i) {
	if (functions)
	    functions[i] = "";
	if (files)
	    files[i] = "";
	if (lines)
	    lines[i] = 0;
    }
	
    return charbuf;	/* beginning of unused area */
}

#define MAX 100

extern int
print_stacktrace(skip, count)
int skip, count;
{
    char *functions[MAX];
    char *addrs[MAX];
    char *files[MAX];
    int lines[MAX];
    char buf[MAX*100];
    int n, i;

    if (count > MAX)
	count = MAX;

    n = get_stacktrace(skip+1, count, addrs);
    (void)symbolize_stacktrace(n, addrs, functions, files, lines,
					    buf, buf+sizeof(buf));
    cleanup_symbolize_stacktrace();
    for (i = 0; i < n; ++i) {
	if (lines[i])
	    printf("%s(), line %d in \"%s\"\n",
		functions[i][0] == '_' ? functions[i]+1 : functions[i],
		lines[i], files[i]);
	else
	    printf("%s() at %#x\n",
		functions[i][0] == '_' ? functions[i]+1 : functions[i],
		addrs[i]);
    }
}

#ifdef MAIN
#include <signal.h>

char *p;
int caught = 0;
catch(sig)
{
    caught = 1;
    fprintf(stderr, "Caught!\n");
    fprintf(stderr, "p = %#x\n", p);
    exit(0);
}

int **ip;
C(x,y,z)
{
    int f = 4, g = 5, h = 6;
    ip = (int **)&f;
    print_stacktrace(0,6000);
}

B(x,y)
{
    int f = 4, g = 5;
    C(x,y,3);
}

A(x)
{
    int f = 4;
    B(x,2);
}

extern int etext, edata, end;
main(argc, argv)
{
    char c;
    fprintf(stderr, "&etext = %#x\n", &etext);
    fprintf(stderr, "&edata = %#x\n", &edata);
    fprintf(stderr, "&end = %#x\n", &end);
    fprintf(stderr, "sbrk(0) = %#x\n", sbrk(0));
    A(1);
    A(1);
    A(1);
    exit(0);
}

#endif /* MAIN */

#endif /* STACKTRACE */
