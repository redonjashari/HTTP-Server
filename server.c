#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_REQUEST_SIZE 1024
#define PORT 8080
#define NR_OF_THREADS 30

struct args {
    int client_socket;
    pthread_mutex_t POST_lock;
};


// Function to handle client connection
static void *handle_connection(void *arg) {
    // Logic for handling connection will be implemented in next push
}

int main(void) {
    // create the socket
    int main_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (main_socket == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }
    puts("Socket Created!");

    int reuse = 1;
    if (setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        perror("setsockopt");
        close(main_socket);
        return EXIT_FAILURE;
    }

    // set up port and address for the socket
    struct sockaddr_in host;
    socklen_t host_len = sizeof(host);

    host.sin_family = AF_INET;
    host.sin_port = htons(PORT);
    host.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind the socket to that port and address
    if (bind(main_socket, (struct sockaddr *) &host, host_len) == -1) {
        perror("bind");
        close(main_socket);
        return EXIT_FAILURE;
    }
    puts("Socket Binded!");

    while (1) {
        // wait for connection request
        if (listen(main_socket, NR_OF_THREADS) == -1) {
            perror("listen");
            close(main_socket);
            return EXIT_FAILURE;
        }
        
        // accept the connection request
        int client_socket = accept(main_socket, (struct sockaddr *)&host, &host_len);
        if (client_socket == -1) {
            perror("accept");
            close(main_socket);
            return EXIT_FAILURE;
        }
        puts("Connection accepted!");

        struct args args = { .client_socket = client_socket, .POST_lock = PTHREAD_MUTEX_INITIALIZER };

        // create the the thread
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_connection, &args)) {
            perror("pthread_create");
            close(client_socket);
            return EXIT_FAILURE;
        }

        // join it back
        if (pthread_detach(tid)) {  
            perror("pthread_detach");
            close(client_socket);
            return EXIT_FAILURE;
        }
    }

    close(main_socket);
    return EXIT_SUCCESS;
}
