/* NUME: CHIRILA ALBERT
 GRUPA: 324CB */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLEN		1501
#define MAX_CLIENTS	100	// numarul maxim de clienti in asteptare
#define MAX_SOCKETS 500 //numarul maxim de clienti conectati la un topic

//structura pentru clinetii abonati
struct online {
	char topic[50];
	int sockets[MAX_SOCKETS];
};

//structura pentru clientii conectati la server
struct client {
	char id[11];
};

//structura pentru comenzile subscribe/unsubscribe
struct command {
	char type;
	char topic[51];
	char sf;
};

//structura pentru mesaj TCP
struct msg_tcp {
	char ip[16];
	uint16_t port;
	char type[11];
	char topic[51];
	char payload[1501];
};

//structura pentru mesaj UDP
struct msg_udp {
	char topic[50];
	uint8_t type;
	char payload[1501];
};


//functii server
void unsubscribe(int socket, char topic[51], struct online *online, int *online_nr);
void subscribe(int socket, struct command *com, struct online *online, int *online_nr);
int message_type(struct msg_udp *udp_msg, struct msg_tcp *tcp_msg);
void offline(int socket, struct online *online, int *online_nr);
int connected (struct client *clients, char buf[BUFLEN]);
int find_client(int socket, int sockets[MAX_SOCKETS]);
int find_topic(struct online* online, int *online_nr, char topic[51]);

//functii subscriber
int make_command(struct command *com, char *buf);
