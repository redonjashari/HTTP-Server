#include "message_handle_tools.h"

// function to remove the curly brackets from the JSON data
char *clean_json(char *json) {
    char *cleaned_json;
    cleaned_json = malloc(100);

    int len = strlen(json);
    int j = 0;

    for (int i = 0; i < len; i++) {
        if (json[i] != '{' && json[i] != '}') {
            cleaned_json[j] = json[i];
            j++;
        }
    }
    cleaned_json[j] = '\0';

    return cleaned_json;
}

char *create_user(char *data) {
    FILE *fp = fopen("server_resources/users.txt", "a");

    char *cleaned_json = clean_json(data);
    char *username = parse_key_value(cleaned_json, "\"username\"", ",", ':');
    char *password = parse_key_value(cleaned_json, "\"password\"", ",", ':');

    free(cleaned_json);
    fprintf(fp, "[ %s : %s ]\n", username, password);
    fclose(fp);

    char *body = malloc(256);
    snprintf(body, 256, "{\"message\": \"User created successfully\", \"username\": %s}", username);

    return body;
}
