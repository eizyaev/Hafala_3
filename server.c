#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFSIZE 516

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "illegal arguments\n");
		exit(1);
	}

	int fd;
	int port = atoi(argv[1]);
	int recvlen; // # bytes recieved
	struct sockaddr_in myaddr; // our address
	struct sockaddr_in remaddr; // remote address
	socklen_t addrlen = sizeof(remaddr); // length of addresses
	unsigned char buf[BUFSIZE]; // recieve buffer


	/* Creating new UDP socket:
	* AF_INET - for ip
	* SOCK_DGRAM - datagram service, connectionless comunication
	* 0 - default value, UDP for SOCK_DGRAM */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		fprintf(stderr, "cannot create socket");
		exit(2);
	}

	/* bind socket to any valid ip address and specific port*/
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
	{
		fprintf(stderr, "bind failed");
		exit(3);
	}

	for (;;) {
		printf("waiting on port %d\n", port);
		recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
		printf("received %d bytes\n", recvlen);
		if (recvlen > 0) {
			buf[recvlen] = 0;
			int i = 0;
			printf("\nHexa display:\n");
			while (i < recvlen)
			{
				printf("%02X",(unsigned)buf[i]);
				i++;
			}
			printf("\nNow chars:\n");
			i = 0;
			while (i < recvlen)
			{
				printf("%c",(unsigned)buf[i]);
				i++;
			}
			printf("\n");
		}
	//	sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, addrlen);
	}
	/* never exits */

	do
	{
		do
		{
			do
			{
				// TODO: Wait WAIT_FOR_PACKET_TIMEOUT to see if something appears
				// for us at the socket (we are waiting for DATA)
				if ()// TODO: if there was something at the socket and
					// we are here not because of a timeout
				{
					// TODO: Read the DATA packet from the socket (at
					// least we hope this is a DATA packet)
				}
				if (...) // TODO: Time out expired while waiting for data
					// to appear at the socket
				{
					//TODO: Send another ACK for the last packet
					timeoutExpiredCount++;
				}
				if (timeoutExpiredCount>= NUMBER_OF_FAILURES)
				{
					// FATAL ERROR BAIL OUT
				}
			}while (...) // TODO: Continue while some socket was ready
			// but recvfrom somehow failed to read the data
			if (...) // TODO: We got something else but DATA
			{
				// FATAL ERROR BAIL OUT
			}
			if (...) // TODO: The incoming block number is not what we have
				// expected, i.e. this is a DATA pkt but the block number
				// in DATA was wrong (not last ACK’s block number + 1)
			{
				// FATAL ERROR BAIL OUT
			}
		}while (FALSE);
			timeoutExpiredCount = 0;
			lastWriteSize = fwrite(...); // write next bulk of data
			// TODO: send ACK packet to the client
	}while (...); // Have blocks left to be read from client (not end of transmission)

		close(fd);
		return 0;
}

