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
#define NO_SOCKET_OPENED 0


typedef struct queueForURLs{
    char data[MAX_URL_SIZE];
    struct queueForURLs *nextNode;
} queueForURLs;

/* Function Prototypes */
void fetchWebPage(char URL[], int numberOfPagesFetched);
int parseHTTPHeaders(char *buffer);
void parseHTML(char buffer[]);
int checkIfValidURL(char possibleURL[]);
void enqueueURL(char *URL);
void printStack(void);
void dequeueURL(char *URL);




#endif //CODE_CRAWLER_H
