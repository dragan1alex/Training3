#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<inttypes.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#include<pthread.h>

#define TRANSFER_BUFFER_SIZE 1024

uint32_t fsize(FILE*);
void print(char*);
uint8_t initializeServer(int, uint16_t);
void * handleRequest(void*);

//get the file size
uint32_t fsize(FILE *fp){
    fseek(fp, 0L, SEEK_END);
    uint32_t sz=ftell(fp);
    fseek(fp,0,SEEK_SET);
    return sz;
}

//for debug purposes, print on a new line
void print(char* s)
{
    printf("\n%s", s);
}

//initialize the socket to accept incoming connections on the specified port
uint8_t initializeServer(int sid, uint16_t port)
{
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = INADDR_ANY;
    if(bind(sid, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    {
        print("Bind failed");
        printf("\nTry running the server as root or chose a different port\n");
        return 0;
    }
    print("Bind successful");
    return 1;
}


//the request handling thread function
void * handleRequest(void * rS)
{
    int replySocket = *(int *)rS;
    uint8_t abortConnection = 0;
    int  recBytes;
    char fileName[30];
    FILE *f;

    //get the file name the client wants
        recBytes = recv(replySocket, fileName, 30, 0);
        if(recBytes < 0)
        {
            print("Received garbage from the client, dropping the connection...");
            shutdown(replySocket, 2);
            pthread_exit(NULL);
        }
        printf("\nReceived a request for %s ", fileName);
        fileName[recBytes] = '\0';
        //try to open the file
        f = fopen(fileName, "r");
        if(f == NULL)
        {
            send(replySocket, "-1", 2, 0);
            print("File not found, closing the connection.");
            print("");
            shutdown(replySocket, 2);
            pthread_exit(NULL);
        }
        
        /*file size*/
        uint32_t fs = fsize(f);
        /*bytes read from file*/
        uint32_t readBytes;
        char buffer[TRANSFER_BUFFER_SIZE];
        sprintf(buffer, "%u", fs);
        printf("\nFile found, sending %u bytes...", fs);


        /*send the file size to the client*/
        send(replySocket, buffer, strlen(buffer), 0);
        strcpy(buffer, "");

        /*send the file in chunks of TRANSFER_BUFFER_SIZE bytes*/
        uint32_t noChunks = (fs / TRANSFER_BUFFER_SIZE) + (fs % TRANSFER_BUFFER_SIZE != 0);
        /*the number of bytes the client received*/
        uint32_t bytesReceived; char clientBytesReceivedString[20];
        /*transmission ok flag*/
        uint8_t TxOK = 0;
        for(uint32_t i = 1; i<= noChunks && !abortConnection; i++)
        {
            readBytes = fread(buffer, sizeof(char), TRANSFER_BUFFER_SIZE, f);
            if(readBytes)
            {
                TxOK = 0;
                while (!TxOK && !abortConnection)
                {
                    printf("\nSent %lu bytes...", send(replySocket, buffer, readBytes, 0));
                    recBytes = recv(replySocket, clientBytesReceivedString, 20, 0);
                    if(recBytes == -1)
                    {
                        abortConnection = 1;
                    }
                    clientBytesReceivedString[recBytes] = '\0';
                    bytesReceived = atoi(clientBytesReceivedString);
                    if(readBytes == bytesReceived)
                    {
                        TxOK = 1;
                        printf("done");
                    }
                    else
                    {
                        printf("failed(client received %d). Retrying...", bytesReceived);
                    }
                }
            }
        }
        /*close the file and the connection*/
        fclose(f);
        shutdown(replySocket, 2);
        pthread_exit(NULL);
}

pthread_t lastThread;
int main(int argc, char const *argv[])
{
    int sockid, replySocket, c;
    struct sockaddr_in client;
    uint16_t port;

    /*check if there's a port mentioned in the arguments*/
    if(argc > 1)
    {
        print("Received port: ");
        printf("%s", argv[1]);
        port = atoi(argv[1]);
    }
    else
    {
        print("Did not receive a port to open. Open the app with \"./server <port>\"");
        print("");
        return -1;
    }
    /*open a socket to accept incoming connections */
    sockid = socket(PF_INET, SOCK_STREAM, 0);
    if(sockid == -1)
    {
        print("Can't create a socket, exiting...");
        return 0;
    }
    print("Socket created.");

    /*initialize it with the port */
    if(!initializeServer(sockid, port))
    {
        return 1;
    }
    
    /*listen on the socket*/
    listen(sockid, 100);
    print("Initialization complete, to stop the server press CTRL+C (^C)");
    print("");
    print("-------------------------------------");
    print("---Now running in an infinite loop---");
    print("-------------------------------------");
    
    /*check for incomming connections and handle them */
    while (1)
    {
        print("Waiting for a connection...");
        c = sizeof(struct sockaddr_in);

        /*open a new socket to talk to the client */
        replySocket = accept(sockid, (struct sockaddr*)&client, (socklen_t*)&c);
        if(replySocket < 0)
        {
            print("Can't create a connection to the client...");
            continue;
        }
        print("Connection accepted.");
        /*Handle the client in a new thread*/
        pthread_create(&lastThread, NULL, handleRequest, (void *) &replySocket);
        print("Connection closed.");
        printf("\nListening on port %d", port);
        print("");
    }
    return 0;
}
