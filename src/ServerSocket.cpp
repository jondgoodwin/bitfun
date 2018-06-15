/*
** ServerSocket.cpp -- a stream socket server
*/

#include "ServerSocket.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int sockfd; // listen on sockfd

int serverInit(void)
{
    struct addrinfo hints, *servinfo, *p;
    int yes=1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

	return 0;
}

// Close the listener
void serverClose()
{
    close(sockfd);
}

int serverPoll()
{
    int new_fd;  // new connection on new_fd
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];

    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
        perror("accept");
        return new_fd;
    }

	inet_ntop(their_addr.ss_family,
        get_in_addr((struct sockaddr *)&their_addr),
        s, sizeof s);
    printf("server: got connection from %s\n", s);
	return new_fd;
}

Socket::Socket(int fd) :
m_sockfd(fd),
m_buffer(NULL),
m_pos(0),
m_size(0)
{
}

Socket::~Socket()
{
	close(m_sockfd);
}

void Socket::BuildBuffer(char *buffer, int size)
{
	m_buffer = buffer;
	m_pos = 0;
	m_size = size;
}

void Socket::Add(const char *str)
{
	int len = strlen(str);
	if (m_pos + len > m_size)
		Send();
	memcpy(&m_buffer[m_pos], str, len);
	m_pos += len;
}

void Socket::Add(int nbr)
{
	char numberString[20];
	sprintf(numberString, "%d", nbr);
	Add(numberString);
}

void Socket::Send()
{
	auto ret = send(m_sockfd, m_buffer, m_pos, 0);
	if (ret == -1)
        perror("send");
	m_pos = 0;
}

void Socket::Send(const char *str)
{
	auto ret = send(m_sockfd, str, strlen(str), 0);
	if (ret == -1)
        perror("send");
}

void Socket::GetLine(char *buf, int len)
{
	int pos = 0;
	while (1)
	{
		int read = recv(m_sockfd, &buf[pos], len-pos, 0);
		if (read <= 0) {
			buf[pos] = '\0';
			return;
		}
		if (buf[pos+read-2] == '\r')
		{
			buf[pos+read-2] = '\0';
			return;
		}
		if (buf[pos+read-1] == '\n')
		{
			buf[pos+read-1] = '\0';
			return;
		}
		pos += read;
	}
}
