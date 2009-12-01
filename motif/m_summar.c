/* m_summar.c	Copyright 1990-95 Z-Code Software Corp. */

#ifndef lint
static char	m_summar_rcsid[] =
    "$Id: m_summar.c,v 2.4 1996/05/23 17:57:33 schaefer Exp $";
#endif

#include "config.h"

#include <Xm/List.h>
#include <Xm/Xm.h>

#include "zmail.h"
#include "catalog.h"
#include "zm_motif.h"
#include "zmframe.h"

static void update_hidden();

/*
 * These global variables are tested by all the header-list-redraw functions
 */
static XmStringTable hdr_list;
static int hdr_cache_len, hdr_cache_msg_ct;
static char *cache_format;

static char hdr_reverse_list = 0;

static XmString XmStringRefill P((XmString, char *, int));

/* Bart: Fri Jan 15 17:22:46 PST 1993 -- workaround for LANG bug */
static char lang_En_US;

/* set the current message to be the message specified.  This may or
 * may not be the same message.  If the msg_num is the same as the
 * current_msg, then it is assumed its status has changed and needs
 * repainting.  Also, it might be necessary to display the message.
 */
void
#ifdef HAVE_PROTOTYPES		/* default argument promotion games */
SetCurrentMsg(Widget list_w, int msg_num, Boolean print_msg)
#else
SetCurrentMsg(list_w, msg_num, print_msg)
Widget list_w;
int msg_num;
Boolean print_msg;
#endif
{
    XmString new_str;
    u_long oldflgs;
    int slot;

    if (msg_num < 0 || msg_num >= msg_cnt) {
	/* Something must have gone *very* wrong ....
	 * This is failsafe code for list widget bugs.
	 */
	gui_clear_hdrs(current_folder);
	gui_redraw_hdrs(current_folder, NULL_GRP);
	return;
    }

    current_msg = msg_num;
    if (msg_is_in_group(&current_folder->mf_hidden, current_msg)) return;
    oldflgs = msg[msg_num]->m_flags;
    if (list_w == hdr_list_w) { /* Is this ever false? */
	if ((XmStringTable)(current_folder->mf_hdr_list) != hdr_list)
	    gui_redraw_hdrs(current_folder, NULL_GRP);	/* Never happens? */
	else if (hdr_cache_msg_ct < msg_cnt)
	    gui_new_hdrs(current_folder, hdr_cache_msg_ct);
    }
    ShowCurrentHdr(list_w, print_msg);
    slot = msg_slot_no(current_folder, msg_num);
    if (print_msg && oldflgs != msg[msg_num]->m_flags) {
	if (list_w == hdr_list_w) { /* Is this ever false? */
	    hdr_list[slot] = new_str =
		XmStringRefill(hdr_list[slot], compose_hdr(msg_num), 0);
	} else
	    new_str = zmXmStr(compose_hdr(msg_num));
	XmListReplaceItemsPos(list_w, &new_str, 1, slot+1);
    }
    /* Bart: Tue Jun 21 18:53:58 PDT 1994
     * OSF says that passing "False" here should NOT cause the previous
     * selection to be cleared when list policy is EXTENDED_SELECT.  (Why?) 
     * They "fixed" this in Motif 1.2.4, which breaks the following:
     */
    XmListSelectPos(list_w, slot+1, False);
    /* So let's try passing the same truth value as "print_msg" and see
     * if that clears up the resulting stupidity.
     *
     * Bart: Tue Jun 21 19:01:49 PDT 1994
     * Answer: It does, but it causes an extra call to do_read() which
     * changes the curent folder's mf_group and then calls gui_refresh()
     * an extra time.  I'm pretty sure this is OK when we're called from
     * gui_refresh() itself, refresh_main() always runs first of anything
     * and gui_refresh() won't execute recursively; and refresh_msg() is
     * already expecting this function to cause a recursive call, though
     * I'm not exactly certain why.
    XmListSelectPos(list_w, slot+1, print_msg);
     * Unfortunately, the above still leaves several other places where
     * a list with EXTENDED_SELECT doesn't work as we need it to.  Hacked
     * the motif/xm/list.c code back to the old behavior to make it work.
     */
}

/*
 * make sure that the current message hdr selection is actually
 * shown in the hdr subwindow.  The print_msg flag is provided
 * as a shortcut for some routines to indicate that the message
 * should be displayed as well.  This function should not be called
 * if the current_msg should be changed.
 */
void
ShowCurrentHdr(list_w, print_msg)
Widget list_w;
int print_msg;
{
    int cur_slot = msg_slot_no(current_folder, current_msg);
    /* find closest non-hidden msg to the current message... */
    while (cur_slot == -1 && current_msg)
	cur_slot = msg_slot_no(current_folder, --current_msg);
    if (cur_slot == -1) cur_slot = 0;
    LIST_VIEW_POS(list_w, cur_slot+1);
    if (print_msg) {
	ask_item = tool;
	display_msg(current_msg, display_flags());
    }
}

/* select the messages in the hdr_list_w associated with the current
 * folder from the FrameData of the widget passed as 'w'.  This will
 * make the selected list reflect the Messages: field of whatever
 * w happens to be.  This may or may not be the Messages: list from
 * the hdr_list_w's FrameData, so update that list, too.
 */
void
gui_select_hdrs(w, frame)
Widget w;
ZmFrame frame;
{
    msg_group items;
    const char *v[2];
    int i, cnt, msg_no;
    FrameData *hdr_frame;
    /* msg_folder *save = current_folder;	/* XXX Necessary? */
    Widget old_ask = ask_item;

    ask_item = tool;

    FrameGet(frame,
	FrameFolder,     &current_folder,
	FrameMsgItemStr, &v[0],
	FrameEndArgs);
    v[1] = NULL;
    init_msg_group(&items, msg_cnt, 1);
    clear_msg_group(&items);
    /* Could change this to str_to_list() 			XXX */
    /* XXX casting away const */
    i = get_msg_list((char **) v, &items);
    /* current_folder = save;			/* XXX Necessary? */

    /* Check against the cache for a possible need to redraw */
    if ((XmStringTable)(current_folder->mf_hdr_list) != hdr_list)
	gui_redraw_hdrs(current_folder, NULL_GRP); /* No recursion */

    if (set_hidden(current_folder, (int *) 0))
	gui_redraw_hdrs(current_folder, NULL_GRP);
    if (i == -1) {
	ask_item = old_ask;
	destroy_msg_group(&items);
	return;
    }
    msg_no = -1;
    for (i = cnt = 0; i < msg_cnt; i++)
	if (msg_is_in_group(&items, i) &&
	    !msg_is_in_group(&current_folder->mf_hidden, i))
	  msg_no = i, cnt++;
    if (cnt == 1)
	SetCurrentMsg(hdr_list_w, msg_no, False);
    else if (cnt) {
#ifndef SELECT_POS_LIST
	XmStringTable redo_list =
	    (XmString *)XtMalloc((cnt+1)*sizeof(XmString));
	for (i = cnt = 0; i < msg_cnt; i++)
	    if (msg_is_in_group(&items, i))
		redo_list[cnt++] = XmStringCreateSimple(compose_hdr(i));
	redo_list[cnt] = NULL_XmStr;
	XtVaSetValues(hdr_list_w,
	    XmNselectedItems,     redo_list,
	    XmNselectedItemCount, cnt,
	    NULL);
	XmStringFreeTable(redo_list);
#else /* SELECT_POS_LIST */
	int *redo_list = (int *)XtMalloc(cnt*sizeof(int));
	for (i = cnt = 0; i < msg_cnt; i++)
	    if (msg_is_in_group(&items, i) &&
		!msg_is_in_group(&current_folder->mf_hidden, i))
	      redo_list[cnt++] = msg_slot_no(current_folder, i)+1;
	XmListSelectPositions(hdr_list_w, redo_list, cnt, False);
	XtFree((char *)redo_list);
#endif /* SELECT_POS_LIST */
    }
    FrameSet(frame, FrameMsgList, &items, NULL);
    if (w != hdr_list_w && frame != (hdr_frame = FrameGetData(hdr_list_w)))
	FrameSet(hdr_frame, FrameMsgList, &items, NULL);
    /* The above leaves bits set in current_folder->mf_group, which is
     * generally undesirable -- that group should refer only to messages
     * that should be redrawn by the next gui_refresh().  We want to set
     * the message strings from a list here, but not set the message list.
     * Unfortunately, cleaning this up from here is hard at the moment.
     */
    ask_item = old_ask;

    destroy_msg_group(&items);
}

/* Clear the header display */
void
gui_clear_hdrs(fldr)
msg_folder *fldr;
{
    if (istool > 1 && fldr == current_folder) {
	XtVaSetValues(hdr_list_w,
	    XmNitems, NULL,
	    XmNitemCount, 0,
	    XmNselectedItems, NULL,
	    XmNselectedItemCount, 0,
	    NULL);
	XmStringFreeTable((XmStringTable)(current_folder->mf_hdr_list));
	hdr_list = 0;
	hdr_cache_len = hdr_cache_msg_ct = 0;
	current_folder->mf_hdr_list = 0;
	current_folder->mf_hdr_list_ct = 0;
    }
}

/* Redraw the entire header list.  If list is not NULL_GRP, selects the
 * messages whose bits are set in list.  Headers and selections are done
 * simultaneously for performance reasons.
 *
 * The list of most recently redrawn headers is cached in the folder
 * structure and its pointer is duplicated in the global hdr_list.
 */
void
gui_redraw_hdrs(fldr, list)
msg_folder *fldr;
msg_group *list;
{
    int pageful;
#ifndef SELECT_POS_LIST
    int i, j;
    XmStringTable sel_list = (XmStringTable)0;
#else /* SELECT_POS_LIST */
    int i, j, *sel_list = (int *)0;
#endif /* SELECT_POS_LIST */

    if (istool <= 1 || fldr != current_folder)
	return;

    /* If we're told to redraw what we last redrew, flush and refill the
     * cache, otherwise use the cached headers from the appropriate folder.
     */
    if ((XmStringTable)current_folder->mf_hdr_list &&
	    (XmStringTable)current_folder->mf_hdr_list != hdr_list) {
	hdr_list = (XmStringTable)current_folder->mf_hdr_list;
	/* XXX casting away const */
	hdr_cache_len = vlen((char **) hdr_list);
	hdr_cache_msg_ct = current_folder->mf_hdr_list_ct;
	XtVaSetValues(hdr_list_w,
	    XmNitems,             hdr_list,
	    XmNitemCount,         hdr_cache_len,
	    NULL);
	if (hdr_cache_msg_ct < msg_cnt)
	    gui_new_hdrs(current_folder, hdr_cache_msg_ct);
	if (list)
	    gui_select_hdrs(hdr_list_w, FrameGetData(hdr_list_w));
	return;
    } else {
	/* Bart: Sat Jun 13 13:21:00 PDT 1992
	 * Flush the cache and the hdr_list_w to prevent reselection
	 * of any currently selected messages during reset of the
	 * list items.  Yes, Motif is stupid that way.
	 */
	gui_clear_hdrs(current_folder);
    }

    if (list) {
	for (i = j = 0; j < msg_cnt; j++)
	    if (msg_is_in_group(list, j))
		i++;
	if (i)
#ifndef SELECT_POS_LIST
	    sel_list = (XmString *)
			XtMalloc((unsigned)((i+1) * sizeof(XmString)));
#else /* SELECT_POS_LIST */
	    sel_list = (int *)XtMalloc(i*sizeof(int));
#endif /* SELECT_POS_LIST */
    }
    init_nointr_mnr(zmVaStr(catgets( catalog, CAT_MOTIF, 46, "Listing %d message summaries..." ), msg_cnt),
	INTR_VAL(msg_cnt));
    hdr_list = (XmString *)
		XtMalloc((unsigned)((msg_cnt+1) * sizeof(XmString)));
    hdr_cache_msg_ct = msg_cnt;
    if (!cache_format || strcmp(cache_format, hdr_format) != 0) {
	xfree(cache_format);
	cache_format = savestr(hdr_format);
    }
    set_hidden(current_folder, (int *) 0);
    if (fldr->mf_pick_list)
	fldr->mf_pick_list = (int *)
	    realloc(fldr->mf_pick_list, (msg_cnt+1)*sizeof(int));
    else
	fldr->mf_pick_list = (int *)
	    malloc((msg_cnt+1)*sizeof(int));
    if (fldr->mf_msg_slots)
	fldr->mf_msg_slots = (int *)
	    realloc(fldr->mf_msg_slots, (msg_cnt+1)*sizeof(int));
    else
	fldr->mf_msg_slots = (int *)
	    malloc((msg_cnt+1)*sizeof(int));
    check_nointr_mnr(catgets( catalog, CAT_MOTIF, 47, "Redrawing ..." ), 0);
    for (i = j = pageful = 0; pageful < msg_cnt; pageful++) {
	int slot;

	if (msg_is_in_group(&fldr->mf_hidden, pageful)) {
	    fldr->mf_msg_slots[pageful] = -1;
	    continue;
	}
	if (hdr_reverse_list)
	    slot = msg_cnt - (++i);
	else
	    slot = i++;
	fldr->mf_msg_slots[pageful] = slot;
	fldr->mf_pick_list[slot] = pageful;
	hdr_list[slot] = XmStringCreateSimple(compose_hdr(pageful));
	if (sel_list && msg_is_in_group(list, pageful))
#ifndef SELECT_POS_LIST
	    sel_list[j++] = XmStringCopy(hdr_list[slot]);
#else /* SELECT_POS_LIST */
	    sel_list[j++] = slot + 1;
#endif /* SELECT_POS_LIST */
	/* The only point in doing this is to pop up or down the
	 * task meter if the state of the parent window changes.
	 */
	if (msg_cnt > 10 && !(pageful % (msg_cnt/10)))
	    check_nointr_mnr(catgets( catalog, CAT_MOTIF, 48, "Redrawing ..." ), (int)(pageful*100/msg_cnt));
    }
    fldr->mf_msg_slots[pageful] = i;
    end_intr_mnr(catgets( catalog, CAT_SHELL, 119, "Done." ), 100);
    hdr_list[i] = NULL_XmStr;
#ifndef SELECT_POS_LIST
    if (sel_list)
	sel_list[j] = NULL_XmStr;
#endif /* SELECT_POS_LIST */
    XtVaSetValues(hdr_list_w,
	XmNitems,             hdr_list,
	XmNitemCount,         i,
#ifndef SELECT_POS_LIST
	XmNselectedItems,     sel_list,
	XmNselectedItemCount, j,
#endif /* SELECT_POS_LIST */
	NULL);
    current_folder->mf_hdr_list = (GuiItem)hdr_list;
    current_folder->mf_hdr_list_ct = msg_cnt;
    hdr_cache_len = pageful;
#ifndef SELECT_POS_LIST
    XmStringFreeTable(sel_list);
#else /* SELECT_POS_LIST */
    XmListSelectPositions(hdr_list_w, sel_list, j, False);
    XtFree((char *)sel_list);
#endif /* SELECT_POS_LIST */
}

/* Redraw exactly the messages indicated in list, then re-select whatever
 * messages were previously selected.
 */
void
gui_redraw_hdr_items(list_w, list, select_flag)
Widget list_w;
msg_group *list;
int select_flag;	/* Bart: Fri Aug  7 20:18:48 PDT 1992 */
{
    XmString *strs = 0;
    register int i, cnt;
    int *sel_pos, sel_cnt, slot;
    char *save_format = hdr_format;
    int touchlen = 0;

    if (list_w == hdr_list_w) { /* Is this ever false? */
	if (!hdr_list ||
		hdr_list != (XmStringTable)(current_folder->mf_hdr_list))
	    gui_redraw_hdrs(current_folder, NULL_GRP);	/* Never happens? */
	else if (hdr_list && hdr_cache_msg_ct < msg_cnt)
	    gui_new_hdrs(current_folder, hdr_cache_msg_ct);/* Never happens? */
    }

    /* Bart: Tue Jun 30 13:16:12 PDT 1992
     * Use of msg_cnt here in place of list->mg_count is not strictly
     * correct.  However, we shouldn't be calling here unless we're
     * redrawing messages from the current folder, and the mg_count
     * of most msg_group structures is not properly initialized.
     * This was the only place in the entire source that referred to
     * the mg_count field directly, everyone else uses either the
     * folder's mf_count or just msg_cnt, so this might as well, too.
     */
    strs = (XmStringTable)XtMalloc((msg_cnt+1)*sizeof(XmString));

    update_hidden(current_folder->mf_count);

    /* Bart: Fri Aug  7 20:19:11 PDT 1992
     * This code makes the function match its description (above).
     * However, some callers don't want the described behavior,
     * so we make it depend on a boolean parameter ...
     */
    if (select_flag)
	select_flag = XmListGetSelectedPos(list_w, &sel_pos, &sel_cnt);

    XtVaSetValues(list_w,
	XmNselectedItems, 0,
	XmNselectedItemCount, 0,
	NULL);

    /* init_nointr_msg("Redrawing message summaries", 0); */
    i = count_msg_list(list);
    init_nointr_mnr(zmVaStr(catgets( catalog, CAT_MOTIF, 50, "Redrawing %d message summaries" ), i), INTR_VAL(i));

    if (!cache_format || strcmp(hdr_format, cache_format) != 0) {
	xfree(cache_format);
	cache_format = savestr(hdr_format);
    } else if (list_w == hdr_list_w && lang_En_US) {
	hdr_format = "";
	touchlen = COMPOSE_HDR_PREFIX_LEN;
    }

    for (i = cnt = slot = 0; i <= msg_cnt; i++) {
	if (i < msg_cnt &&
	    msg_is_in_group(&current_folder->mf_hidden, i)) continue;
	if (current_folder->mf_msg_slots)
	    slot = msg_slot_no(current_folder, i);
	if (i < msg_cnt && msg_is_in_group(list, i)) {
	    if (!(i*100/msg_cnt) % 300)
		check_nointr_mnr(catgets( catalog, CAT_MOTIF, 51, "Redrawing ..." ), i*100/msg_cnt);
	    if (list_w == hdr_list_w) { /* Is this ever false? */
		if (touchlen && hdr_list[slot] == 0) {
		    /* This happens sometimes, but I don't know why. */
		    hdr_format = save_format;
		}
		hdr_list[slot] = strs[cnt] =
		    XmStringRefill(hdr_list[slot], compose_hdr(i), touchlen);
	    } else
		strs[cnt] = XmStringCreateSimple(compose_hdr(i));
	    cnt++;
	} else if (cnt) {
	    XmListReplaceItemsPos(list_w, strs, cnt, slot - cnt + 1);
	    if (list_w != hdr_list_w)
	    while (cnt)
		XmStringFree(strs[--cnt]);
	    else
		cnt = 0;
	}
    }
    hdr_format = save_format;

    if (select_flag) {
#ifdef SELECT_POS_LIST
	XmListSelectPositions(hdr_list_w, sel_pos, sel_cnt, False);
#else /* SELECT_POS_LIST */
	/* can't select multiple items if selectionPolicy is extended select
	 * pf Tue Aug 17 08:16:21 1993
	 */
	XtVaSetValues(hdr_list_w, XmNselectionPolicy, XmMULTIPLE_SELECT, NULL);
	for (i = 0; i < sel_cnt; i++)
	    XmListSelectPos(list_w, sel_pos[i], False);
	XtVaSetValues(hdr_list_w, XmNselectionPolicy, XmEXTENDED_SELECT, NULL);
#endif /* SELECT_POS_LIST */
	XtFree((char *)sel_pos);
    }

    end_intr_mnr(catgets( catalog, CAT_MOTIF, 52, "Done, repainting" ), 100);
    XtFree((char *)strs);
}

void
gui_update_cache(fldr, list)
msg_folder *fldr;
msg_group *list;
{
    register int i, cnt, slot;
    char *save_format = hdr_format;
    int touchlen;

    if (!fldr->mf_hdr_list)
	return;
    if (istool != 2)
	return;

    if (!cache_format || strcmp(hdr_format, cache_format) != 0)
	touchlen = 0;
    else if (lang_En_US) {
	hdr_format = "";
	touchlen = COMPOSE_HDR_PREFIX_LEN;
    } /* else XmStringRefill ignores the value */
    for (i = cnt = 0; i < fldr->mf_count; i++) {
	slot = msg_slot_no(fldr, i);
	if (slot < 0)	/* Summary is hidden */
	    continue;
	if (msg_is_in_group(list, i)) {
	    ((XmStringTable)(fldr->mf_hdr_list))[slot] =
		XmStringRefill(((XmStringTable)(fldr->mf_hdr_list))[slot],
				compose_hdr(i), touchlen);
	}
    }
    hdr_format = save_format;
}

/* Append new headers to the header list. */
void
gui_new_hdrs(fldr, old_cnt)
msg_folder *fldr;
int old_cnt;
{
    int i = 0, j, slot;
    XmStringTable strs;

    if (fldr != current_folder)
	return;

    if (!hdr_list ||
	    hdr_list != (XmStringTable)(current_folder->mf_hdr_list)) {
	gui_redraw_hdrs(current_folder, NULL_GRP);
	return;	/* gui_redraw_hdrs() will have called us recursively */
    }

    update_hidden(old_cnt);
    /* if update_hidden redrew the whole list, just return. */
    if (hdr_cache_msg_ct == msg_cnt) return;
    
    hdr_list = (XmStringTable)XtMalloc(sizeof(XmString)*(msg_cnt+1));
    for (i = j = 0; i < old_cnt; i++) {
	if (msg_is_in_group(&fldr->mf_hidden, i)) continue;
	if (current_folder->mf_hdr_list)
	    hdr_list[j] = ((XmStringTable)(current_folder->mf_hdr_list))[j];
	j++;
    }
    if (current_folder->mf_hdr_list)
	XtFree((char *)current_folder->mf_hdr_list);
    current_folder->mf_hdr_list = (GuiItem)hdr_list;
    current_folder->mf_hdr_list_ct = hdr_cache_msg_ct = msg_cnt;
    hdr_cache_len = j;
    strs = &hdr_list[j];
    if (fldr->mf_pick_list)
	fldr->mf_pick_list = (int *)
	    realloc(fldr->mf_pick_list, (msg_cnt+1)*sizeof(int));
    else
	fldr->mf_pick_list = (int *) malloc((msg_cnt+1)*sizeof(int));
    if (fldr->mf_msg_slots)
	fldr->mf_msg_slots = (int *)
	    realloc(fldr->mf_msg_slots, (msg_cnt+1)*sizeof(int));
    else
	fldr->mf_msg_slots = (int *) malloc((msg_cnt+1)*sizeof(int));
    init_nointr_mnr(catgets( catalog, CAT_MOTIF, 53, "Adding new message items" ), INTR_VAL(msg_cnt-old_cnt));
    slot = (old_cnt) ? msg_slot_no(current_folder, old_cnt) : 0;
    ZASSERT(slot <= old_cnt);
    for (i = 0, j = old_cnt; j < msg_cnt; j++) {
	if (msg_is_in_group(&fldr->mf_hidden, j)) {
	    fldr->mf_msg_slots[j] = -1;
	    continue;
	}
	fldr->mf_pick_list[slot] = i;
	fldr->mf_msg_slots[j] = slot++;
	check_intr_mnr(zmVaStr(catgets( catalog, CAT_MOTIF, 54, "Drawing %d" ), i+1), i*100/(msg_cnt-old_cnt));
	strs[i++] = XmStringCreateSimple(compose_hdr(j));
    }
    fldr->mf_msg_slots[j] = slot;
    end_intr_mnr(catgets( catalog, CAT_SHELL, 119, "Done." ), 100);
    strs[i] = NULL_XmStr;
    XmListAddItems(hdr_list_w, strs, i, 0);
    if (boolean_val(VarNewmailScroll)) {
	i = current_msg, current_msg = msg_cnt - 1;
	ShowCurrentHdr(hdr_list_w, False);
	current_msg = i;
    }
}

/* Optimize update of an XmString in the hdr_list by scribbling right into
 * the text part of the compound object.
 */
static XmString
XmStringRefill(xmstr, new_text, touchlen)
XmString xmstr;
char *new_text;
int touchlen;
{
#if XmVersion <= 1001
    /* Bart: Mon Dec 14 17:49:46 PST 1992 -- workaround for LANG bug */
    static char *lang;

    char *str;
    int len;

    if (xmstr) {
	/* XmString Format: header tag clen charset tag tlen text */
	str = (char *)xmstr   + 4  + 1  + 2   + 9   + 1  + 2;
    }

    /* Bart: Mon Dec 14 17:49:46 PST 1992 -- workaround for LANG bug.
     * This is less than wonderful because it doesn't catch dynamic
     * changes in $LANG, but anybody insane enough to do that deserves
     * whatever bizarrity he gets.
     *
     * Bart: Fri Jan 15 17:26:47 PST 1993 -- had to make lang_En_US
     * static to the entire file in order to pass the correct string
     * to XmStringRefill() from the cache routines above.
     *
     * Bart: Wed Dec 15 19:01:21 PST 1993 -- had to make lang_En_US
     * apply only to the "C" locale, because IBM changed their libs.
     */
    if (!lang) {
	lang = savestr(getenv("LANG"));	/* Deals with NULL */
	if (lang[0])
	    lang_En_US = (strcmp(lang, "C") == 0);
	else
	    lang_En_US = 1;
    }
    if (!lang_En_US || !xmstr) {
#endif /* XmVersion <= 1001 */
	if (xmstr)
	    XmStringFree(xmstr);
	return XmStringCreateSimple(new_text);
#if XmVersion <= 1001
    }

    /* Strings longer than ??? chars use an extra 2 bytes of header */
    if (xmstr[3] & 0x80)	/* Magic from XmString.c */
	str += 2;

    /* This may not work; str isn't explicitly nul-terminated by Xm ...
     * but the odds we're wrong and also exactly the same length as
     * new_text are pretty small.  Urk ...
    len = strlen(str);
     */
    /* This next line makes "purify" happy */
    len = XmStringLength(xmstr) - (str - (char *)xmstr);

    /* XXX THIS ALL HAS TO CHANGE FOR FOREIGN LANGUAGES OR MOTIF 1.2 */
    /* XXX THIS MAY ONLY BE USED FOR SIMPLE STRINGS!! */
    /* XXX NO NEWLINES!  NO FONT CHANGES!  NO FUNKY CHARSETS! */
    if (touchlen > len || touchlen == 0 && strlen(new_text) != len) {
	XmStringFree(xmstr);
	return XmStringCreateSimple(new_text);
    }
    if (touchlen)
	(void) strncpy(str, new_text, touchlen);
    else
	(void) strncpy(str, new_text, strlen(new_text));

    return xmstr;
#endif /* XmVersion <= 1001 */
}

/* update the header list to reflect changes in messages' hidden status */

static void
update_hidden(total)
{
    int i, slot, cnt, oldslot, j, reveal_cnt, redrawn_ct = 0;
    int redisp = FALSE;

    if (!total) return;
    if (reveal_cnt = set_hidden(current_folder, (int *) 0))
	redisp = TRUE;
    if (!hdr_list) return;
    init_nointr_mnr(zmVaStr(
	catgets( catalog, CAT_MOTIF, 46, "Listing %d message summaries..." ),
	reveal_cnt), INTR_VAL(reveal_cnt));
    for (i = slot = cnt = oldslot = 0; i < total;
	 i++, oldslot++) {
	hdr_list[slot] = hdr_list[oldslot];
	if (!msg_is_in_group(&current_folder->mf_hidden, i)) {
	    if (msg_slot_no(current_folder, i) == -1) {
		for (j = total-1; j >= slot; j--)
		    hdr_list[j+1] = hdr_list[j];
		hdr_list[slot] = XmStringCreateSimple(compose_hdr(i));
		redrawn_ct++;
		if (total > 10 && !(redrawn_ct % (total/10)))
		    check_nointr_mnr(
			catgets( catalog, CAT_MOTIF, 48,
			    "Redrawing ..." ),
			(int)(redrawn_ct*100/total));
	    }
	    picked_msg_no(current_folder, slot) = i;
	    msg_slot_no(current_folder, i) = slot++;
	} else if (msg_slot_no(current_folder, i) != -1) {
	    redisp = TRUE;
/*	    strs[cnt++] = hdr_list[msg_slot_no(current_folder, i)];*/
	    msg_slot_no(current_folder, i) = -1;
	} else
	    oldslot--;
    }
    msg_slot_no(current_folder, i) = slot;
    hdr_list[slot] = NULL_XmStr;
    if (redisp)
	XtVaSetValues(hdr_list_w,
		      XmNitems,             hdr_list,
		      XmNitemCount,         slot,
		      NULL);
    end_intr_mnr(catgets( catalog, CAT_SHELL, 119, "Done." ), 100);
}

void
gui_flush_hdr_cache(fldr)
msg_folder *fldr;
{
    if (istool == 2)
	STR_TABLE_FREE((XmStringTable)fldr->mf_hdr_list);
    fldr->mf_hdr_list = 0;
}
