//NUME: CHIRILA ALBERT
//GRUPA: 324CB

#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

#define HOST_MAIN_SERVER "ec2-3-8-116-10.eu-west-2.compute.amazonaws.com"
#define HOST_WEATHER "api.openweathermap.org"
#define PORT 8080
#define PORT_WEATHER 80

#define IP_WEATHER "188.166.16.132"

#define URL_DUMMY "/api/v1/tema/auth/register"
#define URL_LOGIN "/api/v1/tema/auth/login"
#define URL_LOGOUT "/api/v1/tema/auth/logout"
#define URL_LIBRARY "/api/v1/tema/library/access"
#define URL_BOOKS "/api/v1/tema/library/books"

#define URL_WEATHER "/data/2.5/weather"

struct sockaddr *get_addr(char* name)
{
    int ret;
    struct addrinfo hints, *result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    ret = getaddrinfo(name, NULL, &hints, &result);
    if (ret != 0)
    {
        perror("getaddrinfo");
    }

    return result->ai_addr;
}

//payload pentru adaugarea unei carti
char *make_payload_book(char title[200], char author[50], char genre[20], int page_count, char publisher[50])
{
	char *payload = calloc(1000, sizeof(char));

	sprintf(payload, "{\n    \"title\": \"%s\",\n    \"author\": \"%s\",\n    \"genre\": \"%s\",\n   \"page_count\": 	\"%d\",\n    \"publisher\": \"%s\"\n}\n",
	 title, author, genre, page_count, publisher);

	return payload;
}

//payload la inregistrare/autentificare
char *make_payload(char username[50], char password[50])
{
	char *payload = calloc(500, sizeof(char));
	strcpy(payload, "{\n    \"username\": \"");
	strcat(payload, username);
	strcat(payload, "\",\n    \"password\": \"");
	strcat(payload, password);
	strcat(payload, "\"\n}\n");

	return payload;
}

int main(int argc, char *argv[])
{
	int server_socket;

    struct sockaddr_in *serv_addr = (struct sockaddr_in *) get_addr(HOST_MAIN_SERVER);

    char ip[100];
    memset(ip, 0, 100);
    inet_ntop(AF_INET, &(serv_addr->sin_addr), ip, 100);

    server_socket = open_connection(ip, PORT, AF_INET, SOCK_STREAM, 0);

    int is_logged_out = 1;
    char *request;
    char *response;
    char *cookie = calloc(200, sizeof(char));
    char *token = calloc(500, sizeof(char));
    char *payload = calloc(500, sizeof(char));

    char command[20];
    //ruleaza pana se citeste 'exit'
    while((fgets(command, 19, stdin)))
    {	
    	//se inchide programul
    	if (!strcmp(command, "exit\n"))
 			break;

 		if (!strcmp(command, "register\n") || (!strcmp(command, "login\n")))
 		{
 			//se citesc username-ul si parola
 			char *q, copie[20];
 			strcpy(copie, command);
 			char username[50], password[50];
 			memset(copie, 0, 20);
 			scanf("%s", copie);
 			q = strtok(copie, "=");
 			q = strtok(NULL, "");
 			strcpy(username, q);
 			scanf("%s", copie);
 			q = strtok(copie, "=");
 			q = strtok(NULL, "");
 			strcpy(password, q);

 			memset(payload, 0, 500);
 			//se construieste payload-ul
 			payload = make_payload(username, password);

 			server_socket = open_connection(ip, PORT, AF_INET, SOCK_STREAM, 0);

 			if (!(strcmp(command, "register\n")))
 			{
 				request = compute_post_request(HOST_MAIN_SERVER, URL_DUMMY, "application/json", NULL, payload);
 				send_to_server(server_socket, request);
			}
			else
			{
				request = compute_post_request(HOST_MAIN_SERVER, URL_LOGIN, "application/json", NULL, payload);
    			send_to_server(server_socket, request);
			}

			response = receive_from_server(server_socket);
    		printf("RECEIVED RESPONSE:\n<%s>\n\n", response);

    		if (!(strcmp(command, "login\n")))
    		{	
    			//se salveaza cookie-ul
    			char *start = strstr(response, "Set-Cookie: ");
    			if (start != NULL)
    			{
    				start += strlen("Set-Cookie: ");
    				char *end = strstr(start, ";");
    				strncpy(cookie, start, end - start);
    				is_logged_out = 0;
    			}
    		}

 		}

 		if (!strcmp(command, "enter_library\n"))
 		{
 			server_socket = open_connection(ip, PORT, AF_INET, SOCK_STREAM, 0);

    		request = compute_get_request(HOST_MAIN_SERVER, URL_LIBRARY, cookie, NULL, NULL);
    		send_to_server(server_socket, request);

    		response = receive_from_server(server_socket);
    		printf("RECEIVED RESPONSE:\n<%s>\n\n", response);

    		//daca este autentificat(daca nu este logged out)
    		if (!is_logged_out)
    		{

    			memset(token, 0, LINELEN);
    			char *start, *end;

   		 		start = strstr(response, "\"token\":\"");
    			start += strlen("\"token\":\"");
    			end = strstr(start, "\"");
    			strncpy(token, start, end - start);
    		}

 		}

 		if (!(strcmp(command, "get_books\n")))
 		{
 			server_socket = open_connection(ip, PORT, AF_INET, SOCK_STREAM, 0);

    		request = compute_get_request(HOST_MAIN_SERVER, URL_BOOKS, cookie, token, NULL);
    		send_to_server(server_socket, request);

    		response = receive_from_server(server_socket);
    		printf("RECEIVED RESPONSE:\n<%s>\n\n", response);

 		}

 		if (!strcmp(command, "get_book\n"))
 		{
 			//se citeste id-ul cartii
 			char *q, copie[20];
 			strcpy(copie, command);
 			scanf("%s", copie);
 			q = strtok(copie, "=");
 			q = strtok(NULL, "");

 			server_socket = open_connection(ip, PORT, AF_INET, SOCK_STREAM, 0);

 			//se adauga id-ul la adresa URL
 			char api[60];
 			strcpy(api, "/api/v1/tema/library/books/");
 			strcat(api, q);
 			
 			request = compute_get_request(HOST_MAIN_SERVER, api, cookie, token, NULL);
    		send_to_server(server_socket, request);

    		response = receive_from_server(server_socket);
    		printf("RECEIVED RESPONSE:\n<%s>\n\n", response);

 		}

 		if (!strcmp(command, "add_book\n"))
 		{
 			char *q;
 			char title[200], author[50], genre[20], publisher[50];
 			int page_count;

 			//titlul
 			fgets(title, 200, stdin);
 			if (title[strlen(title) - 1] == '\n')
 				title[strlen(title) - 1] = 0;
 			q = strtok(title, "=");
 			q = strtok(NULL, "=");
 			strcpy(title, q);

 			//autorul
 			fgets(author, 50, stdin);
 			if (author[strlen(author) - 1] == '\n')
 				author[strlen(author) - 1] = 0;
 			q = strtok(author, "=");
 			q = strtok(NULL, "=");
 			strcpy(author, q);

 			//genul
 			fgets(genre, 20, stdin);
 			if (genre[strlen(genre) - 1] == '\n')
 				genre[strlen(genre) - 1] = 0;
 			q = strtok(genre, "=");
 			q = strtok(NULL, "=");
 			strcpy(genre, q);

 			//publisher
 			fgets(publisher, 200, stdin);
 			if (publisher[strlen(publisher) - 1] == '\n')
 				publisher[strlen(publisher) - 1] = 0;
 			q = strtok(publisher, "=");
 			q = strtok(NULL, "=");
 			strcpy(publisher, q);

 			//pagini
 			scanf("%s", command);
 			q = strtok(command, "=");
 			q = strtok(NULL, "=");
 			page_count = atoi(q);

 			//se contruieste payload-ul
 			char *payload_book = make_payload_book(title, author, genre, page_count, publisher);

 			server_socket = open_connection(ip, PORT, AF_INET, SOCK_STREAM, 0);

    		request = compute_post_request(HOST_MAIN_SERVER, URL_BOOKS, "application/json", token, payload_book);
    		send_to_server(server_socket, request);

    		response = receive_from_server(server_socket);
    		printf("RECEIVED RESPONSE:\n<%s>\n\n", response);

 		}

 		if (!strcmp(command, "delete_book\n"))
 		{	
 			//se citeste id-ul cartii
 			char *q, copie[20];
 			strcpy(copie, command);
 			scanf("%s", copie);
 			q = strtok(copie, "=");
 			q = strtok(NULL, "");

 			server_socket = open_connection(ip, PORT, AF_INET, SOCK_STREAM, 0);

 			//se adauga id-ul la adresa URL
 			char api[60];
 			strcpy(api, "/api/v1/tema/library/books/");
 			strcat(api, q);

 			request = compute_delete_request(HOST_MAIN_SERVER, api, cookie, token, NULL);
    		send_to_server(server_socket, request);

    		response = receive_from_server(server_socket);
    		printf("RECEIVED RESPONSE:\n<%s>\n\n", response);

 		}

 		if (!strcmp(command, "logout\n"))
 		{
 			server_socket = open_connection(ip, PORT, AF_INET, SOCK_STREAM, 0);

    		request = compute_get_request(HOST_MAIN_SERVER, URL_LOGOUT, cookie, NULL, NULL);
    		send_to_server(server_socket, request);

    		response = receive_from_server(server_socket);
    		printf("RECEIVED RESPONSE:\n<%s>\n\n", response);

    		//se sterg cookie-ul si token-ul
    		memset(token, 0, LINELEN);
    		memset(cookie, 0, LINELEN);
    		is_logged_out = 1;
 		}
    }

    close_connection(server_socket);

    return 0;
}
