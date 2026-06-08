#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

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
    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // for listen on any network interface
    server_addr.sin_port = htons(port); // for listen on the provided port

    // bind to port
    if (bind(socketUDP, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Bind failed \n");
        close(socketUDP);
        return 1;
    }

    char buffer[BUFFER_SIZE];
    socklen_t client_len = sizeof(client_addr);

    // loop for echo
    while (1) {
        // waiting in loop receive a datagram
        // sender address saved to client_addr
        int bytes_received = recvfrom(socketUDP, buffer, BUFFER_SIZE, 0,
                                      (struct sockaddr *)&client_addr, &client_len);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0'; // null terminator
            printf("Received data: %s", buffer);
            // bytes_received send back to the client_addr
            if (sendto(socketUDP, buffer, bytes_received, 0, (struct sockaddr *)&client_addr, client_len) < 0) {
                printf("Send data failed \n");
                continue; // See the Hacker Tip below!
            }
        }
    }

    // close(socketUDP);
    // return 0;
}