#define _GNU_SOURCE
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include "crawler.h"
#include <regex.h>

URLInfo *pointerBottomURL = NULL;
URLInfo *pointerTopURL = NULL;
char givenFirstComponentOfHostname[MAX_URL_SIZE + NULL_BYTE];
int totalURLs = 0;


int main(int argc,char *argv[]) {


    enqueueURL(argv[1]);
    parseURL(NULL);

    int socketFD;
    char sendBuff[SEND_BUFFER_LENGTH] = {0};
    struct sockaddr_in serv_addr;
    char recvBuff[RECEIVED_BUFFER_LENGTH] = {0};
    char host[1024] = "www.columbia.edu";
    char url[1024] = "/~fdc/sample.html";
    struct hostent *server;

    /*
    if (argc == 1){
         printf("No URL was given\n");
         exit(EXIT_FAILURE);
    }*/

    /*
    if (checkIfValidURL(argv[1]) == IS_VALID) {
        enqueueURL(argv[1]);
        parseURL();
    }
    */

    //strcpy(givenFirstComponentOfHostname,pointerTopURL->firstComponentOfHostname);

    int numberOfBytesRead;
    int total;
    char *tail;
    int index;

    //Content Length Header
    char *contentLengthHeader;
    int contentLength, cli;

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
        //parseURL(URL);

        totalURLs++;
    } else {
        printf("Given URL is not Valid");
        exit(EXIT_FAILURE);
    }

    URLInfo * currentURL;

    while(numberOfPagesFetched < 1 && pointerBottomURL != NULL) {
        total = 0;
        contentLength = -2;
        int isHeader = TRUE;


        numberOfPagesFetched++;

        currentURL = malloc(sizeof(URLInfo));
        //dequeueURL(currentURL);





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
        //sprintf(sendBuff, HTTP_REQUEST_HEADER, currentURL->path, currentURL->hostname);
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
                contentType[9] = NULL_BYTE_CHARACTER;
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

        //Print the URL just parsed to the stdout
        //printf("http://%s%s",currentURL->hostname,currentURL->path);

        free(currentURL);
    }
    printf("\nContent Length: %d, Total URL's: %d\n",total,totalURLs);

    //printStack();
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


    URLInfo * tempPointer;


    while ((startURL = strcasestr(buffer, "<a href=")) != NULL) {
        //Find <a
        //Find the ahref=

        startURL = &(startURL[9]);
        si = (startURL ? startURL - buffer : -1);

        endURL = strstr(startURL, "\"");
        if(endURL != NULL) {
            ei = (endURL ? endURL - buffer : -1);

            URLLength = ei - si;

            memcpy(possibleURL, &buffer[si], URLLength);
            possibleURL[URLLength] = NULL_BYTE_CHARACTER;

            if (checkIfValidURL(possibleURL) == IS_VALID) {
                enqueueURL(possibleURL);
                //parseURL();
                totalURLs++;

                /*
                if (strcmp(pointerTopURL->firstComponentOfHostname, givenFirstComponentOfHostname) != 0) {
                    //If its already in history

                    //dequeueURL();
                    tempPointer = pointerTopURL;
                    pointerTopURL = pointerTopURL->nextNode;
                    free(tempPointer);

                    totalURLs--;
                }
                 */

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

    /*
     * A valid URL is one we have not visited it already
     *
     *
     */


    //A valid URL cannot be over 1000 bytes long
    if (strlen(possibleURL) > MAX_URL_SIZE)
    {
        return validURL = NOT_VALID;
    }

    //A valid URL contains no "." path segments
    if (strstr(possibleURL, "./") != NULL)
    {
        return validURL = NOT_VALID;
    }

    //A valid URL contains no ".." path segments
    if (strstr(possibleURL, "../") != NULL)
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
void parseURL(URLInfo * currentURL) {



    char * pointerURL = &(pointerTopURL->fullURL[0]);




    char * firstDot;
    int fdi, fchLength;
    int gotHostname = 0;

    char *firstSlash;
    int fsi;

    char * endURL;
    int eURLi;

    //Check if URL is Absolute (Fully Specified)
    if(strcasestr(pointerTopURL->fullURL,"http://") != NULL) {
        pointerURL = &(pointerTopURL->fullURL[7]);
        //If so move pointer to the character after the last /

    }
    //Check if URL is Absolute (Implied Protocol)
    else if (strstr(pointerTopURL->fullURL,"//") != NULL) {
        //If so move pointer to the character after the last /
        pointerURL = &(pointerTopURL->fullURL[2]);


    }
    //Check if URL is Absolute (Implied Protocol and Hostname)
    else if (pointerTopURL->fullURL[0] == '/'){
        //strcpy(pointerTopURL->hostname,currentURL->hostname);
        //strcpy(pointerTopURL->firstComponentOfHostname,currentURL->firstComponentOfHostname);
        strcpy(pointerTopURL->hostname,"test.com");
        strcpy(pointerTopURL->firstComponentOfHostname,"test");

        pointerURL = &(pointerTopURL->fullURL[1]);


        firstSlash = strchr(pointerURL, '\0');
        fsi = (firstSlash ? firstSlash - pointerURL : -1);
        memcpy(pointerTopURL->path, &pointerURL[0], fsi);
        pointerTopURL->path[fsi] = NULL_BYTE_CHARACTER;

        //Therefore path = ""
        //strcpy(pointerTopURL->path,"");

        //If so move pointer to the character after the last /
        printf("Full URL: %s\n",pointerTopURL->fullURL);
        printf("First Component of Hostname: %s\n",pointerTopURL->firstComponentOfHostname);
        printf("Hostname: %s\n",pointerTopURL->hostname);
        printf("Path: %s\n",pointerTopURL->path);
        return;
    }


    //Pointer is now pointing to the beginning of the hostname

        //get the first component of the hostname.
    if((firstDot = strchr(pointerURL, '.')) != NULL) {
        fdi = (firstDot ? firstDot - pointerURL : -1);

        memcpy(pointerTopURL->firstComponentOfHostname, &pointerURL[0], fdi);
        pointerTopURL->firstComponentOfHostname[fdi] = NULL_BYTE_CHARACTER;
    } else{
        endURL = strchr(pointerURL, '\0');
        eURLi = (endURL ? endURL - pointerURL : -1);
        memcpy(pointerTopURL->firstComponentOfHostname, &pointerURL[0], eURLi);
        pointerTopURL->path[eURLi] = NULL_BYTE_CHARACTER;
    }

    //Pointer is still pointing to the beginning of the hostname

    //Find the end of the hostname


    if ((firstSlash = strchr(pointerURL, '/'))!= NULL) {

        fsi = (firstSlash ? firstSlash - pointerURL : -1);


        memcpy(pointerTopURL->hostname, &pointerURL[0], fsi);
        pointerTopURL->hostname[fsi] = NULL_BYTE_CHARACTER;


        pointerURL = &(pointerURL[fsi]);

        endURL = strchr(pointerURL, '\0');
        eURLi = (endURL ? endURL - pointerURL : -1);
        memcpy(pointerTopURL->path, &pointerURL[0], eURLi);
        pointerTopURL->path[eURLi] = NULL_BYTE_CHARACTER;



    }
    else
    //If above == NUll that means there is no path
    {
        firstSlash = strchr(pointerURL, '\0');
        fsi = (firstSlash ? firstSlash - pointerURL : -1);
        memcpy(pointerTopURL->hostname, &pointerURL[0], fsi);
        pointerTopURL->hostname[fsi] = NULL_BYTE_CHARACTER;

        //Therefore path = ""
        strcpy(pointerTopURL->path,"");
    }

    printf("Full URL: %s\n",pointerTopURL->fullURL);
    printf("First Component of Hostname: %s\n",pointerTopURL->firstComponentOfHostname);
    printf("Hostname: %s\n",pointerTopURL->hostname);
    printf("Path: %s\n",pointerTopURL->path);
}

void dequeueURL(URLInfo *toFetchURL){
    if(pointerBottomURL == NULL){
        return;
    }
    strcpy(toFetchURL->fullURL, pointerBottomURL->fullURL);
    strcpy(toFetchURL->hostname, pointerBottomURL->hostname);
    strcpy(toFetchURL->firstComponentOfHostname, pointerBottomURL->firstComponentOfHostname);
    strcpy(toFetchURL->path, pointerBottomURL->path);


    URLInfo *TempPointer = pointerBottomURL;
    pointerBottomURL = pointerBottomURL->nextNode;
    free(TempPointer);

    if(pointerBottomURL == NULL){
        pointerTopURL = NULL;
    }

}

void enqueueURL(char *URL){
    URLInfo *TempPointer = malloc(sizeof(URLInfo));

    if(TempPointer == NULL){
        return;
    }
    strcpy(TempPointer->fullURL,URL);
    TempPointer->nextNode = NULL;

    if(pointerBottomURL == NULL)
    {
        TempPointer = pointerBottomURL = TempPointer;
    }
    else
    {
        pointerTopURL->nextNode = TempPointer;
    }
    pointerTopURL = TempPointer;

}

void printStack(void){
    printf("***List of URL's***\n");
    printf("**************\n");
    int numberOfNodes = 0;
    URLInfo *TempPointer = pointerBottomURL;
    while(TempPointer != NULL){
        numberOfNodes++;
        printf("%s\n",TempPointer->fullURL);
        //printf("%s\n",TempPointer->hostname);
        TempPointer = TempPointer->nextNode;
    };
    printf("**************\n");
    printf("Counted %d URL's\n",numberOfNodes);
    printf("***List of URL's***\n");
}
