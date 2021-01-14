//NUME: CHIRILA ALBERT
//GRUPA: 324CB

#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

char *compute_get_request(char *host, char *url, char *cookie, char *token, char *payload)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

	sprintf(line, "GET %s? HTTP/1.1", url);
    compute_message(message, line);

    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    //daca este deja autentificat(cookie-ul exista)
    if (cookie != NULL)
    {	
    	//daca nu are acces la biblioteca
    	if (token == NULL)
    	{
    		memset(line, 0, LINELEN);
    		sprintf(line, "Cookie: %s\n", cookie);
    		compute_message(message, line);
    	}
    	else
    	{
    		memset(line, 0, LINELEN);
    		sprintf(line, "Authorization: Bearer %s\n", token);
    		compute_message(message, line);
    	}
    
    }
    else
    {
	    memset(line, 0, LINELEN);
    	sprintf(line, "Content-Type: application/json");
    	compute_message(message, line);

    	memset(line, 0, LINELEN);
    	sprintf(line, "Content-Length: %ld\n", strlen(payload));
    	compute_message(message, line);

    	memset(line, 0, LINELEN);
    	sprintf(line, "%s", payload);
    	compute_message(message, line);
    }

    compute_message(message, "");
    return message;
}

char *compute_post_request(char *host, char *url, char* content_type, char *token, char *payload)
{
	char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    
    memset(line, 0, LINELEN);
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    memset(line, 0, LINELEN);
    sprintf(line, "Content-Length: %ld", strlen(payload));
    compute_message(message, line);

    //acces la biblioteca
    if (token != NULL)
    {
    	memset(line, 0, LINELEN);
    	sprintf(line, "Authorization: Bearer %s", token);
    	compute_message(message, line);
    }

    memset(line, 0, LINELEN);
    sprintf(line, "\n%s", payload);
    compute_message(message, line);

    compute_message(message, "");

    free(line);
    return message;
}

char *compute_delete_request(char *host, char *url, char *cookie, char *token, char *payload)
{
	char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

	sprintf(line, "DELETE %s? HTTP/1.1", url);
    compute_message(message, line);

    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    //acces la biblioteca
    if (token != NULL)
    {
    	memset(line, 0, LINELEN);
    	sprintf(line, "Authorization: Bearer %s", token);
    	compute_message(message, line);
    }

    compute_message(message, "");

    free(line);
    return message;
}
