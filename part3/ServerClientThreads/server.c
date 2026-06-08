#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define N 2
#define DELAY 1 // in seconds
#define BUFFER_SIZE 1024

typedef struct {
    int socket;
    char buffer[BUFFER_SIZE];
    int bytes_received;
    struct sockaddr_in client_address;
    socklen_t client_len;
} request_t;

int active_threads = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t limit_cond = PTHREAD_COND_INITIALIZER;

void* handle_request(void* arg) {
    pthread_detach(pthread_self()); // so OS will free thread when it ends

    request_t *request = (request_t*)arg;

    // data processing delay
    sleep(DELAY);
    //usleep(1000000);

    if (sendto(request->socket,
        request->buffer,
        request->bytes_received,
        0,
        (struct sockaddr *)&request->client_address,
        request->client_len
        ) < 0) {
        printf("Worker sendto failed \n");
    }


    pthread_mutex_lock(&count_mutex);
    active_threads--;
    printf("Worker thread finished. Active threads: %d/%d\n", active_threads, N);


    pthread_cond_signal(&limit_cond);
    pthread_mutex_unlock(&count_mutex);
    free(request);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s PORT_NUMBER\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int socketUDP = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketUDP < 0) {
        printf("Socket creation failed \n");
        return 1;
    }

    // address to listen on
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // for listen on network interface
    server_addr.sin_port = htons(port); // for listen on the provided port

    // bind to port
    if (bind(socketUDP, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Bind failed \n");
        close(socketUDP);
        return 1;
    }

    //char buffer[BUFFER_SIZE];
    //socklen_t client_len = sizeof(client_addr);

    // loop for echo
    while (1) {

        request_t *req = malloc(sizeof(request_t));
        if (req == NULL) continue;


        req->socket = socketUDP;
        req->client_len = sizeof(req->client_address);

        req->bytes_received = recvfrom(socketUDP, req->buffer, BUFFER_SIZE - 1, 0,
                                       (struct sockaddr *)&req->client_address, &req->client_len);

        if (req->bytes_received <= 0) {
            free(req);
            continue;
        }

        //int bytes_received = recvfrom(socketUDP, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);

        req->buffer[req->bytes_received] = '\0'; // null terminator

        pthread_mutex_lock(&count_mutex);


        while (active_threads >= N) {
            printf("M Server full (%d/%d). Pausing intake...\n", active_threads, N);
            pthread_cond_wait(&limit_cond, &count_mutex);
        }

        active_threads++;
        printf("M Created worker thread. Active threads: %d/%d\n", active_threads, N);
        pthread_mutex_unlock(&count_mutex);

        // worker thread creation
        pthread_t worker_thread;
        if (pthread_create(&worker_thread, NULL, handle_request, (void*)req) != 0) {
            printf("Failed to create worker thread \n");

            pthread_mutex_lock(&count_mutex);
            active_threads--;
            pthread_mutex_unlock(&count_mutex);
            free(req);
        }

    }
    // close(socketUDP);
    // return 0;
}