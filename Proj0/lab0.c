#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>

const size_t BYTE_BUFFER_SIZE = 1024;

int main() {
    // read into a larger buffer so we don't have to make a syscall for EVERY single byte
    char outbuf[BYTE_BUFFER_SIZE];
    int charsRead = read(STDIN_FILENO, outbuf, BYTE_BUFFER_SIZE);

    while (charsRead != 0) {
        // error!
        if (charsRead == -1) {
            fprintf(stderr, "Error in reading from standard input.\n%s", strerror(errno));
            exit(1);
        }
        if (write(STDOUT_FILENO, outbuf, charsRead) == -1) {
            fprintf(stderr, "Error in writing to standard output.\n%s", strerror(errno));
            exit(1);
        }
        charsRead = read(STDIN_FILENO, outbuf, BYTE_BUFFER_SIZE);
    }

    return 0;
}
