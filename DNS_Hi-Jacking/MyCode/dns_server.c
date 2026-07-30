/*
 * dns_server.c
 *
 *  Created on: Apr 26, 2016
 *      Author: jiaziyi
 */


#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<stdbool.h>
#include<time.h>

#include "dns.h"

int main(int argc, char *argv[])
{
	int sockfd;
	struct sockaddr server;

	int port = 53; //the default port of DNS service


	//to keep the information received.
	res_record answers[ANS_SIZE], auth[ANS_SIZE], addit[ANS_SIZE];
	query queries[ANS_SIZE];


	if(argc == 2)
	{
		port = atoi(argv[1]); //if we need to define the DNS to a specific port
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	int enable = 1;

	if(sockfd <0 )
	{
		perror("socket creation error");
		exit_with_error("Socket creation failed");
	}

	//in some operating systems, you probably need to set the REUSEADDR
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
	{
	    perror("setsockopt(SO_REUSEADDR) failed");
	}

	//for v4 address
	struct sockaddr_in *server_v4 = (struct sockaddr_in*)(&server);
	server_v4->sin_family = AF_INET;
	server_v4->sin_addr.s_addr = htonl(INADDR_ANY);
	server_v4->sin_port = htons(port);

	//bind the socket
	if(bind(sockfd, &server, sizeof(*server_v4))<0){
		perror("Binding error");
		exit_with_error("Socket binding failed");
	}

	printf("The dns_server is now listening on port %d ... \n", port);
	//print out
	uint8_t buf[BUF_SIZE], send_buf[BUF_SIZE]; //receiving buffer and sending buffer
	struct sockaddr remote;
	int n;
	socklen_t addr_len = sizeof(remote);
	struct sockaddr_in *remote_v4 = (struct sockaddr_in*)(&remote);


	/*
	ssize_t recvfrom(int socket, void *restrict buffer, size_t length,
       int flags, struct sockaddr *restrict address,
       socklen_t *restrict address_len);
	*/


	while(1)
	{
		//an infinite loop that keeps receiving DNS queries and send back a reply
		//complete your code here
		// recvfrom(sockfd, (char*)buf, BUF_SIZE, 0, &remote, &addr_len);

		if(recvfrom(sockfd, buf, BUF_SIZE, 0, &remote, &addr_len) < 0){
			perror("Recvfrom error");
			exit_with_error("Recvfrom failed");
		}




		int query_id = parse_dns_query(buf, queries, answers, auth, addit);

		printf("Domain Name: %s\n", queries[0].qname);
		printf("Query ID: %d\n", query_id);

		// dns_header dnsresp;
		dns_header *dns = (dns_header *)send_buf;
		// answer = 1
		build_dns_header(dns, query_id, 1, 1, 1, 0, 0);



		u_int8_t *qname_ptr = &send_buf[sizeof(dns_header)];
		char *host_name = queries[0].qname; 

		int offset = 0;
		build_name_section(qname_ptr, host_name, &offset);

		//question qst;
		//qst.qclass = queries[0].ques->qclass;
		//qst.qtype = queries[0].ques->qtype;

		question *qst = (question*)(qname_ptr + offset);
		qst->qclass = queries[0].ques->qclass;
		qst->qtype = queries[0].ques->qtype;


		uint8_t *answer_ptr = (uint8_t *)(qst + 1);


		uint16_t *ans_name = (uint16_t *)answer_ptr;
		*ans_name = htons(0xC00C);

		r_element *ans_data = (r_element*)(answer_ptr + 2); // 2 bytes of ansname
		ans_data->type = htons(1);
		ans_data->ttl = htonl(1);
		ans_data->_class = htons(1);
		ans_data->rdlength = htons(4);
		

		u_int8_t ipaddress[4] = {151, 101, 129, 140}; //reddit ip
		//u_int8_t ipaddress[4] = {111, 111, 111, 111}; // ip address for every query
		//u_int8_t ipaddress[4] = {127, 0, 0, 1};
		u_int8_t *ip_ptr =  (u_int8_t*)(ans_data + 1);

		memcpy(ip_ptr, ipaddress, 4);


		int packet_size = ( ip_ptr + 4 ) - send_buf;


		if(sendto(sockfd, send_buf, packet_size, 0, (struct sockaddr*)&remote, addr_len) < 0){
			perror("sendto error");
			exit_with_error("sendto failed");
		}

		printf("Finished receive, construct, response\n\n");
		
	}
}
