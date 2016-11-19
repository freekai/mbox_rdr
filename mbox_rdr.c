#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <assert.h>

#include "config.h"
#include "mbox_rdr.h"

typedef enum state {
    MSG_START_MAYBE,
    MSG_START_MUST,
    MSG_HDR,
    MSG_BODY
} state_t;

static void print_str(const char *buf, const unsigned int len) {
    /* this is debug only */
    char str[BUF_SIZE];
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

static unsigned char _is_from_line(const char *line, unsigned int len) {
    return !strncmp(line, "From ", 5) ? 1 : 0;
}

static int _parse_line(const char *line, unsigned int len) {
    static state_t st = MSG_START_MUST;
    switch (st) {
        case MSG_START_MUST:
        case MSG_START_MAYBE:
            if (!len) {
                break;
            } else if (len < 6) { /* F, R, O, M, space, and at least one more character */
                if (st == MSG_START_MAYBE) st = MSG_BODY;
                return st == MSG_START_MUST ? 1 : 0;
            } else {
                if (_is_from_line(line, len)) {
#                           ifdef DEBUG
                    fprintf(stdout, "Found message.\n");
#                           endif
                    st = MSG_HDR;
                } else {
                    if (st == MSG_START_MAYBE) st = MSG_BODY;
                    return st == MSG_START_MUST ? 1 : 0;
                }
            }
            break;
        case MSG_HDR:
            if (!len) {
                st = MSG_BODY;
            }
            break;
        case MSG_BODY:
            if (!len) {
                st = MSG_START_MAYBE;
            }
            break;
    }
    return 0;
}

static int _mboxr_parse(const char *name, mbox_info_t *info) {
    char buf[BUF_SIZE];
#   ifdef DEBUG
    unsigned int lc = 0;
#   endif
    int fd, bytes;
    unsigned int off, len, avail;

    if ((fd = open(name, O_RDONLY)) == -1) {
        return 1;
    }

    off = 0;
    len = 0;
    /* read the chunk */
    while ((bytes = read(fd, buf+len, BUF_SIZE-len)) != -1) {
        avail = len + bytes;
        while (1) {
            /* find the next line */
            if (_get_next_line((const char *)buf+off, avail-off, &len) == -1) {
                /* copy the leftover to the beginning and retry */
                char tmp[BUF_SIZE];
                assert(off+len == avail);
                memcpy(tmp, buf+off, len);
                memcpy(buf, tmp, len);
                off = 0;
                break;
            }

            _parse_line((const char *)buf+off, len);

            off += len+1;
            len = 0;
#           ifdef DEBUG
            lc++;
#           endif
        }
        if (!bytes) {
            break;
        }
    }
#   ifdef DEBUG
    fprintf(stdout, "Total number of lines: %d.\n", lc);
#   endif
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
