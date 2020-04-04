#define _GNU_SOURCE
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include "crawler.h"
#include <errno.h>

#include <unistd.h>


URLInfo *pointerBottomURL = NULL;
URLInfo *pointerTopURL = NULL;
uniqueURL * pointerToHistory = NULL;
char givenFirstComponentOfHostname[MAX_URL_SIZE + NULL_BYTE];
int totalURLs = 0;



int main(int argc,char *argv[]) {

    //
    //printf("STARTED PROGRAM\n");
    //

    //Check if a URL was given
    if (argc == 1){
         fprintf(stderr,"No URL was given\n");
         exit(EXIT_FAILURE);
    }



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

    //
    //printf("GIVEN URL WAS VALID\n");
    //

    //Store the first component of the given host for future comparisons
    strcpy(givenFirstComponentOfHostname,pointerTopURL->allButFirstComponent);

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
        printf("\tFull URL: %s\n", currentURL->fullURL);
        printf("\tFirst Component: %s\n", currentURL->allButFirstComponent);
        printf("\tHostname: %s\n", currentURL->hostname);
        printf("\tPath: %s\n", currentURL->path);



        if((strcmp(currentURL->allButFirstComponent, givenFirstComponentOfHostname) != 0) && commandLineURLParsed == TRUE){
            free(currentURL);
            continue;
        }


        commandLineURLParsed = TRUE;

        if(checkHistory(currentURL) == IS_NOT_VALID_URL){
            free(currentURL);
            continue;
        }


        //Details of the Server
        server = gethostbyname(currentURL->hostname);
        if(server == NULL){
            printf("NotValidHostname\n");
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
            //fprintf(stderr,"Connected successfully\n");
        }

        //Send HTTP GET request to the server
        sprintf(sendBuff, HTTP_REQUEST_HEADER, currentURL->path, currentURL->hostname, USERNAME);
        printf("GET REQUEST: %s\n",sendBuff);
        if(send(socketFD, sendBuff, strlen(sendBuff), 0) < 0) {
            fprintf(stderr,"Error with sending GET request\n");
            exit(EXIT_FAILURE);
        } else {
            //fprintf(stderr,"Successfully sent html fetch request\n");
        }


        //Add to total of Web pages that were attempted to be fetched
        numberOfPagesFetched++;

        char * fullBuffer = strdup("");

        //Receive the response from the server
        while (contentLength != total && (numberOfBytesRead = read(socketFD, recvBuff, sizeof(recvBuff))) > 0) {

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
                    //Success, All is good
               } else{
                   goto error;
               }

                //Check to see if the end of the header is fully received
                if (endOfHeaderPointer != NULL) {

                    isHeader = FALSE;
                    index = (endOfHeaderPointer ? endOfHeaderPointer - recvBuff : -1) + 4;

                    total += numberOfBytesRead - index;

                    endOfHeaderPointer = &(endOfHeaderPointer[4]);
                    //printf("LENGTH: %lu TOTAL: %d\n",strlen(endOfHeaderPointer),total);

                    fullBuffer = realloc(fullBuffer, total + 10);
                    strcat(fullBuffer,endOfHeaderPointer);

                }

                memset(recvBuff, 0, strlen(recvBuff));
                continue;
            }

            total += numberOfBytesRead;
            fullBuffer = realloc(fullBuffer, total + 10);
            strcat(fullBuffer,recvBuff);
            memset(recvBuff, 0, strlen(recvBuff));

            if(total == contentLength){
                close(socketFD);
            }

        }


        //closeSocket(socketFD);
        //printf("%s\n",fullBuffer);

        if(total == contentLength) {
            parseHTML(fullBuffer, currentURL);
            //printf("http://%s%s\n",currentURL->hostname,currentURL->path);

        }

        //Print the URL just parsed to the stdout

        //Free buffers and URLs
        error:
        free(fullBuffer);


        free(currentURL);
        memset(recvBuff, 0, strlen(recvBuff));
        //shutdown(socketFD,SHUT_RDWR);
        usleep(20000);
        close(socketFD);
    }

    clearHistory();

    return 0;
}

//Function to parse the HTML and find URLs
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

            startURL = strcasestr(startURL, "=");
            startURL = &(startURL[1]);


            while(startURL[0] == ' '){
                startURL = &(startURL[1]);
            }
            startURL = &(startURL[1]);


            si = (startURL ? startURL - anchor : -1);


            quotationMark = strstr(startURL, "\"");
            qi = (quotationMark ? quotationMark - anchor : -1);

            apostrophe = strstr(startURL, "\'");
            ai = (apostrophe ? apostrophe - anchor : -1);


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
                printf("URLENGTH: %d\n",URLLength);
                possibleURL = malloc(URLLength + NULL_BYTE);
                memcpy(possibleURL, &anchor[si], URLLength);
                possibleURL[URLLength] = NULL_BYTE_CHARACTER;
                printf("%s\n",possibleURL);



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

//Function to check if a URL is valid
int checkIfValidURL(char possibleURL[]) {

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
 *  A function breaks up a URL into its constituents
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
        strcpy(pointerTopURL->allButFirstComponent, currentURL->allButFirstComponent);

        pointerURL = &(pointerTopURL->fullURL[1]);


        firstSlash = strchr(pointerURL, '\0');
        fsi = (firstSlash ? firstSlash - pointerURL : -1);
        memcpy(pointerTopURL->path, &pointerURL[0], fsi);
        pointerTopURL->path[fsi] = NULL_BYTE_CHARACTER;

        return;
    }


    //Pointer is now pointing to the beginning of the hostname



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


    if((firstDot = strchr(pointerTopURL->hostname, '.')) != NULL) {
        firstDot = &(firstDot[1]);
        fdi = (firstDot ? firstDot - pointerTopURL->hostname : -1);

        memcpy(pointerTopURL->allButFirstComponent, firstDot, strlen(pointerTopURL->hostname) - fdi);
        pointerTopURL->allButFirstComponent[strlen(pointerTopURL->hostname) - fdi] = NULL_BYTE_CHARACTER;
    } else{
        strcpy(pointerTopURL->allButFirstComponent,pointerTopURL->hostname);
        pointerTopURL->path[strlen(pointerTopURL->allButFirstComponent)] = NULL_BYTE_CHARACTER;
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

void dequeueURL(URLInfo *toFetchURL){
    if(pointerBottomURL == NULL){
        return;
    }
    strcpy(toFetchURL->fullURL, pointerBottomURL->fullURL);
    strcpy(toFetchURL->hostname, pointerBottomURL->hostname);
    strcpy(toFetchURL->allButFirstComponent, pointerBottomURL->allButFirstComponent);
    strcpy(toFetchURL->path, pointerBottomURL->path);


    URLInfo *TempPointer = pointerBottomURL;
    pointerBottomURL = pointerBottomURL->nextNode;
    free(TempPointer);

    if(pointerBottomURL == NULL){
        pointerTopURL = NULL;
    }

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

int getSO_ERROR(int fd) {
    int err = 1;
    socklen_t len = sizeof err;
    if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&err, &len)){
        //fatalError("getSO_ERROR");
        perror("getSO_ERROR");}
    if (err) {
        errno = err;
    }// set errno to the socket SO_ERROR
    return err;
}

void closeSocket(int fd) {      // *not* the Windows closesocket()
    if (fd >= 0) {
        getSO_ERROR(fd); // first clear any errors, which can cause close to fail
        if (shutdown(fd, SHUT_RDWR) < 0) // secondly, terminate the 'reliable' delivery
            if (errno != ENOTCONN && errno != EINVAL) // SGI causes EINVAL
                perror("shutdown");
        if (close(fd) < 0) // finally call close()
            perror("close");
    }
}