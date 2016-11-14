#ifndef MBOX_RDR_H
#   define MBOX_RDR_H       1

typedef struct mbox_info {
    unsigned char has_new;
} mbox_info_t;

int mboxr_open(const char *, mbox_info_t *);

#endif /* MBOX_RDR_H */

