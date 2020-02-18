#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<inttypes.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<ifaddrs.h>
#include<sys/stat.h>

#define TRANSFER_BUFFER_SIZE 1024

struct sockaddr_in serverAddress;

void print(char*);
int connectToServer(const char*, const char* );

void print(char* s)
{
    printf("\n%s", s);
}

int connectToServer(const char* ip, const char* port)
{
    //Create a socket
    int servSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(servSocket < 0)
    {
        print("Could not create socket");
        return -1;
    }

    //Set the IP address and port for the connection
    if(strcmp(ip, "localhost") == 0)
    {
        serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    }
    else
    {
        serverAddress.sin_addr.s_addr = inet_addr(ip);
    }
    if(serverAddress.sin_addr.s_addr == 0)
    {
        print("The IP address is not valid");
        return -1;
    }
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(atoi(port));

    if(serverAddress.sin_port == 0)
    {
        print("The port is not valid");
        return -1;
    }
    
    //Try to connect to the server
    if(connect(servSocket, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr)) < 0)
    {
        print("Can't connect to the server, check the address and port");
        print("");
        return -1;
    }
    return servSocket;
}



int main(int argc, char const *argv[])
{
    int serverSocket;
    
    //Check if there were passed enough arguments
    //!!-- Data validation not done yet for the arguments, try to pass valid arguments --!!
    if (argc < 4)
    {
        print("Run the application with \"./client <ip> <port> <file>\"");
        print("");
        return 1;
    }

    serverSocket = connectToServer(argv[1], argv[2]);
    char* reqFileName = (char*)argv[3];  //String for storing the requested file name

    if (serverSocket < 0)
    {
        return 1;
    }

    //Send the file name to the server
    if(send(serverSocket, reqFileName, strlen(reqFileName), 0) != strlen(reqFileName))
    {
        print("Sent a different number of bytes than expected, oops....crashing");
        print("");
        return -1;
    }
    printf("\nRequested the file %s", reqFileName);
    
    //Get the file size at first
    int count;
    char fileS[10];
    count = recv(serverSocket, fileS, 10, 0);
    if(count < 0)
    {
        print("Did not receive the file size from the server");
        print("");
        return -1;
    }
    int fileSize = atoi(fileS);
    if(fileSize == -1)
    {
        // -1 is the file not found message
        printf("\nThe file %s is not present on the server\n", reqFileName);
        return -1;
    }
    printf("\nThe file size is %d bytes, waiting for transfer...", fileSize);

    //if we're here, we got a valid file
    //transfer the file in chunks of TRANSFER_BUFFER_SIZE bytes each
    char RxBuffer[TRANSFER_BUFFER_SIZE];
    uint32_t noChunks = (fileSize / TRANSFER_BUFFER_SIZE) + (fileSize % TRANSFER_BUFFER_SIZE != 0);

    //create the file to be received in the folder "received"
    char path[50];
    if (mkdir("received", 0775) == -1)
    {
        chmod("received", 0775);
    }
    sprintf(path, "received/%s", reqFileName);
    FILE *f = fopen(path, "w+");

    //keep track of the number of received bytes in each chunk
    int receivedBytes = 0;
    uint8_t RxOK = 0;
    char receivedBytesString[20];
    for(uint32_t i=1; i<=noChunks; i++) //download the chunks
    {
        //try to receive the correct data for each chunk; todo: some kind of sumcheck for data integrity
        RxOK = 0;
        while(!RxOK)
        {
            RxOK = 0;
            receivedBytes = recv(serverSocket, RxBuffer, TRANSFER_BUFFER_SIZE, 0);
            printf("\nReceived a chunk with %d bytes", receivedBytes);
            sprintf(receivedBytesString, "%d", receivedBytes);
            send(serverSocket, receivedBytesString, strlen(receivedBytesString), 0);
            //check if we received the right amount of data
            if((i < noChunks && receivedBytes == TRANSFER_BUFFER_SIZE) || (i == noChunks && receivedBytes == fileSize % TRANSFER_BUFFER_SIZE)) 
            {
                RxOK = 1;
            }
            else
            {
                printf("...retrying");
            }
            
        }
        
        fwrite(RxBuffer, sizeof(char), receivedBytes, f); //write the received data to the disk
    }

    //all done, close the file and set permissions
    fclose(f);
    chmod(path, 0775);

    print("Transfer complete, exiting.");
    printf("\nYou'll find \"%s\" in the received folder\n", reqFileName);

    //close the connection
    shutdown(serverSocket, 2);
    return 0;
}
