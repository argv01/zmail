/* dyncond.c     Copyright 1990, 1991, 1992, 1993 Z-Code Software Corp. */

#include "zmail.h"
#include "linklist.h"
#include "vars.h"
#include "dyncond.h"
#include "catalog.h"
#include <ctype.h>

enum bexp_toks {
    BEXP_DAND,
    BEXP_DOR,
    BEXP_DEQ,
    BEXP_NEQ,
    BEXP_STR,
    BEXP_EOS,
    BEXP_ERROR
};

enum bexp_types {
    BEXP_VALUE,
    BEXP_PAREN,
    BEXP_UNARY,
    BEXP_NONASSOC,
    BEXP_BOOL
};

static const char *bexpin;
static char *bexpstr = NULL;
static int bexp_tok, bexp_skip, bexp_stackptr;

typedef struct bexpop {
    int tok, prec, type;
} bexpop;

typedef struct bexpval {
    int is_str;
    union {
	char *sval;
	int ival;
    } u;
} bexpval;

#define BEXP_TOP_PREC 100

static bexpop bexpops[] = {
    BEXP_STR, 0, BEXP_VALUE,
    '(', 1, BEXP_PAREN,
    '!', 2, BEXP_UNARY,
    '<', 3, BEXP_NONASSOC,
    '>', 3, BEXP_NONASSOC,
    BEXP_DEQ, 4, BEXP_NONASSOC,
    BEXP_NEQ, 4, BEXP_NONASSOC,
    BEXP_DAND, 5, BEXP_BOOL,
    BEXP_DOR,  6, BEXP_BOOL,
    BEXP_EOS,  BEXP_TOP_PREC+1, 0
};

#define BEXP_STACK_SIZE 10
static bexpval bexp_stack[BEXP_STACK_SIZE];

#define TOK_IS(X) { bexp_tok = (X); return bexp_tok; }

static int
bexp_get_tok()
{
    char *bptr;
    static char buf[100];
    int q = 0;

    if (bexpstr) { xfree(bexpstr); bexpstr = NULL; }
    while (*bexpin && isspace(*bexpin)) bexpin++;
    if (!*bexpin) TOK_IS(BEXP_EOS);
    if (index("()<>", *bexpin)) TOK_IS(*bexpin++);
    switch (*bexpin) {
    case '|': if (bexpin[1] == *bexpin) { bexpin += 2; TOK_IS(BEXP_DOR); }
    when '&': if (bexpin[1] == *bexpin) { bexpin += 2; TOK_IS(BEXP_DAND); }
    when '=': if (bexpin[1] == *bexpin) { bexpin += 2; TOK_IS(BEXP_DEQ); }
    when '!': if (bexpin[1] == '=')     { bexpin += 2; TOK_IS(BEXP_NEQ); }
	      else TOK_IS(*bexpin++);
    when '"': q = *bexpin++;
    }
    bptr = buf;
    while (*bexpin && (q ||
	    !isspace(*bexpin) && (!ispunct(*bexpin) || *bexpin == '$'))) {
	if (q) {
	    if (*bexpin == q) q = 0;
	    else *bptr++ = *bexpin;
	    bexpin++;
	    continue;
	}
	*bptr++ = *bexpin++;
	if (bptr[-1] == '$') {
	    if (*bexpin == '?') *bptr++ = *bexpin++;
	    while (isalnum(*bexpin) || *bexpin == '_') *bptr++ = *bexpin++;
	    if (*bexpin == ':') {
		if (bexpin[1] == '(') {
		    while (*bexpin && *bexpin != ')') *bptr++ = *bexpin++;
		    if (*bexpin == ')') *bptr++ = *bexpin++;
		} else if (bexpin[1]) {
		    *bptr++ = *bexpin++;	/* Copy the ':' itself */
		    *bptr++ = *bexpin++;	/* Copy letter after ':' */
		}
	    }
	}
    }
    *bptr = 0;
    if (*buf == 0) TOK_IS(BEXP_ERROR);
    if (*buf == '$' && !bexp_skip) {
	struct expand expansion;

	expansion.orig = buf;
	if (varexp(&expansion)) {
	    strapp(&expansion.exp, expansion.rest);
	    bexpstr = expansion.exp;
	} else
	    TOK_IS(BEXP_ERROR);
    } else
	bexpstr = savestr(buf);
    TOK_IS(BEXP_STR);
}

static void
bexp_push(i)
{
    bexpval *b;

    b = bexp_stack+(bexp_stackptr++);
    if (b->is_str) xfree(b->u.sval);
    b->is_str = FALSE;
    b->u.ival = i;
}

static void
bexp_push_str(s)
char *s;
{
    bexpval *b;

    b = bexp_stack+(bexp_stackptr++);
    if (b->is_str) xfree(b->u.sval);
    b->is_str = TRUE;
    b->u.sval = savestr(s);
}

int
is_number(s)
char *s;
{
    if (*s == '-') s++;
    if (!*s) return FALSE;
    while (*s && isdigit(*s)) s++;
    return (*s == 0);
}

static int
bexp_pop2v(ai, bi, as, bs)
int *ai, *bi;
char **as, **bs;
{
    bexpval *b1, *b2;

    *ai = *bi = 0;
    b2 = bexp_stack+(--bexp_stackptr);
    b1 = bexp_stack+(--bexp_stackptr);
    if (bexp_stackptr <  0) return TRUE;
    if (!b1->is_str && !b2->is_str) {
	*ai = b1->u.ival;
	*bi = b2->u.ival;
	return TRUE;
    }
    if (!b1->is_str) {
	b1->u.sval = savestr(itoa(b1->u.ival));
	b1->is_str = TRUE;
    }
    if (!b2->is_str) {
	b2->u.sval = savestr(itoa(b2->u.ival));
	b2->is_str = TRUE;
    }
    *as = b1->u.sval;
    *bs = b2->u.sval;
    return FALSE;
}

static int
bexp_pop()
{
    bexpval *b;

    b = bexp_stack+(--bexp_stackptr);
    if (bexp_stackptr < 0) return 0;
    if (b->is_str)
	return atoi(b->u.sval);
    else
	return b->u.ival;
}

static void
bexp_par_exp(prec)
int prec;
{
    int n, op;
    int ai, bi;
    char *as, *bs;

    for (;;) {
	if (bexp_stackptr < 0) return;
	for (n = 0; bexpops[n].tok != BEXP_EOS; n++)
	    if (bexpops[n].tok == bexp_tok) break;
	if (bexpops[n].tok == BEXP_EOS) return;
	op = bexp_tok;
	switch (bexpops[n].type) {
	case BEXP_PAREN:
	    bexp_get_tok();
	    bexp_par_exp(BEXP_TOP_PREC);
	    if (bexp_tok != ')') return;
	    bexp_get_tok();
	    op = -1;
	when BEXP_UNARY:
	    bexp_get_tok();
	    bexp_par_exp(bexpops[n].prec);
	when BEXP_NONASSOC:
	    if (bexpops[n].prec > prec) return;
	    bexp_get_tok();
	    bexp_par_exp(bexpops[n].prec-1);
	    prec--;
	when BEXP_BOOL:
	    if (bexpops[n].prec > prec) return;
	when BEXP_VALUE:
	    if (is_number(bexpstr))
		bexp_push(atoi(bexpstr));
	    else
		bexp_push_str(bexpstr);
	    bexp_get_tok();
	    op = -1;
	}
	switch (op) {
	    int val;
#define BEXP_POP()  do { \
			val = bexp_pop(); \
			if (bexp_stackptr < 0) return; \
		    } while (0)
#define BEXP_POP2() do { \
			val = bexp_pop2v(&ai, &bi, &as, &bs); \
			if (bexp_stackptr < 0) return; \
		    } while (0)
	case '<':
	    BEXP_POP2();
	    if (val)
		bexp_push(ai < bi);
	    else
		bexp_push(strcmp(as,bs) < 0);
	when '>':
	    BEXP_POP2();
	    if (val)
		bexp_push(ai > bi);
	    else
		bexp_push(strcmp(as,bs) > 0);
	when '!':
	    BEXP_POP();
	    bexp_push(!val);
	when BEXP_DEQ:
	    BEXP_POP2();
	    if (val)
		bexp_push(ai == bi);
	    else
		bexp_push(!strcmp(as, bs));
	when BEXP_NEQ:
	    BEXP_POP2();
	    if (val)
		bexp_push(ai != bi);
	    else
		bexp_push(!!strcmp(as, bs));
	when BEXP_DAND:
	    BEXP_POP();
	    if (val) {
		bexp_get_tok();
		bexp_par_exp(bexpops[n].prec);
	    } else {
		bexp_skip++;
		bexp_get_tok();
		bexp_par_exp(bexpops[n].prec);
		bexp_skip--;
		BEXP_POP();
		bexp_push(0);
	    }
	    prec--;
	when BEXP_DOR:
	    BEXP_POP();
	    if (!val) {
		bexp_get_tok();
		bexp_par_exp(bexpops[n].prec);
	    } else {
		bexp_skip++;
		bexp_get_tok();
		bexp_par_exp(bexpops[n].prec);
		bexp_skip--;
		BEXP_POP();
		bexp_push(1);
	    }
	    prec--;
	}
	bexp_par_exp(prec);
    }
}

int
eval_bexp(s, ok)
const char *s;
int *ok;
{
    int ret = -1;

    bexpin = s;
    bexp_stackptr = 0;
    bexp_get_tok();
    bexp_par_exp(BEXP_TOP_PREC);
    if (ok) *ok = FALSE;
    if (bexp_stackptr != 1)
	print(catgets( catalog, CAT_SHELL, 302, "bad stackpointer = %d\n" ), bexp_stackptr);
    else if (bexp_tok != BEXP_EOS)
	print(catgets( catalog, CAT_SHELL, 303, "parse error in expression\n" ));
    else {
	ret = bexp_pop();
	if (ok) *ok = TRUE;
    }
    return ret;
}

#define DC_VAR_COUNT 10

DynCondition
CreateDynCondition(str, cbroutine, cbdata)
const char *str;
void_proc cbroutine;
VPTR cbdata;
{
    int varct = 0;
    const char *p;
    char *vars[DC_VAR_COUNT];
    DynCondition dc;
    ZmCallback tmpcb;

    p = str;
    do
    {
      if ('$' == *p++)
      {
	if ('?' == *p)
	  ++p;
	if (!isalpha(*p) && *p != '_') return (DynCondition) 0;
	if (DC_VAR_COUNT == varct) break;
        {
	  const char *head = p++;
	  while (isalnum(*p) || '_' == *p) p++;
	  vars[varct] = savestrn(head, p-head);
	}
	varct++;
      }
    } while (*p);
    if (varct == DC_VAR_COUNT) {
	while (varct--) xfree(vars[varct]);
	return (DynCondition) 0;
    }
    dc = (DynCondition) calloc(sizeof *dc, 1);
    dc->exp = savestr(str);
    while (varct--) {
	tmpcb = dc->callback_chain;
	dc->callback_chain =
	    ZmCallbackAdd(vars[varct], ZCBTYPE_VAR, cbroutine, cbdata);
	xfree(vars[varct]);
	ZCBChain(dc->callback_chain) = tmpcb;
    }
    return dc;
}

void
DestroyDynCondition(dc)
DynCondition dc;
{
    ZmCallback cb, nextcb;

    if (!dc) return;
    cb = dc->callback_chain;
    while (cb) {
	nextcb = ZCBChain(cb);
	ZmCallbackRemove(cb);
	cb = nextcb;
    }
    xfree(dc->exp);
    xfree(dc);
}

void
SetDynConditionValue(dc, val)
DynCondition dc;
int val;
{
    char *exp, *s, *item;
    int truth = FALSE;

    exp = dc->exp;
    if (*exp == '!') { val = !val; exp++; }
    if (*exp != '$') return;
    exp++;
    if (*exp == '?') {
	truth = TRUE;
	exp++;
    }
    s = savestr(exp);
    if ((item = index(s, '(')) && item[-1] == ':') {
	char *ptr;
	item[-1] = 0;
	*item++ = 0;
	if (ptr = index(item, ')')) *ptr = 0;
	if (!truth) { xfree(s); return; }
	cmd_line(zmVaStr("builtin set %s %c= %s",
			 s, (val) ? '+' : '-', item), 0);
    } else if (truth)
	cmd_line(zmVaStr("builtin %sset %s",
			 (val) ? "" : "un", s), 0);
    else
	cmd_line(zmVaStr("builtin set %s=%c", s, val+'0'), 0);
    xfree(s);
}
