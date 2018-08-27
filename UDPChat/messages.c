/*********************************************************
* Module Name:  messages 
*
* File Name:  messages.c
*
* Summary:
*
*   This module contains a "C" interface to routines that
*     allow a calling program to create, delete, and manage
*     messages that an application might need to send to 
*     other applications.
*
*   The basic design from a caller's perspective that is 
*     sending the msg (note the params are not exact):
*      myNetBufPtr = malloc( netBufSize )
*      myMsgPtr = createMsg( .. specific params)
*      numberBytes =  packMsgToNetBuffer (myMsgPtr,myNetbufPtr,netBufSize ) 
*      sendto(socket, myNetbufPtr, numberBytes, .... )
*
*    At the receiving side
*       myNetBufPtr = malloc (netbufSize)
*       numberBytesRxed=recvfrom(socket,myNetBufPtr, netBufSize, ...)
*       myMsgPtr = unpackNetBufferToMsg(myNetBufPtr, numberBytesRxed ..)
*
*
*  Notes:
*       -The Msg structure is the program's internal representation
*        of the message.  It contains a field called msgSize that
*        is the number of octects of this representation
*       -The pack routine copies/translates the internal Msg to a format
*        appropriate for transmission.  The actual number of bytes in this
*        network representation of the msg is returned on the pack call.
*********************************************************/
#include "messages.h"

#include <strings.h>


//Uncomment to turn on printf debug statements
//#define  TRACEME 0



/***********************************************************
* Function: struct MsgHdr *createMsgHdr( uint32_t sequenceNum, uint16_t mode, 
*                             uint32_t timeSentSeconds, uint32_t timeSentUSeconds)
*
* Explanation:  This creates a  msg hdr. 
*
* inputs: 
*       uint32_t sequenceNum : Sequence number to place in the header
*       uint16_t mode : program test mode 
*       int32_t timeSentSeconds : timeval seconds
*       uint32_t timeSentUSeconds: timeval usecs
*
* outputs:
*    Returns a handle to the structure that has been created. 
*    Returns a NULL on error. 
*
* notes: 
*    -The caller owns the memory.
*
***********************************************************/
struct MsgHdr *createMsgHdr( uint32_t sequenceNum, uint16_t mode, uint16_t msgType, 
                             uint32_t timeSentSeconds, uint32_t timeSentUSeconds)
{
  struct MsgHdr *myHdr = malloc(sizeof(MsgHdrType));

  if (myHdr != NULL) {
    bzero(myHdr,sizeof(MsgHdrType));
    myHdr->sequenceNum = sequenceNum;
    myHdr->mode = mode;
    myHdr->msgType = msgType;
    myHdr->timeSentSeconds=timeSentSeconds;
    myHdr->timeSentUSeconds=timeSentUSeconds;
#ifdef TRACEME
    printf("createMsgHdr: Succeeded to create header  sizeof:%d, sequenceNumber:%d \n",
                 (int)sizeof(MsgHdrType),myHdr->sequenceNum);
#endif
  }
#ifdef TRACEME
  else 
    printf("createMsgHdr: Failed to malloc Hdr struct \n");
#endif

  return myHdr; 
}

/***********************************************************
* Function: struct DataMsg *createDataMsg(struct MsgHdr *myHdr, uint32_t dataSize, void **myData) 
*
* Explanation:  This creates a Data msg.
*
* inputs: 
*     MsgHdr *myHdr : Caller's reference to a valid MsgHdr
*                     Ownership transfers to this message.
*     uint32_t dataSize : size of data in the buffer
*     void **myData  : Callers reference to its data buffer
*
* outputs:
*    Returns a handle to the message that has been created. 
*    Returns a NULL on error. 
*
* notes: 
*   The caller owns the memory associated with the Data msg
*   upon return.
*
***********************************************************/
struct DataMsg *createDataMsg(struct MsgHdr *myHdr, uint32_t dataSize, void *myData) 
{
  struct DataMsg *myMsg = NULL;

  if (myHdr != NULL) {
    myMsg = malloc(sizeof(DataMsgType));
    if (myMsg != NULL) {
      bzero(myMsg,sizeof(DataMsgType));
      myMsg->myHdr=myHdr;
      myMsg->dataSize=dataSize;
      myMsg->msgSize=(sizeof(DataMsgType) + dataSize);
      myMsg->dataPtr=myData;
      if (myData == NULL) { 
	/*
        printf("createDataMsg: WARNING: NULL DataPtr, dataSize=%d \n",
           dataSize);
	*/
        if (dataSize != 0)
           dataSize=0;
      }
#ifdef TRACEME
      printf("createDataMsg: Succeeded to create DataMsg sequenceNumber:%d,dataLen:%d,msgSize:%d \n",
           myMsg->myHdr->sequenceNum, myMsg->dataSize,myMsg->msgSize);
#endif
    }
  }
#ifdef TRACEME
  if (myMsg == NULL)
    printf("createDataMsg: Failed to create Data Msg  ??  \n");
#endif

  return myMsg; 
}

/***********************************************************
* Function: struct DataMsg *createEchoDataMsg(struct MsgHdr *myHdr, uint32_t dataSize, void *myData) 
*
* Explanation:  This creates a Data msg.
*
* inputs: 
*     MsgHdr *myHdr : Caller's reference to a valid MsgHdr
*                     Ownership transfers to this message.
*     uint32_t dataSize : size of data in the buffer
*     char *myData  : Callers reference to its data buffer
*
* outputs:
*    Returns a handle to the message that has been created. 
*    Returns a NULL on error. 
*
* notes: 
*   The caller owns the memory associated with the Data msg
*   upon return.
*
***********************************************************/
struct EchoDataMsg *createEchoDataMsg(uint32_t sequenceNum, uint16_t mode, uint32_t dataSize, void *myData)
{
  struct EchoDataMsg *myMsg = NULL;

  myMsg = malloc(sizeof(EchoDataMsgType));
  if (myMsg != NULL) {
      bzero(myMsg,sizeof(EchoDataMsgType));
      myMsg->sequenceNum=sequenceNum;
      myMsg->mode = mode;
      myMsg->dataSize=dataSize;
      myMsg->msgSize=(sizeof(EchoDataMsgType) + dataSize);
      myMsg->dataPtr=myData;
#ifdef TRACEME
      printf("createEchoDataMsg: Succeeded to create EchoDataMsg sequenceNumber:%d, dataLen:%d, msgSize:%d \n",
         myMsg->sequenceNum,myMsg->dataSize,myMsg->msgSize);
#endif
  }

#ifdef TRACEME
  if (myMsg == NULL)
    printf("createEchoDataMsg: Failed to create EchoData Msg  ??  \n");
#endif

  return myMsg; 

}


/***********************************************************
* Function: struct ACKMsg *createACKMsg(struct MsgHdr *myHdr, uint32_t RxSequenceNum) 
*
* Explanation:  This creates a ACK msg
*
* inputs: 
*     MsgHdr *myHdr : Reference to the caller's  msg header 
*                     Ownership transfers to this message.
*     uint32_t RxSequenceNum  : the RxSequence number that is
*            to be placed in this ACK Msg
*
* outputs:
*    Returns a handle to the message that has been created. 
*    Returns a NULL on error. 
*
* notes: 
*   The caller owns the memory associated with the msg
*   This routine will overwrite the sequenceNumber in the myHdr struct
***********************************************************/
struct ACKMsg *createACKMsg(struct MsgHdr *myHdr, uint32_t RxSequenceNum) 
{
  struct ACKMsg *myMsg = NULL;

  if (myHdr != NULL) {
    myMsg = malloc(sizeof(ACKMsgType));
    if (myMsg != NULL) {
      bzero(myMsg,sizeof(ACKMsgType));
      myMsg->myHdr = myHdr;
      myMsg->myHdr->sequenceNum = RxSequenceNum;
      myMsg->msgSize=sizeof(ACKMsgType);
#ifdef TRACEME
      printf("createACKMsg: Succeeded to create ACKMsg RxSequenceNumber:%d,msgSize:%d \n",
         myMsg->myHdr->sequenceNum,myMsg->msgSize);
#endif
    }
  }

#ifdef TRACEME
  if (myMsg == NULL)
    printf("createACKMsg: Failed to create ACK Msg  ??  \n");
#endif

  return myMsg; 
}

/***********************************************************
* Function: int freeDataMsg(struct DataMsg *myMsg) 
*
* Explanation:  This deletes the Msg from the system. 
*
* inputs: 
*     struct DataMsg *myMsg : The data msg to delete.
*
* outputs:
*    Returns a SUCCESS or FAILURE
*
* notes: 
*
***********************************************************/
int freeDataMsg(struct DataMsg *myMsg) 
{
  int rc = SUCCESS;
  if (myMsg != NULL) {
    if (myMsg->myHdr != NULL) {
      free(myMsg->myHdr);
    }
    if (myMsg->dataPtr != NULL) {
      free(myMsg->dataPtr);
    }
    else 
      printf("freeDataMsg: WARNING: Data Ptr NULL ,  myMsg->dataSize:%d   \n",myMsg->dataSize);

    free(myMsg);
  }
  else
    rc = FAILURE;

  return rc;
}

/***********************************************************
* Function: int freeEchoDataMsg(struct EchoDataMsg *myMsg) 
*
* Explanation:  This deletes the Msg from the system. 
*
* inputs: 
*     struct EchoDataMsg *myMsg : The  msg to delete.
*
* outputs:
*    Returns a SUCCESS or FAILURE
*
* notes: 
*
***********************************************************/
int freeEchoDataMsg(struct EchoDataMsg *myMsg) 
{
  int rc = SUCCESS;
  if (myMsg != NULL) {
    if (myMsg->dataPtr != NULL) {
     if (myMsg->dataSize == 0) {
        printf("freeEchoDataMsg: WARNING: dataPtr NOT NULL, but dataSize 0    \n");
      }
      free(myMsg->dataPtr);
    }
    else { 
      if (myMsg->dataSize > 0) {
        printf("freeEchoDataMsg: WARNING: No data to delete ?? dataSize:%d   \n",myMsg->dataSize);
        rc = FAILURE;
      }
    }

    free(myMsg);
  }
  else
    rc = FAILURE;

  return rc;
}

/***********************************************************
* Function: int freeACKMsg(struct DataMsg *myMsg) 
*
* Explanation:  This deletes the Msg from the system. 
*
* inputs: 
*     struct ACKMsg *myMsg : The ACK msg to delete.
*
* outputs:
*    Returns a SUCCESS or FAILURE
*
* notes: 
*
***********************************************************/
int freeACKMsg(struct ACKMsg *myMsg) 
{
  int rc = SUCCESS;
  if (myMsg != NULL) {
    if (myMsg->myHdr != NULL) {
      free(myMsg->myHdr);
    }
    free(myMsg);
  }
  else
    rc = FAILURE;

  return rc;
}

/***********************************************************
* Function: int packMsgHdrToNetworkBuffer(struct MsgHdr *myMsgHdr, char *networkBufferPtr, uint32_t bufSize)
*
* Explanation:  This lays out msg header  into a network buffer that can
*    then be sent of a socket.  
*
* inputs: 
*      struct MsgHdr *myMsgHdr : reference to callers msg hdr to be packed to a network buffer
*                                The caller continues to own the MsgHdr upon return
*      char *networkBufferPtr :  caller's reference to the network buffer
*                                This routine possibly writes into the callers networkBuffer.
*                                The caller continues to own that buffer.
*      uint32_t bufSize : size of the caller's network buffer
*
* outputs:
*    Returns a standard Unix ERROR (-1) on error, else the number of bytes
*      placed in the network buffer. 
*
* notes: 
*    Msg structures and typed information will be placed in the
*    network buffer in Network Byte Order.
*
***********************************************************/
int packMsgHdrToNetworkBuffer(struct MsgHdr *myMsgHdr, void *networkBufferPtr, uint32_t bufSize)
{
  void *dstPtr=NULL;
  int MsgHdrSize = sizeof(MsgHdrType);
  int octetCount = -1;
  if ((myMsgHdr != NULL) && (networkBufferPtr != NULL)) {

    //Check to make sure we do NOT overrun callers networkBuffer
    if (MsgHdrSize  <= bufSize) {

      //init dstPtr 
      dstPtr=networkBufferPtr;
      octetCount=0;

      //First, copy the SimpleHdr fields
      *(uint32_t *)(dstPtr+octetCount) =  htonl(myMsgHdr->sequenceNum);
      octetCount+=4;
      *(uint16_t *)(dstPtr+octetCount) =  htons(myMsgHdr->mode);
      octetCount+=2;
    

      //Then, copy the three remaining fields
      *(uint16_t *)(dstPtr+octetCount) =  htons(myMsgHdr->msgType);
      octetCount+=2;

      *(uint32_t *)(dstPtr+octetCount) =  htonl(myMsgHdr->timeSentSeconds);
      octetCount+=4;
      *(uint32_t *)(dstPtr+octetCount) =  htonl(myMsgHdr->timeSentUSeconds);
      octetCount+=4;
    }
  }

#ifdef TRACEME
  if (octetCount <=0)
    printf("packMsgHdrToNetworkBuffer: Failed   ??, octetCount:%d  \n",octetCount);
  else
    printf("packMsgHdrToNetworkBuffer: SUCCEEDED!  octetCount:%d  \n", octetCount);
#endif

  return octetCount;

}


/***********************************************************
* Function: int packACKMSGToNetworkBuffer(struct ACKMsg *myMsg, void *networkBufferPtr, uint32_t bufSize)
*
* Explanation:  This lays out an ACKMsg into a network buffer that can
*    then be sent of a socket.  
*
* inputs: 
*      struct ACKMsg *myMsg : msg to be packed to a network buffer
*                                The caller continues to own the MsgHdr upon return
*      char *networkBufferPtr :  caller's reference to the network buffer
*                                This routine possibly writes into the callers networkBuffer.
*                                The caller continues to own that buffer.
*      uint32_t bufSize : size of the caller's network buffer
*
* outputs:
*    Returns a standard Unix ERROR (-1) on error, else the number of bytes
*      placed in the network buffer. 
*
* notes: 
*    Msg structures and typed information will be placed in the
*    network buffer in Network Byte Order.
*
***********************************************************/
int packACKMSGToNetworkBuffer(struct ACKMsg *myMsg, void *networkBufferPtr, uint32_t bufSize)
{
  int octetCount = -1;
  void *dstPtr = NULL;

  //First, pack the hdr
  if ((myMsg != NULL) && (networkBufferPtr != NULL)) {
    octetCount =  packMsgHdrToNetworkBuffer(myMsg->myHdr, networkBufferPtr, bufSize);
  }


  //Next, the remaining struct fields
  
  //init dstPtr to point to netbuffer 
  dstPtr=networkBufferPtr;

  *(uint16_t *)(dstPtr+octetCount) =  htons(myMsg->msgSize);
  octetCount+=2;
    
#ifdef TRACEME
  if (octetCount <=0)
    printf("packAckMsgToNetworkBuffer: FAILED    ??, octetCount:%d  \n",octetCount);
  else
    printf("packAckMsgToNetworkBuffer: SUCCEEDED! octetCount:%d, msgSize:%d \n",  
         octetCount,myMsg->msgSize);
#endif

  return octetCount;
}

/***********************************************************
* Function: int packDataMSGToNetworkBuffer(struct DataMsg *myMsg, void *networkBufferPtr, uint32_t bufSize)
*
* Explanation:  This lays out a Data MSG into a network buffer that can
*    then be sent over a socket.  
*
* inputs: 
*      struct DATAMsg *myMsg : msg to be packed to a network buffer
*                                The caller continues to own the MsgHdr upon return
*      char *networkBufferPtr :  caller's reference to the network buffer
*                                This routine possibly writes into the callers networkBuffer.
*                                The caller continues to own that buffer.
*      uint32_t bufSize : size of the caller's network buffer
*
* outputs:
*    Returns a standard Unix ERROR (-1) on error, else the number of bytes
*      placed in the network buffer. 
*
* notes: 
*    Msg structures and typed information will be placed in the
*    network buffer in Network Byte Order.
*
***********************************************************/
int packDataMSGToNetworkBuffer(struct DataMsg *myMsg, void *networkBufferPtr, uint32_t bufSize)
{
  void *dstPtr= networkBufferPtr;
  int octetCount = -1;

  if ((myMsg != NULL) && (networkBufferPtr != NULL)) {

    octetCount =  packMsgHdrToNetworkBuffer(myMsg->myHdr, networkBufferPtr, bufSize);
    if (octetCount == -1) {
      printf("packDataMSGToNetworkBuffer: Failed   ?? octetCount: %d  \n",octetCount);
      return octetCount;
    }


    //Next, the remaining struct fields
    //init dstPtr to point to netbuffer 
    dstPtr=networkBufferPtr;

    *(uint16_t *)(dstPtr+octetCount) =  htons(myMsg->msgSize);
    octetCount+=2;
    *(uint16_t *)(dstPtr+octetCount) =  htons(myMsg->dataSize);
    octetCount+=2;

#ifdef TRACEME
    printf("packDataMsgToNetworkBuffer: After MsgHdr octetCount:%d,  \n",  
        octetCount);
#endif

    //Finally, copy the data 
    dstPtr=networkBufferPtr+octetCount;
    if (myMsg->dataPtr != NULL) {
      if (myMsg->dataSize > 0) {
        memcpy((void *)dstPtr, (void *) myMsg->dataPtr, myMsg->dataSize);
        octetCount+=myMsg->dataSize;
      } else {
        printf("packDataMsgToNetworkBuffer: WARNING: NULL dataPtr !! \n");
      }
    } else {
	/*
      printf("packDataMsgToNetworkBuffer: WARNING: NULL dataPtr, dataSize=%d setting to 0 !! \n",myMsg->dataSize);
     myMsg->dataSize = 0;
      	*/
    }

#ifdef TRACEME
    printf("packDataMsgToNetworkBuffer: SUCCEEDED to pack DataMsg! msgSize:%d,    octetCount:%d, datasize:%d  \n",  
               myMsg->msgSize,octetCount,myMsg->dataSize);
#endif

  }
  else {
    printf("packMsgHdrToNetworkBuffer: WARNING Failed   ??  \n");
  }

  return octetCount;

}

/***********************************************************
* Function: int packEchoDataMsgToNetworkBuffer(struct EchoDataMsg *myMsg, void *networkBufferPtr, uint32_t bufSize)
*
* Explanation:  This lays out an ACKMsg into a network buffer that can
*    then be sent of a socket.  
*
* inputs: 
*      struct EchoDataMsg *myMsg : msg to be packed to a network buffer
*                                The caller continues to own the MsgHdr upon return
*      char *networkBufferPtr :  caller's reference to the network buffer
*                                This routine possibly writes into the callers networkBuffer.
*                                The caller continues to own that buffer.
*      uint32_t bufSize : size of the caller's network buffer
*
* outputs:
*    Returns a standard Unix ERROR (-1) on error, else the number of bytes
*      placed in the network buffer. 
*
* notes: 
*    Msg structures and typed information will be placed in the
*    network buffer in Network Byte Order.
*
***********************************************************/
int packEchoDataMSGToNetworkBuffer(struct EchoDataMsg *myMsg, void  *networkBufferPtr, uint32_t bufSize)
{
  void *dstPtr= networkBufferPtr;
  int octetCount = -1;

  if ((myMsg != NULL) && (networkBufferPtr != NULL)) {
  
    //init dstPtr 
    dstPtr=networkBufferPtr;
    octetCount=0;

    //First, copy the SimpleHdr fields
    *(uint32_t *)(dstPtr+octetCount) =  htonl(myMsg->sequenceNum);
    octetCount+=4;
    *(uint16_t *)(dstPtr+octetCount) =  htons(myMsg->mode);
    octetCount+=2;
    

    //Then, copy the msgSize and dataSize
    *(uint16_t *)(dstPtr+octetCount) =  htons(myMsg->msgSize);
    octetCount+=2;
    *(uint16_t *)(dstPtr+octetCount) =  htons(myMsg->dataSize);
    octetCount+=2;

    //Next, copy the data 
    //Finally, copy the data 
    dstPtr=networkBufferPtr+octetCount;
    if (myMsg->dataPtr != NULL) {
      if (myMsg->dataSize > 0) {
        memcpy((void *)dstPtr, (void *) myMsg->dataPtr, myMsg->dataSize);
        octetCount+=myMsg->dataSize;
      } else {
        printf("packEchoDataMsgToNetworkBuffer: WARNING: NULL dataPtr !! \n");
      }
    } else {
      printf("packEchoDataMsgToNetworkBuffer: WARNING: NULL dataPtr, dataSize=%d setting to 0 !! \n",myMsg->dataSize);
      myMsg->dataSize = 0;
    }
  }

#ifdef TRACEME
  printf("packEchoDataMsgToNetworkBuffer: SUCCEEDED to pack DataMsg! msgSize:%d,    octetCount:%d, datasize:%d  \n",  
             myMsg->msgSize,octetCount,myMsg->dataSize);
#endif

  return octetCount;

}

/***********************************************************
* Function: int unpackNetworkBufferToMsgHdr(struct MsgHdr **CallersMsgHdrPtr, void *networkBufferPtr, uint32_t bufSize)
*
* Explanation:  This decodes a MsgHdr from the network buffer. 
*
* inputs: 
*   struct MsgHdr **CallersMsgHdrPtr, 
*   void *networkBufferPtr, uint32_t bufSize)
*   uint32_t bufSize
*
* outputs:
*    Returns the number of octets unpacked
*    The callers reference to its MsgPtr is set to point to the newly created DataMsg
*    On error, FAILURE is returned.
*
* notes: 
*    Data in the network buffer is assumed to be in network byte order.
*    Required data types are placed into the msg structure in appropriate
*    Host byte format. 
*
***********************************************************/
int unpackNetworkBufferToMsgHdr(struct MsgHdr **CallersMsgHdrPtr, void *networkBufferPtr, uint32_t bufSize)
{
  struct MsgHdr *myMsgHdr = NULL;
  void *srcPtr=NULL;
  int rc = FAILURE;
  int octetCount = -1;

  if (networkBufferPtr != NULL) {
    //Check to make sure we do NOT overrun callers networkBuffer
    //If this is true, the caller has passed all the data....
    if (bufSize>0) {

      //create the data structure with the create routine 
      //myMsgHdr =createMsgHdr( uint32_t sequenceNum, uint16_t mode, uint16_t msgType, 
      //                       uint32_t timeSentSeconds, uint32_t timeSentUSeconds)

      //To make it a bit easier to debug we make the Msg Hdr in this in this code....
      myMsgHdr = malloc(sizeof(MsgHdrType));

      if (myMsgHdr != NULL)
      {
        bzero(myMsgHdr,sizeof(MsgHdrType));
        rc = SUCCESS;
        octetCount = 0;
        //init srcPtr 
        srcPtr=networkBufferPtr;

        //First, copy the sequence/mode fields
        myMsgHdr->sequenceNum = ntohl(*(uint32_t *)(srcPtr+octetCount));
        octetCount+=4;

        myMsgHdr->mode = ntohs(*(uint16_t *)(srcPtr+octetCount));
        octetCount+=2;

        //Then, copy the msgType and then the timestamp 
        myMsgHdr->msgType = ntohs(*(uint16_t *)(srcPtr+octetCount));
        octetCount+=2;

        myMsgHdr->timeSentSeconds = ntohl(*(uint32_t *)(srcPtr+octetCount));
        octetCount+=4;
        myMsgHdr->timeSentUSeconds = ntohl(*(uint32_t *)(srcPtr+octetCount));
        octetCount+=4;
      }
    }
  }

  if ( rc != SUCCESS) {
    printf("unpackNetworkBufferToMsgHdr: Failed   ??  \n");
    myMsgHdr = NULL;
    octetCount = -1;
  }

#ifdef TRACEME
  if (rc == SUCCESS){
    printf("unpackNetworkBufferMsgHdr: SUCCEEDED!  octetCount:%d  \n",  
         octetCount);
    printf("unpackNetworkBufferMsgHdr: seq:%d, mode:%d, msgType:%d, sec.usec:%d.%d \n",  
        myMsgHdr->sequenceNum,
        myMsgHdr->mode,
        myMsgHdr->msgType,
        myMsgHdr->timeSentSeconds,
        myMsgHdr->timeSentUSeconds);
  }
#endif

//  return myMsgHdr;
//Update callers Ptr with the refernece to the msghdr
  *(CallersMsgHdrPtr) = myMsgHdr;
  return octetCount;
}

/***********************************************************
* Function: int unpackNetworkBufferToDataMsg(struct DataMsg **CallersDataMsgPtr, void *networkBufferPtr, uint32_t bufSize)
*
* Explanation:  This decodes a DataMsg from the network buffer. 
*
* inputs: 
*   struct DataMsg **CallersDataMsgPtr, 
*   void *networkBufferPtr, uint32_t bufSize)
*   char *networkBufferPtr
*   uint32_t bufSize
*
* outputs:
*    Returns the number of octets unpacked
*    The callers reference to its MsgPtr is set to point to the newly created DataMsg
*    On error, FAILURE is returned.
*
* notes: 
*    Data in the network buffer is assumed to be in network byte order.
*    Required data types are placed into the DataMsg structure in appropriate
*    Host byte format. 
*
***********************************************************/
int unpackNetworkBufferToDataMsg(struct DataMsg **CallersDataMsgPtr, void *networkBufferPtr, uint32_t bufSize)
{
  struct DataMsg *myMsg = NULL;
  void *myData = NULL;
  struct MsgHdr *myMsgHdr = NULL;
  void *srcPtr = NULL;
  int rc = FAILURE;
  int octetCount = -1;
  int dataCount = -1;


  dataCount = unpackNetworkBufferToMsgHdr(&myMsgHdr,networkBufferPtr, bufSize);

  if (myMsgHdr == NULL) {
    printf("unpackNetworkBufferToDataMsg: Failed unpack MsgHdr, NULL,  dataCount=%d  ??  \n",dataCount);
    return rc;
  }

  if (dataCount  > 0) { 
    srcPtr = networkBufferPtr;
    octetCount = dataCount;
    myMsg = malloc(sizeof(DataMsgType));
    if (myMsg == NULL) {
      return rc;
    }
    bzero(myMsg,sizeof(DataMsgType));
    myMsg->myHdr=myMsgHdr;
    myMsg->msgSize= ntohs(*(uint16_t *)(srcPtr+octetCount));
    octetCount+=2;
    myMsg->dataSize= ntohs(*(uint16_t *)(srcPtr+octetCount));
    octetCount+=2;

    if ((myMsg->dataSize >0) && (myMsg->dataSize < 1000000)) {
      myData = malloc(myMsg->dataSize);
      if (myData == NULL) {
        printf("unpackNetworkBufferToDataMsg: Failed malloc of myData size %d   ??  \n",myMsg->dataSize);
        freeDataMsg(myMsg); 
        myMsg = NULL;
      } else {
        bzero(myData,myMsg->dataSize);
#ifdef TRACEME
        printf("unpackNetworkBufferToDataMsg: malloc databuffer of size :%d  \n",  
            myMsg->dataSize);
#endif

        myMsg->dataPtr= myData;
//        octetCount+=4;
        //Copy from netbuf into  myData 
        srcPtr=networkBufferPtr+octetCount;
        memcpy((void *)myMsg->dataPtr,(void *)srcPtr,myMsg->dataSize);
        octetCount+=myMsg->dataSize;
#ifdef TRACEME
        printf("unpackNetworkBufferToDataMsg: memcpy data %d octects, current octetCount:%d,  \n",  
            myMsg->dataSize,octetCount);
        showBuf(myMsg->dataPtr, myMsg->dataSize);
#endif
      }
    } else 
    {
      printf("unpackNetworkBufferToDataMsg: Failed, bad dataSize : %d   ??  \n",myMsg->dataSize);
      myMsg = NULL;
    }
  }
  else {
    printf("unpackNetworkBufferToDataMsg: Failed, dataCount : %d   ??  \n",dataCount);
    myMsg = NULL;
  }

  if (myMsg == NULL) {
    printf("unpackNetworkBufferToDataMsg: FAILED! Return NULL change   octetCount:%d to FAILURE  \n",  
         octetCount);
    octetCount = -1;
  }

#ifdef TRACEME
  if (myMsg != NULL)
    printf("unpackNetworkBufferToDataMsg: SUCCEEDED! dataSize:%d, msgSize:%d  octetCount:%d  \n",  
         myMsg->dataSize, myMsg->msgSize,octetCount);
#endif

//  return myMsg;
  *(CallersDataMsgPtr) = myMsg;
  return octetCount;
}


/***********************************************************
* Function: int unpackNetworkBufferToEchoDataMsg(struct EchoDataMsg **CallersEchoDataMsgPtr, void *networkBufferPtr, uint32_t bufSize)
*
* Explanation:  This decodes a EchoDataMsg from the network buffer. 
*
* inputs: 
*   struct EchoDataMsg **CallersDataMsgPtr, 
*   void *networkBufferPtr, uint32_t bufSize)
*   char *networkBufferPtr
*   uint32_t bufSize
*
* outputs:
*    Returns the number of octets unpacked
*    The callers reference to its MsgPtr is set to point to the newly created DataMsg
*    On error, FAILURE is returned.
*
* notes: 
*    Data in the network buffer is assumed to be in network byte order.
*    Required data types are placed into the DataMsg structure in appropriate
*    Host byte format. 
*
******************************************************************/
int unpackNetworkBufferToEchoDataMsg(struct EchoDataMsg **CallersEchoDataMsgPtr, void *networkBufferPtr, uint32_t bufSize)
{
  struct EchoDataMsg *myMsg = NULL;
  void *myData = NULL;
  void *srcPtr=NULL;
  int rc = FAILURE;
  int octetCount = -1;

  if (networkBufferPtr == NULL) 
    return rc;
  if (bufSize <= 0) 
    return rc;

  srcPtr = networkBufferPtr;
  myMsg = malloc(sizeof(EchoDataMsgType));
  if (myMsg == NULL) {
    return rc;
  }
  bzero(myMsg,sizeof(EchoDataMsgType));
  octetCount = 0;
  myMsg->sequenceNum = ntohl(*(uint32_t *)(srcPtr+octetCount));
  octetCount+=4;
  myMsg->mode = ntohs(*(uint16_t *)(srcPtr+octetCount));
  octetCount+=2;
  myMsg->msgSize= ntohs(*(uint16_t *)(srcPtr+octetCount));
  octetCount+=2;
  myMsg->dataSize= ntohs(*(uint16_t *)(srcPtr+octetCount));
  octetCount+=2;

  if ((myMsg->dataSize >0) && (myMsg->dataSize < 1000000)) {
    myData = malloc(myMsg->dataSize);
    if (myData == NULL) {
      printf("unpackNetworkBufferToEchoDataMsg: Failed malloc of myData size %d   ??  \n",myMsg->dataSize);
      freeEchoDataMsg(myMsg); 
      myMsg = NULL;
    } else {
      bzero(myData,myMsg->dataSize);
#ifdef TRACEME
      printf("unpackNetworkBufferToEchoDataMsg: malloc databuffer of size :%d  \n",  
            myMsg->dataSize);
#endif

      myMsg->dataPtr= myData;
//        octetCount+=4;
      //Copy from netbuf into  myData 
      srcPtr=networkBufferPtr+octetCount;
      memcpy((void *)myMsg->dataPtr,(void *)srcPtr,myMsg->dataSize);
      octetCount+=myMsg->dataSize;
#ifdef TRACEME
      printf("unpackNetworkBufferToEchoDataMsg: memcpy data %d octects, current octetCount:%d,  \n",  
            myMsg->dataSize,octetCount);
      showBuf(myMsg->dataPtr, myMsg->dataSize);
#endif
    }
  } else 
  {
      printf("unpackNetworkBufferToEchoDataMsg: Failed, bad dataSize : %d   ??  \n",myMsg->dataSize);
      myMsg = NULL;
  }


  if (myMsg == NULL) {
    printf("unpackNetworkBufferToEchoDataMsg: FAILED! Return NULL change   octetCount:%d to FAILURE  \n",  
         octetCount);
    octetCount = -1;
  }

#ifdef TRACEME
  if (myMsg != NULL)
    printf("unpackNetworkBufferToEchoDataMsg: SUCCEEDED! dataSize:%d, msgSize:%d  octetCount:%d  \n",  
         myMsg->dataSize, myMsg->msgSize,octetCount);
#endif

//  return myMsg;
  *(CallersEchoDataMsgPtr) = myMsg;
  return octetCount;

}

/***********************************************************
* Function: struct DataMsg *unpackNetworkBufferToACKMsg(void **networkBufferPtr, uint32_t bufSize)
*
* Explanation:  This access the data in the network buffer and
*               creates and returns an ACK Msg.
*
* inputs: 
*    char *networkBufferPtr
*    uint32_t bufSize
*
* outputs:
*    Returns a pointer to the new ACKMsg structure.  If an error
*      occurs, a NULL is returned.
*
* notes: 
*    Data in the network buffer is assumed to be in network byte order.
*    Required data types are placed into the DataMsg structure in appropriate
*    Host byte format. 
*
***********************************************************/
int unpackNetworkBufferToACKMsg(struct ACKMsg **CallersAckMsgPtr, void *networkBufferPtr, uint32_t bufSize)
{
  struct ACKMsg *myMsg = NULL;
  struct MsgHdr *myMsgHdr = NULL;
  void *srcPtr = NULL;
  int rc = FAILURE;
  int octetCount = -1;
  int dataCount = -1;

  dataCount = unpackNetworkBufferToMsgHdr(&myMsgHdr,networkBufferPtr, bufSize);

  if (myMsgHdr == NULL) {
    printf("unpackNetworkBufferToDataMsg: Failed unpack MsgHdr, NULL,  dataCount=%d  ??  \n",dataCount);
    return rc;
  }

  if (dataCount  > 0)  
  {
    srcPtr = networkBufferPtr;
    octetCount =sizeof(MsgHdrType);
    myMsg = malloc(sizeof(ACKMsgType));
    if (myMsg == NULL) {
      return rc;
    }
    bzero(myMsg,sizeof(ACKMsgType));
    myMsg->myHdr=myMsgHdr;
    myMsg->msgSize= ntohs(*(uint16_t *)(srcPtr+octetCount));
    octetCount+=2;
  }
  else {
    printf("unpackNetworkBufferToAckMsg: Failed, dataCount : %d   ??  \n",dataCount);
    myMsg = NULL;
  }

  if (myMsg == NULL) {
    printf("unpackNetworkBufferToAckMsg: FAILED! Return NULL change   octetCount:%d to FAILURE  \n",  
         octetCount);
    octetCount = -1;
  }

#ifdef TRACEME
  if (myMsg != NULL)
    printf("unpackNetworkBufferToAckMsg: SUCCEEDED!  msgSize:%d  octetCount:%d  \n",  
         myMsg->msgSize,octetCount);
#endif
//  return myMsg;
  *(CallersAckMsgPtr) = myMsg;
  return octetCount;
}

/***********************************************************
* Function: int compareMsgHdrs( struct MsgHdr *MsgHdr1, struct MsgHdr *MsgHdr2)
*
* Explanation: This routine compares the two Msg Hdrs.
*              Returns a TRUE if they are
*              identical in content, else returns a FALSE if they are different.
*
* inputs: 
*  struct MsgHdr *MsgHdr1 - first msg hdr
*  struct MsgHdr *MsgHdr2 - second msg hdr
*
* outputs:
*     Returns TRUE if the msgs are identical, else a FALSE
*
* notes: 
*
***********************************************************/
int compareMsgHdrs( struct MsgHdr *MsgHdr1, struct MsgHdr *MsgHdr2)
{
  int rc = FALSE;

  if (MsgHdr1->sequenceNum != MsgHdr2->sequenceNum) {
    printf("compareMsgHdrs: Fail seqNums: %d  %d \n",
           MsgHdr1->sequenceNum, MsgHdr2->sequenceNum);
    return rc;
  }
  if (MsgHdr1->mode != MsgHdr2->mode) {
    printf("compareMsgHdrs: Fail mode: %d  %d \n",
           MsgHdr1->mode, MsgHdr2->mode);
    return rc;
  }
  if (MsgHdr1->msgType != MsgHdr2->msgType)  {
    printf("compareMsgHdrs: Fail msgType: %d  %d \n",
           MsgHdr1->msgType, MsgHdr2->msgType);
    return rc;
  }
  if (MsgHdr1->timeSentSeconds != MsgHdr2->timeSentSeconds) {
    printf("compareMsgHdrs: Fail Seconds : %d  %d \n",
           MsgHdr1->timeSentSeconds, MsgHdr2->timeSentSeconds);
    return rc;
  }
  if (MsgHdr1->timeSentUSeconds != MsgHdr2->timeSentUSeconds)  {
    printf("compareMsgHdrs: Fail Useconds: %d  %d \n",
           MsgHdr1->timeSentUSeconds, MsgHdr2->timeSentUSeconds);
    return rc;
  }

  rc= TRUE;
#ifdef TRACEME
  printf("compareMsgHdrs: MATCHED sequence num:%d  \n",  
         MsgHdr1->sequenceNum);
#endif
  return rc;
}


/***********************************************************
* Function: int compareAckMsgs( struct ACKMsg *Msg1Ptr, struct ACKMsg *Msg2Ptr)
*
* Explanation: This routine compares the two AckMsgs, returns a TRUE if they are
*              identical in content, else returns a FALSE if they are different.
*
* inputs: 
*     struct ACKMsg *Msg1Ptr : FIrst ACK msg 
*     struct ACKMsg *Msg2Ptr : Second ACK msg 
*
* outputs:
*     Returns TRUE if the msgs are identical, else a FALSE
*
* notes: 
*
***********************************************************/
int compareAckMsgs( struct ACKMsg *Msg1Ptr, struct ACKMsg *Msg2Ptr)
{
  int rc = FALSE;
  if (compareMsgHdrs(Msg1Ptr->myHdr,Msg2Ptr->myHdr) == FALSE) 
    return rc;

  if (Msg1Ptr->msgSize != Msg2Ptr->msgSize) 
    return rc;

  rc=TRUE;
#ifdef TRACEME
  printf("compareAckMsgs: MATCHED Rxsequence num:%d, msgSize:%d  \n",  
         Msg1Ptr->myHdr->sequenceNum,Msg1Ptr->msgSize);
#endif
  return rc;
}


/***********************************************************
* Function: int compareDataMsgs( struct DataMsg *Msg1Ptr, struct DataMsg *Msg2Ptr)
*
* Explanation: This routine compares the two AckMsgs, returns a TRUE if they are
*              identical in content, else returns a FALSE if they are different.
*
* inputs: 
*     struct DataMsg *Msg1Ptr : FIrst Data msg 
*     struct DataMsg *Msg2Ptr : Second Data msg 
*
* outputs:
*     Returns TRUE if the msgs are identical, else a FALSE
*
* notes: 
*
***********************************************************/
int compareDataMsgs( struct DataMsg *Msg1Ptr, struct DataMsg *Msg2Ptr)
{
  int rc = FALSE;

  if (compareMsgHdrs(Msg1Ptr->myHdr,Msg2Ptr->myHdr) == FALSE) 
    return rc;

#ifdef TRACEME
  printf("compareDataMsgs: MATCHED MsgHdrs .... sequence nums %d  %d  \n",  
         Msg1Ptr->myHdr->sequenceNum, Msg2Ptr->myHdr->sequenceNum);
#endif

  if (Msg1Ptr->msgSize != Msg2Ptr->msgSize) 
    return rc;
  if (Msg1Ptr->dataSize != Msg2Ptr->dataSize) 
    return rc;

  if ((Msg1Ptr->dataPtr == NULL) && (Msg2Ptr->dataPtr == NULL))  {
#ifdef TRACEME
    printf("compareDataMsgs: MATCHED and both dataPtrs NULL  sequence nums %d  %d  \n",  
         Msg1Ptr->myHdr->sequenceNum, Msg2Ptr->myHdr->sequenceNum);
#endif
    rc = TRUE;
  }
  else {
    if ((Msg1Ptr->dataPtr == NULL) || (Msg2Ptr->dataPtr == NULL)) {
      printf("compareDataMsgs:FAILED TO MATCH as one dataPtr is NULL  sequence nums %d  %d  \n",  
         Msg1Ptr->myHdr->sequenceNum, Msg2Ptr->myHdr->sequenceNum);
      return rc;
    } else {

#ifdef TRACEME
      printf("compareDataMsgs: MATCHED....now check dataPtr dataSize: %d  %d  \n",  
         Msg1Ptr->dataSize, Msg2Ptr->dataSize);
#endif
      //Compare data
      char *tmpPtr1 =Msg1Ptr->dataPtr;
      char *tmpPtr2 =Msg2Ptr->dataPtr;
      int i = 0;
      for (i=0;i<Msg1Ptr->dataSize; i++) {
#ifdef TRACEME
         printf("compareDataMsgs: index:%d data: %d  %d  \n",  
             i, tmpPtr1[i], tmpPtr2[i]);
#endif
        if (tmpPtr1[i] != tmpPtr2[i])
          return rc;
      }
      rc = TRUE;
    }
  }

#ifdef TRACEME
  printf("compareDataMsgs: MATCHED sequence num:%d, msgSize:%d, dataSize:%d  \n",  
         Msg1Ptr->myHdr->sequenceNum,Msg1Ptr->msgSize,Msg1Ptr->dataSize);
#endif
  return rc;
}

/***********************************************************
* Function: int compareEchoDataMsgs( struct EchoDataMsg *Msg1Ptr, struct DataMsg *Msg2Ptr)
*
* Explanation: This routine compares the two Msgs, returns a TRUE if they are
*              identical in content, else returns a FALSE if they are different.
*
* inputs: 
*     struct EchoDataMsg *Msg1Ptr : FIrst Data msg 
*     struct EchoDataMsg *Msg2Ptr : Second Data msg 
*
* outputs:
*     Returns TRUE if the msgs are identical, else a FALSE
*
* notes: 
*
***********************************************************/
int compareEchoDataMsgs( struct EchoDataMsg *Msg1Ptr, struct EchoDataMsg *Msg2Ptr)
{
  int rc = FALSE;

  if (Msg1Ptr->sequenceNum != Msg2Ptr->sequenceNum) 
    return rc;
  if (Msg1Ptr->mode != Msg2Ptr->mode) 
    return rc;
  if (Msg1Ptr->msgSize != Msg2Ptr->msgSize) 
    return rc;
  if (Msg1Ptr->dataSize != Msg2Ptr->dataSize) 
    return rc;

  rc= TRUE;
#ifdef TRACEME
  printf("compareEchoDataMsgs: MATCHED sequence num:%d, msgSize:%d  \n",  
         Msg1Ptr->sequenceNum, Msg1Ptr->msgSize);
#endif
  return rc;
}

/***********************************************************
* Function: int dumpBufferToFile(void *bufferPtr, uint32_t bufSize, char *fileName)
*
* Explanation:  This dumps the buffer to a file in binary format
*               It is stored as a stream using a fwrite operation.
*
* inputs: 
*    var *bufferPtr : reference to caller's buffer
*    uint32_t bufSize size of buffer
*    char *fileName : file name
*
* outputs:
*    Returns a FAILURE or the number of bytes in the file 
*
* notes: 
*   To view the contents of the file, issue od -x filename
*
***********************************************************/
int dumpBufferToFile(void *bufferPtr, uint32_t bufSize, char *fileName)
{
  int size=FAILURE;
  int rc = FAILURE;
  int statRC=0;
  struct stat st;

  FILE *bufferFile = NULL;


  bufferFile = fopen(fileName, "w+");
  if (bufferFile == NULL)
    return rc;

  size = fwrite(bufferPtr,sizeof(char),bufSize,bufferFile);
  fclose(bufferFile);
 
  statRC = stat(fileName,&st);

  if ( (size <bufSize) || (statRC > 0)) {
    printf("dumpBufferToFile: ERROR on dump to file %s or stat : rc = %d   \n",fileName,rc);
    return rc;
  }

  //We are good if we get here

#ifdef TRACEME
  printf("dumpBufferToFile: Succeded to dump to file %s, paramSize:%d, sizeFwrite:%d, statSize:%d  \n",fileName,bufSize, size, (int)st.st_size);
#endif

  return size;
}

void showBuf(void *bufPtr, uint32_t bufSize)
{
  int loopCount = bufSize;
  char *tmpPtr = bufPtr;

  while(loopCount--) 
  {
    printf(" %u ",*tmpPtr++);
  }
  printf("\n"); 
}



