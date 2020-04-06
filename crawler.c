/*
 *              crawler.c
 * Computer Systems (COMP30023) - Project 1
 * Adam Lawrence || arlawrence || 992684
 * arlawrence@student.unimelb.edu.au
 */

#define _GNU_SOURCE
#include <stdio.h>
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



int main(int argc,char *argv[]) {

    //Check if a URL was given
    if (argc == 1){

         fprintf(stderr,"No URL was given\n");
         exit(EXIT_FAILURE);

    }

    //Check if command line URL is valid
    if (checkIfValidURL(argv[1]) == IS_VALID_URL){

        enqueueURL(argv[1]);
        parseURL(NULL);

    } else {

        fprintf(stderr,"Given URL is not Valid");
        exit(EXIT_FAILURE);

    }

    //Store all but the first component of the hostname for future comparisons
    strcpy(givenFirstComponentOfHostname,pointerTopURL->allButFirstComponent);

    //Variables for using the socket
    int socketFD;
    char sendBuff[SEND_BUFFER_LENGTH + NULL_BYTE] = {0};
    struct sockaddr_in serv_addr;
    char recvBuff[RECEIVED_BUFFER_LENGTH] = {0};
    struct hostent *server;

    int numberOfBytesRead;
    int total;
    char *endOfHeaderPointer;
    int index;

    int totalURLs = 0;

    //Content Length Header
    char *contentLengthHeader;
    int contentLength, cli;

    //Location Header (For 301)
    char * locationHeader;
    char * endLocationHeader;
    int  lhi, elhi;
    char URLFor301[MAX_URL_SIZE+NULL_BYTE];

    //Status Code
    char * pageStatusCode;
    int statusCode, sci;

    //Content Type Header
    char *contentTypeHeader;
    char contentType[1000];
    int cti,scti;

    int commandLineURLParsed = FALSE;

    int numberOfPagesFetched = 0;
    URLInfo * currentURL;

    while(numberOfPagesFetched != MAX_NUMBER_OF_PAGES_FETCHED && pointerBottomURL != NULL) {
        total = 0;
        contentLength = -2;
        int isHeader = TRUE;

        //Get the URL to crawl
        currentURL = malloc(sizeof(URLInfo));
        dequeueURL(currentURL);

        ////Remove Later
        printf("\tFull URL: %s\n", currentURL->fullURL);
        printf("\tFirst Component: %s\n", currentURL->allButFirstComponent);
        printf("\tHostname: %s\n", currentURL->hostname);
        printf("\tPath: %s\n", currentURL->path);
        printf("\tFILE NAME: %s\n", currentURL->htmlFile);

        ////


        //Check if all but the first components of the current URL match with the command line URL
        if((strcmp(currentURL->allButFirstComponent, givenFirstComponentOfHostname) != 0) && commandLineURLParsed == TRUE){
            free(currentURL);
            continue;
        }
        commandLineURLParsed = TRUE;

        //Check if the current URL has already been parsed (Ignore if we are purposely refetching)
        if(currentURL->refetchTimes == 0 || currentURL->refetchTimes == REFETCH_LIMIT) {
            if (checkHistory(currentURL) == IS_NOT_VALID_URL) {
                free(currentURL);
                continue;
            }
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
            fprintf(stderr,"Error with opening socket\n");
            free(currentURL);
            continue;
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
            fprintf(stderr,"Error with connecting\n");
            free(currentURL);
            continue;
        }

        //Send HTTP GET request to the server
        if(currentURL->needAuthorization == TRUE){
            sprintf(sendBuff, REQUEST_WITH_AUTHORIZATION, currentURL->path, currentURL->hostname, USERNAME);
        }else {
            sprintf(sendBuff, HTTP_REQUEST_HEADER, currentURL->path, currentURL->hostname, USERNAME);
        }

        ////Remove Later
        printf("GET REQUEST: %s\n",sendBuff);
        ////Remove Later

        if(send(socketFD, sendBuff, strlen(sendBuff), 0) < 0) {
            fprintf(stderr,"Error with sending GET request\n");
            free(currentURL);
            continue;
        }


        //Add to total of unique Web pages that were attempted to be fetched
        if(currentURL->refetchTimes == 0) {
            numberOfPagesFetched++;
        }

        char * fullBuffer = strdup("");

        //Receive the response from the server
        while (contentLength != total && (numberOfBytesRead = recv(socketFD, recvBuff, sizeof(recvBuff),0)) > 0) {

            if (isHeader == TRUE) {
                endOfHeaderPointer = strstr(recvBuff, END_OF_HTTP_HEADER);

                //Find the Content Type Header
                if ((contentTypeHeader = strcasestr(recvBuff, CONTENT_TYPE)) == NULL){
                    break;
                }

                scti = (int)( contentTypeHeader - recvBuff);

                while(contentTypeHeader[0] != '\n'){
                    contentTypeHeader++;
                }
                cti = (int) (contentTypeHeader - recvBuff);

                memcpy(contentType, &recvBuff[scti], cti-scti);
                contentType[cti-scti] = NULL_BYTE_CHARACTER;

                if((strcasestr(contentType,VALID_MIME_TYPE) == NULL)){
                    break;
                }


                //Find the Content Length Header
                if ((contentLengthHeader = strcasestr(recvBuff, CONTENT_LENGTH)) == NULL){
                    break;
                }
                contentLengthHeader += strlen(CONTENT_LENGTH);

                while(contentLengthHeader[0] == ' '){
                    contentLengthHeader++;
                }
                cli = (int) (contentLengthHeader - recvBuff);
                contentLength = (int) strtol(&(recvBuff[cli]),NULL,10);

                //Find the Status Code
                if ((pageStatusCode = strcasestr(recvBuff, STATUS_CODE)) == NULL){
                    break;
                }
                pageStatusCode += strlen(STATUS_CODE);

                while(pageStatusCode[0] == ' '){
                    pageStatusCode++;
                }

                sci = (int) (pageStatusCode - recvBuff);
                statusCode = (int) strtol(&(recvBuff[sci]),NULL,10);


                if(statusCode == 200){
                    //Success, All is good
               } else if(statusCode == 503){

                    //The page at the current URL is currently unavailable, thus we must refetch it
                    enqueueURL(currentURL->fullURL);
                   parseURL(currentURL);

                   pointerTopURL->refetchTimes = currentURL->refetchTimes + 1;

               } else if(statusCode == 401){

                   //The current URL needs to be refetched again, but with authorization so we may access it.
                   enqueueURL(currentURL->fullURL);
                   parseURL(currentURL);
                   pointerTopURL->refetchTimes = currentURL->refetchTimes + 1;
                   pointerTopURL->needAuthorization = TRUE;

               } else if (statusCode == 301){

                    printf("HELP301.1\n");

                   //The web page has been redirected, thus get the URL for the redirection and add it to the queue
                   if ((locationHeader = strcasestr(recvBuff, LOCATION_HEADER)) == NULL){
                       break;
                   }
                    printf("HELP302.1\n");

                    locationHeader += strlen(LOCATION_HEADER);
                   while(locationHeader[0] == ' '){
                       locationHeader++;
                   }
                    printf("HELP303.1\n");


                    endLocationHeader = locationHeader;

                    printf("HELP304.1\n");

                    while(endLocationHeader[0] != '\r'){
                       endLocationHeader++;
                   }

                    printf("HELP305.1\n");

                    lhi = (int) (locationHeader - recvBuff);
                    elhi = (int) (endLocationHeader - recvBuff);

                    printf("HELP306.1\n");


                    memcpy(URLFor301, &recvBuff[lhi], elhi - lhi);
                   URLFor301[elhi - lhi] = NULL_BYTE_CHARACTER;
                    printf("HELP307.1\n");

                    printf("URL301: %s\n",URLFor301);

                    printf("HELP307.1\n");
                    enqueueURL(URLFor301);
                   parseURL(currentURL);

               } else if (statusCode == 404 ||statusCode == 410 ||statusCode == 414 ||statusCode == 504){

               } else{
                   goto error;
               }


                //Check to see if the end of the header is fully received
                if (endOfHeaderPointer != NULL) {

                    isHeader = FALSE;
                    index = (int) (endOfHeaderPointer - recvBuff) + 4;

                    total += numberOfBytesRead - index;

                    endOfHeaderPointer = &(endOfHeaderPointer[4]);

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

            //If the total bytes received
            if(total >= contentLength){
                close(socketFD);
            }

        }

        if(total == contentLength) {
            parseHTML(fullBuffer, currentURL);
        }

        printf("%s\n",fullBuffer);

        error:


        //Print the URL just parsed to the stdout
        printf("http://%s%s\n",currentURL->hostname,currentURL->path);


        //Free buffers and close down the connection
        free(fullBuffer);
        free(currentURL);
        memset(recvBuff, 0, strlen(recvBuff));
        close(socketFD);

    }

    //Clear the History
    clearHistory();

    ////REMOVE LATER////
    printf("NUMBER OF PAGES %d\n",numberOfPagesFetched);
    ////////////////////

    return 0;
}

/*
 *  Function to parse the HTML and extract URLs
 */
void parseHTML(char buffer[], URLInfo * currentURL)
{
    int si, ei, URLLength;
    char * startURL;
    char *endURL = NULL;
    char * possibleURL;

    char * apostrophe;
    char * quotationMark;
    int qi,ai;


    char * startAnchor;
    char * endAnchor;
    int anchorLength;

    char * anchor;


    while ((startAnchor = strcasestr(buffer, "<a")) != NULL) {
        si = (int) (startAnchor ? startAnchor - buffer : -1);


        if((endAnchor = strcasestr(startAnchor, ">")) == NULL){
            buffer = startAnchor + 2;
            continue;
        }

        ei = (int) (endAnchor ? endAnchor - buffer : -1) + 1;

        //Place everything in the anchor tag into a string
        anchorLength = ei - si;
        anchor = malloc(MAX_URL_SIZE + NULL_BYTE + 10000);
        memcpy(anchor, &buffer[si], anchorLength);
        anchor[anchorLength] = NULL_BYTE_CHARACTER;

        //Check if the href attribute is present within this anchor tag
        if((startURL = strcasestr(anchor, HREF_ATTRIBUTE)) != NULL) {
            startURL = &(startURL[strlen(HREF_ATTRIBUTE)]);

            //Find the equal sign that comes after the href
            startURL = strchr(startURL, EQUAL_SIGN);
            startURL = &(startURL[1]);

            //Find the Start of the URL, this comes after the ' or " character
            while(startURL[0] == ' '){
                startURL = &(startURL[1]);
            }
            startURL = &(startURL[1]);

            si = (int) (startURL - anchor);


            //Find what ends the URL: either a quotation mark or a apostrophe
            quotationMark = strstr(startURL, "\"");
            qi = (int) (quotationMark ? quotationMark - anchor : -1);

            apostrophe = strstr(startURL, "\'");
            ai = (int) (apostrophe ? apostrophe - anchor : -1);

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
                ei = (int) (endURL - anchor);

                URLLength = ei - si;
                possibleURL = malloc(URLLength + NULL_BYTE);
                memcpy(possibleURL, &anchor[si], URLLength);
                possibleURL[URLLength] = NULL_BYTE_CHARACTER;

                if (checkIfValidURL(possibleURL) == IS_VALID_URL) {
                    enqueueURL(possibleURL);
                    parseURL(currentURL);

                }

                free(possibleURL);
            }
        }

        free(anchor);
        buffer = endAnchor;
    }

}

/*
 * Function to check if a URL is valid for this crawler
 */
int checkIfValidURL(char possibleURL[]) {

    ////Remove this
    //A valid URL only uses the http protocol
    if (strcasestr(possibleURL, "https://") != NULL) {
        return IS_NOT_VALID_URL;
    }
    ////Remove this

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

    //A valid URL can not be empty
    if (strcmp(possibleURL, "") == 0) {
        return IS_NOT_VALID_URL;
    }

    return IS_VALID_URL;
}


/*
 *  A function which breaks up a URL into its constituents
 */
void parseURL(URLInfo * currentURL) {

    char * pointerURL = &(pointerTopURL->fullURL[0]);


    char * firstDot;
    int fdi;

    char *firstSlash;
    int fsi;

    char * endURL;
    int eURLi;

    char * lastSlash;
    char * temp;

    //Get the html file name
    if((strcasestr(pointerTopURL->fullURL,".html") != NULL) && (strchr(pointerTopURL->fullURL,'/') == NULL)){

        printf("HELLO: %s\n",pointerTopURL->fullURL);


        lastSlash = currentURL->path;
        printf("HELLO2: %s\n",lastSlash);

        while((temp = strchr(lastSlash,'/')) != NULL){
            ++temp;
            lastSlash = temp;
        }

        printf("HELLO2: %s\n",lastSlash);




        fsi = (int) (lastSlash - currentURL->path);


        memcpy(pointerTopURL->path, currentURL->path, fsi);
        pointerTopURL->htmlFile[fsi] = NULL_BYTE_CHARACTER;


        strcat(pointerTopURL->path, pointerTopURL->fullURL);
        printf("YAY? %s\n",pointerTopURL->path);



        strcpy(pointerTopURL->hostname,currentURL->hostname);
        strcpy(pointerTopURL->allButFirstComponent, currentURL->allButFirstComponent);




        return;
    }

    //Check if URL is Absolute (Fully Specified)
    if(strcasestr(pointerTopURL->fullURL,"http://") != NULL) {
        //If so move pointer to the character after the last /
        pointerURL = &(pointerTopURL->fullURL[strlen("http://")]);

    }
        //Check if URL is Absolute (Implied Protocol)
    else if ((pointerTopURL->fullURL[0] == '/') && (pointerTopURL->fullURL[1] == '/')) {

        //If so move pointer to the character after the last slash
        pointerURL = &(pointerTopURL->fullURL[strlen("//")]);


    }
        //Check if URL is Absolute (Implied Protocol and Hostname)
    else if (pointerTopURL->fullURL[0] == '/'){

        strcpy(pointerTopURL->hostname,currentURL->hostname);
        strcpy(pointerTopURL->allButFirstComponent, currentURL->allButFirstComponent);

        pointerURL = &(pointerTopURL->fullURL[1]);


        firstSlash = strchr(pointerURL, '\0');
        fsi = (int) (firstSlash ? firstSlash - pointerURL : -1);
        memcpy(pointerTopURL->path, &pointerURL[0], fsi);
        pointerTopURL->path[fsi] = NULL_BYTE_CHARACTER;

        return;
    }


    //Pointer is now pointing to the beginning of the hostname



    //Find the end of the hostname
    if ((firstSlash = strchr(pointerURL, '/'))!= NULL) {

        fsi = (int) (firstSlash ? firstSlash - pointerURL : -1);


        memcpy(pointerTopURL->hostname, &pointerURL[0], fsi);
        pointerTopURL->hostname[fsi] = NULL_BYTE_CHARACTER;


        pointerURL = &(pointerURL[fsi]);

        endURL = strchr(pointerURL, '\0');
        eURLi = (int) (endURL ? endURL - pointerURL : -1);
        memcpy(pointerTopURL->path, &pointerURL[0], eURLi);
        pointerTopURL->path[eURLi] = NULL_BYTE_CHARACTER;



    }
    else
        //If above == NUll that means there is no path
    {
        firstSlash = strchr(pointerURL, '\0');
        fsi = (int) (firstSlash ? firstSlash - pointerURL : -1);
        memcpy(pointerTopURL->hostname, &pointerURL[0], fsi);
        pointerTopURL->hostname[fsi] = NULL_BYTE_CHARACTER;

        //Therefore path = "/"
        strcpy(pointerTopURL->path,"/");
    }


    if((firstDot = strchr(pointerTopURL->hostname, '.')) != NULL) {
        firstDot = &(firstDot[1]);
        fdi = (int) (firstDot ? firstDot - pointerTopURL->hostname : -1);

        memcpy(pointerTopURL->allButFirstComponent, firstDot, strlen(pointerTopURL->hostname) - fdi);
        pointerTopURL->allButFirstComponent[strlen(pointerTopURL->hostname) - fdi] = NULL_BYTE_CHARACTER;
    } else{
        strcpy(pointerTopURL->allButFirstComponent,pointerTopURL->hostname);
        pointerTopURL->path[strlen(pointerTopURL->allButFirstComponent)] = NULL_BYTE_CHARACTER;
    }
}

/*
 *  Enqueue an URL to the queue
 */
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

    //Initialise default values
    pointerTopURL->refetchTimes = 0;
    pointerTopURL->needAuthorization = FALSE;
}

/*
 *  Dequeue an URL from the queue
 */
void dequeueURL(URLInfo *toFetchURL){

    if(pointerBottomURL == NULL){
        return;
    }

    strcpy(toFetchURL->fullURL, pointerBottomURL->fullURL);
    strcpy(toFetchURL->hostname, pointerBottomURL->hostname);
    strcpy(toFetchURL->allButFirstComponent, pointerBottomURL->allButFirstComponent);
    strcpy(toFetchURL->path, pointerBottomURL->path);
    strcpy(toFetchURL->htmlFile, pointerBottomURL->htmlFile);
    toFetchURL->refetchTimes = pointerBottomURL->refetchTimes;
    toFetchURL->needAuthorization = pointerBottomURL->needAuthorization;


    URLInfo *TempPointer = pointerBottomURL;
    pointerBottomURL = pointerBottomURL->nextNode;
    free(TempPointer);

    if(pointerBottomURL == NULL){
        pointerTopURL = NULL;
    }

}


/*
 *  Function to check all the unique URLs that have been visited already
 */
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

    //If this URL was not in the existing history, add it to the history
    newUniqueURL = malloc(sizeof(uniqueURL));
    newUniqueURL->nextUniqueURL = pointerToHistory;
    strcpy(newUniqueURL->URLhostname, URLtoCheck->hostname);
    strcpy(newUniqueURL->URLpath, URLtoCheck->path);

    pointerToHistory = newUniqueURL;

    //If it was not in the history, the URL is unique, thus valid for fetching
    return IS_VALID_URL;
}

/*
 *  Free the History Linked List
 */
void clearHistory() {
    uniqueURL * tempPointer;

    while (pointerToHistory != NULL)
    {
        tempPointer = pointerToHistory;
        pointerToHistory = pointerToHistory->nextUniqueURL;
        free(tempPointer);
    }

}

