#define _GNU_SOURCE
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include "crawler.h"

#include <unistd.h>


URLInfo *pointerBottomURL = NULL;
URLInfo *pointerTopURL = NULL;
uniqueURL * pointerToHistory = NULL;
char givenFirstComponentOfHostname[MAX_URL_SIZE + NULL_BYTE];
int totalURLs = 0;


int main(int argc,char *argv[]) {

    //Check if a URL was given
    if (argc == 1){
         fprintf(stderr,"No URL was given\n");
         exit(EXIT_FAILURE);
    }

    printf("FIRST URL: %s\n", argv[1]);

    //Check if command line URL is valid
    if (checkIfValidURL(argv[1]) == IS_VALID_URL){

        enqueueURL(argv[1]);
        parseURL(NULL);

        //Need to remove later//
        totalURLs++;
        ///////////////////////

    } else {

        fprintf(stderr,"Given URL is not Valid");
        exit(EXIT_FAILURE);

    }

    //Store the first component of the given host for future comparisons
    strcpy(givenFirstComponentOfHostname,pointerTopURL->firstComponentOfHostname);

    //Variables for using a socket
    int socketFD;
    char sendBuff[SEND_BUFFER_LENGTH] = {0};
    struct sockaddr_in serv_addr;
    char recvBuff[RECEIVED_BUFFER_LENGTH] = {0};
    struct hostent *server;


    int numberOfBytesRead;
    int total;
    char *endOfHeaderPointer;
    int index;

    //Content Length Header
    char *contentLengthHeader;
    int contentLength, cli;

    //Status Code
    char * pageStatusCode;
    int statusCode, sci;

    //Content Type Header
    char *contentTypeHeader;
    char contentType[100];
    int cti;

    int commandLineURLParsed = FALSE;

    int numberOfPagesFetched = 0;
    URLInfo * currentURL;

    while(numberOfPagesFetched <= MAX_NUMBER_OF_PAGES_FETCHED && pointerBottomURL != NULL) {
        total = 0;
        contentLength = -2;
        int isHeader = TRUE;


        ///printf("Number of Pages Fetched: %d\n",numberOfPagesFetched);

        currentURL = malloc(sizeof(URLInfo));

        dequeueURL(currentURL);
        ///printf("Full URL: %s\n", currentURL->fullURL);
        ///printf("First Component: %s\n", currentURL->firstComponentOfHostname);
        ///printf("Hostname: %s\n", currentURL->hostname);
        ///printf("Path: %s\n", currentURL->path);


        if((strcmp(currentURL->firstComponentOfHostname,givenFirstComponentOfHostname) == 0) && commandLineURLParsed == TRUE){
            free(currentURL);
            continue;
        }

        commandLineURLParsed = TRUE;

        if(checkHistory(currentURL) == IS_NOT_VALID_URL){
            //printf("\nDuplicate: %s\n",currentURL->fullURL);
            free(currentURL);
            continue;
        }


        //Details of the Server
        server = gethostbyname(currentURL->hostname);
        if(server == NULL){
            //fprintf(stderr,"NotValidHostname\n");
            free(currentURL);
            continue;
        }


        //Create the socket
        socketFD = socket(AF_INET, SOCK_STREAM, 0);
        if(socketFD < NO_SOCKET_OPENED) {
            //printf("Error with opening socket\n");
            //exit(EXIT_FAILURE);
            perror("socket error\n");
            free(currentURL);
            continue;
        } else {
            ///fprintf(stderr,"Socket opened successfully.\n");
        }

        //initialise server address
        memset(&serv_addr, 0, sizeof(serv_addr));

        //initialise send buffer
        memset(sendBuff, '0', sizeof(sendBuff));


        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);
        memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

        //Connect to the desired Server
        if(connect(socketFD, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            printf("Error with connecting\n");
            //exit(EXIT_FAILURE);
            free(currentURL);
            continue;
        } else {
            ///fprintf(stderr,"Connected successfully\n");
        }

        //Send HTTP GET request to the server
        sprintf(sendBuff, HTTP_REQUEST_HEADER, currentURL->path, currentURL->hostname);
        if(send(socketFD, sendBuff, strlen(sendBuff), 0) < 0) {
            fprintf(stderr,"Error with sending GET request\n");
            exit(EXIT_FAILURE);
        } else {
            ///fprintf(stderr,"Successfully sent html fetch request\n");
        }

        //Add to total of Web pages that were attempted to be fetched
        numberOfPagesFetched++;

        //Receive the response from the server
        while (contentLength != total && (numberOfBytesRead = recv(socketFD, recvBuff, sizeof(recvBuff) - 1, 0)) > 0) {

            if (isHeader == TRUE) {
                endOfHeaderPointer = strstr(recvBuff, END_OF_HTTP_HEADER);

                //Content Type
                contentTypeHeader = strcasestr(recvBuff, CONTENT_TYPE);
                if (contentTypeHeader == NULL){
                    //fprintf(stderr, "No Content Length Header\n");
                    break;
                }
                contentTypeHeader += strlen(CONTENT_TYPE);
                while(contentTypeHeader[0] == ' '){
                    contentTypeHeader++;
                }

                cti = (contentTypeHeader ? contentTypeHeader - recvBuff : -1);
                memcpy(contentType, &recvBuff[cti], 9);
                contentType[9] = NULL_BYTE_CHARACTER;


                if((strcasecmp(contentType,VALID_MIME_TYPE) != 0 || contentTypeHeader == NULL)){
                    ///printf("\nNot VALID MIME TYPE\n");
                    break;
                }

                //Need to remove later//
                ///printf("\nContent-Type: %s\n",contentType);
                ////////////////////////

                //Content Length
                contentLengthHeader = strcasestr(recvBuff, CONTENT_LENGTH);
                if (contentLengthHeader == NULL){
                    ///printf("\nNo Content Length Header\n");
                    break;
                }
                contentLengthHeader += strlen(CONTENT_LENGTH);
                while(contentLengthHeader[0] == ' '){
                    contentLengthHeader++;
                }
                cli = (contentLengthHeader ? contentLengthHeader - recvBuff : -1);
                contentLength = atoi(&(recvBuff[cli]));

                //Need to remove later//
                ///printf("\nContent-Length: %d\n", contentLength);
                /////////////////////////

                //Status Code
                pageStatusCode = strcasestr(recvBuff, STATUS_CODE) + strlen(STATUS_CODE);
                sci = (pageStatusCode ? pageStatusCode - recvBuff : -1);
                statusCode = atoi(&(recvBuff[sci]));

                //Need to remove later//
                ///printf("\nStatus-Code: %d\n", statusCode);
                ////////////////////////

               if(statusCode == 200){
                    //Success
               } else if(statusCode == 404 || statusCode == 410 ||statusCode == 414){
                    break;
               }
               else if(statusCode == 503){
                    break;
               }
               else if(statusCode == 504){
                    break;
               }
               /*
               else if(statusCode == 301){

               }
               else if(statusCode == 401){

               } */
               else {
                   break;
               };

                //Check to see if the end of the header is fully received
                if (endOfHeaderPointer != NULL) {

                    isHeader = FALSE;
                    index = (endOfHeaderPointer ? endOfHeaderPointer - recvBuff : -1) + 4;

                    total += numberOfBytesRead - index;

                    parseHTML(recvBuff, currentURL);

                }

                memset(recvBuff, 0, strlen(recvBuff));
                continue;
            }

            parseHTML(recvBuff, currentURL);
            total += numberOfBytesRead;
            memset(recvBuff, 0, strlen(recvBuff));

        }

        shutdown(socketFD,SHUT_RDWR);
        close(socketFD);
        //Print the URL just parsed to the stdout
        //printf("\nContent Length: %d, Total URL's Parsed: %d\n",total,numberOfPagesFetched);
        printf("http://%s%s\n",currentURL->hostname,currentURL->path);

        free(currentURL);
    }

    clearHistory();
    printStack();
    return 0;
}




void parseHTML(char buffer[], URLInfo * currentURL)
{
    int si, ei, URLLength;
    char * startURL;
    char * endURL;
    char * possibleURL;

    char * apostrophe;
    char * quotationMark;
    int qi,ai;


    char * startAnchor;
    char * endAnchor;
    int anchorLength;

    char * anchor;


    while ((startAnchor = strcasestr(buffer, "<a")) != NULL) {
        si = (startAnchor ? startAnchor - buffer : -1);

        endAnchor = strcasestr(startAnchor, ">");
        if(endAnchor == NULL){
            buffer = startAnchor + 2;
            continue;
        }
        ei = (endAnchor ? endAnchor - buffer : -1) + 1;

        anchorLength = ei - si;
        anchor = malloc(MAX_URL_SIZE + NULL_BYTE + 10000);
        memcpy(anchor, &buffer[si], anchorLength);
        anchor[anchorLength] = NULL_BYTE_CHARACTER;


        if((startURL = strcasestr(anchor, "href")) != NULL) {

            startURL = &(startURL[4]);

            startURL = strcasestr(anchor, "=");
            startURL = &(startURL[1]);


            while(startURL[0] == ' '){
                startURL = &(startURL[1]);
            }
            startURL = &(startURL[1]);

            //printf("%s\n",startURL);

            si = (startURL ? startURL - anchor : -1);


            quotationMark = strstr(startURL, "\"");
            qi = (quotationMark ? quotationMark - anchor : -1);

            apostrophe = strstr(startURL, "\'");
            ai = (apostrophe ? apostrophe - anchor : -1);

            printf("qi = %d , ai = %d\n",qi,ai);

            if(qi > 0 && ai > 0){

                if (qi < ai){
                    endURL = quotationMark;
                } else if (qi > ai){
                    endURL = apostrophe;
                }

            }
            else if (qi < 0){
                endURL = apostrophe;

            } else if (ai < 0){

                endURL = quotationMark;
            } else {
                continue;
            }



            if (endURL != NULL) {
                ei = (endURL ? endURL - anchor : -1);

                URLLength = ei - si;
                possibleURL = malloc(MAX_URL_SIZE + NULL_BYTE);
                memcpy(possibleURL, &anchor[si], URLLength);
                possibleURL[URLLength] = NULL_BYTE_CHARACTER;



                if (checkIfValidURL(possibleURL) == IS_VALID_URL) {
                    enqueueURL(possibleURL);
                    parseURL(currentURL);
                    totalURLs++;

                }

                free(possibleURL);

                //memset(possibleURL, 0, strlen(possibleURL));
            }
        }

        free(anchor);
        buffer = endAnchor;
        //printf("%s",buffer);
    }


}

int checkIfValidURL(char possibleURL[]) {

    /* A valid URL's host 1st component should match the host of the given command line URL
     *
     *
     */

    /*
     * A valid URL is one we have not visited it already
     *
     *
     */

    if (strstr(possibleURL, "https://") != NULL) {
        return IS_NOT_VALID_URL;
    }


    //A valid URL cannot be over 1000 bytes long
    if (strlen(possibleURL) > MAX_URL_SIZE) {
        return IS_NOT_VALID_URL;
    }

    //A valid URL contains no "." path segments
    if (strstr(possibleURL, "./") != NULL) {
        return IS_NOT_VALID_URL;
    }

    //A valid URL contains no ".." path segments
    if (strstr(possibleURL, "../") != NULL) {
        return IS_NOT_VALID_URL;
    }

    //A valid URL contains no Exclamation Mark
    if (strchr(possibleURL, EXCLAMATION_MARK) != NULL) {
        return IS_NOT_VALID_URL;
    }

    //A valid URL contains no Hashtag
    if (strchr(possibleURL, HASHTAG) != NULL) {
        return IS_NOT_VALID_URL;
    }

    //A valid URL contains no URL Encoded Characters
    if (strchr(possibleURL, URL_ENCODED_CHARACTER) != NULL) {
        return IS_NOT_VALID_URL;
    }

    if (strcmp(possibleURL, "") == 0) {
        return IS_NOT_VALID_URL;
    }





    return IS_VALID_URL;
}

/*
 *  A function that returns the host of a given URL
 */
void parseURL(URLInfo * currentURL) {

    char * pointerURL = &(pointerTopURL->fullURL[0]);

    char * firstDot;
    int fdi;

    char *firstSlash;
    int fsi;

    char * endURL;
    int eURLi;

    //Check if URL is Absolute (Fully Specified)
    if(strcasestr(pointerTopURL->fullURL,"http://") != NULL) {
        //If so move pointer to the character after the last /
        pointerURL = &(pointerTopURL->fullURL[7]);

    }
    //Check if URL is Absolute (Implied Protocol)
    else if (strstr(pointerTopURL->fullURL,"//") != NULL) {

        //If so move pointer to the character after the last /
        pointerURL = &(pointerTopURL->fullURL[2]);


    }
    //Check if URL is Absolute (Implied Protocol and Hostname)
    else if (pointerTopURL->fullURL[0] == '/'){

        strcpy(pointerTopURL->hostname,currentURL->hostname);
        strcpy(pointerTopURL->firstComponentOfHostname,currentURL->firstComponentOfHostname);

        pointerURL = &(pointerTopURL->fullURL[1]);


        firstSlash = strchr(pointerURL, '\0');
        fsi = (firstSlash ? firstSlash - pointerURL : -1);
        memcpy(pointerTopURL->path, &pointerURL[0], fsi);
        pointerTopURL->path[fsi] = NULL_BYTE_CHARACTER;

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




int checkHistory(URLInfo * URLtoCheck){
    uniqueURL * tempPointer = pointerToHistory;
    uniqueURL * newUniqueURL;

    //Check the URL against the existing HISTORY
    while(tempPointer != NULL){
        if((strcmp(URLtoCheck->hostname,tempPointer->URLhostname) == 0) && (strcmp(URLtoCheck->path,tempPointer->URLpath) == 0)){
            return IS_NOT_VALID_URL;
        }

        tempPointer = tempPointer->nextUniqueURL;
    }



    newUniqueURL = malloc(sizeof(uniqueURL));
    newUniqueURL->nextUniqueURL = pointerToHistory;
    strcpy(newUniqueURL->URLhostname, URLtoCheck->hostname);
    strcpy(newUniqueURL->URLpath, URLtoCheck->path);

    pointerToHistory = newUniqueURL;

    return IS_VALID_URL;
}

void clearHistory() {
    uniqueURL * tempPointer;

    while (pointerToHistory != NULL)
    {
        tempPointer = pointerToHistory;
        pointerToHistory = pointerToHistory->nextUniqueURL;
        free(tempPointer);
    }

}
