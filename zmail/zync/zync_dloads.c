/*
 * Copyright (c) 1994 Z-Code Software Corp., an NCD company.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "popper.h"
#include "version.h"
#include <hashtab.h>

static const char zync_dloads_rcsid[] =
    "$Id: zync_dloads.c,v 1.16 1995/10/26 20:21:09 bobg Exp $";

#define DOWNLOAD_NODE_HASH_SIZE 53 /* Size of library node hash table */
#define DOWNLOAD_GLIST_GROWSIZE 10 /* Allocation unit various glists in the
				    * library data structure */

#ifndef S_ISDIR
# define S_ISDIR(mode)   ((mode&S_IFMT) == S_IFDIR)
#endif /* S_ISDIR */

void
stuffDownloadFile(downFile, attrPlist)
    downloadFile *downFile;
    struct plist *attrPlist;
{
    char *str;

    downFile->attributes = attrPlist;
    downFile->name = plist_Get(attrPlist, FILE_ID_PROP);
    if (((str = plist_Get(attrPlist, SEQNO_PROP)) == NULL)
	|| (sscanf(str, "%u", &downFile->seqno) == EOF))
	downFile->seqno = NO_SEQNO;
    downFile->version_str = plist_Get(attrPlist, VERSION_PROP);
}

void
addDownloadFile(p, file_list, fullname, status, filename)
    POP *p;
    struct glist *file_list;
    char *fullname;
    struct stat *status;
    char *filename;
{
    FILE *file;
    char line[MAXLINELEN];

    if (!(file = fopen(fullname, "r"))) {
	if (p->debug & DEBUG_WARNINGS)
	    pop_log(p, POP_PRIORITY,
		    "Warning: can't open download file \"%s\": %s.", 
		    fullname, strerror(errno));
    } else if (fgets(line, MAXLINELEN, file) == NULL) {
	if (p->debug & DEBUG_WARNINGS)
	    pop_log(p, POP_PRIORITY,
		    "Warning: can't read from download file \"%s\": %s.",
		    fullname, strerror(errno));
	fclose(file);
    } else {
	downloadFile newDownloadFile;
	struct plist *attrPlist = plist_New(ATTR_PLIST_GROWSIZE);

	fclose(file);
	if (parseAttrLine(line, attrPlist, p->errGlist)) {
	    if (p->debug & DEBUG_WARNINGS) {
		char **errstrptr;
		int i;

		pop_log(p, POP_PRIORITY, 
			"Warning: download file \"%s\" has errors parsing attribute line:",
			fullname);
		glist_FOREACH(p->errGlist, char *, errstrptr, i)
		    pop_log(p, POP_PRIORITY, "  %s", *errstrptr);
	    }
	    freeErrGlist(p->errGlist);
	}
	stuffDownloadFile(&newDownloadFile, attrPlist);
	if (newDownloadFile.version_str != NULL)
	    if (parseVersion(newDownloadFile.version_str, 
			     &newDownloadFile.version) == -1) {
		if (p->debug & DEBUG_WARNINGS)
		    pop_log(p, POP_PRIORITY,
			    "Warning: download file \"%s\" has bad version string.", 
			    fullname);
		newDownloadFile.version_str = NULL;
	    }
	if (newDownloadFile.name == NULL)
	    newDownloadFile.name = strdup(filename);
	newDownloadFile.fullname = strdup(fullname);
	newDownloadFile.bytes = status->st_size;
	glist_Add(file_list, &newDownloadFile);
    }
}

int
cmpDownloadNode(alpha, beta)
    CVPTR alpha;
    CVPTR beta;
{
    return strcmp(((downloadNode *) alpha)->name, 
		  ((downloadNode *) beta)->name);
}

int
cmpDownloadFile(alpha, beta)
    CVPTR alpha;
    CVPTR beta;
{
    return strcmp(((downloadFile *) alpha)->name,
		  ((downloadFile *) beta)->name);
}


int
strptrcmp(strptr1, strptr2)
    const char **strptr1, **strptr2;
{
    return strcmp(*strptr1, *strptr2);
}

struct glist *
buildGivelist(p, dirname)
    POP *p;
    char *dirname;
{
    char givename[MAXPATHLEN];
    struct glist *givelist;
    struct stat status;
    FILE *file;
    char line[MAXLINELEN];
  
    sprintf(givename, "%s/.give", dirname);
    if (access(givename, F_OK)) {
	if (p->debug & DEBUG_LIBRARY)
	    pop_log(p, POP_DEBUG,
		    "No .give file in %s; giving nothing (bah, humbug.)",
		    dirname);
	return (0);
    }
    if (!(file = fopen(givename, "r"))) {
	if (p->debug & DEBUG_WARNINGS) 
	    pop_log(p, POP_PRIORITY,
		    "Warning: couldn't open give file \"%s\": %s.",
		    givename, strerror(errno));
	return (0);
    }
    givelist = (struct glist *) emalloc(sizeof (struct glist),
					"buildGivelist");
    glist_Init(givelist, sizeof (char *), DOWNLOAD_GLIST_GROWSIZE);
    while (fgets(line, MAXLINELEN, file) != NULL) {
	if (*line != '#') {
	    char *lineCopy;

	    line[strlen(line)-1] = 0;
	    lineCopy = strdup(line);
	    glist_Add(givelist, &lineCopy);
	}
    }
    if (!feof(file)) {
	int tmp_errno = errno;

	fclose(file);
	if (p->debug & DEBUG_WARNINGS)
	    pop_log(p, POP_PRIORITY,
		    "Warning: error reading give file \"%s\": %s.",
		    givename, strerror(tmp_errno));
	return NULL;
    }
    fclose(file);
    if (!glist_EmptyP(givelist))
	glist_Sort(givelist, strptrcmp);
    return givelist;
}

downloadNode *
buildDownloadNode(p, path_buffer, tail_name, nodeHashtab)
    POP *p;
    char *path_buffer;
    char *tail_name;
    struct hashtab *nodeHashtab;
{
    int alloc_size;
    downloadNode *node;
    struct glist *child_list;
    struct glist *file_list;
    char *node_name;
    struct glist *givelist;
    char *bufptr;
    char **give_ptr;
    int idx;
  
    /* Allocate a new node, plus space for the lists and node name. */
    alloc_size = sizeof(downloadNode) + 2 * sizeof(struct glist) +
	strlen(tail_name) + 2;
    node = (downloadNode *) emalloc(alloc_size, "buildDownloadNode");
    if (node == NULL)
	return NULL;

    /* Set up pointers in the node structure. */
    child_list = (struct glist *) (node + 1);
    file_list = child_list + 1;
    node_name = (char *)(file_list + 1);
    node->name = node_name;
    node->file_list = file_list;
    node->child_list = child_list;

    /* Initialize the child and file lists, and copy the node name. */
    glist_Init(child_list, sizeof(downloadNode), DOWNLOAD_GLIST_GROWSIZE);
    glist_Init(file_list, sizeof(downloadFile), DOWNLOAD_GLIST_GROWSIZE);
    strcpy(node_name, tail_name);

    /* Look for and parse a .give file, bail on error... */
    givelist = buildGivelist(p, path_buffer);
    if (givelist == NULL)
	goto unwind;

    /* Add a slash to the name buffer. */
    bufptr = path_buffer + strlen(path_buffer);
    *bufptr = '/';
    bufptr++;

    /* Loop through the give list, building the node's file and child lists. */
    glist_FOREACH(givelist, char *, give_ptr, idx) {
	char *child_name;
	struct stat status;
	nodeHashKey nodeProbe;

	/* Make the child file name. */
	child_name = *give_ptr;
	strcpy(bufptr, child_name);

	/* Stat the child file, skip if it doesn't exist. */
	if (stat(path_buffer, &status) < 0) {
	    if (p->debug & DEBUG_WARNINGS)
		pop_log(p, POP_PRIORITY,
			"Warning: can't stat download file \"%s\": %s.",
			path_buffer, strerror(errno));
	    continue;
	}
      
	/* If it's not a directory, add it to the file list. */
	if (!S_ISDIR(status.st_mode)) {
	    addDownloadFile(p, file_list, path_buffer, &status, child_name);
	    continue;
	}

	/* If a sub-directory, maybe recurse... */
	nodeProbe.st_dev = status.st_dev;
	nodeProbe.st_ino = status.st_ino;
	if (hashtab_Find(nodeHashtab, &nodeProbe) != NULL) {
	    /* Been there, done that... */
	    if (p->debug & DEBUG_WARNINGS)
		pop_log(p, POP_PRIORITY, 
			"Warning: download library recursion detected at directory \"s\".",
			path_buffer);
	} else {
	    downloadNode *child_node;

	    hashtab_Add(nodeHashtab, &nodeProbe);
	    /* Recurse. */
	    child_node = buildDownloadNode(p, path_buffer, bufptr,
					   nodeHashtab);
	    hashtab_Remove(nodeHashtab, &nodeProbe);
	    if (child_node == NULL)
		goto unwind;
	    glist_Add(child_list, child_node);
	}
    }

    /* Sort the child and file lists. */
    if (!glist_EmptyP(child_list))
	glist_Sort(child_list, cmpDownloadNode);
    if (!glist_EmptyP(file_list))
	glist_Sort(file_list, cmpDownloadFile);
    return node;

  unwind:
    glist_Destroy(child_list);
    glist_Destroy(file_list);
    free((char *)node);
    return NULL;
}

void
doDownloadInheritance(p, node)
    POP *p;
    downloadNode *node;
{
    downloadNode *child;
    int i;
    int childIndex, nodeIndex;
    struct glist *newFileList;
    int comp;

    glist_FOREACH(node->child_list, downloadNode, child, i) {
	nodeIndex = 0;
	childIndex = 0;
	newFileList = (struct glist *)malloc(sizeof(struct glist));
	glist_Init(newFileList, sizeof (downloadFile),
		   DOWNLOAD_GLIST_GROWSIZE);

	while ((nodeIndex < glist_Length(node->file_list))
	       && (childIndex < glist_Length(child->file_list))) {
	    comp = strcmp(((downloadFile *) glist_Nth(child->file_list,
						      childIndex))->name,
			  ((downloadFile *) glist_Nth(node->file_list,
						      nodeIndex))->name);
	    if (comp < 0)
		glist_Add(newFileList, glist_Nth(child->file_list,
						 childIndex++));
	    else if (comp > 0)
		glist_Add(newFileList, glist_Nth(node->file_list,
						 nodeIndex++));
	    else {
		glist_Add(newFileList, glist_Nth(child->file_list,
						 childIndex));
		++childIndex;
		++nodeIndex;
	    }
	}
	while (childIndex < glist_Length(child->file_list))
	    glist_Add(newFileList, glist_Nth(child->file_list, childIndex++));
	while (nodeIndex < glist_Length(node->file_list))
	    glist_Add(newFileList, glist_Nth(node->file_list, nodeIndex++));
	child->file_list = newFileList;
	doDownloadInheritance(p, child);
    }
}


unsigned int
nodeHashfn(hashkey)
    CVPTR hashkey;
{
    return ((unsigned int) (((nodeHashKey *)hashkey)->st_dev 
			    + ((nodeHashKey *)hashkey)->st_ino));
}

int
nodeHashCmp(alpha, beta)
    CVPTR alpha;
    CVPTR beta;
{
    return (((((nodeHashKey *) alpha)->st_ino ==
	      ((nodeHashKey *)beta)->st_ino)
	     && (((nodeHashKey *)alpha)->st_dev ==
		 ((nodeHashKey *)beta)->st_dev)) ? 0 : -1);
}
	 
void
dumpDownloads(p, node, indent)
    POP *p;
    downloadNode *node;
    int indent;
{
    downloadFile *dfile;
    downloadNode *dnode;
    int i,j;
    char line[MAXLINELEN];
    char *lintptr;

    for (i=0; i<indent; i++)
	line[i] = ' ';
    sprintf(line+i, "Node: %s", node->name);
    pop_log(p, POP_DEBUG, line);
    glist_FOREACH(node->file_list, downloadFile, dfile, j) {
	for (i=0; i<indent; i++)
	    line[i] = ' ';
	sprintf(line+i, "  File: %s %s %s %d", dfile->name, dfile->fullname,
		((dfile->version_str == NULL) ?
		 "-no-version-" :
		 unparseVersion(&dfile->version)),
		dfile->seqno);
	pop_log(p, POP_DEBUG, line);
    }
    glist_FOREACH(node->child_list, downloadNode, dnode, i)
	dumpDownloads(p, dnode, indent+2);
}


int
init_downloads(p)
    POP *p;
{
    struct stat status;
    struct hashtab nodeHashtab;
    nodeHashKey rootHashKey;
    char path_buffer[MAXPATHLEN];
    downloadNode *tree;

    strcpy(path_buffer, p->lib_path);

    /* Initialize recursion detection hash table */
    hashtab_Init(&nodeHashtab, nodeHashfn, nodeHashCmp, sizeof(nodeHashKey),
		 DOWNLOAD_NODE_HASH_SIZE);
    stat(p->lib_path, &status);
    rootHashKey.st_dev = status.st_dev;
    rootHashKey.st_ino = status.st_ino;
    hashtab_Add(&nodeHashtab, &rootHashKey);

    /* Build the download tree. */
    if (!(tree = buildDownloadNode(p, path_buffer, ".", &nodeHashtab))) {
	pop_log(p, POP_PRIORITY, "Cannot build download tree");
	return -1;
    }

    hashtab_Destroy(&nodeHashtab);
    doDownloadInheritance(p, tree);

    if (p->debug & DEBUG_LIBRARY) {
	pop_log(p, POP_DEBUG, "Download library:");
	dumpDownloads(p, tree, 2);
    }

    p->download_tree = tree;
    return 0;  
}

int
setup_downloads(p)
    POP *p;
{
    char **libkeyptr;
    int i,j;
    downloadFile *serverFile;

    if (p->client_props == NULL) {
	pop_msg(p, POP_FAILURE, "%s", 
		"I don't know enough about vous yet, send ZMOI.");
	return POP_FAILURE;
    }

    /* Find relavant node of download tree */
    if (p->download_tree == NULL)
	init_downloads(p);
    p->downloads=p->download_tree;
    glist_FOREACH(p->lib_structure, char *, libkeyptr, i) {
	downloadNode probe;

	if (((probe.name = plist_Get(p->client_props, *libkeyptr)) != NULL)
	    && ((j = glist_Bsearch(p->downloads->child_list, &probe, 
				   cmpDownloadNode)) != -1))
	    p->downloads = (downloadNode *) glist_Nth(p->downloads->child_list,
						      j);
	else
	    break;
    }

    /* Mark all relevant files as outdated initially */
    p->outdatedCount = 0;
    p->outdatedBytes = 0;
    glist_FOREACH(p->downloads->file_list, downloadFile, serverFile, i) {
	serverFile->outdated = -1;
	p->outdatedCount++;
	p->outdatedBytes += serverFile->bytes;
    }

    return POP_SUCCESS;
}      
