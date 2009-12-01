/* arith.c    Copyright 1992 Z-Code Software Corp. */
/*
 * XXX FIX THIS-- make help entry,
 */

/*===========================================================================*/
/*
 * arith.h  (take this out and put it in a real .h file some day if desired)
 */

#ifndef _ARITH_H_
#define _ARITH_H_

extern int zm_expr_evaluate(/* argc, argv, &val, errbuf */);
extern int zm_calc(/* argc, argv */);	/* prints results */

#ifndef THERE_IS_LIFE_BEYOND_ZMAIL
extern int zm_set_arith(/* argc, argv */);
#include "zmail.h"
#include "arith.h"
#include "catalog.h"
#endif /* !THERE_IS_LIFE_BEYOND_ZMAIL */
#endif /* !_ARITH_H_ */
/*===========================================================================*/


/*
 * arith.c	(would be expr.c, but that name is taken already)
 * C integer expression evaluator, with "**" (power) and "!" (factorial).
 * Subexpressions are evaluated conditionally,
 * so for example the following will produce the proper value if $x == 0:
 *		$x == 0  ||  $a / $x > 5
 * The parsing functions are completely reentrant and interruptable
 * (no static or global variables, and no memory allocation).
 * Don wrote this.
 */

#include <stdio.h>
#ifndef _ARITH_H_
#include "expr.h"
#endif /* !_ARITH_H_ */

#include <ctype.h>

#ifndef numberof
#define numberof(thingys) (sizeof(thingys)/sizeof(*(thingys)))
#endif /* !numberof */
#ifndef streq
#define streq(a,b) (strcmp(a,b)==0)
#endif /* !streq */
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif /* !MIN */


#ifdef MAIN	/* special definitions for the test program */
#define print (void)printf
#define error(type, fmt, a,b) ((void) fprintf(stderr, fmt, a,b))
#endif /* MAIN */



/*
 * Unary and binary operations.
 */
static EXPR_TYPE bitnot(a)   EXPR_TYPE a;   { return ~(long)a; }
static EXPR_TYPE lognot(a)   EXPR_TYPE a;   { return !a; }
static EXPR_TYPE neg(a)	     EXPR_TYPE a;   { return -a; }
static EXPR_TYPE fact(a)     EXPR_TYPE a;   { return a>0 ? a * fact(a-1) : 1; }
static EXPR_TYPE zpow(a,b)    EXPR_TYPE a,b; { return b>0 ? a * zpow(a,b-1) : 1; }
static EXPR_TYPE mult(a,b)  EXPR_TYPE a,b; { return a * b; }
static EXPR_TYPE plus(a,b)   EXPR_TYPE a,b; { return a + b; }
static EXPR_TYPE minus(a,b)  EXPR_TYPE a,b; { return a - b; }
static EXPR_TYPE lshift(a,b) EXPR_TYPE a,b; { return (long)a << (long)b; }
static EXPR_TYPE rshift(a,b) EXPR_TYPE a,b; { return (long)a >> (long)b; }
static EXPR_TYPE leq(a,b)    EXPR_TYPE a,b; { return a <= b; }
static EXPR_TYPE geq(a,b)    EXPR_TYPE a,b; { return a >= b; }
static EXPR_TYPE lt(a,b)     EXPR_TYPE a,b; { return a < b; }
static EXPR_TYPE gt(a,b)     EXPR_TYPE a,b; { return a > b; }
static EXPR_TYPE eq(a,b)     EXPR_TYPE a,b; { return a == b; }
static EXPR_TYPE neq(a,b)    EXPR_TYPE a,b; { return a != b; }
static EXPR_TYPE logand(a,b) EXPR_TYPE a,b; { return a && b; }
static EXPR_TYPE logor(a,b)  EXPR_TYPE a,b; { return a || b; }
static EXPR_TYPE bitand(a,b) EXPR_TYPE a,b; { return (long)a & (long)b; }
static EXPR_TYPE bitxor(a,b) EXPR_TYPE a,b; { return (long)a ^ (long)b; }
static EXPR_TYPE bitor(a,b)  EXPR_TYPE a,b; { return (long)a | (long)b; }
static EXPR_TYPE comma(a,b)  EXPR_TYPE a,b; { return a , b; }
static EXPR_TYPE intdiv(a,b) EXPR_TYPE a,b; { return (long)a / (long)b; }
static EXPR_TYPE mod(a,b)    EXPR_TYPE a,b; { return (long)a % (long)b; }

/*
 * Operator precedence.
 * From Harbison/Steele p. 141, with ** (power) and ! (factorial) added.
 */
#define LEFT 0
#define RIGHT 1
struct operator {
    char assoc;	/* LEFT or RIGHT */
    char prec;	/* precedence */
    char *name;
    EXPR_TYPE (*fun)();
} unops[] = {
    { RIGHT, 15, "~",	bitnot },
    { RIGHT, 15, "!",	lognot },
    { RIGHT, 15, "-",	neg },
}, binops[] = {
    { RIGHT, 14, "**",	zpow },
    { LEFT,  13, "*",	mult },
    { LEFT,  13, "/",	intdiv},
    { LEFT,  13, "%",	mod},
    { LEFT,  12, "+",	plus },
    { LEFT,  12, "-",	minus },
    { LEFT,  11, "<<",	lshift },
    { LEFT,  11, ">>",	rshift },
    { LEFT,  10, "<=",	leq },
    { LEFT,  10, ">=",	geq },
    { LEFT,  10, "<",	lt },
    { LEFT,  10, ">",	gt },
    { LEFT,   9, "==",	eq },
    { LEFT,   9, "!=",	neq },
    { LEFT,   5, "&&",	logand },	/* must come before "&" */
    { LEFT,   4, "||",	logor },	/* must come before "|" */
    { LEFT,   8, "&",	bitand },
    { LEFT,   7, "^",	bitxor },
    { LEFT,   6, "|",	bitor },
    { RIGHT,  3, "?",	comma },	/* special case in expr_parse() */
    { LEFT,   1, ",",	comma },
    { LEFT,  16, "!",	fact },		/* special case in expr_parse() */
};

/*
 * exprio routines
 * These functions perform simple character i/o operations on an argv vector,
 * without doing any memory allocation.
 *
 *	exprio_init(ep, argc,argv,i,j,errbuf) -- initializes the exprio
 *			structure pointed to by ep to refer to argc,argv,
 *			and sets the current position to argv[i][j].
 *	exprio_tell(ep) -- returns an integer that can later be used as an
 *			argument to exprio_seek().
 *	exprio_seek(ep, pos) -- sets the current position. pos must be the
 *			result of an earlier exprio_seek().
 *	exprio_getchar(ep) -- read and return a character
 *	exprio_peek(ep) -- return the character waiting to be read (or EOF)
 *	exprio_advance(ep) -- read and discard one character.
 *	exprio_discard_spaces(ep) -- advance to first nonspace char (or EOF)
 */

struct exprio {
    int argc;
    char **argv;
    int argi, argj;
    char *errbuf;
};

static void
exprio_init(ep, argc, argv, argi, argj, errbuf)
struct exprio *ep;
int argc;
char **argv;
int argi, argj;
char errbuf[EXPR_ERRBUFSIZ];
{
    ep->argc = argc;
    ep->argv = argv;
    ep->argi = argi;
    ep->argj = argj;
    ep->errbuf = errbuf;
}
static int
exprio_tell(ep)
struct exprio *ep;
{
    return ep->argj * (ep->argc+1)  + ep->argi;
}
static void
exprio_seek(ep, pos)
struct exprio *ep;
{
    ep->argj = pos / (ep->argc+1);
    ep->argi = pos % (ep->argc+1);
}
static int
exprio_peek(ep)
struct exprio *ep;
{
    return !ep->argv[ep->argi] ||
	   !ep->argv[ep->argi+1] && !ep->argv[ep->argi][ep->argj] ? EOF
	 : !ep->argv[ep->argi][ep->argj] ? ' '
	 : ep->argv[ep->argi][ep->argj];
}
static void
exprio_advance(ep)
struct exprio *ep;
{
    if (ep->argv[ep->argi]) {
	if (ep->argv[ep->argi][ep->argj])
	    ep->argj++;
	else {
	    ep->argi++;
	    ep->argj = 0;
	}
    }
}
static int
exprio_getchar(ep)
struct exprio *ep;
{
    int c = exprio_peek(ep);
    exprio_advance(ep);
    return c;
}
static void
exprio_discard_spaces(ep)
struct exprio *ep;
{
    int c;
    while ((c = exprio_peek(ep)) != EOF && isspace(c))
	exprio_advance(ep);
}
/*======================= end of exprio stuff ===============================*/


static int
expr_get_literal(ep, s)
struct exprio *ep;
char *s;
{
    int pos = exprio_tell(ep);
    exprio_discard_spaces(ep);
    for (; *s; s++)
	if (exprio_getchar(ep) != *s) {
	    exprio_seek(ep, pos);
	    return 0; /* failure */
	}
    return 1;	/* success */
}


static struct operator *
expr_get_op(ep, ops, number_of_ops, lowest_prec_allowed)
struct exprio *ep;
struct operator ops[];
unsigned number_of_ops;
int lowest_prec_allowed;	/* lowest operator precedence recognized */
{
    int i, pos;

    pos = exprio_tell(ep);
    for (i = 0; i < number_of_ops; ++i) {
	if (expr_get_literal(ep, ops[i].name)) {
	    if (ops[i].prec >= lowest_prec_allowed)
		return &ops[i];	/* success */
	    else {
		/*
		 * Don't continue; e.g. if && is on the input
		 * but its precedence is too low to be recognized,
		 * we want to leave the whole thing on the
		 * input rather than reading the '&'.
		 */
		exprio_seek(ep, pos);
		return (struct operator *)NULL;
	    }
	}
    }
    return (struct operator *)NULL;	/* failure */
}


static int
expr_get_constant(ep, val)
struct exprio *ep;
EXPR_TYPE *val;
{
    int c;
    EXPR_TYPE scale;

    exprio_discard_spaces(ep);

    if ((c = exprio_peek(ep)) == EOF || !isdigit(c))
	return 0;	/* failure */

    if (val)
	*val = 0;
    while ((c = exprio_peek(ep)) != EOF && isdigit(c)) {
	if (val)
	    *val = *val * 10 + (c - '0');
	exprio_advance(ep);
    }
    if (exprio_peek(ep) == '.') {
	exprio_advance(ep);
	scale = 1;
	while ((c = exprio_peek(ep)) != EOF && isdigit(c)) {
	    scale /= 10;	/* if EXPR_TYPE is int, this will always be 0 */
	    if (val)
		*val = *val + scale * (c - '0');
	    exprio_advance(ep);
	}
    }
    return 1;	/* success */
}


/*
 * Returns 1 on success,
 * 0 on syntax or illegal operation error (in which case an error message
 * is printed and the io pointer is left at the character where the error
 * occurred.)
 * If val is NULL, parse but don't evaluate.
 */
static int
expr_parse(ep, lowest_prec_allowed, val)
struct exprio *ep;
int lowest_prec_allowed;	/* lowest operator precedence recognized */
EXPR_TYPE *val;
{
    struct operator *unop, *binop;
    EXPR_TYPE RHS = 0;

    if (unop = expr_get_op(ep, unops, numberof(unops), lowest_prec_allowed)) {
	if (!expr_parse(ep, unop->prec, val))
	    return 0;	/* failure */
	if (val)
	    *val = unop->fun(*val);		/* expr -> unop expr */
    } else if (expr_get_literal(ep, "(")) {
	if (!expr_parse(ep, 0, val))	/* expr -> '(' expr ')'  */
	    return 0;	/* failure */
	if (!expr_get_literal(ep, ")")) {
	    (void) sprintf(ep->errbuf,
		    exprio_peek(ep) == EOF ? catgets( catalog, CAT_SHELL, 1, "unexpected end-of-expression" )
					   : catgets( catalog, CAT_SHELL, 2, "syntax error near '%c'" ),
		    exprio_peek(ep));
	    return 0;	/* failure */
	}
    } else if (!expr_get_constant(ep, val)) {
	if (val)
	    *val = 0;  /* empty string is a valid expression -- should it be? */
    }

    while (binop =
	       expr_get_op(ep, binops, numberof(binops), lowest_prec_allowed)) {
	if (streq(binop->name, "!")) {
	    /*
	     * '!' in this context is the right-unary factorial operator,
	     * in which case no RHS is needed...
	     */
	} else if (streq(binop->name, "?")) {
	    if (!expr_parse(ep, binop->prec, val&&*val ? &RHS
						       : (EXPR_TYPE *)NULL))
		return 0;	/* failure */
	    if (!expr_get_literal(ep, ":")) {
		(void) sprintf(ep->errbuf,
			exprio_peek(ep) == EOF ? catgets( catalog, CAT_SHELL, 1, "unexpected end-of-expression" )
					       : catgets( catalog, CAT_SHELL, 2, "syntax error near '%c'" ),
			exprio_peek(ep));
		return 0;	/* failure */
	    }
	    if (!expr_parse(ep, binop->prec, val&&!*val ? &RHS
							: (EXPR_TYPE *)NULL))
		return 0;	/* failure */
	} else {
	    if (!expr_parse(ep, binop->assoc == RIGHT ? binop->prec
						      : binop->prec+1,
				!val || streq(binop->name, "&&") && *val == 0
				     || streq(binop->name, "||") && *val != 0
					   ? (EXPR_TYPE *)NULL : &RHS))
		return 0;	/* failure */
	}
	if (val) {
	    if (streq(binop->name, "/") && RHS == 0) {
		(void)sprintf(ep->errbuf, catgets( catalog, CAT_SHELL, 5, "divide by zero" ));
		*val = 0;
		return 0;	/* failure */
	    }
	    if (streq(binop->name, "%") && RHS == 0) {
		(void)sprintf(ep->errbuf, catgets( catalog, CAT_SHELL, 6, "mod by zero" ));
		*val = 0;
		return 0;	/* failure */
	    }
	    *val = binop->fun(*val, RHS);
	}
    }
    return 1;	/* success */
}

/*
 * Calculate the value of an expression.
 * Returns 0 (and puts result in val) on success,
 * returns -1 (and puts error message in errbuf) on failure.
 */
extern int
zm_expr_evaluate(argc, argv, val, errbuf)
int argc;
char **argv;
EXPR_TYPE *val;
char errbuf[EXPR_ERRBUFSIZ];
{
    struct exprio exprio;

    exprio_init(&exprio, argc, argv, 0, 0, errbuf);

    if (!expr_parse(&exprio, 0, val))
	return -1;	/* failure, message is in errbuf */

    exprio_discard_spaces(&exprio);
    if (exprio_peek(&exprio) != EOF) {
	(void) sprintf(errbuf, catgets( catalog, CAT_SHELL, 2, "syntax error near '%c'" ),
			       exprio_peek(&exprio));
	return -1;	/* failure */
    }

    return 0;	/* success */
}

/*
 * Returns 0 on success,
 * returns -1 (and outputs error message) on error.
 * Prints result (as far as it could evaluate) in any case.
 */
extern int
zm_calc(argc, argv)
int argc;
char **argv;
{
    int returnval;
    char errbuf[EXPR_ERRBUFSIZ];
    EXPR_TYPE val;

    returnval = zm_expr_evaluate(argc-1, argv+1, &val, errbuf);

    if (returnval == -1)
	print("%s: %s\n", argv[0], errbuf);
	/* error(UserErrWarning, "%s: %s\n", argv[0], errbuf); */

    print(EXPR_FMT, val);
    print("\n");

    return returnval;
}


#define issufix(suf, str) (strlen(str) >= strlen(suf) && \
			   streq(suf, (str) + strlen(str) - strlen(suf)))
#ifndef THERE_IS_LIFE_BEYOND_ZMAIL
extern int
zm_set_arith(argc, argv)
int argc;
char **argv;
{
    char *args[4], buf[50];
    char errbuf[EXPR_ERRBUFSIZ];
    EXPR_TYPE val;

    if (argv[0] && argv[1] && streq(argv[1], "-?"))
	return help(0, argv[0], cmd_help);

    if (argc < 3 || !issufix("=", argv[2])) {
	print(catgets( catalog, CAT_SHELL, 8, "Usage: %s <variable> = <expression>\n" ), argv[0]);
	return -1;
    }
    if (user_settable(&set_options, argv[1], (Variable **)0) == 0)
	return -1;

    if (zm_expr_evaluate(argc-3, argv+3, &val, errbuf) == -1) {
	print("%s: %s\n", argv[0], errbuf);
	/* error(UserErrWarning, "%s: %s\n", argv[0], errbuf); */
	return -1;
    }

    if (!streq(argv[2], "=")) {
	/*
	 * Deal with a command of the form "@ a += <expr>"
	 * by first evaluating expr (which we did already),
	 * then evaluating the new expression "$a + <value of expr>"
	 */
	char *prev_val_str;
	EXPR_TYPE prev_val;
	prev_val_str = value_of(argv[1]);
	if (!prev_val_str) {
	    print(catgets( catalog, CAT_SHELL, 9, "%s: undefined variable\n" ), argv[1]);
	    return -1;
	}
	if (sscanf(prev_val_str, EXPR_FMT, &prev_val) != 1) {
	    print(catgets( catalog, CAT_SHELL, 10, "%s: not a numeric variable\n" ), argv[1]);
	    return -1;
	}
	sprintf(buf, EXPR_FMT, prev_val);
	sprintf(buf+strlen(buf), " %.*s ", MIN(3, strlen(argv[2])-1), argv[2]);
	sprintf(buf+strlen(buf), EXPR_FMT, val);
	args[0] = buf;
	args[1] = NULL;
	if (zm_expr_evaluate(1, args, &val, errbuf) == -1) {
	    print("%s: %s\n", argv[0], errbuf);
	    /* error(UserErrWarning, "%s: %s\n", argv[0], errbuf); */
	    return -1;
	}
    }

    sprintf(buf, EXPR_FMT, val);

    return set_var(argv[1], "=", buf);
}
#endif /* !THERE_IS_LIFE_BEYOND_ZMAIL */


#ifdef MAIN	/* little test program */
main(argc, argv)
int argc;
char **argv;
{
    return zm_calc(argc, argv);
}
#endif /* MAIN */
