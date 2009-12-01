/* dates.c     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	dates_rcsid[] = "$Id: dates.c,v 2.35 2005/05/09 09:15:20 syd Exp $";
#endif

#include "zmail.h"
#include "catalog.h"
#include "dates.h"
#ifndef LICENSE_FREE
# ifndef MAC_OS
#include "license/server.h"
# else
#include "server.h"
# endif /* !MAC_OS */
#endif /* LICENSE_FREE */
#include "strcase.h"
#include "zctime.h"

/*
 *   %10ld%3c%s	gmt_in_secs weekday orig_timezone
 * The standard "date format" stored in the msg data structure.
 */
char *day_names[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
char *month_names[] = {     /* was imported in pick.c - superceded by local_ */
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
catalog_ref local_day_names[] = {
    catref( CAT_MSGS, 915, "Sun" ), catref( CAT_MSGS, 916, "Mon" ), 
    catref( CAT_MSGS, 917, "Tue" ), catref( CAT_MSGS, 918, "Wed" ), 
    catref( CAT_MSGS, 919, "Thu" ), catref( CAT_MSGS, 920, "Fri" ), 
    catref( CAT_MSGS, 921, "Sat" )
};
catalog_ref local_month_names[] = {     /* imported in pick.c */
    catref( CAT_MSGS, 922, "Jan" ), catref( CAT_MSGS, 923, "Feb" ), 
    catref( CAT_MSGS, 924, "Mar" ), catref( CAT_MSGS, 925, "Apr" ), 
    catref( CAT_MSGS, 926, "May" ), catref( CAT_MSGS, 927, "Jun" ), 
    catref( CAT_MSGS, 928, "Jul" ), catref( CAT_MSGS, 929, "Aug" ), 
    catref( CAT_MSGS, 930, "Sep" ), catref( CAT_MSGS, 931, "Oct" ), 
    catref( CAT_MSGS, 932, "Nov" ), catref( CAT_MSGS, 933, "Dec" )
};
static int mtbl[] = { 0,31,59,90,120,151,181,212,243,273,304,334 };

/* Time Zone Stuff */
struct zoneoff {
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
    /* { "AST",	 -4,  0 },	{ "ADT",	 -3,  0 }, */	/* Atlantic */
    { "EST",	 -5,  0 },	{ "EDT",	 -4,  0 },	/* Eastern */
    { "CST",	 -6,  0 },	{ "CDT",	 -5,  0 },	/* Central */
    { "MST",	 -7,  0 },	{ "MDT",	 -6,  0 },	/* Mountain */
    { "PST",	 -8,  0 },	{ "PDT",	 -7,  0 },	/* Pacific */
    { "YST",	 -8,  0 },	{ "YDT",	 -7,  0 },	/* Yukon */
    /* { "AST",	 -9,  0 },	{ "ADT",	 -8,  0 }, */	/* Alaska */
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

long
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
time_t
time2gmt(tym, zone, how)
struct tm *tym;
char *zone;
int how;
{
    time_t year, julian;

#ifdef THE_ZMAIL_WAY
    if (tym->tm_year < 200) {
	year = tym->tm_year + 1900;
	if (year < 1970)
	    year += 100;	/* Assume year > 2000 ? */
    } else {
	year = tym->tm_year + 1900;
    }
#else
#ifdef THE_POSIX_WAY
    if (tym->tm_year < 100) {
	year = tym->tm_year + 1900;
	if (year < 1969)
	    year += 100;	/* Assume year > 2000 */
    } else {
	year = tym->tm_year + 1900;
    }
#else
    /* The DRUMS way */
    if (tym->tm_year < 100) {
	year = tym->tm_year + 1900;
	if (year < 1950)
	    year += 100;	/* Assume year > 2000 */
    } else {
	year = tym->tm_year + 1900;
    }
#endif
#endif

    julian = 365 * (year - 1970) + (int)((year - 1970 + 1) / 4) +
		mtbl[tym->tm_mon] + tym->tm_mday - 1;
		/* tym->tm_yday might not be valid */
    if (tym->tm_mon > 1 && year%4 == 0 && (year%100 != 0 || year%400 == 0))
	julian++;
    julian *= 86400;	/* convert to seconds */
    julian += (tym->tm_hour * 60L + tym->tm_min) * 60L + tym->tm_sec;
#ifndef MAC_OS
    return julian - getzoff(zone) * how;
#else /* MAC_OS */
    return (SYS_TIME_OFFSET + julian) - getzoff(zone) * how;
#endif
}

struct tm *
time_n_zone(zone)
char *zone;
{
    struct tm *T;
    char *tz;
#if defined(HAVE_TZNAME) || defined(HAVE_TM_ZONE) || defined(TIMEZONE)
    time_t	  x;

    (void) time(&x);
    T = localtime(&x);
#ifdef TIMEZONE
#ifdef DAYLITETZ
    if (T->tm_isdst > 0)
	tz = DAYLITETZ;
    else
#endif /* DAYLITETZ */
    tz = TIMEZONE;
#else /* !TIMEZONE */
#ifdef HAVE_TM_ZONE
    tz = T->tm_zone;
#else /* !HAVE_TM_ZONE */
#ifdef HAVE_TZNAME
    {
#ifdef DECLARE_TZNAME
	extern char *tzname[];
#endif /* DECLARE_TZNAME */
	tz = tzname[(T->tm_isdst != 0)];
    }
#endif /* HAVE_TZNAME */
#endif /* HAVE_TM_ZONE */
#endif /* TIMEZONE */
#else /* !HAVE_TZNAME && !HAVE_TM_ZONE && !TIMEZONE */
#ifdef HAVE_TIMEZONE
    extern char     *timezone();
    struct timeval  mytime;
    struct timezone myzone;

    (void) gettimeofday(&mytime, &myzone);
    T = localtime(&mytime.tv_sec);
    tz = timezone(myzone.tz_minuteswest, (T->tm_isdst && myzone.tz_dsttime));
#else /* !HAVE_TIMEZONE */
    error(ZmErrFatal, catgets( catalog, CAT_MSGS, 244, "There's no way to get your timezone!\n" ));
#endif /* HAVE_TIMEZONE */
#endif /* HAVE_TZNAME || TIMEZONE */

    (void) strncpy(zone, tz, 7), zone[7] = 0;
    return T;
}

/* Guts of time_str(), described below. */
char *
tm_time_str(opts, T)
register const char *opts;
struct tm *T;
{
    static char time_buf[30];
    register char *p = time_buf;

    for (;; opts++) {
	switch(*opts) {
	    case 's':
		(void) sprintf(p, "%02d", T->tm_sec);
	    when 't':
		(void) sprintf(p, "%02d%02d", T->tm_hour, T->tm_min);
	    when 'T':
		if (ison(glob_flags, MIL_TIME))
		    (void) sprintf(p, catgets( catalog, CAT_MSGS, 245, "%2d:%02d" ), T->tm_hour, T->tm_min);
		else
		    (void) sprintf(p, catgets( catalog, CAT_MSGS, 246, "%d:%02d%s" ), (T->tm_hour) ?
			  ((T->tm_hour <= 12) ? T->tm_hour : T->tm_hour - 12) :
			  12, T->tm_min,
			  ((T->tm_hour < 12) ? catgets( catalog, CAT_MSGS, 247, "am" ) : catgets( catalog, CAT_MSGS, 248, "pm" )));
	    when 'D': case 'W': (void) strcpy(p, catgetref(local_day_names[T->tm_wday]));
	    when 'm': (void) sprintf(p, "%02d", T->tm_mon + 1);
	    when 'M': (void) strcpy(p, catgetref(local_month_names[T->tm_mon]));
	    when 'y': (void) sprintf(p, "%02d", T->tm_year%100);
	    when 'Y': (void) sprintf(p, "%d", T->tm_year + 1900);
	    when 'n': case 'd': (void) sprintf(p, "%02d", T->tm_mday);
	    when 'N': (void) sprintf(p, "%d", T->tm_mday);
	    otherwise: *p = 0; return time_buf;
	}
	p += strlen(p);
    }
}

/* time_str() returns a string according to criteria:
 *   if "now" is 0, then the current time is gotten and used.
 *       else, use the time described by now
 *   opts points to a string of args which is parsed until an unknown
 *       arg is found and opts will point to that upon return.
 *   valid args are T (time of day), D (day of week), M (month), Y (year),
 *       N (number of day in month -- couldn't think of a better letter).
 */
char *
time_str(opts, now)
register const char *opts;
time_t now;
{
    struct tm 	  *T;
    time_t	  x;

    if (!opts)
	return NULL;
    if (now)
	x = now;	/* 3/12/95 !GF broken for MPW's localtime() */
    else
	(void) time(&x);
    T = localtime(&x);
    return tm_time_str(opts, T);
}

int
days_in_month(Year, Month)
int Year, Month;
{
    int days = mtbl[Month - 1];

    days = (Month < 12? mtbl[Month] : 365) - days;
    if (Month == 2 && Year%4 == 0 && (Year%100 != 0 || Year%400 == 0))
	days++;
    return days;
}

int
day_number(Year, Month, Day)
int Year,	/* Years since 1900 */
    Month,      /* Month (1 thru 12) */
    Day;        /* Day (1 thru 31) */
{
    /* Lots of foolishness with casts for Xenix-286 16-bit ints */

    time_t days_ctr;	/* 16-bit ints overflowed Sept 12, 1989 */

    days_ctr = ((time_t)Year * 365L) + ((Year + 3) / 4);
    days_ctr += mtbl[Month-1] + Day + 6;
    if (Month > 2 && (Year % 4 == 0))
	days_ctr++;
    return (int)(days_ctr % 7L);
}

/* parse date and return a string that looks like
 *   %10ld%3c%s	gmt_in_secs weekday orig_timezone
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
register char *p;
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

#ifdef OLD_BEHAVIOR
    /*   day_number month_name year_number time timezone */
    if (sscanf(p, "%d %63s %d %d:%d:%d %7s %3s",
	    &Day, month, &Year, &Hours, &Mins, &Secs, Zone, dst) >= 6 && Day)
	goto gotit;
#else /* New -- allow . as well as : between numbers in time */
    /*   day_number month_name year_number time timezone */
    if (sscanf(p, "%d %63s %d %d%*1[.:]%d%*1[.:]%d %7s %3s",
	    &Day, month, &Year, &Hours, &Mins, &Secs, Zone, dst) >= 6 && Day)
	goto gotit;
#endif /* OLD_BEHAVIOR */
    Zone[0] = dst[0] = 0;
    if (sscanf(p, "%d %63s %d %d:%d %7s %3s",
	    &Day, month, &Year, &Hours, &Mins, Zone, dst) >= 5 && Day) {
	Secs = 0;
	goto gotit;
    }
    Zone[0] = dst[0] = 0;
    /*   day_name day_number month_name year_number time timezone */
    if (sscanf(p, "%*s %d %63s %d %d:%d:%d %7s %3s",
	    &Day, month, &Year, &Hours, &Mins, &Secs, Zone, dst) >= 6 && Day)
	goto gotit;
    Zone[0] = dst[0] = Secs = 0;
    if (sscanf(p, "%*s %d %63s %d %d:%d %7s %3s",
	    &Day, month, &Year, &Hours, &Mins, Zone, dst) >= 5 && Day) {
	Secs = 0;
	goto gotit;
    }
    Zone[0] = dst[0] = 0;

    /* This next one is technically wrong, but ... sample seen was:
     *          Fri,17 Jul 92 19:30:00 -0300 GMT
     */

    /*   day_name,day_number month_name year_number time timezone */
    if (sscanf(p, "%*c%*c%*c,%d %63s %d %d:%d:%d %7s %3s",
            &Day, month, &Year, &Hours, &Mins, &Secs, Zone, dst) >= 5 && Day)
        goto gotit;
    Zone[0] = dst[0] = Secs = 0;
    if (sscanf(p, "%*c%*c%*c,%d %63s %d %d:%d %7s %3s",
            &Day, month, &Year, &Hours, &Mins, Zone, dst) >= 5 && Day)
        goto gotit;
    Zone[0] = dst[0] = 0;

    /* Ctime format (From_ lines) -- timezone almost never found */

    /*   day_name month_name day_number time timezone year_number */
    if (sscanf(p, "%*s %63s %d %d:%d:%d %7s %d",
	    month, &Day, &Hours, &Mins, &Secs, Zone, &Year) == 7)
	goto gotit;
    Zone[0] = 0;
    /*   day_name month_name day_number time year_number */
    if (sscanf(p, "%*s %63s %d %d:%d:%d %d",
	    month, &Day, &Hours, &Mins, &Secs, &Year) == 6 && Year > 0)
	goto gotit;
    /*   day_name month_name day_number time timezone dst year_number */
    if (sscanf(p, "%*s %63s %d %d:%d:%d %7s %3s %d",
	    month, &Day, &Hours, &Mins, &Secs, Zone, dst, &Year) == 8)
	goto gotit;
    Zone[0] = dst[0] = Secs = 0;

    /* Other common variants */

    /*   day_number month_name year_number time-timezone (day) */
    /*                                       ^no colon separator */
    if (sscanf(p, "%d %63s %d %2d%2d%1[-+]%6[0123456789]",
	    &Day, month, &Year, &Hours, &Mins, &Zone[0], &Zone[1]) == 7)
	goto gotit;
    if (sscanf(p, "%d %63s %d %2d%2d-%7s",	/* Does this _ever_ hit? */
	    &Day, month, &Year, &Hours, &Mins, Zone) == 6) {
	Secs = 0;
	goto gotit;
    }
    Zone[0] = 0;

    /*   day_number month_name year_number time timezone	*/
    /*                                      ^no colon separator */
    /*   (This is the odd one in the RFC822 examples section;	*/
    /*    also catches the slop from partial hits above.)	*/
    if (sscanf(p, "%d %63s %d %2d%2d %7s",
	    &Day, month, &Year, &Hours, &Mins, Zone) >= 5 && Day) {
	Secs = 0;
	goto gotit;
    }
    Zone[0] = 0;
    
    Zone[1] = 0;	/* Yes, Zone[1] -- tested below */

    /*   day_number month_name year_number, time "-" ?? */
    if (sscanf(p,"%d %63s %d, %d:%d:%d %1[-+]%6[0123456789]",
	    &Day, month, &Year, &Hours, &Mins, &Secs,
	    &Zone[0], &Zone[1]) >= 6 && Day)
	goto gotit;

    /*   day_number month_name year_number 12_hour_time a_or_p */
    if (sscanf(p, "%d %63s %d %d:%d:%d %cm %7s",
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
    if (sscanf(p, "%*s %63s %d %d %d:%d:%d %7s",
	    month, &Day, &Year, &Hours, &Mins, &Secs, Zone) >= 6 &&
	    Hours != Year)
	goto gotit;
    Zone[0] = Secs = 0;
    if (sscanf(p, "%*s %63s %d %d %d:%d %7s",
	    month, &Day, &Year, &Hours, &Mins, Zone) >= 5 &&
	    Hours != Year) {
	Secs = 0;
	goto gotit;
    }
    Zone[0] = 0;

    /*   day_name month_name day_number time timezone year_number */
    Year = 0;	/* Some newsreaders omit the year when saving; PR 3837 */
    if (sscanf(p, "%*s %63s %d %d:%d:%d %7s %d",
	    month, &Day, &Hours, &Mins, &Secs, Zone, &Year) >= 6)
	goto gotit;
    Secs = 0;	/* For the next 4 attempts */
    if (sscanf(p, "%*s %63s %d %d:%d %7s %d",
	    month, &Day, &Hours, &Mins, Zone, &Year) >= 5)
	goto gotit;
    Zone[0] = 0;

    /* Secs == 0 must be true here */

    /*   day_number-month_name-year time */
    if (sscanf(p,"%d-%63[^-]-%d %d:%d", &Day, month, &Year, &Hours, &Mins) == 5)
	goto gotit;

    /*   day_name, day_number-month_name-year time */
    if (sscanf(p,"%*s %d-%63[^-]-%d %d:%d",
	    &Day, month, &Year, &Hours, &Mins) == 5)
	goto gotit;

    /*   year_number-month_number-day_number time */
    if (sscanf(p, "%d-%d-%d %d:%d", &Year, &Month, &Day, &Hours, &Mins) == 5)
	goto gotit;

    /*   month_name day_number time year timezone */
    /*   (ctime, but without the day name)        */
    if (sscanf(p, "%63s %d %d:%d:%d %d %7s",
	    month, &Day, &Hours, &Mins, &Secs, &Year, Zone) >= 6)
	goto gotit;

    /* Microsoft Mail generates this travesty:             */
    /*   day_name day_number month_name timezone year time */
    if (sscanf(p,"%*s %d %63s %7[^1-9] %d %d:%d",
	    &Day, month, Zone, &Year, &Hours, &Mins) == 6)
	goto gotit;
    Zone[0] = 0;

    if (ison(glob_flags, WARNINGS))
	print(catgets( catalog, CAT_MSGS, 249, "Unknown date format: %s\n" ), p);
    return NULL;

gotit:
    if (Year > 1900)
	Year -= 1900;
#ifdef _WINDOWS
    if (Year < 70)
#else		
    else if (Year < 0) 
#endif    
    {
	if (ison(glob_flags, WARNINGS))
	    print(catgets( catalog, CAT_MSGS, 250, "Date garbled or bad year: %s\n" ), p);
	return NULL;
    }
    if (!Month && (Month = month_to_n(month)) == -1) {
	if (ison(glob_flags, WARNINGS))
	    print(catgets( catalog, CAT_MSGS, 251, "Date garbled or bad month: %s\n" ), p);
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

#ifndef MAC_OS
    (void) sprintf(month, "%10.10ld%s%s", seconds, Wkday, Zone);
#else /* MAC_OS */
    (void) sprintf(month, "%10.10lu%s%s", seconds, Wkday, Zone);
#endif /* MAC_OS */

#ifndef LICENSE_FREE
    if (today && ls_verify_date(today, seconds) < 0)
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 252, "License server access denied: %s" ), ls_err);
#endif /* LICENSE_FREE */

    return month;
}

/*
 * Convert year,month,day,hour,minute,second,timezone to internal date format.
 * Timezone is the only value represented as a string, because it usually is
 * taken to be the default.
 */
char *
vals_to_date(Date, Year, Month, Day, Hour, Minute, Second, Zone)
char *Date, *Zone;	/* Tim is 12:00:00[ap] format */
int Year, Month, Day, Hour, Minute, Second;
{
    char tz[16];
    struct tm T;
    time_t seconds;

    if (!Date)
	return 0;

    T = *time_n_zone(tz);
    if (!Zone || !*Zone)
	Zone = tz;
    if (Second >= 0)
	T.tm_sec = Second;
    if (Minute >= 0)
	T.tm_min = Minute;
    if (Hour >= 0)
	T.tm_hour = Hour;
    if (Day > 0)
	T.tm_mday = Day;
    if (Month > 0)
	T.tm_mon = Month - 1;
    if (Year > 0) { 
	T.tm_year = Year;
	if (T.tm_year > 1900)
	    T.tm_year -= 1900;
    }
    T.tm_isdst = 0;		/* determined from Zone */
#ifdef HAVE_TM_ZONE
    T.tm_zone = Zone;		/* not currently used; for completeness */
#endif /* HAVE_TM_ZONE */
    seconds = time2gmt(&T, Zone, 1);

#ifndef MAC_OS
    (void) sprintf(Date, "%10.10ld%.3s%s", seconds,
	day_names[day_number(T.tm_year, T.tm_mon+1, T.tm_mday)], Zone);
#else /* MAC_OS */
    (void) sprintf(Date, "%10.10lu%.3s%s", seconds,
	day_names[day_number(T.tm_year, T.tm_mon+1, T.tm_mday)], Zone);
#endif /* !MAC_OS */
    return Date;
}

/* Generate a date in internal format from the given strings.  Any strings
 * that are NULL or empty are filled in from the current local time.  The
 * generated date is copied into Date, which is returned.
 */
char *
strings_to_date(Date, Year, Month, Day, Tim, Zone)
char *Date, *Year, *Month, *Day, *Tim, *Zone;
{
    char tz[16], a_or_p = 0;
    struct tm T;

    if (!Date)
	return 0;

    T = *time_n_zone(tz);
    if (Tim && (sscanf(Tim, "%d:%d:%d%cm",
	    &T.tm_hour, &T.tm_min, &T.tm_sec, &a_or_p) >= 3 ||
	    (T.tm_sec = 0, sscanf(Tim, "%d:%d%cm",
			    &T.tm_hour, &T.tm_min, &a_or_p)) >= 2)) {
	if (a_or_p && lower(a_or_p) == 'p')
	    T.tm_hour += 12;
    }
    if (Day || !*Day)
	T.tm_mday = atol(Day);
    if (Month || !*Month) {
	if (!(T.tm_mon = atol(Month)))
	    T.tm_mon = month_to_n(Month) - 1;
	else
	    T.tm_mon -= 1;
    }
    if (Year || !*Year) {
	T.tm_year = atol(Year);
	if (T.tm_year > 1900)
	    T.tm_year -= 1900;
    }
    T.tm_isdst = 0;		/* determined from Zone */

    return vals_to_date(Date, T.tm_year, T.tm_mon+1, T.tm_mday,
			    T.tm_hour, T.tm_min, T.tm_sec, Zone);
}

/* pass a string in the standard date format, put into string.
 * return values in buffers provided they are not null.
 */
char *
date_to_string(Date, Year, Month, Day, Wkday, Tim, Zone, ret_buf)
char *Date, *Year, *Month, *Day, *Wkday, *Tim, *Zone, *ret_buf;
{
    time_t gmt;
    struct tm *T;
    char *am_or_pm, *p = ret_buf;

    Zone[0] = 0;
#ifndef MAC_OS
    (void) sscanf(Date, "%ld%3c%s", &gmt, Wkday, Zone);
#else /* MAC_OS */
    (void) sscanf(Date, "%lu%3c%s", &gmt, Wkday, Zone);
#endif /* MAC_OS */
    Wkday[3] = 0;
    gmt += getzoff(Zone);
    T = gmtime(&gmt);
    am_or_pm = (T->tm_hour < 12) ? catgets(catalog, CAT_MSGS, 247, "am")
				 : catgets(catalog, CAT_MSGS, 248, "pm");

    (void) sprintf(Year, "%d", T->tm_year + 1900);
    (void) sprintf(Day, "%d", T->tm_mday);
    (void) strcpy(Month, catgetref(local_month_names[T->tm_mon]));
    sprintf(p, "%s %2.d, ", Month, T->tm_mday);
    p += strlen(p);

    if (ison(glob_flags, MIL_TIME))
	(void) sprintf(p, "%2d:%02d",T->tm_hour,T->tm_min);
    else
	(void) sprintf(p, "%2.d:%02d%s",
	      (T->tm_hour)? (T->tm_hour <= 12)? T->tm_hour: T->tm_hour-12: 12,
	      T->tm_min, am_or_pm);
    (void) strcpy(Tim, p);

    return ret_buf;
}

/* pass a string in the internal zmail date format.
 * return pointer to static buffer holding ctime-format date.
 */
char *
date_to_ctime(Date)
char *Date;
{
    static char ret_buf[32];
    time_t gmt;

    ret_buf[0] = 0;
#ifndef MAC_OS
    (void) sscanf(Date, "%ld", &gmt);
#else /* MAC_OS */
    (void) sscanf(Date, "%lu", &gmt);
#endif /* MAC_OS */
    (void) strcpy(ret_buf, ctime(&gmt));

    return ret_buf;
}

/*
 * Build a date string according to the specification in the RFC for Date:
 */
char *
rfc_date(buf)
char buf[];
{
    struct tm *T;
    char zone[8];

    T = time_n_zone(zone);
#ifndef USA_TIMEZONES
    {
	long zoff_hr, zoff_sec = getzoff(zone);
	if (zoff_sec < 0) {
	    zone[0] = '-';
	    zoff_sec = -zoff_sec; 
	} else
	    zone[0] = '+';
	zoff_hr = zoff_sec / 3600;
	zoff_sec -= zoff_hr * 3600;
	(void) sprintf(&zone[1], "%02ld%02ld", zoff_hr, zoff_sec / 60);
    }
#endif /* !USA_TIMEZONES */

    (void) sprintf(buf, "%s, %d %s %d %02d:%02d:%02d %s",
	day_names[T->tm_wday],	/* day name */
	T->tm_mday,		/* day of the month */
	month_names[T->tm_mon],	/* month name */
	T->tm_year + 1900,	/* year number */
	T->tm_hour,		/* hours (24hr) */
	T->tm_min, T->tm_sec,	/* mins/secs */
	zone);			/* timezone */

    return buf;
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
