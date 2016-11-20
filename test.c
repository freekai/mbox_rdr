#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#include "mbox_rdr.h"

#define LOG(...) \
    fprintf(stderr, __VA_ARGS__)

int main(int argc, char **argv) {
    const char *name = "mbox_example";
    mbox_info_t info;
    memset(&info, 0, sizeof(mbox_info_t));

    if (mboxr_open("does_not_exist", &info)) {
        LOG("PASS: fails to open non-existant file.\n");
    } else {
        LOG("FAIL: returned 0 (OK) when opening non-existant file.\n");
    }

    if (mboxr_open(name, &info)) {
        LOG("FAIL: fails to open a valid file.\n");
    } else {
        LOG("PASS: opens existing mbox.\n");
    }

    if (info.msg_new) {
        LOG("PASS: correctly determines that mbox has new messages.\n");
    } else {
        LOG("FAIL: does not detect new messages in the mbox.\n");
    }

    return 0;
}
