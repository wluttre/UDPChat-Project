
/*********************************************************
* Program:   UDPEchoV1-2 -  client   and server  
*
* Summary:  This is an implementation of a UDP version of ping.
*
* Usage :   client  
*             <Server IP> 
*             <Server Port> 
*             [<Iteration Delay (usecs)>] 
*             [<Message Size (bytes)>] 
*             [<# of iterations>] 
*             [<Debug Flag (default : 129)>]\n", 
*
*
* Usage :    server 
*             <Server Port> 
*             [<Debug Flag (default : 129)>]\n", 
*
*
*  This file contains the echo server code
*   debugFlag && 0x007f  = debugLevel
*   if (debugFlag && 0x80) 
*        creates RTT.dat
*
*   debugLevel:
*    0:  nothing displayed except final result
*    1:  (default) adds errors and warning
*    2:  adds RTT samples
*    4:  some debug
*    5:  lots of debug
*
*    Ex.   debugFlag 2  Does NOT create RTT.dat, does show RTT samples to stdout
*                    129 :  only shows WARNINGs/Errors to stdout AND creates RTT.dat
*
*
*
*
*********************************************************/



*********************************************************

Invocation:

This invokes the server, specifying port 5000
./server <port> <debugLevel>
Example:  ./server 5000 1 ; #debug level  1 to just show warnings

This invokes the client, specifying the server name as localhost (ie., the
server runs on the same machiine), server port 5000,  an iteration
delay of 1 second, a message size of 1000 bytes, and 100 iterations.
./client server serverPort <iteration delay>  <message size> <numberIterations> <debugLevel>
Example : ./client localhost 5000 1000000 1000 100 130 

The client shows the following output ....
  to standard out if debugLevel is >=2
  and to RTT.dat if the 8th bit of debugLevel is set (so effectively if debugLevel >=128

timestamp       RTT sample smoothedRTT RxSeqNumber largestSeqNumberSent #outOforder #Lost
1487747591.227005 0.002747 0.001373 1 1 0 0
1487747592.229611 0.002295 0.001834 2 2 0 0
1487747593.295905 0.066198 0.034016 3 3 0 0
1487747594.320419 0.024117 0.029067 4 4 0 0
1487747595.344165 0.023650 0.026358 5 5 0 0
1487747596.368747 0.024425 0.025392 6 6 0 0


Avg Ping: 184 microseconds Loss: 0 Percent

The server show no summary information.  

*********************************************************



*********************************************************

TODO's:
-better testing for strange socket errors
-explore the use of more accurate delays
   (e.g., evaluate usleep, nanosleep, or select )


*********************************************************














