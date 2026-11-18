/* Ф0 bench: newlib system calls (from core-pico8). Console = MMIO UART, heap up to __heap_end.
   The only file: the WAD in memory (any name ending in .wad), so M_FileExists finds it;
   w_file_kg.c maps it without reading. */
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include "mmio.h"

extern char __heap_start[], __heap_end[];
static char *brk_ptr = __heap_start;

void *_sbrk(ptrdiff_t incr) {
    char *old = brk_ptr;
    if (brk_ptr + incr > __heap_end) {
        errno = ENOMEM;
        return (void *)-1;
    }
    brk_ptr += incr;
    return old;
}

#define WAD_FD 3
static uint32_t wad_pos;

int kg_is_wad(const char *name) {
    size_t n = strlen(name);
    return n > 4 && !strcasecmp(name + n - 4, ".wad") && MMIO_WADSIZE != 0;
}

int _write(int fd, const char *buf, int len) {
    if (fd != 1 && fd != 2) { errno = EBADF; return -1; }
    for (int i = 0; i < len; i++) MMIO_UART = (uint8_t)buf[i];
    return len;
}
int _read(int fd, char *buf, int len) {
    if (fd != WAD_FD) return 0;
    uint32_t sz = MMIO_WADSIZE;
    if (wad_pos >= sz) return 0;
    if ((uint32_t)len > sz - wad_pos) len = (int)(sz - wad_pos);
    memcpy(buf, (const char *)WAD_BASE + wad_pos, len);
    wad_pos += len;
    return len;
}
int _open(const char *name, int flags, int mode) {
    (void)mode;
    if ((flags & 3) == 0 && kg_is_wad(name)) { wad_pos = 0; return WAD_FD; }
    errno = ENOENT;
    return -1;
}
int _close(int fd) { return fd == WAD_FD ? 0 : -1; }
int _fstat(int fd, struct stat *st) {
    memset(st, 0, sizeof *st);
    if (fd == WAD_FD) { st->st_mode = S_IFREG; st->st_size = MMIO_WADSIZE; }
    else st->st_mode = S_IFCHR;
    return 0;
}
int _isatty(int fd) { return fd != WAD_FD; }
int _lseek(int fd, int off, int whence) {
    if (fd != WAD_FD) return 0;
    uint32_t base = whence == 0 ? 0 : whence == 1 ? wad_pos : MMIO_WADSIZE;
    wad_pos = base + off;
    return (int)wad_pos;
}
int _unlink(const char *name) { (void)name; errno = ENOENT; return -1; }
int _link(const char *a, const char *b) { (void)a; (void)b; errno = EMLINK; return -1; }
int _stat(const char *name, struct stat *st) {
    memset(st, 0, sizeof *st);
    if (kg_is_wad(name)) { st->st_mode = S_IFREG; st->st_size = MMIO_WADSIZE; return 0; }
    errno = ENOENT;
    return -1;
}
int _mkdir(const char *name, int mode) { (void)name; (void)mode; errno = ENOENT; return -1; }
int mkdir(const char *name, mode_t mode) { (void)name; (void)mode; errno = ENOENT; return -1; }
int _getpid(void) { return 1; }
int _kill(int pid, int sig) { (void)pid; (void)sig; errno = EINVAL; return -1; }
clock_t _times(struct tms *t) { (void)t; return (clock_t)MMIO_MS; }
int _gettimeofday(struct timeval *tv, void *tz) {
    (void)tz;
    uint32_t ms = MMIO_MS;
    tv->tv_sec = ms / 1000u;
    tv->tv_usec = (ms % 1000u) * 1000u;
    return 0;
}
void _exit(int code) {
    MMIO_EXIT = (uint32_t)code;
    for (;;) {}
}
