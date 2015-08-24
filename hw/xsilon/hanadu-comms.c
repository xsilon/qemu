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

#include <fcntl.h>

#define NETSIM_DEFAULT_PORT					(HANADU_NODE_PORT)
#define NETSIM_DEFAULT_ADDR					(INADDR_LOOPBACK)

/* TODO: What to do if the network simulator crashes and shutsdown.
 * send and sendto will return EPIPE, currently we block SIGPIPE. For receive
 * it will bomb out in netsim_rx_read_bytes.  Maybe we create
 * a state that it goes into on this condition and we keep trying to reconnect??
 */

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

/*
 * Returns 0 for timeout, recv error codes or -ESHUTDOWN if server disconnected.
 * Otherwise number of bytes read which will always equal bytes parameter if
 * no error or exception conditions hit.
 */
static int
netsim_rx_read_bytes(void *buf, int bytes, int timeo_ms)
{
	struct timeval timeout, *timeout_p;
	int fdmax;
	fd_set read_fdset;
	int count;

	int n;
	int left;
	char *cur;

	/* Use select with a timeout for reads */
	left = bytes;
	cur = (char *)buf;
	if(timeo_ms == 0) {
		timeout_p = NULL;
	} else {
		timeout.tv_sec = timeo_ms / 1000;
		timeout.tv_usec = (timeo_ms * 1000) % 1000000;
		timeout_p = &timeout;
	}

	do {
		FD_ZERO(&read_fdset);
		FD_SET(han.netsim.sockfd, &read_fdset);
		fdmax = han.netsim.sockfd;

		count = select(
			fdmax+1,
			&read_fdset,
			NULL,   /* No write set */
			NULL,   /* No exception set */
			timeout_p);  /* Block indefinitely */

		if(count == -1) {
			if (errno == EINTR) {
				/* Select system call was interrupted by a signal so restart
				 * the system call */
				qemu_log_mask(LOG_XSILON, "%s: Select system call interrupted, restarting\n", __FUNCTION__);
				/* TODO: As per manual page we should consider timeout to be
				 * undefined after select returns so we need to recalculate
				 * it. */
				continue;
			}
			/* Shouldn't really get here so abort */
			qemu_log_mask(LOG_XSILON, "%s: Select system call failed %s\n",
				      __FUNCTION__, strerror(errno));
			abort();
		} else if(count == 0) {
			/* Timed out */
			return 0;
		} else {
			if(FD_ISSET(han.netsim.sockfd, &read_fdset)) {
				n = recv(han.netsim.sockfd, cur, left, 0);
				if (n < 0) {
					if (errno != EINTR)
						return n;
				} else if (n == 0) {
					/* Server has shutdown, inform caller so
					 * we can go into reconnect server state. */
					return -ESHUTDOWN;

				} else {
					left -= n;
					cur += n;
				}
			} else {
				/* Either a new fd has been added and the lazy
				 * git adding it has forgot to add the handler or
				 * we are in serious trouble. for now log and
				 * exit. */
				qemu_log_mask(LOG_XSILON, "%s: Select system call has returned an unknown read fd.\n",
					      __FUNCTION__);
				abort();
			}
		}

	} while(left);

	return bytes;
}

/* This will malloc the message, caller must free
 * Returns 0 for timeout,
 * recv error codes
 * -ESHUTDOWN if server disconnected.
 * -EPROTONOSUPPORT if interface version doesn't match.
 * -EBADMSG if checksum fails.
 * Otherwise number of bytes read which will always equal bytes parameter if
 * no error or exception conditions hit.
 * On errors any malloced msg will be freed before returning.
 */
int
netsim_rx_read_msg(void **returned_msg)
{
	uint16_t cksum, calculated_cksum;
	struct netsim_pkt_hdr *hdr;
	uint16_t len;
	char *msg;
	int n;

	n = netsim_rx_read_bytes(&len, 2, 5000);
	if (n == 2) {
		len = ntohs(len);
		assert(len >= sizeof(struct netsim_pkt_hdr));
		msg = malloc(len);
		hdr = (struct netsim_pkt_hdr *)msg;

		n = netsim_rx_read_bytes(msg + sizeof(uint16_t), len - sizeof(uint16_t), 1000);
		if (n == (len - sizeof(uint16_t))) {

			hdr->len = htons(len);
			cksum = ntohs(hdr->cksum);
			hdr->cksum = 0;
			calculated_cksum = generate_checksum(msg, len);

			/* Check checksum and interface version. */
			if(calculated_cksum != cksum) {
				qemu_log_mask(LOG_XSILON, "%s: Calculated checksum (0x%04x) != (0x%04x)\n",
					      __FUNCTION__, calculated_cksum, cksum);
				free(msg);
				return -EBADMSG;
			}
			if(ntohl(hdr->interface_version) != NETSIM_INTERFACE_VERSION) {
				qemu_log_mask(LOG_XSILON, "%s: Received interface vers (0x%04x) != (0x%04x)\n",
					      __FUNCTION__, ntohl(hdr->interface_version),
					      NETSIM_INTERFACE_VERSION);
				free(msg);
				return -EPROTONOSUPPORT;
			}

			*returned_msg = msg;
			return n;
		} else {
			qemu_log_mask(LOG_XSILON, "%s: Failed to read the full msg (%d), read (%d) bytes\n",
				      __FUNCTION__, (int)(len - sizeof(uint16_t)), n);
			free(msg);
			return n;
		}
	} else {
		return n;
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

/*
 * This will send a msg and handle EINTR and cases where send returns early
 * due to a signal and all data hasn't been sent.  If we detect other errors
 * the function will stop and return this errno, otherwise if all is well it
 * will return the number of bytes transmitted which is the passed parameter
 * msglen.
 */
static int
netsim_tx_control_msg(uint8_t *msg, int msglen)
{
	int left = msglen;
	uint8_t *cur = msg;
	ssize_t n;

	do {
		n = send(han.netsim.sockfd, cur, left, MSG_NOSIGNAL);
		if (n == -1 && errno != EINTR) {
			qemu_log_mask(LOG_XSILON, "%s: Failed to send message\n",
				      __FUNCTION__);
			return n;
		}
		left -= n;
		cur += n;
	} while (left && (n == -1 && errno == EINTR));

	return msglen;
}

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

	msg_len = sizeof(struct netsim_data_ind_pkt) - 1 + psdu_len;
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
	data_ind_pkt->extra_hdr = htons(extra_hdr);
	data_ind_pkt->cca_mode = 0;
	data_ind_pkt->tx_power = tx_power;
	memcpy(data_ind_pkt->pktData, buf, psdu_len);

	data_ind_pkt->hdr.cksum = htons(generate_checksum(data_ind_pkt, msg_len));

	rv = netsim_tx_control_msg((uint8_t *)data_ind_pkt, msg_len);

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
	data_ind_pkt->extra_hdr = htons(han_mac_ack_extra_hdr_get(han.mac_dev));
	/* CCA not used for acks, set to 0 */
	data_ind_pkt->cca_mode = 0;
	data_ind_pkt->tx_power = tx_power;
	memcpy(data_ind_pkt->pktData, ack_buf, ACK_FRAME_LENGTH);

	data_ind_pkt->hdr.cksum = htons(generate_checksum(data_ind_pkt, msg_len));
	rv = netsim_tx_control_msg((uint8_t *)data_ind_pkt, msg_len);

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

	return netsim_tx_control_msg((uint8_t *)&reg_con, sizeof(reg_con));
}

ssize_t
netsim_tx_cca_req(void)
{
	struct node_to_netsim_cca_req_pkt cca_req;

	netsim_tx_fill_header(&cca_req.hdr, sizeof(cca_req), MSG_TYPE_CCA_REQ);

	cca_req.hdr.cksum = htons(generate_checksum(&cca_req, sizeof(cca_req)));

	return netsim_tx_control_msg((uint8_t *)&cca_req, sizeof(cca_req));
}

void
netsim_rx_reg_req_msg(struct netsim_to_node_registration_req_pkt *reg_req)
{
	assert(ntohs(reg_req->hdr.msg_type) == MSG_TYPE_REG_REQ);

	/* Take node id and save and send back confirm */
	han.netsim.node_id = ntohll(reg_req->hdr.node_id);
}

/* Returns whether CCA was successful or not */
int
netsim_rx_cca_con_msg(struct netsim_to_node_cca_con_pkt * cca_con)
{
	int rv = true;

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
	int flags;

	qemu_log_mask(LOG_XSILON, "Starting NetSim Rx Thread\n");
	han.netsim.rxmcast.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	optval = 1;
	rv = setsockopt(han.netsim.rxmcast.sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
	if (rv < 0) {
		perror("Setting SO_REUSEADDR error");
		close(han.netsim.rxmcast.sockfd);
		exit(EXIT_FAILURE);
	}
	/* Ensure new socket is closed when using any of the exec functions */
	flags = fcntl(han.netsim.rxmcast.sockfd, F_GETFD, 0);
	if(flags == -1) {
		qemu_log_mask(LOG_XSILON, "Failed to get flags for multicast receive socket (%s)\n", strerror(errno));
		exit(EXIT_FAILURE);
	} else {
		int ret;

		flags |= FD_CLOEXEC;
		ret = fcntl(han.netsim.rxmcast.sockfd, F_SETFD, flags);
		if (ret == -1) {
			qemu_log_mask(LOG_XSILON, "Failed to set flags for multicast receive socket (%s)\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
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
}

int
netsim_comms_connect(void)
{
	int flags;
	int rv;

	/* Create TCP connection to Network Simulator */
	han.netsim.sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (han.netsim.sockfd == -1) {
		perror("Failed to create netsim socket");
		exit(EXIT_FAILURE);
	}
	/* Ensure new socket is closed when using any of the exec functions */
	flags = fcntl(han.netsim.sockfd, F_GETFD, 0);
	if(flags == -1) {
		qemu_log_mask(LOG_XSILON, "Failed to get flags for TCP control socket (%s)\n", strerror(errno));
		exit(EXIT_FAILURE);
	} else {
		int ret;

		flags |= FD_CLOEXEC;
		ret = fcntl(han.netsim.sockfd, F_SETFD, flags);
		if (ret == -1) {
			qemu_log_mask(LOG_XSILON, "Failed to set flags for TCP control socket (%s)\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	bzero(&han.netsim.server_addr,sizeof(han.netsim.server_addr));
	han.netsim.server_addr.sin_family = AF_INET;
	if (han.netsim.addr)
		han.netsim.server_addr.sin_addr.s_addr=inet_addr(han.netsim.addr);
	else
		han.netsim.server_addr.sin_addr.s_addr=htonl(NETSIM_DEFAULT_ADDR);
	if (han.netsim.port)
		han.netsim.server_addr.sin_port=htons(han.netsim.port);
	else
		han.netsim.server_addr.sin_port=htons(NETSIM_DEFAULT_PORT);

	rv = connect(han.netsim.sockfd, (struct sockaddr *)&han.netsim.server_addr,
			sizeof(han.netsim.server_addr));

	if (rv == -1)
		netsim_comms_close();

	return rv;
}

void
netsim_comms_init(void)
{
	/* Connect to NetSim server */
	if (netsim_comms_connect() < 0) {
		perror("Failed to connect to Network Simualtor TCP socket\n");
		exit(EXIT_FAILURE);
	}

	netsim_rxmcast_init();
}

void
netsim_comms_close(void)
{
	if (han.netsim.sockfd != -1) {
		close(han.netsim.sockfd);
		han.netsim.sockfd = -1;
	}
	if (han.netsim.rxmcast.sockfd != -1) {
		close(han.netsim.rxmcast.sockfd);
		han.netsim.rxmcast.sockfd = -1;
	}
}
