/**
 *  Jiazi Yi
 * LIX, Ecole Polytechnique
 * jiazi.yi@polytechnique.edu
 *
 * Updated by Pierre Pfister
 * Cisco Systems
 * ppfister@cisco.com
 *
 * Updated by Kevin Jiokeng
 * LIX, Ecole Polytechnique
 * kevin.jiokeng@polytechnique.edu
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "url.h"
#include "wgetX.h"

int main(int argc, char* argv[]) {
    url_info info;
    const char * file_name = "received_page";
    if (argc < 2) {
	fprintf(stderr, "Missing argument. Please enter URL.\n");
	return 1;
    }

    char *url = argv[1];

    // Get optional file name
    if (argc > 2) {
	file_name = argv[2];
    }

    // First parse the URL
    int ret = parse_url(url, &info);
    if (ret) {
	fprintf(stderr, "Could not parse URL '%s': %s\n", url, parse_url_errstr[ret]);
	return 2;
    }

    //If needed for debug
    print_url_info(&info);

    // Download the page
    struct http_reply reply;

    ret = download_page(&info, &reply);
    if (ret) {
	return 3;
    }

    // Now parse the responses
    char *response = read_http_reply(&reply);
    if (response == NULL) {
	fprintf(stderr, "Could not parse http reply\n");
	return 4;
    }

    // Write response to a file
    write_data(file_name, response, reply.reply_buffer + reply.reply_buffer_length - response);

    // Free allocated memory
    free(reply.reply_buffer);

    // Just tell the user where is the file
    fprintf(stderr, "the file is saved in %s. \n", file_name);
    return 0;
}

int download_page(url_info *info, http_reply *reply) {

    /*
     * To be completed:
     *   You will first need to resolve the hostname into an IP address.
     *
     *   Option 1: Simplistic
     *     Use gethostbyname function.
     *
     *   Option 2: Challenge
     *     Use getaddrinfo and implement a function that works for both IPv4 and IPv6.
     *
     */
	struct hostent *he;
	he = gethostbyname(info->host);
	if (he == NULL) {
		printf("DNS resolve fail for host: %s\n", info->host);
		return 1;
	}
	printf("Hostname: %s\n", info->host);



    /*
     * To be completed:
     *   Next, you will need to send the HTTP request.
     *   Use the http_get_request function given to you below.
     *   It uses malloc to allocate memory, and snprintf to format the request as a string.
     *
     *   Use 'write' function to send the request into the socket.
     *
     *   Note: You do not need to send the end-of-string \0 character.
     *   Note2: It is good practice to test if the function returned an error or not.
     *   Note3: Call the shutdown function with SHUT_WR flag after sending the request
     *          to inform the server you have nothing left to send.
     *   Note4: Free the request buffer returned by http_get_request by calling the 'free' function.
     *
     */
	// socket
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Socket error");
		return 2;
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr)); // reset
	server_addr.sin_family = AF_INET; // IPv4
	server_addr.sin_port = htons(info->port); // port set
	server_addr.sin_addr = *((struct in_addr *)he->h_addr_list[0]); // resolved IP


	if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		printf("Connection fail");
		return 3;
	}


    /*
     * To be completed:
     *   Now you will need to read the response from the server.
     *   The response must be stored in a buffer allocated with malloc, and its address must be save in reply->reply_buffer.
     *   The length of the reply (not the length of the buffer), must be saved in reply->reply_buffer_length.
     *
     *   Important: calling recv only once might only give you a fragment of the response.
     *              in order to support large file transfers, you have to keep calling 'recv' until it returns 0.
     *
     *   Option 1: Simplistic
     *     Only call recv once and give up on receiving large files.
     *     BUT: Your program must still be able to store the beginning of the file and
     *          display an error message stating the response was truncated, if it was.
     *
     *   Option 2: Challenge
     *     Do it the proper way by calling recv multiple times.
     *     Whenever the allocated reply->reply_buffer is not large enough, use realloc to increase its size:
     *        reply->reply_buffer = realloc(reply->reply_buffer, new_size);
     *
     *
     */

	char *request = http_get_request(info);

	// string to socket
	if (write(sockfd, request, strlen(request)) < 0) {
		printf("Failed to send the HTTP request \n");
		free(request);
		return 4;
	}

	free(request);
	shutdown(sockfd, SHUT_WR); // shut so server can start replying



	int allocated_size = 4096;

	// initial memory bucket
	reply->reply_buffer = (char *)malloc(allocated_size);
	if (reply->reply_buffer == NULL) {
		printf("Failed to allocate initial memory \n");
		close(sockfd);
		return 5;
	}

	reply->reply_buffer_length = 0;
	int bytes_received;

	// Loop to keep reading until the server closes the connection (returns 0)
	// recv() args: (Socket, where to write data, how much empty space is left, flags)
	// https://man7.org/linux/man-pages/man2/recv.2.html
	// X bytes received
	// 0 ok connectin end
	// -1 error

	while ((bytes_received = recv(sockfd,
								  reply->reply_buffer + reply->reply_buffer_length,
								  allocated_size - reply->reply_buffer_length,
								  0)) > 0)
	{
		// tracks bytes received
		reply->reply_buffer_length += bytes_received;

		// bucket fill check
		if (reply->reply_buffer_length == allocated_size) {
			// to double capactiy for realloc
			allocated_size *= 2;
			//allocated_size *= 1;
			char *new_buffer = (char *)realloc(reply->reply_buffer, allocated_size);
			if (new_buffer == NULL) {
				printf("Realloc error \n");
				free(reply->reply_buffer);
				close(sockfd);
				return 6;
			}
			reply->reply_buffer = new_buffer; // update for pointer to the larger bucket
		}
	}


	if (bytes_received < 0) { // check for recv error
		printf("Recv error \n");
		free(reply->reply_buffer);
		close(sockfd);
		return 7;
	}

	close(sockfd);

    return 0;
}

void write_data(const char *path, const char * data, int len) {
    /*
     * To be completed:
     *   Use fopen, fwrite and fclose functions.
     */
	// wb - binary mode
	FILE *f = fopen(path, "wb");
	if (f == NULL) {
		printf("Failed to open file \n");
		return;
	}
	int bytes_written = fwrite(data, 1, len, f);
	printf("Write bytes_written %d / %d \n", bytes_written, len);

	fclose(f);
}


char* http_get_request(url_info *info) {
    char * request_buffer = (char *) malloc(100 + strlen(info->path) + strlen(info->host));
    snprintf(request_buffer, 1024, "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
	    info->path, info->host);
    return request_buffer;
}

char *next_line(char *buff, int len) {
    if (len == 0) {
	return NULL;
    }

    char *last = buff + len - 1;
    while (buff != last) {
	if (*buff == '\r' && *(buff+1) == '\n') {
	    return buff;
	}
	buff++;
    }
    return NULL;
}

char *read_http_reply(struct http_reply *reply) {

    // Let's first isolate the first line of the reply
    char *status_line = next_line(reply->reply_buffer, reply->reply_buffer_length);
    if (status_line == NULL) {
	fprintf(stderr, "Could not find status\n");
	return NULL;
    }
    *status_line = '\0'; // Make the first line is a null-terminated string

    // Now let's read the status (parsing the first line)
    int status;
    double http_version;
    int rv = sscanf(reply->reply_buffer, "HTTP/%lf %d", &http_version, &status);
    if (rv != 2) {
	fprintf(stderr, "Could not parse http response first line (rv=%d, %s)\n", rv, reply->reply_buffer);
	return NULL;
    }

    if (status != 200) {
	fprintf(stderr, "Server returned status %d (should be 200)\n", status);
	return NULL;
    }

    char *buf = status_line + 2;

    /*
     * To be completed:
     *   The previous code only detects and parses the first line of the reply.
     *   But servers typically send additional header lines:
     *     Date: Mon, 05 Aug 2019 12:54:36 GMT<CR><LF>
     *     Content-type: text/css<CR><LF>
     *     Content-Length: 684<CR><LF>
     *     Last-Modified: Mon, 03 Jun 2019 22:46:31 GMT<CR><LF>
     *     <CR><LF>
     *
     *   Keep calling next_line until you read an empty line, and return only what remains (without the empty line).
     *   Hint: Take a look at how end of lines are tested in next_line function declaration, to get inspiration
     *
     *   Difficul challenge:
     *     If you feel like having a real challenge, go on and implement HTTP redirect support for your client.
     *
     */

	char *line_end;

	// search for the next \r\n in the buffer
	while ((line_end = next_line(buf, reply->reply_buffer_length - (buf - reply->reply_buffer))) != NULL) {

		// If the \r\n is  at the start of current buffer position,
		// then it is empty line, end of http header
		if (line_end == buf) {
			buf = line_end + 2; // Skip past the blank line\r\n
			break;
		}

		// skip past this header line to check the next one
		buf = line_end + 2;
	}

	// downloaded file data
	return buf;
}
