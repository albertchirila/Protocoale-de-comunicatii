/* NUME: CHIRILA ALBERT
 GRUPA: 324CB */
#include "header.h"

int main(int argc, char** argv)
{	
	//initializari variabile
	char buf[BUFLEN];

	int sockfd, ret, port, fdmax, enable = 1;

	struct sockaddr_in serv_addr;

	struct msg_tcp *msg_tcp;
	struct command com;

	fd_set read_fds, tmp_fds; //multimile de file-descriptori

	//socket-ul corespunzator conexiunii cu server-ul
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "Can not open server socket.\n");

	port = atoi(argv[3]);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port); 
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	//se cere conectarea clientului
	ret = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "conect");

	//se trimite ID-ul
	ret = send(sockfd, argv[1], strlen(argv[1]) + 1, 0);
	DIE(ret < 0, "send");

	FD_ZERO(&read_fds);
	FD_SET(sockfd, &read_fds);
	FD_SET(0, &read_fds);
	fdmax = sockfd;

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &enable, 4);

	while(1)
	{
		tmp_fds = read_fds;
		memset(buf, 0, BUFLEN);

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		//comanda de la STDIN
		if (FD_ISSET(0, &tmp_fds))
		{
			fgets(buf, BUFLEN - 1, stdin);

			if (strcmp(buf, "exit\n") == 0)
				break;

			//comanda diferita de exit
			if (make_command(&com, buf))
			{
				//se trimite comanda la server
				ret = send(sockfd, (char*) &com, sizeof(com), 0);
				DIE(ret < 0, "Can not send message");
				if (com.type == 's')
					printf("subscribed %s\n", com.topic);
				else
					printf("unsubscribed %s\n", com.topic);
			}
		}
		//s-a primit un mesaj de la server
		if (FD_ISSET(sockfd, &tmp_fds))
		{
			memset(buf, 0, BUFLEN);
			ret = recv(sockfd, buf, sizeof(struct msg_tcp), 0);
			DIE(ret < 0, "Message not received");

			//s-a inchis conexiunea ('exit' de la server)
			if (ret == 0) break;

			//s-a primit avertisment de la server
			if (!(strcmp(buf, "You are already connected.")))
			{
				printf("%s\n", buf);
				break;
			}

			msg_tcp = (struct msg_tcp *)buf;

			printf("%s:%u - %s - %s - %s\n", msg_tcp -> ip, msg_tcp -> port,
				msg_tcp -> topic, msg_tcp -> type, msg_tcp -> payload);

		}

	}
	
	//se inchide socketul
	close(sockfd);

	return 0;
}


int make_command(struct command *com, char *buf)
{

	buf[strlen(buf) - 1] = 0;
	char *token = strtok(buf, " ");
	if (token == NULL)
	{
		printf("Command should be: (un)subscribe <TOPIC> <SF>\n");
		return 0;
	}

	if (!strcmp(token, "subscribe"))
		com -> type = 's';
	else if (!strcmp(token, "unsubscribe"))
		com -> type = 'u';
	else
	{
		printf("Command should be: (un)subscribe <TOPIC> <SF>\n");
		return 0;
	}

	token = strtok(NULL, " ");
	if (token == NULL)
	{
		printf("Command should be: (un)subscribe <TOPIC> <SF>\n");
		return 0;
	}
	strcpy(com -> topic, token);

	if (com -> type == 's')
	{
		token = strtok(NULL, " ");
		if (token == NULL)
		{
			printf("Command should be: subscribe <TOPIC> <SF>\n");
			return 0;
		}
		com -> sf = token[0];
	}

	return 1;
}