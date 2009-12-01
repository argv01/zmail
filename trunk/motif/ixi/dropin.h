/*
 *  __ ___ ___  Copyright (C) 1988-1991 IXI Limited.
 *  \ /  //  /
 *   /  //  /   IXI Limited, 62-74 Burleigh Street,
 *  /__//__/_\  Cambridge, CB1 1OJ, United Kingdom
 *
 *  Description: Drop_in_out handler definitions
 */

#ifndef INCLUDED_DROPIN_H
#define INCLUDED_DROPIN_H

#ifndef lint
static char sccs_id_dropin_h [] = "@(#) with dropin.h dropin.h 7.1.3.1";
#endif

/* Mouse button definitions
 */
#define IXI_BUTTON1		0x00000100
#define IXI_BUTTON2		0x00000200
#define IXI_BUTTON3		0x00000400
#define IXI_BUTTON4		0x00000800
#define IXI_BUTTON5		0x00001000
#define IXI_SHIFT		0x00000001
#define IXI_LOCK 		0x00000002
#define IXI_CONTROL		0x00000004
#define IXI_MOD1		0x00000008
#define IXI_MOD2		0x00000010
#define IXI_MOD3		0x00000020
#define IXI_MOD4		0x00000040
#define IXI_MOD5		0x00000080
#define NULL_MASK		0x00000000


/* This structure is used for breaking down dropped in data.
 */
typedef struct ixi_ArgvList
{
    int argc;
    char **argv;
} ArgvList;

/* This is the data passed to a dropout call, if a dropout is valid
 * the function is called to determine what data to drop out.
 * Note, the dropin library does NOT take a local copy of the passed
 * on structure.
 */
typedef void (*FetchDataProc) ();
typedef struct ixi_DropOutDataFetch
{
    FetchDataProc fetch_data;
    caddr_t client_data;
} DropOutDataFetch;

/* This is the return data that needs to be supplied by the get
 * drop out data function (detailed above), default values are
 * type   = XA_STRING
 * format = 8
 * size   = 0
 * addr   = NULL
 */
typedef struct ixi_DropOutData
{
    Atom type;
    int format;
    int size;
    caddr_t addr;
    Boolean delete;
} DropOutData;

/* This is the data passed to a dropin callback via call_data.
 * The structure itself is in static memory and will be overwritten
 * by future calls.
 * The addr data is malloc's by Xlib and the recipient is responsible
 * for freeing the addr via XFree.
 */
typedef struct ixi_DropInData
{
    Atom type;
    int x, y;
    int state;
    int time;
    Boolean more;
    Boolean same_screen;
    int size;
    caddr_t addr;
} DropInData;

/* This is passed on as client data if using action
 * and translation tables.
 */
typedef struct ixi_DropInOutActionData
{
    XEvent *event;
    String *params;
    Cardinal *num_params;
    caddr_t client_data;
} DropInOutActionData;


/* Externs. */

extern Atom host_filelist_type;

extern char *set_host_and_file_names();
extern char **get_host_and_file_names();
extern void free_host_and_file_names();

#endif
