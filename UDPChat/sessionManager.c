/*********************************************************
* Module Name: session Manager 
*
* File Name:  sessionManager.c
*
* Summary:
*  This file contains code to manage a list of UDP IP sessions. 
*
*  The methods include:
*    void initSessions() 
*    int getNumberSessions() 
*    session *createSession(struct in_addr clientIP, unsigned short clientPort) 
*    session *findSession(struct in_addr clientIP, unsigned short clientPort) 
*    int displaySession(struct session *sPtr, int verboseFlag)
*    int displayAllSessions(int verboseFlag)
* 
*  A linked list of session state information is maintained. 
*
*  Notes: 
*        1 socket address/port fields are stored in network byte order.
*         When we display these fields in printfs, we convert to 
*         host byte order.
*
*        2 inet_ntoa expects the supplied address to be in network byte order- 
*          it formats the address and returns  a string in host byte order.
*  
*        3 The list is a implemented with a linked list- new elements placed at the head
*
*********************************************************/
#include "sessionManager.h"

//The head of the list
//Elements are enqueued at the head....so the 'oldest' session is the last in the list
session *firstSession = NULL;
unsigned int sessionCount = 0;

//Uncomment to turn on printf debug statements
//#define  TRACEME 0
double convertTimeval(struct timeval *t);
double timestamp();


/***********************************************************
* Function: session *findSession(struct in_addr clientIP, unsigned short clientPort) 
*
* Explanation:  This searches the active list of sessions for the client IP/port.
*               It returns the session handle if found, else a NULL;
*
* inputs: passed the IP and port of the client 
*
* outputs:
*    Returns an address of the session struct entry.
*    Or a NULL on error.
*
* notes: 
*
***********************************************************/
session *findSession(struct in_addr clientIP, unsigned short clientPort) {
  session *sPtr = firstSession;
  while (sPtr != NULL) {
    if (sPtr->clientIP.s_addr == clientIP.s_addr && sPtr->clientPort == clientPort)
      break;
    sPtr = sPtr->next;
  }
  return sPtr;
}

/***********************************************************
* Function: session *removeSession(session *sPtr )
*
* Explanation:  This removes the session from the list
*               It also frees the memory associated with the 
*                session list element.
*
* inputs: passed a pointer to the session list element to be removed. 
*
* outputs:
*    Returns SUCCESS or ERROR 
*
* notes: 
*
***********************************************************/
int removeSession(session *sPtr) {

int rc = ERROR;

  if (sPtr != NULL) {

    // ERROR case
    if (sessionCount == 0) { 
      printf("removeSession:  HARD ERROR  session count 0 ??\n ");
    } 
    else if  (sessionCount == 1) { 
      firstSession = NULL;
      rc = SUCCESS;
    } else 
    {
      rc = SUCCESS;
      //if first element
      if (sPtr == firstSession) {
        firstSession = sPtr->next;
        sPtr->next->prev = sPtr->next;
      }
      //if middle or last element
      else {
        sPtr->prev->next = sPtr->next;
        if (sPtr->next->prev != NULL)
          sPtr->next->prev = sPtr->prev;
      }
    }
  }
  if (rc == SUCCESS) {
    sessionCount--;
    free(sPtr);
  }
  return rc;
}

/***********************************************************
* Function: session *createSession(struct in_addr clientIP, unsigned short clientPort) 
*
* Explanation:  This searches the active list of known sessions for the client IP/port.
*               It returns the session handle if found, else creates
*               a new session and returns this handle.
*
* inputs: passed the IP and port of the client in Network Byte Order
*
* outputs:
*    Returns an address of the session struct entry.
*    Or a NULL on error.
*
* notes: 
*
***********************************************************/
session *createSession(struct in_addr clientIP, unsigned short clientPort) {

  session *sPtr = NULL;

#ifdef TRACEME 
   printf("createSession: Entered(current#:%d): IP:%s port:%d   \n",
        sessionCount,inet_ntoa(clientIP), ntohs(clientPort));
#endif 

  // First, chk to see if we have the session....if so return error. 
  session *s = findSession(clientIP, clientPort);

  // If matching session is not found malloc a new
  //  but only if < MAX_SESSIONS  
  if (s == NULL) {
    if (sessionCount < MAX_SESSIONS) {
#ifdef TRACEME 
      printf("createSession: Malloc new session- size %d ....  \n", (int)sizeof(session));
#endif 
      // ...add new session
      s = malloc(1 * sizeof(session));
      s->clientIP = clientIP;
      s->clientPort = clientPort;
      s->mode = RTT_MODE;  //by default assume the RTT (or ECHO) mode
      gettimeofday(&(s->creationTime), NULL);
      s->lastRxTime = s->creationTime;
      s->lastSeqNumRxed = 0;
      s->largestSeqNumRxed = 0;
      s->bytesReceived = 0;
      s->messagesReceived = 0;
      s->messagesLost = 0;
      ///begin sequence at 1
      s->nextSeqNum = 1;
      s->next = firstSession;
      s->prev = NULL;
      firstSession = s;
      sessionCount++;
      sPtr=s;
    } else 
      printf("createSession:  WARNING:   Hit MAX_SESSIONS : %d \n ",sessionCount);
  } else {
#ifdef TRACEME 
    printf("createSession: WARNING Session exists ??   (currentCount:%d): IP:%s port:%d   \n",
        sessionCount,inet_ntoa(clientIP), ntohs(clientPort));
#endif 
  }
  return sPtr;
}


/***********************************************************
* Function: void initSessions() 
*
* Explanation:  This inits the sessionManager. If there 
*               are sessions in the list, they are freed.
*
* inputs: none
*
* outputs:
*
* notes: 
*
***********************************************************/
void initSessions() {
  while (firstSession != NULL) {
    sessionCount--;
    session *tofree = firstSession;
    firstSession = tofree->next;
    free(tofree);
  }
  sessionCount = 0;
}

/***********************************************************
* Function: int getNumberSessions() 
*
* Explanation:  This returns the number of sessions in the list.
*
* inputs: none
*
* outputs:
*         returns the number of sessions.
*
* notes: 
*
***********************************************************/
int getNumberSessions() {

  return sessionCount;
}

/***********************************************************
* Function: int displaySession(struct sessoin *sPtr, int verboseFlag)
*
* Explanation:  This displays session info to standard out.
*
* inputs: 
*     struct session *sPtr :   ptr to the session data
*     int verboseFlag:  If TRUE (1), lots of info displayed, 
*      else if FALSE  a terse summary is displayed
*
* outputs:
*    returns SUCCESS or ERROR 
*
* notes: 
*
***********************************************************/
int displaySession(struct session *sPtr, int verboseFlag)  {

  int rc = SUCCESS;
  double duration = 0.0;
  double throughput = 0;
  double avgLoss = 0.0;

#ifdef TRACEME 
  printf("displaySession: Entered: current list size:%d, verboseFlag:%d  \n", sessionCount,verboseFlag);
#endif 

  if (sPtr != NULL) {

    duration = convertTimeval((struct timeval *)&(sPtr->lastRxTime)) - convertTimeval((struct timeval *)&sPtr->creationTime);

    if (duration > 0)
      throughput =    (((double)sPtr->bytesReceived) * 8) / duration;

    if (sPtr->largestSeqNumRxed != 0)
      avgLoss = ((double)sPtr->messagesLost*100)/sPtr->largestSeqNumRxed;


    if (verboseFlag == TRUE) {

      printf("clientIP port duration mode totalMsgsRxed totalBytesRxed Throughput avgLoss totalMsgsLost largestSeqNumRxed lastSeqNumberRxed \n"); 
      printf("%s %d %f %d %d %lld %d %2.4f %d %d %d\n",
          inet_ntoa(sPtr->clientIP),ntohs(sPtr->clientPort), 
          duration, sPtr->mode, sPtr->messagesReceived, sPtr->bytesReceived, (unsigned int) throughput, avgLoss,
          sPtr->messagesLost, sPtr->lastSeqNumRxed, sPtr->largestSeqNumRxed);

    } else {
      printf("%s %d %f %d %d %lld %d %2.4f %d %d %d\n",
          inet_ntoa(sPtr->clientIP),ntohs(sPtr->clientPort), 
          duration, sPtr->mode, sPtr->messagesReceived, sPtr->bytesReceived, (unsigned int) throughput, avgLoss,
          sPtr->messagesLost, sPtr->lastSeqNumRxed, sPtr->largestSeqNumRxed);
    }
  }
  else
    rc = ERROR;

  return rc;
}


/***********************************************************
* Function: int displayAllSessions(struct sessoin *sPtr, int verboseFlag)
*
* Explanation:  This displays ALL sessions info to standard out.
*
* inputs: 
*     int verboseFlag:  If TRUE (1), lots of info displayed, 
*      else if FALSE  a terse summary is displayed
*
* outputs:
*    returns the number of Sessions displayed 
*
* notes: 
*
***********************************************************/
int displayAllSessions(int verboseFlag) 
{
int rc = SUCCESS;
unsigned int count=0;

  session *sPtr = firstSession;
  while (sPtr != NULL) {
    rc =  displaySession(sPtr,verboseFlag);
    if (rc == SUCCESS)
      sessionCount++;
    sPtr = sPtr->next;
  }
  return count;
}




