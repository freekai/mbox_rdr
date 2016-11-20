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
    MSG_STATUS,
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

static int _parse_line(const char *line, unsigned int len, mbox_info_t *info) {
    static state_t st = MSG_START_MUST;
    switch (st) {
        case MSG_START_MUST:
        case MSG_START_MAYBE:
            if (!len) {
                break;
            } else if (len < 6) { /* not from line: F, R, O, M, space, and at
                                     least one more character */
                if (st == MSG_START_MAYBE) st = MSG_BODY;
                return st == MSG_START_MUST ? 1 : 0;
            } else {
                if (!strncmp(line, "From ", 5)) { /* message start */
                    st = MSG_HDR;
                    info->msg_total++;
                } else {
                    if (st == MSG_START_MAYBE) st = MSG_BODY;
                    return st == MSG_START_MUST ? 1 : 0;
                }
            }
            break;
        case MSG_HDR:
        case MSG_STATUS:
            if (!len) {
                if (st == MSG_HDR) {
                    /* no status found, this is a new message. */
                    info->msg_new++;
                }
                st = MSG_BODY;
            }
            if (len > 8) { /* possibly status: S, T, A, T, U, S, :, space, and
                              at least one more character. */
                if (!strncmp(line, "Status: ", 8)) {
                    if (st == MSG_STATUS) {
                        fprintf(stderr, "Second status line while parsing header!");
                        break;
                    }
                    st = MSG_STATUS;
                    char msg_st[8]; /* max 7 status markers, but we only recon 2. */
                    int msg_st_len = len-8;
                    char msg_st_info = 0; /* this is how we're going to count the message as. */
                    int i;
                    assert(msg_st_len < 8);
                    memcpy(msg_st, line+8, msg_st_len);
                    msg_st[msg_st_len] = '\0';
                    for (i=0; i < msg_st_len; i++) {
                        switch (msg_st[i]) {
                            case 'R':
                                msg_st_info = 'R';
                                break;
                            case 'O':
                                if (!msg_st_info) {
                                    msg_st_info = 'O';
                                }
                                break;
                            default:
                                fprintf(stderr, "Unknown message status:"
                                        " %c.\n", msg_st[i]);
                                break;
                        }
                        if (msg_st_info == 'R') break;
                    }
                    switch (msg_st_info) {
                        case 'R':
                            info->msg_read++;
                            break;
                        case 'O':
                            info->msg_seen++;
                            break;
                        default:
                            fprintf(stderr, "The status is neither Seen or Read: %s.\n", msg_st);
                            break;
                    }
                }
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

            _parse_line((const char *)buf+off, len, info);

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
    assert(info->msg_total == info->msg_new+info->msg_seen+info->msg_read);
#   ifdef DEBUG
    fprintf(stdout, "Mailbox: %lu new, %lu seen, and %lu read.\n", info->msg_new, info->msg_seen, info->msg_read);
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

    memset(info, 0, sizeof(mbox_info_t));

    return _mboxr_parse(name, info);
}
