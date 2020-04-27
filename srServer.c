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

int outputFd;
bool receivedLastPkt;
bool pendingLastPktWrite;
char oooBuffer[WINDOW_SIZE][PACKET_SIZE+1];
bool receivedPkt[WINDOW_SIZE];
uint recvBase;
int sockfd;
uint lastPktSeqMod;
FILE *logFile;


void initServerGlobals(){
    receivedLastPkt = false;
    pendingLastPktWrite = true;
    memset(receivedPkt,false,sizeof(receivedPkt));
    recvBase = 0;
    lastPktSeqMod = 0;
}

bool initSocket(struct sockaddr_in *sa, int servPort){
    bzero(sa,sizeof(*sa));
    sockfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(sockfd < 0){
        fprintf(stderr,"initSocket: Error in opening socket.\n");
        return false;
    }
    int en = 1;
//    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &en, sizeof(int));
    sa->sin_family = AF_INET;
    sa->sin_addr.s_addr = htonl(INADDR_ANY);
    sa->sin_port = htons(servPort);
    if(bind(sockfd, (struct sockaddr *) sa,sizeof(*sa))<0){
        fprintf(stderr,"initSocket: Error in binding server socket.\n");
        return false;
    }
    fprintf(stdout,"Server now accepting datagrams on port %d.\n",servPort);
    return true;
}

//checks if the packet belongs to our window or not
bool isPktInRecvWindow(uint seq){
    if(seq >= recvBase || seq <= md2add(recvBase,WINDOW_SIZE-1))
        return true;
    else
        return false;
//    if(seq >= recvBase && seq <= (recvBase+WINDOW_SIZE-1))
//        return true;
//    else
//        return false;
}

//checks if this packet was received again
bool isPktRetransmitted(uint seq){
    if(seq >= md2sub(recvBase,WINDOW_SIZE) || seq <= md2sub(recvBase,1))
        return true;
    else
        return false;
//    if(seq < recvBase)
//        return true;
//    else
//        return false;
}

bool loadDataOnBuffer(packet *pkt){
    if(pkt == NULL){
        fprintf(stderr,"loadDataOnBuffer: Received a NULL Node.\n");
        return false;
    }
    uint seq = mdval(pkt->seq);
    if(receivedPkt[seq]){
        fprintf(stderr,"loadDataOnBuffer: Error, this packet is already marked received.\n");
        return false;
    }
    receivedPkt[seq] = true;
    strcpy(oooBuffer[seq],pkt->payload);
    return true;
}

bool processPkt(packet *pkt){
    if(pkt == NULL){
        fprintf(stderr,"processPkt: Received a NULL Node.\n");
        return false;
    }
    if(!loadDataOnBuffer(pkt))
        return false;
    if(pkt->seq == recvBase){
        if(DEBUG_MODE)
            fprintf(stderr,"[DEBUG]: Moving the receive base.\n");
        //time to move the window
        uint start = mdval(pkt->seq);
        uint i = start;
        do{
            if(receivedPkt[i]){
                recvBase = md2add(recvBase,1);
                receivedPkt[i] = false;
                write(outputFd,oooBuffer[i],strlen(oooBuffer[i]));
                if(receivedLastPkt && lastPktSeqMod == i)
                    pendingLastPktWrite = false;
            }
            else{
                break;
            }
            i = mdadd(i,1);
        }while(i != start);
    }
    else{
        //nothing to do
    }
    return true;
}

bool sendAckPkt(packet *pkt, struct sockaddr_in *cliAddr, uint *cliLen){
    if(DEBUG_MODE)
        fprintf(stderr,"[DEBUG]: Sending ACK packet for seq %u.\n",pkt->seq);
    if(pkt == NULL || cliAddr == NULL || cliLen == NULL){
        fprintf(stderr,"sendAckPkt: Received a NULL Node.\n");
        return false;
    }
    int nwrite = sendto(sockfd,pkt,sizeof(packet),0,(struct sockaddr*)cliAddr,*cliLen);
    if(nwrite < 0){
        perror("sendto");
        return false;
    }
    else if(nwrite == 0){
        fprintf(stderr,"sendto: Some Error, Return value 0.\n");
        return false;
    }
    logEntryNode le = {NN_SERVER,E_SEND,0,ACK_PKT,pkt->seq,NN_SERVER,(pkt->seq)%2,NULL};
    addNewLogEntry(le,logFile);
    return true;
}

bool srReceiveFile(char *saveFileName, int port){
    if(saveFileName == NULL){
        fprintf(stderr,"srReceiveFile: Invalid file name received.\n");
        return false;
    }
    FILE *fp = fopen(saveFileName,"w");
    outputFd = fileno(fp);
    if(outputFd == -1){
        fprintf(stderr,"srReceiveFile: Unable to open file for writing.\n");
        return false;
    }

    //server accepts packets from Relay nodes on given port
    struct sockaddr_in servAddr;
    struct sockaddr_in cliAddr;
    uint cliLen;

    if(!initSocket(&(servAddr),port))
        return false;

    initServerGlobals();

    while(pendingLastPktWrite){
        packet tmpPkt;
        if(DEBUG_MODE)
            fprintf(stderr,"[DEBUG]: Server ready to receive packet.\n");
//        fprintf(stderr,"we have sockfd = %d\n",sockfd);
        bzero(&cliAddr,sizeof(cliAddr));
        cliLen = sizeof(cliAddr);
        int nread = recvfrom(sockfd,&tmpPkt,sizeof(packet),0,(struct sockaddr *)&cliAddr,&cliLen);
        if(nread < 0){
            perror("recvfrom");
            return false;
        }
        else if(nread == 0){
            fprintf(stderr,"recvfrom: Some Error, Received empty packet.\n");
            return false;
        }
        //received a packet
        if(DEBUG_MODE)
            fprintf(stderr,"[DEBUG]: Pkt with seq %u received.\n",tmpPkt.seq);
        logEntryNode le = {NN_SERVER,E_RECV,0,DATA_PKT,tmpPkt.seq,(tmpPkt.seq)%2,NN_SERVER,NULL};
        addNewLogEntry(le,logFile);
        if(isPktInRecvWindow(tmpPkt.seq)){
            //this packet can be added to buffer
            if(DEBUG_MODE)
                fprintf(stderr,"[DEBUG]: Packet is in receiveWindow\n");
            if(tmpPkt.isLastPkt){
                receivedLastPkt = true;
                lastPktSeqMod = mdval(tmpPkt.seq);
            }
            if(!processPkt(&tmpPkt))
                return false;
            tmpPkt.payload[0] = '\0';
            tmpPkt.ptype = ACK_PKT;
            if(!sendAckPkt(&tmpPkt,&cliAddr,&cliLen))
                return false;
        }
        else if(isPktRetransmitted(tmpPkt.seq)){
            tmpPkt.payload[0] = '\0';
            tmpPkt.ptype = ACK_PKT;
            if(!sendAckPkt(&tmpPkt,&cliAddr,&cliLen))
                return false;
        }
        else{
            //Discard this packet, it is not of our use
            logEntryNode le = {NN_SERVER,E_DROP,0,tmpPkt.ptype,tmpPkt.seq,(tmpPkt.seq)%2,NN_SERVER,NULL};
            addNewLogEntry(le,logFile);
            if(DEBUG_MODE)
                fprintf(stderr,"srReceiveFile: Discarding Packet with Seq %u\n",tmpPkt.seq);
        }

    }
    close(outputFd);
    close(sockfd);
    return true;
}

int main(int argc, char *argv[]){
    if(argc != 2) {
        fprintf(stderr,"Usage: %s <output file>\n", argv[0]);
        exit(1);
    }
    logFile = fopen(TMP_SERVER_LOG,"w");
    if(!srReceiveFile(argv[1],SERVER_PORT)){
        fprintf(stderr,"Some Error Occurred!\n");
        close(outputFd);
        close(sockfd);
        fclose(logFile);
        return -1;
    }
    fclose(logFile);
    return 0;
}