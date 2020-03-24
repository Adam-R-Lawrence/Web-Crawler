//
// Created by Arlawrence on 22/03/2020.
//

#ifndef CODE_CRAWLER_H
#define CODE_CRAWLER_H

/* Defines */
#define PORT 80
#define HTTP_REQUEST_HEADER "GET /%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: arlawrence\r\n\r\n"
#define MAX_URL_LENGTH 1000
#define MAX_NUMBER_OF_PAGES_FETCHED 100

/* Function Prototypes */
void fetchWebPage(char URL[], int numberOfPagesFetched);
void parseHTTPHeaders(int socketFD);
void parseHTML(int socketFD);
void checkIfValidURL(char URL[]);





#endif //CODE_CRAWLER_H
