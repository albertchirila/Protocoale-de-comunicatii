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

#define URL_DUMMY "/api/v1/dummy"
#define URL_LOGIN "/api/v1/auth/login"
#define URL_LOGOUT "/api/v1/auth/logout"
#define URL_KEY "/api/v1/weather/key"

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

int main(int argc, char *argv[])
{
   int server_socket;

    // Get main server IP
    struct sockaddr_in *serv_addr = (struct sockaddr_in *) get_addr(HOST_MAIN_SERVER);

    char ip[100];
    memset(ip, 0, 100);
    inet_ntop(AF_INET, &(serv_addr->sin_addr), ip, 100);

    server_socket = open_connection(ip, PORT, AF_INET, SOCK_STREAM, 0);

    char *request;
    char *response;

    // Ex 1.1: GET dummy from main server
    char *get_request = compute_get_request(HOST_MAIN_SERVER, URL_DUMMY, NULL, NULL, 0);
    send_to_server(server_socket, get_request);
    printf("SENT REQUEST:\n<%s>\n\n\n", get_request);

    char *get_request_response = receive_from_server(server_socket);
    printf("RECEIVED RESPONSE:\n<%s>\n\n\n", get_request_response);

    // Ex 1.2: POST dummy and print response from main server
    int data_lines = 1;
    char **data = malloc(data_lines * sizeof(char*));
    data[0] = calloc(LINELEN, sizeof(char));
    sprintf(data[0], "hello=world");

    request = compute_post_request(HOST_MAIN_SERVER, URL_DUMMY,
        "application/x-www-form-urlencoded", data, data_lines, NULL, 0);
    send_to_server(server_socket, request);
    printf("SENT REQUEST:\n<%s>\n\n\n", request);

    response = receive_from_server(server_socket);
    printf("RECEIVED RESPONSE:\n<%s>\n\n\n", response);

    // Ex 2: Login into main server
    memset(data[0], 0, LINELEN);
    sprintf(data[0], "username=student&password=student");
    
    request = compute_post_request(HOST_MAIN_SERVER, URL_LOGIN,
        "application/x-www-form-urlencoded", data, data_lines, NULL, 0);
    send_to_server(server_socket, request);
    printf("SENT REQUEST:\n<%s>\n\n\n", request);

    response = receive_from_server(server_socket);
    printf("RECEIVED RESPONSE:\n<%s>\n\n\n", response);

    // Isolate the cookie
    char cookie[LINELEN];
    memset(cookie, 0, LINELEN);

    char *start = strstr(response, "Set-Cookie: ");
    start += strlen("Set-Cookie: ");
    char *end = strstr(start, "\r\n");
    strncpy(cookie, start, end - start);
    printf("Isolated the cookie: <%s>\n\n\n", cookie);

    // Ex 3: GET weather key from main server
    memset(data[0], 0, LINELEN);
    sprintf(data[0], "%s", cookie);

    request = compute_get_request(HOST_MAIN_SERVER, URL_KEY, 0, data, 1);
    send_to_server(server_socket, request);
    printf("SENT REQUEST:\n<%s>\n\n\n", request);

    response = receive_from_server(server_socket);
    printf("RECEIVED RESPONSE:\n<%s>\n\n\n", response);

    // Isolate the key
    char key[LINELEN];
    memset(key, 0, LINELEN);

    start = strstr(response, "\"key\":\"");
    start += strlen("\"key\":\"");
    end = strstr(start, "\"");
    strncpy(key, start, end - start);
    printf("Isolated the key: <%s>\n\n\n", key);

    // Ex 6: Logout from main server
    memset(data[0], 0, LINELEN);
    sprintf(data[0], "%s", cookie);
    request = compute_get_request(HOST_MAIN_SERVER, URL_LOGOUT, NULL, data, 1);

    // Disconnect from main server
    close_connection(server_socket);

    // Get weather server IP
    int weather_socket = open_connection(IP_WEATHER, PORT_WEATHER, AF_INET, SOCK_STREAM, 0);

    // Ex 4: GET weather data from OpenWeather API
    char query[LINELEN];
    memset(query, 0, LINELEN);
    sprintf(query, "lat=%s&lon=%s&appid=%s", "44.4357327", "26.0455634", key);

    request = compute_get_request(HOST_WEATHER, URL_WEATHER, query, NULL, 0);
    send_to_server(weather_socket, request);
    printf("SENT REQUEST:\n<%s>\n\n\n", request);

    response = receive_from_server(weather_socket);
    printf("RECEIVED RESPONSE:\n<%s>\n\n\n", response);

    // Disconnect from weather server
    close_connection(weather_socket);

    free(data[0]);
    free(data);

    return 0;
}
