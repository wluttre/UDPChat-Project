/************************************************************************
* File:  common.h
*
* Purpose:
*   This include file is for common includes/defines.
*   Assumes utils.c is compiled/built.
*
* Notes:
*
************************************************************************/
#ifndef	__common_h
#define	__common_h

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>  /*brings in C99 types of uint32_t uint64_t  ...*/
#include <limits.h>  /*brings in limits such as LONG_MIN LLONG_MAX ... */
#include <math.h>    /* floor, ... */

#include <string.h>     
#include <errno.h>


#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <netinet/in.h> /* for in_addr */
#include <arpa/inet.h> /* for inet_addr ... */

#include <unistd.h>     /* for close() */
#include <fcntl.h>
#include <netdb.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>



#if defined(__linux__) 

uint64_t htonll (uint64_t InAddr) ;
#elif defined(__APPLE__)
//htonll provided
#endif

int is_bigendian();
//void swapbytes(void *_object, size_t size);



#define MESSAGEMIN 24
#define MESSAGEMAX 65535

#ifndef LINUX
#define INADDR_NONE 0xffffffff
#endif


//Definition, FALSE is 0,  TRUE is anything other
#define TRUE 1
#define FALSE 0

#define VALID 1
#define NOTVALID 0

//Defines max size temp buffer that any object might create
#define MAX_TMP_BUFFER 1024

/*
  Consistent with C++, use EXIT_SUCCESS, SUCCESS is a 0, otherwise  EXIT_FAILURE  is not 0

  If the method returns a valid rc, EXIT_FAILURE is <0
   Should use bool when the method has a context appropriate for returning T/F.   
*/
#define SUCCESS 0
#define NOERROR 0

#define ERROR   -1
#define FAILURE -1
#define FAILED -1

//We assume a NULL can be interpretted as an error

unsigned int getSeed();
int packBufferWithInt(char *bufPtr, unsigned int intValue);
void die(const char *msg);
void DieWithError(char *errorMessage); /* External error handling function */
double timestamp();
extern char Version[];

double getTime(int);
double getTime1();

int delay(int64_t ns);
int myDelay(double delayTime);


int gettimeofday_benchmark();

long getMicroseconds(struct timeval *t);
double convertTimeval(struct timeval *t);

long getTimeSpan(struct timeval *start_time, struct timeval *end_time);
void setUnblockOption(int sock, char unblock);
void sockBlockingOn(int sock);
void sockBlockingOff(int sock);

#endif


