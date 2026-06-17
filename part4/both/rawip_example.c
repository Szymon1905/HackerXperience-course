/*
 * rawip_example.c
 *
 *  Created on: May 4, 2016
 *      Author: jiaziyi
 */


#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include "header.h"

#define SRC_IP  "192.168.1.111" //set your source ip here. It can be a fake one
#define SRC_PORT 54321 //set the source port here. It can be a fake one

#define DEST_IP "127.0.0.1" //set your destination ip here
#define DEST_PORT 4200 //set the destination port here
#define TEST_STRING "test data" //a test string as packet payload

int main(int argc, char *argv[])
{
	char source_ip[] = SRC_IP;
	char dest_ip[] = DEST_IP;


	int fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

    int hincl = 1;                  /* 1 = on, 0 = off */
    setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &hincl, sizeof(hincl));

	if(fd < 0)
	{
		perror("Error creating raw socket ");
		exit(1);
	}

	char packet[65536], *data;
	char data_string[] = TEST_STRING;
	memset(packet, 0, 65536);

	//IP header pointer
	struct iphdr *iph = (struct iphdr *)packet;

	//UDP header pointer
	struct udphdr *udph = (struct udphdr *)(packet + sizeof(struct iphdr));
	struct pseudo_udp_header psh; //pseudo header

	//data section pointer
	data = packet + sizeof(struct iphdr) + sizeof(struct udphdr);

	//fill the data section
	strncpy(data, data_string, strlen(data_string));

	//fill the IP header here
	iph->version = 4;
	iph->ihl = 5;
	iph->tos = 0;
	iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(data_string));
	
	iph->id = htons(50000);
	iph->frag_off = 0;
	iph->ttl = 255;
	iph->protocol = IPPROTO_UDP;
	
	iph->saddr = inet_addr(source_ip);
	iph->daddr = inet_addr(dest_ip);
	iph->check = 0;
	iph->check = checksum((unsigned short*)packet, iph->ihl * 4);
	//fill the UDP header
	udph->source = htons(SRC_PORT);
	udph->dest = htons(DEST_PORT);
	udph->len = htons(sizeof(struct udphdr) + strlen(data_string));
	udph->check = 0;
	
	psh.source_address = inet_addr(source_ip);
	psh.dest_address = inet_addr(dest_ip);
	psh.placeholder = 0;
	psh.protocol = IPPROTO_UDP;
	psh.udp_length = htons(sizeof(struct udphdr) + strlen(data_string));
	
	int packet_size = sizeof(struct pseudo_udp_header) + sizeof(struct udphdr) + strlen(data_string);
	
	char *pseudogram = malloc(packet_size);
	
	memcpy(pseudogram, (char *)&psh, sizeof(struct pseudo_udp_header));
	memcpy(pseudogram + sizeof(struct pseudo_udp_header), udph, sizeof(struct udphdr) + strlen(data_string));
	
	udph->check = checksum((unsigned short*)pseudogram, packet_size);
	free(pseudogram);
	
	//send the packet
	struct sockaddr_in socketin;
	socketin.sin_family = AF_INET;
	socketin.sin_port = htons(DEST_PORT);
	socketin.sin_addr.s_addr = inet_addr(dest_ip);
	
	int send_len = sendto(fd, packet,ntohs(iph->tot_len), 0, (struct sockaddr *)&socketin, sizeof(socketin));
	
	if (send_len < 0){
		printf("Error\n");
	} else {
		printf("Success\n");
	}
	
	return 0;

}
