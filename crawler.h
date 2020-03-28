//
// Created by Arlawrence on 22/03/2020.
//

#ifndef CODE_CRAWLER_H
#define CODE_CRAWLER_H

/* Defines */
#define PORT 80
#define HTTP_REQUEST_HEADER "GET /%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: arlawrence\r\n\r\n"
#define MAX_NUMBER_OF_PAGES_FETCHED 100
#define IS_VALID 1
#define NOT_VALID 0
#define EXCLAMATION_MARK '!'
#define HASHTAG '#'
#define URL_ENCODED_CHARACTER '%'
#define MAX_URL_SIZE 1000
#define END_OF_HTTP_HEADER "\r\n\r\n"
#define CONTENT_LENGTH "Content-Length: "
#define CONTENT_TYPE "Content-Type: "
#define STATUS_CODE "HTTP/1.1 "
#define VALID_MIME_TYPE "text/html"
#define NULL_BYTE_CHARACTER '\0'
#define NULL_BYTE 1
#define SEND_BUFFER_LENGTH 2000
#define RECEIVED_BUFFER_LENGTH 3000
#define TRUE 1
#define FALSE 0


#define NO_SOCKET_OPENED 0


typedef struct queueForURLs{
    char data[MAX_URL_SIZE + NULL_BYTE];
    struct queueForURLs *nextNode;
} queueForURLs;

typedef struct URLInfo {
    char fullURL[MAX_URL_SIZE + NULL_BYTE];
    char hostname[MAX_URL_SIZE + NULL_BYTE];
    char firstComponentOfHostname[MAX_URL_SIZE + NULL_BYTE];
    char path[MAX_URL_SIZE + NULL_BYTE];
    struct URLInfo *nextNode;
} URLInfo;

/* Function Prototypes */
int parseHTTPHeaders(char *buffer);
void parseHTML(char buffer[]);
int checkIfValidURL(char possibleURL[]);
void enqueueURL(char *URL);
void printStack(void);
void dequeueURL(URLInfo *toFetchURL);
void parseURL();




#endif //CODE_CRAWLER_H
