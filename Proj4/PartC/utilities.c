/*
 * NAME: Matthew Pombo
 * EMAIL: mppombo5@gmail.com
 * UID: 405140036
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <mraa.h>
#include "utilities.h"

const char* progName = "lab4c";

// kills the program, outputs 'msg' to stderr, and exits with code 'exitStat'
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

// Celsius to Fahrenheit
double CtoF(double tempC) {
    return (tempC * 1.8) + 32;
}

// convert raw resistance(? voltage?) from temperature sensor to Celsius
double RawtoC(int raw) {
    int B = 4275;       // magic number
    int R0 = 100000;    // magic constant

    double R = 1023.0/raw - 1.0;
    R *= R0;

    // somehow get temp in C from this
    return 1.0 / (log(R/R0)/B + 1/298.15) - 273.15;
}

void sampleTemp(int sockfd, mraa_aio_context* sensor, scale_t tempScale, FILE* log, time_t* curTime) {
    int rawTemp = mraa_aio_read(*sensor);
    if (debug) {
        fprintf(stderr, "rawTemp is %d\n", rawTemp);
    }

    double temp = RawtoC(rawTemp);
    if (debug) {
        fprintf(stderr, "temp after raw to C is %.1f\n", temp);
    }
    if (tempScale == LAB4_FAHRENHEIT) {
        temp = CtoF(temp);
    }

    struct tm* curLocaltime = localtime(curTime);

    char logString[30];

    // generate string for logging, and pad the hour/min/sec with 0s
    if (sprintf(logString, "%02d:%02d:%02d %.1f\n", curLocaltime->tm_hour, curLocaltime->tm_min, curLocaltime->tm_sec, temp) < 0) {
        killProg("Unable to generate log string using sprintf()", 1);
    }

    // print the log string to socket and logfile
    if (write(sockfd, logString, sizeof(char) * strlen(logString)) == -1) {
        killProg("Unable to print log string to socket", 1);
    }
    if (fprintf(log, "%s", logString) < 0) {
        killProg("Unable to print log string to log file", 1);
    }
}
