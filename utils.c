#include "config.h"
#include "utils.h"
#include "packet.h"
#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<sys/sendfile.h>
#include <sys/time.h>
#include <stdbool.h>

/*
 * Tests whether strings s1 and s2 are equal
 */
bool equals(char *s1, char *s2){
    if(s1 == NULL && s2 == NULL)
        return true;
    else if(s1 == NULL || s2 == NULL)
        return false;
    else
        return (strcmp(s1,s2) == 0);
}

uint mdval(uint seq) {
    return (seq%WINDOW_SIZE);
}

uint mdadd(uint seq, uint val) {
    return ((seq+val)%WINDOW_SIZE);
}

uint mdsub(uint seq, uint val) {
    return (seq+WINDOW_SIZE-val)%WINDOW_SIZE;
}

uint md2val(uint seq) {
    return (seq%(2*WINDOW_SIZE));
}

uint md2add(uint seq, uint val) {
    return ((seq+val)%(2*WINDOW_SIZE));
}

uint md2sub(uint seq, uint val) {
    uint w2 = WINDOW_SIZE*2;
    return (seq+w2-val)%w2;
}

/*
 * Returns the size of the given file
 * Returns -1 if the file can't be opened.
 */
int getFileSize(char *file) {
    int fd = open(file,O_RDONLY);
    if(fd == -1)
        return -1;
    int size = lseek(fd,0,SEEK_END);
    close(fd);
    return size;
}

int max(int x, int y){
    return (x>y)?x:y;
}

void initPacket(packet *pkt){
    if(pkt == NULL)
        return;
    pkt->size = 0;
    pkt->seq = 0;
    pkt->isLastPkt = false;
    pkt->ptype = ACK_PKT;
    memset(pkt->payload,0,sizeof(pkt->payload));
}

double convTimeval2MilliSec(struct timeval *tv){
    double tt;
    tt = (tv->tv_sec)*1e6;
    tt = (tt + tv->tv_usec)* 1e-3;
    return tt;
}

void convMilliSec2Timeval(double milliSec, struct timeval *tv){
    long long usec = ((milliSec*1e3) + 0.5);
    tv->tv_sec = usec/1000000;
    tv->tv_usec = usec % 1000000;
}

double findRemainingTime(struct timeval *startTime, struct timeval *remainingTime, double delayMS) {
    struct timeval currTime, tmpDiff;
    gettimeofday(&currTime,NULL);
    timerclear(remainingTime);
    timersub(&currTime,startTime,&tmpDiff);
    if(convTimeval2MilliSec(&tmpDiff) < delayMS){
        struct timeval timeoutVal;
        convMilliSec2Timeval(delayMS,&timeoutVal);
        timersub(&timeoutVal,&tmpDiff,remainingTime);
        return convTimeval2MilliSec(remainingTime);
    } else
        return 0;
}

