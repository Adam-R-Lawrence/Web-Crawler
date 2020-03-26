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

    int socketfd;
    char sendBuff[1025] = {0};
    struct sockaddr_in serv_addr;
    char recvBuff[30000] = {0};
    char host[1024] = "www.columbia.edu";
    char url[1024] = "/~fdc/sample.html";
    struct hostent *server;

/*
    push(host);
    push(host);
    push("Bye");
    enqueueURL("Hello");
    printStack();
*/

    //Check if given URL is valid
    if (checkIfValidURL(host) == IS_VALID){
        enqueueURL(host);
    }

    printStack();

    int numberOfBytesRead;
    int total = 0;
    int isheader = 0;
    char *tail;
    char * ahrefTail;
    int index;
    char *contentLengthHeader;
    int contentLength = -2, cli;
    int numberOfPagesFetched = 0;
    char URL[1000];

    while(numberOfPagesFetched < 1 && bottomStackPointer != NULL) {

        numberOfPagesFetched++;
        //dequeueURL(URL);

        //Create the socket
        socketfd = socket(AF_INET, SOCK_STREAM, 0);
        if(socketfd < NO_SOCKET_OPENED) {
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
        if(connect(socketfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            printf("Error with connecting\n");
            exit(EXIT_FAILURE);
        } else {
            printf("Connected successfully\n");
        }

        //Send HTTP GET request to the server
        sprintf(sendBuff, HTTP_REQUEST_HEADER, url, host);
        if(send(socketfd, sendBuff, strlen(sendBuff), 0) < 0) {
            printf("Error with sending GET request\n");
            exit(EXIT_FAILURE);
        } else {
            printf("Successfully sent html fetch request\n");
        }

        //Receive the response from the server
        while (contentLength != total && (numberOfBytesRead = recv(socketfd, recvBuff, sizeof(recvBuff) - 1, 0)) > 0) {


            if (isheader == 0) {
                tail = strstr(recvBuff, END_OF_HTTP_HEADER) + 4;
                contentLengthHeader = strstr(recvBuff, CONTENT_LENGTH) + strlen(CONTENT_LENGTH);
                cli = (contentLengthHeader ? contentLengthHeader - recvBuff : -1);
                contentLength = atoi(&(recvBuff[cli]));
                printf("\n\n\n%d\n\n\n", contentLength);
                //printf("\n\n\n%s\n\n\n", recvBuff);

                if (tail != NULL) {
                    //tail = tail -2;
                    isheader = 1;
                    index = (tail ? tail - recvBuff : -1);

                    total += numberOfBytesRead - index;

                    //printf ("%s", &(recvBuff[index]));
                    //memset(recvBuff,0,strlen(recvBuff));
                    continue;
                }
            }


            /*
            ahrefTail = strstr(recvBuff, "<a href=") + 8;
            if( ahrefTail != NULL) {
                ali = (ahrefTail ? ahrefTail - recvBuff : -1);
                printf("\n\n\n\n%c\n\n\n\n", recvBuff[ali]);
                //strncpy(substr, recvBuff[ali], 100);
            }
            */



            parseHTML(recvBuff);
            //printf("%s", recvBuff);

            total += numberOfBytesRead;
            memset(recvBuff, 0, strlen(recvBuff));

        }
    }
    printf("\n\n\n\n%d,%d\n\n\n\n",total,totalURLs);

    printStack();
    return 0;
}

void fetchWebPage(char URL[], int numberOfPagesFetched)
{
    char possibleURL[1024];




    if(checkIfValidURL(possibleURL) == IS_VALID){
        enqueueURL(possibleURL);
    }


}

int parseHTTPHeaders(char buffer[]) {


    return 1;
}

void parseHTML(char buffer[])
{
    int si, ei, URLLength;
    char * startURL;
    char * endURL;
    char possibleURL[MAX_URL_SIZE + 1];



    while ((startURL = strstr(buffer, "<a href=")) != NULL) {
        startURL = &(startURL[9]);
        si = (startURL ? startURL - buffer : -1);
       // printf("Start: %c\n", buffer[si]);
        //printf("\n\n\n\n%s\n\n\n\n", startURL);

        endURL = strstr(startURL, "\"");
        ei = (endURL ? endURL - buffer : -1);
        //printf("End: %c\n", buffer[ei]);

       // printf("URL: ");

        URLLength = ei - si;
        //printf("%d\n",URLLength);

        for (int i = 0; i < URLLength; i++) {
           // printf("%c",buffer[si + i]);
        }
        memcpy(possibleURL, &buffer[si], URLLength);
        possibleURL[URLLength] = '\0';

        if(checkIfValidURL(possibleURL) == IS_VALID){
            enqueueURL(possibleURL);
        }
        //enqueueURL(possibleURL);

        memset(possibleURL, 0, strlen(possibleURL));



        totalURLs++;
        printf("\n");
       // memcpy(possibleURL, &buffer[si], URLLength);
        //possibleURL[URLLength] = '\0';
        //printf("URL: %s\n",possibleURL);
        //memset(possibleURL, 0, strlen(possibleURL));

        //strncpy(substr, recvBuff[ali], 100);


        buffer = endURL;
    }
    //printf("\n\n\n\nHello\n\n\n");

}

int checkIfValidURL(char possibleURL[])
{
    int validURL;

    /* A valid URL's host should match the host of the given command line URL
     *
     *
     */


    //A valid URL cannot be over 1000 bytes long
    if (strlen(possibleURL) > 1000)
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
