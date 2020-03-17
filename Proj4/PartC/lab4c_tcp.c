/*
 * NAME: Matthew Pombo
 * EMAIL: mppombo5@gmail.com
 * UID: 405140036
 */

#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define LAB4_FAHRENHEIT 'F'
#define LAB4_CELSIUS 'C'
#define LAB4_TEMPSENSOR_PIN 1   // AIO pin number for temperature sensor
#define LAB4_BUTTON_PIN 60      // GPIO pin number for button
#define LAB4_BUFFERSIZE 256     // buffer size with which to read commands from stdin

int main(int argc, char* argv[]) {
    FILE* logfile = NULL;

    struct option options[] = {
            {"scale", required_argument, NULL, 's'},
            {"period", required_argument, NULL, 'p'},
            {"log", required_argument, NULL, 'l'},  // mandatory
            {"id", required_argument, NULL, 'i'},   // mandatory
            {"host", required_argument, NULL, 'h'}, // mandatory
            {"debug", no_argument, NULL, 'd'},
            {0,0,0,0}
    };

    return 0;
}
