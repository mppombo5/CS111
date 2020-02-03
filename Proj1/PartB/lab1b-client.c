/* NAME: Matthew Pombo
 * EMAIL: mppombo5@gmail.com
 * ID: 405140036
 */

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define READ_BUF_SIZE 512
#define KEYBUF_SIZE 64
#define CTRL_D 0x04
#define CTRL_C 0x03

// just to make my life easier (long live C++)
#ifndef bool
# define bool int
#endif
#ifndef true
# define true 1
#endif
#ifndef false
# define false 0
#endif

const char* progName = "lab1b-client";
const char* usage = "lab1b-client --port=NUM [--log=FILENAME] [--compress] [--host=HOSTNAME]";
void killProg(const char* msg, bool sockInUse, int sockfd);

// separate (global) termios struct to restore in atexit()
struct termios initSettings;
void restoreAttr();

int main(int argc, char** argv) {
    // declared so I can use killProg
    bool sockUsed = false;

    bool suppliedPort = false;
    int portNo = -1;

    bool useLog = false;
    FILE* logFile = NULL;

    bool compress = false;
    bool debug = false;
    const char* hostName = "localhost";

    struct option options[] = {
            {"port", required_argument, NULL, 'p'},
            {"log", required_argument, NULL, 'l'},
            {"compress", no_argument, NULL, 'c'},
            {"host", required_argument, NULL, 'h'},
            {"debug", no_argument, NULL, 'd'},
            {0,0,0,0}
    };

    int optIndex;
    const char* optString = "p:l:ch:d";
    int ch = getopt_long(argc, argv, optString, options, &optIndex);
    while (ch != -1) {
        switch (ch) {
            case 'p':
                suppliedPort = true;
                portNo = atoi(optarg);
                if (debug) {
                    fprintf(stderr, "port set to %d", portNo);
                }
                break;
            case 'l':
                useLog = true;
                logFile = fopen(optarg, "w+");
                if (logFile == NULL) {
                    fprintf(stderr, "%s: Unable to open file '%s': %s", progName, optarg, strerror(errno));
                    exit(1);
                }
                break;
            case 'c':
                compress = true;
                break;
            case 'h':
                hostName = optarg;
                break;
            case 'd':
                debug = true;
                break;
            default:
                fprintf(stderr, "%s: Unrecognized argument\n%s", progName, usage);
                exit(1);
        }
        ch = getopt_long(argc, argv, optString, options, &optIndex);
    }
    if (!suppliedPort) {
        fprintf(stderr, "%s: Expected --port=[number]; argument not found.", progName);
        exit(1);
    }

    // get the current termios settings
    if (tcgetattr(STDIN_FILENO, &initSettings) == -1) {
        killProg("Error getting initial termios attributes", sockUsed, 0);
    }
    if (atexit(&restoreAttr) != 0) {
        killProg("Unable to register atexit() function", sockUsed, 0);
    }

    struct termios newSettings = initSettings;
    newSettings.c_iflag = ISTRIP;
    newSettings.c_oflag = 0;
    newSettings.c_lflag = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &newSettings) == -1) {
        killProg("Unable to set new non-canonical termios settings", sockUsed, 0);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        killProg("Error opening socket", sockUsed, 0);
    }

    struct hostent* server = gethostbyname(hostName);
    if (server == NULL) {
        killProg("Error getting server by hostname", sockUsed, sockfd);
    }

    struct sockaddr_in servAddr;
    bzero((char*) &servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    bcopy((char*) server->h_addr, (char*) &servAddr.sin_addr.s_addr, server->h_length);
    servAddr.sin_port = htons(portNo);

    if (connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr)) == -1) {
        killProg("Syscall connect() failed", sockUsed, sockfd);
    }
    sockUsed = true;

    // now that we've copy-pasted everything from the tutorial, we can get to passing I/O
    struct pollfd fds[2] = {
            {STDIN_FILENO, POLLIN, 0},
            {sockfd, POLLIN, 0}
    };
    struct pollfd* keyPoll = fds;
    struct pollfd* sockPoll = fds + 1;

    while (true) {
        int ret = poll(fds, 2, 0);
        if (ret == -1) {
            killProg("Error in poll() syscall", sockUsed, sockfd);
        }

        // deal with keyboard input
        if (keyPoll->revents & POLLERR) {
            killProg("Received POLLERR from poll() call on keyboard", sockUsed, sockfd);
        }
        if (keyPoll->revents & POLLIN) {
            // got input from keyboard, deal with it
            char keyBuf[KEYBUF_SIZE];
            int charsRead = read(STDIN_FILENO, keyBuf, KEYBUF_SIZE);
            if (charsRead == -1) {
                killProg("Error in read() call on keyboard", sockUsed, sockfd);
            }

            if (useLog) {
                fprintf(logFile, "SENT %d bytes: ", charsRead);
            }
            // process all chars read from keyBuf
            for (int i = 0; i < charsRead; i++) {
                char curChar = keyBuf[i];
                // lf/cr -> lf
                if (curChar == '\r' || curChar == '\n') {
                    if (write(STDOUT_FILENO, "\r\n", 2) == -1) {
                        killProg("Error writing \\r\\n to stdout from keyboard", sockUsed, sockfd);
                    }

                    if (write(sockfd, "\n", 1) == -1) {
                        close(sockfd);
                        killProg("Error writing \\n to server from keyboard", sockUsed, sockfd);
                    }
                    else {
                        if (useLog) {
                            fprintf(logFile, "%c", '\n');
                        }
                    }
                }
                else if (curChar == CTRL_D) {
                    if (write(STDOUT_FILENO, "^D", 2) == -1) {
                        close(sockfd);
                        killProg("Error writing ^D to stdout from keyboard", sockUsed, sockfd);
                    }

                    if (write(sockfd, keyBuf + i, 1) == -1) {
                        close(sockfd);
                        killProg("Error writing ^D to server from keyboard", sockUsed, sockfd);
                    }
                    else {
                        if (useLog) {
                            fprintf(logFile, "%c", curChar);
                        }
                    }
                }
                else if (curChar == CTRL_C) {
                    if (write(STDOUT_FILENO, "^C", 2) == -1) {
                        close(sockfd);
                        killProg("Error writing ^C to stdout from keyboard", sockUsed, sockfd);
                    }

                    if (write(sockfd, keyBuf + i, 1) == -1) {
                        close(sockfd);
                        killProg("Error writing ^C to server from keyboard", sockUsed, sockfd);
                    }
                    else {
                        if (useLog) {
                            fprintf(logFile, "%c", curChar);
                        }
                    }
                }
                else {
                    if (write(STDOUT_FILENO, keyBuf + i, 1) == -1) {
                        close(sockfd);
                        killProg("Error writing character to stdout", sockUsed, sockfd);
                    }

                    if (write(sockfd, keyBuf + i, 1) == -1) {
                        close(sockfd);
                        killProg("Error writing character to server", sockUsed, sockfd);
                    }
                    else {
                        if (useLog) {
                            fprintf(logFile, "%c", curChar);
                        }
                    }
                }
            }
            // append a newline after each log line
            if (useLog) {
                fprintf(logFile, "\n");
            }
        }

        // deal with output from server
        if (sockPoll->revents & POLLERR) {
            close(sockfd);
            killProg("Received POLLERR from poll on server", sockUsed, sockfd);
        }
        if (sockPoll->revents & POLLIN) {
            char srvBuf[READ_BUF_SIZE];
            int charsRead = read(sockfd, srvBuf, READ_BUF_SIZE);
            if (charsRead == -1) {
                close(sockfd);
                killProg("Error read()-ing from server", sockUsed, sockfd);
            }

            // server shut down
            if (charsRead == 0) {
                if (debug) {
                    fprintf(stderr, "server shut down; terminating.");
                }
                close(sockfd);
                exit(0);
            }

            if (useLog) {
                fprintf(logFile, "RECEIVED %d bytes: ", charsRead);
            }
            for (int i = 0; i < charsRead; i++) {
                char curChar = srvBuf[i];
                // lf -> cr-lf
                if (curChar == '\n') {
                    if (write(STDOUT_FILENO, "\r\n", 2) == -1) {
                        killProg("Error writing \\r\\n to stdout from server", sockUsed, sockfd);
                    }
                    else {
                        if (useLog) {
                            fprintf(logFile, "%c", curChar);
                        }
                    }
                }
                else {
                    if (write(STDOUT_FILENO, srvBuf + i, 1) == -1) {
                        killProg("Error writing server output to stdout", sockUsed, sockfd);
                    }
                    else {
                        if (useLog) {
                            fprintf(logFile, "%c", curChar);
                        }
                    }
                }
            }
            if (useLog) {
                fprintf(logFile, "\n");
            }
        }
        if (sockPoll->revents & POLLHUP) {
            if (debug) {
                fprintf(stderr, "POLLHUP from server");
            }
            close(sockfd);
            exit(0);
        }
    }
    
    return 0;
}

void killProg(const char* msg, bool sockInUse, int sockfd) {
    if (sockInUse) {
        close(sockfd);
    }
    fprintf(stderr, "%s: %s: %s\n", progName, msg, strerror(errno));
    exit(1);
}

void restoreAttr() {
    if (tcsetattr(STDIN_FILENO, TCSANOW, &initSettings) == -1) {
        killProg("Unable to restore initial termios settings", false, 0);
    }
}

