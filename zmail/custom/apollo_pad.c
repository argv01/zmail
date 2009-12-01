/*
 * apollo_pad.c: 10/24/1991 version 1.2
 *
 * apollo_ispad() - returns true if output is going into an Display Manger
 * 			pad.  Needed because "more" won't work right in a pad.
 *
 * apollo_padheight() - Returns the height in characters of the pad associated
 *			with stdout.  If an error occurs or the stream isn't
 *			a pad, a default value of 20 is returned.
 *
 *	Written by Mike Pelletier based heavily
 *	on work by Paul Killey.
 */

#ifdef apollo

#ifdef __STDC__
#include <apollo/base.h>
#include <apollo/pad.h>
#include <apollo/ios.h>
#else
#include "/sys/ins/base.ins.c"
#include "/sys/ins/pad.ins.c"
#include "/sys/ins/ios.ins.c"
#endif /* __STDC__ */

#ifdef __STDC__
extern status_$t errno_$status;
#else
extern  status_$t unix_fio_$status;
#define errno_$status unix_fio_$status
#endif

/*
 *	Return true if we're outputting into an Apollo DM pad...
 */

apollo_ispad() {

    short disunit = 1;
    pad_$display_type_t distype;
    status_$t status;

#ifdef __STDC__
    pad_$inq_disp_type(ios_$stdout, &distype, &disunit, &status);
#else
    pad_$inq_disp_type(ios_$stdout, distype, disunit, status);
#endif /* __STDC__ */

    return (status.all == status_$ok);
}


/*
 * Returns the height of the pad associated with stdout
 */

apollo_padheight() {

    static short FontHeight, FontWidth, FontNameLength;
    static int height;
    status_$t Status;
    char FontName[256];
    short WindowCount;

#ifdef __STDC__
    pad_$window_list_t CurrentWindow;
#else
    pad_$window_desc_t CurrentWindow;
#endif /* __STDC__ */

#ifdef __STDC__
    pad_$inq_font(ios_$stdout, &FontWidth, &FontHeight, FontName, 256,
			&FontNameLength, &Status);
#else
    pad_$inq_font(ios_$stdout, FontWidth, FontHeight, FontName, 256,
			FontNameLength, Status);
#endif /* __STDC__ */

    if (Status.all == status_$ok) {
#ifdef __STDC__
	pad_$set_scale(ios_$stdout, 1, 1, &Status);
	pad_$inq_windows(ios_$stdout, CurrentWindow, 1, &WindowCount, &Status);
	height = CurrentWindow[0].height / FontHeight;
#else
        pad_$set_scale(ios_$stdout, 1, 1, Status);
        pad_$inq_windows(ios_$stdout, CurrentWindow, 1, WindowCount, Status);
        height = CurrentWindow.height / FontHeight;
#endif /* __STDC__ */
    } else {
	height = 20;
    }
}

#endif /* apollo */
