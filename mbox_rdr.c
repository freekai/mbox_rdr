#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <assert.h>

#include "mbox_rdr.h"

static void print_str(const char *buf, const unsigned int len) {
    /* this is debug only */
    char str[2048];
    memcpy(str, buf, len);
    *(str+len) = '\0';
    fprintf(stdout, "%s\n", str);
}

static int _get_next_line(const char *buf, unsigned int size, unsigned int *len) {
    unsigned int l = 0;
    while (l < size && *(buf+l) != '\n') {
        l++;
    }
    *len = l;
    if (l == size) {
        return -1;
    } else {
        return 0;
    }
}

static int _mboxr_parse(const char *name, mbox_info_t *info) {
    char buf[2048];
    unsigned int lc = 0;
    int fd, bytes;
    unsigned int off, len, avail;

    if ((fd = open(name, O_RDONLY)) == -1) {
        return 1;
    }

    off = 0;
    len = 0;
    /* read the chunk */
    while ((bytes = read(fd, buf+len, 2048-len)) != -1) {
        avail = len + bytes;
        while (1) {
            /* find the next line */
            if (_get_next_line((const char *)buf+off, avail-off, &len) == -1) {
                // copy the leftover to the beginning and retry
                char tmp[2048];
                assert(off+len == avail);
                memcpy(tmp, buf+off, len);
                memcpy(buf, tmp, len);
                off = 0;
                break;
            }
            lc++;
            off += len+1;
            len = 0;
        }
        if (!bytes) {
            break;
        }
    }
    return 0;
}

int mboxr_open(const char *name, mbox_info_t *info) {
    struct stat st; 

    if (stat(name, &st)) {
        return 1;
    }

    if (!S_ISREG(st.st_mode) || !(S_IRUSR & st.st_mode)) {
        return 1;
    }

    return _mboxr_parse(name, info);
}
