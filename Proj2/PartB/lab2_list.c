/* NAME: Matthew Pombo
 * EMAIL: mppombo5@gmail.com
 * UID: 405140036
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <gperftools/profiler.h>

const char* const progName = "lab2_list";

void killProg(const char* msg, int exitStat);

int main(int argc, char** argv) {
    printf("Hello, World!\n");
    return 0;
}

void killProg(const char* msg, int exitStat) {
    if (errno == 0) {
        fprintf(stderr, "%s: %s\n", progName, msg);
    }
    else {
        fprintf(stderr, "%s: %s: %s\n", progName, msg, strerror(errno));
    }
    exit(exitStat);
}
