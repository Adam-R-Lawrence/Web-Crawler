/*
 *              crawler.c
 * Computer Systems (COMP30023) - Project 1
 * Adam Lawrence || arlawrence || 992684
 * arlawrence@student.unimelb.edu.au
 *
 */

// _GNU_SOURCE is used for the strcasestr function
#define _GNU_SOURCE

//Libraries used
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include "crawler.h"
#include <unistd.h>

//Global Variables
URLInfo *pointerBottomURL = NULL;
URLInfo *pointerTopURL = NULL;
uniqueURL * pointerToHistory = NULL;
char commandLineAllButFirstComponent[MAX_URL_SIZE + NULL_BYTE];

int main(int argc,char *argv[]) {

    //Check if a URL was given
    if (argc == 1) {

         fprintf(stderr,"No URL was given\n");
         exit(EXIT_FAILURE);

    }

    //Enqueue the command line URL
    enqueueURL(argv[1]);
    parseURL(NULL);

    //Store all but the first component of the hostname for future comparisons
    strcpy(commandLineAllButFirstComponent,
            pointerTopURL->allButFirstComponent);


    //Variables for using the socket
    int socketFD;
    char sendBuff[SEND_BUFFER_LENGTH + NULL_BYTE] = {0};
    struct sockaddr_in server_address;
    char recvBuff[RECEIVED_BUFFER_LENGTH] = {0};
    struct hostent *server;

    char *endOfHeaderPointer;
    int index, isHeader, numberOfBytesRead, total;

    //Content Length Header variables
    char *contentLengthHeader;
    int contentLength, clIndex;

    //Location Header variables (For 301 status codes)
    char * locationHeader;
    char * endLocationHeader;
    int  lhIndex, elhIndex;
    char URLFor301[MAX_URL_SIZE+NULL_BYTE];

    //Status Code variables
    char * pageStatusCode;
    int statusCode, scIndex;

    //Content Type Header variables
    char *contentTypeHeader;
    char contentType[4];
    int ectIndex, ctIndex;

    int commandLineURLParsed = FALSE;
    int numberOfPagesFetched = 0, validToParse;
    URLInfo * currentURL;

    while(numberOfPagesFetched != MAX_NUMBER_OF_PAGES_FETCHED
            && pointerBottomURL != NULL) {

        total = 0;
        contentLength = -2;
        isHeader = TRUE;
        validToParse = TRUE;

        //Get the URL to attempt to crawl
        currentURL = malloc(sizeof(URLInfo));
        dequeueURL(currentURL);

        /*
         * Check if all but the first components of the current URL
         *      match with the command line URL
         */
        if((strcmp(currentURL->allButFirstComponent, commandLineAllButFirstComponent) != 0)
                    && commandLineURLParsed == TRUE) {

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

        //Hostname details of the Server
        server = gethostbyname(currentURL->hostname);
        if(server == NULL) {
            fprintf(stderr,"NotValidHostname\n");
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
        memset(&server_address, 0, sizeof(server_address));

        //initialise send buffer
        memset(sendBuff, '0', sizeof(sendBuff));

        //Information of the server
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(PORT);
        memcpy(&server_address.sin_addr.s_addr, server->h_addr, server->h_length);

        //Connect to the desired Server
        if(connect(socketFD, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
            fprintf(stderr,"Error with connecting\n");
            free(currentURL);
            continue;
        }

        //Send HTTP GET request to the server
        if(currentURL->needAuthorization == TRUE) {
            sprintf(sendBuff, REQUEST_WITH_AUTHORIZATION, currentURL->path, currentURL->hostname, USERNAME);
        }else {
            sprintf(sendBuff, HTTP_REQUEST_HEADER, currentURL->path, currentURL->hostname, USERNAME);
        }

        if(send(socketFD, sendBuff, strlen(sendBuff), 0) < 0) {
            fprintf(stderr,"Error with sending GET request\n");
            free(currentURL);
            continue;
        }


        if(currentURL->refetchTimes == 0) {

            //Add to total of unique Web pages that were attempted to be fetched
            numberOfPagesFetched++;

            //Print the unique URL just parsed to the stdout
            printf("http://%s%s\n",currentURL->hostname,currentURL->path);
        }

        //Set memory for the buffer
        char * fullBuffer = strdup("");

        //Receive the response from the server
        while (contentLength != total && (numberOfBytesRead = recv(socketFD, recvBuff, sizeof(recvBuff),0)) > 0) {

            //Check if we are still reading the HTTP response headers
            if (isHeader == TRUE) {
                endOfHeaderPointer = strstr(recvBuff, END_OF_HTTP_HEADER);

                //Find the Status Code
                if ((pageStatusCode = strcasestr(recvBuff, STATUS_CODE)) == NULL) {
                    break;
                }
                pageStatusCode += strlen(STATUS_CODE);

                while(pageStatusCode[0] == ' ') {
                    pageStatusCode++;
                }
                scIndex = (int) (pageStatusCode - recvBuff);
                statusCode = (int) strtol(&(recvBuff[scIndex]), NULL, 10);


                //Find the Content Type Header
                if ((contentTypeHeader = strcasestr(recvBuff, CONTENT_TYPE)) == NULL) {
                    validToParse = FALSE;
                    break;
                }
                ctIndex = (int)(contentTypeHeader - recvBuff);

                while(contentTypeHeader[0] != '\n'){
                    contentTypeHeader++;
                }
                ectIndex = (int) (contentTypeHeader - recvBuff);

                memcpy(contentType, &recvBuff[ctIndex], ectIndex - ctIndex);
                contentType[ectIndex - ctIndex] = NULL_BYTE_CHARACTER;

                //If content type is not text/html, do not crawl
                if(statusCode != 301) {
                    if ((strcasestr(contentType, VALID_MIME_TYPE) == NULL)) {
                        validToParse = FALSE;
                        break;
                    }
                }

                //Find the Content Length Header
                if ((contentLengthHeader = strcasestr(recvBuff, CONTENT_LENGTH)) == NULL) {
                    validToParse = FALSE;
                    break;
                }
                contentLengthHeader += strlen(CONTENT_LENGTH);

                while(contentLengthHeader[0] == ' ') {
                    contentLengthHeader++;
                }
                clIndex = (int) (contentLengthHeader - recvBuff);
                contentLength = (int) strtol(&(recvBuff[clIndex]), NULL, 10);


                if(statusCode == 200) {
                    /*
                     * If we receive a status code of 200, we are connecting to the web page with no issues,
                     * Thus we can now crawl the html file it contains.
                     *
                    */
               } else if(statusCode == 503 ||statusCode == 504) {

                    /*
                     * For web pages which return a temporary failure error; 503 and 504, we crawl the
                     * error report page and we requeue the URL to try again later.
                     *
                     *  - A 503 Status code means that he server is unable to our GET request due to a
                     *  temporary overload, this should be alleviated after a short delay, thus it is
                     *  only temporary failure.
                     *
                     *  - A 504 Status code means that the page we are trying to load did not get a
                     *  timely response from another server 'upstream', this should also be alleviated
                     *  after a short delay, thus it is only a temporary failure.
                     *
                    */

                    //The page at the current URL is currently unavailable, thus we must refetch it

                    enqueueURL(currentURL->fullURL);
                    parseURL(currentURL);

                   pointerTopURL->refetchTimes = currentURL->refetchTimes + 1;

               } else if(statusCode == 401) {

                   //The current URL needs to be refetched again, but with authorization so we may access it.
                   enqueueURL(currentURL->fullURL);
                   parseURL(currentURL);
                   pointerTopURL->refetchTimes = currentURL->refetchTimes + 1;
                   pointerTopURL->needAuthorization = TRUE;

               } else if (statusCode == 301) {


                   //The web page has been redirected, thus get the URL for the redirection and add it to the queue
                   if ((locationHeader = strcasestr(recvBuff, LOCATION_HEADER)) == NULL) {
                       break;
                   }

                   locationHeader += strlen(LOCATION_HEADER);
                   while(locationHeader[0] == ' ') {
                       locationHeader++;
                   }


                    endLocationHeader = locationHeader;

                    while(endLocationHeader[0] != '\r') {
                       endLocationHeader++;
                   }


                    lhIndex = (int) (locationHeader - recvBuff);
                    elhIndex = (int) (endLocationHeader - recvBuff);

                    memcpy(URLFor301, &recvBuff[lhIndex], elhIndex - lhIndex);
                    URLFor301[elhIndex - lhIndex] = NULL_BYTE_CHARACTER;

                    enqueueURL(URLFor301);
                    parseURL(currentURL);

               } else if (statusCode == 404 ||statusCode == 410 ||statusCode == 414) {

                    /*
                     * For web pages which return a permanent error; 404,410 and 414, we crawl the
                     * error report page.
                     *
                     *  - A 404 status code means that the server found nothing matching the request
                     *    URI. Although no indication is given where it is temporary or permanent. But
                     *    to be cautious we should handle it as a permanent
                     *
                     *  - A 410 status code means the same as a 404 status code except for the fact
                     *    that we definitely know it is never going to be found. Thus we may handle it
                     *    as a permanent failure.
                     *
                     *  - A 414 status code means that the URL we used to request the web page is too long.
                     *    As we do not know any other URL to access the web page, we should handle this
                     *    status code as a permanent failure.
                     *
                    */


               } else {
                   //Ignore all other status codes, so don't crawl the web page
                   validToParse = FALSE;
                   break;
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
            if(total >= contentLength) {
                close(socketFD);
            }
        }

        /*
         * If the file is not truncated we can now parse the HTML,
         * if it is we omit this web page to be crawled.
         * If the file is not valid, such as having a non valid mime-type
         * or a non supported status code, do not crawl as well.
         */
        if(total == contentLength && validToParse == TRUE) {
            parseHTML(fullBuffer, currentURL);
        }


        //Free buffers and close down the connection
        free(fullBuffer);
        free(currentURL);
        memset(recvBuff, 0, strlen(recvBuff));
        close(socketFD);

    }

    //Clear the History
    clearHistory();

    return 0;
}

/*
 *  Function to parse the HTML and extract URLs
 *
 *  No regular expressions were used as I discovered there was no need
 *   for them, using pointers is enough.
 *
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


        if((endAnchor = strcasestr(startAnchor, ">")) == NULL) {
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
            while(startURL[0] == ' ') {
                startURL = &(startURL[1]);
            }
            startURL = &(startURL[1]);

            si = (int) (startURL - anchor);

            //Find what ends the URL: either a quotation mark or a apostrophe
            quotationMark = strstr(startURL, "\"");
            qi = (int) (quotationMark ? quotationMark - anchor : -1);

            apostrophe = strstr(startURL, "\'");
            ai = (int) (apostrophe ? apostrophe - anchor : -1);

            if(qi > 0 && ai > 0) {

                if (qi < ai) {
                    endURL = quotationMark;
                } else if (qi > ai) {
                    endURL = apostrophe;
                }

            }
            else if (qi < 0) {
                endURL = apostrophe;

            } else if (ai < 0) {

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
    char * placeholder;

    //The URL might be a relative stand alone .html file, check if it is
    if((strcasestr(pointerTopURL->fullURL,".html") != NULL)
                    && (strchr(pointerTopURL->fullURL,'/') == NULL)) {

        lastSlash = currentURL->path;

        while((placeholder = strchr(lastSlash, '/')) != NULL) {
            ++placeholder;
            lastSlash = placeholder;
        }

        fsi = (int) (lastSlash - currentURL->path);

        memcpy(pointerTopURL->path, currentURL->path, fsi);
        strcat(pointerTopURL->path, pointerTopURL->fullURL);

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
    else if (pointerTopURL->fullURL[0] == '/') {

        strcpy(pointerTopURL->hostname,currentURL->hostname);
        strcpy(pointerTopURL->allButFirstComponent, currentURL->allButFirstComponent);

        pointerURL = &(pointerTopURL->fullURL[0]);


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
    //If above == NUll that means there is no path
    else {
        firstSlash = strchr(pointerURL, '\0');
        fsi = (int) (firstSlash ? firstSlash - pointerURL : -1);
        memcpy(pointerTopURL->hostname, &pointerURL[0], fsi);
        pointerTopURL->hostname[fsi] = NULL_BYTE_CHARACTER;

        //Therefore path = "/"
        strcpy(pointerTopURL->path,"/");
    }

    //Get all but the first component of the hostname
    if((firstDot = strchr(pointerTopURL->hostname, '.')) != NULL) {
        firstDot = &(firstDot[1]);
        fdi = (int) (firstDot ? firstDot - pointerTopURL->hostname : -1);

        memcpy(pointerTopURL->allButFirstComponent, firstDot, strlen(pointerTopURL->hostname) - fdi);
        pointerTopURL->allButFirstComponent[strlen(pointerTopURL->hostname) - fdi] = NULL_BYTE_CHARACTER;
    } else {
        strcpy(pointerTopURL->allButFirstComponent,pointerTopURL->hostname);
        pointerTopURL->path[strlen(pointerTopURL->allButFirstComponent)] = NULL_BYTE_CHARACTER;
    }
}

/*
 *  Enqueue an URL to the queue
 */
void enqueueURL(char *URL) {
    URLInfo *TempPointer = malloc(sizeof(URLInfo));

    if(TempPointer == NULL) {
        return;
    }

    strcpy(TempPointer->fullURL,URL);
    TempPointer->nextNode = NULL;

    if(pointerBottomURL == NULL) {
        TempPointer = pointerBottomURL = TempPointer;
    }
    else {
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
void dequeueURL(URLInfo *toFetchURL) {

    if(pointerBottomURL == NULL) {
        return;
    }

    strcpy(toFetchURL->fullURL, pointerBottomURL->fullURL);
    strcpy(toFetchURL->hostname, pointerBottomURL->hostname);
    strcpy(toFetchURL->allButFirstComponent, pointerBottomURL->allButFirstComponent);
    strcpy(toFetchURL->path, pointerBottomURL->path);
    toFetchURL->refetchTimes = pointerBottomURL->refetchTimes;
    toFetchURL->needAuthorization = pointerBottomURL->needAuthorization;

    URLInfo *TempPointer = pointerBottomURL;
    pointerBottomURL = pointerBottomURL->nextNode;
    free(TempPointer);

    if(pointerBottomURL == NULL) {
        pointerTopURL = NULL;
    }
}


/*
 *  Function to check all the unique URLs that have been visited already
 */
int checkHistory(URLInfo * URLtoCheck) {
    uniqueURL * tempPointer = pointerToHistory;
    uniqueURL * newUniqueURL;

    //Check the URL against the existing history
    while(tempPointer != NULL) {
        if((strcmp(URLtoCheck->hostname,tempPointer->URLhostname) == 0)
                    && (strcmp(URLtoCheck->path,tempPointer->URLpath) == 0)) {

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

    while (pointerToHistory != NULL) {

        tempPointer = pointerToHistory;
        pointerToHistory = pointerToHistory->nextUniqueURL;
        free(tempPointer);
    }
}