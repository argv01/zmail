/* 
 * $RCSfile: zync_extras.c,v $
 * $Revision: 1.24 $
 * $Date: 2005/05/09 09:15:25 $
 * $Author: syd $
 */

#include "popper.h"
#include <stdio.h>
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif /* HAVE_MEMORY_H */
#include <dynstr.h>
#include <dpipe.h>
#include <dputil.h>
#include <bfuncs.h>
#include <mstore/message.h>
#include <mstore/mime-api.h>
#include <strcase.h>
#include <msgs/encode/base64.h>

static const char zync_extras_rcsid[] =
    "$Id: zync_extras.c,v 1.24 2005/05/09 09:15:25 syd Exp $";

#undef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

/**********************************************************************\
**	      BEGIN SELECTED EXCERPTS FROM MSGS/DATES.C               **
\**********************************************************************/

static const char *day_names[] = {
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat"
};

static const char *month_names[] = {
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
};

/* time_str() returns a string according to criteria:
 *   if "now" is 0, then the current time is gotten and used.
 *       else, use the time described by now
 *   opts points to a string of args which is parsed until an unknown
 *       arg is found and opts will point to that upon return.
 *   valid args are T (time of day), D (day of week), M (month), Y (year),
 *       N (number of day in month -- couldn't think of a better letter).
 */
static char *
time_str(opts, now)
    const char *opts;
    time_t now;
{
    static char time_buf[30];
    struct tm *T;
    register char *p = time_buf;
    time_t x;

    if (!opts)
	return NULL;
    if (now)
	x = now;
    else
	(void) time(&x);
    T = localtime(&x);
    for (;; opts++) {
	switch(*opts) {
	  case 't':
	    (void) sprintf(p, "%02d%02d", T->tm_hour, T->tm_min);
	    break;
	  case 'T':
	    (void) sprintf(p, "%d:%02d%s",
			   ((T->tm_hour) ? ((T->tm_hour <= 12) ?
					    T->tm_hour :
					    T->tm_hour - 12) :
			    12),
			   T->tm_min,
			   ((T->tm_hour < 12) ? "am" : "pm"));
	    break;
	  case 'D':
	  case 'W':
	    (void) strcpy(p, day_names[T->tm_wday]);
	    break;
	  case 'm':
	    (void) sprintf(p, "%02d", T->tm_mon + 1);
	    break;
	  case 'M':
	    (void) strcpy(p, month_names[T->tm_mon]);
	    break;
	  case 'y':
	    (void) sprintf(p, "%02d", T->tm_year % 100);
	    break;
	  case 'Y':
	    (void) sprintf(p, "%d", T->tm_year + 1900);
	    break;
	  case 'n':
	  case 'd':
	    (void) sprintf(p, "%02d", T->tm_mday);
	    break;
	  case 'N':
	    (void) sprintf(p, "%d", T->tm_mday);
	    break;
	  default:
	    *p = 0;
	    return (time_buf);
	}
	p += strlen(p);
    }
}

static int mtbl[] = { 0,31,59,90,120,151,181,212,243,273,304,334 };

static int
day_number(Year, Month, Day)
    int Year;			/* Years since 1900 */
    int Month;			/* Month (1 thru 12) */
    int Day;			/* Day (1 thru 31) */
{
    /* Lots of foolishness with casts for Xenix-286 16-bit ints */

    time_t days_ctr;	/* 16-bit ints overflowed Sept 12, 1989 */

    days_ctr = ((time_t)Year * 365L) + ((Year + 3) / 4);
    days_ctr += mtbl[Month-1] + Day + 6;
    if (Month > 2 && (Year % 4 == 0))
	days_ctr++;
    return (int)(days_ctr % 7L);
}

static struct tm *
time_n_zone(zone)
    char *zone;
{
    struct tm *T;
    char *tz;
#if defined(HAVE_TZNAME) || defined(HAVE_TM_ZONE) || defined(TIMEZONE)
    time_t	  x;	/* Greg: 3/11/93.  Was type long */

    (void) time(&x);
    T = localtime(&x);
# ifdef TIMEZONE
#  ifdef DAYLITETZ
    if (T->tm_isdst)
	tz = DAYLITETZ;
    else
#  endif /* DAYLITETZ */
	tz = TIMEZONE;
# else /* !TIMEZONE */
#  ifdef HAVE_TM_ZONE
    tz = T->tm_zone;
#  else /* !HAVE_TM_ZONE */
#   ifdef HAVE_TZNAME
    {
#    ifdef DECLARE_TZNAME
	extern char *tzname[];
#    endif /* DECLARE_TZNAME */
	tz = tzname[T->tm_isdst];
    }
#   endif /* HAVE_TZNAME */
#  endif /* HAVE_TM_ZONE */
# endif /* TIMEZONE */
#else /* !HAVE_TZNAME && !HAVE_TM_ZONE && !TIMEZONE */
# ifdef HAVE_TIMEZONE
    extern char     *timezone();
    struct timeval  mytime;
    struct timezone myzone;

    (void) gettimeofday(&mytime, &myzone);
    T = localtime(&mytime.tv_sec);
    tz = timezone(myzone.tz_minuteswest, (T->tm_isdst && myzone.tz_dsttime));
# else /* !HAVE_TIMEZONE */
#  if 0				/* what to do? */
    error(ZmErrFatal, catgets( catalog, CAT_MSGS, 244, "There's no way to get your timezone!\n" ));
#  endif
# endif /* HAVE_TIMEZONE */
#endif /* HAVE_TZNAME || TIMEZONE */

    (void) strncpy(zone, tz, 7), zone[7] = 0;
    return T;
}

/*
 *   %ld%3c%s	gmt_in_secs weekday orig_timezone
 * The standard "date format" stored in the msg data structure.
 */

/* Time Zone Stuff */
static struct zoneoff {
    char *zname;
    int hr_off;
    int mn_off;
} time_zones[] = {
    /* Universal Time */
    { "UT",	  0,  0 },	{ "GMT",	  0,  0 },
    { "UTC",	  0,  0 },
    /* European Time */
    { "BST",	  1,  0 },				    /* Brit. Summer */
    { "EET",	  2,  0 },	{ "EEST",	  3,  0 },	/* Eastern */
    				{ "EET DST",	  3,  0 },
    { "MET",	  1,  0 },	{ "MEST",	  2,  0 },	/* Middle */
    				{ "MET DST",	  2,  0 },
    { "WET",	  0,  0 },	{ "WEST",	  1,  0 },	/* Western */
    				{ "WET DST",	  1,  0 },
    /* North American Time */
    { "NST",	 -3,-30 },				    /* Newfoundland */
    { "AST",	 -4,  0 },	{ "ADT",	 -3,  0 },	/* Atlantic */
    { "EST",	 -5,  0 },	{ "EDT",	 -4,  0 },	/* Eastern */
    { "CST",	 -6,  0 },	{ "CDT",	 -5,  0 },	/* Central */
    { "MST",	 -7,  0 },	{ "MDT",	 -6,  0 },	/* Mountain */
    { "PST",	 -8,  0 },	{ "PDT",	 -7,  0 },	/* Pacific */
    { "YST",	 -9,  0 },	{ "YDT",	 -8,  0 },	/* Yukon */
    { "HST",	-10,  0 },	{ "HDT",	 -9,  0 },	/* Hawaii */
    /* Japan and Australia Time */
    {"JST",	  9,  0 },					/* Japan */
    {"AEST",	 10,  0 },	{"AESST",	 11,  0 },	/* Eastern */	
    {"ACST",	  9, 30 },	{"ACSST",	 10, 30 },	/* Central */
    {"AWST",	  8,  0 },					/* Western */
    /* Military Time */
    { "A",	  1,  0 },	{ "N",		 -1,  0 },
    { "B",	  2,  0 },	{ "O",		 -2,  0 },
    { "C",	  3,  0 },	{ "P",		 -3,  0 },
    { "D",	  4,  0 },	{ "Q",		 -4,  0 },
    { "E",	  5,  0 },	{ "R",		 -5,  0 },
    { "F",	  6,  0 },	{ "S",		 -6,  0 },
    { "G",	  7,  0 },	{ "T",		 -7,  0 },
    { "H",	  8,  0 },	{ "U",		 -8,  0 },
    { "I",	  9,  0 },	{ "V",		 -9,  0 },
    { "K",	 10,  0 },	{ "W",		-10,  0 },
    { "L",	 11,  0 },	{ "X",		-11,  0 },
    { "M",	 12,  0 },	{ "Y",		-12,  0 },
    { "Z",	  0,  0 },
    /* Also legal is +/- followed by hhmm offset from UT */
    { 0, 0, 0 }
};

static time_t
getzoff(zone)
    char *zone;
{
    struct zoneoff *z;
    int toff;
    char sign[2];

    if (!zone || !*zone)
	return 0;
    if (sscanf(zone, "%1[-+]%d", sign, &toff) == 2)
	return ((toff / 100) * 3600 + (toff % 100) * 60)
	  * (*sign == '-' ? -1 : 1);
    for (z = time_zones; z->zname; z++)
	if (ci_strcmp(zone, z->zname) == 0)
	    return z->hr_off * 3600 + z->mn_off * 60;
    return 0;
}

/*
 * Kind of the reverse of localtime() and gmtime() -- converts a struct tm
 * to time in seconds since 1970.  Valid until 2038.
 * If the "zone" argument is present, it modifies the return value.
 * The zone should be a string, either +/-hhmm or symbolic (above).
 * The "how" argument should be -1 to convert FROM gmt, 1 to convert TO gmt,
 * and (as a "side-effect") 0 if the Zone parameter is to be ignored.
 *
 */
static time_t
time2gmt(tym, zone, how)
    struct tm *tym;
    char *zone;
    int how;
{
    int year;
    time_t julian;

    if (tym->tm_year < 200) {
	year = tym->tm_year + 1900;
	if (year < 1970)
	    year += 100;	/* Assume year > 2000 ? */
    } else {
	year = tym->tm_year + 1900;
    }

    julian = 365 * (year - 1970) + (int)((year - 1970 + 1) / 4) +
		mtbl[tym->tm_mon] + tym->tm_mday - 1;
		/* tym->tm_yday might not be valid */
    if (tym->tm_mon > 1 && year%4 == 0 && (year%100 != 0 || year%400 == 0))
	julian++;
    julian *= 86400;	/* convert to seconds */
    julian += (tym->tm_hour * 60 + tym->tm_min) * 60 + tym->tm_sec;
#ifndef MAC_OS
    return julian - getzoff(zone) * how;
#else /* MAC_OS */
    return (SYS_TIME_OFFSET + julian) - getzoff(zone) * how;
#endif
}

#define JAN	1
#define FEB	2
#define MAR	3
#define APR	4
#define MAY	5
#define JUN	6
#define JUL	7
#define AUG	8
#define SEP	9
#define OCT	10
#define NOV	11
#define DEC	12

/* stolen direct from ELM */
int
month_to_n(name)
    const char *name;
{
    /** return the month number given the month name... **/

    register char ch;

    switch (lower(*name)) {
	case 'a' : if ((ch = lower(name[1])) == 'p')
		       return(APR);
		   else if (ch == 'u')
		       return(AUG);
		   else return(-1);	/* error! */
	case 'd' : return(DEC);
	case 'f' : return(FEB);
	case 'j' : if ((ch = lower(name[1])) == 'a')
		       return(JAN);
		   else if (ch == 'u') {
		     if ((ch = lower(name[2])) == 'n')
			 return(JUN);
		     else if (ch == 'l')
			 return(JUL);
		     else return(-1);		/* error! */
		   }
		   else return(-1);		/* error */
	case 'm' : if ((ch = lower(name[2])) == 'r')
		       return(MAR);
		   else if (ch == 'y')
		       return(MAY);
		   else return(-1);		/* error! */
	case 'n' : return(NOV);
	case 'o' : return(OCT);
	case 's' : return(SEP);
	default  : return(-1);
    }
}

/* This macro is from include/zccmac.h */
#define skipspaces(n)     for(p += (n); *p == ' ' || *p == '\t'; ++p)

/* parse date and return a string that looks like
 *   %ld%3c%s	gmt_in_secs weekday orig_timezone
 * This function is a bunch of scanfs on known date formats.  Don't
 * trust the "weekday" name fields because they may not be spelled
 * right, or have the correct punctuation.  Figure it out once the
 * year and month and date have been determined.
 *
 * If the today parameter is nonzero, the parsed date is checked
 * against today's date.  If the parsed date exceeds todays date
 * by more than a threshold value, a fatal error is generated.
 */
char *
parse_date(p, today)
    char *p;
    time_t today;
{
    /* When scanf-ing if month isn't a month, it could be a _long_ string.
     * this is also the static buffer whose address we return.
     */
    static char month[64];
    char Wkday[4], Zone[12], dst[4];
    char a_or_p;
    int Month = 0, Day = 0, Year = 0;
    int Hours = -1, Mins = -1, Secs = -1;
    time_t seconds;
    struct tm T;

    Zone[0] = dst[0] = 0;
    skipspaces(0);

    /* programmer's note -- there are too many scanfs here for some compilers
     * to put them all into one if statement.  Use goto's :-(  Also reset
     * Zone[0] after any sscanf() that could corrupt it on a partial match.
     *
     * Not yet handling all possible combinations of mailers using two-word
     * time zones, e.g. MET DST instead of MEST.  Only the specific case
     * where this was reported has been handled here.
     */

    /* RFC822 formats and minor variations -- order important */

    /*   day_number month_name year_number time timezone */
    if (sscanf(p, "%d %s %d %d%*1[.:]%d%*1[.:]%d %7s %3s",
	       &Day, month, &Year, &Hours, &Mins, &Secs, Zone, dst) >= 6
	&& Day)
	goto gotit;
    Zone[0] = dst[0] = 0;
    if (sscanf(p, "%d %s %d %d:%d %7s %3s",
	       &Day, month, &Year, &Hours, &Mins, Zone, dst) >= 5 && Day) {
	Secs = 0;
	goto gotit;
    }
    Zone[0] = dst[0] = 0;
    /*   day_name day_number month_name year_number time timezone */
    if (sscanf(p, "%*s %d %s %d %d:%d:%d %7s %3s",
	       &Day, month, &Year, &Hours, &Mins, &Secs, Zone, dst) >= 6
	&& Day)
	goto gotit;
    Zone[0] = dst[0] = Secs = 0;
    if (sscanf(p, "%*s %d %s %d %d:%d %7s %3s",
	       &Day, month, &Year, &Hours, &Mins, Zone, dst) >= 5 && Day) {
	Secs = 0;
	goto gotit;
    }
    Zone[0] = dst[0] = 0;

    /* This next one is technically wrong, but ... sample seen was:
     *          Fri,17 Jul 92 19:30:00 -0300 GMT
     */

    /*   day_name,day_number month_name year_number time timezone */
    if (sscanf(p, "%*c%*c%*c,%d %s %d %d:%d:%d %7s %3s",
	       &Day, month, &Year, &Hours, &Mins, &Secs, Zone, dst) >= 5
	&& Day)
        goto gotit;
    Zone[0] = dst[0] = Secs = 0;
    if (sscanf(p, "%*c%*c%*c,%d %s %d %d:%d %7s %3s",
	       &Day, month, &Year, &Hours, &Mins, Zone, dst) >= 5 && Day)
        goto gotit;
    Zone[0] = dst[0] = 0;

    /* Ctime format (From_ lines) -- timezone almost never found */

    /*   day_name month_name day_number time timezone year_number */
    if (sscanf(p, "%*s %s %d %d:%d:%d %7s %d",
	       month, &Day, &Hours, &Mins, &Secs, Zone, &Year) == 7)
	goto gotit;
    Zone[0] = 0;
    /*   day_name month_name day_number time year_number */
    if (sscanf(p, "%*s %s %d %d:%d:%d %d",
	       month, &Day, &Hours, &Mins, &Secs, &Year) == 6 && Year > 0)
	goto gotit;
    /*   day_name month_name day_number time timezone dst year_number */
    if (sscanf(p, "%*s %s %d %d:%d:%d %7s %3s %d",
	       month, &Day, &Hours, &Mins, &Secs, Zone, dst, &Year) == 8)
	goto gotit;
    Zone[0] = dst[0] = Secs = 0;

    /* Other common variants */

    /*   day_number month_name year_number time-timezone (day) */
    /*                                       ^no colon separator */
    if (sscanf(p, "%d %s %d %2d%2d%1[-+]%6[0123456789]",
	       &Day, month, &Year, &Hours, &Mins, &Zone[0], &Zone[1]) == 7)
	goto gotit;
    if (sscanf(p, "%d %s %d %2d%2d-%7s",	/* Does this _ever_ hit? */
	       &Day, month, &Year, &Hours, &Mins, Zone) == 6) {
	Secs = 0;
	goto gotit;
    }
    Zone[0] = 0;

    /*   day_number month_name year_number time timezone	*/
    /*                                      ^no colon separator */
    /*   (This is the odd one in the RFC822 examples section;	*/
    /*    also catches the slop from partial hits above.)	*/
    if (sscanf(p, "%d %s %d %2d%2d %7s",
	       &Day, month, &Year, &Hours, &Mins, Zone) >= 5 && Day) {
	Secs = 0;
	goto gotit;
    }
    Zone[0] = 0;
    
    Zone[1] = 0;	/* Yes, Zone[1] -- tested below */

    /*   day_number month_name year_number, time "-" ?? */
    if (sscanf(p,"%d %s %d, %d:%d:%d %1[-+]%6[0123456789]",
	       &Day, month, &Year, &Hours, &Mins, &Secs,
	       &Zone[0], &Zone[1]) >= 6 && Day)
	goto gotit;

    /*   day_number month_name year_number 12_hour_time a_or_p */
    if (sscanf(p, "%d %s %d %d:%d:%d %cm %7s",
	       &Day, month, &Year, &Hours, &Mins, &Secs, &a_or_p, Zone) >= 7) {
	if (a_or_p == 'p')
	    Hours += 12;
	goto gotit;
    }

    /* Some sscanf()s have a bug that causes the 19 in 19:30:00 of
     * this date:       Fri,17 Jul 92 19:30:00 -0300 GMT
     * to be scanned into both the Year and Hours values, which
     * causes a false hit in the next sscanf().  What to do?
     *
     * MIPS is one such broken sscanf.  Added the Hours != Year hack.
     */

    /*   day_name month_name day_number year_number time */
    if (sscanf(p, "%*s %s %d %d %d:%d:%d %7s",
	       month, &Day, &Year, &Hours, &Mins, &Secs, Zone) >= 6 &&
	Hours != Year)
	goto gotit;
    Zone[0] = Secs = 0;
    if (sscanf(p, "%*s %s %d %d %d:%d %7s",
	       month, &Day, &Year, &Hours, &Mins, Zone) >= 5 &&
	Hours != Year) {
	Secs = 0;
	goto gotit;
    }
    Zone[0] = 0;

    /*   day_name month_name day_number time timezone year_number */
    Year = 0;	/* Some newsreaders omit the year when saving; PR 3837 */
    if (sscanf(p, "%*s %s %d %d:%d:%d %7s %d",
	       month, &Day, &Hours, &Mins, &Secs, Zone, &Year) >= 6)
	goto gotit;
    Secs = 0;	/* For the next 4 attempts */
    if (sscanf(p, "%*s %s %d %d:%d %7s %d",
	       month, &Day, &Hours, &Mins, Zone, &Year) >= 5)
	goto gotit;
    Zone[0] = 0;

    /* Secs == 0 must be true here */

    /*   day_number-month_name-year time */
    if (sscanf(p,"%d-%[^-]-%d %d:%d", &Day, month, &Year, &Hours, &Mins) == 5)
	goto gotit;

    /*   day_name, day_number-month_name-year time */
    if (sscanf(p,"%*s %d-%[^-]-%d %d:%d",
	       &Day, month, &Year, &Hours, &Mins) == 5)
	goto gotit;

    /*   year_number-month_number-day_number time */
    if (sscanf(p, "%d-%d-%d %d:%d", &Year, &Month, &Day, &Hours, &Mins) == 5)
	goto gotit;

    /*   month_name day_number time year Zone */
    /*   (ctime, but without the day name)    */
    if (sscanf(p, "%s %d %d:%d:%d %d %7s",
	       month, &Day, &Hours, &Mins, &Secs, &Year, Zone) >= 6)
	goto gotit;
    Zone[0] = 0;

#if 0				/* xxx what to do in this case? */
    if (ison(glob_flags, WARNINGS))
	print("Unknown date format: %s\n", p);
#endif
    return NULL;

  gotit:
    if (Year > 1900)
	Year -= 1900;
    else if (Year < 0) {
#if 0				/* xxx what to do? */
	if (ison(glob_flags, WARNINGS))
	    print("Date garbled or bad year: %s\n", p);
#endif
	return NULL;
    }
    if (!Month && (Month = month_to_n(month)) == -1) {
#if 0
	if (ison(glob_flags, WARNINGS))
	    print("Date garbled or bad month: %s\n", p);
#endif
	return NULL;
    }
    if (!ci_strcmp(dst, "dst")) {
	(void) strcat(Zone, " ");
	(void) strcat(Zone, dst);
    }
    if (Zone[0] == 0) {
	/* Use local time zone if none found -- important for date_recv */
	(void) time_n_zone(Zone);
    }

    (void) sprintf(Wkday, "%.3s", day_names[day_number(Year, Month, Day)]);

    T.tm_sec = Secs;
    T.tm_min = Mins;
    T.tm_hour = Hours;
    T.tm_mday = Day;
    T.tm_mon = Month - 1;
    T.tm_year = Year;
    T.tm_wday = T.tm_yday = 0;	/* not used in time2gmt() */
    T.tm_isdst = 0;		/* determined from Zone */
#ifdef HAVE_TM_ZONE
    T.tm_zone = Zone;		/* not currently used; for completeness */
#endif /* HAVE_TM_ZONE */
    seconds = time2gmt(&T, Zone, 1);

    (void) sprintf(month, "%ld%s%s", seconds, Wkday, Zone);

    return month;
}
/**********************************************************************\
**	       END SELECTED EXCERPTS FROM MSGS/DATES.C                **
\**********************************************************************/

static int mnum;

struct dp_wr_msg_data {
    FILE *fp;
    long hoff;			/* header offset */
    long hbytes;		/* header bytes */
    long boff;			/* body offset */
    long bbytes;		/* body bytes */
    struct dpipe *dp1, *dp2;
};

static void
dp_wr_msg(dp, data)
    struct dpipe *dp;		/* ignored */
    struct dp_wr_msg_data *data;
{
    char buf[BUFSIZ];
    int bytes;

    if (data->hoff >= 0) {
	efseek(data->fp, data->hoff, SEEK_SET, "compute_extras");
	data->hoff = -1;
    } else if ((data->hbytes <= 0) && (data->boff >= 0)) {
	efseek(data->fp, data->boff, SEEK_SET, "compute_extras");
	data->boff = -1;
    }
    if (data->hbytes > 0) {
	bytes = efread(buf, 1, MIN(BUFSIZ, data->hbytes), data->fp,
		       "compute_extras");
	data->hbytes -= bytes;
	if (data->dp1)
	    dpipe_Write(data->dp1, buf, bytes);
	if (data->dp2)
	    dpipe_Write(data->dp2, buf, bytes);
    } else if (data->bbytes > 0) {
	bytes = efread(buf, 1, MIN(BUFSIZ, data->bbytes), data->fp,
		       "compute_extras");
	data->bbytes -= bytes;
	if (data->dp1)
	    dpipe_Write(data->dp1, buf, bytes);
	if (data->dp2)
	    dpipe_Write(data->dp2, buf, bytes);
    }
    if (!(data->hbytes) && !(data->bbytes)) {
	if (data->dp1)
	    dpipe_Close(data->dp1);
	if (data->dp2)
	    dpipe_Close(data->dp2);
    }
}

static void
do_hashes(dp, minfo)
    struct dpipe *dp;
    struct msg_info *minfo;
{
    if (minfo->have_key_hash && minfo->have_header_hash) {
	char *ptr;

	dpipe_Get(dp, &ptr);
	free(ptr);
    } else {
	mmsg_ComputeHashes(dp, &(minfo->key_hash), &(minfo->header_hash));
	minfo->have_key_hash = 1;
	minfo->have_header_hash = 1;
    }
}

#define CR (13)
#define LF (10)

#define turnon(var,val) ((var) |= (val))
#define turnoff(var,val) ((var) &= ~(val))

#define any strpbrk

/* From msgs/addrs.c */

/*
 * The guts of get_name_n_addr(), above.
 */
char *
parse_address_and_name(str, addr, name)
    char *str;
    char *addr, *name;
{
    register char *p, *p2, *beg_addr = addr, *beg_name = name, c;
    static char *specials = "<>@,;:\\.[]";	/* RFC822 specials */
    int angle = 0;

    if (!*str)
	return str;

    /* We need this again because this function is recursive */
    while (isspace(*str))
	str++;

    /* first check to see if there's something to look for */
    if (!(p = any(str, ",(<\""))) {
	/* no comma or indication of a quote character. Find a space and
	 * return that.  If nothing, the entire string is a complete address
	 */
	if (p = any(str, " \t"))
	    c = *p, *p = 0;
	if (addr)
	    (void) strcpy(addr, str);
	if (p)
	    *p = c;
	return p ? p : (char *) str + strlen(str);
    }

    /* comma terminated before any comment stuff.  If so, check for whitespace
     * before-hand cuz it's possible that strings aren't comma separated yet
     * and they need to be.
     *
     * address address address, address
     *                        ^p  <- p points here.
     *        ^p2 <- should point here.
     */
    if (*p == ',') {
	c = *p, *p = 0;
	if (p2 = any(str, " \t"))
	    *p = ',', c = *p2, p = p2, *p = 0;
	if (addr)
	    (void) strcpy(addr, str);
	*p = c;
	return p;
    }

    /* starting to get hairy -- we found an angle bracket. This means that
     * everything outside of those brackets are comments until we find that
     * all important comma.  A comment AFTER the <addr> :
     *  <address> John Doe
     * can't call this function recursively or it'll think that "John Doe"
     * is a string with two legal address on it (each name being an address).
     *
     * Bart: Wed Sep  9 16:42:48 PDT 1992 -- CRAY_CUSTOM
     * Having the comment after the address is incorrect RFC822 syntax anyway.
     * Just to be forgiving, fudge it so that we accept a comment after an <>
     * address if and only if the comment doesn't contain any 822 specials.
     * Otherwise, treat it as another address (which is also more forgiving
     * than strict RFC822, but maintains a little backward compatibility).
     */
    if (*p == '<') { /* note that "str" still points to comment stuff! */
	angle = 1;
	if (name && *str) {
	    *p = 0;
	    strcpy(name, str);
	    name += strlen(name);
	    *p = '<';
	}
	if (!(p2 = index(p+1, '>')))
	    return NULL;
	if (addr) {
	    /* to support <addr (comment)> style addresses, add code here */
	    *p2 = 0;
	    skipspaces(1);
	    strcpy(addr, p);
	    addr += strlen(addr);
	    while (addr > beg_addr && isspace(*(addr-1)))
		*--addr = 0;
	    *p2 = '>';
	}
	/* take care of the case "... <addr> com (ment)" */
	{
	    int p_cnt = 0; /* parenthesis counter */
	    int inq = 0; /* Quoted-string indicator */
	    char *orig_name = name;

	    p = p2;
	    /* don't recurse yet -- scan till null, comma or '<'(add to name) */
	    for (p = p2; p[1] && (p_cnt || p[1] != ',' && p[1] != '<'); p++) {
		if (p[1] == '(')
		    p_cnt++;
		else if (p[1] == ')')
		    p_cnt--;
		else if (p[1] == '"' && p[0] != '\\')
		    inq = !inq;
		else if (!inq && !p_cnt && p[0] != '\\' &&
			index(specials, p[1])) {
		    if (orig_name)
			*(name = orig_name) = 0;
		    return p2 + 1;
		}
		if (name)
		    *name++ = p[1];
	    }
	    if (p_cnt)
		return NULL;
	}
	if (name && name > beg_name) {
	    while (isspace(*(name-1)))
		--name;
	    *name = 0;
	}
    }

    /* this is the worst -- now we have parentheses/quotes.  These guys can
     * recurse pretty badly and contain commas within them.
     */
    if (*p == '(' || *p == '"') {
	char *start = p;
	int comment = 1;
	c = *p;
	/* "str" points to address while p points to comments */
	if (addr && *str) {
	    *p = 0;
	    while (isspace(*str))
		str++;
	    strcpy(addr, str);
	    addr += strlen(addr);
	    while (addr > beg_addr && isspace(*(addr-1)))
		*--addr = 0;
	    *p = c;
	}
	while (comment) {
	    if (c == '"' && !(p = index(p+1, '"')) ||
		c == '(' /*)*/ && !(p = any(p+1, "()")))
		return NULL;
	    if (p[-1] != '\\') {
		if (*p == '(')	/* loop again on parenthesis */
		    comment++;
		else		/* quote or close paren may end loop */
		    comment--;
	    }
	}
	/* Hack to handle "address"@domain */
	if (c == '"' && *(p + 1) == '@') {
	    c = *++p; *p = 0;
	    strcpy(addr, start);
	    addr += strlen(addr);
	    *p-- = c;
	} else if ((p2 = any(p+1, "<,")) && *p2 == '<') {
	    /* Something like ``Comment (Comment) <addr>''.  In this case
	     * the name should include both comment parts with the
	     * parenthesis.   We have to redo addr.
	     */
	    angle = 1;
	    if (!(p = index(p2, '>')))
		return NULL;
	    if (addr = beg_addr) { /* reassign addr and compare to null */
		c = *p; *p = 0;
		strcpy(addr, p2+1);
		addr += strlen(addr);
		while (addr > beg_addr && isspace(*(addr-1)))
		    *--addr = 0;
		*p = c;
	    }
	    if (name) {
		c = *p2; *p2 = 0;
		strcpy(name, str);
		name += strlen(name);
		while (name > beg_name && isspace(*(name-1)))
		    *--name = 0;
		*p2 = c;
	    }
	} else if (name && start[1]) {
	    c = *p, *p = 0; /* c may be ')' instead of '(' now */
	    strcpy(name, start+1);
	    name += strlen(name);
	    while (name > beg_name && isspace(*(name-1)))
		*--name = 0;
	    *p = c;
	}
    }
    p2 = ++p;	/* Bart: Fri May 14 19:11:33 PDT 1993 */
    skipspaces(0);
    /* this is so common, save time by returning now */
    if (!*p)
	return p;
    /* Bart: Fri May 14 19:11:40 PDT 1993
     * If we hit an RFC822 special here, we've already scanned one
     * address and are looking at another.  Since we're supposed to
     * return the end of the address just parsed, back up.  See the
     * CRAY_CUSTOM comment above for rationales.
     *
     * pf Tue Aug 10 16:51:06 1993
     * Only do this if we actually read a '<'.  (xxx"foobar"@zen doesn't
     * work otherwise)
     *
     * Bart: Fri Dec 10 15:37:48 PST 1993
     * Special-case '[' to force Cray address-book stuff to work.
     */
    if (*p == '[' || angle && index(specials, *p))
	return p2;
    return parse_address_and_name(p, addr, name);
}

/*
 * Get address and name from a string (str) which came from an address header
 * in a message or typed by the user.  The string may contain one or more
 * well-formed addresses.  Each must be separated by a comma.
 *
 * address, address, address
 * address (comment or name here)
 * comment or name <address>
 * "Comment, even those with comma's!" <address>
 * address (comma, (more parens), etc...)
 *
 * This does *not* handle cases like:
 *    comment <address (comment)>
 *
 * find the *first* address here and return a pointer to the end of the
 * address (usually a comma).  Return NULL on error: non-matching parens,
 * brackets, quotes...
 *
 * Suppress error mesages if BOTH name and addr are NULL.
 */
static char *
get_name_n_addr(str, name, addr)
    const char *str;
    char *name, *addr;
{
    if (addr)
	*addr = 0;
    if (name)
	*name = 0;
    if (!str || !*str)
	return NULL;

    while (isspace(*str) || *str == ',')
	str++;
    return parse_address_and_name(str, addr, name);
}

/* check for valid "From " line, adapted from match_msg_sep() in foload.c */
int
is_from_line(buf)
    char *buf;
{
    char *p = buf+5; /* skip "From " */

    skipspaces(0);
    if (!(p = any(p, " \t")))
	return 0;
    if (parse_date(p+1, 0L))
	return 1;
    /* Try once more the hard way */
    if ((p = get_name_n_addr(buf + 5, NULL, NULL)) != 0)
	return (parse_date(p + 1, 0L) != NULL);
    return 0;
}

void
parse_status(str, val)
    const char *str;
    unsigned long *val;
{
    const char *p;

    for (p = str; *p; ++p) {
	switch (*p) {
	  case 'D':
	    turnon(*val, mmsg_status_DELETED);
	    turnoff(*val, mmsg_status_NEW);
	    turnoff(*val, mmsg_status_UNREAD);
	    break;
	  case 'O':
	    turnoff(*val, mmsg_status_NEW);
	    break;
	  case 'R':
	    turnoff(*val, mmsg_status_NEW);
	    turnoff(*val, mmsg_status_UNREAD);
	    break;
	  case 'N':
	    turnon(*val, mmsg_status_NEW);
	    turnon(*val, mmsg_status_UNREAD);
	    break;
	  case 'P':
	    turnon(*val, mmsg_status_UNREAD);
	    break;
	  case 'S':
	    turnon(*val, mmsg_status_SAVED);
	    turnoff(*val, mmsg_status_NEW);
	    break;
	  case 'r':
	    turnon(*val, mmsg_status_REPLIED);
	    turnoff(*val, mmsg_status_NEW);
	    break;
	  case 'f':
	    turnon(*val, mmsg_status_RESENT);
	    break;
	  case 'p':
	    turnon(*val, mmsg_status_PRINTED);
	    break;
	}
    }
}

static void
do_other(dp, minfo)
    struct dpipe *dp;
    struct msg_info *minfo;
{
    struct dynstr name, val;
    int c, sawstatus = 0, l;
    struct dynstr summ;
    char *subject = 0, *from = 0, frombuf[1024];
    char from2buf[1024];

    dynstr_Init(&name);
    dynstr_Init(&val);
    TRY {
	while (((c = dpipe_Peekchar(dp)) != dpipe_EOF)
	       && (c != CR)
	       && (c != LF)) {
	    dynstr_Set(&name, 0);
	    dynstr_Set(&val, 0);
	    mime_Header(dp, &name, &val, 0);
	    if (!ci_strcmp(dynstr_Str(&name), "date")) {
		char *parsed;

		mime_Unfold(&val, 1);
		parsed = parse_date(dynstr_Str(&val), 0);

		if (parsed) {
		    sscanf(parsed, "%ld", &(minfo->date));
		    minfo->have_date = 1;
		}
	    } else if (!ci_strcmp(dynstr_Str(&name), "status")) {
		parse_status(dynstr_Str(&val), &(minfo->status));
		sawstatus = 1;
	    } else if (!subject && !ci_strcmp(dynstr_Str(&name), "subject")) {
		mime_Unfold(&val, 1);
		subject = dynstr_GiveUpStr(&val);
		dynstr_Init(&val);
	    } else if (!from && !ci_strcmp(dynstr_Str(&name), "from")) {
		mime_Unfold(&val, 1);
		from = frombuf;
		get_name_n_addr(dynstr_Str(&val), from, from2buf);
		if (!*from)
		    from = from2buf;
	    } else if (!ci_strcmp(dynstr_Str(&name), "x-uidl")) {
		char *end;

		mime_Unfold(&val, 1);
		for (end = dynstr_Str(&val); ' ' == *end || '\t' == *end;
		     end++)
		    ;
		minfo->unique_id = strdup(end);
		end = minfo->unique_id;
		/* [0x21, 0x73] are legal characters according to RFC 1725 */
		while (*end >= 0x21 && *end <= 0x7E)
		    ++end;
		*end = '\0';
	    }
	}

	/* Now use mnum, subject (if non-zero), from (if non-zero),
	 * extras->date and extras->status to compute
	 * extras->summary.
	 *
	 * message-number status from-address date lines/chars subject
	 */
	dynstr_Init(&summ);

	/* The message number */
	{
	    char num[16];

	    sprintf(num, "%3.d", mnum);	/* 1-based */
	    dynstr_Set(&summ, num);
	    if (mnum < 999)
		dynstr_Append(&summ, "  ");	/* Two spaces */
	}

	/* status of the message */
	if (minfo->status & mmsg_status_DELETED)
	    dynstr_AppendChar(&summ, '*');
	else if (minfo->status & mmsg_status_PRESERVED)
	    dynstr_AppendChar(&summ, 'P');
	else if (minfo->status & mmsg_status_SAVED)
	    dynstr_AppendChar(&summ, 'S');
	else if ((minfo->status & mmsg_status_UNREAD) &&
		 !(minfo->status & mmsg_status_NEW))
	    dynstr_AppendChar(&summ, 'U');
	else if (minfo->status & mmsg_status_PRINTED)
	    dynstr_AppendChar(&summ, 'p');
	else if (minfo->status & mmsg_status_RESENT)
	    dynstr_AppendChar(&summ, 'f');
	else if (minfo->status & mmsg_status_NEW)
	    dynstr_AppendChar(&summ, 'N');
	else
	    dynstr_AppendChar(&summ, ' ');

	if (minfo->status & mmsg_status_REPLIED)
	    dynstr_AppendChar(&summ, 'r');
	else
	    dynstr_AppendChar(&summ, ' ');
	dynstr_AppendChar(&summ, ' ');

	/* 16 chars of from line */
	l = (from ? strlen(from) : 0);
	if (from)
	    dynstr_AppendN(&summ, from, 16);
	if (l < 16)
	    dynstr_AppendN(&summ, "                ", 16 - l);
	dynstr_AppendChar(&summ, ' ');

	/* day-of-week month day-of-month time */
	/* XXX turnon(glob_flags, MIL_TIME); */
	if (minfo->have_date)
	    dynstr_Append(&summ, time_str("D M d T", minfo->date));
	else
	    dynstr_Append(&summ, "                  ");
	dynstr_Append(&summ, "  ");	/* Two spaces */

	/* XXX Do we have/want the lines and chars numbers?? */

	/* subject */
	if (subject)
	    dynstr_Append(&summ, subject);

	/* XXX Trim the entire thing to 80 chars ?? */
    
	/* XXX Fix everything above! */
	minfo->summary = dynstr_GiveUpStr(&summ);

	/* Now discard the rest of the input */
	mime_NextBoundary(dp, 0, 0, 0);
    } FINALLY {
	dynstr_Destroy(&name);
	dynstr_Destroy(&val);
	if (subject)
	    free(subject);
    } ENDTRY;
}

static void
ComputeUid(p, msginfo)
    POP *p;
    struct msg_info *msginfo;
{
    struct dpipe udp;
    struct dputil_MD5buf md5buf;
    struct mailhash uid_hash;
    char msg_key[4*MAILHASH_BYTES];
    char buf[4*MAILHASH_BYTES];
    char msg_num[10];
    Enc64 E64;
    int i;

    sprintf(msg_num,"%d",msginfo->number);
    mailhash_to_string(msg_key, &(msginfo->key_hash));
    dputil_MD5buf_init(&md5buf, uid_hash.x);
    dpipe_Init(&udp, dputil_MD5, &md5buf, 0, 0, 0);
    TRY {
	dpipe_Write(&udp, p->time_stamp, p->time_stamp_len);
	dpipe_Write(&udp, msg_num, strlen(msg_num));
	dpipe_Write(&udp, msg_key, strlen(msg_key));
	dpipe_Close(&udp);
	dpipe_Flush(&udp);
	dputil_MD5buf_final(&md5buf);
	mailhash_to_string(buf, &uid_hash);
        memset((void *)buf, 0, sizeof(buf));
	/* initialize the E64 structure */
	E64.partial[1] = E64.partial[2] = E64.partial[3] = E64.partial[4] = 0;
	E64.partialCount = 0;
	E64.bytesOnLine = 0;
	Encode64(uid_hash.x, MAILHASH_BYTES, buf, "", &E64);
	msginfo->unique_id = strdup(buf);
    } FINALLY {
        dpipe_Destroy(&udp);
    } ENDTRY;
}


/*
 * Compute hash codes and canonicalized keys for a given list of messages.
 * Sends a failure message to the output if something goes wrong.
 */

int
compute_extras(p, msg_count, msg_numbers)
    POP *p;
    int msg_count;
    int *msg_numbers;
{
    int *mptr;
    struct msg_info *minfo;
    struct dpipe hashesdp, otherdp;
    struct dp_wr_msg_data dp_wr_msg_data;

    for (mptr = msg_numbers; mptr < (msg_numbers + msg_count); ++mptr) {
	mnum = *mptr;
	minfo = NTHMSG(p, mnum);
	if (minfo->have_key_hash
	    && minfo->have_header_hash
	    && minfo->summary
	    && minfo->unique_id
	    && minfo->have_date) /* already computed */
	    continue;

	dp_wr_msg_data.fp = p->drop;
	dp_wr_msg_data.hoff = minfo->header_offset;
	dp_wr_msg_data.hbytes = minfo->header_length;
	dp_wr_msg_data.boff = minfo->body_offset;
	dp_wr_msg_data.bbytes = minfo->body_length;
	if (minfo->have_key_hash && minfo->have_header_hash)
	    dp_wr_msg_data.dp1 = 0;
	else
	    dp_wr_msg_data.dp1 = &hashesdp;
	if (minfo->summary && minfo->have_date && minfo->unique_id)
	    dp_wr_msg_data.dp2 = 0;
	else
	    dp_wr_msg_data.dp2 = &otherdp;
	if (dp_wr_msg_data.dp1)
	    dpipe_Init(dp_wr_msg_data.dp1,
		       do_hashes, minfo,
		       dp_wr_msg, &dp_wr_msg_data,
		       0);
	if (dp_wr_msg_data.dp2)
	    dpipe_Init(dp_wr_msg_data.dp2,
		       do_other, minfo,
		       dp_wr_msg, &dp_wr_msg_data,
		       0);
	TRY {
	    if (dp_wr_msg_data.dp1)
		dpipe_Pump(dp_wr_msg_data.dp1);
	    if (dp_wr_msg_data.dp2)
		dpipe_Pump(dp_wr_msg_data.dp2);
	} FINALLY {
	    if (dp_wr_msg_data.dp1)
		dpipe_Destroy(dp_wr_msg_data.dp1);
	    if (dp_wr_msg_data.dp2)
		dpipe_Destroy(dp_wr_msg_data.dp2);
	} ENDTRY;
	if (!(minfo->unique_id)) {
	    ComputeUid(p, minfo);
	}
    }
    return (0);
}


static int
do_uidl(p)
    POP *p;
{
    int msg_num, i, count;
    struct msg_info *mp;
    char buf[4 * MAILHASH_BYTES];

    if (p->parm_count > 0) {
	msg_num = atoi(p->pop_parm[1]);

	/* Is requested message out of range? */
	if ((msg_num < 1) || (msg_num > NUMMSGS(p))) {
	    pop_msg(p, POP_FAILURE, "Message %d does not exist.", msg_num);
	    return POP_FAILURE;
	}

	/* Get a pointer to the message in the message list */
	mp = NTHMSG(p, msg_num);

	/* Is the message already flagged for deletion? */
	if (mp->status & mmsg_status_DELETED) {
	    pop_msg(p, POP_FAILURE, "Message %d has been deleted.", msg_num);
	    return POP_FAILURE;
	}
	if (compute_extras(p, 1, &msg_num) != 0)
	    return POP_FAILURE;

	pop_msg(p, POP_SUCCESS, "%d %s", msg_num, mp->unique_id);
	return POP_SUCCESS;
    }

    /* Loop through the message information list.  Skip deleted messages */
    count = 0;
    glist_FOREACH(&(p->minfo), struct msg_info, mp, i) {
	if (!(mp->status & mmsg_status_DELETED)) {
	    if (compute_extras(p, 1, &mp->number) != 0)
		return POP_FAILURE;
	    ++count;
	}
    }

    pop_msg(p, POP_SUCCESS, "%d messages", count);
    allow(p, count * 25);	/* roughly */

    glist_FOREACH(&(p->minfo), struct msg_info, mp, i) {
	if (!(mp->status & mmsg_status_DELETED))
	    fprintf(p->output, "%d %s%s", mp->number, mp->unique_id,
		    mime_CRLF);
    }
    putc('.', p->output);
    fputs(mime_CRLF, p->output);
    fflush(p->output);
    return POP_SUCCESS;
}

/* Send key uidl identifier strings for a given list of messages. */

int
zync_uidl(p)
    POP *p;
{
    do_drop(p);

    return (do_uidl(p)); 
}

