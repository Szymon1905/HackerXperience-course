#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s IP_ADDRESS PORT_NUMBER\n", argv[0]);
        return 1;
    }

    char *ip_address = argv[1];
    int port = atoi(argv[2]); // str to int
    int socketUDP = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketUDP < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // // destination
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // reset
    server_addr.sin_family = AF_INET; // ipv4
    server_addr.sin_port = htons(port); // port to netowrk byte order

    // ip from string to binary form
    if (inet_pton(AF_INET, ip_address, &server_addr.sin_addr) <= 0) {
        printf("Wrong IP format \n");
        close(socketUDP);
        return 1;
    }

    char buffer[BUFFER_SIZE];


    while (1) {

        printf("Type text to send: ");
        

        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            // Ctrl+D (EOF), break
            printf("\n Exit ctrl+D\n");
            break;
        }


        int message_len = strlen(buffer);

        //socket, message buffer, message len, flags, destination struct, struct size
        int bytes_sent = sendto(socketUDP, buffer, message_len, 0,
                                (struct sockaddr *)&server_addr, sizeof(server_addr));
                                
        if (bytes_sent < 0) {
            printf("UDP error \n");
        }
    }

    close(socketUDP);
    return 0;
}