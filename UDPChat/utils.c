/*********************************************************
* Module Name:  Tools common routines 
*
* File Name:    utils..c
*
*
*********************************************************/
#include "common.h"
#include <stdio.h>  /* for perror() */
#include <stdlib.h> /* for exit() */
#include "portable_endian.h"

char Version[] = "1.6";   


unsigned int getSeed() {
  struct timeval curTime;
  (void) gettimeofday (&curTime, (struct timezone *) NULL);
  return curTime.tv_usec;
}


void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}


void die(const char *msg) {
  if (errno == 0) {
    /* Just the specified message--no error code */
    puts(msg);
  } else {
    /* Message WITH error code/name */
    perror(msg);
  }
  printf("Die message: %s \n", msg);
  
  /* DIE */
  exit(EXIT_FAILURE);
}

long getMicroseconds(struct timeval *t) {
  return (t->tv_sec) * 1000000 + (t->tv_usec);
}

//Returns timeval in seconds with usecond precision
double convertTimeval(struct timeval *t) {
  return ( t->tv_sec + ( (double) t->tv_usec)/1000000 );
}

//Returns time difference in microseconds 
long getTimeSpan(struct timeval *start_time, struct timeval *end_time) {
  long usec2 = getMicroseconds(end_time);
  long usec1 = getMicroseconds(start_time);
  return (usec2 - usec1);
}

double timestamp() 
{
  struct timeval tv;
  if (gettimeofday(&tv, NULL) < 0) { 
    die("the sky is falling!"); 
  }
  return ((double)tv.tv_sec + ((double)tv.tv_usec / 1000000));
}

void setUnblockOption(int sock, char unblock) {
  int opts = fcntl(sock, F_GETFL);
  if (unblock == 1)
    opts |= O_NONBLOCK;
  else
    opts &= ~O_NONBLOCK;
  fcntl(sock, F_SETFL, opts);
}

void sockBlockingOn(int sock) { setUnblockOption(sock, 0); }
void sockBlockingOff(int sock) { setUnblockOption(sock, 1); }


/*************************************************************
* Function: int is_bigendian()
* 
* Summary: returns TRUE if big endian else FALSE 
*
* Inputs:
* outputs:  
*   returns TRUE or FALSE 
* notes: 
*************************************************************/
int is_bigendian()
{
  int rc = FALSE;
unsigned int x = 0x12345678;
//  uint32_t x = 0x12345678;
  char *ptr = (char *) &x;

  //Test if this machine is big or little endian.
  if (ptr[0] == 0x01) {
    rc = TRUE;
  } else if (ptr[0] == 0x78) {
    rc = FALSE;
  } else {
    printf("is_bigendian: HARD ERROR ??   %0xX ??\n",ptr[0]);
  }
  return rc;
}


/*************************************************************
* Function: 
*   int packBufferWithInt(char *bufPtr, unsigned int intValue)
* 
* Summary: copies the intValue to the buffer in network byte order
*          Returns SUCCESS or ERROR 
*
* Inputs:
*   char *bufPtr
*   unsigned int intValue
*
* outputs:  
*   returns TRUE or FALSE 
* notes: 
*************************************************************/
int packBufferWithInt(char *bufPtr, unsigned int intValue)
{
  int rc = SUCCESS;
  unsigned int mySize = sizeof(intValue);
  unsigned int *myIntPtr = (unsigned int *)bufPtr;
  unsigned char *myCharPtrSrc = (unsigned char *)&mySize;
  unsigned char *myCharPtrDst = (unsigned char *)bufPtr;


  printf("packBufferWithInt: value: %d  size:%d \n",intValue, mySize);

  if(!is_bigendian()){
    //If this host is not bigendian 
    //swap octets
    myCharPtrDst[0]=myCharPtrSrc[3];
    myCharPtrDst[1]=myCharPtrSrc[2];
    myCharPtrDst[2]=myCharPtrSrc[1];
    myCharPtrDst[3]=myCharPtrSrc[0];
    
    printf("packBufferWithInt: Host little endian intValue:%0xX   packed:%0xX\n",intValue, *myIntPtr);
  } else {
    //Else this host is bigendian
    *myIntPtr = intValue;  
    printf("packBufferWithInt: Host big endian intValue:%d  packed:%d\n",intValue, *myIntPtr);
  }

  return rc;
}

/*************************************************************
*
* Function: void swapbytes(void *_object, size_t size)
* 
* Summary: In-place swapping of bytes to match endianness of hardware
*
* Inputs:
*   *object : memory to swap in-place
*   size   : length in bytes
*           
*
* outputs:  
*     updates caller's object data
*
* notes: 
*    
*   Timeval struct defines the two components as long ints
*         The following nicely printers: 
*         printf("%ld.%06ld\n", usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
*
***************************************************************/
void swapbytes(void *_object, size_t size)
{
  unsigned char *start, *end;
//TODO:  replace with preprocessor macro
  if(!is_bigendian())
  {
    for ( start = (unsigned char *)_object, end = start + size - 1; start < end; ++start, --end )
    {
      unsigned char swap = *start;
      *start = *end;
      *end = swap;
    }
   }
}

/*************************************************************
*
* Function: uint64_t htonll (uint64_t InAddr) 
* 
* Summary:  equivalent to htonl but operates on long long which
*           is assumed to be uint64_t 
*
* Inputs:
*   uint64_t InAddr :  callers 64 bit data
*           
*
* outputs:  
*   returns the InAddr in network byte (Big Endian) format
*
* notes: Returns all 1's on error
*    
***************************************************************/
#if defined(__linux__)

uint64_t htonll(uint64_t InAddr) 
{
  uint64_t rvalue = InAddr;
//  swapbytes((void *)(&rvalue),size(uint64_t));

//TODO:  replace with preprocessor macro
  if(!is_bigendian())
  {
    rvalue = htobe64(rvalue);
  }
  return rvalue;
}
#endif

#if defined(__APPLE__)

double getTime(int clockType) {
  return getTime1();
}

#elif defined(__linux__)

double getTime(int clockType) {
return getTime1();
  clockid_t clockLinuxType = CLOCK_REALTIME;
  struct timeval myTime;
  struct timespec curTime;
  int rc;


  switch(clockType) {
    case 1:
	rc= gettimeofday (&myTime, (struct timezone *) NULL);
        if (rc==0) 
	  return (((((double) myTime.tv_sec) * 1000000.0) 
             + (double) myTime.tv_usec) / 1000000.0); 
        else{
         printf("getTime: Error on gettimeofday:%d, errno:%d (clockType:%d) \n",
           rc,errno,clockType);
          return(0.0);
        }

    break;

    case 2:
      clockLinuxType = CLOCK_REALTIME;
      //Use clock_gettime
      rc =clock_gettime(clockLinuxType, &curTime);
      if (rc==0) { 
        //return (1000000000 * (double)curTime.tv_sec + (double)curTime.tv_nsec);
        return ((double)curTime.tv_sec + ((double)curTime.tv_nsec)/1000000000);
      }
      else{
        printf("getTime: Error on clock_gettime:%d, errno:%d (clockType:%d) \n",
          rc,errno,clockType);
        return(0.0);
      }
    break;

    case 3:
      clockLinuxType = CLOCK_PROCESS_CPUTIME_ID;
      //Use clock_gettime
      rc =clock_gettime(clockLinuxType, &curTime);
      if (rc==0) { 
        //return (1000000000 * (double)curTime.tv_sec + (double)curTime.tv_nsec);
        return ((double)curTime.tv_sec + ((double)curTime.tv_nsec)/1000000000);
      }
      else{
        printf("getTime: Error on clock_gettime:%d, errno:%d (clockType:%d) \n",
          rc,errno,clockType);
        return(0.0);
      }
    break;

    default:
      printf("Error on clockType: %d \n",clockType);
      return(0.0);
  }
}
#endif 

double getTime1()
{
	struct timeval curTime;
	(void) gettimeofday (&curTime, (struct timezone *) NULL);
	return (((((double) curTime.tv_sec) * 1000000.0) 
             + (double) curTime.tv_usec) / 1000000.0); 
}

#if defined(__APPLE__)

int gettimeofday_benchmark()
{
  return 0;
}

#elif defined(__linux__)

int gettimeofday_benchmark()
{
  int i;
  struct timespec tv_start, tv_end;
  struct timeval tv_tmp;
  int count = 1 * 1000 * 1000 * 50;
  clockid_t clockid;
        
  int rv = clock_getcpuclockid(0, &clockid);

  if (rv) {
    perror("clock_getcpuclockid");
    return 1;
  }

  clock_gettime(clockid, &tv_start);

  for(i = 0; i < count; i++)
      gettimeofday(&tv_tmp, NULL);

  clock_gettime(clockid, &tv_end);

  long long diff = (long long)(tv_end.tv_sec - tv_start.tv_sec)*(1*1000*1000*1000);
  diff += (tv_end.tv_nsec - tv_start.tv_nsec);

  printf("%d cycles in %lld ns = %f ns/cycle\n", count, diff, (double)diff / (double)count);
  return 0;
}
#endif

int
delay(int64_t ns)
{
    struct timespec req, rem;

    req.tv_sec = 0;

    while (ns >= 1000000000L) {
        ns -= 1000000000L;
        req.tv_sec += 1;
    }

    req.tv_nsec = ns;

    while (nanosleep(&req, &rem) == -1)
        if (EINTR == errno)
            memcpy(&req, &rem, sizeof(rem));
        else
            return -1;
    return 0;
}


/*************************************************************
* Function: int myDelay()
int myDelay(double delayTime)
* 
* Summary: Delays the specified amount of time 
* Inputs:   double delayTime:  the time to delay in seconds
*
* outputs:  
*   returns  ERROR or SUCCESS
*
* Set CHECK_SLEEP_TIME to measure actual time
*
*************************************************************/
int myDelay(double delayTime)
{
  int rc = SUCCESS;

  int64_t ns = (int64_t) (delayTime * 1000000000.0); 
//  printf("myDelay:  delayTime:%f   %ld \n",delayTime, ns);

  rc = delay(ns);
  return rc;
}


