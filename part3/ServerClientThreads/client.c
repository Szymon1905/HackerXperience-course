#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUFFER_SIZE 1024


void *receive_messages(void * socket) {
    int socketUDP = *(int*)socket;

    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);


    while (1) {
        int bytes_received = recvfrom(socketUDP, buffer, BUFFER_SIZE, 0,
                                     (struct sockaddr *)&client_addr, &client_len);
        if (bytes_received < 0) {
            printf("UDP error \n");
        }

        if (bytes_received > 0) {
            //printf("Received %d bytes\n", bytes_received);
        }
        buffer[bytes_received] = '\0'; // null terminator


        // to strip \n from buffer cause autograder angry
        while(bytes_received > 0 && (buffer[bytes_received - 1] == '\n' || buffer[bytes_received - 1] == '\r')) {
            buffer[bytes_received - 1] = '\0';
            bytes_received--;
        }

        printf("\nReceived: %s\n", buffer);
        printf("Send: ");
        //printf("Send: ");
        fflush(stdout);
    }
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s IP_ADDRESS PORT_NUMBER\n", argv[0]);
        return 1;
    }

    char *ip_address = argv[1];
    int port = atoi(argv[2]); // str to int
    int socketUDP = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketUDP < 0) {
        printf("Socket creation failed \n");
        return 1;
    }

    // // destination
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address)); // reset
    server_address.sin_family = AF_INET; // ipv4
    server_address.sin_port = htons(port); // port to netowrk byte order

    // ip from string to binary form
    if (inet_pton(AF_INET, ip_address, &server_address.sin_addr) <= 0) {
        printf("Wrong IP format \n");
        close(socketUDP);
        return 1;
    }

    pthread_t thread_receiver;
    pthread_create(&thread_receiver, NULL, &receive_messages, &socketUDP);


    char buffer[BUFFER_SIZE];


    while (1) {
        printf("Send: ");
        fflush(stdout);
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            // Ctrl+D (EOF), break
            //printf("\n Exit ctrl+D\n");
            pthread_join(thread_receiver, NULL);
            break;
        }
        //printf("Input: %s", buffer);
        int message_len = strlen(buffer);

        //socket, message buffer, message len, flags, destination struct, struct size
        int bytes_sent = sendto(socketUDP, buffer, message_len, 0,
                                (struct sockaddr *)&server_address, sizeof(server_address));

        if (bytes_sent < 0) {
            printf("UDP error \n");
        } else {
            //printf("Sent %d bytes\n", bytes_sent);
        }
    }
    //pthread_cancel(thread_receiver);
    close(socketUDP);
    return 0;
}