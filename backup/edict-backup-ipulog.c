/* ulog_test, $Revision: 1.4 $
 *
 * small testing program for libipulog, part of the netfilter ULOG target
 * for the linux 2.4 netfilter subsystem.
 *
 * (C) 2000 by Harald Welte <laforge@gnumonks.org>
 *
 * this code is released under the terms of GNU GPL
 *
 * $Id: ulog_test.c 286 2002-06-13 12:56:53Z laforge $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libnetfilter_log/libipulog.h>

#define MYBUFSIZ 2048

/* prints some logging about a single packet */
void handle_packet(ulog_packet_msg_t *pkt)
{
	unsigned char *p;
	int i;
	
	if (pkt->mac_len)
	{
		// if MAC address is in device log
		// then, if IPv4
		//   then, log (MAC address + source port) in bloom filter
		//   else, log (MAC address + source IPv6) in bloom filter	
		// else, try to identify device
		// and if IPv4
		//   then, log (MAC address + source port) in bloom filter
		//   else, log (MAC address + source IPv6) in bloom filter
		printf("mac=");
		p = pkt->mac;
		for (i = 0; i < pkt->mac_len; i++, p++)
			printf("%02x%c", *p, i==pkt->mac_len-1 ? ' ':':');
	}
	printf("\n");
	//printf("indev_name=%s, ", pkt->indev_name);
	//printf("outdev_name=%s, ", pkt->outdev_name);
	//printf("payload=%s\ndd", pkt->payload[1]);
	char *py = &(pkt->payload);
	printf("py=%s\n", py);
}

int main(int argc, char *argv[])
{
	struct ipulog_handle *h;
	unsigned char* buf;
	int len;
	ulog_packet_msg_t *upkt;
	int i;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s group \n", argv[0]);
		exit(2);
	}

	/* allocate a receive buffer */
	buf = malloc(MYBUFSIZ);
	if (!buf)
		exit(1);
	
	/* create ipulog handle */
	h = ipulog_create_handle(ipulog_group2gmask(atoi(argv[1])), 65535);
	if (!h)
	{
		/* if some error occurrs, print it to stderr */
		ipulog_perror(NULL);
		exit(1);
	}

	/* loop receiving packets and handling them over to handle_packet */
	for (;;) {
		len = ipulog_read(h, buf, MYBUFSIZ, 1);
		if (len <= 0) {
			ipulog_perror("ulog_test: short read");
			exit(1);
		}
		while ((upkt = ipulog_get_packet(h, buf, len)) != NULL) {
			handle_packet(upkt);
		}
	}
	
	/* just to give it a cleaner look */
	ipulog_destroy_handle(h);
	return 0;
}