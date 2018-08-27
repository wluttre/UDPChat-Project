/*********************************************************
* Group: Harrison Diesl, William Luttrell, Jawaun McKelvey
* 
* Module Name: UDP Echo client source 
*
* File Name:    UDPEchoClient2.c
*
* Summary:
*  This file contains the client portion of a client/server
*         UDP-based performance tool.
*  
* Usage :   ./ client  
*             <Server IP> 
*             <Server Port>   
*             [<User Name>]\n",
*********************************************************/
#include "UDPEcho.h"
#include "messages.h"
#include "stdlib.h"
#include "stdio.h"
#include "pthread.h"

void clientCNTCCode();
void CatchAlarm(int ignored);
void exitProcessing(double curTime);
void * outputScreen();

///Use this to have a uniform chat log name
#define FILE_NAME "Log.txt"


//Define this globally so our async handlers can access

char *serverIP = NULL;                   /* IP address of server */
int sock = -1;                         /* Socket descriptor */
int bStop;
int cStop;
FILE *newFile = NULL;
double startTime = 0.0;
double endTime = 0.0;

unsigned int numberOfTimeOuts=0;
unsigned int numberOfTrials=0; /*counts number of attempts */
unsigned int numberOfSocketsUsed = 0;
unsigned int numberLost=0;
unsigned int numberOutOfOrder=0;
double avgLoss = 0.0;
long long totalBytesSent =0;	

///Variables for tracking order
unsigned int nextSeqNum = 1;

//RTT as a long
long avgPing = 0.0; /* final mean */
long totalPing = 0.0;
unsigned int numberRTTSamples = 0;

//RTT as a double
double curRTT = 0.0;
double sumOfCurRTT = 0.0;
double meanCurRTT = 0.0; /* final mean */
double smoothedRTT = 0.0;

   ////dropcount
   int repeatedDropCount = 0;
   ////drop detect boolean
   int dropDetected = 0;

//Maintains the next seq number to use
  unsigned int seqNumber = 1;
  unsigned int *seqNumberPtr;

struct sockaddr_in serverAddress; /* Echo server address */

char echoBuffer[ECHOMAX+1];      /* Buffer for receiving echoed string */
char echoBuffer2[ECHOMAX+1];

///initialize child process
pthread_t child;

unsigned int errorCount = 0;  /* each loop iteration, any/all errors increment */
unsigned int  outOfOrderArrivals = 0;


void myUsage(char *myName, int paramCount, char *myVersion)
{

  printf("Usage: %s V%s): [Server IP] [Server Port] [Display Name]  \n",  
         myName,myVersion);

}


/*************************************************************
*
* Function: Main program for  UDPEcho client
*           
* inputs: 
*  char *  serverIP = argv[1]; 
*  unsigned int  serverPort = atoi(argv[2]);
*  string uName = argv[3];
*  Usage :   client  
*             <Server IP> 
*             <Server Port> 
*	      [<User Name>]\n", 
***************************************************************/
int main(int argc, char *argv[])
{
  
  unsigned short serverPort;        /* Echo server port */

  char *echoString;                /* String to send to echo server */
  
  int echoStringLen;               /* Length of string to echo */
  struct hostent *thehost;	     /* Hostent from gethostbyname() */

  ///set to largest number to allow messages of differing size
  int messageSize = ECHOMAX;                  /* PacketSize*/

  double curTime = 0.0;

  /// initialize username
  char *uName = "DEFAULT:";


  //Used to compute the RTT
  struct timeval theTime1;

  //Needed for the alarm timer
  struct sigaction myaction;

  int rc  = SUCCESS;

  //tmp variables used for various calculations...
  int i = 0;


  //Initialize values
  numberOfTimeOuts = 0;
  numberOfTrials = 0;
  totalPing =0;
  bStop = 0;
  cStop = 0;

  curTime = timestamp();
//  startTime = timestamp();
//  curTime = startTime;

/// OPENS NEW FILE AND RUNS TAIL IN NEW TERMINAL

  FILE* file_ptr = fopen(FILE_NAME, "w");
  fclose(file_ptr);

  int er = system("gnome-terminal -e 'tail -qf Log.txt'");
	if(er == -1)
	{
		printf("ERROR OPENING CHAT LOG: EXITING");
		exit(0);
	}
	char header[50];
	strcpy(header, "echo 'CHAT BOARD--DO NOT TYPE HERE' >>");
	strcat(header, FILE_NAME);
	er = system(header);
	if(er == -1)
	{
		printf("ERROR PROVIDING CHAT LOG HEADER \n");
		exit(0);
	}
	printf("PLEASE ENTER MESSAGES HERE: \n");
	
  if (argc != 4)    
  {
    myUsage(argv[0],argc, Version);
    exit(1);
  }

  signal (SIGINT, clientCNTCCode);

  serverIP = argv[1];           /* First arg: server IP address (dotted quad) */
  serverPort = atoi(argv[2]);   /* Second arg: server port */
  ///last arg: username
  uName = argv[3];

  myaction.sa_handler = CatchAlarm;
  if (sigfillset(&myaction.sa_mask) < 0){
    errorCount++;
    printf("UDPEchoClient(%s): HARD ERROR sigfillset failed:  errno:%d \n", 
             argv[0],errno);
    exit(1);
  }

  myaction.sa_flags = 0;

  if (sigaction(SIGALRM, &myaction, 0) < 0)  {
    errorCount++;
    printf("UDPEchoClient(%s): HARD ERROR sigaction failed:  errno:%d \n", 
             argv[0],errno);
    exit(1);
  }

  /* Set up the echo string */

  echoStringLen = messageSize;
  echoString = (char *) echoBuffer;

  for (i=0; i<messageSize; i++) {
     echoString[i] = 0;
  }

//$A3
//These will point to the Msg to be sent
  seqNumberPtr = (unsigned int *)echoString;
//These will point to the Msg that is received
  echoString[messageSize-1]='\0';

  /* Construct the server address structure */
  memset(&serverAddress, 0, sizeof(serverAddress));    /* Zero out structure */
  serverAddress.sin_family = AF_INET;                 /* Internet addr family */
  serverAddress.sin_addr.s_addr = (in_addr_t)inet_addr(serverIP);  /* Server IP address */
    
    /* If user gave a dotted decimal address, we need to resolve it  */
  if (serverAddress.sin_addr.s_addr == -1) {
      thehost = gethostbyname(serverIP);
      serverAddress.sin_addr.s_addr = *((unsigned long *) thehost->h_addr_list[0]);
  }
    
  serverAddress.sin_port   = htons(serverPort);     /* Server port */
  /* Create a datagram/UDP socket */
  if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
//  if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDPLITE)) < 0) {
      errorCount++;
      DieWithError("socket() failed");
  } else 
      numberOfSocketsUsed++;

   ///code here
    long long intervalBytes = 0;


    /// spawn child thread to handle output

    pthread_create(&child, NULL, outputScreen, NULL); 

while ((bStop != 1)) {
    ////if for dropping it all
    if(repeatedDropCount > 5)
    {
      printf("\nERROR: REPEATED DROP LIMIT REACHED. CONNECTION CLOSED.\n");
      exit(0);
    }
    ////calculate time and expected seqNum
    gettimeofday(&theTime1, NULL);
    curTime = convertTimeval(&theTime1);
    if (startTime == 0.0 ) 
    {
        startTime = curTime;
    }
    ///code for getting input at terminal
    char input[messageSize];
    char temp[messageSize];
    memset(echoString, 0, echoStringLen);
    void* check = fgets(input, messageSize-1, stdin);
    if (check != NULL){
	strcat(temp, uName);
	strcat(temp, ": ");
	strcat(temp, input);
	echoString = temp;
	echoStringLen = strlen(echoString);
	/*
	printf("strlen = %d \n", echoStringLen);
	printf("%s \n", echoString);
	*/
    ///code for sending ack
	struct MsgHdr *myHdr = createMsgHdr(seqNumber, 2, DATAMSG, 
           theTime1.tv_sec, theTime1.tv_usec);
        struct DataMsg *myDat = createDataMsg(myHdr, echoStringLen, echoString);

        //using echo buffer to pack data msg
        int dataCount = packDataMSGToNetworkBuffer(myDat, (void*) echoBuffer, ECHOMAX);

        rc = sendto(sock, echoBuffer, dataCount, 0,  
           (struct sockaddr *) &serverAddress, sizeof(serverAddress));
	////
	dropDetected = 0;
	if (rc >= 0) 
        totalBytesSent += (long long)rc;
	intervalBytes += (long long)rc;
	numberOfTrials++;
	seqNumber ++;
	rc = -1;
    ///code for receiving
	alarm(TIMEOUT);
	
    }
}
  exitProcessing(curTime);
  exit(0);
}

void * outputScreen()
{
    while ((cStop != 1)){
	///code for receiving
	int returnMSG = -1;
	//printf("Child2 running\n");
	memset(echoBuffer2, 0, ECHOMAX);
	returnMSG = recvfrom(sock, echoBuffer2, ECHOMAX, 0, (struct sockaddr *) &serverAddress, (socklen_t *) sizeof(serverAddress));
	struct DataMsg *tempDat;
	returnMSG = unpackNetworkBufferToDataMsg(&tempDat, (void*) echoBuffer2, ECHOMAX);
	/*
	printf("received msg: '%s'\n", (char*) tempDat -> dataPtr);
	printf("datasize: '%d'\n", tempDat -> dataSize);
	
	printf("repeatedDropCount: %d \n", repeatedDropCount);
	*/
	if(returnMSG == -1){
			alarm(0);
			seqNumber = tempDat -> myHdr ->sequenceNum;
			////
            	    if(dropDetected == 0)
           	     {
           	       repeatedDropCount++;
           	       dropDetected = 1;
          	      }
	}
	else if(tempDat -> dataSize == 1){
		alarm(0);
		//printf("received ack \n");
		if(tempDat -> myHdr -> sequenceNum == nextSeqNum+1){
			nextSeqNum++;
			////
          	     repeatedDropCount = 0;
		}
		else if(tempDat -> myHdr -> sequenceNum != nextSeqNum+1){
			seqNumber = tempDat -> myHdr ->sequenceNum;
			////
            	    if(dropDetected == 0)
            	    {
            	      repeatedDropCount++;
            	      dropDetected = 1;
           	     }
		}
	}
	else{
	/// CODE TO PRINT TO OUTPUT FILE
	
	char command[tempDat -> dataSize+20];
	strcpy(command, "echo '");
	strcat(command, tempDat -> dataPtr);
	strcat(command, "' >>");
	strcat(command, FILE_NAME);
	int er = system(command);
		if(er == -1)
		{
		printf("ERROR UPDATING CHAT LOG: EXITING");
		exit(0);
		}
	}
    }
    return 0;
}

void CatchAlarm(int ignored) 
{ 
            	    if(dropDetected == 0)
           	     {
           	       repeatedDropCount++;
			numberOfTimeOuts++;
           	       dropDetected = 1;
          	      }
    printf("MESSAGE LOST, PLEASE RESEND:  #timeouts:%d \n", numberOfTimeOuts);
	

}

void clientCNTCCode() {

  bStop = 1;
  cStop = 1;
    printf("UDPEchoClient2:CNTC: #iterations:%d,  #TIMEOUTS:%d \n", 
           numberOfTrials,numberOfTimeOuts);
  exitProcessing(timestamp());
  exit(0);
}


void exitProcessing(double curTime) {

  pthread_cancel(child);

  char endLog[100];
	strcpy(endLog, "echo 'CONNECTION CLOSED: PLEASE ISSUE A CNT-C' >>");
	strcat(endLog, FILE_NAME);
	int er = system(endLog);
		if(er == -1)
		{
		printf("ERROR DISPLAYING EXIT MESSAGE: PLEASE CLOSE OUTPUT WINDOW");
		}

  char ExitCommand[20];
	strcpy(ExitCommand, "rm ");
	strcat(ExitCommand, FILE_NAME);
	er = system(ExitCommand);
		if(er == -1)
		{
		printf("ERROR CLEARING CHAT LOG: PLEASE DELETE %s", FILE_NAME);
		}
  endTime = curTime;
  double duration = endTime - startTime;
  double actualSendRate = 0;


  if (sock != -1) 
    close(sock);

  sock = -1;
  numberLost = numberOfTrials - numberRTTSamples;

  if (duration > 0.0)
    actualSendRate = ((double )totalBytesSent)*8 / duration;

  if (numberRTTSamples != 0) {
    avgPing = (totalPing/numberRTTSamples);
    meanCurRTT = (sumOfCurRTT/numberRTTSamples);
  }
  if (numberOfTrials != 0) 
    avgLoss = (((double)numberOfTimeOuts*100)/numberOfTrials);

 
        printf("UDPEcho2Client:(%f):SentMsgs:%d TotalBytes:%lld actualSendRate:%9.0f \n",
             duration,numberOfTrials, totalBytesSent, actualSendRate);
}



