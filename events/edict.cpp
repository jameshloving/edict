#include <arpa/inet.h>
#include <map>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../libs/ip_log/ip_log.cpp"

extern "C" {
	#include <libnetfilter_log/libnetfilter_log.h>
}

struct iphdr_v4
{
	// little Endian (reverse ihl & version if big endian)
	__u8    ihl:4,
		version:4;
	__u8    tos;
	__u16   tot_len;
	__u16   id;
	__u16   frag_off;
	__u8    ttl;
	__u8    protocol;
	__u16   check;
	__u32   saddr;
	__u32   daddr;
};

struct iphdr_v6
{
	__u32	version:4,
		traffic_class:8,
		flow_label:20;
	__u16	payload_len;
	__u8	next_hdr;
	__u8	hop_limit;
	unsigned char saddr[16];
	unsigned char daddr[16];	
};

struct device_log_entry
{
	std::string make_model;
	time_t first_seen;		
};

std::map<std::string, struct device_log_entry> device_log;

static int print_pkt(struct nflog_data *ldata)
{
	struct nfulnl_msg_packet_hdr *ph = nflog_get_msg_packet_hdr(ldata);
	char *payload;
	nflog_get_payload(ldata, &payload);	

	// additions for testing
	struct nfulnl_msg_packet_hw *packet_hw = nflog_get_packet_hw(ldata);
	struct iphdr_v4 *packet_header_v4 = (struct iphdr_v4*)payload;
	struct iphdr_v6 *packet_header_v6;

	printf("S_MAC:%u ", &packet_hw->hw_addr);
	printf("Version:%u ", packet_header_v4->version);

	/**
	if (!device_log.count(std::to_string(static_cast<unsigned long>(&packet_hw->hw_addr))))
	{
		struct device_log_entry = {};
		device_log_entry.make_model = ""; // TODO: get make_model from wifi code
		device_log_entry.first_seen = time(nullptr);
		device_log.insert(std::to_string(&packet_hw->hw_addr), device_log_entry);
	}

	if (packet_header_v4->version == 4)
	{
		char str[INET_ADDRSTRLEN];
		struct in_addr *saddr = (struct in_addr*)&(packet_header_v4->saddr);
		inet_ntop(AF_INET, saddr, str, INET_ADDRSTRLEN);
		printf("S_IPv4:%s ", str);
		
		__u16 *source_port = (__u16*)(packet_header_v4 + (packet_header_v4->ihl * 4));	
		__u16 *dest_port = (__u16*)(packet_header_v4 + (packet_header_v4->ihl * 4) + 2);
		printf("  [&header:%u", &packet_header_v4);
		printf(", ihl:%u]  ", packet_header_v4->ihl);
		printf("S_Port:%u ", *source_port);
		printf("D_Port:%u ", *dest_port);
		// TODO: log.add_ipv4_connection(string_mac_address, uint16_t_source_port)
		
	}
	else if (packet_header_v4->version == 6)
	{
		packet_header_v6 = (struct iphdr_v6*)payload;
		//printf("S_IPv6:%u\n",packet_header_v6->saddr);
		// TODO: add ip6tables rule
		// TODO: get source port
		// TODO: log.add_ipv6_connection(string_mac_address, string_ipv6_address)
	}
	**/

	printf("\n");
	return 0;
}

static int cb(struct nflog_g_handle *gh, struct nfgenmsg *nfmsg,
		struct nflog_data *nfa, void *data)
{
	print_pkt(nfa);
	return 0;
}

int main(int argc, char **argv)
{
	struct nflog_handle *h;
	struct nflog_g_handle *qh;
	struct nflog_g_handle *qh100;
	int rv, fd;
	char buf[4096];
	char *payload;

	h = nflog_open();
	if (!h) {
		fprintf(stderr, "error during nflog_open()\n");
		exit(1);
	}

	printf("unbinding existing nf_log handler for AF_INET (if any)\n");
	if (nflog_unbind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error nflog_unbind_pf()\n");
		exit(1);
	}

	printf("binding nfnetlink_log to AF_INET\n");
	if (nflog_bind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nflog_bind_pf()\n");
		exit(1);
	}
	printf("binding this socket to group 2\n");
	qh = nflog_bind_group(h, 2);
	if (!qh) {
		fprintf(stderr, "no handle for grup 0\n");
		exit(1);
	}

	printf("setting copy_packet mode\n");
	if (nflog_set_mode(qh, NFULNL_COPY_PACKET, 0xffff) < 0) {
		fprintf(stderr, "can't set packet copy mode\n");
		exit(1);
	}

	fd = nflog_fd(h);

	printf("registering callback for group 2\n");
	nflog_callback_register(qh, &cb, NULL);

	printf("going into main loop\n");
	while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0) {
		printf("pkt received (len=%u)\n", rv);
		/* handle messages in just-received packet */
		nflog_handle_packet(h, buf, rv);
	}

	printf("unbinding from group 2\n");
	nflog_unbind_group(qh);

#ifdef INSANE
	/* norally, applications SHOULD NOT issue this command,
	 * since it detaches other programs/sockets from AF_INET, too ! */
	printf("unbinding from AF_INET\n");
	nflog_unbind_pf(h, AF_INET);
#endif

	printf("closing handle\n");
	nflog_close(h);

	return EXIT_SUCCESS;
}