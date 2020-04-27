/*
 * NAME: TUSSANK GUPTA
 * ID: 2016B3A70528P
 */
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

char *getNNtext(nodeName nn){
    switch(nn){
        case NN_SERVER:
            return "SERVER";
        case NN_CLIENT:
            return "CLIENT";
        case NN_RELAY1:
            return "RELAY1";
        case NN_RELAY2:
            return "RELAY2";
    }
}

void printLogEntry(logEntryNode *len, FILE *fp){
    char *nn, *src, *dst, *pt, *et;
    nn = getNNtext(len->nn);
    src = getNNtext(len->src);
    dst = getNNtext(len->dst);
    pt = (len->pt == ACK_PKT) ? "ACK" : "DATA";
    switch(len->e){
        case E_SEND:
            et = "S";
            break;
        case E_RECV:
            et = "R";
            break;
        case E_DROP:
            et = "D";
            break;
        case E_TO:
            et = "TO";
            break;
        case E_RE:
            et = "RE";
            break;
    }
    fprintf(fp,"%-11s%-13s%-19s%-13s%-10u%-9s%-9s\n",nn,et,len->ltime,pt,len->seq,src,dst);

}

void addNewLogEntry(logEntryNode len, FILE *fp){
    //nodeName nn, eventType e, long long ts, pktType pt, uint seq, nodeName src, nodeName dst
    if(fp == NULL)
        return;

    char ms[8];
    time_t curr = time(NULL);
    struct tm* timeptr;
    struct timeval tmptime;

    timeptr = localtime(&curr);
    gettimeofday(&tmptime,NULL);
    strftime(len.ltime,20,"%H:%M:%S",timeptr);
    sprintf(ms,".%06ld",tmptime.tv_usec);
    strcat(len.ltime,ms);
    len.ts = ((tmptime.tv_sec)*1e6) + (tmptime.tv_usec);
    fprintf(fp,"%u,%u,%u,%u,%u,%u,%lld,%s\n",len.nn,len.e,len.pt,len.seq,len.src,len.dst,len.ts,len.ltime);
    printLogEntry(&len,stdout);
}

void addEntryToList(logEntryNode **head, logEntryNode len){
    logEntryNode *node = (logEntryNode *)(malloc(sizeof(logEntryNode)));
    *node = len;
    node->next = NULL;
    if(*head == NULL){
        *head = node;
    }
    else{
        if((*head)->ts >= len.ts){
            node->next = *head;
            *head = node;
        }
        else{
            logEntryNode *ptr = *head;
            while(ptr != NULL && (len.ts >= ptr->ts)){
                if(ptr->next != NULL && len.ts <= ptr->next->ts){
                    node->next = ptr->next;
                    ptr->next = node;
                    break;
                }
                else if(ptr->next == NULL){
                    ptr->next = node;
                    break;
                }
                ptr = ptr->next;
            }
        }
    }
}

void addFileToList(logEntryNode **head, FILE *fp){
    if(fp != NULL){
        logEntryNode tmp;
        while((fscanf(fp,"%u,%u,%u,%u,%u,%u,%lld,%s\n",&tmp.nn,&tmp.e,&tmp.pt,&tmp.seq,&tmp.src,&tmp.dst,&tmp.ts,tmp.ltime)) == 8){
            addEntryToList(head,tmp);
        }
        fclose(fp);
    }
}

void prepareSortedLog(){
    logEntryNode *head = NULL;

    FILE *cli = fopen(TMP_CLIENT_LOG,"r");
    FILE *ser = fopen(TMP_SERVER_LOG,"r");
    FILE *rl1 = fopen(TMP_RELAY_1_LOG,"r");
    FILE *rl2 = fopen(TMP_RELAY_2_LOG,"r");

    addFileToList(&head,cli);
    addFileToList(&head,ser);
    addFileToList(&head,rl1);
    addFileToList(&head,rl2);

    remove(TMP_CLIENT_LOG);
    remove(TMP_SERVER_LOG);
    remove(TMP_RELAY_1_LOG);
    remove(TMP_RELAY_2_LOG);

    FILE *logFile = fopen(LOG_FILE_NAME,"w");

    fprintf(logFile,"%-11s%-13s%-19s%-13s%-10s%-9s%-9s\n","Node Name","Event Type","Timestamp","Packet Type","Seq. No.","Source","Dest");

    logEntryNode *ptr = head;
    while(ptr != NULL){
        logEntryNode *tmpNext = ptr->next;
        printLogEntry(ptr,logFile);
        free(ptr);
        ptr = tmpNext;
    }

    fclose(logFile);

    fprintf(stdout,"\n[INFO]: Sorted Log file has been generated and stored in %s.\n",LOG_FILE_NAME);

}

