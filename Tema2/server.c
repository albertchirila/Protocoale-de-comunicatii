/* NUME: CHIRILA ALBERT
 GRUPA: 324CB */
#include "header.h"

int main(int argc, char** argv)
{	
	DIE(argc != 2, "Usage: ./server <PORT>\n");
	DIE(atoi(argv[1]) < 1024, "Port must be more than 1024\n");

	//initializari variabile
	char buf[BUFLEN];

	int udp_sock, tcp_sock, new_sock, port, ret, i, fdmax, enable = 1;

	struct sockaddr_in udp_addr, tcp_addr, new_tcp;
	socklen_t tcp_len = sizeof(struct sockaddr);
	socklen_t udp_len = sizeof(struct sockaddr);

	struct command *com;
	struct msg_tcp tcp_msg;
	struct msg_udp *udp_msg;

	//vector ce contine topicurile si clientii abonati la acestea
	struct online *online = calloc(1000, sizeof(struct online));
	int online_nr = 0;
	//vector ce contine clientii conectati la server
	struct client *clients = calloc(10000, sizeof(struct client));

	fd_set read_fds, tmp_fds; //multimi de file-descriptori

	//creare socket UDP
	udp_sock = socket(PF_INET, SOCK_DGRAM, 0); // = 3

	//creare socket TCP
	tcp_sock = socket(AF_INET, SOCK_STREAM, 0); // = 4
	
	port = atoi(argv[1]);

	//informatii socket UDP si socket TCP
	udp_addr.sin_family = tcp_addr.sin_family = AF_INET;
	udp_addr.sin_port = tcp_addr.sin_port = htons(port);
	udp_addr.sin_addr.s_addr = tcp_addr.sin_addr.s_addr = INADDR_ANY;

	setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, (char *) &enable, 4);

	ret = bind(udp_sock, (struct sockaddr *) &udp_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");
	ret = bind(tcp_sock, (struct sockaddr *) &tcp_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");
	ret = listen(tcp_sock, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
	FD_SET(udp_sock, &read_fds);
	FD_SET(tcp_sock, &read_fds);
	FD_SET(0, &read_fds);
	fdmax = tcp_sock;

	//serverul ruleaza pana primeste comanda 'exit'
	char exit_msg = 0;
	while(!exit_msg)
	{
		tmp_fds = read_fds;
		memset(buf, 0, BUFLEN);

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for(i = 0; i <= fdmax; i++)
		{
			if (FD_ISSET(i, &tmp_fds))
			{
				if (i == 0) //comanda STDIN
				{
					fgets(buf, BUFLEN - 1, stdin);
					//s-a primit comanda 'exit'
					if (strcmp(buf, "exit\n") == 0)
					{
						exit_msg = 1;
						break;
					}
					printf("Only accepted command is 'exit'.\n");
				} 
				else if (i == udp_sock) //mesaj de la un client UDP
				{
					//s-a primit un mesaj de la clientul UDP
					ret = recvfrom(udp_sock, buf, BUFLEN, 0, 
						(struct sockaddr *) &udp_addr, &udp_len);
					DIE(ret < 0, "Nothing received from UDP socket");

					/* se completeaza structura pentru mesajul ce urmeaza
					a fi trimis catre un client TCP */
					tcp_msg.port = htons(udp_addr.sin_port);
					strcpy(tcp_msg.ip, inet_ntoa(udp_addr.sin_addr));
					udp_msg = (struct msg_udp *)buf;

					//mesaj UDP valid
					if (message_type(udp_msg, &tcp_msg))
					{
						//se trimite mesajul la toti clientii abonati la topic
						int value = find_topic(online, &online_nr, tcp_msg.topic);
						if (value != -1)
						{
							int j;
							for (j = 5; j < MAX_SOCKETS; j++)
							{
								if (online[value].sockets[j] >= 5)
								{
									ret = send(j, (char *) &tcp_msg, 
										sizeof(struct msg_tcp), 0);
									DIE(ret < 0, "Can not send message for TCP client");
								}
							}
						}
					}

				}
				else if (i == tcp_sock) //cerere de la client TCP
				{
					//se accepta cererea de conectare
					new_sock = accept(tcp_sock, (struct sockaddr *) &new_tcp, &tcp_len);
					DIE(new_sock < 0, "accept");

					setsockopt(new_sock, SOL_SOCKET, SO_REUSEADDR, 
						(char *) &enable, 4);

					/* se adauga in mutimea de fd noul socket si se
					actualizeaza fdmax */
					FD_SET(new_sock, &read_fds);
					if (new_sock > fdmax)
						fdmax = new_sock;

					//s-a primit ID-ul clientului
					ret = recv(new_sock, buf, BUFLEN - 1, 0);
					DIE(ret < 0, "recv");

					//se verifica daca clientul este conectat
					if (connected(clients, buf))
					{
						strcpy(buf, "You are already connected.");
						//se trimite un mesaj clientului conectat deja
						ret = send(new_sock, buf, strlen(buf) + 1, 0);
						DIE(ret < 0, "Message not sent.\n");
						continue;
					}

					//se actualizeaza vectorul de clienti conectati
					strcpy(clients[new_sock].id, buf);

					printf("New client %s connected from %s:%d.\n", buf, 
						inet_ntoa(new_tcp.sin_addr), ntohs(new_tcp.sin_port));
				}
				else //comanda client TCP
				{
					ret = recv(i, buf, BUFLEN - 1, 0);
					DIE(ret < 0, "recv command");

					//raspunsul 0 specifica inchiderea conexiunii cu clientul
					if (ret == 0)
					{
						//se verifica daca clientul este conectat
						if (strcmp(clients[i].id, ""))
						{
							printf("Client %s disconnected.\n", clients[i].id);
							offline(i, online, &online_nr);
							strcpy(clients[i].id, "");
						}
						FD_CLR(i, &read_fds);
						close(i);
					}
					else
					{	
						//se verifica ce comanda s-a primit
						com = (struct command *)buf;

						if (com -> type == 's')
							subscribe(i, com, online, &online_nr);
						else
							unsubscribe(i, com -> topic, online, &online_nr);
					}

				}
			}
		}

	}

	//se inchid toti socketii
	int k;
	for (k = 0; k <= fdmax; k++)
	{
		FD_CLR(i, &read_fds);
		close(i);
	}

	return 0;
}

//clientul cu socketul corespunzator va fi dezabonat de fiecare topic
void offline(int socket, struct online *online, int *online_nr)
{
	int i, j;
	for (i = 0; i < *online_nr; i++)
	{
		for (j = 5; j < MAX_SOCKETS; j++)
		{
			if (online[i].sockets[j] == socket)
			{
				online[i].sockets[j] = -1;
				break;
			}
		}
	}
}

//clientul nu mai este interesat de topicul corespunzator si se dezaboneaza
void unsubscribe(int socket, char topic[51], struct online *online, int *online_nr)
{
	int value = find_topic(online, online_nr, topic); //verifica daca topicul exista
	//daca topicul exista si clientul este abonat la topic
	if (value != -1)
	{
		int is_subscribed = find_client(socket, online[value].sockets);
		if (is_subscribed != -1)
			online[value].sockets[is_subscribed] = -1;
	}
}

//clientul se aboneaza la topicul corespunzator
void subscribe(int socket, struct command *com, struct online *online, int *online_nr)
{
	//se cauta topicul in vectorul 'online' si se adauga socketul
	int value = find_topic(online, online_nr, com -> topic);
	if (value < 0)
	{
		strcpy(online[*online_nr].topic, com -> topic);
		online[*online_nr].sockets[socket] = socket;
		(*online_nr)++;
	}
	else
	{
		online[value].sockets[socket] = socket;
	}

}

//se completeaza mesajul TCP in functie de cel UDP
int message_type(struct msg_udp *udp_msg, struct msg_tcp *tcp_msg)
{
	//se introduce topicul in structura pentru mesajul TCP
	strcpy(tcp_msg -> topic, udp_msg -> topic);
	tcp_msg -> topic[50] = 0;

	long long num;
	double real;

	if (udp_msg -> type == 0) //tipul INT
	{
		DIE(udp_msg -> payload[0] > 1, "Incorrect sign byte");
		num = ntohl(*(uint32_t*)(udp_msg -> payload + 1));

		if (udp_msg -> payload[0] == 1) //numar negativ
			num = num * (-1);

		sprintf(tcp_msg -> payload, "%lld", num);
		strcpy(tcp_msg -> type, "INT");
	}
	else if (udp_msg -> type == 1) //tipul SHORT_REAL
	{	
		real = ntohs(*(uint16_t *)(udp_msg -> payload));
		real = real / 100;

		sprintf(tcp_msg -> payload, "%.2f", real);
		strcpy(tcp_msg -> type, "SHORT_REAL");
	}
	else if (udp_msg -> type == 2) //tipul FLOAT
	{	
		DIE(udp_msg -> payload[0] > 1, "Incorrect sign byte");
		int i, n = 1;
		real = ntohl(*(uint32_t*)(udp_msg -> payload + 1));

		for (i = 0; i < udp_msg -> payload[5]; i++) n *= 10;

		real = real / n;
		if (udp_msg -> payload[0] == 1) //numar negativ
			real = real * (-1);

		sprintf(tcp_msg -> payload, "%lf", real);
		strcpy(tcp_msg -> type, "FLOAT");
	}
	else //tipul STRING
	{
		strcpy(tcp_msg -> type, "STRING");
		strcpy(tcp_msg -> payload, udp_msg -> payload);
	}

	return 1;

}

//functia verifica daca un client este conectat la server
int connected (struct client *clients, char buf[BUFLEN])
{
	int i;
	for (i = 5; i < MAX_SOCKETS; i++)
	{
		if (!strcmp(clients[i].id, buf))
			return 1;
	}

	return 0;
}

//se cauta un client conectat la un anumit topic
int find_client(int socket, int sockets[MAX_SOCKETS])
{
	int i;
	for (i = 5; i < MAX_SOCKETS; i++)
	{
		if (sockets[i] == socket)
			return i;
	}

	return -1;
}

//se cauta un topic in vectorul de topicuri
int find_topic(struct online *online, int *online_nr, char topic[51])
{
	int i;
	for (i = 0; i < *online_nr; i++)
	{
		if (!strcmp(online[i].topic, topic))
			return i;
	}

	return -1;
}


