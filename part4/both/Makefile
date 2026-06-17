TARGETS = pcap_example rawip_example


all: $(TARGETS)

pcap_example: pcap_example.o header.o
	gcc -g -Wall -o  pcap_example pcap_example.o header.o -lpcap

rawip_example: rawip_example.o header.o
	gcc -g -Wall -o  rawip_example rawip_example.o header.o -lpcap

%.o : %.c
	gcc -g -Wall -c $<
	
clean:
	rm -f $(TARGETS) *.o


