#include <basetsd.h>
#ifndef FIFO_CACHE_H
#define FIFO_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllimport) int lab2_open(const char *path);
__declspec(dllimport) int lab2_close(int fd);
__declspec(dllimport) SSIZE_T lab2_read(int fd, void *buf, size_t count);
__declspec(dllimport) SSIZE_T lab2_write(int fd, const void *buf, size_t count);
__declspec(dllimport) long lab2_lseek(int fd, long offset, int whence);
__declspec(dllimport) int lab2_fsync(int fd);

#ifdef __cplusplus
}
#endif

#endif // FIFO_CACHE_H
