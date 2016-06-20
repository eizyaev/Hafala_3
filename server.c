#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFSIZE 516

/* ACK Structure */
typedef struct _ACK
{
	unsigned char a;
	unsigned char b;
	unsigned char c;
	unsigned char d;
} __attribute__((packed)) ACK;

/* Main function for TFTP implementation
 * arg1 - socket number
 * Returns status : success, failure */
bool GetFromClient(int fd);

/* Gets an ack packet & returns its block number */
int BlockNumber(ACK ack);

/* Gets an ack packet & increases its block number */
void IncreaseAck(ACK* ack);

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "illegal arguments\n");
		exit(1);
	}

	int fd;
	int port = atoi(argv[1]);
	struct sockaddr_in MyAddr; // Our address //

	/* Creating a new UDP socket:
	* AF_INET - for ip
	* SOCK_DGRAM - datagram service, connectionless communication (by UDP)
	* 0 - default value, UDP for SOCK_DGRAM */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		fprintf(stderr, "cannot create socket");
		exit(2);
	}

	/* Bind socket to any valid ip address and specific port */
	memset((char *)&MyAddr, 0, sizeof(MyAddr));
	MyAddr.sin_family = AF_INET;
	MyAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	MyAddr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *)&MyAddr, sizeof(MyAddr)) < 0)
	{
		fprintf(stderr, "bind failed");
		exit(3);
	}

	// Infinite loop for getting connections //
	do
	{
		if (GetFromClient(fd))
			printf("RECVOK\n");
		else
			printf("RECVFAIL\n");
	}while(1);

	close(fd); // Close socket //

	return 0;
}



/* Main function for TFTP implementation
 * arg1 - socket number
 * returns status : success, failure */
bool GetFromClient(int fd)
{
	unsigned char buf[BUFSIZE];	// Receive buffer //
	struct sockaddr_in RemAddr;	// Remote address //
	socklen_t AddrLen = sizeof(RemAddr);	// Length of addresses //
	int RecvLen;	// # Bytes recieved //
	const int NUMBER_OF_FAILURES = 7;
	int TimeoutExpiredCount = 0;
	char file[BUFSIZE] = "";	// File name //
	char mode[BUFSIZE] = "";	// Mode //
	int i = 0;
	int LastWriteSize;	// Actual writing size //

	// Recieved first message, should be WRQ //
	if ((RecvLen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&RemAddr, &AddrLen)) == -1)
	{
		fprintf(stderr, "Error in srecvfrom\n");
		exit(6);
	}

	// Not WRQ opcode //
	if (RecvLen < 10 || !(buf[0] == 0 && buf[1] == 2))
	{
		printf("FLOWERROR: not WRQ request\n");
		return false;
	}

	// Parsing WRQ //
	strcpy(file, (char*)buf+2);
	for (i = 2 ; i < BUFSIZE ; i++)
	{
		if(buf[i] == 0)
			break;
	}
	strcpy(mode, (char*)buf+i+1);

	printf("IN:WRQ, %s, %s\n", file, mode);
	// Creating file for copy //
	FILE *f;
	f = fopen(file, "w");
	if (f == NULL)
	{
		fprintf(stderr, "Error opening file\n");
		exit(4);
	}

	// Variabels for select() //
	fd_set rfds;
	struct timeval tv;
	int retval;

	ACK MyAck;
	MyAck.a = 0;
	MyAck.b = 4;
	MyAck.c = 0;
	MyAck.d = 0;
	
	if (sendto(fd, &MyAck, 4, 0, (struct sockaddr *)&RemAddr, AddrLen) == -1)
	{
		fprintf(stderr, "Error in sendto\n");
		exit(7);
	}
	printf("OUT:ACK, %d\n", BlockNumber(MyAck));
	do
	{
		do
		{
			// Wait WAIT_FOR_PACKET_TIMEOUT //
			FD_ZERO(&rfds);
			FD_SET(fd, &rfds);
			tv.tv_sec = 3;
			tv.tv_usec = 0;
			retval = select(fd + 1, &rfds, NULL, NULL, &tv);

			if (retval > 0)	// If there was something at the socket //
			{
				if ((RecvLen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&RemAddr, &AddrLen)) == -1)
				{
					fprintf(stderr, "Error in srecvfrom\n");
					exit(6);
				}
				IncreaseAck(&MyAck);
				break;
			}
			else if (retval == 0)	// Timeout expired while waiting for data //
			{
				if (sendto(fd, &MyAck, 4, 0, (struct sockaddr *)&RemAddr, AddrLen) == -1)
				{
					fprintf(stderr, "Error in sendto\n");
					exit(7);
				}
				printf("FLOWERROR: time out expired for packet\n");
				TimeoutExpiredCount++;
			}
			else
			{
				fprintf(stderr, "Error in select\n");
				exit(5);
			}
			if (TimeoutExpiredCount >= NUMBER_OF_FAILURES)
			{
				printf("FLOWERROR: timeout exceeded\n");
				fclose(f);	// Close file //
				return false;
			}
		}while (true);	// Continue while some socket was ready //

		// We got something else but DATA //
		if (RecvLen < 4 || !(buf[0] == 0 && buf[1] == 3))
		{
			printf("FLOWERROR: packet is not DATA\n");
			fclose(f);	// Close file //
			return false;
		}
		// Incorrect block number //
		if (!(buf[2] == MyAck.c && buf[3] == MyAck.d))
		{
			printf("FLOWERROR: incorrect block number\n");
			fclose(f);	// Close file //
			return false;
		}

		ACK tmp;
		tmp.a = 0;
		tmp.b = 0;
		tmp.c = buf[2];
		tmp.d = buf[3];
		printf("IN:DATA, %d, %d\n", BlockNumber(tmp), RecvLen);

		TimeoutExpiredCount = 0;

		if (RecvLen != 4)
		{
			LastWriteSize = fwrite(buf+4 , 1 , RecvLen-4 , f);
			printf("WRITING: %d\n", LastWriteSize);
		}

		sendto(fd, &MyAck, 4, 0, (struct sockaddr *)&RemAddr, AddrLen);
		printf("OUT:ACK, %d\n", BlockNumber(MyAck));
	}while (RecvLen == 516);	// Have blocks left to be read from client (not the end of transmission) //

	fclose(f);	// Close file //
	return true;
}

/* Gets an ack packet & returns its block number */
int BlockNumber(ACK ack)
{
	return ack.c*256 + ack.d;
}

/* Gets an ack packet & increases its block number */
void IncreaseAck(ACK* ack)
{
	if (ack->d == 255)
	{
		ack->d = 0;
		ack->c++;
	}
	else
		ack->d++;
}
