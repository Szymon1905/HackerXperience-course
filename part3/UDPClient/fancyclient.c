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
        printf("Socket creation failed \n");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip_address, &server_addr.sin_addr);

    char buffer[BUFFER_SIZE];

    while (1) {
        printf("Type text to send: ");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            // Ctrl+D (EOF), break
            printf("\n Exit ctrl+D\n");
            break;
        }
        //printf("Input: %s", buffer);
        int message_len = strlen(buffer);

        int statussend = sendto(socketUDP, buffer, message_len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (statussend < 0) {
            printf("UDP error \n");
            return 1;
        }
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        
        int bytes_received = recvfrom(socketUDP, buffer, BUFFER_SIZE - 1, 0,
                                      (struct sockaddr *)&from_addr, &from_len);

        if (bytes_received < 0) {
            printf("UDP error \n");
            return 1;
        }

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0'; // null terminator
            printf("Received data: %s", buffer);
        }
    }

    close(socketUDP);
    return 0;
}