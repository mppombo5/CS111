/*
 * NAME: Matthew Pombo
 * EMAIL: mppombo5@gmail.com
 * UID: 405140036
 */

#ifndef PARTC_UTILITIES_H
#define PARTC_UTILITIES_H

#define LAB4_FAHRENHEIT    'F'
#define LAB4_CELSIUS       'C'
#define LAB4_TEMPSENSOR_PIN 1       // AIO pin number for temperature sensor
#define LAB4_TLS_BUFFERSIZE 256
typedef char scale_t;

extern _Bool debug;

void killProg(const char* msg, int exitStat);

double  CtoF(double tempC);
double  RawtoC(int raw);
void    sampleTemp(int sockfd, mraa_aio_context* sensor, scale_t tempScale,
                   FILE* log, time_t* curTime);

#endif //PARTC_UTILITIES_H
