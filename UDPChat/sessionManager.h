/************************************************************************
* File:  sessionManager.h
*
* Purpose:
*   This include file is for sessionManager includes/defines.
*   Assumes utils.c is compiled/built.
*
* Notes:
*
************************************************************************/
#ifndef	__sessionManager_h
#define	__sessionManager_h

#include "common.h"


#define MAX_SESSIONS 2048
//Possible modes of operation for the sessions 
//The possible tool modes of operation  
#define RTT_MODE 0      //server ack's each arrival (subject to its AckStrategy param)
#define CBR_MODE 1       //server sinks all data, monitors loss and throughput



/*******************************************
•	Client IP :  The client’s IPV4 IP address
•	Client port: The client’s port
•	Creation time:  The time that the session is created 
•	Session mode : A value 0 is echo mode which is what the original UDPEcho program supports. A value of 1 is the new operating mode (CBR mode,  see Part 1.2 of this problem)
•	Time last message received :  Timestamp of last message of the session.
•	Total number of messages received:
•	Total bytes received
•	Most recent sequence number
•	Count of packets out of order
•	Count of packets dropped
*******************************************/
typedef struct session  {
  struct in_addr clientIP;
  unsigned short clientPort;
  unsigned short mode;
  struct timeval creationTime;
  struct timeval lastRxTime;
  long long bytesReceived;
  unsigned int messagesReceived;
  unsigned int messagesLost;
  unsigned int outOfOrderArrivals;
  unsigned int lastSeqNumRxed;
  unsigned int largestSeqNumRxed;

  ///code here
  unsigned int nextSeqNum;

  struct session *prev;
  struct session *next;
} session;


void initSessions();
int getNumberSessions();
session *createSession(struct in_addr clientIP, unsigned short clientPort);
session *findSession(struct in_addr clientIP, unsigned short clientPort);
int displaySession(session *sPtr, int verboseFlag);
int displayAllSessions(int verboseFlag);
int removeSession(session *sPtr);



#endif


