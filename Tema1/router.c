/* NUME: CHIRILA ALBERT
   GRUPA: 324CB */

#include "skel.h"
#include "queue.h"

int main(int argc, char *argv[])
{	
	//initizari si declarari variabile
	packet m, m_aux;
	queue q;
	q = queue_create();
	int rc;
	int rtable_size = 0;
	int atable_size = 0;
	//tabela de rutare
	struct table *routes = (rtable *)calloc(100000, sizeof(rtable));
	//tabela arp
	struct arp_table *arp = (arp_table *)calloc(100, sizeof(arp_table));

	init();
	parse_table(routes, &rtable_size); //se citeste tabela de rutare
	//se sorteaza tabela de rutare
	qsort(routes, rtable_size, sizeof(routes[0]), comparator);

	while (1) {
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");

		struct ether_header *eth_hdr = (struct ether_header *)m.payload;

		if (ntohs(eth_hdr -> ether_type) == 0x806) /* ---------- PROTOCOL ARP ---------- */
		{
			struct ether_arp *arp_hdr = (struct ether_arp *)(m.payload + sizeof(struct ether_header));

			if (ntohs(arp_hdr -> ea_hdr.ar_op) == 1) /* ---------- ARP REQUEST ---------- */
			{
				uint8_t mac[6];
				uint32_t ip = inet_addr(get_interface_ip(m.interface));
				get_interface_mac(m.interface, mac);

				//se modifica ether_header si ether_arp
				memcpy(eth_hdr -> ether_dhost, arp_hdr -> arp_sha, 6);
				memcpy(eth_hdr -> ether_shost, mac, 6);
				memcpy(arp_hdr -> arp_tha, arp_hdr -> arp_sha, 6);
				memcpy(arp_hdr -> arp_tpa, arp_hdr -> arp_spa, 4);
				memcpy(arp_hdr -> arp_sha, mac, 6);
				memcpy(arp_hdr -> arp_spa, &ip, 4);
				arp_hdr -> arp_op = htons(2);

				//se trimite ARP REPLY
				send_packet(m.interface, &m);
			}
			else /* ---------- ARP REPLY ---------- */
			{	
				//se actualizeaza tabela ARP
				update_arp(arp, &atable_size, arp_hdr -> arp_spa, arp_hdr -> arp_sha);
				int size = set_size(q);
				int j;
				for (j = 0; j < size; j++)
				{	
					packet *p = queue_deq(q);
					struct ether_header *eth_hdr = (struct ether_header *)p -> payload;
					struct iphdr *ip_hdr = (struct iphdr *)(p -> payload + sizeof(struct ether_header));
					
					//se extrage adresa ip sursa a pachetului ARP REPLY
					uint32_t source = convert(arp_hdr -> arp_spa);
					//se extrage adresa ip destinatie a pachetului extras
					uint32_t dest = ip_hdr -> daddr;
					dest = ntohl(dest);
					if (source == dest)
					{	
						//verificare checksum
						uint16_t aux = ip_hdr -> check;
						ip_hdr -> check = 0;
						uint16_t checksum = ip_checksum(ip_hdr, sizeof(struct iphdr));

						if (aux != checksum) continue;
				
						//actualizare ttl si checksum
						ip_hdr -> ttl = (ip_hdr -> ttl - 1);
						ip_hdr -> check = 0;
						ip_hdr -> check = ip_checksum(ip_hdr, sizeof(struct iphdr));

						uint8_t mac[6];
						get_interface_mac(m.interface, mac);
						memcpy(eth_hdr -> ether_dhost, arp_hdr -> arp_sha, 6);
						memcpy(eth_hdr -> ether_shost, mac, 6);

						p -> interface = m.interface;
						//se trimite pachetul IP
						send_packet(p -> interface, p);
					}
					else
						queue_enq(q, p);

				}
			}
		}
		else /* ---------- PROTOCOL IP ---------- */
		{
			struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
			
			uint32_t ip = inet_addr(get_interface_ip(m.interface));
			uint32_t dest = ip_hdr -> daddr;
			dest = ntohl(dest);

			//pachet ICMP trimis router-ului: ECHO REPLY
			if (ntohl(ip) == dest)
			{
				packet reply = create_icmp_packet(m);
				send_packet(m.interface, &reply);
				continue;
			}

			//nu exista ruta pana la destinatie: host unreachable
			struct table* route = get_best_route(dest, rtable_size, routes);
			if (route == NULL)
			{
				packet reply = create_icmp_packet(m);
				struct icmphdr *icmp_hdr = (struct icmphdr *)(reply.payload + sizeof(struct ether_header) + sizeof(struct iphdr));
				icmp_hdr -> type = 3;
				icmp_hdr -> code = 0;
				send_packet(m.interface, &reply);
				continue;
			}

			//time exceeded
			if (ip_hdr -> ttl <= 1)
			{
				packet reply = create_icmp_packet(m);
				struct icmphdr *icmp_hdr = (struct icmphdr *)(reply.payload + sizeof(struct ether_header) + sizeof(struct iphdr));
				icmp_hdr -> type = 11;
				send_packet(m.interface, &reply);
				continue;
			}

			//exista ruta; se cauta in tabela arp intrarea
			struct arp_table *entry = get_arp_entry(dest, atable_size, arp);
			if (entry == NULL)	//se trimite un pachet ARP REQUEST
			{	
				packet packet = create_arp_packet(route);
				memcpy(&m_aux, &m, sizeof(m));
				queue_enq(q, &m_aux);	//se introduce pachetul in coada
				send_packet(packet.interface, &packet);
			}
			else
			{
				uint16_t aux = ip_hdr -> check;
				ip_hdr -> check = 0;
				uint16_t checksum = ip_checksum(ip_hdr, sizeof(struct iphdr));

				if (aux != checksum) continue; //verificare checksum

				//actualizare ttl si checksum
				ip_hdr -> ttl = (ip_hdr -> ttl - 1);
				ip_hdr -> check = ip_checksum(ip_hdr, sizeof(struct iphdr));

				uint8_t mac[6];
				get_interface_mac(route -> interface, mac);
				//modificare ether_header
				struct ether_header *eth_hdr = (struct ether_header *)(m.payload);
				memcpy(eth_hdr -> ether_dhost, entry -> mac, 6);
				memcpy(eth_hdr -> ether_shost, mac, 6);

				//modificare interfata
				m.interface = route -> interface;

				send_packet(route -> interface, &m);
			}
			
		}
	}

}

//functia creeaza un pachet de tip ARP REQUEST
packet create_arp_packet(struct table *route)
{
	packet m;
	uint8_t broadcast[6];
	uint8_t mac[6];
	uint32_t ip;
	uint32_t next_hop = route -> next_hop;
	next_hop = ntohl(next_hop);

	hwaddr_aton("ff:ff:ff:ff:ff:ff", broadcast);
	ip = inet_addr(get_interface_ip(route -> interface));
	get_interface_mac(route -> interface, mac);

	struct ether_header *eth_hdr = (struct ether_header *)m.payload;
	struct ether_arp *arp_hdr = (struct ether_arp *)(m.payload + sizeof(struct ether_header));

	m.len = sizeof(struct ether_header) + sizeof(struct ether_arp);

	//setez ether_header
	memcpy(eth_hdr -> ether_dhost, broadcast, 6);
	memcpy(eth_hdr -> ether_shost, mac, 6);
	eth_hdr -> ether_type = htons(0x806);

	//setez ether_arp
	memcpy(arp_hdr -> arp_tha, broadcast, 6);
	memcpy(arp_hdr -> arp_tpa, &(next_hop), 4);
	memcpy(arp_hdr -> arp_sha, mac, 6);
	memcpy(arp_hdr -> arp_spa, &ip, 4);
	arp_hdr -> ea_hdr.ar_op = htons(1);
	arp_hdr -> ea_hdr.ar_hrd = 256;
	arp_hdr -> ea_hdr.ar_pro = 8;
	arp_hdr -> ea_hdr.ar_hln = 6;
	arp_hdr -> ea_hdr.ar_pln = 4;

	m.interface = route -> interface;
	return m;

}

//creaza pachet ICMP ECHO REPLY
packet create_icmp_packet(packet m)
{
	packet p;
	struct ether_header *eth_hdr = (struct ether_header *)(p.payload);
	struct iphdr *ip_hdr = (struct iphdr *)(p.payload + sizeof(struct ether_header));
	struct icmphdr *icmp_hdr = (struct icmphdr *)(p.payload + sizeof(struct ether_header) + sizeof(struct iphdr));
	
	struct ether_header *e = (struct ether_header *)(m.payload);
	struct iphdr *ip = (struct iphdr *)(m.payload + sizeof(struct ether_header));
	struct icmphdr *icmp = (struct icmphdr *)(m.payload + sizeof(struct ether_header) + sizeof(struct iphdr));

	p.len = sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr);

	//ether_header
	memcpy(eth_hdr -> ether_dhost, e -> ether_shost, sizeof(eth_hdr -> ether_dhost));
	memcpy(eth_hdr -> ether_shost, e -> ether_dhost, sizeof(eth_hdr -> ether_shost));
	eth_hdr -> ether_type = htons(0x0800);

	//ip_hdr
	ip_hdr -> ihl = 5;
	ip_hdr -> version = 4;
	ip_hdr -> tos = 0;
	ip_hdr -> tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr));
	ip_hdr -> id = ip -> id;
	ip_hdr -> frag_off = ip -> frag_off;
	ip_hdr -> ttl = 64;
	ip_hdr -> protocol = 1;
	ip_hdr -> check = 0;
	ip_hdr -> check = ip_checksum(ip_hdr, sizeof(struct iphdr));
	ip_hdr -> saddr = ip -> daddr;
	ip_hdr -> daddr = ip -> saddr;

	//icmp_header
	icmp_hdr -> type = 0;
	icmp_hdr -> code = 0;
	icmp_hdr -> checksum = 0;
	icmp_hdr -> un.echo.sequence = icmp -> un.echo.sequence;
	icmp_hdr -> un.echo.id = icmp -> un.echo.id;
	icmp_hdr -> checksum = ip_checksum(icmp_hdr, sizeof(struct icmphdr));

	p.interface = m.interface;

	return p;

}

//functia cauta in tabela ARP adresa ip destinatie
struct arp_table *get_arp_entry(uint32_t ip, int atable_size, struct arp_table *arp)
{
	int i;
	for (i = 0; i < atable_size; i++)
	{
		if (arp[i].ip == ip)
			return &arp[i];
	}

	return NULL;
}

struct table *get_best_route(uint32_t dest_ip, int rtable_size, struct table *routes)
{	
	int mid, i, contor = 0, left = 0, right = rtable_size - 1;
	uint32_t max = 0;
	struct table *route = NULL;
	struct table *aux = (rtable *)calloc(50, sizeof(rtable));

	while(left <= right) //cautare binara O(log(n))
	{
		mid = left + (right - left) / 2;

		if (((dest_ip & routes[mid].mask) == routes[mid].prefix))
		{	
			memcpy(&(aux[contor]), &(routes[mid]), sizeof(struct table));
			contor++;
		} 
		if ((dest_ip & routes[mid].mask) >= routes[mid].prefix)
		{
			left = mid + 1;
		}
		else
		{
			right = mid - 1;
		}
	}

	for (i = 0; i < contor; i++)
	{
		if (aux[i].mask > max)
		{
			route = &aux[i];
			max = aux[i].mask;
		}
	}

	return route;
}

//functia introduce in tabela ARP informatiile despre host
void update_arp(struct arp_table *arp, int *atable_size, uint8_t ip[4], uint8_t mac[6])
{
	int i = 0;
	uint32_t addr;
	memcpy(&addr, ip, sizeof(addr));
	addr = ntohl(addr);
	for (i = 0; i < *atable_size; i++)
	{
		if (arp[i].ip == addr)
			return;
	}
	memcpy(&arp[i].ip, &addr, sizeof(addr));
	memcpy(arp[i].mac, mac, sizeof(arp[i].mac));
	*atable_size = i + 1;
}

//functia citeste din fisier
void parse_table(struct table *routes, int *rtable_size)
{
	FILE *file = fopen("rtable.txt", "r");
	if (file == NULL) 
	{
		printf("Can open this file!\n");
		return;
	}
	char line[100];
	int i;
	for (i = 0; fgets(line, sizeof(line), file); i++)
	{
		char prefix[15], next_hop[15], mask[15];
		int interface;

		char *token = strtok(line, " ");
		strcpy(prefix, token);
		routes[i].prefix = take_value(prefix);

		token = strtok(NULL, " ");
		strcpy(next_hop, token);
		routes[i].next_hop = take_value(next_hop);

		token = strtok(NULL, " ");
		strcpy(mask, token);
		routes[i].mask = take_value(mask);

		token = strtok(NULL, " ");
		interface = atoi(token);
		routes[i].interface = interface;
	}
	*rtable_size = i;

	fclose(file);
}

/* ----- Functii ajutatoare ----- */

//functia returneaza valoarea intreaga a unei adrese ip
uint32_t take_value(char str[15])
{
	uint32_t res = 0;
	int i, idx = 0, count = 1;
	char byte[3];

	for (i = 0; i < strlen(str); i++)
	{
		if (str[i] == '.')
		{
			int nr = atoi(byte);
			memset(byte, 0, 3);
			idx = 0;
			res += nr << (32 - (8 * count));
			count++;
		}
		else 
		{
			byte[idx] = str[i];
			idx++;
		}
	}
	res += atoi(byte);

	return res;
}

//functie ajutatoare pentru apelarea qsort
int comparator(const void *p, const void *q)
{
	uint32_t prefix1 = ((struct table *)p) -> prefix;
	uint32_t prefix2 = ((struct table *)q) -> prefix;

	if (prefix1 == prefix2)
		return (((struct table *)p) -> mask) - (((struct table *)q) -> mask);
	else
		return prefix1 - prefix2;
}

uint32_t convert(u_char ip[4])
{
	uint32_t res = 0;
	int i;
	for (i = 0; i < 4; i++)
	{	
		res += ip[i] << (32 - (8 * (i + 1)));
	}

	return res;
}

//functia returneaza numarul de elemente din coada
int set_size(queue q)
{
	int i = 0;
	queue qaux = queue_create();

	while(queue_empty(q) != 1)
	{
		packet *m = queue_deq(q);
		queue_enq(qaux, m);
		i++;
	}

	while(queue_empty(qaux) != 1)
	{
		packet *m1 = queue_deq(qaux);
		queue_enq(q, m1);
	}

	return i;
}

