#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include "crawler.h"
#include <regex.h>


int main(int argc,char *argv[]) {

    /*
    if (argc == 1){
        printf("No URL was given\n");
        exit(EXIT_FAILURE);
    }*/
    int socketfd = 0;
    char sendBuff[1025] = {0};
    struct sockaddr_in serv_addr;
    char recvBuff[1024] = {0};
    //int port_number = 80;
    char host[1024] = "www.columbia.edu";
    char url[1024] = "/~fdc/sample.html";
    struct hostent *server;




    socketfd = socket(AF_INET, SOCK_STREAM, 0); //create socket
    if(socketfd < 0) {
        printf("ERROR opening socket\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Socket opened successfully.\n");
    }

    memset(&serv_addr, '0', sizeof(serv_addr)); //initialise server address
    memset(sendBuff, '0', sizeof(sendBuff)); //initialise send buffer

    server = gethostbyname(host);

    //Create a Socket
    serv_addr.sin_family = AF_INET; //Type of address – internet IP
    serv_addr.sin_port = htons(PORT); //Listen on port 80
    memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

    //Connect
    if(connect(socketfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Can't connect\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Connected successfully\n");
    }

    //Send HTTP GET request
    sprintf(sendBuff, HTTP_REQUEST_HEADER, url, host);


    if(send(socketfd, sendBuff, strlen(sendBuff), 0) < 0) {
        printf("Error with send()");
    } else {
        printf("Successfully sent html fetch request\n");
    }


    int n;
    while ( (n = recv(socketfd, recvBuff, sizeof(recvBuff) - 1 ,0) ) > 0)
    {
            //process received buffer
    printf("%s", recvBuff);
    bzero(recvBuff, 1025);

    }


    return 0;
}

void fetchWebPage(char URL[], int numberOfPagesFetched)
{
    if (numberOfPagesFetched >= MAX_NUMBER_OF_PAGES_FETCHED){
        exit(EXIT_SUCCESS);
    } else {
        numberOfPagesFetched++;
    }


}

void parseHTTPHeaders(int socketFD)
{

}

void parseHTML(int socketFD)
{

}

void checkIfValidURL(char URL[])
{

}