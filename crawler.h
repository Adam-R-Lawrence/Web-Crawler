//
// Created by arlawrence on 22/03/2020.
//

#ifndef CODE_CRAWLER_H
#define CODE_CRAWLER_H

/* Define Constants */
#define PORT 80
#define HTTP_REQUEST_HEADER "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n"
#define REQUEST_WITH_AUTHORIZATION "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\nAuthorization: Basic YXJsYXdyZW5jZTpwYXNzd29yZA==\r\n\r\n"
#define USERNAME "arlawrence"
#define MAX_NUMBER_OF_PAGES_FETCHED 100
#define IS_VALID_URL 1
#define IS_NOT_VALID_URL 0
#define EXCLAMATION_MARK '!'
#define HASHTAG '#'
#define URL_ENCODED_CHARACTER '%'
#define MAX_URL_SIZE 1000
#define END_OF_HTTP_HEADER "\r\n\r\n"
#define CONTENT_LENGTH "Content-Length:"
#define CONTENT_TYPE "Content-Type:"
#define LOCATION_HEADER "Location:"
#define STATUS_CODE "HTTP/1.1 "
#define VALID_MIME_TYPE "text/html"
#define NULL_BYTE_CHARACTER '\0'
#define NULL_BYTE 1
#define SEND_BUFFER_LENGTH 2100
#define RECEIVED_BUFFER_LENGTH 3000
#define NO_SOCKET_OPENED 0

/* Define Enumerations */
enum boolean {FALSE, TRUE};


/* Struct Typedefs */
typedef struct uniqueURL {
    char URLhostname[MAX_URL_SIZE + NULL_BYTE];
    char URLpath[MAX_URL_SIZE + NULL_BYTE];
    struct uniqueURL *nextUniqueURL;
} uniqueURL;

typedef struct URLInfo {
    char fullURL[MAX_URL_SIZE + NULL_BYTE];
    char hostname[MAX_URL_SIZE + NULL_BYTE];
    char allButFirstComponent[MAX_URL_SIZE + NULL_BYTE];
    char path[MAX_URL_SIZE + NULL_BYTE];
    int needAuthorization;
    int refetchTimes;
    struct URLInfo *nextNode;
} URLInfo;


/* Function Prototypes */
void parseHTML(char buffer[], URLInfo *currentURL);
int checkIfValidURL(char possibleURL[]);
void enqueueURL(char *URL);
void dequeueURL(URLInfo *toFetchURL);
void parseURL(URLInfo * currentURL);
int checkHistory(URLInfo * URLtoCheck);
void clearHistory();


#endif //CODE_CRAWLER_H
