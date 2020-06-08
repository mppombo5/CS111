/*
 * NAME: Matthew Pombo
 * EMAIL: mppombo5@gmail.com
 * UID: 405140036
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <mraa.h>
#include "utilities.h"

const char* usage = "lab4c_tcp --id={9-digit #} --host={hostname} --log={filename} {port #}\
                    [--period=#] [--scale={F,C}]";

void sampleTemp_tls(SSL* ssl, mraa_aio_context* sensor, scale_t tempScale, FILE* log, time_t* curTime);

volatile _Bool RUN_FLAG = 1;
_Bool debug = 0;

int main(int argc, char* argv[]) {
    FILE*   logfile = NULL;
    scale_t tempScale = LAB4_FAHRENHEIT;
    int     samplePeriod = 1;
    int     idNo = 0;
    char*   hostname = NULL;

    struct option options[] = {
            {"scale", required_argument, NULL, 's'},
            {"period", required_argument, NULL, 'p'},
            {"log", required_argument, NULL, 'l'},  // mandatory
            {"id", required_argument, NULL, 'i'},   // mandatory
            {"host", required_argument, NULL, 'h'}, // mandatory
            {"debug", no_argument, NULL, 'd'},
            {0,0,0,0}
    };

    int optIndex;
    const char* optstring = "s:p:l:i:h:d";
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

            case 'i':
                if (strlen(optarg) != 9) {
                    killProg("--id=# argument must be 9 digits long", 1);
                }
                for (char* c = optarg; *c != '\0'; c++) {
                    if (!isdigit(*c)) {
                        killProg("--id=# argument must be strictly numeric", 1);
                    }
                }
                idNo = atoi(optarg);
                break;

            case 'h':
                hostname = optarg;
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

    // stop if no logfile was supplied
    if (logfile == NULL) {
        killProg("Must supply log file as --log={filename}", 1);
    }

    // stop if no id was supplied
    if (idNo == 0) {
        killProg("Must supply id as --id=#", 1);
    }

    // stop if no hostname was supplied
    if (hostname == NULL) {
        killProg("Must supply hostname as --host={hostname}", 1);
    }

    // all arguments taken care of, now initialize temp sensor
    mraa_aio_context sensor = mraa_aio_init(LAB4_TEMPSENSOR_PIN);
    if (sensor == NULL) {
        killProg("Unable to initialize temperature sensor AIO pin", 1);
    }

    // initialize SSL
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    const SSL_METHOD* method;
    SSL_CTX* ctx;

    method = TLSv1_client_method();
    ctx = SSL_CTX_new(method);
    if (ctx == NULL) {
        ERR_print_errors_fp(stderr);
        exit(1);
    }

    // open TCP connection
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        killProg("Error on socket()", 1);
    }

    struct hostent* server = gethostbyname(hostname);
    if (server == NULL) {
        killProg("Error on gethostbyname()", 1);
    }

    struct sockaddr_in servAddr;
    bzero((char*) &servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    bcopy((char*) server->h_addr_list[0], (char*) &servAddr.sin_addr.s_addr, server->h_length);
    servAddr.sin_port = htons(19000);

    if (connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr)) == -1) {
        killProg("Error on connect()", 1);
    }

    // create SSL structure
    SSL* ssl = SSL_new(ctx);
    if (ssl == NULL) {
        ERR_print_errors_fp(stderr);
        exit(1);
    }

    // set which file descriptor output is going to
    if (SSL_set_fd(ssl, sockfd) == 0) {
        ERR_print_errors_fp(stderr);
        exit(1);
    }

    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(1);
    }

    // now we can read and write across the connection
    // buffer to log time and temperature to socket
    char logString[30];

    // immediately print ID
    if (sprintf(logString, "ID=%09d\n", idNo) < 0) {
        killProg("Unable to format ID message", 1);
    }
    if (SSL_write(ssl, logString, sizeof(char) * strlen(logString)) == -1) {
        killProg("Error writing ID message to socket", 1);
    }
    fprintf(logfile, "ID=%09d\n", idNo);

    // needed for time logging
    struct tm* curLocaltime;
    time_t curTime;
    time_t lastSampleTime;

    // buffer variables for reading commands
    char buffer[LAB4_TLS_BUFFERSIZE];
    int charsRead;
    int bufOffset = 0;

    // pollfd struct to perform polling on socket
    int pollRet;
    struct pollfd inputPoll;
    inputPoll.fd = sockfd;
    inputPoll.events = POLLIN;
    inputPoll.revents = 0;

    // bool to control output of reports (for START and STOP)
    _Bool keepGoing = 1;

    _Bool isFirstRun = 1;
    while (RUN_FLAG) {
        curTime = time(NULL);
        /*
         * temperature sensor. only run if it's the first sample,
         * or if 'samplePeriod' has passed since the last one
         */
        if ((keepGoing && isFirstRun) || (keepGoing && (curTime - lastSampleTime >= samplePeriod))) {
            if (isFirstRun) {
                isFirstRun = 0;
            }
            sampleTemp_tls(ssl, &sensor, tempScale, logfile, &curTime);
            lastSampleTime = time(NULL);
        }

        // handle input from socket
        pollRet = poll(&inputPoll, 1, 0);
        if (pollRet == -1) {
            killProg("Error on poll() call", 1);
        }

        // only parse input if the poll returned that it has input
        if (pollRet == 1) {
            if (inputPoll.revents & POLLERR) {
                killProg("poll() on socket returned POLLERR", 1);
            }

            if (inputPoll.revents & POLLIN) {
                charsRead = SSL_read(ssl, buffer + bufOffset, sizeof(char) * (LAB4_TLS_BUFFERSIZE - bufOffset));
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
                        if (fprintf(logfile, "%s\n", cmd) < 0) {
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
                            RUN_FLAG = 0;
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
                if (j >= LAB4_TLS_BUFFERSIZE && buffer[LAB4_TLS_BUFFERSIZE-1] != '\n') {
                    // can't do anything if the buffer can't handle at least one command
                    if (i == 0) {
                        killProg("Buffer size cannot handle sent command; exiting with code 1.\n(Consider sending smaller commands.)", 1);
                    }
                    // i points to the start of the last command
                    int k = 0;
                    while (i < LAB4_TLS_BUFFERSIZE) {
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

    // generate string for SHUTDOWN message; 0-pad the time again
    if (sprintf(logString, "%02d:%02d:%02d SHUTDOWN\n", curLocaltime->tm_hour, curLocaltime->tm_min, curLocaltime->tm_sec) < 0) {
        killProg("Unable to generate final SHUTDOWN log string", 1);
    }

    // print shutdown message to socket and logfile
    if (SSL_write(ssl, logString, sizeof(char) * strlen(logString)) == -1) {
        killProg("Error in last SHUTDOWN write() call", 1);
    }
    if (fprintf(logfile, "%s", logString) < 0) {
        killProg("Error in last SHUTDOWN fprintf to logfile", 1);
    }

    // shutdown and free SSL stuff
    SSL_shutdown(ssl);
    SSL_free(ssl);

    return 0;
}

void sampleTemp_tls(SSL* ssl, mraa_aio_context* sensor, scale_t tempScale, FILE* log, time_t* curTime) {
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
    if (SSL_write(ssl, logString, sizeof(char) * strlen(logString)) == -1) {
        killProg("Unable to print log string to socket", 1);
    }
    if (fprintf(log, "%s", logString) < 0) {
        killProg("Unable to print log string to log file", 1);
    }
}
