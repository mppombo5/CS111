/*
 * NAME: Matthew Pombo
 * EMAIL: mppombo5@gmail.com
 * UID: 405140036
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "utilities.h"

void killProg(const char* msg, int exitStat) {
    if (errno == 0) {
        fprintf(stderr, "%s: %s\n", progName, msg);
        exit(1);
    }
    else {
        fprintf(stderr, "%s: %s: %s\n", progName, msg, strerror(errno));
        exit(1);
    }
}
