/* 
 * $RCSfile: addrbook.c,v $
 * $Revision: 2.19 $
 * $Date: 2005/05/31 07:36:42 $
 * $Author: syd $
 */

#include <spoor.h>
#include <addrbook.h>

#include <dynstr.h>

#include <spoor/cmdline.h>
#include <spoor/textview.h>
#include <spoor/text.h>
#include <spoor/button.h>
#include <spoor/buttonv.h>
#include <spoor/toggle.h>
#include <spoor/wrapview.h>
#include <spoor/splitv.h>
#include <spoor/listv.h>
#include <spoor/list.h>
#include <spoor/menu.h>
#include <spoor/event.h>

#include <zmlite.h>
#include <dirserv.h>

#include "catalog.h"

#define Split spSplitview_Create
#define Wrap spWrapview_Create

#ifndef lint
static const char addrbook_rcsid[] =
    "$Id: addrbook.c,v 2.19 2005/05/31 07:36:42 syd Exp $";
#endif /* lint */
 struct spWrapview *w;
/* This structure information from the ldap resources file */
typedef struct LDAPDef {
    int  n_ldap_patterns;
    int  n_ldap_returns;
    int  n_ldap_visible;
    int  n_ldap_current_host;
    char ldap_host_address[MAX_LDAP_NAME+1];
    char ldap_host_name[MAX_LDAP_NAME+1];
    char ldap_search_base[MAX_LDAP_NAME+1];
    char ldap_options[MAX_LDAP_NAME+1];
    char ldap_name_index[MAX_LDAP_NAME+1];
    char ldap_password[MAX_LDAP_NAME+1];
    char ldap_attribute_name[MAX_LDAP_LINES][MAX_LDAP_NAME+1];
    char ldap_symbolic_name[MAX_LDAP_LINES][MAX_LDAP_NAME+1];
    char ldap_compare_type[MAX_LDAP_LINES][16];
    char ldap_returns_symbolic_name[MAX_LDAP_LINES][MAX_LDAP_NAME+1];
} LDAPDef;
 
/* A temporary place to read ldap resources into */
static LDAPDef ldap_def_read;

/* An array of ldap host names */
static char *ldap_host_names[MAX_LDAP_HOSTS+1];
 
/* An array of pointers to ldap host names */
static char ldap_host_array[MAX_LDAP_HOSTS+1][MAX_LDAP_NAME+1];
 
/* The name of the current ldap host */
static char the_current_ldap_host[MAX_LDAP_NAME+1] = "";

/* This variable is true when using LDAP */
static Boolean using_ldap;

/* This variable is used to detect changes in the ldap service */
char ldap_service[MAX_LDAP_NAME+1];

/*
  This function generates the command line arguments for the stand alone
  program lookup.ldap. lookup.ldap generates a ldap search script, executes
  the script, and formats the search results prior to sending then back
  here to be displayed.
*/

static void addrbook_activate();
static void addrbook_alter();

static 
void generate_ldap_search_pattern(ldap_search_pattern,n_patterns,ldap_pattern)
char *ldap_search_pattern;
int n_patterns;
char *ldap_pattern[MAX_LDAP_LINES];
{
int i , j;
  strcpy(ldap_search_pattern,"\"");
  if ((strcmp(ldap_def_read.ldap_host_address,"none") != 0) && (strlen(ldap_def_read.ldap_host_address) != 0))
    {
      strcat(ldap_search_pattern,"-h ");
      strcat(ldap_search_pattern,ldap_def_read.ldap_host_address);
      strcat(ldap_search_pattern," ");
    }
  if ((strcmp(ldap_def_read.ldap_search_base,"none") != 0) && (strlen(ldap_def_read.ldap_search_base) != 0))
    {
      strcat(ldap_search_pattern,"-b ");
      strcat(ldap_search_pattern,ldap_def_read.ldap_search_base);
      strcat(ldap_search_pattern," ");
    }
  if ((strcmp(ldap_def_read.ldap_options,"none") != 0) && (strlen(ldap_def_read.ldap_options) != 0))
    {
      strcat(ldap_search_pattern," ");
      strcat(ldap_search_pattern,ldap_def_read.ldap_options);
      strcat(ldap_search_pattern," ");
    }
  if ((strcmp(ldap_def_read.ldap_password,"none") != 0) && (strlen(ldap_def_read.ldap_password) != 0))
    {
      strcat(ldap_search_pattern,"-w ");
      strcat(ldap_search_pattern,ldap_def_read.ldap_password);
      strcat(ldap_search_pattern," ");
    }
  strcat(ldap_search_pattern,"'(");
  if (n_patterns == 1)
    {
      for (i=0;i<(ldap_def_read.n_ldap_visible);i++)
        if (ldap_pattern[i] != 0)
          if (strlen(ldap_pattern[i]) != 0)
            {
              strcat(ldap_search_pattern,ldap_def_read.ldap_symbolic_name[i]);
              strcat(ldap_search_pattern,ldap_def_read.ldap_compare_type[i]);
              strcat(ldap_search_pattern,ldap_pattern[i]);
              if (strcmp(ldap_def_read.ldap_compare_type[i],"=*") == 0)
                strcat(ldap_search_pattern,"*");
            }
    }
  else
    {
      for (i=0;i<n_patterns-2;i++)
        strcat(ldap_search_pattern,"&(");
      strcat(ldap_search_pattern,"&");
      j = 0;
      for (i=0;i<(ldap_def_read.n_ldap_visible);i++)
        if (ldap_pattern[i] != 0)
          if (strlen(ldap_pattern[i]) != 0)
            {
              strcat(ldap_search_pattern,"(");
              strcat(ldap_search_pattern,ldap_def_read.ldap_symbolic_name[i]);
              strcat(ldap_search_pattern,ldap_def_read.ldap_compare_type[i]);
              strcat(ldap_search_pattern,ldap_pattern[i]);
              if (strcmp(ldap_def_read.ldap_compare_type[i],"=*") == 0)
                strcat(ldap_search_pattern,"*");
              strcat(ldap_search_pattern,")");
              if ((j > 0) && (j < (n_patterns-1)))
                strcat(ldap_search_pattern,")");
              j++;
            }
    }
  strcat(ldap_search_pattern,")'");
  for (i=0;i<(ldap_def_read.n_ldap_returns);i++)
    {
      strcat(ldap_search_pattern," ");
      strcat(ldap_search_pattern,ldap_def_read.ldap_returns_symbolic_name[i]);
    }
  strcat(ldap_search_pattern,"\"");
}

/*
  This function returns the ldap name index as an integer. This
  function call should be preceeded by a call to load_ldap_resources.
*/
int get_ldap_name_index()
{
  return(atoi(ldap_def_read.ldap_name_index));
}

/*
  This function generates an search pattern to be used by ldapsearch for
  address verification. The variable the_name_index contains what is assumed
  to be the index of the persons name within the list of patterns. This
  function call should be preceeded by a call to load_ldap_resources.
*/
void generate_ldap_verification_pattern(ldap_search_pattern,name_to_verify,the_name_index)
char *ldap_search_pattern;
char *name_to_verify;
int  the_name_index;
{
int i , j;
 
  strcpy(ldap_search_pattern,"\"");
  if ((strcmp(ldap_def_read.ldap_host_address,"none") != 0) && (strlen(ldap_def_read.ldap_host_address) != 0))
    {
      strcat(ldap_search_pattern,"-h ");
      strcat(ldap_search_pattern,ldap_def_read.ldap_host_address);
      strcat(ldap_search_pattern," ");
    }
  if ((strcmp(ldap_def_read.ldap_search_base,"none") != 0) && (strlen(ldap_def_read.ldap_search_base) != 0))
    {
      strcat(ldap_search_pattern,"-b ");
      strcat(ldap_search_pattern,ldap_def_read.ldap_search_base);
      strcat(ldap_search_pattern," ");
    }
  if ((strcmp(ldap_def_read.ldap_options,"none") != 0) && (strlen(ldap_def_read.ldap_options) != 0))
    {
      strcat(ldap_search_pattern," ");
      strcat(ldap_search_pattern,ldap_def_read.ldap_options);
      strcat(ldap_search_pattern," ");
    }
  if ((strcmp(ldap_def_read.ldap_password,"none") != 0) && (strlen(ldap_def_read.ldap_password) != 0))
    {
      strcat(ldap_search_pattern,"-w ");
      strcat(ldap_search_pattern,ldap_def_read.ldap_password);
      strcat(ldap_search_pattern," ");
    }
  strcat(ldap_search_pattern,"'(");
  strcat(ldap_search_pattern,ldap_def_read.ldap_symbolic_name[the_name_index]);
  strcat(ldap_search_pattern,ldap_def_read.ldap_compare_type[the_name_index]);
  strcat(ldap_search_pattern,name_to_verify);
  if (strcmp(ldap_def_read.ldap_compare_type[the_name_index],"=*") == 0)
    strcat(ldap_search_pattern,"*");
  strcat(ldap_search_pattern,")'");
  for (i=0;i<(ldap_def_read.n_ldap_returns);i++)
    {
      strcat(ldap_search_pattern," ");
      strcat(ldap_search_pattern,ldap_def_read.ldap_returns_symbolic_name[i]);
    }
  strcat(ldap_search_pattern,"\"");
}

/*
  This function clears the ldap resources structure.
*/
static
void clear_ldap_resources()
{
int i;
  ldap_def_read.n_ldap_patterns=0;
  ldap_def_read.n_ldap_returns=0;
  ldap_def_read.n_ldap_visible = 0;
  strcpy(ldap_def_read.ldap_host_address,"");
  strcpy(ldap_def_read.ldap_host_name,"");
  strcpy(ldap_def_read.ldap_search_base,"");
  strcpy(ldap_def_read.ldap_options,"");
  strcpy(ldap_def_read.ldap_name_index,"0");
  strcpy(ldap_def_read.ldap_password,"");
  for (i=0;i<MAX_LDAP_LINES;i++)
    {
      strcpy(ldap_def_read.ldap_attribute_name[i],"");
      strcpy(ldap_def_read.ldap_symbolic_name[i],"");
      strcpy(ldap_def_read.ldap_compare_type[i],"");
      strcpy(ldap_def_read.ldap_returns_symbolic_name[i],"");
    }
}

/*
  This is a utility function to trim spaces off of both ends of a string.
*/
static
void trim_both_ends(str)
char *str;
{
  while (str[0] == ' ')
    strcpy(str,str+1);
  while (strlen(str)> 0)
    if ((str[strlen(str)-1] == ' ') || (str[strlen(str)-1] == '\n'))
      str[strlen(str)-1] = '\0';
    else
      break;
}

/*
  This is a utility function to pad a string with spaces.
*/
static
void pad_the_string(str,len)
char *str;
int len;
{
  while(strlen(str)<len)
    strcat(str," ");
}

/*
  This function returns the index of the host name passed to it or 0 if 
  not found.
*/
static int ldap_host_index(str)
char *str;
{
int i;
  for (i=0;i<ldap_def_read.n_ldap_current_host;i++)
    if (strcmp(str,ldap_host_array[i]) == 0)
      return(i);
  return(0);
}

/* 
  This function reads the selected resources into the ldap resources structure.
  It also builds a list of the names of the ldap host defined in the ldap
  resources file.
*/
Boolean load_ldap_resources(ldap_service_name,initial)
char *ldap_service_name;
int initial;
{
FILE *fp;
int i;
Boolean end_of_file , found_it;
char the_type[MAX_LDAP_NAME+1];
char trash[MAX_LDAP_NAME+1];
char zmlib_path[256];
char test_path[256];
int max_len = 0;
struct stat buf;

  if (getenv("ZMLIB") != NULL)
    {
      strcpy(zmlib_path,getenv("ZMLIB"));
      strcat(zmlib_path,"/");
      strcat(zmlib_path,LDAP_RESOURCES);
    }
  else
    {
      strcpy(zmlib_path,"./lib/");
      strcat(zmlib_path,LDAP_RESOURCES);
    }
  if (getenv("HOME") != NULL)
    {
      strcpy(test_path,getenv("HOME"));
      strcat(test_path,"/.");
      strcat(test_path,LDAP_RESOURCES);
      if (stat(test_path,&buf) == 0)
        strcpy(zmlib_path,test_path);
    }

/*
  Read it just to get the LDAP host names.
*/
  if (!(fp = fopen(zmlib_path, "r"))) {
    if (initial == 0)
      error(SysErrWarning, catgets( catalog, CAT_SHELL, 767, "Cannot find ldap resources" ));
    if (fp)
      (void) fclose(fp);
    return False;
  }
  i = 0;
  while (fscanf(fp,"%s",ldap_def_read.ldap_host_address) != EOF)
    {
      if (strcmp(ldap_def_read.ldap_host_address,"hostname:") != 0)
        continue;
      if (fscanf(fp,"%s",ldap_def_read.ldap_host_address) == EOF)
          break;
      if (fgets(ldap_def_read.ldap_host_name,MAX_LDAP_NAME,fp) == NULL)
          break;
      trim_both_ends(ldap_def_read.ldap_host_name);
      strcpy(ldap_host_array[i],ldap_def_read.ldap_host_name);
      ldap_host_names[i] = ldap_host_array[i];
      if (i < MAX_LDAP_HOSTS)
        i++;
    }
  ldap_host_names[i] = NULL;
  (void) fclose(fp);

/*
  Read it again to get the specific LDAP resources.
*/
  found_it = False;
  end_of_file = False;
  ldap_def_read.n_ldap_current_host = -1;
  clear_ldap_resources();
/* Open the file */
  if (!(fp = fopen(zmlib_path, "r"))) {
    if (initial == 0)
      error(SysErrWarning, catgets( catalog, CAT_SHELL, 767, "Cannot find ldap resources" ));
    if (fp)
      (void) fclose(fp);
    return False;
  }
/*
  Loop through the file reading the resources and storing them. 
*/
  while (!end_of_file)
    {
/* Read a key word */
      if (fscanf(fp,"%s",the_type) == EOF)
        break;
/* If the key word is a hostname ... */
      if (strcmp(the_type,"hostname:") == 0)
        {
/* If the host name previously read is a match I am finished */
          if (strcmp(ldap_service_name,ldap_def_read.ldap_host_name) == 0)
            break;
          clear_ldap_resources();
          if (fscanf(fp,"%s",ldap_def_read.ldap_host_address) == EOF)
            break;
          if (fgets(ldap_def_read.ldap_host_name,MAX_LDAP_NAME,fp) == NULL)
            break;
          trim_both_ends(ldap_def_read.ldap_host_name);
          ldap_def_read.n_ldap_current_host++;
        }
/* If the key word is a password ... */
      else if (strcmp(the_type,"password:") == 0)
        {
          if (fgets(ldap_def_read.ldap_password,MAX_LDAP_NAME,fp) == NULL)
            break;
          trim_both_ends(ldap_def_read.ldap_password);
        }
/* If the key word is a searchbase ... */
      else if (strcmp(the_type,"searchbase:") == 0)
        {
          if (fgets(ldap_def_read.ldap_search_base,MAX_LDAP_NAME,fp) == NULL)
            break;
          trim_both_ends(ldap_def_read.ldap_search_base);
        }
/* If the key word is a options ... */
      else if (strcmp(the_type,"options:") == 0)
        {
          if (fgets(ldap_def_read.ldap_options,MAX_LDAP_NAME,fp) == NULL)
            break;
          trim_both_ends(ldap_def_read.ldap_options);
        }
/* If the key word is a nameindex ... */
      else if (strcmp(the_type,"nameindex:") == 0)
        {
          if (fgets(ldap_def_read.ldap_name_index,MAX_LDAP_NAME,fp) == NULL)
            break;
          trim_both_ends(ldap_def_read.ldap_name_index);
        }
/* If the key word is a pattern ... */
      else if (strcmp(the_type,"pattern:") == 0)
        {
          if (fscanf(fp," %s ",ldap_def_read.ldap_symbolic_name[ldap_def_read.n_ldap_patterns]) == EOF)
            break;
          if (fscanf(fp," %s ",ldap_def_read.ldap_compare_type[ldap_def_read.n_ldap_patterns]) == EOF)
            break;
          if (fgets(ldap_def_read.ldap_attribute_name[ldap_def_read.n_ldap_patterns],MAX_LDAP_NAME,fp) == NULL)
            break;
          trim_both_ends(ldap_def_read.ldap_attribute_name[ldap_def_read.n_ldap_patterns]);
          if (strlen(ldap_def_read.ldap_attribute_name[ldap_def_read.n_ldap_patterns])>max_len)
            max_len = strlen(ldap_def_read.ldap_attribute_name[ldap_def_read.n_ldap_patterns]);
          if (ldap_def_read.n_ldap_patterns < MAX_LDAP_LINES)
            ldap_def_read.n_ldap_patterns++;
        }
/* If the key word is a return ... */
      else if (strcmp(the_type,"returns:") == 0)
        {
          if (fgets(ldap_def_read.ldap_returns_symbolic_name[ldap_def_read.n_ldap_returns],MAX_LDAP_NAME,fp) == NULL)
            break;
          trim_both_ends(ldap_def_read.ldap_returns_symbolic_name[ldap_def_read.n_ldap_returns]);
          if (ldap_def_read.n_ldap_returns < MAX_LDAP_LINES)
            ldap_def_read.n_ldap_returns++;
        }
/* If the line is a comment ... */
      else if (the_type[0] == '#')
        {
          if (fgets(trash,MAX_LDAP_NAME,fp) == NULL)
            break;
        }
/* Else trash it */
      else
        {
          if (fgets(trash,MAX_LDAP_NAME,fp) == NULL)
            break;
        }
    }

  fclose(fp);
  if (strcmp(ldap_service_name,ldap_def_read.ldap_host_name) == 0)
    found_it = True;
  if (ldap_def_read.n_ldap_returns < 1) {
    if (initial == 0)
      error(SysErrWarning, catgets( catalog, CAT_SHELL, 767, "No returns specified in resources" ));
    return False;
  }
  if (!found_it) {
    if (initial == 0)
      error(SysErrWarning, catgets( catalog, CAT_SHELL, 767, "No matching LDAP service" ));
    return False;
  }
  ldap_def_read.n_ldap_visible = ldap_def_read.n_ldap_patterns;
  for (i=0;i<ldap_def_read.n_ldap_patterns;i++)
    {
      pad_the_string(ldap_def_read.ldap_attribute_name[i],strlen(ldap_def_read.ldap_attribute_name[i])+1);
    }
  return True;
}
/*
  This function sets up a dummy ldap resources structure.
*/
static
void dummy_up_ldap_resources()
{
int i;
  ldap_def_read.n_ldap_patterns=1;
  ldap_def_read.n_ldap_returns=2;
  ldap_def_read.n_ldap_visible = 1;
  strcpy(ldap_def_read.ldap_host_address,"");
  strcpy(ldap_def_read.ldap_host_name,"");
  strcpy(ldap_def_read.ldap_search_base,"");
  strcpy(ldap_def_read.ldap_options,"");
  strcpy(ldap_def_read.ldap_name_index,"0");
  strcpy(ldap_def_read.ldap_password,"");
  strcpy(ldap_def_read.ldap_attribute_name[0], catgets(catalog, CAT_LITE, 22, "Pattern:"));
  strcpy(ldap_def_read.ldap_symbolic_name[0],"cn");
  strcpy(ldap_def_read.ldap_compare_type[0],"=*");
  strcpy(ldap_def_read.ldap_returns_symbolic_name[0],"cn");
  strcpy(ldap_def_read.ldap_returns_symbolic_name[1],"mail");
  for (i=1;i<MAX_LDAP_LINES;i++)
    {
      strcpy(ldap_def_read.ldap_attribute_name[i],"");
      strcpy(ldap_def_read.ldap_symbolic_name[i],"");
      strcpy(ldap_def_read.ldap_compare_type[i],"");
    }
  for (i=2;i<MAX_LDAP_LINES;i++)
    {
      strcpy(ldap_def_read.ldap_returns_symbolic_name[i],"");
    }
  strcpy(ldap_host_array[0],"host");
  ldap_host_names[0] = ldap_host_array[0];
  ldap_host_names[1] = NULL;
  ldap_def_read.n_ldap_current_host = 0;
}

/* The class descriptor */
struct spWclass *addrbook_class = 0;

static void
do_clear(self)
    struct addrbook *self;
{
int i;

    for (i=0;i<MAX_LDAP_LINES;i++)
      spSend(spView_observed(self->pattern_array[i]), m_spText_clear);
    spSend(spView_observed(self->recalls), m_spText_clear);
    spSend(self->addrlist, m_spText_clear);
    spSend(self->descrlist, m_spText_clear);
}

static void
do_search(self)
    struct addrbook *self;
{
    struct dynstr pattern, recalls , array_of_patterns[MAX_LDAP_LINES];
    char *ldap_pattern[MAX_LDAP_LINES];
    int x, i, j, k;
    char **hits, *desc, **strs, *ca;
    char ldap_search_pattern[MAX_LDAP_SEARCH_PATTERN];
    char *lmax;

    if (using_ldap)
      for (i=0;i<ldap_def_read.n_ldap_visible;i++)
        dynstr_Init(&array_of_patterns[i]);
    dynstr_Init(&pattern);
    dynstr_Init(&recalls);

    TRY {
        if (using_ldap)
          {
            j = -1;
            k = 0;
            for (i=0;i<ldap_def_read.n_ldap_visible;i++)
              {
	        spSend(spView_observed(self->pattern_array[i]), m_spText_appendToDynstr,
	           &array_of_patterns[i], 0, -1);
                if (dynstr_Str(&array_of_patterns[i]) != NULL)
                  {
                    if (j < 0)
                      j = k;
                    ldap_pattern[i] = dynstr_Str(&array_of_patterns[i]);
                    if (strlen(ldap_pattern[i]) != 0)
                      k++;
                  }
                else
                  ldap_pattern[i] = 0;
              }
	    spSend(spView_observed(self->pattern_array[j]), m_spText_appendToDynstr, &pattern, 0, -1); 
            generate_ldap_search_pattern(ldap_search_pattern,k,ldap_pattern);
          }
        else
	  spSend(spView_observed(self->pattern_array[0]), m_spText_appendToDynstr,
	       &pattern, 0, -1);
	spSend(spView_observed(self->recalls), m_spText_appendToDynstr,
	       &recalls, 0, -1);
	spSend(spView_observed(self->recalls), m_spText_clear);
	spSend(self->addrlist, m_spText_clear);
	spSend(self->descrlist, m_spText_clear);
        if (!(lmax = value_of(VarLookupMax)))
            lmax = "-1";            /* This is defined as "unlimited" */
	LITE_BUSY {
            if (using_ldap)
              {
   	        if (ca = fetch_cached_addr(dynstr_Str(&pattern)))
		    spSend(spView_observed(self->recalls), m_spText_insert,
		       0, -1, ca, spText_mNeutral);

	        x = lookup_run(ldap_search_pattern, 0, lmax, &hits);
              }
            else
              {
   	        if (ca = fetch_cached_addr(dynstr_Str(&pattern)))
		    spSend(spView_observed(self->recalls), m_spText_insert,
		       0, -1, ca, spText_mNeutral);
	        x = lookup_run(dynstr_Str(&pattern), 0, lmax, &hits);
              }
	} LITE_ENDBUSY;
	switch (x) {
	  case -1:
	    error(SysErrWarning,
                  catgets(catalog, CAT_LITE, 1, "Unable to read from address lookup: %s"),
		  dynstr_Str(&pattern));
	    /* Fall through */
	  case 0:		/* I have no idea */
	  case 5:		/* what these mean */
	    break;
	  case 4:
	    error(UserErrWarning, catgets(catalog, CAT_LITE, 2, "No names matched."));
	    free_vec(hits);
	    x = -1;
	    break;
	  default:
	    error(UserErrWarning, "%s", desc = joinv(0, hits, "\n"));
	    xfree(desc);
	    free_vec(hits);
	    x = -1;
	    break;
	}
	if (x >= 0) {
	    if (hits) {
		strs = lookup_split(hits, 0, 0);
		for (i = 0; hits[i]; ++i) {
		    spSend(self->descrlist, m_spList_append, hits[i]);
		}
		if (strs)
		    for (i = 0; strs[i]; ++i) {
			spSend(self->addrlist, m_spList_append, strs[i]);
		    }
		else
		    for (i = 0; hits[i]; ++i) {
			spSend(self->addrlist, m_spList_append, hits[i]);
		    }
	    }
	}
    } FINALLY {
        if (using_ldap)
          for (i=0;i<ldap_def_read.n_ldap_visible;i++)
    	    dynstr_Destroy(&array_of_patterns[i]);
  	dynstr_Destroy(&pattern);
	dynstr_Destroy(&recalls);
    } ENDTRY;
}

static void
chooseservice(self, service)
    struct addrbook *self;
    char *service;
{
    self->selected.service = service;
    spSend(spButtonv_button(self->service, 0), m_spButton_setLabel, service);
    if (strcmp(service,the_current_ldap_host) != 0)
      {
        strcpy(ldap_service,service);
        addrbook_activate(self,NULL);
      }
}
 
static void
service_select(self, arg)
    struct addrbook *self;
    spArgList_t arg;
{
    char *service = spArg(arg, char *);
    int i;
    struct spButtonv *bv;
 
    if (!service || !*service)
        return;
    bv = (struct spButtonv *) spMenu_Nth(self->service, 0)->content.menu;
    for (i = 0; i < spButtonv_length(bv); ++i) {
        if (!ci_strcmp(service, spButton_label(spButtonv_button(bv, i)))) {
            chooseservice(self, service);
            return;
        }
    }
}
 
static void
serviceActivate(self, button, ad, data)
    struct spMenu *self;
    struct spButton *button;
    struct addrbook *ad;
    GENERIC_POINTER_TYPE *data;
{
    chooseservice(ad, spButton_label(button));
}

static void
pattern_cb(c, str)
    struct spCmdline *c;
    char *str;
{
  do_search((struct addrbook *) spCmdline_obj(c));
}

static void
aa_search(b, self)
    struct spButton *b;
    struct addrbook *self;
{
    do_search(self);
}

static int
mail_fn(ev, im)
    struct spEvent *ev;
    struct spIm *im;
{
    char *cmd = (char *) spEvent_data(ev);

    ZCommand(cmd, zcmd_commandline);
    free(cmd);
    return (1);
}

static void
do_mail(self)
    struct addrbook *self;
{
    int i, n = 0, givenup = 0;
    struct dynstr d, d2;

    dynstr_Init(&d);
    dynstr_Init(&d2);
    TRY {
	dynstr_Set(&d, "\\mail ");
	for (i = 0; i < spSend_i(self->addrlist, m_spList_length); ++i) {
	    if (intset_Contains(spListv_selections(self->matches), i)) {
		dynstr_Set(&d2, "");
		spSend(self->addrlist, m_spList_getNthItem, i, &d2);
		if (n++)
		    dynstr_Append(&d, ", ");
		dynstr_Append(&d, quotezs(dynstr_Str(&d2), 0));
	    }
	}
	spSend(self, m_dialog_deactivate, dialog_Close);
	spSend(ZmlIm, m_spIm_enqueueEvent,
	       spEvent_Create((long) 0, (long) 0, 1,
			      mail_fn,
			      ((givenup = 1), dynstr_GiveUpStr(&d))));
    } FINALLY {
	if (!givenup)
	    dynstr_Destroy(&d);
	dynstr_Destroy(&d2);
    } ENDTRY;
}

static void
aa_done(b, self)
    struct spButton *b;
    struct addrbook *self;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
aa_mail(b, self)
    struct spButton *b;
    struct addrbook *self;
{
    do_mail(self);
}

static void
do_to(self)
    struct addrbook *self;
{
    struct dynstr d;
    int i;

    dynstr_Init(&d);
    TRY {
	for (i = 0; i < spSend_i(self->addrlist, m_spList_length); ++i) {
	    if (intset_Contains(spListv_selections(self->matches), i)) {
		dynstr_Set(&d, "");
		spSend(self->addrlist, m_spList_getNthItem, i, &d);
		ZCommand(zmVaStr("\\compcmd insert-header To: %s",
				 quotezs(dynstr_Str(&d), 0)),
			 zcmd_commandline);
	    }
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
}

static void
aa_to(b, self)
    struct spButton *b;
    struct addrbook *self;
{
    do_to(self);
}

static void
aa_cc(b, self)
    struct spButton *b;
    struct addrbook *self;
{
    struct dynstr d;
    int i;

    dynstr_Init(&d);
    TRY {
	for (i = 0; i < spSend_i(self->addrlist, m_spList_length); ++i) {
	    if (intset_Contains(spListv_selections(self->matches), i)) {
		dynstr_Set(&d, "");
		spSend(self->addrlist, m_spList_getNthItem, i, &d);
		ZCommand(zmVaStr("\\compcmd insert-header Cc: %s",
				 quotezs(dynstr_Str(&d), 0)),
			 zcmd_commandline);
	    }
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
}

static void
aa_bcc(b, self)
    struct spButton *b;
    struct addrbook *self;
{
    struct dynstr d;
    int i;

    dynstr_Init(&d);
    TRY {
	for (i = 0; i < spSend_i(self->addrlist, m_spList_length); ++i) {
	    if (intset_Contains(spListv_selections(self->matches), i)) {
		dynstr_Set(&d, "");
		spSend(self->addrlist, m_spList_getNthItem, i, &d);
		ZCommand(zmVaStr("\\compcmd insert-header Bcc: %s",
				 quotezs(dynstr_Str(&d), 0)),
			 zcmd_commandline);
	    }
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
}

static void
matches_cb(matches, which, clicktype)
    struct spListv *matches;
    int which;
    enum spListv_clicktype clicktype;
{
    if (clicktype == spListv_doubleclick) {
	struct addrbook *self = ((struct addrbook *)
				 spView_callbackData(matches));

	if (spoor_IsClassMember(spIm_view(ZmlIm),
				(struct spClass *) zmlcomposeframe_class)) {
	    do_to(self);
	    spSend(self, m_dialog_deactivate, dialog_Close);
	} else {
	    do_mail(self);
	}
    }
}

static void
remember_fn(menu, b, self, data)
    struct spMenu *menu;
    struct spButton *b;
    struct addrbook *self;
    GENERIC_POINTER_TYPE *data;
{
int i;
    struct dynstr d, d2;

    dynstr_Init(&d);
    dynstr_Init(&d2);
    TRY {
        if (using_ldap)
          {
            for (i=0;i<ldap_def_read.n_ldap_visible;i++)
              {
                spSend(spView_observed(self->pattern_array[i]), 
                       m_spText_appendToDynstr,
	               &d, 0, -1);
                if (dynstr_EmptyP(&d))
                  continue;
                else
                  break;
              }
          }
        else
	  spSend(spView_observed(self->pattern_array[0]), 
                 m_spText_appendToDynstr,
	         &d, 0, -1);
	spSend(spView_observed(self->recalls), m_spText_appendToDynstr,
	       &d2, 0, -1);
	if (dynstr_EmptyP(&d) || dynstr_EmptyP(&d2)) {
	    error(UserErrWarning,
		  catgets(catalog, CAT_LITE, 3, "Please supply both a search pattern and an address to remember"));
	} else {
	    xfree(uncache_address(dynstr_Str(&d)));
	    cache_address(dynstr_Str(&d), dynstr_Str(&d2), 1);
	}
    } FINALLY {
	dynstr_Destroy(&d);
	dynstr_Destroy(&d2);
    } ENDTRY;
}

static void
forget_fn(menu, b, self, data)
    struct spMenu *menu;
    struct spButton *b;
    struct addrbook *self;
    GENERIC_POINTER_TYPE *data;
{
int i;
    struct dynstr d, d2;
    char *oldpat = 0, *oldaddr = 0;

    dynstr_Init(&d);
    dynstr_Init(&d2);
    TRY {
        if (using_ldap)
          {
            for (i=0;i<ldap_def_read.n_ldap_patterns;i++)
              {
                spSend(spView_observed(self->pattern_array[i]), m_spText_appendToDynstr,
	       &d, 0, -1);
                if (dynstr_EmptyP(&d))
                  continue;
                else
                  break;
              }
          }
        else
	  spSend(spView_observed(self->pattern_array[0]), m_spText_appendToDynstr,
	       &d, 0, -1);
	spSend(spView_observed(self->recalls), m_spText_appendToDynstr,
	       &d2, 0, -1);
	if (dynstr_EmptyP(&d)) {
	    if (dynstr_EmptyP(&d2)) {
		error(UserErrWarning, catgets(catalog, CAT_LITE, 4, "Please supply address to forget"));
	    } else {
		oldpat = revert_address(oldaddr = dynstr_Str(&d2));
	    }
	} else {
	    oldaddr = uncache_address(oldpat = dynstr_Str(&d));
	}
        if (dynstr_EmptyP(&d)) {
		error(UserErrWarning, catgets(catalog, CAT_LITE, 4, "Please supply pattern to forget"));
        }
        else
	if (oldaddr || oldpat) {
	    error(Message, catgets(catalog, CAT_LITE, 5, "Forgotten:\n%s -->\n%s"),
		  oldpat,
		  (!oldaddr ?
		   catgets(catalog, CAT_LITE, 6, "Already forgotten or never remembered") :
		   ((strlen(oldaddr) > 50) ?
		    zmVaStr("%47s...", oldaddr) :
		    oldaddr)));
	}
    } FINALLY {
	if (oldpat != dynstr_Str(&d))
	    free(oldpat);
	if (oldaddr != dynstr_Str(&d2))
	    free(oldaddr);
	dynstr_Destroy(&d);
	dynstr_Destroy(&d2);
    } ENDTRY;
}

static void
aa_clear(b, self)
    struct spButton *b;
    struct addrbook *self;
{
    do_clear(self);
}

static void
forgetall_fn(menu, b, self, data)
    struct spMenu *menu;
    struct spButton *b;
    struct addrbook *self;
    GENERIC_POINTER_TYPE *data;
{
    address_cache_erase();
    do_clear(self);
}

static void
addrhelp_fn(menu, b, self, data)
    struct spMenu *menu;
    struct spButton *b;
    struct addrbook *self;
    GENERIC_POINTER_TYPE *data;
{
    zmlhelp("Address Browser");
}

static void
indexhelp_fn(menu, b, self, data)
    struct spMenu *menu;
    struct spButton *b;
    struct addrbook *self;
    GENERIC_POINTER_TYPE *data;
{
    zmlhelp("Help Index");
}

static void
aliases_fn(menu, b, self, data)
    struct spMenu *menu;
    struct spButton *b;
    struct addrbook *self;
    GENERIC_POINTER_TYPE *data;
{
    ZCommand("\\dialog compaliases", zcmd_ignore);
}

static void
display_cb(bv, which)
    struct spButtonv *bv;
    int which;
{
    struct addrbook *self = (struct addrbook *) spView_callbackData(bv);

    spButtonv_radioButtonHack(bv, which);
    if (spToggle_state(self->display_toggle)) {
	spSend(self->matches, m_spView_setObserved, self->descrlist);
    } else {
	spSend(self->matches, m_spView_setObserved, self->addrlist);
    }
}

static void
addrbook_initialize(self)
    struct addrbook *self;
{
int i;
struct spMenu *menu;
struct spButton *b;
char str[128];

    spWrapview_boxed(self) = 1;
    spWrapview_highlightp(self) = 1;
    if (!boolean_val(VarLookupService))
        return;

    using_ldap = boolean_val(VarUseLdap);
    strcpy(ldap_service,value_of(VarLdapService));
 
/* If using ldap and have not read default ldap resources, read them */
    if (strlen(the_current_ldap_host) == 0)
      {
        if (!load_ldap_resources(ldap_service,1) || (ldap_def_read.n_ldap_patterns == 0))
          {
            dummy_up_ldap_resources();
            strcpy(the_current_ldap_host,"host");
          }
        else
          strcpy(the_current_ldap_host,ldap_service);
      }
   if (!using_ldap) 
      {
        clear_ldap_resources();
        strcpy(the_current_ldap_host," ");
        ldap_def_read.n_ldap_patterns = 1;
        ldap_def_read.n_ldap_visible = 1;
        strcpy(ldap_def_read.ldap_attribute_name[0],"Pattern: ");
      }

    spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 18, "Address Browser"),
	   spWrapview_top);

    spSend(self->instructions = spTextview_NEW(), m_spView_setObserved,
	   self->host_label = spText_NEW());
    spSend(self->host_label, m_spText_insert, 0, -1,
	   catgets(catalog, CAT_LITE, 8, "Enter address Pattern and press Search."),
	   spText_mAfter);

    for (i=0;i<MAX_LDAP_LINES;i++)
      {
        self->pattern_array[i] = spCmdline_Create(pattern_cb);
        sprintf(str,"addrbrowse-pattern-field-%d",i);
        ZmlSetInstanceName(self->pattern_array[i] ,str , self);
        spCmdline_obj(self->pattern_array[i]) = (struct spoor *) self;
      }
    self->recalls = spCmdline_Create(0);
    ZmlSetInstanceName(self->recalls, "addrbrowse-recalls-field", self);
    self->addrlist = spList_NEW();
    self->descrlist = spList_NEW();
    spSend(self->matches = spListv_NEW(), m_spView_setObserved,
	   self->descrlist);
    ZmlSetInstanceName(self->matches, "addrbrowse-matches", self);
    spListv_callback(self->matches) = matches_cb;
    spView_callbackData(self->matches) = (GENERIC_POINTER_TYPE *) self;
    self->display = spButtonv_Create(spButtonv_horizontal,
				     (self->display_toggle =
				      spToggle_Create(catgets(catalog, CAT_LITE, 9, "Descriptions"),
						      0, 0, 1)),
				     spToggle_Create(catgets(catalog, CAT_LITE, 10, "Addresses"),
						     0, 0, 0),
				     0);
    ZmlSetInstanceName(self->display, "addrbrowse-display-rg", self);
    spButtonv_toggleStyle(self->display) = spButtonv_checkbox;
    spSend(self->display, m_spView_setWclass, spwc_Radiogroup);
    spButtonv_callback(self->display) = display_cb;
    spView_callbackData(self->display) = self;

    self->service = spMenu_NEW();
    ZmlSetInstanceName(self->service, "service-menu", self);
    spMenu_cancelfn(self->service) = 0;
    spButtonv_style(self->service) = spButtonv_vertical;
    menu = spMenu_NEW();
    spSend(menu, m_spView_setWclass, spwc_PullrightMenu);
    ZmlSetInstanceName(menu, "service-pullright", self);
    self->selected.service = 0;
    for (i = 0; ldap_host_names[i]; ++i) {
        b = spButton_Create(ldap_host_names[i], 0, 0);
        if (!(self->selected.service))
            self->selected.service = spButton_label(b);
        spSend(menu, m_spMenu_addFunction, b, serviceActivate, -1, self, 0);
    }
    spSend(self->service, m_spMenu_addMenu,
           spButton_Create(the_current_ldap_host, 0, 0), menu, 0);
    dialog_MUNGE(self) {
	spSend(self, m_dialog_setMenu,
	       spMenu_Create((struct spoor *) self, 0, spButtonv_horizontal,
			     catgets(catalog, CAT_LITE, 11, "Edit"), spMenu_menu,
			     spMenu_Create((struct spoor *) self, 0,
					   spButtonv_vertical,
					   catgets(catalog, CAT_LITE, 12, "Remember"),
					   spMenu_function,
					   remember_fn,
					   catgets(catalog, CAT_LITE, 13, "Forget"),
					   spMenu_function,
					   forget_fn,
					   catgets(catalog, CAT_LITE, 14, "Forget All"),
					   spMenu_function,
					   forgetall_fn,
					   0),
			     catgets(catalog, CAT_LITE, 15, "Options"), spMenu_menu,
			     spMenu_Create((struct spoor *) self, 0,
					   spButtonv_vertical,
					   catgets(catalog, CAT_LITE, 16, "Aliases ..."),
					   spMenu_function,
					   aliases_fn,
					   0),
			     catgets(catalog, CAT_LITE, 17, "Help"), spMenu_menu,
			     spMenu_Create((struct spoor *) self, 0,
					   spButtonv_vertical,
					   catgets(catalog, CAT_LITE, 18, "Address Browser"),
					   spMenu_function,
					   addrhelp_fn,
					   catgets(catalog, CAT_LITE, 19, "Index"),
					   spMenu_function,
					   indexhelp_fn,
					   0),
			     0));
        if (using_ldap)
          strcpy(str, catgets(catalog, CAT_LITE, 21, "Service: "));
        else
          strcpy(str," ");
	spSend(self, m_dialog_setView,
	       Split(self->instructions,
		     Split(self->service_wrap = Wrap(self->service,
				0, 0, str, 0,
				0, 0, 0),
		     Split(self->pattern_array_wrap[0] = Wrap(self->pattern_array[0],
				0, 0, ldap_def_read.ldap_attribute_name[0], 0,
				0, 0, 0),
		     Split(self->pattern_array_wrap[1] = Wrap(self->pattern_array[1],
				0, 0, ldap_def_read.ldap_attribute_name[1], 0,
				0, 0, 0),
		     Split(self->pattern_array_wrap[2] = Wrap(self->pattern_array[2],
				0, 0, ldap_def_read.ldap_attribute_name[2], 0,
				0, 0, 0),
		     Split(self->pattern_array_wrap[3] = Wrap(self->pattern_array[3],
				0, 0, ldap_def_read.ldap_attribute_name[3], 0,
				0, 0, 0),
		     Split(self->pattern_array_wrap[4] = Wrap(self->pattern_array[4],
				0, 0, ldap_def_read.ldap_attribute_name[4], 0,
				0, 0, 0),
			   Split(Wrap(self->recalls,
				      0, 0, catgets(catalog, CAT_LITE, 27, "Recalls: "), 0,
				      0, 0, 0),
				 Split(Wrap(self->matches,
					    0, 0, catgets(catalog, CAT_LITE, 28, "Matches: "), 0,
					    0, 1, 0),
				       Wrap(self->display,
					    0, 0, catgets(catalog, CAT_LITE, 29, "Display: "), 0,
					    0, 0, 0),
				       1, 1, 0,
				       spSplitview_topBottom,
				       spSplitview_plain, 0),
				 1, 0, 0,
				 spSplitview_topBottom,
				 spSplitview_plain, 0),
			   1, 0, 0,
			   spSplitview_topBottom,
			   spSplitview_plain, 0),
			   1, 0, 0,
			   spSplitview_topBottom,
			   spSplitview_plain, 0),
			   1, 0, 0,
			   spSplitview_topBottom,
			   spSplitview_plain, 0),
			   1, 0, 0,
			   spSplitview_topBottom,
			   spSplitview_plain, 0),
			   1, 0, 0,
			   spSplitview_topBottom,
			   spSplitview_plain, 0),
			   1, 0, 0,
			   spSplitview_topBottom,
			   spSplitview_plain, 0),
		     1, 0, 0,
		     spSplitview_topBottom,
		     spSplitview_boxed, spSplitview_plain));
    } dialog_ENDMUNGE;

    spSend(self, m_dialog_clearFocusViews);
    if (using_ldap)
      spSend(self, m_dialog_addFocusView, self->service);
    for (i=0;i<ldap_def_read.n_ldap_visible;i++)
      spSend(self, m_dialog_addFocusView, self->pattern_array[i]);
    spSend(self, m_dialog_addFocusView, self->recalls);
    spSend(self, m_dialog_addFocusView, self->matches);
    spSend(self, m_dialog_addFocusView, self->display);

    ZmlSetInstanceName(self, "addrbrowse", self);
}

static void
addrbook_finalize(self)
    struct addrbook *self;
{
printf("finalizing\n");
    /* Code to finalize a struct addrbook */
}

static void
aa_less(b, self)
    struct spButton *b;
    struct addrbook *self;
{
  if (ldap_def_read.n_ldap_patterns > 1)
    addrbook_alter(self);
}

static void
addrbook_alter(self)
    struct addrbook *self;
{
int i;
char str[128];
char button_label[5];
    struct spButtonv *aa;

    ldap_def_read.n_ldap_visible--;
    if (ldap_def_read.n_ldap_visible <= 0)
      ldap_def_read.n_ldap_visible = ldap_def_read.n_ldap_patterns;
    if (ldap_def_read.n_ldap_visible == 1)
      strcpy(button_label,catgets(catalog, CAT_LITE, 913, "More"));
    else
      strcpy(button_label,catgets(catalog, CAT_LITE, 914, "Less"));

    if (spoor_IsClassMember(spIm_view(ZmlIm),
			    (struct spClass *) zmlcomposeframe_class)) {
	spSend(self, m_spView_setWclass, spwc_ComposeAddrbrowse);
	aa = (struct spButtonv *) spSend_p(self, m_dialog_setActionArea,
					   ActionArea(self,
						      catgets(catalog, CAT_LITE, 30, "Done"), aa_done,
						      catgets(catalog, CAT_LITE, 31, "Search"), aa_search,
						      catgets(catalog, CAT_LITE, 32, "Clear"), aa_clear,
						      catgets(catalog, CAT_LITE, 33, "To"), aa_to,
						      catgets(catalog, CAT_LITE, 34, "Cc"), aa_cc,
						      catgets(catalog, CAT_LITE, 35, "Bcc"), aa_bcc,
						      button_label, aa_less,
						      0));
	spButtonv_selection(dialog_actionArea(self)) = 3; /* To */
    } else {
	spSend(self, m_spView_setWclass, spwc_Addrbrowse); /* should be
							    * redundant */
	aa = (struct spButtonv *) spSend_p(self, m_dialog_setActionArea,
					   ActionArea(self,
						      catgets(catalog, CAT_LITE, 30, "Done"), aa_done,
						      catgets(catalog, CAT_LITE, 31, "Search"), aa_search,
						      catgets(catalog, CAT_LITE, 32, "Clear"), aa_clear,
						      catgets(catalog, CAT_LITE, 33, "Mail"), aa_mail,
						      button_label, aa_less,
						      0));
	spButtonv_selection(dialog_actionArea(self)) = 3; /* Mail */
    }
    if (aa) {
	spSend(aa, m_spView_destroyObserved);
	spoor_DestroyInstance(aa);
    }
    ZmlSetInstanceName(dialog_actionArea(self), "addrbrowse-aa", self);
    spSuper(addrbook_class, self, m_dialog_activate); 
 
    for (i=0;i<MAX_LDAP_LINES;i++)
      {
        if (i>(ldap_def_read.n_ldap_visible-1))
          {
            spSend(self->pattern_array_wrap[i], m_spWrapview_setLabel,
              "", 
              spWrapview_left); 
            spSend(spView_observed(self->pattern_array[i]), m_spText_clear);
          }
        else
          spSend(self->pattern_array_wrap[i], m_spWrapview_setLabel,
                ldap_def_read.ldap_attribute_name[i], 
                spWrapview_left); 
      }
    spSend(self, m_dialog_clearFocusViews);
    if (using_ldap)
      {
        spSend(self, m_dialog_addFocusView, self->service);
        spSend(self->service_wrap, m_spWrapview_setLabel,catgets(catalog, CAT_LITE, 915, "Service: "), spWrapview_left); 
      }
    else
      spSend(self->service_wrap, m_spWrapview_setLabel," ", spWrapview_left); 
    for (i=0;i<ldap_def_read.n_ldap_visible;i++)
      spSend(self, m_dialog_addFocusView, self->pattern_array[i]);
    spSend(self, m_dialog_addFocusView, self->recalls);
    spSend(self, m_dialog_addFocusView, self->matches);
    spSend(self, m_dialog_addFocusView, self->display);
    spSend(self->pattern_array[0], m_spView_wantFocus, self->pattern_array[0]);
}

static void
addrbook_activate(self, arg)
    struct addrbook *self;
    spArgList_t arg;
{
int i;
    struct spButtonv *aa;

    if (spoor_IsClassMember(spIm_view(ZmlIm),
			    (struct spClass *) zmlcomposeframe_class)) {
	spSend(self, m_spView_setWclass, spwc_ComposeAddrbrowse);
      if (boolean_val(VarUseLdap))
	aa = (struct spButtonv *) spSend_p(self, m_dialog_setActionArea,
					   ActionArea(self,
						      catgets(catalog, CAT_LITE, 29, "Done"), aa_done,
						      catgets(catalog, CAT_LITE, 30, "Search"), aa_search,
						      catgets(catalog, CAT_LITE, 31, "Clear"), aa_clear,
						      catgets(catalog, CAT_LITE, 32, "To"), aa_to,
						      catgets(catalog, CAT_LITE, 33, "Cc"), aa_cc,
						      catgets(catalog, CAT_LITE, 34, "Bcc"), aa_bcc,
						      catgets(catalog, CAT_LITE, 35, "Less"), aa_less,
						      0));
      else 
	aa = (struct spButtonv *) spSend_p(self, m_dialog_setActionArea,
					   ActionArea(self,
						      catgets(catalog, CAT_LITE, 29, "Done"), aa_done,
						      catgets(catalog, CAT_LITE, 30, "Search"), aa_search,
						      catgets(catalog, CAT_LITE, 31, "Clear"), aa_clear,
						      catgets(catalog, CAT_LITE, 32, "To"), aa_to,
						      catgets(catalog, CAT_LITE, 33, "Cc"), aa_cc,
						      catgets(catalog, CAT_LITE, 34, "Bcc"), aa_bcc,
						      0));
	spButtonv_selection(dialog_actionArea(self)) = 3; /* To */
    } else {
	spSend(self, m_spView_setWclass, spwc_Addrbrowse); /* should be
							    * redundant */
      if (boolean_val(VarUseLdap))
	aa = (struct spButtonv *) spSend_p(self, m_dialog_setActionArea,
					   ActionArea(self,
						      catgets(catalog, CAT_LITE, 29, "Done"), aa_done,
						      catgets(catalog, CAT_LITE, 30, "Search"), aa_search,
						      catgets(catalog, CAT_LITE, 31, "Clear"), aa_clear,
						      catgets(catalog, CAT_LITE, 32, "Mail"), aa_mail,
						      catgets(catalog, CAT_LITE, 33, "Less"), aa_less,
						      0));
      else
	aa = (struct spButtonv *) spSend_p(self, m_dialog_setActionArea,
					   ActionArea(self,
						      catgets(catalog, CAT_LITE, 29, "Done"), aa_done,
						      catgets(catalog, CAT_LITE, 30, "Search"), aa_search,
						      catgets(catalog, CAT_LITE, 31, "Clear"), aa_clear,
						      catgets(catalog, CAT_LITE, 32, "Mail"), aa_mail,
						      0));
	spButtonv_selection(dialog_actionArea(self)) = 3; /* Mail */
    }
    if (aa) {
	spSend(aa, m_spView_destroyObserved);
	spoor_DestroyInstance(aa);
    }
    ZmlSetInstanceName(dialog_actionArea(self), "addrbrowse-aa", self);
    spSuper(addrbook_class, self, m_dialog_activate); 
 
/* If using_ldap ldap_service changed then change the dialog */
    if ((using_ldap != boolean_val(VarUseLdap)) ||
        (strcmp(ldap_service,the_current_ldap_host) != 0))
      {
        using_ldap = boolean_val(VarUseLdap);
        if (using_ldap)
          {
            if (!load_ldap_resources(ldap_service,0) || (ldap_def_read.n_ldap_patterns == 0))
              {
                dummy_up_ldap_resources();
                strcpy(the_current_ldap_host,"host");
                error(SysErrWarning, catgets( catalog, CAT_SHELL, 767, "Cannot locate LDAP host" ));
              }
            else
              strcpy(the_current_ldap_host,ldap_service);
          }
        if (!using_ldap) 
          {
            clear_ldap_resources();
            ldap_def_read.n_ldap_patterns = 1;
            ldap_def_read.n_ldap_visible = 1;
            strcpy(ldap_def_read.ldap_attribute_name[0],catgets(catalog, CAT_SHELL, 928, "Pattern: "));
            strcpy(the_current_ldap_host," ");
          }
        do_clear(self);
        spSend(spButtonv_button(self->service, 0), m_spButton_setLabel, the_current_ldap_host);
        for (i=0;i<MAX_LDAP_LINES;i++)
          spSend(self->pattern_array_wrap[i], m_spWrapview_setLabel,
                ldap_def_read.ldap_attribute_name[i], 
                spWrapview_left); 
        spSend(self, m_dialog_clearFocusViews);
        if (using_ldap)
          {
            spSend(self, m_dialog_addFocusView, self->service);
            spSend(self->service_wrap, m_spWrapview_setLabel,catgets(catalog, CAT_SHELL, 929, "Service: "), spWrapview_left); 
          }
        else
          spSend(self->service_wrap, m_spWrapview_setLabel," ", spWrapview_left); 
        for (i=0;i<ldap_def_read.n_ldap_visible;i++)
          spSend(self, m_dialog_addFocusView, self->pattern_array[i]);
        spSend(self, m_dialog_addFocusView, self->recalls);
        spSend(self, m_dialog_addFocusView, self->matches);
        spSend(self, m_dialog_addFocusView, self->display);
      }
      spSend(self->pattern_array[0], m_spView_wantFocus, self->pattern_array[0]);
}

static void
addrbook_desiredSize(self, arg)
    struct addrbook *self;
    spArgList_t arg;
{
    int *minh, *minw, *maxh, *maxw, *besth, *bestw;
    int screenw = 80, screenh = 24;

    minh = spArg(arg, int *);
    minw = spArg(arg, int *);
    maxh = spArg(arg, int *);
    maxw = spArg(arg, int *);
    besth = spArg(arg, int *);
    bestw = spArg(arg, int *);

    spSuper(addrbook_class, self, m_spView_desiredSize,
	    minh, minw, maxh, maxw, besth, bestw);
    if (spView_window(ZmlIm))
	spSend(spView_window(ZmlIm), m_spWindow_size, &screenh, &screenw);
    *besth = screenh; 
    *bestw = screenw - 10;
}

struct spWidgetInfo *spwc_Addrbrowse = 0;
struct spWidgetInfo *spwc_ComposeAddrbrowse = 0;

void
addrbook_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (addrbook_class) 
	return; 

    addrbook_class =
	spWclass_Create("addrbook",
			catgets(catalog, CAT_LITE, 18, "Address Browser"),
			(struct spClass *) dialog_class,
			(sizeof (struct addrbook)),
			addrbook_initialize,
			addrbook_finalize,
			spwc_Addrbrowse = spWidget_Create("Addrbrowse",
							  spwc_MenuPopup));
    spwc_ComposeAddrbrowse = spWidget_Create("ComposeAddrbrowse",
					     spwc_Addrbrowse);

    /* Override inherited methods */
    spoor_AddOverride(addrbook_class,
		      m_dialog_activate, NULL,
		      addrbook_activate);
    spoor_AddOverride(addrbook_class,
		      m_spView_desiredSize, NULL,
		      addrbook_desiredSize);

    /* Add new methods */

    /* Initialize classes on which the addrbook class depends */
    spEvent_InitializeClass();
    spCmdline_InitializeClass();
    spTextview_InitializeClass();
    spText_InitializeClass();
    spButton_InitializeClass();
    spButtonv_InitializeClass();
    spToggle_InitializeClass();
    spWrapview_InitializeClass();
    spSplitview_InitializeClass();
    spListv_InitializeClass();
    spList_InitializeClass();
    spMenu_InitializeClass();

    /* Initialize class-specific data */
}
