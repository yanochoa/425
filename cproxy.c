/*
.Author: Yan Ochoa (No partner)
 CSC 425 Milestone 2
 client proxy restart
  * this is me hail marying at 10:11 4/26
 */

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <sys/select.h>
#include <netdb.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_LINE 1024
#define HEARTBEAT 1
#define ACKNUM    2
#define CONNECT   3
#define DATA      4

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

//MR WORLD WIDES
 int knownAck = 1;
 int currSeq = 1;  //
 int connectionClient =-1;
 int serverConnection;
 int almostReady = 0;
 int serverSocket = -1;
 int lastMsg = -1;
 int clientListenSocket =-1;
 int dummy = 1;



int byteStream(int socket, char *buff, int size, int incore) {
    int stream = 0;
    int in = incore;
    bool ready = false;
    int working = 1;
    int iterator = 0;
    int debugg =1;
    if(incore ==1){
        //printf("Incore was set");
    }

    while((size - stream) > 0) {
        iterator = read(socket, (buff + stream), (size - stream));
        if( iterator <= 0) {
            working =0;
            return iterator;
        }else {
            working = 1;
            stream = stream+ iterator;
            printf("Bytestream receiving progress %d bytes -- %d total.\n", iterator,stream);
        }
    }
    return stream;
}


void mySend(int socket, myMessage *message) {
    
    if(!message) {
        printf("mySend got a null message\n");
        return;
    }
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
            printf("No payload in the payload message received\n");
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
       
    }
    else if(mtype == 99){ //DEBUG TYPE
            printf("The debug message was received\n");
            return NULL;
     
    }
    else{
        //sorry somethings gone wrong
       // printf("The msg received has an unknown type\n");
        return NULL;
            
    }
    
}//close myRead()

int failSwitch(char * s_host, int sport){
    close(serverSocket);
    serverSocket = -1;
    //printf("Disconnect Occurred\n");
   // printf("Re-connect protocol initiated\n");
    serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &dummy, sizeof(int));
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &dummy, sizeof(int));
    serverConnection = -1;
    //printf("new server socket created\n");
    struct hostent *hostess = gethostbyname(s_host);
    
    struct sockaddr_in cAdd;
    bzero((char *)&cAdd, sizeof(cAdd));
    cAdd.sin_family = AF_INET;
    
    bcopy(hostess->h_addr, (char *)&cAdd.sin_addr, hostess->h_length);
    cAdd.sin_port = htons(sport);
    
     serverConnection = connect(serverSocket, (struct sockaddr *) &cAdd, sizeof(cAdd));
     if(serverConnection < 0){
         //printf("Trouble in failSwitch, couldnt open new server Connection\n");
         return -1;
     }
     //notify mailbox
     myMessage *newConMsg = malloc(sizeof(myMessage));
     newConMsg->type = 3;
     newConMsg ->newSesh = 0;
     newConMsg ->lastReceivedMessage = lastMsg;
     mySend(serverSocket, newConMsg);
     //printf("re-connect protocol succeeded.\n");
     return 0;
}


// 1st, start cproxy to listen on TCP port 5200
// (cproxy 5200 sip 6200)
// 2nd telnet to cproxy
// (telnet localhost 5200)
int main(int argc, char * argv[]){
    
    
    int telnetConnection;
    int lport;
    int sport;
    char *s_host;
    char buffer[MAX_LINE];
    memset(buffer, 0 , 1024);
    int bind_;
    int listen_;
    int maxIncoming;
    int lengthPayload = -1;
    int receivingValue;
    
    socklen_t len_sock;
    
    struct sockaddr_in clientAdress;
    bzero((char *)&clientAdress, sizeof(clientAdress));
    clientAdress.sin_family = AF_INET;
    struct timeval timeValue;
    struct timeval lastHBsent;
    struct timeval lastHBreceived;
    
    
    struct sockaddr_in listenAdress;
    listenAdress.sin_family = AF_INET;
    listenAdress.sin_addr.s_addr = INADDR_ANY;
    
    fd_set socketIncoming;
    timeValue.tv_sec = 10;
    timeValue.tv_usec = 450000;
    
    
    
    //------------------------setup end----------------------------
    
    
    //error check user input
    if(argc != 4){
        printf("Invalid input\n");
        printf("Correct input consists of: ./cproxy listen_port server_ip server_port\n");
               return 0;
               }
               
               //parse user input
               lport   = atoi(argv[1]);
               s_host = argv[2];
               sport   = atoi(argv[3]);
               
               //int lport_htons = htons(lport);
               //printf("lport: %d \n", lport);
               //printf("%s\n", s_host);
               
               
               
               //printf("User input has been parsed\n");
               
               //socket for client listening
               //-------CLIENTLISTENSOCKET---------
               clientListenSocket = socket(PF_INET, SOCK_STREAM, 0);
               if(clientListenSocket < 0){
                   printf("clientListenSocket creation failed\n");
               }
               setsockopt(clientListenSocket, SOL_SOCKET, SO_REUSEADDR, &dummy, sizeof(int));
               setsockopt(clientListenSocket, SOL_SOCKET, SO_REUSEPORT, &dummy, sizeof(int));
               //set_socket opts
               //clientAdress.sin_port = htons();
               int lport_htons = htons(lport);
               listenAdress.sin_port = lport_htons;
               //156
               
               //bind and listen
           bind_ = bind(clientListenSocket, (struct sockaddr *)&listenAdress, sizeof(listenAdress));
           listen_ = listen(clientListenSocket, 7);
           if((bind_ < 0) || (listen_ < 0)){
           printf("bind and listen failed\n");
           }
           printf("waiting for a Telnet client connection..\n");
           connectionClient = accept(clientListenSocket,(struct sockaddr *)&listenAdress, &len_sock);
            if(connectionClient < 0){
              printf("Cant create connection to client\n");
            }  else if(connectionClient == 0){
                printf("connection client was 0 \n");
                //not super sure
                printf("ConnectionClient == 0 \n ");
                } else{
                    //NOOOOW we wanna open sproxy
                    serverSocket = socket(PF_INET, SOCK_STREAM, 0);
                    if(serverSocket < 1){
                     printf("serverSocket creation failed\n");
                    }
                    
                }
                setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &dummy, sizeof(int));
                setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &dummy, sizeof(int));
                //connect to the server with user inputs
                    struct hostent *specifiedHost = gethostbyname(s_host);
                    if(!specifiedHost){
                        printf("The Host IP is not online\n");
                    }
         printf("Ready to connect\n");      
               
         bcopy(specifiedHost->h_addr, (char *)&clientAdress.sin_addr, specifiedHost->h_length);
         clientAdress.sin_port = htons(sport); //passed in as arg[3]
         
          //printf("conencted stuff\n");
          serverConnection = connect(serverSocket, (struct sockaddr *)&clientAdress, sizeof(clientAdress));
          if(serverConnection < 0){
           printf("server connection(sproxy)  failed\n");
            return -1;
           }
                                                    
           
           //printf("Ready to connect\n");
                                                                                                                              
           
                                         
           printf("Connection established\n");
        
        //accept data and foward it
        //maxIncoming= (serverSocket < connectionClient) ? connectionClient : serverSocket;
        //new message for connection. MOVE
        myMessage *newConn = malloc(sizeof(myMessage)); 
        newConn-> type = 3;
        newConn-> newSesh = 1;
        newConn-> lastReceivedMessage = 1;
        mySend(serverSocket, newConn);
                
        gettimeofday(&lastHBsent, NULL);
        gettimeofday(&lastHBreceived, NULL);
                  
        //forever until -1
        while(1){
            struct timeval TO;  //time ouuuuuuut
            TO.tv_usec = 0;
            TO.tv_sec = 1;
            bzero(buffer, 1024);
            //printf("while loop top\n");
           
            //set the options for set
            FD_ZERO(&socketIncoming);
            
            if(serverSocket >0){
                FD_SET(serverSocket, &socketIncoming);
            }
            
            if(connectionClient > 0){
                FD_SET(connectionClient, &socketIncoming);
                maxIncoming= (serverSocket < connectionClient) ? connectionClient : serverSocket;
            }
        
                                             
            receivingValue = select(maxIncoming +1,&socketIncoming, NULL, NULL, &TO);
                                             
        if(receivingValue == -1){
        //something went wrong
        printf("Something went wrong with the receiving of data\n");
        printf("Something went wrong with the receiving of data\n");
         //breakit all down
         }
         else if(receivingValue == 0){
            //WE GOT A TIMEOUT PEOPLE
            //send the server a hb
             if(serverSocket > 0) {
                printf("Heartbeat being sent to server.\n");
                
                gettimeofday(&lastHBsent, NULL);
                
                myMessage *HBmsg =  malloc(sizeof(myMessage)); 
                HBmsg-> type = 1;
                mySend(serverSocket, HBmsg);
                
                //check if we've REALLY timed out
                //int differenceinHBs = lastHBsent.tv_sec - lastHBreceived.tv_sec;
                if(lastHBsent.tv_sec - lastHBreceived.tv_sec >= 3){
                    printf("Timeout Occurred/n");
                    int state =failSwitch(s_host, sport);
                    if(state != 0){
                        printf("the failSwitch failed\n");
                    }
                    gettimeofday(&lastHBreceived, NULL);
                    gettimeofday(&lastHBsent, NULL);
                } 
            }
         }
    else{
         //select a path
         //but before that, send HB
        // printf("we went to the big else\n");
         struct timeval HBnow;
         gettimeofday(&HBnow, NULL);
         int diff =  HBnow.tv_sec - lastHBsent.tv_sec;
         if(diff >= 1){
             gettimeofday(&lastHBsent, NULL);
             myMessage *HBmsg1 =  malloc(sizeof(myMessage)); 
             HBmsg1-> type = 1;
             mySend(serverSocket, HBmsg1);
         }
        //-----------------------------------------------------
                           
            
            if(FD_ISSET(serverSocket, &socketIncoming)){
            //if this entered then we are receiving data from server
            //lengthPayload= recv(serverSocket, buffer, 1024, 0);
             myMessage *msgr = myRead(serverSocket); 
             if(!msgr){
		int out =0 ;
                 //printf("message recieved null..\n");
                while(out == 0){ 
		int state1 =failSwitch(s_host, sport);
                 	if(state1 != 0){
                     	printf("New IP has not been aquired yet..\n");
                 	}
			else{
				out =1;
			}
		
                 }
		printf("Connection stable\n");
		continue;
             }
             //figure out what type of message came in, now that we know its not null:
             int ttype = msgr->type;
             
             if(ttype == 1){  //HEARTBEAT
             printf("MSGTYPE: Heart Beat\n");
                 gettimeofday(&lastHBreceived, NULL);
                 //break;
             }
             else if(ttype == 2){ //ACKNUM
             printf("MSGTYPE: Ack Num\n");
                 myMessage *ackS = malloc(sizeof(myMessage)); 
                  ackS-> type = 2; //ack type
                  int msq = msgr->seqNum;
                  printf("Ack received =  %d, expected ack = %d \n", msq,knownAck);
                  if(knownAck >= msq) {
                        knownAck = msq + 1;
                                //remove messages waiting in a queue?
                                //not sure what to do 
                    }
                //break;
                  
             }
             else if(ttype == 3){  //CONNECTION
             printf("MSGTYPE: Connection Message\n");
                 if(msgr->newSesh == 1) {
                    lastMsg = msgr->lastReceivedMessage;
                    printf("Last received message starting = %d\n", lastMsg);
                } else {
                    //idk. we're losing messages somehow
                }
                
                     
            }
             else if(ttype == 4){  //DATA RECV
             printf("MSGTYPE: Data Message\n");
              int msq = msgr->seqNum;
              printf("Message received. Sequence Number = %d\n",msq);
              
              if(lastMsg == msq){
                  //send ack
                  myMessage *ackS = malloc(sizeof(myMessage)); 
                  ackS-> type = 2; //ack type
                  ackS-> seqNum = msq;
                  ackS-> ackNum = currSeq;
                  mySend(serverSocket, ackS);
                  //ack sent
                  printf("--about to foward data from message received\n");
                  send(connectionClient, msgr->payload, msgr->messageSize, 0);
                    lastMsg++;
              }else{
                  //something happened, we're out of order
                  printf("sequence number of the message received is out of order\n");
                  printf("we received seq num %d, and were expecting %d\n", msq, lastMsg);
              }
              //break;
                //----------type data end--------------
             }
             else{
                 
             }
             
             
             //printf("Data read from server: %s\n", buffer);
             //if(lengthPayload <= 0){
             //its over
             //break;
             //}
             //foward it to the client
            
            //send(connectionClient, (void *) buffer, 1024, 0);
            //memset(buffer, 0, lengthPayload);
                
            //fetch next piece of data
            }
        //--------------------------------------------------------
                     
                     
            //other option, client is where data is coming in from
        if((connectionClient >= 0) && FD_ISSET(connectionClient, &socketIncoming)){
            
            lengthPayload = recv(connectionClient, buffer, 1024, 0);
            printf("Data read from client: %s", buffer);
            if(lengthPayload <= 0){
              //its over
              printf("length of the payload was 0\n ** closing\n");
               break;
            }//close error if
                         
             if(strcmp("exit", buffer) ==0){
              printf("Closing connection \n");
              goto DONE;
              }
            
            myMessage * msgg = calloc(1, sizeof(myMessage));
            msgg-> type = 4;
            msgg-> seqNum = currSeq;
            msgg-> ackNum = knownAck;
            msgg-> messageSize = lengthPayload;
            msgg->payload = calloc(lengthPayload, sizeof(char));
            memcpy(msgg->payload, buffer, lengthPayload);
            //printf("could be a problem here: \n\n");
            printf("what we read from the buffer: %s", msgg->payload);
            mySend(serverSocket, msgg);
            currSeq++;
              //send(serverSocket, (void *) buffer, lengthPayload, 0);
              //memset(buffer, 0, lengthPayload);
        }//close if connectionClient
                     
    }//close else (selectPath)
                    
                    
                    
    }//close while loop
             DONE:
             close(connectionClient);
             close(serverSocket);
             close(clientListenSocket);
                
}//close main