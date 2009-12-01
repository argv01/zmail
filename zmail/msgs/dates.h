#ifndef INCLUDE_MSGS_DATES_H
#define INCLUDE_MSGS_DATES_H


#include <general.h>
#include "zctime.h"

/* dates.c 21/12/94 16.20.46 */
long getzoff P((char *zone));
time_t time2gmt P((struct tm *tym, char *zone, int how));
struct tm *time_n_zone P((char *zone));
/* return a string representation of the time */
char *tm_time_str P((register const char *, struct tm *));
/* return a string representation of the time */
char *time_str P((register const char *opts, time_t now));
int days_in_month P((int Year, int Month));
int day_number P((int Year, int Month, int Day));
/* parse an ascii date, and return message-id str */
char *parse_date P((register char *p, time_t today));
/* create an internal date from integer Y M D h m s */
char *vals_to_date P((char *Date,
	 int Year,
	 int Month,
	 int Day,
	 int Hour,
	 int Minute,
	 int Second,
	 char *Zone));
/* create an internal date from strings */
char *strings_to_date P((char *Date,
	 char *Year,
	 char *Month,
	 char *Day,
	 char *Tim,
	 char *Zone));
/* returns a string as described by parse_date() */
char *date_to_string P((char *Date,
	 char *Year,
	 char *Month,
	 char *Day,
	 char *Wkday,
	 char *Tim,
	 char *Zone,
	 char *ret_buf));
/* convert a date into ctime() format */
char *date_to_ctime P((char *Date));
/* create a date string compliant to RFC822 */
char *rfc_date P((char buf[]));
int month_to_n P((const char *name));


#endif /* INCLUDE_MSGS_DATES_H */
