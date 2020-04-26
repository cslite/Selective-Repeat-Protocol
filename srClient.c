#include "packet.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <stdbool.h>
#include <time.h>
#include "config.h"

int fileSize;
int sockfd;
int inputFd;
int currRead;
packet *SentPkt[WINDOW_SIZE];

void init2RelayAddr(struct sockaddr_in relayAddr[], int port1, int port2){
    //init first relayAddr
    bzero(&(relayAddr[0]),sizeof(relayAddr[0]));
    relayAddr[0].sin_family = AF_INET;
    relayAddr[0].sin_port = htons(port1);
    inet_pton(AF_INET,RELAY_NODE_1_IP,&(relayAddr[0].sin_addr));

    //init second relayAddr
    bzero(&(relayAddr[1]),sizeof(relayAddr[1]));
    relayAddr[2].sin_family = AF_INET;
    relayAddr[2].sin_port = htons(port2);
    inet_pton(AF_INET,RELAY_NODE_2_IP,&(relayAddr[1].sin_addr));

}

bool srSendFile(char *fileName, int relay1port, int relay2port){
    if(fileName == NULL){
        fprintf(stderr,"srSendFile: Invalid file name received.\n");
        return false;
    }
    fileSize = getFileSize(fileName);
    if(fileSize == -1){
        fprintf(stderr,"srSendFile: Cannot open file '%s'.\n",fileName);
        return false;
    }
    else if(fileSize == 0){
        fprintf(stderr,"srSendFile: File '%s' is empty.\n",fileName);
        return false;
    }

    struct sockaddr_in relayAddr[2];
    sockfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    init2RelayAddr(relayAddr,relay1port,relay2port);

    inputFd = open(fileName,O_RDONLY);

}
