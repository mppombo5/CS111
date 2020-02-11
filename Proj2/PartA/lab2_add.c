/*
 * NAME: Matthew Pombo
 * EMAIL: mppombo5@gmail.com
 * UID: 405140036
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>

#ifndef bool
# define bool int
#endif
#ifndef true
# define true 1
#endif
#ifndef false
# define false 0
#endif

void add(long long* pointer, long long value);

void killProg(const char* msg);
const char* progName = "lab2_add";
const char* usage = "Usage: lab2_add [--threads=NUM] [--iterations=NUM]";

int main(int argc, char** argv) {
    int numThreads = 1;
    int numIters = 1;
    bool debug = false;

    struct option options[] = {
            {"threads", required_argument, NULL, 't'},
            {"iterations", required_argument, NULL, 'i'},
            {"debug", no_argument, NULL, 'd'},
            {0,0,0,0}
    };

    int optIndex;
    const char* optstring = "t:i:d";
    int ch = getopt_long(argc, argv, optstring, options, &optIndex);
    while (ch != -1) {
        switch (ch) {
            case 't':
                numThreads = atoi(optarg);
                break;
            case 'i':
                numIters = atoi(optarg);
                break;
            case 'd':
                debug = true;
                break;
            default:
                // no valid arguments were found
                exit(1);
        }

        ch = getopt_long(argc, argv, optstring, options, &optIndex);
    }

    return 0;
}

void add(long long* pointer, long long value) {
    long long sum = *pointer + value;
    *pointer = sum;
}

void killProg(const char* msg) {
    fprintf(stderr, "%s: %s: %s", progName, msg, strerror(errno));
    exit(1);
}
