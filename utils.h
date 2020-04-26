#ifndef CNP2_UTILS_H
#define CNP2_UTILS_H

typedef unsigned int uint;
#include "packet.h"
#include <sys/time.h>




bool equals(char *s1, char *s2);

uint mdval(uint seq);

uint mdadd(uint seq, uint val);

uint mdsub(uint seq, uint val);

uint md2val(uint seq);

uint md2add(uint seq, uint val);

uint md2sub(uint seq, uint val);

/*
 * Returns the size of the given file
 * Returns -1 if the file can't be opened.
 */
int getFileSize(char *file);

int max(int x, int y);

void initPacket(packet *pkt);

double convTimeval2MilliSec(struct timeval *tv);

void convMilliSec2Timeval(double milliSec, struct timeval *tv);

double findRemainingTime(struct timeval *startTime, struct timeval *remainingTime, double delayMS);


#endif //CNP2_UTILS_H
