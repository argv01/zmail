#ifdef SPTX21
#define _XOS_H_
#define NEED_U_LONG
#endif /* SPTX21 */

#include "dirserv.h"
#include "dynstr.h"
#include "config/features.h"
#include "strcase.h"
#include "uicomp.h"
#include "vars.h"
#include "zcstr.h"
#include "zmaddr.h"
#include "zmalloc.h"
#include "zmcomp.h"
#include "zmstring.h"
#include "zprint.h"


#ifndef lint
static char	uicomp_rcsid[] =
    "$Id: uicomp.c,v 1.12 1995/10/24 23:51:40 bobg Exp $";
#endif

char *uicomp_flavor_names[] = { "To:", "Cc:", "Bcc:", 0 };


enum uicomp_flavor
uicomp_seek_flavor(mixed)
    char **mixed;
{
    int flavor;

    for (flavor = (int)uicomp_To; flavor < (int)uicomp_Unknown; flavor++)
        if (!ci_strncmp(*mixed, uicomp_flavor_names[flavor],
                        strlen(uicomp_flavor_names[flavor]))) {
	    *mixed += strlen(uicomp_flavor_names[flavor]);
	    while (isspace(**mixed)) ++*mixed;
	    return (enum uicomp_flavor)flavor;
	}
    return uicomp_Unknown;
}


char *
uicomp_make_bland(edibles)
    const char *edibles;
{
    char **addressVector = addr_vec(edibles);
    if (addressVector) {
	char **scan;
	struct dynstr bland;
	dynstr_Init(&bland);
	
	for (scan = addressVector; *scan; scan++) {
	    char *addressPart = *scan;
	    uicomp_seek_flavor(&addressPart);
	    if (*addressPart) {
	    if (dynstr_Length(&bland)) dynstr_Append(&bland, ", ");
	    dynstr_Append(&bland, addressPart);
	    }
	}

	free_vec(addressVector);
	return dynstr_GiveUpStr(&bland);
    } else
	return 0;
}




enum uicomp_flavor
uicomp_predominant_flavor(triple)
    char **triple[3];
{
    return triple[uicomp_To] && !triple[uicomp_Cc] && !triple[uicomp_Bcc] ? uicomp_To
	: !triple[uicomp_To] &&  triple[uicomp_Cc] && !triple[uicomp_Bcc] ? uicomp_Cc
	: !triple[uicomp_To] && !triple[uicomp_Cc] &&  triple[uicomp_Bcc] ? uicomp_Bcc
	: uicomp_Unknown;
}


void
uicomp_vector_to_triple(initial, vector, final, triple)
    enum uicomp_flavor initial;
    char **vector;
    enum uicomp_flavor *final;
    char **triple[3];
{
    triple[uicomp_To] = triple[uicomp_Cc] = triple[uicomp_Bcc] = 0;
    if (initial == uicomp_Unknown) initial = uicomp_To;

    if (vector) {
	    char **scan;
	    for (scan = vector; *scan; scan++) {
		char *addressPart = *scan;
		
		enum uicomp_flavor flavor = uicomp_seek_flavor(&addressPart);
		if (flavor == uicomp_Unknown)
		    flavor = initial;
		else
		    initial = flavor;
		
		if (*addressPart)
		    vcatstr(&triple[(int)flavor], addressPart);
	    }
    };
    
    if (final)
	*final = initial;
}


void
uicomp_string_to_triple(initial, string, final, triple)
    enum uicomp_flavor initial;
    const char *string;
    enum uicomp_flavor *final;
    char **triple[3];
{
    char **vector = addr_vec(string);
    uicomp_vector_to_triple(initial, vector, final, triple);
    free_vec(vector);
}

    
char **
uicomp_triple_to_vector(triple, count)
    char **triple[3];
    unsigned int *count;
{
    int flavor;
    char **vector = 0;
    if (count) *count = 0;
    
    for (flavor = (int)uicomp_To; flavor < (int)uicomp_Unknown; flavor++)
	if (triple[flavor]) {
	    char **scan;
	    for (scan = triple[flavor]; *scan; scan++) {
		if (count) ++*count;
		vcatstr(&vector, zmVaStr("%s %s", uicomp_flavor_names[flavor], *scan));
	    }
	}

    return vector;
}


char *
uicomp_triple_to_string(triple, count)
    char **triple[3];
    unsigned int *count;
{
    int flavor;
    enum uicomp_flavor dominant = uicomp_predominant_flavor(triple);
    struct dynstr string;
    dynstr_Init(&string);
    
    for (flavor = (int)uicomp_To; flavor < (int)uicomp_Unknown; flavor++)
	if (triple[flavor]) {
	    char *joined = joinv(NULL, triple[flavor], ", ");
	    if (dynstr_Length(&string))
		dynstr_Append(&string, ", ");
	    if (flavor != (int)dominant) {
		dynstr_Append(&string, uicomp_flavor_names[flavor]);
		dynstr_AppendChar(&string, ' ');
	    }
	    dynstr_Append(&string, joined);
	    free(joined);
	}

    return dynstr_GiveUpStr(&string);
}


void
uicomp_free_triple(chaff)
    char **chaff[3];
{
    int flavor;
    for (flavor = (int)uicomp_To; (int)flavor < uicomp_Unknown; flavor++)
	free_vec(chaff[flavor]);
}


void
uicomp_triple_to_compose(triple, compose)
    char **triple[3];
    struct Compose *compose;
{
    if (compose) {
	set_address(compose,  TO_ADDR, triple[uicomp_To]);
	set_address(compose,  CC_ADDR, triple[uicomp_Cc]);
	set_address(compose, BCC_ADDR, triple[uicomp_Bcc]);
    }
}


void
uicomp_vector_to_compose(vector, compose)
    char **vector;
    struct Compose *compose;
{
    if (compose) {
	char **triple[3];
	uicomp_vector_to_triple(uicomp_Unknown, vector, NULL, triple);
	uicomp_triple_to_compose(triple, compose);
	uicomp_free_triple(triple);
    }
}


void
uicomp_compose_to_triple(compose, triple)
    struct Compose *compose;
    char **triple[3];
{
    if (compose) {
	triple[uicomp_To]  = get_address(compose,  TO_ADDR);
	triple[uicomp_Cc]  = get_address(compose,  CC_ADDR);
	triple[uicomp_Bcc] = get_address(compose, BCC_ADDR);
    } else {
	triple[uicomp_To]  = NULL;
	triple[uicomp_Cc]  = NULL;
	triple[uicomp_Bcc] = NULL;
    }

}


char **
uicomp_compose_to_vector(compose, count)
    struct Compose *compose;
    unsigned int *count;
{
    char **triple[3];
    char **vector;
    
    uicomp_compose_to_triple(compose, triple);
    vector = uicomp_triple_to_vector(triple, count);

    uicomp_free_triple(triple);
    return vector;
}


char *
uicomp_vector_to_string(vector)
    char **vector;
{
    char **triple[3];
    char *string;

    uicomp_vector_to_triple(uicomp_Unknown, vector, NULL, triple);

    string = uicomp_triple_to_string(triple, 0);

    uicomp_free_triple(triple);
    return string;
}


static void
make_obsolete(element, triple)
    char *element;
    char **triple[3];
{
    int flavor;
    
    for (flavor = (int)uicomp_To; (int)flavor < uicomp_Unknown; flavor++) {
	char **duplicate = vindex(triple[flavor], element);
	if (duplicate)
	    **duplicate = 0;
    }
}



static void
merge_triples(newer, older, merged)
    char **newer[3];
    char **older[3];
    char **merged[3];
{
    int flavor;
    
    for (flavor = (int)uicomp_To; flavor < (int)uicomp_Unknown; flavor++) {
	if (newer[flavor]) {
	    char **sweep;
	    for (sweep = newer[flavor]; *sweep; sweep++)
		make_obsolete(*sweep, older);
	}
    }

    for (flavor = (int)uicomp_To; flavor < (int)uicomp_Unknown; flavor++) {
	if (newer[flavor]) {
	    merged[flavor] = vdup(newer[flavor]);
	    if (older[flavor]) {
		char **sweep;
		for (sweep = older[flavor]; *sweep; sweep++)
		    if (**sweep)
			vcatstr(&merged[flavor], *sweep);
	    }
	} else if (older[flavor])
	    merged[flavor] = vdup(older[flavor]);
	else
	    merged[flavor] = 0;
    }
}


static const char *
expand_one(string, unalias, directory)
    const char *string;
    zmBool unalias;
    zmBool directory;
{
#ifdef DSERV
    if (directory)
	return address_book(string, unalias, False);
    else
#endif /* DSERV */
    if (unalias)
	return alias_to_address(string);
    else
	return string;
}


zmBool
uicomp_expand_triple(before, after, unalias, directory)
    char **before[3];
    char **after[3];
    zmBool unalias;
    zmBool directory;
{
    int flavor;
    zmBool changed = False;

    for (flavor = (int)uicomp_To; flavor < (int)uicomp_Unknown; flavor++) {
	after[flavor] = 0;
	    
	if (before[flavor]) {
	    char **sweep;
	    
	    for (sweep = before[flavor]; *sweep; sweep++) {
		const char *expanded = expand_one(*sweep, unalias, directory);
		if (expanded) {
		    if (!changed) changed = strcmp(*sweep, expanded);
		    vcat(&after[flavor], addr_vec(expanded));
		}
	    }
	}
    }
    return changed;
}	


void
uicomp_merge_triple(newer, compose)
    char **newer[3];
    Compose *compose;
{
    char **older[3];
    char **merged[3];
    
    uicomp_compose_to_triple(compose, older);
    merge_triples(newer, older, merged);
    uicomp_triple_to_compose(merged, compose);
    
    uicomp_free_triple(older);
    uicomp_free_triple(merged);
}
