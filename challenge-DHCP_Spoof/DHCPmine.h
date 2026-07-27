#ifndef DHCPFAKE_H
#define DHCPFAKE_H

#include <stdint.h>
#include <netinet/in.h>


#define DHCP_CLIENT_PORT 68
#define DHCP_SERVER_PORT 67


#define DHCP_DISCOVER 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_ACK 5



struct dhcp_packet{
    uint8_t  op;
    uint8_t  htype;
    uint8_t  hlen;
    uint8_t  hops;

    uint32_t xid;

    uint16_t secs;
    uint16_t flags;

    struct in_addr ciaddr;
    struct in_addr yiaddr;
    struct in_addr siaddr;
    struct in_addr giaddr;

    uint8_t chaddr[16];

    uint8_t sname[64];
    uint8_t file[128];
    uint8_t options[312];
};



int create_socket(void);

int receive_discover(int sockfd);

int send_offer(int sockfd);

int receive_request(int sockfd);

int send_ack(int sockfd);


void create_dhcp_reply(struct dhcp_packet *packet, int message_type);


void print_packet(const struct dhcp_packet packet);


#endif