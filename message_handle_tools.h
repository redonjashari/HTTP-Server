#ifndef MESSAGE_HANDLE_TOOLS
#define MESSAGE_HANDLE_TOOLS

// some shared libraries from server.c and the message_handle_tools.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef enum {
    GET, 
    POST
} request_types;

struct message {
    char *line;
    char *headers;
    char *body;
    request_types request_type;
};

// functions shared betwen 'server.c' and 'message_handle_tools.c'
int parse_request(struct message *req, char *req_buff, int req_size);
void message_cleanup(struct message *msg);
int create_response(struct message *req, struct message *resp, pthread_mutex_t *POST_lock);

// functions shared betwen 'api_example.c' and 'message_handle_tools.c'
int parse_request(struct message *req, char *req_buff, int req_size);
char *parse_key_value(char *pairs, char *target, char *pair_seperator, char key_value_seperator);

// add api endpoint function prototypes here
char *create_user(char *data);

#endif
