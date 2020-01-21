#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#define BUFFER_SIZE 64

const char* progName = "lab1a";
void killProg(const char* errMsg);

// separate termios struct to restore in atexit()
struct termios initSettings;
void restoreAttr();

int main(int argc, char** argv) {
    // first get the current termios settings
    if (tcgetattr(STDIN_FILENO, &initSettings) == -1) {
        killProg("Error getting initial termios attributes");
    }
    if (atexit(&restoreAttr) != 0) {
        killProg("Unable to register atexit() function");
    }

    struct termios newSettings = initSettings;
    newSettings.c_iflag = ISTRIP;
    newSettings.c_oflag = 0;
    newSettings.c_lflag &= ~(ICANON|ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newSettings);

    int useShell = 0;
    char* shell;
    int debug = 0;

    struct option options[] = {
            {"shell", optional_argument, NULL, 's'},
            {"debug", no_argument, NULL, 'd'},
            {0,0,0,0}
    };

    int optIndex;
    int ch = getopt_long(argc, argv, "s::", options, &optIndex);
    while (ch != -1) {
        switch (ch) {
            case 's':
                useShell = 1;
                shell = (optarg == 0) ? "/bin/bash" : optarg;
                break;
            case 'd':
                debug = 1;
                break;
            default:
                killProg("Unrecognized option argument");
        }

        ch = getopt_long(argc, argv, "i::", options, &optIndex);
    }

    /*
    if (useShell) {
        printf("--shell=%s passed!", shell);
    }
     */

    // pipe[0] is read, pipe[1] is write
    int pipe1[2];
    int pipe2[2];
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        killProg("Error creating pipe in parent process");
    }

    int pid = fork();
    if (pid < 0) {
        killProg("Error in forking into two processes");
    }
    else if (pid == 0) {
        // child process
        // read through pipe2[0], write through pipe1[1]
        if (close(pipe1[0]) == -1 || close(pipe2[1]) == -1) {
            killProg("Error closing pipes in child process");
        }

        // make the read end of the pipe be the shell's standard input
        if (close(STDIN_FILENO) == -1 || dup2(pipe2[0], STDIN_FILENO) == -1) {
            killProg("Error redirecting pipe to stdin in child process");
        }
        // make stderr/stdout of the shell go to the write end
        if (close(STDOUT_FILENO) == -1 || dup2(pipe1[1], STDOUT_FILENO) == -1) {
            killProg("Error redirecting stdout to pipe in child process");
        }
        if (close(STDERR_FILENO) == -1 || dup2(pipe1[1], STDERR_FILENO) == -1) {
            killProg("Error redirecting stderr to pipe in child process");
        }
    }
    else {
        // parent process
        // read through pipe1[0], write through pipe2[1]
        if (close(pipe2[0]) == -1 || close(pipe1[1]) == -1) {
            killProg("Error closing pipes in parent process");
        }
    }

    char byteBuf[BUFFER_SIZE];
    int charsRead = read(STDIN_FILENO, byteBuf, BUFFER_SIZE);
    int reachedEnd = 0;
    while (charsRead != 0) {
        if (charsRead == -1) {
            killProg("Error reading from standard input");
        }
        for (int i = 0; i < charsRead; i++) {
            if (byteBuf[i] == EOF || (char) byteBuf[i] == 0x04) {
                reachedEnd = 1;
                break;
            }
            char curChar = byteBuf[i];
            if (curChar == '\n' || curChar == '\r') {
                char bytes[2] = {'\r', '\n'};
                if (write(STDOUT_FILENO, bytes, 2) == -1) {
                    killProg("Error writing <cr><lf> to standard output");
                }
            }
            else {
                if (write(STDOUT_FILENO, byteBuf + i, 1) == -1) {
                    killProg("Error writing to standard output");
                }
            }
        }
        if (reachedEnd) {
            break;
        }

        charsRead = read(STDIN_FILENO, byteBuf, BUFFER_SIZE);
    }

    return 0;
}

void killProg(const char* errMsg) {
    fprintf(stderr, "%s: %s: %s", progName, errMsg, strerror(errno));
    exit(1);
}

void restoreAttr() {
    if (tcsetattr(STDIN_FILENO, TCSANOW, &initSettings) == -1) {
        killProg("Unable to restore initial termios settings");
    }
}
