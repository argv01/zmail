#include "osconfig.h"

#ifdef HAVE_MMAP

#include "fmap.h"
#include <sys/mman.h>

/* Doug Kingston says:
 *
 * mmap() will alloc unused memory from your 4GB address space that is
 * currently unused and place the file there.  You can also at your option
 * place it at a location of your choice if you wish.  You might do the
 * later if your file contains address pointer and cannot be relocated.  We
 * do this with our tickplant incore database which is shared with another
 * process(s) much like shared memory, but unlike shared memory, a shared
 * mapped file is pagable.  Typically the Sun mmap and others I have seen
 * put the mapped files a little below the stack at the top of the address
 * space so you really dont have to worry about the heap and the mmap-ed
 * file colliding.
 *
 * You do not have to malloc space, and the kernel will page align it
 * when it assigns an address.  If you assign an address, it must be
 * page aligned.  See the mmap(2) man page for specific details on the
 * limitations on addresses.
 *
 * You no longer need the file descriptor unless we want to do periodic
 * stats or some other fd type operation (perhaps ftruncate()?) so
 * we close it once we no longer need it.  This means that we can mmap()
 * LOTS of files, many more than our FD limit.  munmap does NOT require
 * the file descriptor.
 *
 * You cannot map a file larger than its size.  This make mapping files that
 * you expect to grow a bit weird.  You always have the option of munmap,
 * followed by an mmap at the new size.  This is not very expensive.
 *
 * You can also map things as writable, and what you scribble on that memory
 * will make its way to disk eventually, and certainly at the time you
 * unmap the region.
 */

/* Bart says:
 *
 * What we're doing here is a fast hack to read a few files more quickly.
 * The interface defined here isn't ideal, and it imposes a limit on the
 * number of files we can map.  Callers should be prepared to deal with
 * the files directly if they can't map them (for example, zmsource.c).
 */

struct {
    char *base;
    size_t size;
} fmapdata[FMAP_MAX];

static int fmapcount;
int fmappingok = 1;

/* Map a file into memory; return the base of the mapped region, or zero
 * if the mapping failed.  The caller must preserve the base for calling
 * funmap() when finished with the file.
 */
char *
fmap(fp)
FILE *fp;
{
    struct stat sb;
    char *base;
    int fd = fileno(fp);

    if (fmapcount >= FMAP_MAX)
	return 0;

    if (fstat(fd, &sb) < 0)
	return 0;

    if (fmappingok) {
	base = (char *) mmap(0, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (base == (char *)(-1))
	    return 0;
    } else {
	return 0;
    }

    fmapdata[fmapcount].base = base;
    fmapdata[fmapcount++].size = sb.st_size;

    return base;
}

/* Locate a previously mapped region and unmap it. */
void
funmap(base)
char *base;
{
    int i;

    for (i = 0; i < fmapcount; i++) {
	if (fmapdata[i].base == base) {
	    (void)munmap(base, fmapdata[i].size);
	    fmapdata[i] = fmapdata[--fmapcount];
	}
    }
}

/* Return the size of a previously mapped region. */
size_t
fmap_size(base)
char *base;
{
    int i;

    for (i = 0; i < fmapcount; i++) {
	if (fmapdata[i].base == base)
	    return fmapdata[i].size;
    }
    return -1;
}

#else

int fmappingok;

#endif /* HAVE_MMAP */
