/*
 * Copyright 1994 by Z-Code Software Corp., an NCD company.
 */

#include "zyncd.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <glist.h>

#define DEFAULT_TRACE_FILE "/usr/tmp/zyncd-trace"
#define DEFAULT_DEBUG_ARGS "WARNINGS"
#define DEFAULT_CONFIG "/usr/local/etc/Zync/zync.config"
#define DEFAULT_SCRIPT_PATH "/usr/local/etc/Zync/scripts"
#define DEFAULT_PREF_PATH "/usr/local/etc/Zync/prefs"
#define DEFAULT_LIB_PATH "/usr/local/etc/Zync/lib"
#define DEFAULT_LIBRARY_STRUCTURE "PRODUCT PLATFORM VERSION"
#define DEFAULT_TIMEOUT 60

int zync_debug = 0;
FILE *zync_trace_file;
pid_t zync_pid;
char *zync_script_path = NULL;
char *zync_pref_path = NULL;
char *zync_lib_path = NULL;


/* Debug keys and associated values. */
struct {char* key; int val;} debug_flags[] = {
  {"ERRORS", DEBUG_ERRORS},
  {"WARNINGS", DEBUG_WARNINGS|DEBUG_ERRORS},
  {"CONFIG", DEBUG_CONFIG|DEBUG_WARNINGS|DEBUG_ERRORS},
  {"LIBRARY", DEBUG_LIBRARY|DEBUG_WARNINGS|DEBUG_ERRORS},
  {"EXEC", DEBUG_EXEC|DEBUG_WARNINGS|DEBUG_ERRORS},
  {"CCLIENT", DEBUG_CCLIENT|DEBUG_WARNINGS|DEBUG_ERRORS},
  {"ALL", -1},
  {NULL, 0}
};


/* Initialize log file */
void trace(tracefile)
     char *tracefile;
{
  static int done_that = 0;
  
  if ((zync_trace_file = fopen(tracefile, "a+")) == NULL) {
    

/* Zync specific initializations */
void zync_init(argc_ptr, argv_ptr)
     int *argc_ptr;
     char ***argv_ptr;
{
  int argc = *argc_ptr;
  char **argv = *argv_ptr;
  char c;
  char *debug_args = NULL;
  char *config_name = DEFAULT_CONFIG_NAME;
  char *libstruct_arg = NULL;
  char *timeout_arg = NULL;
  FILE *config_file;

  openlog("zyncd", LOG_PID, ZYNC_SYSLOG_FACILITY);
  zync_pid = getpid();

  /* Parse arg list */
  while ((c = getopt(argc, argv, "t:d:c:s:p:l:o:m:z:")) != EOF) {
    switch c {
    case 't': trace(optarg); break;
    case 'd': debug_args = optarg; break;
    case 'c': config_file = optarg; break;
    case 's': strcpy(zync_script_path, otparg); break;
    case 'p': strcpy(zync_pref_path, optarg); break;
    case 'l': strcpy(zync_lib_path, optarg); break;
    case 'o': libstruct_arg = optarg; break;
    case 'z': timeout_arg = optarg; break;
    default:
      mm_dlog("Warning: unrecognized command-line option: -%c", c);
      errflag++;
    }
  }

  /* Fake leftovers as entire arg list */
  argv[optind-1] = argv[0];
  *argv_ptr = argv + optind - 1;
  *argc_ptr = argc - optind + 1;

  /* Parse config file */
  if ((config_file = fopen(config_name, "r")) == NULL) {
    mm_dlog("Warning: couldn't open config file %s: %s.",
	    config_name, strerror(errno));
    errflag++;
  } else {
    char line[MAXLINELEN];
    char *word;
    while(!feof(config_file)) {
      if (fgets(line, sizeof(line), config_file) == NULL) {
	mm_dlog("Warning: error reading config file %s: %s.",
		config_name, strerror(errno));
	errflag++;
	break;
      }
      if (line[0] == '#'|| ((word = strtok(line, " \t")) == NULL))
	continue;
      if strcasecmp(word, "trace")
	trace(strtok(NULL, " \t\n"));
      else if ((debug_args == NULL) 
	       && (strcasecmp(word, "debug") == 0))
	debug_args = strcpy(strtok(NULL, "\n"));
      else if ((zync_script_path == NULL) 
	       && (strcasecmp(word, "scripts") == 0))
	zync_script_path = strtok(NULL, " \t\n");
      else if ((zync_pref_path == NULL)
	       && (strcasecmp(word, "prefs") == 0))
	zync_prefs_path = strtok(NULL, " \t\n");
      else if ((zync_lib_path == NULL)
	       && (strcasecmp(word, "library") == 0))
	zync_lib_path = strtok(NULL, " \t\n");
      else if ((libstruct_arg == NULL)
	       && (strcasecmp(word, "library-structure") == 0))
	libstruct_arg = strtok(NULL, "\n");
      else if ((timeout_arg == NULL)
	       && (strcasecmp(word, "timeout") == 0))
	timeout_arg = strtok(NULL, " \t\n");
      else {
	mm_dlog("Warning: unrecognized configuration file option: %s", word);
	errflag++;
      }
    }
    fclose(config_file);
  }
  
  /* Default unspecified config options. */
  trace(DEFAULT_TRACE_FILE);
  if (debug_args == NULL)
    debug_args = DEFAULT_DEBUG_ARGS;
  if (zync_script_path == NULL)
    zync_script_path = DEFAULT_SCRIPT_PATH;
  if (zync_pref_path == NULL)
    zync_pref_path == DEFAULT_PREF_PATH;
  if (zync_lib_path == NULL)
    zync_lib_path = DEFAULT_LIB_PATH;
  if (libstruct_arg == NULL)
    libstruct_arg = DEFAULT_LIBRARY_STRUCTURE;

  /* Setup debug flags */
  for (word=strtok(debug_args, " \n\t");
       word!=NULL;
       word=strtok(NULL, " \n\t")) {
    int i;
    for (i=0; debug_flags[i].key; i++) {
      if (strcasecmp(word, debug_flags[i].key) == 0) {
	zync_debug |= debug_flags[i].val;
	break;
      }
    }
  }

  /* Setup library structure */
  zync_lib_structure = (struct glist *)malloc(sizeof(struct glist));
  glist_Init(zync_lib_structure, sizeof(char **), 
	     LIB_STRUCTURE_GLIST_GROWSIZE);
  for (word=strtok(libstruct_arg, " \n\t");
       word!=NULL;
       word=strtok(NULL, " \n\t")) {
    word = strdup(word);
    glist_Add(zync_lib_structure, &word);
  }

  

}  
       
