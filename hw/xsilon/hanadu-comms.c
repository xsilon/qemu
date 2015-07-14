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
#include "hanadu-defs.h"


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
void *
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

ssize_t
netsim_tx_data_ind(void)
{
	uint16_t psdu_len;
	uint8_t rep_code;
	uint16_t extra_hdr;
	uint8_t * buf;
	int8_t tx_power;
	int msg_len;
	struct netsim_data_ind_pkt *data_ind_pkt;
	int rv;

	if (han_trxm_tx_mem_bank_select_get(han.trx_dev) == 0) {
		psdu_len = han_trxm_tx_psdu_len0_get(han.trx_dev);
		rep_code = han_trxm_tx_rep_code0_get(han.trx_dev);
		extra_hdr = han_trxm_tx_hdr_extra0_get(han.trx_dev);
		buf = han.tx.buf[0].data;
		tx_power = han_trxm_tx_pga_gain0_get(han.trx_dev);
		if (han_trxm_tx_att_20db0_get(han.trx_dev))
			tx_power |= 0X80;
		if (han_trxm_tx_att_10db0_get(han.trx_dev))
			tx_power |= 0X40;
	} else {
		psdu_len = han_trxm_tx_psdu_len1_get(han.trx_dev);
		rep_code = han_trxm_tx_rep_code1_get(han.trx_dev);
		extra_hdr = han_trxm_tx_hdr_extra1_get(han.trx_dev);
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
	data_ind_pkt->extra_hdr = extra_hdr;
	data_ind_pkt->cca_mode = 0;
	data_ind_pkt->tx_power = tx_power;
	memcpy(data_ind_pkt->pktData, buf, psdu_len);

	data_ind_pkt->hdr.cksum = htons(generate_checksum(data_ind_pkt, msg_len));
	rv = sendto(han.netsim.sockfd, data_ind_pkt, msg_len, 0,
		   (struct sockaddr *)&han.netsim.server_addr,
		   sizeof(han.netsim.server_addr));
	/* TODO: Check return value and log if error */
	free(data_ind_pkt);

	return rv;
}

ssize_t
netsim_tx_ack_data_ind(uint8_t seq_num)
{
	uint8_t rep_code;
	int8_t tx_power;
	int msg_len;
	struct netsim_data_ind_pkt *data_ind_pkt;
	int rv;
	uint8_t ack_buf[ACK_FRAME_LENGTH];

	ack_buf[0] = han_mac_ack_fc0_get(han.mac_dev);
	ack_buf[1] = han_mac_ack_fc1_get(han.mac_dev);;
	ack_buf[2] = seq_num;

	rep_code = han_mac_ack_rep_code_get(han.mac_dev);
	tx_power = han_mac_ack_pga_gain_get(han.mac_dev);

	msg_len = sizeof(struct netsim_data_ind_pkt) - 1 + ACK_FRAME_LENGTH;
	data_ind_pkt = (struct netsim_data_ind_pkt *)malloc(msg_len);

	memset(data_ind_pkt, 0, msg_len);
	netsim_tx_fill_header(&data_ind_pkt->hdr, msg_len, MSG_TYPE_TX_DATA_IND);

	/* han.mac_addr is in big endian format as we copy the bytes of EA ADDR
	 * register into this array so we can copy it into data_ind_pkt->source_addr
	 * and it will be in network byte order.
	 */
	memcpy(&data_ind_pkt->source_addr, han.mac_addr, sizeof(data_ind_pkt->source_addr));
	data_ind_pkt->psdu_len = htons(ACK_FRAME_LENGTH);
	data_ind_pkt->rep_code = rep_code;
	data_ind_pkt->extra_hdr = han_mac_ack_extra_hdr_get(han.mac_dev);
	/* CCA not used for acks, set to 0 */
	data_ind_pkt->cca_mode = 0;
	data_ind_pkt->tx_power = tx_power;
	memcpy(data_ind_pkt->pktData, ack_buf, ACK_FRAME_LENGTH);

	data_ind_pkt->hdr.cksum = htons(generate_checksum(data_ind_pkt, msg_len));
	rv = sendto(han.netsim.sockfd, data_ind_pkt, msg_len, 0,
		   (struct sockaddr *)&han.netsim.server_addr,
		   sizeof(han.netsim.server_addr));
	/* TODO: Check return value and log if error */
	free(data_ind_pkt);

	return rv;
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
netsim_rx_reg_req_msg(struct netsim_to_node_registration_req_pkt *reg_req)
{
	/* TODO: Change assert to log and drop message */
	assert(ntohs(reg_req->hdr.msg_type) == MSG_TYPE_REG_REQ);

	/* Take node id and save and send back confirm */
	han.netsim.node_id = ntohll(reg_req->hdr.node_id);
}

/* Returns whether CCA was successful or not */
int
netsim_rx_cca_con_msg(struct netsim_to_node_cca_con_pkt * cca_con)
{
	int rv = true;

	/* TODO: Change assert to log and drop message */
	assert(ntohs(cca_con->hdr.msg_type) == MSG_TYPE_CCA_CON);
	assert(ntohll(cca_con->hdr.node_id) == han.netsim.node_id);

	rv = cca_con->result;

	return rv;
}

/* Read Tx Done indication message and return the result of the Tx which isn't
 * of use to our HW model as in real life the Modem doesn't get this information
 * so we will ignore. */
int
netsim_rx_tx_done_ind_msg(struct netsim_to_node_tx_done_ind_pkt *tx_done_ind)
{
	int rv = true;

	/* TODO: Change assert to log and drop message */
	assert(ntohs(tx_done_ind->hdr.msg_type) == MSG_TYPE_TX_DONE_IND);
	assert(ntohll(tx_done_ind->hdr.node_id) == han.netsim.node_id);

	rv = tx_done_ind->result;

	return rv;
}

struct netsim_data_ind_pkt *
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

	return data_ind_copy;
}

void
netsim_rxmcast_init(void)
{
	struct ip_mreq mreq;
	int rv;
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
	/* TODO: Non blocking and closeexec flags */

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
}

