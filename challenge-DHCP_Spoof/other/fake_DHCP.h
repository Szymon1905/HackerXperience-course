/*
 * DHCP_starvation.h
 *
 */

#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>


// Some structures from :
// - https://github.com/apple/darwin-xnu/blob/main/bsd/netinet/dhcp.h

/*
struct udphdr {
  u_int16_t	source;
  u_int16_t	dest;
  u_int16_t	len;
  u_int16_t	check;
};


struct iphdr {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int ihl:4;
    unsigned int version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
    unsigned int version:4;
    unsigned int ihl:4;
#else
# error	"Please fix <bits/endian.h>"
#endif
    u_int8_t tos;
    u_int16_t tot_len;
    u_int16_t id;
    u_int16_t frag_off;
    u_int8_t ttl;
    u_int8_t protocol;
    u_int16_t check;
    u_int32_t saddr;
    u_int32_t daddr;
};
*/
// Here is the header information of DHCP ;
struct dhcp_packet {
	u_char dp_op;      /* packet opcode type */
	u_char dp_htype;   /* hardware addr type */
	u_char dp_hlen;    /* hardware addr length */
	u_char dp_hops;    /* gateway hops */
	u_int32_t dp_xid;     /* transaction ID */
	u_int16_t dp_secs;    /* seconds since boot began */
	u_int16_t dp_flags;   /* flags */
	struct in_addr dp_ciaddr;  /* client IP address */
	struct in_addr dp_yiaddr;  /* 'your' IP address */
	struct in_addr dp_siaddr;  /* server IP address */
	struct in_addr dp_giaddr;  /* gateway IP address */
	u_char dp_chaddr[16];  /* client hardware address */
	u_char dp_sname[64];  /* server host name */
	u_char dp_file[128];  /* boot file name */
	u_char dp_options[312];  /* variable-length options field */
};

#define CLIENT_PORT 68
#define SERVER_PORT 67

int create_socket();
int send_offer(int sockfd);
int send_ack(int sockfd);
int config_packet( struct dhcp_packet *sent_packet , int type_message );
void print_packet(struct dhcp_packet packet);




/*
struct dhcp_packet { //udp packet
	struct iphdr ip;
	struct udphdr udp;
	struct dhcp dhcp;
};
*/
