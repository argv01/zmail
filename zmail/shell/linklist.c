/* linklist.c	Copyright 1991 Z-Code Software Corp. */

#include "osconfig.h"
#include "linklist.h"
#include "zcstr.h"
#include "zm_ask.h"
#include "catalog.h"
#ifndef MAC_OS
# include <general/general.h>
#else /* MAC_OS */
# include "general.h"
#endif /* !MAC_OS */

#ifndef HAVE_STRING_H
# ifdef HAVE_PROTOTYPES
extern char *strcpy(char *to, const char *from);
extern int strcmp(const char *a, const char *b);
# else /* HAVE_PROTOTYPES */
#ifndef strcpy
extern char *strcpy();
#endif /* strcpy */
#ifndef strcmp
extern int strcmp();
#endif /* strcmp */
# endif /* HAVE_PROTOTYPES */
#endif /* HAVE_STRING_H */

/*
 * READ include/linklist.h BEFORE USING ROUTINES IN THIS FILE!
 */

/* Insert an element at the "end" of a circular linked list */
void
insert_link(list, new)
GENERIC_POINTER_TYPE *list, *new;
{
    if (*(struct link **)list) {
	((struct link *)new)->l_prev = (*(struct link **)list)->l_prev;
	(*(struct link **)list)->l_prev->l_next = (struct link *)new;
	(*(struct link **)list)->l_prev = (struct link *)new;
	((struct link *)new)->l_next = *(struct link **)list;
    } else {
	((struct link *)new)->l_prev = (struct link *)new;
	((struct link *)new)->l_next = (struct link *)new;
	*(struct link **)list = (struct link *)new;
    }
}

/* Treat a circular linked list as a stack */
void
push_link(list, new)
GENERIC_POINTER_TYPE *list, *new;
{
    insert_link(list, new);
    *(struct link **)list = (struct link *)new;
}

/* Insert a link element in sorted order */
/* XXX NOTE: There is currenlty a difference in the compar functions
 *     required by insert_sorted_link and retrieve_link.  This should
 *     probably be corrected by changing the current retrieve_link to
 *     retrieve_named_link and adding a new retrieve_link that works
 *     like insert_sorted_link with respect to comparisons.
 */
void
insert_sorted_link(list, new, compar)
GENERIC_POINTER_TYPE *list, *new;
int (*compar)();
{
    struct link *tmp;
    int n;

    if (!compar)
	compar = strcmp;

    if (*(struct link **)list) {
	tmp = *(struct link **)list;
	do {
	    if ((n = (*compar)(tmp, new)) > 0)
		break;
	    tmp = tmp->l_next;
	} while (tmp != *(struct link **)list);
	if (tmp == *(struct link **)list)
	    if (n > 0)
		push_link(list, new);
	    else
		insert_link(&tmp, new);
	else
	    insert_link(&tmp, new);
    } else
	insert_link(list, new);
}

/* Remove an element from a circular linked list */
void
remove_link(list, link)
GENERIC_POINTER_TYPE *list, *link;
{
    if (((struct link *)link)->l_next == (struct link *)link)
	if (link == *(struct link **)list)
	    *(struct link **)list = 0;
	else
	    error(ZmErrWarning, catgets( catalog, CAT_SHELL, 410, "link not in list" ));
    else {
	((struct link *)link)->l_next->l_prev = ((struct link *)link)->l_prev;
	((struct link *)link)->l_prev->l_next = ((struct link *)link)->l_next;
	if (*(struct link **)list == (struct link *)link)
	    *(struct link **)list = (*(struct link **)list)->l_next;
    }
    ((struct link *)link)->l_next = ((struct link *)link)->l_prev = 0; /* Sever the connection */
}

/* return nth element in a circular linked list */
struct link *
retrieve_nth_link(list, n)
    CVPTR list;
    int n;
{
    struct link *tmp = (struct link *)list;

    if (list)
	do {
	    if (--n == 0)
		return tmp;
	    tmp = tmp->l_next;
	} while (tmp != list);
    return 0;
}

/* Find an element in a circular linked list by allowing the caller to
 * compare the link structures directly using his own compare routine.
 * The object "x" is an arbitrary value (of size pointer), so it is assumed
 * the caller has provided a routine that knows what to expect.  The compare
 * routine will be called:
 *      compar(x, list_object)
 */
struct link *
find_link(list, x, compar)
CVPTR list;
CVPTR x;
int (*compar)();
{
    struct link *tmp = (struct link *)list, *tmp2;

    if (!compar || !list || !x)
	return 0;

    do {
	tmp2 = tmp->l_next;
	if ((*compar)(x, tmp) == 0)
	    return tmp;
	tmp = tmp2;
    } while (tmp != list);
    return 0;
}

/* Search for a specified element in a circular linked list */
struct link *
retrieve_link(list, id, compar)
    CVPTR list, id;
    int (*compar)();
{
    struct link *tmp = (struct link *)list;

    if (!compar)
	compar = strcmp;

    if (list && id)
	do {
	    /* XXX this could be optimized if we know the list is in
	     * alphabetical order by seeing if compar returns < 0.
	     */
	    if (tmp->l_name && (*compar)(tmp->l_name, id) == 0)
		return tmp;
	    tmp = tmp->l_next;
	} while (tmp != list);
    return 0;
}

/* Copy a linked list */
struct link *
copy_all_links(list, copyit)
    CVPTR list;
    struct link *(*copyit)();
{
    struct link *tmp, *cpy, *new = 0;

    if (tmp = (struct link *)list)
	do {
	    if (cpy = (*copyit)(tmp))
		insert_link(&new, cpy);
	} while ((tmp = tmp->l_next) != list);

    return new;
}

int
number_of_links(list)
    CVPTR list;
{
    struct link *tmp;
    int i = 0;

    if (tmp = (struct link *)list)
	do {
	    i++;
	} while ((tmp = tmp->l_next) != list);

    return i;							        
}

/* Insert an element into additional linked lists.
 *
 * This function requires that the list element structure contains two
 * (or more) struct link subfields.  The "elem" points to the element to
 * be inserted and the "link" points to the struct link that should be
 * inserted in the list.  The l_name pointer of the link is cast to point
 * to elem, so after the link is retrieved, l_name must be dereferenced
 * with an appropriate cast in order to reach the actual element.
 *
 * NOTE THAT THIS REQUIRES THE CALLER TO BE AWARE THAT THE LIST DOES
 * NOT CONTAIN POINTERS TO THE ACTUAL ELEMENTS!  Functions passed to
 * retrieve_link() etc. must be prepared to do the additional deref to
 * get the element from the link.
 *
 * The restrictions on position of the struct link within the element
 * structure do not apply when using this function.
 *
 * Example of usage:
 *  struct chain {
 *      struct link primary;
 *      struct link secondary;
 *  };
 *
 *  struct list *first, *other;
 *  struct chain shackle, *bond;
 *
 *  shackle.primary.l_name = "Shackle";
 *
 *  insert_link(&first, &shackle.primary);
 *  multi_link(&other, &shackle, &shackle.secondary);
 *
 *  bond = retrieve_link(first, "Shackle", strcmp);
 *  bond = retrieve_link(other, "Shackle", strptrcmp);
 *
 * The multi_link() function can of course be used with the primary link
 * structure as well, in which case the l_name may be treated as a "self"
 * pointer.  However, this is not very useful for retrieve_link().
 */
void
multi_link(list, elem, link)
GENERIC_POINTER_TYPE *list, *elem, *link;
{
    insert_link(list, link);
    ((struct link *)link)->l_name = (char *)elem;
}
