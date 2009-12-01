#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>

/*
 * an implementation of ftruncate for systems that don't have ftruncate or
 * chsize, but do have the F_FREESP fcntl flag -- spencer
 */

int
ftruncate(fd, offset)
  int fd;
  off_t offset;
{
  struct flock fl;

  fl.l_whence = SEEK_SET;
  fl.l_start = offset;
  fl.l_len = 0; /* 0 means "until EOF" */

  return fcntl(fd, F_FREESP, &fl);
}
