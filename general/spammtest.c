#include <dlist.h>
#include <sklist.h>
#include <spamm.h>

#include <sys/types.h>
#include <sys/times.h>
#include <sys/param.h>

static struct Obj {
    int id;
    struct dlist pointers;
} *Root = 0;

static time_t gctime = 0;

static int livenodes = 0;

struct spamm_ObjectPool Pool;

char *ProgramName;

int
nthindex(dl, n)
    struct dlist *dl;
    int n;
{
    int result = dlist_Head(dl);

    while (--n >= 0)
	result = dlist_Next(dl, result);

    return (result);
}

struct Obj *
random_node()
{
    int i, j;
    struct Obj *node = Root;

    while (1) {
	if (dlist_EmptyP(&(node->pointers)))
	    return (node);
	i = random() % (dlist_Length(&(node->pointers)) + 1);
	if (i == dlist_Length(&(node->pointers)))
	    return (node);
	node = *((struct Obj **) dlist_Nth(&(node->pointers),
					   nthindex(&(node->pointers), i)));
    }
}

struct Obj *
newnode()
{
    static int counter = 0;
    struct Obj *result = spamm_Allocate(&Pool);

    ++livenodes;
    result->id = counter++;
    dlist_Init(&(result->pointers), (sizeof (struct Obj *)), 8);
    return (result);
}

add_existing(node)
    struct Obj *node;
{
    struct Obj *new = random_node();

    printf("Add to %d: existing=%d\n", node->id, new->id);
    dlist_Append(&(node->pointers), &new);
}

add_new(node)
    struct Obj *node;
{
    struct Obj *new = newnode();

    printf("Add to %d: new=%d\n", node->id, new->id);
    dlist_Append(&(node->pointers), &new);
}

replace_existing(node)
    struct Obj *node;
{
    struct Obj *new = random_node();
    int indx = nthindex(&(node->pointers),
			random() % dlist_Length(&(node->pointers)));

    printf("Replace in %d: old=%d, existing=%d\n", node->id,
	   (*((struct Obj **) dlist_Nth(&(node->pointers), indx)))->id,
	   new->id);
    dlist_Replace(&(node->pointers), indx, &new);
}

replace_new(node)
    struct Obj *node;
{
    struct Obj *new = newnode();
    int indx = nthindex(&(node->pointers),
			random() % dlist_Length(&(node->pointers)));

    printf("Replace in %d: old=%d, new=%d\n", node->id,
	   (*((struct Obj **) dlist_Nth(&(node->pointers), indx)))->id,
	   new->id);
    dlist_Replace(&(node->pointers), indx, &new);
}

delete(node)
    struct Obj *node;
{
    int indx = nthindex(&(node->pointers),
			random() % dlist_Length(&(node->pointers)));

    printf("Delete from %d: old=%d\n", node->id,
	   (*((struct Obj **) dlist_Nth(&(node->pointers), indx)))->id);
    dlist_Remove(&(node->pointers), indx);
}

int maxdepth;

void
Trace(node)
    struct Obj *node;
{
    static int depth = 0;
    int i;
    struct Obj **p;

    ++depth;
    dlist_FOREACH(&(node->pointers), struct Obj *, p, i) {
	spamm_Trace(*p);
    }
    if (depth > maxdepth)
	maxdepth = depth;
    --depth;
}

void
Reclaim(node)
    struct Obj *node;
{
    --livenodes;
    printf("Destroying %d\n", node->id);
    dlist_Destroy(&(node->pointers));
}

struct tms gcstart_tm, gcend_tm;

void
gcstart()
{
    maxdepth = 0;
    puts("GC start");
    (void) times(&gcstart_tm);
}

void
gcend()
{
    int empty, neither, full, total;
    time_t elapsed;

    (void) times(&gcend_tm);
    elapsed = ((gcend_tm.tms_utime + gcend_tm.tms_stime) -
	       (gcstart_tm.tms_utime + gcstart_tm.tms_stime));
    gctime += elapsed;
    printf("GC end, elapsed time %ld, total live nodes = %d, maxdepth = %d\n",
	   elapsed, livenodes, maxdepth);
    total = spamm_PoolStats(&Pool, &empty, &neither, &full);
    printf("Pages: %d empty, %d neither, %d full, %d total\n",
	   empty, neither, full, total);
}

main(argc, argv)
    int argc;
    char **argv;
{
    struct tms start_tm, end_tm;
    time_t progtime;
    extern int optind;
    extern char *optarg;
    int limit = 1000;
    int A = 20;
    int a = 20;
    int r = 20;
    int R = 20;
    int d = 20;
    struct Obj *node = 0, **nodep;
    int action, c, i, j;
    extern char *rindex();
    char *tmp = rindex(argv[0], '/');
    ProgramName = (tmp ? tmp + 1 : argv[0]);

    (void) times(&start_tm);
    Srandom(time(0));

    spamm_Initialize();
    spamm_GcStart = gcstart;
    spamm_GcEnd = gcend;
    spamm_InitPool(&Pool, (sizeof (struct Obj)), ULBITS, Trace, Reclaim);
    spamm_Root((GENERIC_POINTER_TYPE **) &Root);
    spamm_Root((GENERIC_POINTER_TYPE **) &node);
    Root = newnode();

    while ((c = getopt(argc, argv, "l:A:a:R:r:d:")) != EOF) {
	switch (c) {
	  case 'l':
	    limit = atoi(optarg);
	    break;
	  case 'A':
	    A = atoi(optarg);
	    break;
	  case 'a':
	    a = atoi(optarg);
	    break;
	  case 'R':
	    R = atoi(optarg);
	    break;
	  case 'r':
	    r = atoi(optarg);
	    break;
	  case 'd':
	    d = atoi(optarg);
	    break;
	  default:
	    fprintf(stderr, "Usage: %s [-l limit] [-{a|A|r|R|d} prob] ...\n",
		    ProgramName);
	    exit(c != '?');
	}
    }

    while (limit-- > 0) {
	node = random_node();
	if (dlist_EmptyP(&(node->pointers))) {
	    action = (random() % (A + a));
	    if (action < a) {
		add_existing(node);
	    } else {
		add_new(node);
	    }
	} else {
	    action = (random() % (A + a + R + r + d));
	    if (action < a) {
		add_existing(node);
	    } else if (action < (a + A)) {
		add_new(node);
	    } else if (action < (a + A + r)) {
		replace_existing(node);
	    } else if (action < (a + A + r + R)) {
		replace_new(node);
	    } else {
		delete(node);
	    }
	}
    }
    dlist_FOREACH2(&(Root->pointers), struct Obj *, nodep, i, j) {
	dlist_Remove(&(Root->pointers), i);
    }
    (void) times(&end_tm);
    progtime = ((end_tm.tms_utime + end_tm.tms_stime) -
		(start_tm.tms_utime + start_tm.tms_stime));
    printf("Program time %ld, gc time %ld (%d%%)\n",
	   progtime, gctime, (100 * gctime) / progtime);
    spamm_CollectGarbage();
    exit(0);
}
