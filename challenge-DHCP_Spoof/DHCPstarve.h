#ifndef DHCP_STARVE_H
#define DHCP_STARVE_H

#include <stdint.h>
#include <netinet/in.h>


/*
DHCP packet structure based on the BOOTP/DHCP
based on RFC 2131
custom for direct access to DHCP fields
 */
struct dhcp_packet{
    uint8_t  op; // Message type - request/reply
    uint8_t  htype; // Hardware address type
    uint8_t  hlen; // Hardware address len
    uint8_t  hops; // Relay hops
    uint32_t xid; // Transaction ID
    uint16_t secs; // Seconds elapsed
    uint16_t flags; // DHCP flags
    struct in_addr ciaddr; // Client IP 
    struct in_addr yiaddr; // Assigned IP 
    struct in_addr siaddr; // DHCP server IP 
    struct in_addr giaddr; // Relay gateway IP
    uint8_t chaddr[16]; // MAC Client hardware address
    uint8_t sname[64]; // Optional server hostname
    uint8_t file[128]; // Boot file name
    uint8_t options[400]; // DHCP options, fixed buffer size for simplicity
};


#define DHCP_CLIENT_PORT 68
#define DHCP_SERVER_PORT 67


int create_socket(void);

int dhcp_discover(int sockfd);

int check_response(int sockfd);

void print_packet(const struct dhcp_packet packet);

int send_packets(int sockfd);


#endif