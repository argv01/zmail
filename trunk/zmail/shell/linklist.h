/* linklist.h	Copyright 1991 Z-Code Software Corp. */

/*
 * $Revision: 2.9 $
 * $Date: 1994/04/23 00:49:02 $
 * $Author: liblit $
 */

#ifndef _LINKLIST_H_
#define _LINKLIST_H_

/*
 * This file defines the interface to a set of linked list manipulation
 * routines.  The lists maintained are doubly-linked in a cycle, so list
 * traversal requires a loop of the form:
 *
 *	if ((link = list) != 0) do {
 *	    operate_on(link);
 *	} while ((link = link->l_next) != list);
 *
 * The linked list routines are usable with arbitrary structures, provided
 * that the following rules are followed:
 *
 *	1.  The structure must contain an element of type struct link
 *	    (NOT pointer to struct link).
 *	2.  The struct link must be the FIRST ELEMENT of the enclosing
 *	    structure.
 *	3.  The structure may be inserted into AT MOST ONE linked list
 *	    at a time.
 *
 * If rule (1) is not followed, nothing will work.  It is possible to
 * violate rule (2) by passing the address of the struct link element to
 * insert_link(); however, in this case retrieve_link() will return the
 * address OF THE LINK ELEMENT, not that of the enclosing structure.  It
 * is possible to violate rule (3) by declaring multiple struct link
 * elements and using one element for each list, but this by definition
 * violates rule (2) as well.  The multi_link() function supports this,
 * by requiring that the list element structure contains two (or more)
 * struct link subfields, and using the l_name field of the other links
 * to refer to the enclosing structure.
 *
 * NOTE THAT multi_link() REQUIRES THE CALLER TO BE AWARE THAT THE LIST
 * DOES NOT CONTAIN POINTERS TO THE ACTUAL ELEMENTS!  Functions passed
 * to retrieve_link() etc. must be prepared to do an additional deref to
 * get the element from the link.
 *
 * Example of multi-link usage:
 *
 *    struct web {
 *        struct link primary;
 *        struct link secondary;
 *        struct link tertiary;
 *    };
 *
 *    struct list *first, *second, *third;
 *    struct web knot, *net;
 *    char *name = "Knot";
 *
 *    knot.primary.l_name = name;
 *
 *    insert_link(&first, &knot.primary);
 *    multi_link(&second, &knot, &knot.secondary);
 *    multi_link(&third, &knot, &knot.tertiary);
 *
 *    net = retrieve_link(first, name, strcmp);
 *    net = retrieve_link(second, &name, strptrcmp);
 *    net = retrieve_link(third, &name, strptrcmp);
 *
 * The multi_link() function can of course be used with the primary link
 * structure as well, in which case the l_name may be treated as a "self"
 * pointer.  However, this is not very useful for retrieve_link().
 */

#include "zctype.h"
#ifndef MAC_OS
# include <general/general.h>
#else /* MAC_OS */
# include "general.h"
#endif /* !MAC_OS */

struct link {
    char *l_name;		/* THIS MUST BE THE FIRST ELEMENT */
    struct link *l_next;
    struct link *l_prev;
};

#define link_next(type,field,item) (type *)((item)->field.l_next)
#define link_prev(type,field,item) (type *)((item)->field.l_prev)


extern struct link
    *copy_all_links P((CVPTR, struct link *(*)())),
    *retrieve_link P((CVPTR, CVPTR, int (*)())),
    *find_link P((CVPTR, CVPTR, int (*)())),
    *retrieve_nth_link P((CVPTR, int));
extern void
    insert_link P((VPTR, VPTR)),
    insert_sorted_link P((VPTR, VPTR, int (*)())),
    multi_link P((VPTR, VPTR, VPTR)),
    push_link P((VPTR, VPTR)),
    remove_link P((VPTR, VPTR));
extern int
    number_of_links P((CVPTR));

#endif /* _LINKLIST_H_ */
