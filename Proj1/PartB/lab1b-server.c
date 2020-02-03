/* NAME: Matthew Pombo
 * EMAIL: mppombo5@gmail.com
 * ID: 405140036
 */

#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>

#define KEYBUF_SIZE 64
#define SHELLBUF_SIZE 512
#define CTRL_D 0x04
#define CTRL_C 0x03

// just to make my life easier
#ifndef bool
# define bool int
#endif
#ifndef true
# define true 1
#endif
#ifndef false
# define false 0
#endif

const char* progName = "lab1b-server";
const char* usage = "lab1b-server --port=NUM [--compress]";
void killProg(const char* msg);

int main(int argc, char** argv) {
    bool suppliedPort = false;
    int portNo = -1;

    bool compress = false;

    bool debug = false;

    struct option options[] = {
            {"port", required_argument, NULL, 'p'},
            {"compress", no_argument, NULL, 'c'},
            {"debug", no_argument, NULL, 'd'},
            {0,0,0,0}
    };

    int optIndex;
    const char* optString = "p:cd";
    int ch = getopt_long(argc, argv, optString, options, &optIndex);
    while (ch != -1) {
        switch (ch) {
            case 'p':
                suppliedPort = true;
                portNo = atoi(optarg);
                break;
            case 'c':
                compress = true;
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

    int oldsockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (oldsockfd == -1) {
        killProg("Unable to open socket");
    }

    struct sockaddr_in servAddr;
    bzero((char*) &servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(portNo);
    servAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(oldsockfd, (struct sockaddr*) &servAddr, sizeof(servAddr)) == -1) {
        killProg("Unable to bind socket");
    }
    listen(oldsockfd, 5);

    struct sockaddr_in cliAddr;
    unsigned int clilen = sizeof(cliAddr);
    int sockfd = accept(oldsockfd, (struct sockaddr*) &cliAddr, &clilen);
    if (sockfd == -1) {
        killProg("Error on accept() call");
    }

    // now that we have a connection, fork and execute the shell
    int pipe1[2];
    int pipe2[2];
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        killProg("Error creating pipe");
    }

    int pid = fork();
    if (pid < 0) {
        killProg("Error on fork()");
    }
    else if (pid == 0) {
        // child process
        // read through pipe2[0], write through pipe1[1]
        if (close(pipe1[0]) == -1 || close(pipe2[1]) == -1) {
            killProg("Error closing pipes in child process");
        }
        // easier names, pipe-read and pipe-write
        int childRead = pipe2[0];
        int childWrite = pipe1[1];

        // make the read end of the pipe be the shell's standard input
        if (dup2(childRead, STDIN_FILENO) == -1 || close(childRead) == -1) {
            killProg("Error redirecting pipe to stdin in child process");
        }
        // make stderr/stdout of the shell go to the write end
        if (dup2(childWrite, STDOUT_FILENO) == -1) {
            killProg("Error redirecting stdout to pipe in child process");
        }
        if (dup2(childWrite, STDERR_FILENO) == -1 || close(childWrite) == -1) {
            killProg("Error redirecting stderr to pipe in child process");
        }
        if (execl("/bin/bash", "/bin/bash", (char *) NULL) == -1) {
            killProg("Error executing shell");
        }
    }

    // if this is reached, we're in the parent process
    // read through pipe1[0], write through pipe2[1]
    if (close(pipe2[0]) == -1 || close(pipe1[1]) == -1) {
        killProg("Error closing pipes in parent process");
    }
    int pRead = pipe1[0];
    int pWrite = pipe2[1];

    struct pollfd fds[2] = {
            {sockfd, POLLIN, 0},
            {pRead, POLLIN, 0}
    };
    struct pollfd* sockPoll = fds;
    struct pollfd* shellPoll = fds + 1;

    while (true) {
        int ret = poll(fds, 2, 0);
        if (ret == -1) {
            killProg("Error in poll() syscall");
        }

        // deal with socket input
        if (sockPoll->revents & POLLERR) {
            killProg("Received POLLERR from poll() call on keyboard");
        }
        if (sockPoll->revents & POLLIN) {
            // got input from socket, send it to shell
            char keyBuf[KEYBUF_SIZE];
            int charsRead = read(sockfd, keyBuf, KEYBUF_SIZE);
            if (charsRead == -1) {
                killProg("Error in read() call on client");
            }

            // process all chars from client
            for (int i = 0; i < charsRead; i++) {
                char curChar = keyBuf[i];
                // no need to handle <cr><lf>, already translated in client
                if (curChar == CTRL_C) {
                    int killRet = kill(pid, SIGINT);
                    if (killRet == -1) {
                        killProg("Error sending SIGINT to shell");
                    }
                    if (debug) {
                        fprintf(stderr, "SIGINT sent to shell");
                    }
                    break;
                }
                else if (curChar == CTRL_D) {
                    if (close(pWrite) == -1) {
                        killProg("Unable to close pipe to shell");
                    }
                }
                else {
                    if (write(pWrite, keyBuf + i, 1) == -1) {
                        killProg("Error writing char to shell");
                    }
                }
            }
        }
        if (sockPoll->revents & POLLHUP) {
            if (close(pWrite) == -1) {
                killProg("Error in close()ing shell pipe");
            }

            int status;
            int waitRet = waitpid(pid, &status, 0);
            if (waitRet == -1) {
                killProg("Error in waitpid() for shell");
            }
            int sig = status & 0x7F;
            int stat = status >> 8;
            fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS= %d\n", sig, stat);
            close(sockfd);
            exit(0);
        }

        // output from shell
        if (shellPoll->revents & POLLERR) {
            killProg("Received POLLERR from poll() on shell");
        }
        if (shellPoll->revents & POLLIN) {
            char shellBuf[SHELLBUF_SIZE];
            int charsRead = read(pRead, shellBuf, SHELLBUF_SIZE);
            if (charsRead == -1) {
                killProg("Error in read() to shell");
            }

            if (write(sockfd, shellBuf, charsRead) == -1) {
                killProg("Error writing chars to socket");
            }
        }
        if (shellPoll->revents & POLLHUP) {
            if (debug) {
                fprintf(stderr, "POLLHUP from shell");
            }

            int status;
            int waitRet = waitpid(pid, &status, 0);
            if (waitRet == -1) {
                killProg("Error in waitpit() for shell");
            }
            int sig = status & 0x7F;
            int stat = status >> 8;
            fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", sig, stat);
            close(sockfd);
            exit(0);
        }
    }
    return 0;
}

void killProg(const char* msg) {
    fprintf(stderr, "%s: %s: %s\n", progName, msg, strerror(errno));
    exit(1);
}

