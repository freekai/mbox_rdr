#ifndef MBOX_RDR_H
#   define MBOX_RDR_H       1

typedef struct mbox_info {
    unsigned long msg_new;
    unsigned long msg_read;
    unsigned long msg_seen;
    unsigned long msg_total;
} mbox_info_t;

int mboxr_open(const char *, mbox_info_t *);

#endif /* MBOX_RDR_H */

