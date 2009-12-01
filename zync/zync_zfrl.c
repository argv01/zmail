/* 
 * $RCSfile: zync_zfrl.c,v $
 * $Revision: 1.9 $
 * $Date: 1996/04/05 01:11:24 $
 * $Author: spencer $
 */

#include "popper.h"
#include <general/dynstr.h>

static const char zync_zfrl_rcsid[] =
    "$Id: zync_zfrl.c,v 1.9 1996/04/05 01:11:24 spencer Exp $";

/* ZFRL
 * Yield the "From " line of a message
 */

int
zync_zfrl(p)
     POP *p;
{
    int mnum = atoi(p->pop_parm[1]);
    struct msg_info *m;

    do_drop(p);

    if ((mnum <= 0) || (mnum > NUMMSGS(p))) {
	pop_msg(p, POP_FAILURE, "Message %d does not exist", mnum);
	return (POP_FAILURE);
    }
    m = NTHMSG(p, mnum);
    pop_msg(p, POP_SUCCESS, "%s", dynstr_Str(&m->from_line));
    return (POP_SUCCESS);
}
    

#if 0
int
zync_zfrl(p)
    POP *p;
{
    int mnum = atoi(p->pop_parm[1]);
    struct msg_info *m;
    struct dynstr d;

    do_drop(p);

    m = NTHMSG(p, mnum);
    efseek(p->drop, m->header_offset, L_SET, "zync_zfrl");
    dynstr_Init(&d);
    TRY {
	int c;
	char *s, buf[MAXMSGLINELEN+1], path[MAXMSGLINELEN+1], user[256];
	struct dynstr uucpfrom;

	while (((c = fgetc(p->drop)) != EOF) && (c != '\r') && (c != '\n'))
	    dynstr_AppendChar(&d, c);

	user[0] = path[0] = '\0';
	s = NULL;
	while ((fgets(buf, MAXMSGLINELEN, p->drop) != NULL)
              && !strncmp(buf, ">From ", 6)) {
	    /* the body of this while loop is largely taken from
	       parse_from() in msgs/foload.c */
	    s = buf+6;
	    (void)sscanf(s, "%s", user);
	    while (s = index(s+1, 'r')) {
	        if (!strncmp(s, "remote from ", 12)) {
		    char *p2 = path+strlen(path);
		    /* skipspaces(12); */
		    for (s += 12; *s == ' ' || *s == '\t'; ++s)
		        ;
		    /* add the new machine to path */
		    (void) sscanf(s, "%s", p2);
		    (void) strcat(p2, "!");
		    break;
		}
	    }
	}
	if (user[0])
	    bang_form(path+strlen(path), user);

	if (path[0]) {
	    sscanf(dynstr_Str(&d)+5, "%s", user);
	    pop_msg(p, POP_SUCCESS, "From %s%s", path,
		    dynstr_Str(&d)+5+strlen(user));
	} else
	    pop_msg(p, POP_SUCCESS, "%s", dynstr_Str(&d));
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
    return (POP_SUCCESS);
}
#endif
