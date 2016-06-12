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

// ACK structure
typedef struct _ACK{
	unsigned char a;
	unsigned char b;
	unsigned char c;
	unsigned char d;
} __attribute__((packed)) ACK;

/* main function for tftp implementation
 * arg1 - socket
 * returns status for success, failure*/
bool get_from_client(int fd);
/* gets and returns its block number */
int block_number(ACK ack);
/* gets ack and increases its block number */
void increase_ack(ACK* ack);

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "illegal arguments\n");
		exit(1);
	}

	int fd;
	int port = atoi(argv[1]);
	struct sockaddr_in myaddr; // our address

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

	// infinite loop for getting connecions
	do
	{
		if (get_from_client(fd))
			printf("RECVOK\n");
		else
			printf("RECVFAIL\n");
	}while(1);

	close(fd); // close socket
	return 0;
}



/* main function for tftp implementation
 * arg1 - socket
 * returns status for success, failure*/
bool get_from_client(int fd)
{
	unsigned char buf[BUFSIZE]; // recieve buffer
	struct sockaddr_in remaddr; // remote address
	socklen_t addrlen = sizeof(remaddr); // length of addresses
	int recvlen; // # bytes recieved
	const int NUMBER_OF_FAILURES = 7;
	int timeoutExpiredCount = 0;
	char file[BUFSIZE] = ""; // file name
	char mode[BUFSIZE] = ""; // mode
	int i = 0;
	int lastWriteSize; // actual writing size

	// recieved first message, should be WRQ
	recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);

	// not WRQ opcode
	if (!(buf[0] == 0 && buf[1] == 2))
	{
		printf("FLOWERROR: not WRQ request\n");
		return false;
	}

	// parsing WRQ
	strcpy(file, (char*)buf+2);
	for (i = 2 ; i < BUFSIZE ; i++)
	{
		if(buf[i] == 0)
			break;
	}
	strcpy(mode, (char*)buf+i+1);

	printf("IN:WRQ, %s, %s\n", file, mode);
	// creating file for copy
	FILE *f;
	f = fopen(file, "w");
	if (f == NULL)
	{
		fprintf(stderr, "Error copying file\n");
		exit(4);
	}


	// variabels for select()
	fd_set rfds;
	struct timeval tv;
	int retval;

	ACK my_ack;
	my_ack.a = 0;
	my_ack.b = 4;
	my_ack.c = 0;
	my_ack.d = 0;
	sendto(fd, &my_ack, 4, 0, (struct sockaddr *)&remaddr, addrlen);
	printf("OUT:ACK, %d\n", block_number(my_ack));
	do
	{
		do
		{
			//Wait WAIT_FOR_PACKET_TIMEOUT
			FD_ZERO(&rfds);
			FD_SET(fd, &rfds);
			tv.tv_sec = 3;
			tv.tv_usec = 0;
			retval = select(fd + 1, &rfds, NULL, NULL, &tv);

			if (retval)//if there was something at the socket
			{
				recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
				increase_ack(&my_ack);
				break;
			}
			if (retval < 0) //Time out expired while waiting for data
			{
				sendto(fd, &my_ack, 4, 0, (struct sockaddr *)&remaddr, addrlen);
				printf("FLOWERROR: time out expired for packet\n");
				timeoutExpiredCount++;
			}
			if (timeoutExpiredCount >= NUMBER_OF_FAILURES)
			{
				printf("FLOWERROR: timeout exceeded\n");
				fclose(f); // close file
				return false;
			}
		}while (true); //Continue while some socket was ready

		// We got something else but DATA
		if (!(buf[0] == 0 && buf[1] == 3))
		{
			printf("FLOWERROR: packet is not DATA\n");
			fclose(f); // close file
			return false;
		}
		// incorrect block number
		if (!(buf[2] == my_ack.c && buf[3] == my_ack.d))
		{
			printf("FLOWERROR: incorrect block number\n");
			fclose(f); // close file
			return false;
		}
		ACK tmp;
		tmp.a = 0;
		tmp.b = 0;
		tmp.c = buf[2];
		tmp.d = buf[3];
		printf("IN:DATA, %d, %d\n", block_number(tmp), recvlen);

		timeoutExpiredCount = 0;

		if (recvlen != 4)
		{
			lastWriteSize = fwrite(buf+4 , 1 , recvlen-4 , f);
			printf("WRITING: %d\n", lastWriteSize);
		}

		sendto(fd, &my_ack, 4, 0, (struct sockaddr *)&remaddr, addrlen);
		printf("OUT:ACK, %d\n", block_number(my_ack));
		// Have blocks left to be read from client (not end of transmission)
	}while (recvlen == 516);

	fclose(f); // close file
	return true;
}

/* gets and returns its block number */
int block_number(ACK ack)
{
	return ack.c*256 + ack.d;
}

/* gets ack and increases its block number */
void increase_ack(ACK* ack)
{
	if (ack->d == 255)
	{
		ack->d = 0;
		ack->c++;
	}
	else
		ack->d++;
}
