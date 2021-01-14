#pragma once
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <unistd.h>
/* According to POSIX.1-2001, POSIX.1-2008 */
#include <sys/select.h>
/* ethheader */
#include <net/ethernet.h>
/* ether_header */
#include <arpa/inet.h>
/* icmphdr */
#include <netinet/ip_icmp.h>
/* arphdr */
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <asm/byteorder.h>
#include <queue.h>

/* 
 *Note that "buffer" should be at least the MTU size of the 
 * interface, eg 1500 bytes 
 */
#define MAX_LEN 1600
#define ROUTER_NUM_INTERFACES 4

#define DIE(condition, message) \
	do { \
		if ((condition)) { \
			fprintf(stderr, "[%d]: %s\n", __LINE__, (message)); \
			perror(""); \
			exit(1); \
		} \
	} while (0)

typedef struct {
	int len;
	char payload[MAX_LEN];
	int interface;
} packet;

typedef struct table{
	uint32_t prefix;
	uint32_t next_hop;
	uint32_t mask;
	int interface;
} rtable;

typedef struct arp_table{
	uint32_t ip;
	uint8_t mac[6];

} arp_table;

extern int interfaces[ROUTER_NUM_INTERFACES];

int send_packet(int interface, packet *m);
int get_packet(packet *m);
char *get_interface_ip(int interface);
int get_interface_mac(int interface, uint8_t *mac);
void init();
uint16_t ip_checksum(void* vdata,size_t length);
packet create_arp_packet(struct table *route);
packet create_icmp_packet(packet m);
struct table *get_best_route(uint32_t dest_ip, int rtable_size, struct table *routes);
struct arp_table *get_arp_entry(uint32_t ip, int atable_size, struct arp_table *arp);
void update_arp(struct arp_table *arp, int *atable_size, uint8_t ip[4], uint8_t mac[6]);
void parse_table(struct table *routes, int *rtable_size);
int comparator(const void *p, const void *q);
int set_size(queue q);
uint32_t take_value(char str[15]);
uint32_t convert(u_char ip[4]);

/**
 * hwaddr_aton - Convert ASCII string to MAC address (colon-delimited format)
 * @txt: MAC address as a string (e.g., "00:11:22:33:44:55")
 * @addr: Buffer for the MAC address (ETH_ALEN = 6 bytes)
 * Returns: 0 on success, -1 on failure (e.g., string not a MAC address)
 */
int hwaddr_aton(const char *txt, uint8_t *addr);

