/*
 Author: Yan Ochoa (No partner)
 CSC 425
  * server restart
 Milestone 3
 */

#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <string.h>
#include <sys/select.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define MAX_LINE 1024


typedef struct{
    uint32_t type; //type of the message, look for macros values
    int lastReceivedMessage;
    int newSesh;
    int messageSize;
    int seqNum;
    int inUse;
    int corrupt;
    int ackNum;
    char *payload;
    
}myMessage; 



    int sPort;
    int dummy = 1;
    int dummy2 = -1;
    int rvVal;
    int lengthPayload = -1;
    struct sockaddr_in clientAdress;
    struct sockaddr_in serverAdress;
    struct timeval timeVal;
    int telnetSocket;
    int clientListenSocket;
    int lastMsg = -1;
    int cproxy;
    int currSeq= 1000;
    int knownAck = 1000;
    
    
int byteStream(int socket, char *buff, int size, int incore) {
    int stream = 0;
    int in = incore;
    bool ready = false;
    int working = 1;
    int iterator = 0;
    int debugg =1;

    while((size - stream) > 0) {
        iterator = read(socket, (buff + stream), (size - stream));
        if( iterator <= 0) {
            return iterator;
        }else {
            stream = stream+ iterator;
            printf("bytestream progress %d bytes (%d total), %d remaining \n", iterator,stream, (size - stream));
        }
    }
    return stream;
}


void mySend(int sock, myMessage *message) {
    
    
    //alright, we first send a header
    //socket complains if ints arent unsigned so we gotta use uint32_t
    uint32_t flaggy = htonl((uint32_t) message->type);
    //now send to the socket
    if(flaggy < 0){
        //not sure what happened here
    }
    send(socket, (void *) &flaggy, sizeof(flaggy), 0);
    int mtype = (int )message->type;
    //decide whattttt youre trying to send with some ifs

    if(mtype == 1){ //HEARTBEAT
        //no body
        //break;
    }
    else if(mtype == 2){  //ACK NUM
        //socket doesnt like it if you dont send unsigned ints
        uint32_t macknum      = htonl((uint32_t) message->ackNum);
        uint32_t msequenceNum = htonl((uint32_t) message->seqNum);
        send(socket,(void * ) &msequenceNum, sizeof(msequenceNum), 0);
        send(socket,( void * ) &macknum, sizeof(macknum), 0);
        //break;
        
    }
    else if(mtype == 3){  //CONNECT
    
        uint32_t mnewSesh             = htonl((uint32_t) message->newSesh);
        uint32_t mlastReceivedMessage = htonl((uint32_t) message->lastReceivedMessage);
        send(socket,( void * ) &mnewSesh, sizeof(mnewSesh), 0);
        send(socket, ( void * ) &mlastReceivedMessage, sizeof(mlastReceivedMessage), 0);
        //break;
        
    }
    else if(mtype == 4){  //DATA
        //the big boy
        uint32_t macknum      = htonl((uint32_t) message->ackNum);
        uint32_t msequenceNum = htonl((uint32_t) message->seqNum);
        uint32_t mmessageSize   = htonl((uint32_t) message->messageSize);
        send(socket,(void * ) &mmessageSize, sizeof(mmessageSize), 0);
        send(socket, (void * ) &msequenceNum, sizeof(msequenceNum), 0);
        send(socket,(void * ) &macknum, sizeof(macknum), 0);
        send(socket, (void * ) message->payload, message->messageSize, 0);
        //break;
    }
    else{
            //not a known type and we're in trouble
            printf("HOUSTON: we have a problem in the message sender\n\n");
           // break;
    }
        
}// close mySend

myMessage * myRead(int socket){
    int status =0;
    int debugg = 2;
    uint32_t lastReceivedMessage;
    uint32_t newSesh;
    uint32_t messageSize;
    uint32_t seqNum;
    uint32_t inUse;
    uint32_t corrupt;
    uint32_t ackNum;
    //uint32_t *payload; 
    
    
    int mtype = 0;
    uint32_t mmtype;
    status = byteStream(socket, (char *) &mmtype, sizeof(uint32_t), 0);
    mtype = ntohl(mmtype);
    
    if(status == 0){
        return NULL;
    }
    if(mtype == 1){ //HEART
        myMessage *msg = malloc(sizeof(myMessage));
        msg->type =1;
        return msg;
        //break;
    }
    else if (mtype == 2){ //ACKNUM
         status = byteStream( socket, (char *) &seqNum, sizeof(uint32_t), 0);
        if(status <=0){
            return NULL;
        }
        status = byteStream( socket, (char *) &ackNum, sizeof(uint32_t) , 0);
        if(status <=0){
            return NULL;
        }
        uint32_t mseqNum;
        uint32_t mackNum;
        mseqNum = ntohl(seqNum);
        mackNum = ntohl(ackNum);
        myMessage *msg = malloc(sizeof(myMessage));
        msg->type =2;
        msg->ackNum =mackNum;
        msg->seqNum = mseqNum;
        return msg;
        //break;
        
    }
    else if(mtype == 3){ //CONNECT
        status = byteStream(socket, (char*) &newSesh, sizeof(uint32_t), 0);
        status = byteStream(socket, (char*) &lastReceivedMessage, sizeof(uint32_t), 0);
        if(status < 0){
            printf("problem reading connection string\n");
        }
        uint32_t mnewSesh;
        uint32_t mlastmsg;
        
        mnewSesh =  ntohl(newSesh);
        mlastmsg =  ntohl(lastReceivedMessage);
        myMessage * msg= malloc(sizeof(myMessage));
        msg->type = 3;
        msg->newSesh = mnewSesh;
        msg->lastReceivedMessage = mlastmsg;
        printf("A connection message was read\n");
        return msg;
        //break;
    }
    else if(mtype == 4){ //DATA
        status = byteStream( socket, (char *) &seqNum, sizeof(uint32_t), 0);
        if(status <=0){
            return NULL;
        }
        status = byteStream( socket, (char *) &messageSize, sizeof(uint32_t), 0);
        if(status <=0){
            return NULL;
        }
        
        status = byteStream( socket, (char *) &ackNum, sizeof(uint32_t), 0);
        if(status <=0){
            return NULL;
        }
        uint32_t mseqNum;
        uint32_t mMsgSize;
        uint32_t mackNum;
        mseqNum = ntohl(seqNum);
        mackNum = ntohl(ackNum); 
        mMsgSize = ntohl(messageSize); 

        char *payload = calloc(mMsgSize, sizeof(char));
        status = byteStream(socket, payload, mMsgSize, 0);
        
        if(status <= 0){
            printf("No payload in the payload message received/n");
            return NULL;
        }
        
        myMessage * msg= malloc(sizeof(myMessage));
        msg->type = 4;
        msg->seqNum = mseqNum;
        msg->messageSize = mMsgSize;
        msg->ackNum = mackNum;
        msg->payload = calloc(mMsgSize, sizeof(char));
        memcpy(msg->payload, payload, mMsgSize);  //there might be an issue with this 
        
        return msg;
       // break;
    }
    else if(mtype == 99){ //DEBUG TYPE
            printf("The debug message was received\n");
            return NULL;
            //break;
    }
    else{
        //sorry somethings gone wrong
        printf("The msg received has an unknown type\n");
        return NULL;
            //break;
    }
    
}//close myRead()

    
    
    
    
    
void tareItDown(){
    printf("taring it all down\n");
    close(clientListenSocket);
    close(cproxy);
    close(telnetSocket);
}
int setUpTelnet(){
    // Set up all things TELNET
    telnetSocket = socket(PF_INET, SOCK_STREAM, 0);
    if(telnetSocket == -1){
        printf( "Telnet socket creation failed\n");
        close(telnetSocket);
    }
    setsockopt(telnetSocket, SOL_SOCKET, SO_REUSEADDR, &dummy, sizeof(int));
    setsockopt(telnetSocket, SOL_SOCKET, SO_REUSEPORT, &dummy, sizeof(int));
    
     //actually open telnet connection
    int telnetConnectionOpen = connect(telnetSocket, (struct sockaddr *)&clientAdress, sizeof(clientAdress));
    if(telnetConnectionOpen < 0){
        printf("Telnet connection failed to open \n");
    }
    return 0;
}


int main(int argc, char * argv[]){
    
    
    int telPort23 = htons(23);
    char buffer[MAX_LINE];
    memset(buffer, 0, 1024);
    //---------set up structs and initial values
    struct hostent *sHostStruct = gethostbyname("localhost");
    

    socklen_t length;
    bzero((char*)&clientAdress, sizeof(clientAdress));
    fd_set incomingSocket;
    timeVal.tv_sec = 10;
    timeVal.tv_usec = 400000;
    
    
    
    //error check input
    if (argc < 2){
        printf("Input is no good. \n");
    }
    
    sPort = atoi(argv[1]);
    
    printf("listen on port: %d \n", sPort);
    
    
    
    // a couple more adresses
    clientAdress.sin_family = AF_INET;
    bcopy(sHostStruct->h_addr, (char *)&clientAdress.sin_addr, sHostStruct->h_length);
    clientAdress.sin_port = telPort23; //specified in spec
    
   
    
    //Begin client listening socket setup
    clientListenSocket = socket(PF_INET, SOCK_STREAM, 0);
    if(clientListenSocket < -1){
        printf("Client-Listening socket creation failed\n");
    }
    setsockopt(clientListenSocket, SOL_SOCKET, SO_REUSEADDR, &dummy, sizeof(int));
    setsockopt(clientListenSocket, SOL_SOCKET, SO_REUSEPORT, &dummy, sizeof(int));
    serverAdress.sin_addr.s_addr = INADDR_ANY;
    serverAdress.sin_family = AF_INET;
    serverAdress.sin_port = htons(sPort);
    printf("Client-Listening socket created\n");
    //bind and listen
    int sockBind = bind(clientListenSocket, (struct sockaddr*)&serverAdress, sizeof(serverAdress));
    int sockListen = listen(clientListenSocket, 5);
    
    if( (sockBind < 0) || (sockListen < 0) ){
        printf("bind &listen  failed \n");
    }
    //11111111114
    printf("Client-Listening bin& listen succeeded\n");
    
    //accept connection
    struct timeval lastHBsent;
    struct timeval lastHBreceived;
    gettimeofday(&lastHBsent, NULL);
    gettimeofday(&lastHBreceived, NULL);
    
    
    
    
    
    
    
    int maxIncoming; //(telnetSocket < cproxy) ? cproxy : telnetSocket;
    
    //alright lets do the listening and foward now
    
    while(1){
        //run forever until the input is invalid
        struct timeval TO;
        TO.tv_sec = 1;
        TO.tv_usec = 0;

        
        FD_ZERO(&incomingSocket);
        if(telnetSocket > 0){
          FD_SET(telnetSocket, &incomingSocket);
          maxIncoming = telnetSocket;
        }
        else{
            maxIncoming = -1;
        }
        //---------------------------------------
        if(cproxy >0){
            FD_SET(cproxy, &incomingSocket);
            maxIncoming = (maxIncoming < cproxy) ? cproxy : maxIncoming;
        }else{
            FD_SET(clientListenSocket, &incomingSocket);
             maxIncoming = (maxIncoming < clientListenSocket) ? clientListenSocket : maxIncoming;
        }
        
        
        
        rvVal= select((maxIncoming + 1), &incomingSocket, NULL, NULL, &TO);
        
        
        
        
        
        if(rvVal < 0){
            printf("Error in the incoming data. (rvVal Error)\n");
            tareItDown();
        }
        else if(rvVal == 0){
            // I think we got a timeout counter to increase
             //WE GOT A TIMEOUT PEOPLE
            //send the server a hb
             if(cproxy > 0) {
                printf("Timeout. Heartbeat being sent to server.\n");
                
                gettimeofday(&lastHBsent, NULL);
                
                myMessage *HBmsg =  malloc(sizeof(myMessage)); 
                HBmsg-> type = 1;
                mySend(cproxy, HBmsg);
                
                //check if we've REALLY timed out
                int differenceinHBs = lastHBsent.tv_sec - lastHBreceived.tv_sec;
                if(differenceinHBs >= 3){
                    printf("Timeout Occurred. connection cProxy has timed out./n");
                    //int state =failSwitch(s_host, sport);
                    close(cproxy);
                    cproxy = -1;
                }
            }
        }
        
        //--------------------We got one---------------------------
        else{
            struct timeval currTime;
            gettimeofday(&currTime, NULL);
            
            int diffCurrTime = (currTime.tv_sec - lastHBsent.tv_sec);
            if(diffCurrTime >= 1){ //weve lasted more than 1 sec
                gettimeofday(&lastHBsent, NULL);
                //make s HB message and send it
                myMessage *HBmsg2 =  malloc(sizeof(myMessage)); 
                HBmsg2-> type = 1;
                mySend(cproxy, HBmsg2);
            }
            //decide where the data is coming
            
            //the client listening socket has a message
            if(FD_ISSET(clientListenSocket, &incomingSocket)){
                printf("about to accept a socket\n");
                cproxy = accept(clientListenSocket, (struct sockaddr *)&serverAdress, &length);
            if(cproxy < 0){
                printf("Error: clientListenAcceptSocket accept failed\n");
                tareItDown();
                break;
                //printf("connection to cproxy failed to be established\n");
            }else if(cproxy == 0){
                printf("connection to cproxy failed to be established\n");
                break;
            }else{
                if(telnetSocket < 0){
                    printf("telnetSocket < 0");
                    int telStatus = setUpTelnet();
                    
                    //notify of new connection
                    if(telStatus == 0){
                        printf("Successfully opened new Telnet\n");
                    }
                    //1st connection message
                        myMessage *newConn = malloc(sizeof(myMessage)); 
                        newConn-> type = 3;
                        newConn-> newSesh = 1;
                        newConn-> lastReceivedMessage = currSeq;
                        mySend(cproxy, newConn);
                        continue;
                }
                //2nd connecti
                myMessage *newConnn = malloc(sizeof(myMessage)); 
                        newConnn-> type = 3;
                        newConnn-> newSesh = 0;
                        newConnn-> lastReceivedMessage = lastMsg;
                        mySend(cproxy, newConnn);
                        
                    gettimeofday(&lastHBreceived, NULL);
                    gettimeofday(&lastHBsent, NULL);
                    continue;
                }
                
            }
            //250000000
            
            //--------telnet had data-----
            
            if(FD_ISSET(telnetSocket, &incomingSocket)){
                //DAEMON WINS
                memset(buffer, 0, 1024);
                lengthPayload = recv(telnetSocket, buffer, 1024,0);
                printf("Data received from Telnet %s\n", buffer);
                if(lengthPayload <= 0){
                    //houston we have aproblem
                    break;
                    //close the ports
                }
                //FOWARD DATA to client
                //send(cproxy, (void *) buffer, lengthPayload, 0);
                //memset(buffer, 0, lengthPayload);
                myMessage * msgg = calloc(1, sizeof(myMessage));
                msgg-> type = 4;
                msgg-> seqNum = currSeq;
                msgg-> ackNum = knownAck;
                msgg-> messageSize = lengthPayload;
                msgg->payload = calloc(lengthPayload, sizeof(char));
                memcpy(msgg->payload, buffer, lengthPayload);
                mySend(cproxy, msgg);
                currSeq++;
                
            } //close daemons "if"
            
            
            if(FD_ISSET(cproxy, &incomingSocket)) {
                //the clients got shit
                printf("Receiving message from cProxy\n");
                myMessage *msgr = myRead(cproxy); 
             if(!msgr){
                 printf("message recieved null..timeout must have occurred\n");
                 //int state1 =failSwitch(s_host, sport);
                 //if(state1 != 0){
                     //printf("problem reconnecting line 485\n");
                 //}
                 close(cproxy);
                 cproxy =-1;
                 printf("Current cProxy connection ahs been shut down\n");
                 continue;
                }
             //figure out what type of message came in, now that we know its not null:
             int ttype = msgr->type;
             
             if(ttype == 1){  //HEARTBEAT
                 gettimeofday(&lastHBreceived, NULL);
                 //break;
                }
             else if(ttype == 2){ //ACKNUM
                 myMessage *ackS = malloc(sizeof(myMessage)); 
                  ackS-> type = 2; //ack type
                  int msq = msgr->seqNum;
                  printf("Ack received = %d, correct Ack expected = %d \n", msq, knownAck);
                  if( knownAck >= msq) {
                        knownAck = msq + 1;
                        //we're just forgetting the last ack
                    }
               
                }
             else if(ttype == 3){  //CONNECTION
             
                 if(msgr->newSesh == 1) {
                    lastMsg = msgr->lastReceivedMessage;
                    printf("Last received message starting = %d\n", lastMsg);
                } else {
                    //idk. we're losing messages somehow
                }
                
                     
                }
            
             else if(ttype == 4){  //DATA RECV
              int msq = msgr->seqNum;
              printf("Message received. Sequence Number = %d\n",msq);
              
              if(lastMsg == msq){
                  //send ack
                  myMessage *ackS = malloc(sizeof(myMessage)); 
                  ackS-> type = 2; //ack type
                  ackS-> seqNum = msq;
                  ackS-> ackNum = currSeq;
                  mySend(cproxy, ackS);
                  //ack sent
                  send(telnetSocket, msgr->payload, msgr->messageSize, 0);
                    lastMsg++;
              }else{
                  //something happened, we're out of order
                  printf("sequence number of the message received is out of order\n");
                  printf("we received seq num %d, and were expecting %d\n", msq, lastMsg);
              }
             }//close data recv
            
    

        } 
            
    } // close rvVal else
        
        
        //FD_CLR(telnetSocket, &incomingSocket);
        //FD_CLR(cproxy, &incomingSocket);
    }
    close(clientListenSocket);
    close(cproxy);
    close(telnetSocket);
    
}//close main