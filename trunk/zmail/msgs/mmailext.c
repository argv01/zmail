/* Copyright (c) 1993 Z-Code Software Corp. */
/*
 * mmailext.c --
 *	 
 *	 This file consists of routines derived from metamail code,
 *	 which are useful for handling MIME messages.
 *	 
 *						C.M. Lowery, 1993
 */
/*
  Copyright (c) 1991 Bell Communications Research, Inc. (Bellcore)
  
  Permission to use, copy, modify, and distribute this material 
  for any purpose and without fee is hereby granted, provided 
  that the above copyright notice and this permission notice 
  appear in all copies, and that the name of Bellcore not be 
  used in advertising or publicity pertaining to this 
  material without the specific, prior written permission 
  of an authorized representative of Bellcore.  BELLCORE 
  MAKES NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY 
  OF THIS MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS", 
  WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES.
  */
/****************************************************** 
  Metamail -- A tool to help diverse mail readers 
  cope with diverse multimedia mail formats.
  
  Author:  Nathaniel S. Borenstein, Bellcore
  
  ******************************************************* */


#ifndef lint
static char	mmail_rcsid[] = "$Id: mmailext.c,v 2.33 2005/05/09 09:15:20 syd Exp $";
#endif

#include "zmail.h"
#include "mmailext.h"
#include "catalog.h"
#include "strcase.h"

/*
 *	Global data types and variables
 */

/*
 *	Forward declarations
 */

/*
 *	Functions
 */

void
StripTrailingSpace(s)
    char *s;
{
    char *t = s+strlen(s) -1;
    while (isspace((unsigned char) *t) && (t >= s)) *t-- = 0;
}

char *
StripLeadingSpace(s)
    char *s;
{
    char	*tmp = s, *orig = s;

    while (*tmp && isspace((unsigned char) *tmp)) tmp++;
    if (tmp > s) while (*s++ = *tmp++);
    return (orig);
}

char *
Cleanse(s) /* no leading or trailing space, all lower case */
    char	*s;
{
    char	*tmp;
    
    /* strip leading white space */
    tmp = s;
    while (*tmp && isspace((unsigned char) *tmp)) tmp++;
    strcpy(s, tmp);
    /* put in lower case */
    for (tmp=s; *tmp; ++tmp)
        Lower(*tmp);
    /* strip trailing white space */
    while (*--tmp && isspace((unsigned char) *tmp)) *tmp = 0;
    return(s);
}

char *
UnquoteString(s)
    char *s;
{
    char *ans, *t;
    
    if (*s != '"') return(s);
    ans = (char *) malloc(1+strlen(s));
    if (!ans)
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 642, "UnquoteString: cannot allocate space.\n" ));
    ++s;
    t = ans;
    while (*s) {
        if (*s == '\\') {
            *t++ = *++s;
        } else if (*s == '"') {
            break;
        } else {
            *t++ = *s;
        }
        ++s;
    }
    *t = 0;
    return(ans);
}

/* Copy the string, replacing unsafe characters with underscores. */
int
strcpyStrip(t, f, stripFull)
    char *t;
    const char *f;
    int	stripFull;
{
    int	modifiedStrFlag = 0;
    
    while (*f) {
	if (*f == '\"' || *f == '\'' || *f == '`' || *f == ';' ||
	    *f == '|' || *f == '^' || *f == '&' || *f == '$' ||
	    *f == '<' || *f == '>' || *f == '(' || *f == ')' ||
	    /* isspace(*f) || */
	    (stripFull &&
		(*f == '*' || *f == '?' || *f == '[' || *f == ']'))) {
	    *t++='_';
	    modifiedStrFlag = 1;
	} else {
	    *t++ = *f; 
	}
	++f;
    }
    *t='\0';
    return(modifiedStrFlag);
}

char *
paramend(s)
    char	*s;
{
    int inquotes=0;
    while (*s) {
        if (inquotes) {
            if (*s == '"') {
                inquotes = 0;
            } else if (*s == '\\') {
                ++s; /* skip a char */
            }
        } else if (*s == ';') {
            return(s);
        } else if (*s == '"') {
            inquotes = 1;
        }
        ++s;
    }
    return(NULL);
}        

/* 
 * Add the given content parameter to the params structure.  Allocate
 * the necessary space.
 */
void
AddContentParameter(params, name, value)
    mimeContentParams	*params;
    const char		*name;
    const char		*value;
{
/*
  Do not add blank charsets
*/
    if (!ci_strcmp(name,"charset"))
      if (strlen(value) <= 0)
        {
          Debug("Attempted to add blank charset\n");
          return;
        }
    if (name) {
	if (params) {
	    int i;
	    for (i = 0; i < params->numParamsUsed; ++i) {
		if (!ci_strcmp(name, params->paramNames[i])) {
		    free(params->paramValues[i]);
		    params->paramValues[i] = savestr(value);
		    if (debug > 2)
			Debug("Changed content parameter: %s Value: %s\n",
				name, value);
		    return;
		}
	    }
	}
	if (params->numParamsUsed >= params->numParamsAlloced) {
	    params->numParamsAlloced += 10;
	    if (params->paramNames) {
		params->paramNames = 
		    (char **) realloc(params->paramNames,
				      (1+params->numParamsAlloced) * 
				      sizeof (char *));
		params->paramValues = 
		    (char **) realloc(params->paramValues, 
				      (1+params->numParamsAlloced) * 
				      sizeof (char *));
	    } else {
		params->paramNames = 
		    (char **) malloc((1+params->numParamsAlloced) * 
				     sizeof (char *));
		params->paramValues = 
		    (char **) malloc((1+params->numParamsAlloced) * 
				     sizeof (char *));
	    }
	    if (!params->paramNames || !params->paramValues) 
		error(SysErrWarning,
		      catgets( catalog, CAT_MSGS, 644, 
			      "AddContentParameter: cannot reallocate space.\n" ));
	}
	params->paramNames[params->numParamsUsed] = savestr(name);
	params->paramValues[params->numParamsUsed++] = savestr(value);
	if (debug > 2)
	    Debug("New content parameter: %s Value: %s\n", name, value);
    }
}

/*
 *  Side effect: alters the original string, chopping off the 
 * "; param=value..."
 */
void
ParseContentParameters(ct, params)
    char		*ct;
    mimeContentParams	*params;
{
    char	*s, *t, *eq;
    int		changedSFlag;
    int		changedEqFlag;
    
    params->numParamsUsed = 0;
    if (!ct) return;
    s = index(ct, ';');
    if (!s) return;
    *s++ = 0;
    StripTrailingSpace(ct);
    do {
	changedSFlag = 0;
	changedEqFlag = 0;
        t = paramend(s);
        if (t) *t++ = 0;
        eq = index(s, '=');
        if (!eq) {
	    Debug("Ignoring unparsable content-type parameter: '%s'\n", s);
        } else {
            *eq++ = 0;
            s = Cleanse(s);
	    if (*s == '"') {
		s = UnquoteString(s);
		changedSFlag = 1;
	    }
	    /* strip leading and trailing white space but don't 
	     * convert case
	     */
	    while (*eq && isspace((unsigned char) *eq)) ++eq;
	    StripTrailingSpace(eq);
	    if (*eq == '"') {
		eq = UnquoteString(eq);
		changedEqFlag = 1;
	    }
	    AddContentParameter(params, s, eq);
	    /* These frees are a bit dangerous, as they are dependent 
	     * upon the current combination of the implementation of 
	     * UnquoteString and the way we call it.  It should be rewritten 
	     * in the light of day.
	     */
	    if (changedSFlag)
		free(s);
	    if (changedEqFlag)
		free(eq);
        }
        s = t;
    } while (t);
}

void
FreeContentParameters(params)
    mimeContentParams	*params;
{
    int i;
    
    if (!params)
	return;
    for (i = 0; i < params->numParamsUsed; ++i) {
        xfree(params->paramNames[i]);
	xfree(params->paramValues[i]);
    }
    xfree(params->paramNames);
    xfree(params->paramValues);
    params->paramNames = NULL;
    params->paramValues = NULL;
    params->numParamsUsed = 0;
    params->numParamsAlloced = 0;
}

char *
FindParam(s, params)
    const char		*s;
    const mimeContentParams	*params;
{
    int i;

    if (params)
	for (i = 0; i < params->numParamsUsed; ++i) {
	    if (!ci_strcmp(s, params->paramNames[i]))
		return(params->paramValues[i]);
	}
    return(NULL);
}

void
DeleteContentParameter(params,s)
    mimeContentParams *params;
    const char *s;
{
    int i , j;

    if (params) {
	for (i = 0; i < params->numParamsUsed; ++i) {
	    if (!ci_strcmp(s, params->paramNames[i])) {
		  free(params->paramNames[i]);
		  free(params->paramValues[i]);
                  for (j=i;j<(params->numParamsUsed-1);j++)
                    {
                       params->paramNames[j] = params->paramNames[j+1];
                       params->paramValues[j] = params->paramValues[j+1];
                    }
                  params->numParamsUsed--;
                  return;
            }
	}
    }
}

int
PrintContentParameters(outBuf, params)
    char		*outBuf;
    mimeContentParams	*params;
{
    int	indx;
    
    if (!outBuf || !params)
	return -1;
    *outBuf = '\0';
    for (indx = 0; indx < params->numParamsUsed; ++indx) {
        sprintf(outBuf, " ; %s=",
                params->paramNames[indx]);
        outBuf += strlen(outBuf);
        if (strcpyStrip(outBuf, params->paramValues[indx], FALSE))
            error(UserErrWarning,
                  catgets(catalog, CAT_MSGS, 832, "Warning! Malformed value for message %s parameter: %s.\n"),
                  params->paramNames[indx], params->paramValues[indx]);
        outBuf += strlen(outBuf);
    }
    return 0;
}

int
CopyContentParameters(destParams, sourceParams)
    mimeContentParams	*destParams;
    mimeContentParams	*sourceParams;
{
    int	indx;
    
    if (!destParams || !sourceParams)
	return -1;
    for (indx=0; indx<sourceParams->numParamsUsed; ++indx) {
	AddContentParameter(destParams, sourceParams->paramNames[indx],
			     sourceParams->paramValues[indx]);
    }
    return 0;
}

/*
 *  Substitute the values of content parameters for strings of the form
 *  %{paramname} in the input string.  Copies into output buffer; does
 *  not replace in-place, but does modify the input string.
 */

void
InsertContentParameters(outBuf, formatString, attachPtr)
    char	  *outBuf;
    char	  *formatString;
    struct Attach *attachPtr;
{
    if (attachPtr) {
	char	*to, *p, *s;
	int		prefixed = 0;
	char 	*from, *tmp;
    
	for (from=(formatString), to=outBuf; *from; ++from) {
	    if (prefixed) {
		prefixed = 0;
		switch(*from) {
		case '%':
		    *to++ = '%';
		    break;
		case '{':
		    s = index(from, '}');
		    if (!s) {
			print(catgets( catalog, CAT_MSGS, 646, "Ignoring ill-formed parameter reference in attach.types file: %s\n" ), from);
			break;
		    }
		    ++from;
		    *s = 0;
		    /* put in lower case */
		    for (tmp=from; *tmp; ++tmp)
			Lower(*tmp);
		    p = FindParam(from, &attachPtr->content_params);
		    if (!p) {
			p = "\"\"";
			strcpy(to, p);
		    } else if (strcpyStrip(to, p, FALSE)) {
			error(UserErrWarning,
			      catgets(catalog, CAT_MSGS, 832, "Warning! Malformed value for message %s parameter: %s.\n"),
			      from, p);
		    }
		    to += strlen(p);
		    *s = '}'; /* restore */
		    from = s;
		    break;
		case 't':
		    /* type/subtype */
		    strcpyStrip(to, attach_data_type(attachPtr), FALSE);
		    to += strlen(attach_data_type(attachPtr));
		    break;
		default:
		    /* copy it, it could be a %s, where a filename string is 
		     * to be inserted later, or could be a z-script variable
		     */
		    *to++ = '%';
		    *to++ = *from;
		    break;
		    /* OLD behavior */
		    /*
		      print(catgets( catalog, CAT_MSGS, 647, "Ignoring unrecognized format code in attach.types file: %%%c\n" ), *from);
		      */
		    break;
		}
	    } else if (*from == '%') {
		prefixed = 1;
	    } else {
		*to++ = *from;
	    }
	}
	*to = 0;
    } else
	strcpy(outBuf, formatString);
}
