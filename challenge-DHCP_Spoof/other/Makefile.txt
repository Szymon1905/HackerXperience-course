all: DHCP_starvation fake_DHCP


DHCP_starvation.o: DHCP_starvation.c
	cc -c -Wall -g DHCP_starvation.c


DHCP_starvation: DHCP_starvation.o
	cc -g -o DHCP_starvation DHCP_starvation.o


fake_DHCP.o: fake_DHCP.c
	cc -c -Wall -g fake_DHCP.c

fake_DHCP: fake_DHCP.o
	cc -g -o fake_DHCP fake_DHCP.o


clean:
	rm DHCP_starvation.o DHCP_starvation
	rm fake_DHCP.o fake_DHCP
