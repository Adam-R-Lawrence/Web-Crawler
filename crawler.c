#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include "crawler.h"
#include <regex.h>

queueForURLs *bottomStackPointer = NULL;
queueForURLs *topStackPointer = NULL;
int totalURLs = 0;


int main(int argc,char *argv[]) {

    /*
    if (argc == 1){
        printf("No URL was given\numberOfBytesRead");
        exit(EXIT_FAILURE);
    }*/

    int socketFD;
    char sendBuff[SEND_BUFFER_LENGTH] = {0};
    struct sockaddr_in serv_addr;
    char recvBuff[RECEIVED_BUFFER_LENGTH] = {0};
    char host[1024] = "www.columbia.edu";
    char url[1024] = "/~fdc/sample.html";
    struct hostent *server;




    int numberOfBytesRead;
    int total = 0;
    char *tail;
    int index;

    //Content Length Header
    char *contentLengthHeader;
    int contentLength = -2, cli;

    //Content Length Header
    char * pageStatusCode;
    int statusCode, sci;

    //Content Type Header
    char *contentTypeHeader;
    char contentType[100];
    int cti;

    int numberOfPagesFetched = 0;
    char URL[MAX_URL_SIZE + NULL_BYTE];

    //Check if command line URL is valid
    if (checkIfValidURL(host) == IS_VALID){
        enqueueURL(host);
        totalURLs++;
    } else {
        printf("Given URL is not Valid");
        exit(EXIT_FAILURE);
    }

    while(numberOfPagesFetched < 1 && bottomStackPointer != NULL) {
        int isHeader = TRUE;
        numberOfPagesFetched++;
        //dequeueURL(URL);

        //Create the socket
        socketFD = socket(AF_INET, SOCK_STREAM, 0);
        if(socketFD < NO_SOCKET_OPENED) {
            printf("Error with opening socket\n");
            exit(EXIT_FAILURE);
        } else {
            printf("Socket opened successfully.\n");
        }

        memset(&serv_addr, '0', sizeof(serv_addr)); //initialise server address
        memset(sendBuff, '0', sizeof(sendBuff)); //initialise send buffer


        //Details of the Server
        server = gethostbyname(host);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);
        memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

        //Connect to the desired Server
        if(connect(socketFD, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            printf("Error with connecting\n");
            exit(EXIT_FAILURE);
        } else {
            printf("Connected successfully\n");
        }

        //Print out URL to stdout
        printf("%s\n",url);

        //Send HTTP GET request to the server
        sprintf(sendBuff, HTTP_REQUEST_HEADER, url, host);
        if(send(socketFD, sendBuff, strlen(sendBuff), 0) < 0) {
            printf("Error with sending GET request\n");
            exit(EXIT_FAILURE);
        } else {
            printf("Successfully sent html fetch request\n");
        }

        //Receive the response from the server
        while (contentLength != total && (numberOfBytesRead = recv(socketFD, recvBuff, sizeof(recvBuff) - 1, 0)) > 0) {


            if (isHeader == TRUE) {
                tail = strstr(recvBuff, END_OF_HTTP_HEADER) + 4;

                //Content Type
                contentTypeHeader = strstr(recvBuff, CONTENT_TYPE);
                cti = (contentTypeHeader ? contentTypeHeader - recvBuff : -1) + strlen(CONTENT_TYPE);
                memcpy(contentType, &recvBuff[cti], 9);
                contentType[9] = '\0';
                if(strcmp(contentType,VALID_MIME_TYPE) != 0 || contentTypeHeader == NULL){
                    printf("\nNot VALID MIME TYPE\n");
                    break;
                }
                printf("\nContent-Type: %s\n",contentType);

                //Content Length
                contentLengthHeader = strstr(recvBuff, CONTENT_LENGTH) + strlen(CONTENT_LENGTH);
                cli = (contentLengthHeader ? contentLengthHeader - recvBuff : -1);
                contentLength = atoi(&(recvBuff[cli]));
                printf("\nContent-Length: %d\n", contentLength);
                //printf("\n\n\n%s\n\n\n", recvBuff);

                //Status Code
                pageStatusCode = strstr(recvBuff, STATUS_CODE) + strlen(STATUS_CODE);
                sci = (pageStatusCode ? pageStatusCode - recvBuff : -1);
                statusCode = atoi(&(recvBuff[sci]));
                printf("\nStatus-Code: %d\n", statusCode);

                if (tail != NULL) {
                    //tail = tail -2;
                    isHeader = FALSE;
                    index = (tail ? tail - recvBuff : -1);

                    total += numberOfBytesRead - index;

                    //printf ("%s", &(recvBuff[index]));
                    //memset(recvBuff,0,strlen(recvBuff));
                    continue;
                }
            }

            parseHTML(recvBuff);
            //printf("%s", recvBuff);

            total += numberOfBytesRead;
            memset(recvBuff, 0, strlen(recvBuff));

        }
    }
    printf("\nContent Length: %d, Total URL's: %d\n",total,totalURLs);

    printStack();
    return 0;
}


int parseHTTPHeaders(char buffer[]) {


    return 1;
}

void parseHTML(char buffer[])
{
    int si, ei, URLLength;
    char * startURL;
    char * endURL;
    char possibleURL[MAX_URL_SIZE + NULL_BYTE];



    while ((startURL = strstr(buffer, "<a href=")) != NULL) {
        startURL = &(startURL[9]);
        si = (startURL ? startURL - buffer : -1);

        endURL = strstr(startURL, "\"");
        if(endURL != NULL) {
            ei = (endURL ? endURL - buffer : -1);

            URLLength = ei - si;

            memcpy(possibleURL, &buffer[si], URLLength);
            possibleURL[URLLength] = '\0';

            if (checkIfValidURL(possibleURL) == IS_VALID) {
                enqueueURL(possibleURL);
                getHost(possibleURL);
                totalURLs++;
            }


            memset(possibleURL, 0, strlen(possibleURL));
        }





        buffer = startURL;
    }

}

int checkIfValidURL(char possibleURL[])
{
    int validURL;

    /* A valid URL's host should match the host of the given command line URL
     *
     *
     */


    //A valid URL cannot be over 1000 bytes long
    if (strlen(possibleURL) > MAX_URL_SIZE)
    {
        return validURL = NOT_VALID;
    }

    //A valid URL contains no Exclamation Mark
    if (strchr(possibleURL, EXCLAMATION_MARK) != NULL)
    {
        return validURL = NOT_VALID;
    }

    //A valid URL contains no Hashtag
    if (strchr(possibleURL, HASHTAG) != NULL)
    {
        return validURL = NOT_VALID;
    }

    //A valid URL contains no URL Encoded Characters
    if (strchr(possibleURL, URL_ENCODED_CHARACTER) != NULL)
    {
        return validURL = NOT_VALID;
    }


    return validURL = IS_VALID;
}

/*
 *  A function that returns the host of a given URL
 */
char * getHost(char URL[]){

    char host[1000];

    char * startOfHostName;
    /*
    int sohni;

    startOfHostName = strstr(URL, "http://");
    if (startOfHostName != NULL) {
        sohni = (startOfHostName ? startOfHostName - URL : -1) + strlen(CONTENT_TYPE);
        memcpy(host, &URL[sohni], 9);
        host[9] = '\0';
    }

     */

    sscanf( URL, "%*[^/]%*[/]%[^/]", host);
    printf("\n%s\n",host);

    return "Hello";


}

void dequeueURL(char *URL){
    if(bottomStackPointer == NULL){
        return;
    }
    strcpy(URL,bottomStackPointer->data);
    queueForURLs *TempPointer = bottomStackPointer;
    bottomStackPointer = bottomStackPointer->nextNode;
    free(TempPointer);

    if(bottomStackPointer == NULL){
        topStackPointer = NULL;
    }

}

void enqueueURL(char *URL){
    queueForURLs *TempPointer = malloc(sizeof(queueForURLs));

    if(TempPointer == NULL){
        return;
    }
    strcpy(TempPointer->data,URL);
    TempPointer->nextNode = NULL;

    if(bottomStackPointer == NULL)
    {
        TempPointer = bottomStackPointer = TempPointer;
    }
    else
    {
        topStackPointer->nextNode = TempPointer;
    }
    topStackPointer = TempPointer;

}

void printStack(void){
    printf("***List of URL's***\n");
    printf("**************\n");
    int numberOfNodes = 0;
    queueForURLs *TempPointer = bottomStackPointer;
    while(TempPointer != NULL){
        numberOfNodes++;
        printf("%s\n",TempPointer->data);
        TempPointer = TempPointer->nextNode;
    };
    printf("**************\n");
    printf("Counted %d URL's\n",numberOfNodes);
    printf("***List of URL's***\n");
}
