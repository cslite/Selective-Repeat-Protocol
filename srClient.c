#include "packet.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
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
packet *sentPkt[WINDOW_SIZE];
struct timeval sentPktTime[WINDOW_SIZE];
uint sendBase;
uint nextSeqNum;
bool morePacketsAvailable;
struct timeval resetTimeoutValue;
uint stopSeqNum;

void initClientGlobals(){
    nextSeqNum = 0;
    sendBase = 0;
    morePacketsAvailable = true;
    currRead = 0;
    convMilliSec2Timeval(TIMEOUT_MILLISECONDS,&resetTimeoutValue);
    memset(sentPkt,0,sizeof(sentPkt));
}

bool initClientSocket(struct sockaddr_in *sa, int cliPort){
    bzero(sa,sizeof(*sa));
    sockfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(sockfd < 0){
        perror("initClientSocket");
        return false;
    }
    sa->sin_family = AF_INET;
    sa->sin_addr.s_addr = htonl(INADDR_ANY);
    sa->sin_port = htons(cliPort);
    if(bind(sockfd, (struct sockaddr *) sa,sizeof(*sa))<0){
        fprintf(stderr,"initSocket: Error in binding client socket.\n");
        return false;
    }
    fprintf(stdout,"Client now bound to port %d.\n",cliPort);
    return true;
}

void init2RelayAddr(struct sockaddr_in relayAddr[], int port1, int port2){
    //init first relayAddr
    bzero(&(relayAddr[0]),sizeof(relayAddr[0]));
    relayAddr[0].sin_family = AF_INET;
    relayAddr[0].sin_port = htons(port1);
    relayAddr[0].sin_addr.s_addr = inet_addr(RELAY_NODE_1_IP);


    //init second relayAddr
    bzero(&(relayAddr[1]),sizeof(relayAddr[1]));
    relayAddr[1].sin_family = AF_INET;
    relayAddr[1].sin_port = htons(port2);
    relayAddr[1].sin_addr.s_addr = inet_addr(RELAY_NODE_2_IP);
//    inet_pton(AF_INET,RELAY_NODE_2_IP,&(relayAddr[1].sin_addr));

}

packet *makeNextPktFromFile(){
    packet *ptmp = (packet *)malloc(sizeof(packet));
    initPacket(ptmp);
    ptmp->seq = nextSeqNum;
    nextSeqNum = md2add(nextSeqNum,1);
    ptmp->ptype = DATA_PKT;
    int actualRead = read(inputFd,ptmp->payload,PACKET_SIZE);
    if(actualRead < 0){
        perror("open");
        free(ptmp);
        return NULL;
    }
    else if(actualRead == 0){
        free(ptmp);
        return NULL;
    }
    ptmp->size = actualRead;
    currRead += actualRead;
    if(currRead == fileSize){
        ptmp->isLastPkt = true;
        morePacketsAvailable = false;
        stopSeqNum = md2add(ptmp->seq,1);
    }
    else
        ptmp->isLastPkt = false;
    return ptmp;
}

bool isPktInSendWindow(uint seq){
    if(seq >= sendBase || seq < md2add(sendBase,WINDOW_SIZE))
        return true;
    else
        return false;
}

bool sendDataPkt(packet *pkt, struct sockaddr_in relayAddr[]){
    if(pkt == NULL)
        return false;
    int destIdx = (pkt->seq) % 2 ? 0 : 1;
    int ret;


    if((ret = sendto(sockfd,pkt,sizeof(packet),0,(struct sockaddr*)(relayAddr+destIdx),sizeof(relayAddr[destIdx]))) <= 0){
        fprintf(stderr,"sendto returned %d\n",ret);
        perror("sendto");
        return false;
    }
    uint idx = mdval(pkt->seq);
    gettimeofday(&sentPktTime[idx],NULL);
    sentPkt[idx] = pkt;
    //TODO: log SEND event
    if(DEBUG_MODE)
        fprintf(stderr,"[DEBUG]: SENT PKT: Seq No. %u of size %u bytes.\n",pkt->seq,pkt->size);
    return true;
}

bool receiveAckPkt(packet *pkt){
    struct sockaddr_in cliAddr;
    unsigned int clilen;
    int nread;
    if((nread = recvfrom(sockfd,pkt,sizeof(packet),0,(struct sockaddr*)&cliAddr,&clilen)) < 0){
        perror("recvfrom");
        return false;
    }
    else if(nread == 0){
        fprintf(stderr,"recvfrom returned 0.\n");
        return false;
    }
    if(DEBUG_MODE)
        fprintf(stderr,"[DEBUG]: ACK with seq %u received.\n",pkt->seq);
    return true;
}

bool advanceSendBase(struct sockaddr_in relayAddr[]){
    do{
        if(sentPkt[mdval(sendBase)] == NULL){
            sendBase = md2add(sendBase,1);
            //can send additional packet
            if (morePacketsAvailable) {
                packet *nextPkt = makeNextPktFromFile();
                if (!sendDataPkt(nextPkt, relayAddr))
                    return false;
            }
        }
        else
            break;
    }while(sendBase != nextSeqNum);
    if(DEBUG_MODE)
        fprintf(stderr,"[DEBUG]: sendBase now at %u\n",sendBase);
    return true;
}

bool srSendFile(char *fileName, int relay1port, int relay2port, int cliPort) {
    //check for errors in file
    if (fileName == NULL) {
        fprintf(stderr, "srSendFile: Invalid file name received.\n");
        return false;
    }
    fileSize = getFileSize(fileName);
    if (fileSize == -1) {
        fprintf(stderr, "srSendFile: Cannot open file '%s'.\n", fileName);
        return false;
    } else if (fileSize == 0) {
        fprintf(stderr, "srSendFile: File '%s' is empty.\n", fileName);
        return false;
    }

    struct sockaddr_in relayAddr[2];
    struct sockaddr_in cliAddr;
    if(!initClientSocket(&cliAddr,cliPort))
        return false;
    init2RelayAddr(relayAddr, relay1port, relay2port);
    inputFd = open(fileName, O_RDONLY);
    initClientGlobals();
    fd_set rset, rset0;
    FD_ZERO(&rset0);
    FD_SET(sockfd, &rset0);
    packet *nextPkt;

    if(DEBUG_MODE)
        fprintf(stderr,"[DEBUG]: Client ready to send initial packets.\n");

    //send initial packets
    for (int i = 0; i < WINDOW_SIZE; i++) {
        if (morePacketsAvailable) {
            nextPkt = makeNextPktFromFile();
            if (!sendDataPkt(nextPkt, relayAddr))
                return false;
//            sleep(20);
        } else break;
    }

    struct timeval timeRemaining = resetTimeoutValue;
    while (morePacketsAvailable || sendBase != stopSeqNum) {
        rset = rset0;
        int nsel = select(sockfd + 1, &rset, NULL, NULL, &timeRemaining);

        if (FD_ISSET(sockfd, &rset)) {
            if(DEBUG_MODE)
                fprintf(stderr,"[DEBUG]: Some ACK is available for reading.\n");
            //ack available
            packet tmpPkt;
            if (!receiveAckPkt(&tmpPkt))
                return false;
            if (isPktInSendWindow(tmpPkt.seq)) {
                if(DEBUG_MODE)
                    fprintf(stderr,"[DEBUG]: ACK %u is in SendWindow.\n",tmpPkt.seq);
                uint idx = mdval(tmpPkt.seq);
                free(sentPkt[idx]);
                sentPkt[idx] = NULL;
                if (tmpPkt.seq == sendBase) {
                    if(DEBUG_MODE)
                        fprintf(stderr,"[DEBUG]: sendBase can be moved.\n");
                    if (!advanceSendBase(relayAddr))
                        return false;
                }
            } else {
                //discard packet
            }
        }

        bool linit = false;
        double leastTime = TIMEOUT_MILLISECONDS, currTime;
        struct timeval leastTimeVal = resetTimeoutValue;
        struct timeval currTimeVal;
        if(DEBUG_MODE)
            fprintf(stderr,"[DEBUG]: Checking if there are any timeouts.\n");
        for (uint i = 0; i < WINDOW_SIZE; i++) {
//            if(DEBUG_MODE)
//                fprintf(stderr,"[DEBUG]: i = %u\n",i);
            if (sentPkt[i] != NULL) {
                currTime = findRemainingTime(&sentPktTime[i], &currTimeVal, TIMEOUT_MILLISECONDS);
                if(DEBUG_MODE)
                    fprintf(stderr,"[DEBUG]: Pkt with seq %u has %lf time remaining.\n",sentPkt[i]->seq,currTime);
                if (currTime == 0) {
                    //timeout for this packet
                    if(DEBUG_MODE)
                        fprintf(stderr,"[DEBUG]: Timeout for Pkt with seq %u.\n",sentPkt[i]->seq);
                    if (!sendDataPkt(sentPkt[i], relayAddr))
                        return false;
                }
                else if (linit) {
                    if (currTime < leastTime) {
                        leastTime = currTime;
                        leastTimeVal = currTimeVal;
                    }
                } else {
                    linit = true;
                    leastTime = currTime;
                    leastTimeVal = currTimeVal;
                }
            }
        }
        if(DEBUG_MODE)
            fprintf(stderr,"[DEBUG]: Next return will be within %lf milliseconds.\n",leastTime);
        timeRemaining = leastTimeVal;
    }

    close(inputFd);
    close(sockfd);
    return true;
}

int main(int argc, char *argv[]){
    if(argc != 2) {
        fprintf(stderr,"Usage: %s <input file>\n", argv[0]);
        exit(1);
    }
    if(!srSendFile(argv[1], RELAY_NODE_1_PORT, RELAY_NODE_2_PORT,CLIENT_PORT)){
        fprintf(stderr,"Some Error Occurred!\n");
        return -1;
    }
    return 0;
}
