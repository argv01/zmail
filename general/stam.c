/* 
 * $RCSfile: stam.c,v $
 * $Revision: 2.5 $
 * $Date: 1995/02/17 02:46:07 $
 * $Author: bobg $
 */

#include <stam.h>
#include <excfns.h>

#ifndef lint
static const char stam_rcsid[] =
    "$Id";
#endif /* lint */

struct transition {
    int start, token, end;
};

struct state {
    char *name;
    void (*enter) NP((struct stam *, int, int, int));
    void (*leave) NP((struct stam *, int, int, int));
};

struct statename {
    char *name;
    int number;
};

static int
transition_cmp(t1, t2)
    struct transition *t1, *t2;
{
    int s = t1->s - t2->s;

    return (s ? s : t1->token - t2->token);
}

static int
statename_cmp(x1, x2)
    struct statename *sn1, *sn2;
{
    return (ci_strcmp(sn1->name, sn2->name));
}

static unsigned int
statename_hash(x)
    struct statename *sn;
{
    return (hashtab_StringHash(sn->name));
}

int
stam_AddState(st, name, enter, leave)
    struct stam *st;
    char *name;
    void (*enter) NP((struct stam *, int, int, int));
    void (*leave) NP((struct stam *, int, int, int));
{
    struct state newstate;
    struct statename newstatename;

    newstate.name = (char *) emalloc(1 + strlen(name),
				     "stam_AddState");
    strcpy(newstate.name, name);
    newstate.enter = enter;
    newstate.leave = leave;
    newstatename.name = newstate.name;
    newstatename.number = dlist_Append(&(st->states), &newstate);
    hashtab_Add(&(st->statenames), &newstatename);
    return (newstatename.number);
}

void
stam_AddTransition(st, start, token, end)
    struct stam *st;
    int start, token, end;
{
    struct transition t, *tp;

    t.start = start;
    t.token = token;
    if (tp = (struct transition *) sklist_Find(&(st->transitions), &t)) {
	/* Replace existing transition */
	tp->end = end;
    } else {
	t.end = end;
	sklist_Insert(&(st->transitions), &t);
    }
}

int
stam_GetState(st, name)
    struct stam *st;
    char *name;
{
    struct statename probe, *found;

    probe.name = name;
    return ((found = hashtab_Find(&(st->statenames), &probe)) :
	    found->number : -1);
}

void
stam_Init(st, statetab)
    struct stam *st;
    int statetab;
{
    sklist_Init(&(st->transitions), (sizeof (struct transition)),
		transition_cmp, 1, 4);
    hashtab_Init(&(st->statenames), statename_hash,
		 statename_cmp, (sizeof (struct statename)), statetab);
    st->transitionfn = transitionfn;

    (void) stam_AddState(st, "BEGIN");
    (void) stam_AddState(st, "END");

    st->state = stam_BeginState;
}

int
stam_DoToken(st, state, token)
    struct stam *st;
    int state, token;
{
    struct transition probe, *found;

    probe.start = (state < 0) ? st->state : state;
    probe.token = token;
    if (found = (struct transition *) sklist_Find(&(st->transitions),
						  &probe, 0)) {
	st->state = found->end;
	if (st->transitionfn)
	    (*(st->transitionfn))(st, probe.start, token, found->end);
	return (found->end);
    }
    return (-1);
}

int
stam_Execute(st, state, tokenfn)
    struct stam *st;
    int state;
    int (*tokenfn) NP((struct stam *, int));
{
    if (state < 0)
	state = st->state;
    while (state != stam_EndState) {
	if ((state = stam_DoToken(st, state, (*tokenfn)(st, state))) < 0)
	    return (-1);
    }
    return (state);
}

void
stam_Destroy(st)
    struct stam *st;
{
    struct hashtab_iterator hti;
    struct state_or_token *x;

    hashtab_InitIterator(&hti);
    while (x = (struct state_or_token *) hashtab_Iterate(&(st->names.tokens),
							 &hti))
	free(x->name);
    hashtab_Destroy(&(st->names.tokens));

    hashtab_InitIterator(&hti);
    while (x = (struct state_or_token *) hashtab_Iterate(&(st->names.states),
							 &hti))
	free(x->name);
    hashtab_Destroy(&(st->names.states));

    sklist_Destroy(&(st->transitions));
}
