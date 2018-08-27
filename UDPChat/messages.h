/************************************************************************
* File:  messages.h
*
* Purpose:
*   This include file defines the formats for the different messages 
*       in the system.
*   Provides the pack and unpack routines that should be used to
*   when sending/receiving msg's over a network.
*
* Notes:
*   The msgSize field in the msg structs should be the sum of all components
*   in the struct -  identical to sizeof
*   dataSize - is the amount of data in any message with actual data.
*
*   The size of the packed message in a network buffer will be different
*   from the msgSize. 
*
************************************************************************/
#ifndef	__messages_h
#define	__messages_h

#include "common.h"


/*possible msg types */
#define NOMSG   0  
#define DATAMSG 1
#define ACKMSG  2
#define CONTROLMSG  3

/*possible DATA MSG data formats */
#define BINARY    0
#define TEXT      1
#define JSON      2
#define PROTOBUFs 3

/* 
 *  Currently not used
*/
typedef struct SimpleMsgHdr {
  uint32_t sequenceNum;
  uint16_t mode;       //for backwards compatibility- operating modes 0 and 1
} SimpleMsgHdrType ;
/*
 *  Currently not used
*/
typedef struct OLDMsgHdr {
  SimpleMsgHdrType mySimpleHdr;
  uint16_t msgType;
  uint32_t timeSentSeconds;
  uint32_t timeSentUSeconds;
} OLDMsgHdrType ;

typedef struct MsgHdr {
  uint32_t sequenceNum;
  uint16_t mode;       //for backwards compatibility- operating modes 0 and 1
  uint16_t msgType;
  uint32_t timeSentSeconds;
  uint32_t timeSentUSeconds;
} MsgHdrType ;


/* 
 * The following define the internal or abstract representation of the messages.
*/
typedef struct ACKMsg {
  MsgHdrType  *myHdr;
  uint16_t msgSize;
} ACKMsgType ;

typedef struct DataMsg {
  MsgHdrType  *myHdr;
  uint16_t msgSize;
  uint16_t dataSize;
  void *dataPtr;
} DataMsgType ;


//Can be used for Modes 0 or 1
typedef struct EchoDataMsg {
  uint32_t sequenceNum;
  uint16_t mode;       
  uint16_t msgSize;
  uint16_t dataSize;
  void *dataPtr;
} EchoDataMsgType ;


struct SimpleMsgHdr *createSimpleMsgHdr( uint32_t sequenceNum, uint16_t mode);

struct MsgHdr *createMsgHdr( uint32_t sequenceNum, uint16_t mode, uint16_t msgType, 
                             uint32_t timeSentSeconds, uint32_t timeSentUSeconds);

struct DataMsg *createDataMsg(struct MsgHdr *myHdr, uint32_t dataSize, void *myData); 
struct ACKMsg *createACKMsg(struct MsgHdr *myHdr, uint32_t RxSequenceNum); 
struct EchoDataMsg *createEchoDataMsg(uint32_t sequenceNum, uint16_t mode, uint32_t dataSize, void *myData);
 
int compareMsgHdrs( struct MsgHdr *MsgHdr1, struct MsgHdr *MsgHdr2);
int compareAckMsgs( struct ACKMsg *Msg1Ptr, struct ACKMsg *Msg2Ptr);
int compareDataMsgs( struct DataMsg *Msg1Ptr, struct DataMsg *Msg2Ptr);
int compareEchoDataMsgs( struct EchoDataMsg *Msg1Ptr, struct EchoDataMsg *Msg2Ptr);

int freeDataMsg(struct DataMsg *myMsg);
int freeACKMsg(struct ACKMsg *myMsg);
int freeEchoDataMsg(struct EchoDataMsg *myMsg);

int packMsgHdrToNetworkBuffer(struct MsgHdr *myMsgHdr, void *networkBufferPtr, uint32_t bufSize);
int packACKMSGToNetworkBuffer(struct ACKMsg *myMsg, void *networkBufferPtr, uint32_t bufSize);
int packDataMSGToNetworkBuffer(struct DataMsg *myMsg, void  *networkBufferPtr, uint32_t bufSize);
int packEchoDataMSGToNetworkBuffer(struct EchoDataMsg *myMsg, void  *networkBufferPtr, uint32_t bufSize);

//struct MsgHdr *unpackMsgHdrToNetworkBuffer(void *netBufferPtr, uint32_t bufSize);
//struct DataMsg *unpackNetworkBufferToDataMsg(void *networkBufferPtr, uint32_t bufSize);
//struct ACKMsg *unpackNetworkBufferToACKMsg(void *networkBufferPtr, uint32_t bufsize);
int unpackNetworkBufferToMsgHdr(struct MsgHdr **CallersMsgPtr, void *netBufferPtr, uint32_t bufSize);
int unpackNetworkBufferToDataMsg(struct DataMsg **CallersDataMsgPtr, void *networkBufferPtr, uint32_t bufSize);
int unpackNetworkBufferToACKMsg(struct ACKMsg **CallersAckMsgPtr, void *networkBufferPtr, uint32_t bufsize);
int unpackNetworkBufferToEchoDataMsg(struct EchoDataMsg **CallersEchoDataMsgPtr, void *networkBufferPtr, uint32_t bufSize);

int dumpBufferToFile(void *bufferPtr, uint32_t bufSize, char *fileName);
void showBuf(void *bufPtr, uint32_t bufSize);

#endif


