/* @(#)dates.c	3.0	(c) copyright 3/01/90 (Dan Heller, Bart Schaefer) */

#include "mush.h"

/*
 *   %ld%3c%s	gmt_in_secs weekday orig_timezone
 * The standard "date format" stored in the msg data structure.
 */
char *day_names[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
char *month_names[] = {     /* imported in pick.c */
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
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

long
getzoff(zone)
char *zone;
{
    struct zoneoff *z;
    int hours, mins;
    char sign[2];

    if (!zone || !*zone)
	return 0;
    if (sscanf(zone, "%1[-+]%2d%2d", sign, &hours, &mins) == 3)
	return (hours * 3600 + mins * 60) * (*sign == '-' ? -1 : 1);
    for (z = time_zones; z->zname; z++)
	if (lcase_strncmp(zone, z->zname, -1) == 0)
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
 * Thanks to ktl@wag240.caltech.edu (Kian-Tat Lim) for similar algorithm
 * written in perl from which this was derived.
 */
long
time2gmt(tym, zone, how)
struct tm *tym;
char *zone;
int how;
{
    long year, julian;

    /* Y2K patch by JP <jeanpaul@duteca.et.tudelft.nl> */
    if (tym->tm_year < 1900)
	year = tym->tm_year + 1900;
    if (year < 1969)
	year += 100;

    julian = 365 * (year - 1970) + (int)((year - 1970 + 1) / 4) +
		mtbl[tym->tm_mon] + tym->tm_mday - 1;
		/* tym->tm_yday might not be valid */
    if (tym->tm_mon > 1 && year%4 == 0 && (year%100 != 0 || year%400 == 0))
	julian++;
    julian *= 86400;	/* convert to seconds */
    julian += (tym->tm_hour * 60 + tym->tm_min) * 60 + tym->tm_sec;
    return julian - getzoff(zone) * how;
}

struct tm *
time_n_zone(zone)
char *zone;
{
    struct tm *T;
    char *tz;
#if defined(SYSV) || defined(TIMEZONE)
    long	  x;

    (void) time(&x);
    T = localtime(&x);
#ifndef TIMEZONE
    {
	extern char *tzname[];
	tz = tzname[T->tm_isdst];
    }
#endif /* TIMEZONE */
#else /* SYSV || TIMEZONE */
    extern char     *timezone();
    struct timeval  mytime;
    struct timezone myzone;

    (void) gettimeofday(&mytime, &myzone);
    T = localtime(&mytime.tv_sec);
    tz = timezone(myzone.tz_minuteswest, (T->tm_isdst && myzone.tz_dsttime));
#endif /* !SYSV */

#ifdef TIMEZONE
#ifdef DAYLITETZ
    if (T->tm_isdst)
	tz = DAYLITETZ;
    else
#endif /* DAYLITETZ */
    tz = TIMEZONE;
#endif /* TIMEZONE */

    (void) strncpy(zone, tz, 7), zone[7] = 0;
    return T;
}

/* Time() returns a string according to criteria:
 *   if "now" is 0, then the current time is gotten and used.
 *       else, use the time described by now
 *   opts points to a string of args which is parsed until an unknown
 *       arg is found and opts will point to that upon return.
 *   valid args are T (time of day), D (day of week), M (month), Y (year),
 *       N (number of day in month -- couldn't think of a better letter).
 */
char *
Time(opts, now)
register char *opts;
long now;
{
    static char time_buf[30];
    struct tm 	  *T;
    register char *p = time_buf;
    long	  x;

    if (!opts)
	return NULL;
    if (now)
	x = now;
    else
	(void) time(&x);
    T = localtime(&x);
    for (;; opts++) {
	switch(*opts) {
	    case 'T':
		if (ison(glob_flags, MIL_TIME))
		    (void) sprintf(p, "%2d:%02d", T->tm_hour, T->tm_min);
		else
		    (void) sprintf(p, "%d:%02d", (T->tm_hour) ?
			  ((T->tm_hour <= 12) ? T->tm_hour : T->tm_hour - 12) :
			  12, T->tm_min);
	    when 'D': case 'W': (void) strcpy(p, day_names[T->tm_wday]);
	    when 'M': (void) strcpy(p, month_names[T->tm_mon]);
	    when 'y': (void) sprintf(p, "%d", T->tm_year);
	    when 'Y': (void) sprintf(p, "%d", T->tm_year + 1900);
	    when 'N': (void) sprintf(p, "%d", T->tm_mday);
	    otherwise: *--p = 0; return time_buf;
	}
	p += strlen(p);
	*p++ = ' ';
    }
}

/* parse date and return a string that looks like
 *   %ld%3c%s	gmt_in_secs weekday orig_timezone
 * This function is a bunch of scanfs on known date formats.  Don't
 * trust the "weekday" name fields because they may not be spelled
 * right, or have the correct punctuation.  Figure it out once the
 * year and month and date have been determined.
 */
char *
parse_date(p)
register char *p;
{
    /* When scanf-ing if month isn't a month, it could be a _long_ string.
     * this is also the static buffer whose address we return.
     */
    static char month[64];
    char Wkday[4], Zone[12], dst[4];
    char a_or_p;
    int Month = 0, Day = 0, Year = 0;
    int Hours = -1, Mins = -1, Secs = -1;
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
    if (sscanf(p, "%d %s %d %d:%d:%d %7s %3s",
	    &Day, month, &Year, &Hours, &Mins, &Secs, Zone, dst) >= 6 && Day)
	goto gotit;
    Zone[0] = dst[0] = 0;
    if (sscanf(p, "%d %s %d %d:%d %7s",
	    &Day, month, &Year, &Hours, &Mins, Zone) >= 5 && Day) {
	Secs = 0;
	goto gotit;
    }
    Zone[0] = dst[0] = 0;
    /*   day_name day_number month_name year_number time timezone */
    if (sscanf(p, "%*s %d %s %d %d:%d:%d %7s %3s",
	    &Day, month, &Year, &Hours, &Mins, &Secs, Zone, dst) >= 6 && Day)
	goto gotit;
    Zone[0] = dst[0] = 0;
    if (sscanf(p, "%*s %d %s %d %d:%d %7s %3s",
	    &Day, month, &Year, &Hours, &Mins, Zone, dst) >= 5 && Day) {
	Secs = 0;
	goto gotit;
    }
    Zone[0] = dst[0] = 0;

    /* Ctime format (From_ lines) -- timezone almost never found */

    /*   day_name month_name day_number time timezone year_number */
    if (sscanf(p, "%*s %s %d %d:%d:%d %7s %d",
	    month, &Day, &Hours, &Mins, &Secs, Zone, &Year) == 7)
	goto gotit;
    Zone[0] = 0;
    /*   day_name month_name day_number time year_number */
    if (sscanf(p, "%*s %s %d %d:%d:%d %d",
	    month, &Day, &Hours, &Mins, &Secs, &Year) == 6)
	goto gotit;
    /*   day_name month_name day_number time timezone dst year_number */
    if (sscanf(p, "%*s %s %d %d:%d:%d %7s %3s %d",
	    month, &Day, &Hours, &Mins, &Secs, Zone, dst, &Year) == 8)
	goto gotit;
    Zone[0] = dst[0] = 0;

    /* Other common variants */

    /*   day_number month_name year_number time-timezone (day) */
    /*                                       ^no colon separator */
    if (sscanf(p, "%d %s %d %2d%2d%1[-+]%6[0123456789]",
	    &Day, month, &Year, &Hours, &Mins, &Secs, &Zone[0], &Zone[1]) == 8)
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

    /*   day_name month_name day_number year_number time */
    if (sscanf(p, "%*s %s %d %d %d:%d:%d %7s",
	    month, &Day, &Year, &Hours, &Mins, &Secs, Zone) >= 6)
	goto gotit;
    Zone[0] = 0;
    if (sscanf(p, "%*s %s %d %d %d:%d %7s",
	    month, &Day, &Year, &Hours, &Mins, Zone) >= 5) {
	Secs = 0;
	goto gotit;
    }
    Zone[0] = 0;

    /*   day_name month_name day_number time timezone year_number */
    if (sscanf(p, "%*s %s %d %d:%d:%d %7s %d",
	    month, &Day, &Hours, &Mins, &Secs, Zone, &Year) == 7)
	goto gotit;
    Zone[0] = 0;
    if (sscanf(p, "%*s %s %d %d:%d %7s %d",
	    month, &Day, &Hours, &Mins, Zone, &Year) == 6) {
	Secs = 0;
	goto gotit;
    }
    Zone[0] = 0;

    Secs = 0;	/* For the next 3 attempts */

    /*   year_number-month_number-day_number time */
    if (sscanf(p, "%d-%d-%d %d:%d", &Year, &Month, &Day, &Hours, &Mins) == 5)
	goto gotit;

    /*   day_number-month_name-year time */
    if (sscanf(p,"%d-%[^-]-%d %d:%d", &Day, month, &Year, &Hours, &Mins) == 5)
	goto gotit;

    /*   day_name, day_number-month_name-year time */
    if (sscanf(p,"%*s, %d-%[^-]-%d %d:%d",
	    &Day, month, &Year, &Hours, &Mins) == 5)
	goto gotit;

    /*   month_name day_number time year Zone */
    /*   (ctime, but without the day name)    */
    if (sscanf(p, "%s %d %d:%d:%d %d %7s",
	    month, &Day, &Hours, &Mins, &Secs, &Year, Zone) >= 6)
	goto gotit;
    Zone[0] = 0;

    if (ison(glob_flags, WARNING))
	print("Unknown date format: %s\n", p);
    return NULL;

gotit:
    if (!lcase_strncmp(dst, "dst", -1)) {
	(void) strcat(Zone, " ");
	(void) strcat(Zone, dst);
    }
    if (Year > 1900)
	Year -= 1900;
    if (!Month && (Month = month_to_n(month)) == -1) {
	print("Bad month in date: %s%s", p,
	    *(p + strlen(p) - 1) == '\n' ? "" : "\n");
	return NULL;
    }
    if (Zone[0] == 0) {
	/* Use local time zone if none found -- important for date_recv */
	(void) time_n_zone(Zone);
    }
    {
	/* Lots of foolishness with casts for Xenix-286 16-bit ints */

	long days_ctr;	/* 16-bit ints overflowed Sept 12, 1989 */

    	days_ctr = ((long)Year * 365L) + ((Year + 3) / 4);
    	days_ctr += mtbl[Month-1] + Day + 6;
    	if (Month > 2 && (Year % 4 == 0))
	    days_ctr++;
    	(void) (sprintf(Wkday, "%.3s", day_names[(int)(days_ctr % 7L)]));
    }
    T.tm_sec = Secs;
    T.tm_min = Mins;
    T.tm_hour = Hours;
    T.tm_mday = Day;
    T.tm_mon = Month - 1;
    T.tm_year = Year;
    T.tm_wday = T.tm_yday = 0;	/* not used in time2gmt() */
    T.tm_isdst = 0;		/* determined from Zone */
    return sprintf(month, "%ld%s%s", time2gmt(&T, Zone, 1), Wkday, Zone);
}

/* pass a string in the standard date format, put into string.
 * return values in buffers provided they are not null.
 */
char *
date_to_string(Date, Yr, Mon, Day, Wkday, Tm, Zone, ret_buf)
char *Date, *Yr, *Mon, *Day, *Wkday, *Tm, *Zone, *ret_buf;
{
    long gmt;
    struct tm *T;
    char a_or_p, *p = ret_buf;

    Zone[0] = 0;
    (void) sscanf(Date, "%ld%3c%s", &gmt, Wkday, Zone);
    Wkday[3] = 0;
    gmt += getzoff(Zone);
    T = gmtime(&gmt);
    a_or_p = (T->tm_hour < 12)? 'a': 'p';

    (void) sprintf(Yr, "%d", T->tm_year + 1900);
    (void) sprintf(Day, "%d", T->tm_mday);
    (void) strcpy(Mon, month_names[T->tm_mon]);
    p += strlen(sprintf(p, "%s %2.d, ", Mon, T->tm_mday));

    if (ison(glob_flags, MIL_TIME))
	(void) sprintf(p, "%2d:%02d",T->tm_hour,T->tm_min);
    else
	(void) sprintf(p, "%2.d:%02d%cm",
	      (T->tm_hour)? (T->tm_hour <= 12)? T->tm_hour: T->tm_hour-12: 12,
	      T->tm_min, a_or_p);
    (void) strcpy(Tm, p);

    return ret_buf;
}

/* pass a string in the internal mush date format.
 * return pointer to static buffer holding ctime-format date.
 */
char *
date_to_ctime(Date)
char *Date;
{
    static char ret_buf[32];
    long gmt;

    ret_buf[0] = 0;
    (void) sscanf(Date, "%ld", &gmt);
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
#ifndef USA
    {
	long zoff_hr, zoff_sec = getzoff(zone);
	if (zoff_sec < 0) {
	    zone[0] = '-';
	    zoff_sec = -zoff_sec; 
	} else
	    zone[0] = '+';
	zoff_hr = zoff_sec / 3600;
	zoff_sec -= zoff_hr * 3600;
	(void) sprintf(&zone[1], "%02d%02d", zoff_hr, zoff_sec / 60);
    }
#endif /* !USA */

    return sprintf(buf, "%s, %d %s %d %02d:%02d:%02d %s",
	day_names[T->tm_wday],	/* day name */
	T->tm_mday,		/* day of the month */
	month_names[T->tm_mon],	/* month name */
	T->tm_year + 1900,	/* year number */
	T->tm_hour,		/* hours (24hr) */
	T->tm_min, T->tm_sec,	/* mins/secs */
	zone);			/* timezone */
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
month_to_n(name)
register char *name;
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
