/*********************************************************
*
* Module Name: UDP Echo server 
*
* File Name:    UDPEchoServer.c	
*
* Summary:
*  This file contains the echo server code
*  The server loops forever, receives a message and echoe's it back to the client.
*
* Parameters:
*    port :   the port to use.  Port 7 is used by default.
*    debugFlag: contains two pieces of information.
*               -To log samples in an output file
*               -The debugLevel which controls how much info is displayed
*
*               To log samples, set the 8th bit of the debugFlag. E.g., 128.
*               The debugLevel is set by anding the debugFlag with 0x7f                
*       
*               By default, this is 129 which sets debugLevel to 1 and creates samples file.
* 
*    0:  serious error messages only
*    1:  adds start msg and final results  and warnings
*    2:  shows iteration results
*    3:  debug information
*
*    Example:  Value 129 sets debugLevel to 1 and creates samples file
* 
*********************************************************/
#include "UDPEcho.h"
#include "sessionManager.h"
#include "messages.h"

void CNTCCode();
void CatchAlarm(int ignored);
void exitProcessing(double curTime);

int sock = -1;                         /* Socket descriptor */
int bStop = 1;;
FILE *newFile = NULL;
double startTime = 0.0;
double lastMessageTime =0.0;

///Use this to have a uniform log name
#define FILE_NAME "ServerLog.txt"

//For all client sessions
unsigned int receivedCount = 0;
long long totalBytesReceived =0;	

//$A3
unsigned int arrivalCount = 0;


//Used to calculate per session updates
unsigned int numberRTTSamples = 0;
unsigned int numberOfSocketsUsed = 0;
unsigned int numberOutOfOrder=0;


long avgPing = 0.0; /* final mean */
long totalPing = 0.0;

double curRTT = 0.0;
double sumOfCurRTT = 0.0;
double meanCurRTT = 0.0; /* final mean */
double smoothedRTT = 0.0;

unsigned int debugLevel = 1;
unsigned int createDataFileFlag = 0;
unsigned int errorCount = 0;  /* each loop iteration, any/all errors increment */

unsigned int debugFlagMask =  0x00000080;

void myUsage(char *myName, int paramCount, char *myVersion)
{
  printf("Usage(V%s,%d): %s [port] [debugFlag]\n", myVersion,paramCount,myName);
}



/*************************************************************
*
* Function: Main program for  UDPEcho  server
*           
*  Parameters:
*      serverPort : port to use 
*      debugFlag =  specifies the level of debug info displayed
*
* outputs:  success or failure
*
* notes: 
*
***************************************************************/
int main(int argc, char *argv[])
{
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int cliAddrLen;         /* Length of incoming message */

    char echoBuffer[ECHOMAX];        /* Buffer for echo string */
    unsigned short echoServPort;     /* Server port */
    int recvMsgSize;                 /* Size of received message */
    char dataFile[] = "EchoServer.dat";
    unsigned int debugFlag = 129;
    unsigned int loopCount = 0;
    unsigned int errorCount = 0;


    unsigned int RxSeqNumber = 0;
    int *RxSeqNumberPtr = (int *)echoBuffer;
//$A2
    unsigned short RxMode = 0;
    unsigned short *RxModePtr = (unsigned short *)&echoBuffer[4];
    session *mySession = NULL;

    /// OPENS NEW FILE AND RUNS TAIL IN NEW TERMINAL

    FILE* file_ptr = fopen(FILE_NAME, "w");
    fclose(file_ptr);
    char header[50];
	strcpy(header, "echo 'SERVER LOG FILE' >>");
	strcat(header, FILE_NAME);
	int er = system(header);
	if(er == -1)
	{
		printf("ERROR CREATING CHAT LOG\n");
		exit(0);
	}

    ///Session list
    session * session_list[MAX_SESSIONS];
    int active_count = 0;

    int rc = 0;
    double curTime = 0.0;

    bStop = 0;
    startTime = timestamp();
    curTime = startTime;

    if (argc < 2)         /* Needs at least the port number */ 
    {
      myUsage(argv[0],argc, Version);
      exit(1);
    }

    echoServPort = atoi(argv[1]);  /* First arg:  local port */
    signal (SIGINT, CNTCCode);

    if (argc == 3) {        /* get debugFlag and set the Level */
      debugFlag = atoi(argv[2]);
      debugLevel = (debugFlag & 0x0000007f);
    }


    if (debugFlag && debugFlagMask)
      createDataFileFlag = 1;
    else 
      createDataFileFlag = 0;


    if (debugLevel > 0) {
      printf("UDPEchoServer(#args:%d): port:%d  debugLevel:%d debugFlag:%d createFlag:%d \n\n",
          argc,echoServPort, debugLevel, debugFlag, createDataFileFlag);
    }

    if (createDataFileFlag == 1 ) {
      newFile = fopen(dataFile, "w");
      if ((newFile ) == NULL) {
        printf("UDPEchoServer(%s): HARD ERROR failed fopen of file %s,  errno:%d \n", 
             argv[0],dataFile,errno);
        exit(1);
      }
    }

    /* Create socket for sending/receiving datagrams */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
//    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDPLITE)) < 0){
      errorCount++;
      printf("UDPEchoServer(%f): HARD ERROR : Failure on socket call errorCount:%d  errno:%d \n", 
              curTime,errorCount,  errno);
      exit(1);
    }

    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */

    rc = bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr));
    /* Bind to the local address */
    if (rc < 0) {
      errorCount++;
      printf("UDPEchoServer(%f): HARD ERROR : bind to port %d error, errorCount:%d  errno:%d \n", 
           curTime,echoServPort,errorCount,  errno);
      exit(1);
    }
  
    while ( bStop != 1) 
    {
      loopCount++;
      curTime = timestamp();
      /* Set the size of the in-out parameter */
      cliAddrLen = sizeof(echoClntAddr);

      /* Block until receive message from a client */
      recvMsgSize = recvfrom(sock, echoBuffer, ECHOMAX, 0, (struct sockaddr *) &echoClntAddr, &cliAddrLen);
      if (recvMsgSize < 0)
      {
        errorCount++;
        printf("UDPEchoServer(%f)(%d) HARD ERROR : error recvfrom  errorCount%d  errno:%d \n", 
              curTime,loopCount, errorCount,  errno);
        if (createDataFileFlag == 1 ) {
            fprintf(newFile,"UDPEchoServer(%f)(%d) HARD ERROR : error recvfrom  errorCount%d  errno:%d \n", 
              curTime,loopCount, errorCount,  errno);
        }
        continue;
      }

      ///code to unpack dataMessage received
	struct DataMsg *tempDataMsg;
	int returnDat = unpackNetworkBufferToDataMsg(&tempDataMsg,(void*) echoBuffer, ECHOMAX);
		if(returnDat <= 0)
			printf("BLANK MESSAGE RECEIVED \n");
	//printf("%s \n",(char*) tempDataMsg -> dataPtr);
	
      //Check if Session already exists
      lastMessageTime =curTime;
      mySession = findSession(echoClntAddr.sin_addr, echoClntAddr.sin_port);

      if(mySession == NULL) // If not added already, create a session
      {
        mySession = createSession(echoClntAddr.sin_addr, echoClntAddr.sin_port);
	if(active_count < MAX_SESSIONS)
	{
		session_list[active_count] = mySession;
		active_count++;
		printf("NEW SESSION ADDED \nACTIVE SESSION COUNT: %d \n", active_count);
		if(active_count == MAX_SESSIONS)
			printf("MAX SESSIONS REACHED");
		
	}
	else
	{
		printf("MAX SESSIONS REACHED--DROPPING NEW SESSION \n");		
		removeSession(mySession);
		mySession = NULL;
	}
      }

   if(mySession != NULL){
     	 //Tracks total number received from all clients
      	receivedCount++;
      	totalBytesReceived += recvMsgSize;

      	RxSeqNumber = (unsigned int)ntohl( (unsigned int)(*RxSeqNumberPtr) );
      	//two cases, arrival is a 'new' segment  
      	if (mySession->largestSeqNumRxed  < RxSeqNumber)
      	  mySession->largestSeqNumRxed = RxSeqNumber;

      	//the arrival is an 'old' segment 
      	if (mySession->largestSeqNumRxed >= RxSeqNumber)
       	 mySession->outOfOrderArrivals++;


      	mySession->lastSeqNumRxed = RxSeqNumber;

      	RxMode = (unsigned short )ntohs( (unsigned short )(*RxModePtr));
      	mySession->mode = RxMode;

      	//Update session info 
      	gettimeofday(&(mySession->lastRxTime), NULL);
      	mySession->messagesReceived++;
      	mySession->bytesReceived += recvMsgSize;

      	mySession->messagesLost = mySession->largestSeqNumRxed - mySession->messagesReceived;

	
      	if (debugLevel > 2) {
       	 printf("UDPEchoServer(%f):(%d,%d),rxed %d bytes, RxSeqNumber:%d Rxmode:%d errorCount:%d #sessions:%d \n",
        	    curTime,loopCount,receivedCount, recvMsgSize, RxSeqNumber, RxMode,errorCount, getNumberSessions());
       	 displaySession(mySession,FALSE);
     	 }

       	 if(RxSeqNumber == mySession->nextSeqNum)
        	{
        	  mySession -> nextSeqNum++;
        	}
        ///send data Msg back as an ack
        struct MsgHdr *myHdr = createMsgHdr(mySession->nextSeqNum, RxMode, DATAMSG, 
           mySession->lastRxTime.tv_sec, mySession->lastRxTime.tv_usec);
        struct DataMsg *myDat = createDataMsg(myHdr, 1, NULL);

        //using echo buffer to pack msg
        int dataCount = packDataMSGToNetworkBuffer(myDat, (void*) echoBuffer, ECHOMAX);
	//printf("sending msg: %s\n", (char*)myDat -> dataPtr);
        rc = sendto(sock, echoBuffer, dataCount, 0,  
           (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr));

	/// clear echoBuffer for second message
	memset(echoBuffer, 0, ECHOMAX);

	///send data out for output
        struct DataMsg *myDat2 = createDataMsg(myHdr, tempDataMsg -> dataSize, tempDataMsg -> dataPtr);
	//printf("max len: %d \n", tempDataMsg -> dataSize);

        //using echo buffer to pack msg
        dataCount = packDataMSGToNetworkBuffer(myDat2, (void*) echoBuffer, ECHOMAX);

	///for sending back to single client
	/*
	rc = sendto(sock, echoBuffer, dataCount, 0,  
           (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr));
	*/

	///SAVE MESSAGE TO LOG
	char dataLog[myDat2 -> dataSize+20];
	strcpy(dataLog, "echo '");
	strcat(dataLog, myDat2 -> dataPtr);
	strcat(dataLog, "' >>");
	strcat(dataLog, FILE_NAME);
	int er = system(dataLog);
		if(er == -1)
		{
		printf("ERROR UPDATING CHAT LOG");
		}
	

	///go through sessionList
	for(int iter = 0; iter < active_count; iter++)
	{
		/////
		//printf("sending data msg: %s\n", (char*)myDat2 -> dataPtr);
		struct sockaddr_in returnAddress;
		returnAddress.sin_family = AF_INET;   
		returnAddress.sin_addr = session_list[iter] -> clientIP;
		returnAddress.sin_port = session_list[iter]->clientPort;
		//printf("address: %s \n", inet_ntoa(session_list[iter] -> clientIP));
		//printf("port: %d \n", htons(session_list[iter] -> clientPort));
       	 	rc = sendto(sock, echoBuffer, dataCount, 0,  
       	    	(struct sockaddr*) &returnAddress, sizeof(returnAddress));

        	///later, need a create datafile flage == 1 section
	}
    }
  }
    //If we get here, we are supposed to end....
    printf("UDPEchoServer:(%f)(%d,%d) WARNING:  Ending Loop  \n", curTime,loopCount,receivedCount);
    //exitProcessing(curTime);
    exit(0);
}


void CNTCCode() {

  bStop = 1;
  exitProcessing(timestamp());
  exit(0);
}


void exitProcessing(double curTime) 
{

  double  duration;
  double totalThroughput=0.0;
//double endTime = curTime;
// duration = endTime - startTime;
  duration = lastMessageTime - startTime;

  if (sock != -1) 
    close(sock);
  sock = -1;

  if (duration > 0.0)
    totalThroughput = ((double )totalBytesReceived)*8 / duration;


  if (debugLevel >0) {
    printf("UDPEcho2Server(%f secs, #sessions:%d) receivedCount:%d, totalBytesRxed:%lld, aggregateThroughput: %9.0f\n",
       duration, getNumberSessions(), receivedCount, totalBytesReceived, totalThroughput);
  }

  displayAllSessions(FALSE);

  if (newFile != NULL) {
    fflush(newFile);
    fclose(newFile);
  }
}





