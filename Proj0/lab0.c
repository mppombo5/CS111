#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <signal.h>

const char* progName = "lab0";
const size_t BYTE_BUFFER_SIZE = 1024;
const char* usageMsg = "usage: lab0 [--input=filename] [--output=filename] [--catch] [--segfault]";

int main(int argc, char** argv) {
    // variables to manipulate with options
    int usesInput = 0;
    char* infile;
    int usesOutput = 0;
    char* outfile;
    int crashProg = 0;
    int catchCrash = 0;

    // define options
    struct option options[] = {
            {"input", required_argument, NULL, 'i'},
            {"output", required_argument, NULL, 'o'},
            {"segfault", no_argument, NULL, 's'},
            {"catch", no_argument, NULL, 'c'},
            {0,0,0,0}
    };

    int optIndex;
    int ch = getopt_long(argc, argv, "i:o:sc", options, &optIndex);
    while (ch != -1) {
        switch (ch) {
            case 'i':
                infile = optarg;
                usesInput = 1;
                break;
            case 'o':
                outfile = optarg;
                usesOutput = 1;
                break;
            case 's':
                crashProg = 1;
                break;
            case 'c':
                catchCrash = 1;
                printf("option -c was passed\n");
                break;
            // if '?' is returned
            default:
                fprintf(stderr, "%s: unrecognized option argument.\n%s\n", progName, usageMsg);
                exit(1);
                break;
        }

        // option_index is the index of the option in options that we're using
        ch = getopt_long(argc, argv, "i:o:sc", options, &optIndex);
    }

    // handle --input option
    if (usesInput) {
        int tmpIFD = open(infile, O_RDONLY);
        if (tmpIFD == -1) {
            fprintf(stderr, "%s: error in argument --input, while opening %s: %s\n", progName, infile, strerror(errno));
            exit(2);
        }
        if (tmpIFD != STDIN_FILENO) {
            if (close(STDIN_FILENO) == -1 || dup2(tmpIFD, STDIN_FILENO) == -1 || close(tmpIFD) == -1) {
                fprintf(stderr, "%s: error in argument --input, while redirecting to %s: %s\n", progName, infile, strerror(errno));
                exit(2);
            }
        }
    }

    // handle --output option
    if (usesOutput) {
        int tmpOFD = creat(outfile, 0666);
        if (tmpOFD == -1) {
            fprintf(stderr, "%s: error in argument --output, while creating %s: %s\n", progName, outfile, strerror(errno));
            exit(3);
        }
        if (tmpOFD != STDOUT_FILENO) {
            if (close(STDOUT_FILENO) == -1 || dup2(tmpOFD, STDOUT_FILENO) == -1 || close(tmpOFD) == -1) {
                fprintf(stderr, "%s: error in argument --output, while redirecting to %s: %s\n", progName, outfile, strerror(errno));
                exit(3);
            }
        }
    }

    // register handler

    // handle --segfault option
    // this will be fun
    if (crashProg) {
        int* numbah = NULL;
        *numbah = 420;
    }

    // read into a larger buffer so we don't have to make a syscall for EVERY single byte
    char outbuf[BYTE_BUFFER_SIZE];
    int charsRead = read(STDIN_FILENO, outbuf, BYTE_BUFFER_SIZE);

    while (charsRead != 0) {
        // error!
        if (charsRead == -1) {
            fprintf(stderr, "%s: error in reading from standard input: %s\n", progName, strerror(errno));
            exit(1);
        }
        if (write(STDOUT_FILENO, outbuf, charsRead) == -1) {
            fprintf(stderr, "%s: error in writing to standard output: %s\n", progName, strerror(errno));
            exit(1);
        }
        charsRead = read(STDIN_FILENO, outbuf, BYTE_BUFFER_SIZE);
    }

    // close input file if one was specified
    if (usesInput && close(STDIN_FILENO) == -1) {
        fprintf(stderr, "%s: error in argument --input, while closing %s: %s\n", progName, infile, strerror(errno));
        exit(2);
    }

    // close output file if one was specified
    if (usesOutput && close(STDOUT_FILENO) == -1) {
        fprintf(stderr, "%s: error in argument --output, while closing %s: %s\n", progName, outfile, strerror(errno));
        exit(3);
    }

    return 0;
}
