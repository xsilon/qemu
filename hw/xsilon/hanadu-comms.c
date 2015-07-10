/*
 * implementation for socket based communication to the Network Simulator for
 * the Hanadu Modem Device Model.
 *
 * Author Martin Townsend
 *        skype: mtownsend1973
 *        email: martin.townsend@xsilon.com
 *               mtownsend1973@gmail.com
 */
#include "hanadu.h"
#include "hanadu-inl.h"

#ifndef ntohll
#if __BYTE_ORDER == __LITTLE_ENDIAN
static inline uint64_t ntohll(uint64_t x) {
	return bswap_64(x);
}
#elif __BYTE_ORDER == __BIG_ENDIAN
static inline uint64_t ntohll(uint64_t x)
{
	return x;
}
#endif
#endif
#ifndef htonll
#define htonll ntohll
#endif


static uint16_t
generate_checksum(void *msg, int msglen)
{
	int cksum = 0;
	uint16_t *p = (uint16_t *)msg;

	while (msglen > 1) {
		cksum += *p++;
		msglen -= 2;
	}
	if (msglen == 1)
		cksum += htons(*(unsigned char *)p << 8);

	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);
	return (~(uint16_t)cksum);
}

static void
netsim_tx_fill_header(struct netsim_pkt_hdr *hdr, uint16_t len, uint16_t msg_type)
{
	hdr->len = htons(len);
	hdr->msg_type = htons(msg_type);
	hdr->interface_version = htonl(NETSIM_INTERFACE_VERSION);
	hdr->node_id = htonll(han.netsim.node_id);
	hdr->cksum = 0;
}

static int
netsim_rx_read_bytes(void *buf, int bytes)
{
	int n;
	int left;
	char *cur;

	/* TODO: Use select with a timeout */
	left = bytes;
	cur = (char *)buf;
	do {
		n = recv(han.netsim.sockfd, cur, left, 0);
		if (n < 0) {
			if (errno != EINTR)
				return n;
		} else if (n == 0) {
			/* TODO: Server has shutdown */
			return 0;

		} else {
			left -= n;
			cur += n;
		}
	} while(left);

	return bytes;
}

/* This will malloc the message, caller must free */
static void *
netsim_rx_read_msg(void)
{
	uint16_t cksum, calculated_cksum;
	struct netsim_pkt_hdr *hdr;
	uint16_t len;
	char *msg;
	int n;

	n = netsim_rx_read_bytes(&len, 2);
	if (n == 2) {
		len = ntohs(len);
		assert(len >= sizeof(struct netsim_pkt_hdr));
		msg = malloc(len);
		hdr = (struct netsim_pkt_hdr *)msg;

		netsim_rx_read_bytes(msg + sizeof(uint16_t), len - sizeof(uint16_t));

		hdr->len = htons(len);
		cksum = ntohs(hdr->cksum);
		hdr->cksum = 0;
		calculated_cksum = generate_checksum(msg, len);

		/* TODO: Change assert to log and drop message */
		assert(calculated_cksum == cksum);
		/* TODO: Change assert to log and drop message */
		assert(ntohl(hdr->interface_version) == NETSIM_INTERFACE_VERSION);

		return msg;
	} else {
		return NULL;
	}
}

#if 0
float
tx_power_hex_to_float(uint8_t hexvalue){
	switch(hexvalue){
	case 0x0f : return 0.0;
	case 0x0a : return -2.5;
	case 0x05 : return -5.0;
	case 0x00 : return -7.5;
	case 0x4f : return -10.0;
	case 0x4a : return -12.5;
	case 0x45 : return -15.0;
	case 0x40 : return -17.5;
	case 0x8f : return -20.0;
	case 0x8a : return -22.5;
	case 0x85 : return -25.0;
	case 0x80 : return -27.5;
	case 0xc0 : return -30.0;
	default : abort();
	}
	return 0.0;
}
const char* tx_power_hex_to_dbstring(u8 hexvalue){
	switch(hexvalue){
	case 0x0f : return "0";
	case 0x0a : return "-2.5";
	case 0x05 : return "-5";
	case 0x00 : return "-7.5";
	case 0x4f : return "-10";
	case 0x4a : return "-12.5";
	case 0x45 : return "-15";
	case 0x40 : return "-17.5";
	case 0x8f : return "-20";
	case 0x8a : return "-22.5";
	case 0x85 : return "-25";
	case 0x80 : return "-27.5";
	case 0xc0 : return "-30";
	default : abort();
	return "";
	}
}
#endif

void
hanadu_tx_buffer_to_netsim(void)
{
	uint16_t psdu_len;
	uint8_t rep_code;
	uint8_t * buf;
	int8_t tx_power;
	int msg_len;
	struct netsim_data_ind_pkt *data_ind_pkt;

	if (han_trxm_tx_mem_bank_select_get(han.trx_dev) == 0) {
		psdu_len = han_trxm_tx_psdu_len0_get(han.trx_dev);
		rep_code = han_trxm_tx_rep_code0_get(han.trx_dev);
		buf = han.tx.buf[0].data;
		tx_power = han_trxm_tx_pga_gain0_get(han.trx_dev);
		if (han_trxm_tx_att_20db0_get(han.trx_dev))
			tx_power |= 0X80;
		if (han_trxm_tx_att_10db0_get(han.trx_dev))
			tx_power |= 0X40;
	} else {
		psdu_len = han_trxm_tx_psdu_len1_get(han.trx_dev);
		rep_code = han_trxm_tx_rep_code1_get(han.trx_dev);
		buf = han.tx.buf[1].data;
		tx_power = han_trxm_tx_pga_gain1_get(han.trx_dev);
		if (han_trxm_tx_att_20db1_get(han.trx_dev))
			tx_power |= 0X80;
		if (han_trxm_tx_att_10db1_get(han.trx_dev))
			tx_power |= 0X40;
	}

	msg_len = sizeof(struct netsim_data_ind_pkt) -1 + psdu_len;
	data_ind_pkt = (struct netsim_data_ind_pkt *)malloc(msg_len);

	memset(data_ind_pkt, 0, msg_len);
	netsim_tx_fill_header(&data_ind_pkt->hdr, msg_len, MSG_TYPE_TX_DATA_IND);

	/* han.mac_addr is in big endian format as we copy the bytes of EA ADDR
	 * register into this array so we can copy it into data_ind_pkt->source_addr
	 * and it will be in network byte order.
	 */
	memcpy(&data_ind_pkt->source_addr, han.mac_addr, sizeof(data_ind_pkt->source_addr));
	data_ind_pkt->psdu_len = htons(psdu_len);
	data_ind_pkt->rep_code = rep_code;
	data_ind_pkt->cca_mode = 0;
	data_ind_pkt->tx_power = tx_power;
	memcpy(data_ind_pkt->pktData, buf, psdu_len);

	data_ind_pkt->hdr.cksum = htons(generate_checksum(data_ind_pkt, msg_len));
	sendto(han.netsim.sockfd, data_ind_pkt, msg_len, 0,
		   (struct sockaddr *)&han.netsim.server_addr,
		   sizeof(han.netsim.server_addr));
	/* TODO: Check return value and log if error */
	free(data_ind_pkt);
}

void
hanadu_rx_buffer_from_netsim(struct han_trxm_dev *s, struct netsim_pkt_hdr *rx_pkt)
{
#if 0
	int i;
	uint8_t fifo_used;
	uint8_t *rxbuf;
	uint8_t * data;

	/* first up get next available buffer */
	i=0;
	while(!(han.rx.bufs_avail_bitmap & (1<<i)) && i < 4)
		i++;
	if (i == 4) {
		/* Overflow */
		han_trxm_rx_mem_bank_overflow_set(s, true);
	} else {
		/* Clear the buffer from the bitmap as we are going to use it. */
		han.rx.bufs_avail_bitmap &= ~(1<<i);
		han_trxm_rx_mem_bank_overflow_set(s, false);
		assert(i>=0 && i<4);
		assert(!fifo8_is_full(&han.rx.nextbuf));
		switch(i) {
		case 0:
			han_trxm_rx_psdulen0_set(s, rx_pkt->psdu_len);
			han_trxm_rx_repcode0_set(s, rx_pkt->rep_code);
			han_trxm_rx_mem_bank_full0_flag_set(s, 1);
			break;
		case 1:
			han_trxm_rx_psdulen1_set(s, rx_pkt->psdu_len);
			han_trxm_rx_repcode1_set(s, rx_pkt->rep_code);
			han_trxm_rx_mem_bank_full1_flag_set(s, 1);
			break;
		case 2:
			han_trxm_rx_psdulen2_set(s, rx_pkt->psdu_len);
			han_trxm_rx_repcode2_set(s, rx_pkt->rep_code);
			han_trxm_rx_mem_bank_full2_flag_set(s, 1);
			break;
		case 3:
			han_trxm_rx_psdulen3_set(s, rx_pkt->psdu_len);
			han_trxm_rx_repcode3_set(s, rx_pkt->rep_code);
			han_trxm_rx_mem_bank_full3_flag_set(s, 1);
			break;
		}
		fifo8_push(&han.rx.nextbuf, i);
		fifo_used = (uint8_t)han.rx.nextbuf.num;
		han_trxm_rx_nextbuf_fifo_wr_level_set(s, fifo_used);
		han_trxm_rx_nextbuf_fifo_rd_level_set(s, fifo_used);
		han_trxm_rx_rssi_latched_set(s, rx_pkt->rssi);

		/* copy data into the relevant buffer */
		rxbuf = han.rx.buf[i].data;
		data = (uint8_t *)rx_pkt + NETSIM_PKT_HDR_SZ;
		for(i=0;i<rx_pkt->psdu_len;i++)
			*rxbuf++ = *data++;
	}
	/* Raise Rx Interrupt */
	qemu_irq_pulse(s->rx_irq);
#endif
}

ssize_t
netsim_tx_reg_con(void)
{
	struct node_to_netsim_registration_con_pkt reg_con;

	netsim_tx_fill_header(&reg_con.hdr, sizeof(reg_con), MSG_TYPE_REG_CON);

	memset(reg_con.os, 0, sizeof(reg_con.os));
	memset(reg_con.os_version, 0, sizeof(reg_con.os_version));
	strncpy(reg_con.os, han.sysinfo.os, sizeof(reg_con.os));
	strncpy(reg_con.os_version, han.sysinfo.os_version, sizeof(reg_con.os_version));

	reg_con.hdr.cksum = htons(generate_checksum(&reg_con, sizeof(reg_con)));
	return send(han.netsim.sockfd, &reg_con, sizeof(reg_con), MSG_NOSIGNAL);
}

ssize_t
netsim_tx_cca_req(void)
{
	struct node_to_netsim_cca_req_pkt cca_req;

	netsim_tx_fill_header(&cca_req.hdr, sizeof(cca_req), MSG_TYPE_CCA_REQ);

	cca_req.hdr.cksum = htons(generate_checksum(&cca_req, sizeof(cca_req)));
	return send(han.netsim.sockfd, &cca_req, sizeof(cca_req), MSG_NOSIGNAL);
}

void
netsim_rx_reg_req_msg(void)
{
	struct netsim_to_node_registration_req_pkt * reg_req;

	reg_req = netsim_rx_read_msg();

	if (reg_req) {
		/* TODO: Change assert to log and drop message */
		assert(ntohs(reg_req->hdr.msg_type) == MSG_TYPE_REG_REQ);

		/* Take node id and save and send back confirm */
		han.netsim.node_id = ntohll(reg_req->hdr.node_id);
		free(reg_req);

		/* TODO: Check return and log errors */
		netsim_tx_reg_con();
	} else {
		/* TODO: Log message */
		exit(EXIT_FAILURE);
	}
}

/* Returns whether CCA was successful or not */
int
netsim_rx_cca_con_msg(void)
{
	struct netsim_to_node_cca_con_pkt * cca_con;
	int rv = true;

	cca_con = netsim_rx_read_msg();

	if (cca_con) {
		/* TODO: Change assert to log and drop message */
		assert(ntohs(cca_con->hdr.msg_type) == MSG_TYPE_CCA_CON);
		assert(ntohll(cca_con->hdr.node_id) == han.netsim.node_id);

		rv = cca_con->result;
		free(cca_con);
	} else {
		/* TODO: Log message */
		exit(EXIT_FAILURE);
	}

	return rv;
}

/* Read Tx Done indication message and return the result of the Tx which isn't
 * of use to our HW model as in real life the Modem doesn't get this information
 * so we will ignore. */
int
netsim_rx_tx_done_ind_msg(void)
{
	struct netsim_to_node_tx_done_ind_pkt * tx_done_ind;
	int rv = true;

	tx_done_ind = netsim_rx_read_msg();

	if (tx_done_ind) {
		/* TODO: Change assert to log and drop message */
		assert(ntohs(tx_done_ind->hdr.msg_type) == MSG_TYPE_TX_DONE_IND);
		assert(ntohll(tx_done_ind->hdr.node_id) == han.netsim.node_id);

		rv = tx_done_ind->result;
		free(tx_done_ind);
	} else {
		/* TODO: Log message */
		exit(EXIT_FAILURE);
	}

	return rv;
}

void
netsim_rx_tx_data_ind_msg(struct netsim_data_ind_pkt *data_ind)
{
	struct netsim_pkt_hdr * hdr;
	uint16_t calculated_cksum;
	uint16_t cksum;
	int len;
	struct netsim_data_ind_pkt *data_ind_copy;


	hdr = (struct netsim_pkt_hdr *)data_ind;
	len = htons(hdr->len);
	assert(ntohs(hdr->msg_type) == MSG_TYPE_TX_DATA_IND);
	cksum = ntohs(hdr->cksum);
	hdr->cksum = 0;
	calculated_cksum = generate_checksum(hdr, len);

	assert(calculated_cksum == cksum);
	assert(ntohl(hdr->interface_version) == NETSIM_INTERFACE_VERSION);

	data_ind_copy = malloc(len);
	memcpy(data_ind_copy, data_ind, len);

	/* Hanada Modem HW model is now responsible for the data_ind_copy buffer */
	han.state->handle_rx_pkt(data_ind_copy);
}

/* ______________________________________________ Receive 802.15.4 Packet Thread
 */
void *
netsim_rxthread(void *arg)
{
	struct ip_mreq mreq;
	int rv;
	uint8_t *rxbuf;
	int optval;

	qemu_log("Starting NetSim Rx Thread");
	han.netsim.rxmcast.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	optval = 1;
	rv = setsockopt(han.netsim.rxmcast.sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
	if (rv < 0) {
		perror("Setting SO_REUSEADDR error");
		close(han.netsim.rxmcast.sockfd);
		exit(EXIT_FAILURE);
	}
//	rv = setsockopt(han.netsim.rxmcast.sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
//	if (rv < 0) {
//		perror("Setting SO_REUSEPORT error");
//		close(han.netsim.rxmcast.sockfd);
//		exit(EXIT_FAILURE);
//	}

	bzero(&han.netsim.rxmcast.addr,sizeof(han.netsim.rxmcast.addr));
	han.netsim.rxmcast.addr.sin_family = AF_INET;
	han.netsim.rxmcast.addr.sin_addr.s_addr=htonl(INADDR_ANY);
	han.netsim.rxmcast.addr.sin_port=htons(HANADU_MCAST_TX_PORT);
	rv = bind(han.netsim.rxmcast.sockfd,
			  (struct sockaddr *)&han.netsim.rxmcast.addr,
			  sizeof(han.netsim.rxmcast.addr));
	if (rv) {
		perror("Error binding rx multicast socket");
		close(han.netsim.rxmcast.sockfd);
		exit(EXIT_FAILURE);
	}
	/* Set IP Multicast address of group */
	mreq.imr_multiaddr.s_addr = inet_addr("224.1.1.1");
	/* local IP Address of interface */
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	rv = setsockopt(han.netsim.rxmcast.sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
			   sizeof(mreq));
	if (rv < 0) {
		perror("Setting IP_ADD_MEMBERSHIP error");
		close(han.netsim.rxmcast.sockfd);
		exit(EXIT_FAILURE);
	}

	rxbuf = (uint8_t *)malloc(256);

	for(;;) {
		struct netsim_pkt_hdr *hdr;
		struct sockaddr_in cliaddr;
		socklen_t len;
		int n;

		len = sizeof(cliaddr);
		n = recvfrom(han.netsim.rxmcast.sockfd, rxbuf, 256, 0 /*MSG_DONTWAIT*/,
					 (struct sockaddr *)&cliaddr, &len);

		if (n == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
		} else if (n == 0) {
			continue;
		}

		/* We will receive data packets we send out as they are sent
		 * over the TCP control channel and the network channel then
		 * sends them over the multicast channel. */
		assert(n > sizeof(*hdr));
		hdr = (struct netsim_pkt_hdr *)rxbuf;
		if(!(ntohll(hdr->node_id) == han.netsim.node_id)) {
			netsim_rx_tx_data_ind_msg((struct netsim_data_ind_pkt *)hdr);
		}

	}
	free(rxbuf);
	close(han.netsim.rxmcast.sockfd);
	pthread_exit(NULL);
}

