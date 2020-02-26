/*
 * NAME: Matthew Pombo
 * EMAIL: mppombo5@gmail.com
 * UID: 405140036
 */

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <poll.h>
#include "mraa-include/mraa.h"
#include "utilities.h"

#define LAB4_FAHRENHEIT 'F'
#define LAB4_CELSIUS 'C'
#define LAB4_TEMPSENSOR_PIN 1   // AIO pin number for temperature sensor
#define LAB4_BUTTON_PIN 60      // GPIO pin number for button
#define LAB4_BUFFERSIZE 256     // buffer size with which to read commands from stdin

const char* usage = "lab4b [--scale={F,C}] [--period=#] [--log={FILENAME}]";

volatile short runFlag = 1;     // flag to determine whether or not to keep sampling temperature

void onButtonPress();

int main(int argc, char** argv) {
    char tempScale = LAB4_FAHRENHEIT;
    int samplePeriod = 1;       // global variable for sampling period
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
                samplePeriod = atoi(optarg);
                if (samplePeriod <= 0) {
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
                exit(1);
        }
        ch = getopt_long(argc, argv, optstring, options, &optIndex);
    }

    if (debug) {
        fprintf(stderr, "running in DEBUG mode...\n");
    }

    // register handler for button press
    mraa_gpio_context button = mraa_gpio_init(LAB4_BUTTON_PIN);
    if (button == NULL) {
        killProg("Unable to initialize button GPIO pin", 1);
    }
    if (mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &onButtonPress, NULL) != MRAA_SUCCESS) {
        killProg("Unable to register button signal handler", 1);
    }

    // initialize temperature sensor
    mraa_aio_context sensor = mraa_aio_init(LAB4_TEMPSENSOR_PIN);
    if (sensor == NULL) {
        killProg("Unable to initialize temperature sensor AIO pin", 1);
    }

    // just make local variables for time report
    struct tm* curLocaltime;
    time_t curTime;
    time_t lastSampleTime;

    // int to control output of reports (for START and STOP)
    int keepGoing = 1;

    // buffer to log things to the logfile, if supplied
    char logString[30];

    // buffer variables for reading commands
    char buffer[LAB4_BUFFERSIZE];
    int charsRead;
    int bufOffset = 0;

    // pollfd struct to perform polling on stdin
    int pollRet;
    struct pollfd stdinPoll;
    stdinPoll.fd = STDIN_FILENO;
    stdinPoll.events = POLLIN;
    stdinPoll.revents = 0;

    int firstRun = 1;
    // handle input from stdin and generate output from sensor
    while (runFlag) {
        curTime = time(NULL);
        /*
         * temperature sensor. only run if it's the first sample,
         * or if 'samplePeriod' has passed since the last one
         */
        if ((keepGoing && firstRun) || (keepGoing && (curTime - lastSampleTime >= samplePeriod))) {
            if (firstRun) {
                firstRun = 0;
            }
            int rawTemp = mraa_aio_read(sensor);
            if (debug) {
                fprintf(stderr, "rawTemp is %d\n", rawTemp);
            }
            curTime = time(NULL);

            double temp = RawtoC(rawTemp);
            if (debug) {
                fprintf(stderr, "temp after raw to C is %.1f\n", temp);
            }
            if (tempScale == LAB4_FAHRENHEIT) {
                temp = CtoF(temp);
            }

            curLocaltime = localtime(&curTime);

            // generate two characters each for hour, min, and sec
            char hour[5], min[5], sec[5];
            int curHr = curLocaltime->tm_hour;
            int curMin = curLocaltime->tm_min;
            int curSec = curLocaltime->tm_sec;
            if (sprintf(hour, (curHr < 10) ? "0%d" : "%d", curHr) < 0) {
                killProg("Unable to generate hour string for log string", 1);
            }
            if (sprintf(min, (curMin < 10) ? "0%d" : "%d", curMin) < 0) {
                killProg("Unable to generate minute string for log string", 1);
            }
            if (sprintf(sec, (curSec < 10) ? "0%d" : "%d", curSec) < 0) {
                killProg("Unable to generate second string for log string", 1);
            }

            // printf logString to stdout and the logfile
            if (sprintf(logString, "%s:%s:%s %.1f\n", hour, min, sec, temp) < 0) {
                killProg("Unable to generate log string using sprintf()", 1);
            }
            if (printf("%s", logString) < 0) {
                killProg("Unable to print log string to stdout", 1);
            }
            if (logfile != NULL && fprintf(logfile, "%s", logString) < 0) {
                killProg("Unable to print log string to log file", 1);
            }
            lastSampleTime = time(NULL);
        }

        // handle input from stdin
        pollRet = poll(&stdinPoll, 1, 0);
        if (pollRet == -1) {
            killProg("Error on poll() call", 1);
        }

        // only parse input if the poll returned that it has input
        if (pollRet == 1) {
            if (debug) {
                fprintf(stderr, "poll() returned 1\n");
            }
            if (stdinPoll.revents & POLLERR) {
                killProg("poll() on stdin returned POLLERR", 1);
            }

            if (stdinPoll.revents & POLLIN) {
                charsRead = read(STDIN_FILENO, buffer + bufOffset, sizeof(char) * (LAB4_BUFFERSIZE - bufOffset));
                if (debug) {
                    fprintf(stderr, "read %d characters\n", charsRead);
                }
                int i = 0, j = 0;
                char* arg;  // for getting arguments to SCALE=, PERIOD=, etc.
                while (j < charsRead) {
                    while (j < charsRead && buffer[j] != '\n') {
                        j++;
                    }

                    if (j >= charsRead) {
                        break;
                    }

                    /*
                     * if buffer[j] is \n, then we have a new command to parse.
                     * 'i' will point to the start of the new command, so replace
                     * '\n' with '\0' so that library string functions work more easily.
                     */
                    else if (buffer[j] == '\n') {
                        buffer[j] = '\0';
                        char* cmd = buffer + i;
                        // log if needed
                        if (logfile != NULL && fprintf(logfile, "%s\n", cmd) < 0) {
                            killProg("Unable to print command to log", 1);
                        }

                        if (strcmp(cmd, "START") == 0) {
                            keepGoing = 1;
                            if (debug) {
                                fprintf(stderr, "START detected\n");
                            }
                        }
                        else if (strcmp(cmd, "STOP") == 0) {
                            keepGoing = 0;
                            if (debug) {
                                fprintf(stderr, "STOP detected\n");
                            }
                        }
                        else if (strcmp(cmd, "OFF") == 0) {
                            runFlag = 0;
                            if (debug) {
                                fprintf(stderr, "OFF detected\n");
                            }
                            break;
                        }
                        else if (strncmp(cmd, "SCALE=", 6) == 0) {
                            // advance i to the beginning of the argument
                            i += 6;
                            arg = buffer + i;
                            if (debug) {
                                fprintf(stderr, "arg to SCALE is %s\n", arg);
                            }
                            if (j - i == 1) {
                                switch (buffer[i]) {
                                    case LAB4_CELSIUS:
                                        tempScale = LAB4_CELSIUS;
                                        break;
                                    case LAB4_FAHRENHEIT:
                                        tempScale = LAB4_FAHRENHEIT;
                                        break;
                                    default: break;
                                }
                            }
                        }
                        else if (strncmp(cmd, "PERIOD=", 7) == 0) {
                            // advance i to the start of the arg
                            i += 7;
                            arg = buffer + i;
                            if (debug) {
                                fprintf(stderr, "arg to PERIOD is %s\n", arg);
                            }
                            int newPeriod = atoi(arg);
                            if (newPeriod > 0) {
                                samplePeriod = newPeriod;
                            }
                        }
                        // no need to check for LOG, it doesn't need anything we don't already do

                        j++;
                        i = j;
                        continue;
                    }
                    j++;
                }
                // handle the case when the buffer was filled up and the last command wasn't '\n' terminated
                if (j >= LAB4_BUFFERSIZE && buffer[LAB4_BUFFERSIZE-1] != '\n') {
                    // can't do anything if the buffer can't handle at least one command
                    if (i == 0) {
                        killProg("Buffer size cannot handle sent command; exiting with code 1.\n(Consider sending smaller commands.)", 1);
                    }
                    // i points to the start of the last command
                    int k = 0;
                    while (i < LAB4_BUFFERSIZE) {
                        buffer[k] = buffer[i];
                        i++;
                        k++;
                    }
                    bufOffset = k;
                }
            }
        }
    }

    if (mraa_aio_close(sensor) != MRAA_SUCCESS) {
        killProg("Error closing analog IO pin on shutdown", 1);
    }

    curTime = time(NULL);
    curLocaltime = localtime(&curTime);

    // get two characters for hour, minute, and second
    char hour[5], min[5], sec[5];
    int curHr = curLocaltime->tm_hour, curMin = curLocaltime->tm_min, curSec = curLocaltime->tm_sec;
    if (sprintf(hour, (curHr < 10) ? "0%d" : "%d", curHr) < 0) {
        killProg("Unable to generate hour string for log string", 1);
    }
    if (sprintf(min, (curMin < 10) ? "0%d" : "%d", curMin) < 0) {
        killProg("Unable to generate minute string for log string", 1);
    }
    if (sprintf(sec, (curSec < 10) ? "0%d" : "%d", curSec) < 0) {
        killProg("Unable to generate second string for log string", 1);
    }

    // print SHUTDOWN string to stdout and log file
    if (sprintf(logString, "%s:%s:%s SHUTDOWN\n", hour, min, sec) < 0) {
        killProg("Unable to generate log string using sprintf()", 1);
    }
    if (printf("%s", logString) < 0) {
        killProg("Error in last SHUTDOWN printf call", 1);
    }
    if (logfile != NULL && fprintf(logfile, "%s", logString) < 0) {
        killProg("Error in last SHUTDOWN fprintf to logfile", 1);
    }

    return 0;
}

void onButtonPress() {
    runFlag = 0;
}
