/*
 * NAME: Matthew Pombo
 * EMAIL: mppombo5@gmail.com
 * UID: 405140036
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "utilities.h"

const char* progName = "lab4b";

void killProg(const char* msg, int exitStat) {
    if (errno == 0) {
        fprintf(stderr, "%s: %s\n", progName, msg);
        exit(exitStat);
    }
    else {
        fprintf(stderr, "%s: %s: %s\n", progName, msg, strerror(errno));
        exit(exitStat);
    }
}

double FtoC(double tempF) {
    return (tempF - 32) / 1.8;
}

double CtoF(double tempC) {
    return (tempC * 1.8) + 32;
}

double RawtoC(int raw) {
    int B = 4275;       // magic number
    int R0 = 100000;    // magic constant

    double R = 1023.0 / (raw - 1.0);
    R *= R0;

    // somehow get temp in C from this
    return 1.0 / (log(R/R0)/B + 1/298.15) - 273.15;
}
