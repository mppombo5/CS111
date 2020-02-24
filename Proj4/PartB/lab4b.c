/*
 * NAME: Matthew Pombo
 * EMAIL: mppombo5@gmail.com
 * UID: 405140036
 */

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include "mraa-include/mraa.h"
#include "utilities.h"

#define LAB4_FAHRENHEIT 'F'
#define LAB4_CELSIUS 'C'

int main(int argc, char** argv) {
    char tempScale = LAB4_FAHRENHEIT;
    int sampleFreq = 1;
    FILE* logfile = NULL;
    int debug = 0;

    struct option options[] = {
            {"scale", required_argument, NULL, 's'},
            {"period", required_argument, NULL, 'p'},
            {"log", required_argument, NULL, 'l'},
            {"debug", no_argument, NULL, 'd'},
            {0,0,0,0}
    };

    int optIndex;
    char* optstring = "s:p:l:d";
    int ch = getopt_long(argc, argv, optstring, options, &optIndex);
    while (ch != -1) {
        switch (ch) {
            case 's':
                if (strlen(optarg) != 1 || (*optarg != LAB4_FAHRENHEIT && *optarg != LAB4_CELSIUS)) {
                    killProg("'--scale' argument must be either 'F' or 'C'", 1);
                }
                tempScale = *optarg;
                break;
            case 'p':
                sampleFreq = atoi(optarg);
                if (sampleFreq <= 0) {
                    killProg("'--period' argument must be strictly greater than 0", 1);
                }
                break;
            case 'l':
                logfile = fopen(optarg, "w+");
                if (logfile == NULL) {
                    killProg("Unable to open specified logfile", 1);
                }
                break;
            case 'd':
                debug = 1;
                break;
            default:
                fprintf(stderr, "%s\n", usage);
        }
        ch = getopt_long(argc, argv, optstring, options, &optIndex);
    }

    return 0;
}
