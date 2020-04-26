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

uint mdval(uint seq) {
    return (seq%WINDOW_SIZE);
}

uint mdadd(uint seq, uint val) {
    return ((seq+val)%WINDOW_SIZE);
}

uint mdsub(uint seq, uint val) {
    return (seq+WINDOW_SIZE-val)%WINDOW_SIZE;
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