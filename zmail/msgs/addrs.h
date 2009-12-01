#ifndef INCLUDE_MSGS_ADDRS_H
#define INCLUDE_MSGS_ADDRS_H

#include <general.h>

/* addrs.c 21/12/94 16.20.46 */
int compare_addrs P((char *list1, char *list2, char *ret_buf));
char *unscramble_addr P((char *addr, char *naddr));
char *bang_form P((char *d, char *s));
void route_addresses P((char *to, char *cc, char *route_path));
void improve_uucp_paths P((char *original,
	 int size,
	 char *route_path,
	 int fix_route));
void rm_cmts_in_addr P((register char *str));
void take_me_off P((char *str));
void fix_up_addr P((char *str, int safety));
void rm_redundant_addrs P((char *to, char *cc));
char *get_name_n_addr P((register const char *str,
	 register char *name,
	 register char *addr));
int append_address P((char *buf, const char *addr, int size));
int addrs_from_file P((char *buf, char *file, int size));
int addrs_from_alias P((char *buf, char *s, int size));
char *alias_to_address P((register const char *s));
char *wrap_addrs P((char *str, int n, int as822));
void prepare_mta_addrs P((char *str, unsigned long));
char *message_id P((char *buf));
char *message_boundary P((void));
char *get_from_host P((int use_domain, int not_ourhost));
char *get_full_hostname P((void));


#endif /* !INCLUDE_MSGS_ADDRS_H */
