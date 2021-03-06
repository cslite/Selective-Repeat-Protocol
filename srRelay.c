/*
 * NAME: TUSSANK GUPTA
 * ID: 2016B3A70528P
 */
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
#include <signal.h>
#include "config.h"

/*GLOBAL VARIABLES*/
packet *pktStore[2*WINDOW_SIZE];
struct timeval pktReceiveTime[2 * WINDOW_SIZE];
double pktDelayTime[2*WINDOW_SIZE];
int sockfd;
struct sockaddr_in servAddr, cliAddr;
struct timeval resetTimeoutValue;
bool loopOver;
int cntNonNullPkts;
char *logFileName;
FILE *logFile;
/*---*/

void handleSigint(int signo){
    loopOver = true;
    close(sockfd);
    if(logFile)
        fclose(logFile);
    fprintf(stderr,"Exiting...\n");
    exit(1);
}

void initRelayGlobals(){
    memset(pktStore,0,sizeof(pktStore));
    srand(time(0));
    convMilliSec2Timeval(TIMEOUT_MILLISECONDS,&resetTimeoutValue);
    loopOver = false;
    cntNonNullPkts = 0;
}

void addNewRelayLogEntry(logEntryNode len){
    logFile = fopen(logFileName,"a");
    if(!logFile) logFile = fopen(logFileName,"w");
    addNewLogEntry(len,logFile);
    fclose(logFile);
    logFile = NULL;
}

bool initRelaySocket(struct sockaddr_in *sa, int relayPort){
    bzero(sa,sizeof(*sa));
    sockfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(sockfd < 0){
        fprintf(stderr,"initRelaySocket: Error in opening socket.\n");
        return false;
    }
    sa->sin_family = AF_INET;
    sa->sin_addr.s_addr = htonl(INADDR_ANY);
    sa->sin_port = htons(relayPort);
    if(bind(sockfd, (struct sockaddr *) sa,sizeof(*sa))<0){
        fprintf(stderr,"initRelaySocket: Error in binding relay socket.\n");
        return false;
    }
    fprintf(stdout,"Relay Server now accepting datagrams on port %d.\n",relayPort);
    return true;
}

void initAddrs(struct sockaddr_in *servAddr, struct sockaddr_in *cliAddr){
    //init servAddr
    bzero(servAddr,sizeof(*servAddr));
    servAddr->sin_family = AF_INET;
    servAddr->sin_port = htons(SERVER_PORT);
    servAddr->sin_addr.s_addr = inet_addr(SERVER_IP);

    //init cliAddr
    bzero(cliAddr,sizeof(*cliAddr));
    cliAddr->sin_family = AF_INET;
    cliAddr->sin_port = htons(CLIENT_PORT);
    cliAddr->sin_addr.s_addr = inet_addr(CLIENT_IP);
}

bool forwardDataPkt(uint idx){
    int nwrite = sendto(sockfd,pktStore[idx],sizeof(packet),0,(struct sockaddr*)&servAddr,sizeof(servAddr));
    if(nwrite < 0){
        perror("sendto");
        free(pktStore[idx]);
        pktStore[idx] = NULL;
        return false;
    }
    else if(nwrite == 0){
        fprintf(stderr,"sendto: Some Error, Return value 0.\n");
        free(pktStore[idx]);
        pktStore[idx] = NULL;
        return false;
    }
    logEntryNode le = {(pktStore[idx]->seq)%2,E_SEND,DATA_PKT,pktStore[idx]->seq,(pktStore[idx]->seq)%2,NN_SERVER};
    addNewRelayLogEntry(le);
    if(DEBUG_MODE)
        fprintf(stderr,"[DEBUG]: Sent DATA Pkt with seq %u to Server.\n",pktStore[idx]->seq);
    free(pktStore[idx]);
    pktStore[idx] = NULL;
    cntNonNullPkts--;
    return true;
}

bool processDataPkt(packet *pkt){
    int x = rand()%100;
    if(x <= PACKET_DROP_RATE){
        logEntryNode le = {(pkt->seq)%2,E_DROP,DATA_PKT,pkt->seq,NN_CLIENT,(pkt->seq)%2};
        addNewRelayLogEntry(le);
        if(DEBUG_MODE)
            fprintf(stderr,"[DEBUG]: Dropping pkt with seq %u (randVal = %d).\n",pkt->seq,x);
        return true;
    }
    else{
        //find random delay in range 0 to 2 milliseconds
        double dlay = (rand()%21) * 0.1;
        uint idx = pkt->seq;
        if(pktStore[idx] != NULL)
            forwardDataPkt(idx);
        pktStore[idx] = (packet *)malloc(sizeof(packet));
        *(pktStore[idx]) = *pkt;
        for(uint i=0; i<(pkt->size);i++){
            (pktStore[idx]->payload)[i] = (pkt->payload)[i];
        }
        pktDelayTime[idx] = dlay;
        gettimeofday(&(pktReceiveTime[idx]),NULL);
        if(DEBUG_MODE)
            fprintf(stderr,"[DEBUG]: Pkt with seq %u delayed by %lf ms.\n",pkt->seq,dlay);
        cntNonNullPkts++;
        return true;
    }
}

bool forwardACKPkt(packet *pkt){
    int nwrite = sendto(sockfd,pkt,sizeof(packet),0,(struct sockaddr*)&cliAddr,sizeof(cliAddr));
    if(nwrite < 0){
        perror("sendto");
        return false;
    }
    else if(nwrite == 0){
        fprintf(stderr,"sendto: Some Error, Return value 0.\n");
        return false;
    }
    logEntryNode le = {(pkt->seq)%2,E_SEND,ACK_PKT,pkt->seq,(pkt->seq)%2,NN_CLIENT};
    addNewRelayLogEntry(le);
    if(DEBUG_MODE)
        fprintf(stderr,"[DEBUG]: Sent ACK with seq %u to Client.\n",pkt->seq);
    return true;
}

bool runRelay(uint relayPort){

    struct sockaddr_in relayAddr;

    if(!initRelaySocket(&(relayAddr),relayPort))
        return false;

    initAddrs(&servAddr,&cliAddr);
    initRelayGlobals();
    fd_set rset0,rset;
    FD_ZERO(&rset0);
    FD_SET(sockfd,&rset0);

    if(DEBUG_MODE)
        fprintf(stderr,"[DEBUG]: Relay ready.\n");
    fprintf(stdout,"%-11s%-13s%-19s%-13s%-10s%-9s%-9s\n","Node Name","Event Type","Timestamp","Packet Type","Seq. No.","Source","Dest");


    //just for starting
    struct timeval timeRemaining = resetTimeoutValue;
    struct sockaddr_in tmpAddr;
    uint tmpLen;
    while(1){
        rset = rset0;
        int nsel;
        if(cntNonNullPkts > 0)
            nsel = select(sockfd+1,&rset,NULL,NULL,&timeRemaining);
        else
            nsel = select(sockfd+1,&rset,NULL,NULL,NULL);
        if(loopOver)
            break;
        if(FD_ISSET(sockfd,&rset)){
            //packet arrived
            if(DEBUG_MODE)
                fprintf(stderr,"[DEBUG]: Now reading an incoming packet.\n");
            packet tmpPkt;
            tmpLen = sizeof(tmpAddr);
            int nread = recvfrom(sockfd,&tmpPkt,sizeof(packet),0,(struct sockaddr *)&tmpAddr,&tmpLen);
            if(nread < 0){
                perror("recvfrom");
                return false;
            }
            else if(nread == 0){
                fprintf(stderr,"recvfrom: Some Error, Received empty packet.\n");
                return false;
            }
            if(tmpPkt.ptype == ACK_PKT){
                logEntryNode le = {(tmpPkt.seq)%2,E_RECV,ACK_PKT,tmpPkt.seq,NN_SERVER,(tmpPkt.seq)%2};
                addNewRelayLogEntry(le);
                if(DEBUG_MODE)
                    fprintf(stderr,"[DEBUG]: ACK Packet with seq %u found.\n",tmpPkt.seq);
                if(!forwardACKPkt(&tmpPkt))
                    return false;
            }
            else{
                logEntryNode le = {(tmpPkt.seq)%2,E_RECV,DATA_PKT,tmpPkt.seq,NN_CLIENT,(tmpPkt.seq)%2};
                addNewRelayLogEntry(le);
                if(DEBUG_MODE)
                    fprintf(stderr,"[DEBUG]: DATA Packet with seq %u found.\n",tmpPkt.seq);
                if(!processDataPkt(&tmpPkt))
                    return false;
            }
        }
        bool linit = false;
        double leastTime = TIMEOUT_MILLISECONDS, currTime;
        struct timeval leastTimeVal = resetTimeoutValue;
        struct timeval currTimeVal;
        if(cntNonNullPkts > 0){
            if(DEBUG_MODE)
                fprintf(stderr,"[DEBUG]: Now checking for timeouts.\n");
            for(uint i=0; i<(2*WINDOW_SIZE); i++){
                if (pktStore[i] != NULL) {
                    currTime = findRemainingTime(&pktReceiveTime[i], &currTimeVal, pktDelayTime[i]);
                    if (currTime == 0) {
                        //timeout for this packet
                        if(DEBUG_MODE)
                            fprintf(stderr,"[DEBUG]: Timeout for Pkt with seq %u.\n",pktStore[i]->seq);
                        if (!forwardDataPkt(i))
                            return false;
                    }
                    else if (linit) {
                        if (currTime < leastTime) {
                            leastTime = currTime;
                            leastTimeVal = currTimeVal;
                        }
                    } else {/*---*/
                        linit = true;
                        leastTime = currTime;
                        leastTimeVal = currTimeVal;
                    }
                }
            }
            timeRemaining = leastTimeVal;
            if(DEBUG_MODE)
                fprintf(stderr,"[DEBUG]: Next return will be within %lf milliseconds.\n",leastTime);
        }
        if(loopOver)
            break;
    }
    return true;
}

int main(int argc, char *argv[]){
    if(argc != 2) {
        fprintf(stderr,"Usage: %s 1\nor\nUsage: %s 2\n", argv[0],argv[0]);
        exit(1);
    }
    signal(SIGINT,handleSigint);
    if(equals(argv[1],"1")){
        logFileName = TMP_RELAY_1_LOG;
        logFile = fopen(logFileName,"w");
        fclose(logFile);
        if(!runRelay(RELAY_NODE_1_PORT)){
            close(sockfd);
            fprintf(stderr,"Some error occurred.\n");
            return -1;
        }
    }
    else if(equals(argv[1],"2")){
        logFileName = TMP_RELAY_2_LOG;
        logFile = fopen(logFileName,"w");
        fclose(logFile);
        logFile = NULL;
        if(!runRelay(RELAY_NODE_2_PORT)){
            close(sockfd);
            fprintf(stderr,"Some error occurred.\n");
            return -1;
        }
    }
    else{
        fprintf(stderr,"Usage: %s 1\nor\nUsage: %s 2\n", argv[0],argv[0]);
        exit(1);
    }

    return 0;
}