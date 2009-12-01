#include <stdio.h>
#include <isode/ldap/lber.h>
#include <isode/ldap/ldap.h>

char ldap_host[] = "x500.arc.nasa.gov";
int ldap_port = 389;
char base[] = "ou=Ames Research Center,o=National Aeronautics and Space
Administration,c=US";
char *attrs[] = {
    "mail",
    "postalAddress",
    0
};

main(argc, argv)
int argc;
char **argv;
{
    int maxresults;
    LDAP *ld;
    int iid;
    char filter[256];
    LDAPMessage *result = 0;
    int n;

    if (argc != 3) {
	fputs("Usage: lookup max-results address\n", stderr);
	exit(3);
    }
    maxresults = atoi(argv[1]);
    if (!maxresults) {
	fprintf(stderr, "Invalid max-results argument: %s\n", argv[1]);
	exit(1);
    }
    ld = ldap_open(ldap_host, ldap_port);
    if (!ld) {
	exit(1);
    }
    if (maxresults > 0)
	ld->ld_sizelimit = maxresults;
    if (ldap_bind(ld, NULL, NULL, LDAP_AUTH_SIMPLE) < 0) {
	ldap_perror(ld, "bind");
	exit(1);
    }
    sprintf(filter, "(&(commonname=*%s*)(mail=*))", argv[2]);
    iid = ldap_search(ld, base, LDAP_SCOPE_ONELEVEL, filter, attrs, 0);
    if (iid < 0) {
	ldap_perror(ld, "search");
	exit(1);
    }
    if (ldap_result(ld, iid, 1, (struct timeval *)0, &result) < 0) {
	ldap_perror(ld, "search");
	exit(1);
    }
    n = ldap_result2error(ld, result, 0);
    switch (n) {
	case LDAP_SUCCESS:
	case LDAP_NO_SUCH_OBJECT:
	    break;
	case LDAP_SIZELIMIT_EXCEEDED:
	    maxresults = 1;
	    break;
	default:
	    ldap_perror(ld, "search");
	    exit(1);
    }
    n = ldap_count_entries(ld, result);
    if (!n) {
	printf("%s\n", argv[2]);
	(void)ldap_unbind(ld);
	exit(4);
    }
    n = print_entries(ld, result);
    (void)ldap_unbind(ld);
    if (maxresults > 0 && n > maxresults)
	exit(2);
    if (n == 1)
	exit(5);
    exit(0);
}

int
print_entries(ld, result)
LDAP *ld;
LDAPMessage *result;
{
    LDAPMessage *entry;
    BerElement *cookie = 0;
    char *attr;
    char **value;
    char *dn, **rdn;
    int i;
    int n = 0;

    entry = ldap_first_entry(ld, result);
    while (entry) {
	dn = ldap_get_dn(ld, entry);
	rdn = ldap_explode_dn(dn, 1);
	attr = ldap_first_attribute(ld, entry, &cookie);
	while (attr) {
	    if (strcmp(attr, "mail") == 0) {
		value = ldap_get_values(ld, entry, attr);
		for (i = 0; value[i]; i++) {
		    printf("%s, %s\n", value[i], rdn[0]);
		    n++;
		}
		ldap_value_free(value);
	    } else {
		value = ldap_get_values(ld, entry, attr);
		fprintf(stderr, "attr %s: %s\n", attr, value[0]);
		ldap_value_free(value);
	    }
	    attr = ldap_next_attribute(ld, entry, cookie);
	}
	ldap_value_free(rdn);
	free(dn);
	entry = ldap_next_entry(ld, entry);
    }
    return n;
}
