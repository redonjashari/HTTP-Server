#include "message_handle_tools.h"

// function protypes
void str_trim(char *str);
char *get_resource_type(char *resource);
int set_headers(struct message *resp, char *resource_type, int body_length);
char *read_resource(FILE *fp);
char *api_call(char *endpoint, char *data);
int GET_response(struct message *resp, char *uri);
int POST_response(struct message *req, struct message *resp, char *endpoint);

void message_cleanup(struct message *msg) {
    // free allocated memory for message components
    if (msg->line != NULL) {
        free(msg->line);
        msg->line = NULL;
    }
    if (msg->headers != NULL) {
        free(msg->headers);
        msg->headers = NULL;
    }
    if (msg->body != NULL) {
        free(msg->body);
        msg->body = NULL;
    }
}

// function to remove whitespace and newlines for a string
void str_trim(char *str) {
    int i = 0, j = strlen(str) - 1;
    
    while ((str[i] == ' ') || (str[i] == '\n')) {
        i++;
    }
    while (((str[j] == ' ') || (str[j] == '\n')) && j >= i) {
        j--;
    }

    for (int k = i; k <= j; k++) {
        str[k - i] = str[k];
    }
    
    str[j - i + 1] = '\0';
}

int parse_request(struct message *req, char *req_buff, int req_size) {
    // parse the request line of the request
    char *end_request_line = strstr(req_buff, "\r\n");
    if (end_request_line == NULL) {
        return -1;
    }
    int request_line_length = end_request_line - req_buff;
    req->line = (char *)malloc(request_line_length + 1);
    if (req->line == NULL) {
        perror("malloc");
        return -2;
    }
    strncpy(req->line, req_buff, request_line_length);
    req->line[request_line_length] = '\0';

    // parse the headers of the request
    if (strncmp(req->line, "GET", 3) == 0) {
        req->request_type = GET;
    }
    else {
        req->request_type = POST;
    }
    char *end_headers = strstr(end_request_line + 2, "\r\n\r\n");
    if (end_headers == NULL) {
        return -1;
    }
    int headers_length = end_headers - end_request_line - 2;
    req->headers = (char *)malloc(headers_length + 1);
    if (req->headers == NULL) {
        perror("malloc");
        return -2;
    }
    strncpy(req->headers, end_request_line + 2, headers_length);
    req->headers[headers_length] = '\0';

    // parse the body of the request
    int body_length = req_size - (end_headers - req_buff) - 4;
    req->body = (char *)malloc(body_length + 1);
    if (req->body == NULL) {
        perror("malloc");
        return -2;
    }
    strncpy(req->body, end_headers + 4, body_length);
    req->body[body_length] = '\0';

    return 0;
}

int create_response(struct message *req, struct message *resp, pthread_mutex_t *POST_lock) {
    // extract the request-uri from the request
    char *uri_start = strchr(req->line, ' ') + 1;
    char *uri_end = strrchr(req->line, ' ');
    int uri_length = uri_end - uri_start;
    char uri[uri_length + 1];
    strncpy(uri, uri_start, uri_length);
    uri[uri_length] = '\0';
    
    if (req->request_type == GET) {
        return GET_response(resp, uri);
    }
    else if (req->request_type == POST) {
        (void) pthread_mutex_lock(POST_lock);
        return POST_response(req, resp, uri);
        (void) pthread_mutex_unlock(POST_lock);

    }

    return 0;
}

int GET_response(struct message *resp, char *uri) {
    // extaract the requested resource
    char *resource = strrchr(uri, '/');
    resource++;

    // open the file
    char path[] = "server_resources";
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", path, resource);
    FILE *fp = fopen(filepath, "rb");

    // if the resource is not found, read not_found page instead
    int isNULL = 0;
    if (fp == NULL) {
        fp = fopen("server_resources/not_found.html", "rb");
        isNULL = 1;
    }

    // read the resource and get the content
    char *body = read_resource(fp);
    if (body == NULL) {
        return -1;
    }
    fclose(fp);
    int body_length = strlen(body);

    // set the body component
    resp->body = malloc(body_length + 1);
    strcpy(resp->body, body);
    resp->body[body_length] = '\0';
    free(body);

    // set the header component
    if (isNULL == 0) {
        char *resource_type = get_resource_type(resource);
        if (set_headers(resp, resource_type, body_length) == -1) {
            return -1;
        }
    }
    else {
        if (set_headers(resp, "text/html", body_length) == -1) {
            return -1;
        }
    }


    // set the line component
    if (isNULL == 0) {
        char *line = "HTTP/1.1 200 OK";
        resp->line = malloc(strlen(line) + 1);
        strcpy(resp->line, line);
        resp->line[strlen(line)] = '\0';
    }
    else {
        char *line = "HTTP/1.1 404 Not Found";
        resp->line = malloc(strlen(line) + 1);
        strcpy(resp->line, line);
        resp->line[strlen(line)] = '\0';
    }

    return 0;
}

char *read_resource(FILE *fp) {
    // get the length of the file
    fseek(fp, 0, SEEK_END);
    int content_length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // allocate memory for the content and read it
    char *content = malloc(content_length);
    if (content == NULL) {
        perror("malloc");
        fclose(fp);
        return NULL;
    }
    if (fread(content, 1, content_length, fp) != content_length) {
        perror("fread");
        fclose(fp);
        free(content);
        return NULL;
    }
    return content;
}

char *get_resource_type(char *resource) {
    char *extension = strchr(resource, '.');

    // depending on the file extension requested, return a different content-type
    if (extension != NULL) {
        if (strcmp(extension, ".html") == 0) {
            return "text/html";
        }
        else if (strcmp(extension, ".css") == 0) {
            return "text/css";
        }
        else if (strcmp(extension, ".txt") == 0) {
            return "text/plain";
        }
        else if ((strcmp(extension, ".jpeg") == 0) || (strcmp(extension, ".png") == 0)) {
            return "image/jpeg";
        }
    }

    return "application/octet-stream";
}

// set headers of a message
int set_headers(struct message *resp, char *resource_type, int body_length) {
    // get the potenial size
    int headers_length = snprintf(NULL, 0, "Content-Type: %s\r\n"
                                            "Server: flori/1.0\r\n"
                                            "Content-Length: %d",
                                            resource_type, body_length) + 1;
    if (headers_length < 0) {
        perror("snprintf");
        return -1;
    }     

    // allocate memory for the headers and put them into the message component
    resp->headers = malloc(headers_length);
    if (sprintf(resp->headers, "Content-Type: %s\r\n"
                               "Server: flori/1.0\r\n"
                               "Content-Length: %d", 
                               resource_type, body_length) == -1) {
        perror("sprintf");
        return -1;
    } 

    return 0;
}

int POST_response(struct message *req, struct message *resp, char *endpoint) {
    // get the content-type for the api endpoint call
    char *content_type = parse_key_value(req->headers, "Content-Type", "\r\n", ':');

    // just an example api, you can add your own one here
    char *body = api_call(endpoint, req->body);

    // if the endpoint does not exist, set an appropriate body
    int isNULL = 0;
    if (body == NULL) {
        body = strdup("API endpoint not implemented yet");
    }

    // set line component
    if (isNULL == 0) {
        char *line = "HTTP/1.1 201 Created";
        resp->line = malloc(strlen(line));
        strcpy(resp->line, line);
    }
    else {
        char *line = "HTTP/1.1 501 Not Implemented";
        resp->line = malloc(strlen(line));
        strcpy(resp->line, line); 
    }

    int body_length = strlen(body);
    // set the header component
    if (isNULL == 0) {
        if (set_headers(resp, content_type, body_length) == -1) {
            return -1;
        }
    }
    else {
        if (set_headers(resp, "text/plain", body_length) == -1) {
            return -1;
        }
    }

    resp->body = malloc(body_length);
    strcpy(resp->body, body);

    free(content_type);
    free(body);
    return 0;
}

// function to parse the headers and the JSON data from the api example call
char *parse_key_value(char *pairs, char *target, char *pair_seperator, char key_value_seperator) {
    char *local_pairs = strdup(pairs);
    str_trim(local_pairs);

    char *local_pair = strtok(local_pairs, pair_seperator);
    char *key, *value;

    while (local_pair != NULL) {
        str_trim(local_pair);
        char *key_end = strchr(local_pair, key_value_seperator) -1;
        key = malloc(key_end - local_pair + 1);
        strncpy(key, local_pair, key_end - local_pair + 1);
        
        if (strncmp(key, target, strlen(target)) == 0) {
            char *value_start = local_pair + strlen(key) + 1;
            char *value_end = local_pair + strlen(local_pair);
            value = malloc(value_end - value_start);
            strncpy(value, value_start, value_end - value_start);
            str_trim(value); 
            free(key);
            free(local_pairs);

            return value;
        }

        local_pair = strtok(NULL, pair_seperator);
    }
    
    free(key);
    free(local_pairs);

    return NULL;
}

// function to handle API calls
char *api_call(char *endpoint, char *data) {

    if (strcmp(endpoint, "create-user")) {
        return create_user(data);
    }
    else if (strcmp(endpoint, "some_other_endpoint")) {
        // add your own API endpoint here
    }

    return NULL;
}