/*
 * This file contains the main implementation of the Hanadu HW modem device's
 * State Machine as documented by the Papyrus Model in x6losim.
 *
 * Author Martin Townsend
 *        skype: mtownsend1973
 *        email: martin.townsend@xsilon.com
 *               mtownsend1973@gmail.com
 *
 * TODO: Have a mode where we don't process Acks within the QEMU model, let the
 * network simulator decide on whether the packet was successfuly acked and how
 * many retries it took.   This is because we can't really process an ack
 * in time and if there are network and processing delays we will always
 * breach the ack waitdur.
 */
#include "hanadu.h"
#include "hanadu-inl.h"
#include "hanadu-defs.h"

#include <math.h>
#include <signal.h>
#include <sys/timerfd.h>
#include <errno.h>

enum han_state_enum {
	HAN_STATE_WAIT_SYSINFO = 0,
	HAN_STATE_IDLE,
	HAN_STATE_RX_PKT,
	HAN_STATE_CSMA,
	HAN_STATE_TX_PKT,
	HAN_STATE_WAIT_RX_ACK,
	HAN_STATE_TX_ACK,
	HAN_STATE_IFS,
	HAN_STATE_RECONNECT_SERVER,
	HAN_STATE_MAX
};
const char *han_state_str[HAN_STATE_MAX] = {
	"HAN_STATE_WAIT_SYSINFO",
	"HAN_STATE_IDLE",
	"HAN_STATE_RX_PKT",
	"HAN_STATE_CSMA",
	"HAN_STATE_TX_PKT",
	"HAN_STATE_WAIT_RX_ACK",
	"HAN_STATE_TX_ACK",
	"HAN_STATE_IFS",
	"HAN_STATE_RECONNECT_SERVER",
};

typedef void (* han_state_enter)(void);
typedef void (* han_state_exit)(void);
typedef void (* han_state_handle_sysinfo_event)(void);
typedef void (* han_state_handle_rx_pkt)(struct netsim_data_ind_pkt *data_ind);
typedef void (* han_state_handle_start_tx)(void);
typedef void (* han_state_handle_cca_con)(int);
typedef void (* han_state_handle_tx_done_ind)(int);
typedef void (* han_state_handle_lost_server_conn)(void);
typedef void (* han_state_handle_reg_req)(void);
typedef void (* han_state_handle_backoff_timer_expires)(void);
typedef void (* han_state_handle_tx_timer_expires)(void);
typedef void (* han_state_handle_ifs_timer_expires)(void);
typedef void (* han_state_handle_ack_wait_timer_expires)(void);
typedef void (* han_state_handle_reconnect_timer_expires)(void);


struct han_state {
	enum han_state_enum id;
	const char *name;

	han_state_enter enter;
	han_state_exit exit;
	han_state_handle_sysinfo_event handle_sysinfo_event;
	han_state_handle_rx_pkt handle_rx_pkt;
	han_state_handle_start_tx handle_start_tx;
	han_state_handle_cca_con handle_cca_con;
	han_state_handle_tx_done_ind handle_tx_done_ind;
	han_state_handle_lost_server_conn handle_lost_server_conn;
	han_state_handle_reg_req handle_reg_req;
	han_state_handle_backoff_timer_expires handle_backoff_timer_expires;
	han_state_handle_tx_timer_expires handle_tx_timer_expires;
	han_state_handle_ifs_timer_expires handle_ifs_timer_expires;
	han_state_handle_ack_wait_timer_expires handle_ack_wait_timer_expires;
	han_state_handle_reconnect_timer_expires handle_reconnect_timer_expires;
};

void hs_wait_sysinfo_enter(void);
void hs_wait_sysinfo_handle_sysinfo_event(void);
void hs_wait_sysinfo_handle_rx_pkt(struct netsim_data_ind_pkt *data_ind);

void hs_idle_enter(void);
void hs_idle_handle_rx_pkt(struct netsim_data_ind_pkt *data_ind);
void hs_idle_handle_start_tx(void);

void hs_rx_pkt_enter(void);
void hs_rx_pkt_handle_start_tx(void);

void hs_csma_enter(void);
void hs_csma_exit(void);
void hs_csma_handle_rx_pkt(struct netsim_data_ind_pkt *data_ind);
void hs_csma_handle_cca_con(int cca_result);
void hs_csma_handle_backoff_timer_expires(void);

void hs_tx_pkt_enter(void);
void hs_tx_pkt_handle_tx_done_ind(int tx_result);
void hs_tx_pkt_handle_tx_timer_expires(void);

void hs_wait_rx_ack_enter(void);
void hs_wait_rx_ack_handle_rx_pkt(struct netsim_data_ind_pkt *data_ind);
void hs_wait_rx_ack_wait_timer_expires(void);

void hs_tx_ack_enter(void);
void hs_tx_ack_handle_start_tx(void);
void hs_tx_ack_handle_tx_done_ind(int tx_result);

void hs_ifs_enter(void);
void hs_ifs_exit(void);
void hs_ifs_handle_rx_pkt(struct netsim_data_ind_pkt *data_ind);
void hs_ifs_handle_start_tx(void);
void hs_ifs_handle_ifs_timer_expires(void);

void hs_reconnect_server_enter(void);
void hs_reconnect_server_exit(void);
void hs_reconnect_server_handle_start_tx(void);
void hs_reconnect_server_handle_event_cca_con(int cca_result);
void hs_reconnect_server_handle_event_tx_done(int tx_result);
void hs_reconnect_server_handle_reg_req(void);
void hs_reconnect_server_ignore_timer_expiry(void);
void hs_reconnect_server_handle_reconnect_timer_expires(void);


static void hw_all_handle_lost_server_conn(void);

static void
invalid_event(void)
{
	qemu_log_mask(LOG_XSILON, "Erroneously received event whilst in %s", han.state->name);
	abort();
}
static void
invalid_event_cca_con(int cca_result)
{
	qemu_log_mask(LOG_XSILON, "Erroneously received CCA Con whilst in %s", han.state->name);
	abort();
}
static void
invalid_event_rx_pkt(struct netsim_data_ind_pkt *data_ind)
{
	qemu_log_mask(LOG_XSILON, "Erroneously received packet whilst in %s", han.state->name);
	abort();
}
static void
invalid_event_tx_done(int tx_done_result)
{
	qemu_log_mask(LOG_XSILON, "Erroneously received tx done whilst in %s", han.state->name);
	abort();
}
static void
ignore_reconnect_timer_event(void)
{
	qemu_log_mask(LOG_XSILON, "Ignoring reconnect_timer event whilst in %s", han.state->name);
};

static struct han_state han_state_wait_sysinfo = {
	.id = HAN_STATE_WAIT_SYSINFO,
	.name = "HAN_STATE_WAIT_SYSINFO",
	.enter = hs_wait_sysinfo_enter,
	.exit = NULL,
	.handle_sysinfo_event = hs_wait_sysinfo_handle_sysinfo_event,
	.handle_rx_pkt = hs_wait_sysinfo_handle_rx_pkt,
	.handle_start_tx = invalid_event,
	.handle_cca_con = invalid_event_cca_con,
	.handle_tx_done_ind = invalid_event_tx_done,
	.handle_lost_server_conn = hw_all_handle_lost_server_conn,
	.handle_reg_req = invalid_event,
	.handle_backoff_timer_expires = invalid_event,
	.handle_tx_timer_expires = invalid_event,
	.handle_ifs_timer_expires = invalid_event,
	.handle_ack_wait_timer_expires = invalid_event,
	.handle_reconnect_timer_expires = ignore_reconnect_timer_event,
};

static struct han_state han_state_idle = {
	.id = HAN_STATE_IDLE,
	.name = "HAN_STATE_IDLE",
	.enter = hs_idle_enter,
	.exit = NULL,
	.handle_sysinfo_event = invalid_event,
	.handle_rx_pkt = hs_idle_handle_rx_pkt,
	.handle_start_tx = hs_idle_handle_start_tx,
	.handle_cca_con = invalid_event_cca_con,
	.handle_tx_done_ind = invalid_event_tx_done,
	.handle_lost_server_conn = hw_all_handle_lost_server_conn,
	.handle_reg_req = invalid_event,
	.handle_backoff_timer_expires = invalid_event,
	.handle_tx_timer_expires = invalid_event,
	.handle_ifs_timer_expires = invalid_event,
	.handle_ack_wait_timer_expires = invalid_event,
	.handle_reconnect_timer_expires = ignore_reconnect_timer_event,
};

static struct han_state han_state_rx_pkt = {
	.id = HAN_STATE_RX_PKT,
	.name = "HAN_STATE_RX_PKT",
	.enter = hs_rx_pkt_enter,
	.exit = NULL,
	.handle_sysinfo_event = invalid_event,
	.handle_rx_pkt = invalid_event_rx_pkt,
	.handle_start_tx = hs_rx_pkt_handle_start_tx,
	/* TODO: Should we handle this, we could potentially get an Rx Packet
	 * during CSMA and then while in Rx Pkt get the CCA result. */
	.handle_cca_con = invalid_event_cca_con,
	.handle_tx_done_ind = invalid_event_tx_done,
	.handle_lost_server_conn = hw_all_handle_lost_server_conn,
	.handle_reg_req = invalid_event,
	.handle_backoff_timer_expires = invalid_event,
	.handle_tx_timer_expires = invalid_event,
	.handle_ifs_timer_expires = invalid_event,
	.handle_ack_wait_timer_expires = invalid_event,
	.handle_reconnect_timer_expires = ignore_reconnect_timer_event,
};

static struct han_state han_state_csma = {
	.id = HAN_STATE_CSMA,
	.name = "HAN_STATE_CSMA",
	.enter = hs_csma_enter,
	.exit = hs_csma_exit,
	.handle_sysinfo_event = invalid_event,
	.handle_rx_pkt = hs_csma_handle_rx_pkt,
	.handle_start_tx = invalid_event,
	.handle_cca_con = hs_csma_handle_cca_con,
	.handle_tx_done_ind = invalid_event_tx_done,
	.handle_lost_server_conn = hw_all_handle_lost_server_conn,
	.handle_reg_req = invalid_event,
	.handle_backoff_timer_expires = hs_csma_handle_backoff_timer_expires,
	.handle_tx_timer_expires = invalid_event,
	.handle_ifs_timer_expires = invalid_event,
	.handle_ack_wait_timer_expires = invalid_event,
	.handle_reconnect_timer_expires = ignore_reconnect_timer_event,
};
static struct han_state han_state_tx_pkt = {
	.id = HAN_STATE_TX_PKT,
	.name = "HAN_STATE_TX_PKT",
	.enter = hs_tx_pkt_enter,
	.exit = NULL,
	.handle_sysinfo_event = invalid_event,
	.handle_rx_pkt = invalid_event_rx_pkt,
	.handle_start_tx = invalid_event,
	.handle_cca_con = invalid_event_cca_con,
	.handle_tx_done_ind = hs_tx_pkt_handle_tx_done_ind,
	.handle_lost_server_conn = hw_all_handle_lost_server_conn,
	.handle_reg_req = invalid_event,
	.handle_backoff_timer_expires = invalid_event,
	.handle_tx_timer_expires = hs_tx_pkt_handle_tx_timer_expires,
	.handle_ifs_timer_expires = invalid_event,
	.handle_ack_wait_timer_expires = invalid_event,
	.handle_reconnect_timer_expires = ignore_reconnect_timer_event,
};
static struct han_state han_state_wait_rx_ack = {
	.id = HAN_STATE_WAIT_RX_ACK,
	.name = "HAN_STATE_WAIT_RX_ACK",
	.enter = hs_wait_rx_ack_enter,
	.exit = NULL,
	.handle_sysinfo_event = invalid_event,
	.handle_rx_pkt = hs_wait_rx_ack_handle_rx_pkt,
	.handle_start_tx = invalid_event,
	.handle_cca_con = invalid_event_cca_con,
	.handle_tx_done_ind = invalid_event_tx_done,
	.handle_lost_server_conn = hw_all_handle_lost_server_conn,
	.handle_reg_req = invalid_event,
	.handle_backoff_timer_expires = invalid_event,
	.handle_tx_timer_expires = invalid_event,
	.handle_ifs_timer_expires = invalid_event,
	.handle_ack_wait_timer_expires = hs_wait_rx_ack_wait_timer_expires,
	.handle_reconnect_timer_expires = ignore_reconnect_timer_event,
};

static struct han_state han_state_tx_ack = {
	.id = HAN_STATE_TX_ACK,
	.name = "HAN_STATE_TX_ACK",
	.enter = hs_tx_ack_enter,
	.exit = NULL,
	.handle_sysinfo_event = invalid_event,
	.handle_rx_pkt = invalid_event_rx_pkt,
	.handle_start_tx = hs_tx_ack_handle_start_tx,
	.handle_cca_con = invalid_event_cca_con,
	.handle_tx_done_ind = hs_tx_ack_handle_tx_done_ind,
	.handle_lost_server_conn = hw_all_handle_lost_server_conn,
	.handle_reg_req = invalid_event,
	.handle_backoff_timer_expires = invalid_event,
	.handle_tx_timer_expires = invalid_event,
	.handle_ifs_timer_expires = invalid_event,
	.handle_ack_wait_timer_expires = invalid_event,
	.handle_reconnect_timer_expires = ignore_reconnect_timer_event,
};

static struct han_state han_state_ifs = {
	.id = HAN_STATE_IFS,
	.name = "HAN_STATE_IFS",
	.enter = hs_ifs_enter,
	.exit = hs_ifs_exit,
	.handle_sysinfo_event = invalid_event,
	.handle_rx_pkt = hs_ifs_handle_rx_pkt,
	.handle_start_tx = hs_ifs_handle_start_tx,
	.handle_cca_con = invalid_event_cca_con,
	.handle_tx_done_ind = invalid_event_tx_done,
	.handle_lost_server_conn = hw_all_handle_lost_server_conn,
	.handle_reg_req = invalid_event,
	.handle_backoff_timer_expires = invalid_event,
	.handle_tx_timer_expires = invalid_event,
	.handle_ifs_timer_expires = hs_ifs_handle_ifs_timer_expires,
	.handle_ack_wait_timer_expires = invalid_event,
	.handle_reconnect_timer_expires = ignore_reconnect_timer_event,
};

static struct han_state han_state_reconnect_server = {
	.id = HAN_STATE_RECONNECT_SERVER,
	.name = "HAN_STATE_RECONNECT_SERVER",
	.enter = hs_reconnect_server_enter,
	.exit = hs_reconnect_server_exit,
	.handle_sysinfo_event = invalid_event,
	.handle_rx_pkt = invalid_event_rx_pkt,
	.handle_start_tx = hs_reconnect_server_handle_start_tx,
	.handle_cca_con = hs_reconnect_server_handle_event_cca_con,
	.handle_tx_done_ind = hs_reconnect_server_handle_event_tx_done,
	.handle_lost_server_conn = invalid_event,
	.handle_reg_req = hs_reconnect_server_handle_reg_req,
	.handle_backoff_timer_expires = hs_reconnect_server_ignore_timer_expiry,
	.handle_tx_timer_expires = hs_reconnect_server_ignore_timer_expiry,
	.handle_ifs_timer_expires = hs_reconnect_server_ignore_timer_expiry,
	.handle_ack_wait_timer_expires = hs_reconnect_server_ignore_timer_expiry,
	.handle_reconnect_timer_expires = hs_reconnect_server_handle_reconnect_timer_expires,
};


int
han_event_init(struct han_event * event)
{
	return pipe2(event->pipe_fds, O_NONBLOCK | O_CLOEXEC);
}

void
han_event_close(struct han_event * event)
{
	close(event->pipe_fds[0]);
	close(event->pipe_fds[1]);
}

void
han_event_signal(struct han_event * event)
{
	int rv;

	do {
		rv = write(event->pipe_fds[EVENT_PIPE_FD_WRITE], "1", 1);
		assert (rv == 1 || errno == EINTR);
	} while(rv != 1 && errno == EINTR);
}

static bool
hanadu_state_is_listening(void)
{
	return (han.state->id == HAN_STATE_IDLE
		|| han.state->id == HAN_STATE_RX_PKT
		|| han.state->id == HAN_STATE_CSMA
		|| han.state->id == HAN_STATE_WAIT_RX_ACK);
}

static bool
hanadu_state_is_transmitting(void)
{
	return !hanadu_state_is_listening();
}

/*
 * NOTE: It is important that the caller ensures that it returns straight
 * away after performing a state change otherwise we will be running code
 * whilst in a different state.
 */
static void
hanadu_state_change(struct han_state *new_state)
{
	if (han.state)
		qemu_log_mask(LOG_XSILON, "Leaving %s\n", han_state_str[han.state->id]);
	if (han.state != NULL)
		if (han.state->exit != NULL)
			han.state->exit();
	han.state = new_state;
	qemu_log_mask(LOG_XSILON, "Entering %s\n", han_state_str[han.state->id]);
	if (han.state->enter != NULL)
		han.state->enter();
}

static void
start_timer(int timer, long nsecs)
{
	struct itimerspec ts;

	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = nsecs / 1000000000;
	ts.it_value.tv_nsec = (nsecs % 1000000000);

	timerfd_settime(timer, 0, &ts, NULL);
}

static void
stop_timer(int timer)
{
	struct itimerspec ts;

	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 0;
	ts.it_value.tv_nsec = 0;

	timerfd_settime(timer, 0, &ts, NULL);
}

static void
hw_all_handle_lost_server_conn(void)
{
	hanadu_state_change(&han_state_reconnect_server);
}

/* ______________________________________________________ HAN_STATE_WAIT_SYSINFO
 */

void
hs_wait_sysinfo_enter(void)
{
	/* Handle case where we receive sysinfo event before the registration
	 * request, this can happen with RIOT. */
	if (han.sysinfo_avail_latch) {
		/* Simulate the event :) */
		hs_wait_sysinfo_handle_sysinfo_event();
		han.sysinfo_avail_latch = false;
	}
}

void
hs_wait_sysinfo_handle_sysinfo_event(void)
{
	/* TODO: Check return and log errors */
	netsim_tx_reg_con();
	hanadu_state_change(&han_state_idle);
}

void
hs_wait_sysinfo_handle_rx_pkt(struct netsim_data_ind_pkt *data_ind)
{
	/* TODO: For now drop packets in this state */
	assert(data_ind);
	free(data_ind);
	assert(han.rx.data_ind == NULL);
}


/* ______________________________________________________________ HAN_STATE_IDLE
 */

void
hs_idle_enter(void)
{
	/* Check so see if tx start has been set which may have happened whilst
	 * we were in IFS state or we are currently in the process of transmitting
	 * a packet.  The latter can occur when we receive a packet whilst
	 * doing CSMA. */
	if (han.mac.start_tx_latched || han.mac.tx_started) {
		if (han.mac.start_tx_latched)
			assert(han_trxm_tx_start_get(han.trx_dev));

		if (han_mac_lower_mac_bypass_get(han.mac_dev)) {
			hanadu_state_change(&han_state_tx_pkt);
		} else {
			hanadu_state_change(&han_state_csma);
		}
		han.mac.start_tx_latched = false;
	}
}

void
hs_idle_handle_rx_pkt(struct netsim_data_ind_pkt *data_ind)
{
	assert(han.rx.data_ind == NULL);
	han.rx.data_ind = data_ind;
	hanadu_state_change(&han_state_rx_pkt);
}

void
hs_idle_handle_start_tx(void)
{
	/* If MAC Bypassed then go straight to transmitting packet,
	 * otherwise we go into the CSMA state. */
	if (han_mac_lower_mac_bypass_get(han.mac_dev)) {
		hanadu_state_change(&han_state_tx_pkt);
	} else {
		hanadu_state_change(&han_state_csma);
	}
}


/* ____________________________________________________________ HAN_STATE_RX_PKT
 */

static void
log_hexdump(
	const void * start_addr,
	uint16_t dump_byte_len,
	const void * print_address)
{
#define HEXDUMP_BYTES_PER_LINE		(16)
#define HEXDUMP_MAX_HEX_LENGTH		(80)
	char *hex_buff_p;
	char * char_buff_p;
	char *hex_wr_p;
	const char *dump_p = (const char*) start_addr;
	void * row_start_addr =
			(void *)((long long unsigned)print_address & ~((HEXDUMP_BYTES_PER_LINE - 1)));
	unsigned int first_row_start_column = (long long unsigned)print_address % HEXDUMP_BYTES_PER_LINE;
	unsigned int column = 0;
	unsigned int bytes_left = dump_byte_len;
	unsigned int i;

	hex_buff_p = (char *)malloc(HEXDUMP_MAX_HEX_LENGTH + 1);
	char_buff_p = (char *)malloc(HEXDUMP_BYTES_PER_LINE + 1);
	hex_wr_p = hex_buff_p;

	// Print the lead in
	for (i = 0; i < first_row_start_column; i++) {
		hex_wr_p += sprintf(hex_wr_p, ".. ");
		char_buff_p[column++] = ' ';
	}

	while (bytes_left) {
		hex_wr_p += sprintf(hex_wr_p, "%02X ", ((unsigned int)*dump_p) & 0xFF);
		if ((*dump_p >= ' ') && (*dump_p <= '~')) {
			char_buff_p[column] = *dump_p;
		} else {
			char_buff_p[column] = '.';
		}

		dump_p++;
		column++;
		bytes_left--;

		if (column >= HEXDUMP_BYTES_PER_LINE) {
			// Print the completed line
			hex_buff_p[HEXDUMP_MAX_HEX_LENGTH] = '\0';
			char_buff_p[HEXDUMP_BYTES_PER_LINE] = '\0';

			qemu_log_mask(LOG_XSILON, "%p: %s  [%s]\n", row_start_addr, hex_buff_p, char_buff_p);

			row_start_addr = ((char *)row_start_addr + HEXDUMP_BYTES_PER_LINE);
			hex_wr_p = hex_buff_p;
			column = 0;
		}
	}

	if (column) {
		// Print the lead out
		for (i = column; i < HEXDUMP_BYTES_PER_LINE; i++) {
			hex_wr_p += sprintf(hex_wr_p, ".. ");
			char_buff_p[i] = ' ';
		}

		hex_buff_p[HEXDUMP_MAX_HEX_LENGTH] = '\0';
		char_buff_p[HEXDUMP_BYTES_PER_LINE] = '\0';

		qemu_log_mask(LOG_XSILON, "%p: %s  [%s]\n", row_start_addr, hex_buff_p, char_buff_p);
	}

	free(hex_buff_p);
	free(char_buff_p);
}


static void
hanadu_rx_frame_fill_buffer(void)
{
	int i;
	uint8_t fifo_used;
	uint8_t *rxbuf;
	uint8_t *data;

	/* first up get next available buffer */
	i=0;
	while(!(han.rx.bufs_avail_bitmap & (1<<i)) && i < 4)
		i++;
	if (i == 4) {
		/* Overflow */
		han_trxm_rx_mem_bank_overflow_set(han.trx_dev, true);
	} else {
		uint16_t rxind_psdu_len;
		/* Clear the buffer from the bitmap as we are going to use it. */
		han.rx.bufs_avail_bitmap &= ~(1<<i);
		han_trxm_rx_mem_bank_overflow_set(han.trx_dev, false);
		assert(i>=0 && i<4);
		assert(!fifo8_is_full(&han.rx.nextbuf));

		rxind_psdu_len = ntohs(han.rx.data_ind->psdu_len);
		switch(i) {
		case 0:
			han_trxm_rx_psdulen0_set(han.trx_dev, rxind_psdu_len);
			han_trxm_rx_repcode0_set(han.trx_dev, han.rx.data_ind->rep_code);
			//TODO: Premable
			//han_trxm_rx_preamble0_set(han.trx_dev, han.rx.data_ind->preamble);
			han_trxm_rx_mem_bank_full0_flag_set(han.trx_dev, 1);
			break;
		case 1:
			han_trxm_rx_psdulen1_set(han.trx_dev, rxind_psdu_len);
			han_trxm_rx_repcode1_set(han.trx_dev, han.rx.data_ind->rep_code);
			han_trxm_rx_mem_bank_full1_flag_set(han.trx_dev, 1);
			break;
		case 2:
			han_trxm_rx_psdulen2_set(han.trx_dev, rxind_psdu_len);
			han_trxm_rx_repcode2_set(han.trx_dev, han.rx.data_ind->rep_code);
			han_trxm_rx_mem_bank_full2_flag_set(han.trx_dev, 1);
			break;
		case 3:
			han_trxm_rx_psdulen3_set(han.trx_dev, rxind_psdu_len);
			han_trxm_rx_repcode3_set(han.trx_dev, han.rx.data_ind->rep_code);
			han_trxm_rx_mem_bank_full3_flag_set(han.trx_dev, 1);
			break;
		}
		fifo8_push(&han.rx.nextbuf, i);
		han_trxm_rx_mem_bank_fifo_empty_set(han.trx_dev, false);
		fifo_used = (uint8_t)han.rx.nextbuf.num;
		han_trxm_rx_nextbuf_fifo_wr_level_set(han.trx_dev, fifo_used);
		han_trxm_rx_nextbuf_fifo_rd_level_set(han.trx_dev, fifo_used);
		//TODO: Do this per packet once Ian has changed HW.
		han_trxm_rx_rssi_latched_set(han.trx_dev, han.rx.data_ind->rssi);

		/* copy data into the relevant buffer */
		rxbuf = han.rx.buf[i].data;
		data = (uint8_t *)han.rx.data_ind->pktData;

		assert(rxind_psdu_len <= HANADU_MTU);
		memcpy(rxbuf, data, rxind_psdu_len);
	}
}

static bool
hanadu_rx_frame_filter_accept(void)
{
	uint8_t frame_type;
	uint8_t sa_mode;
	uint8_t da_mode;
	uint16_t dst_pan_id;
	uint16_t our_pan_id;

	assert(han.rx.data_ind);

	if (!han_mac_ctrl_filter_enable_get(han.mac_dev)) {
		qemu_log_mask(LOG_XSILON, "FILTER ACCEPT: Promiscuous\n");
		return true;
	}

	frame_type = han.rx.data_ind->pktData[0] & 0x07;
	sa_mode = (han.rx.data_ind->pktData[1] & 0xC0) >> 6;
	da_mode = (han.rx.data_ind->pktData[1] & 0x0C) >> 2;
	if (frame_type == FRAME_TYPE_ACK) {
		qemu_log_mask(LOG_XSILON, "FILTER REJECT: ACK\n");
		return false;
	}
	our_pan_id = han_mac_ctrl_pan_id_get(han.mac_dev);
	if (frame_type == FRAME_TYPE_BEACON) {
		uint16_t src_pan_id;
		src_pan_id = han.rx.data_ind->pktData[3];
		src_pan_id |= (uint16_t)han.rx.data_ind->pktData[4] << 8;

		if (our_pan_id == 0xffff || our_pan_id == src_pan_id) {
			qemu_log_mask(LOG_XSILON, "FILTER ACCEPT: Beacon for us\n");
			return true;
		} else {
			qemu_log_mask(LOG_XSILON, "FILTER REJECT: Beacon not for us\n");
			return false;
		}
	}
	if (da_mode == ADDR_MODE_ELIDED) {
		uint16_t src_pan_id;
		src_pan_id = han.rx.data_ind->pktData[3];
		src_pan_id |= (uint16_t)han.rx.data_ind->pktData[4] << 8;
		if (sa_mode != ADDR_MODE_ELIDED
			&& han_mac_ctrl_pan_coord_get(han.mac_dev)
			&& src_pan_id == our_pan_id) {
			qemu_log_mask(LOG_XSILON, "FILTER ACCEPT: DA_ELIDED\n");
			return true;
		} else {
			qemu_log_mask(LOG_XSILON, "FILTER REJECT: DA_ELIDED\n");
			return false;
		}
	}
	dst_pan_id = han.rx.data_ind->pktData[3];
	dst_pan_id |= (uint16_t)han.rx.data_ind->pktData[4] << 8;
	/* We know destination PAN ID isn't elided */
	if (dst_pan_id != 0xffff && dst_pan_id != our_pan_id) {
		qemu_log_mask(LOG_XSILON, "FILTER REJECT: DEST PAN (0x%04x) not ours (0x%04x) or broadcast\n", dst_pan_id, our_pan_id);
		return false;
	}

	if (da_mode == ADDR_MODE_SHORT) {
		uint16_t dst_short_addr;
		uint16_t our_short_addr;

		dst_short_addr = han.rx.data_ind->pktData[5];
		dst_short_addr |= (uint16_t)han.rx.data_ind->pktData[6] << 8;
		our_short_addr = han_mac_ctrl_short_addr_get(han.mac_dev);
		if (dst_short_addr == 0xffff || dst_short_addr == our_short_addr) {
			qemu_log_mask(LOG_XSILON, "FILTER ACCEPT: DA_SHORT DST_SA(0x%04x) OURS(0x%04x)\n",
					dst_short_addr, our_short_addr);
			return true;
		} else {
			qemu_log_mask(LOG_XSILON, "FILTER REJET: DA_SHORT DST_SA(0x%04x) OURS(0x%04x)\n",
					dst_short_addr, our_short_addr);
			return false;
		}
	}

	if (da_mode == ADDR_MODE_EXTENDED) {
		uint64_t dst_ext_addr;
		uint64_t our_ext_addr;

		dst_ext_addr = han.rx.data_ind->pktData[5];
		dst_ext_addr |= ((uint64_t)han.rx.data_ind->pktData[6]) << 8;
		dst_ext_addr |= ((uint64_t)han.rx.data_ind->pktData[7]) << 16;
		dst_ext_addr |= ((uint64_t)han.rx.data_ind->pktData[8]) << 24;
		dst_ext_addr |= ((uint64_t)han.rx.data_ind->pktData[9]) << 32;
		dst_ext_addr |= ((uint64_t)han.rx.data_ind->pktData[10]) << 40;
		dst_ext_addr |= ((uint64_t)han.rx.data_ind->pktData[11]) << 48;
		dst_ext_addr |= ((uint64_t)han.rx.data_ind->pktData[12]) << 56;

		our_ext_addr = han_mac_ctrl_ea_upper_get(han.mac_dev);
		our_ext_addr <<= 32;
		our_ext_addr |= han_mac_ctrl_ea_lower_get(han.mac_dev);

		if (dst_ext_addr == our_ext_addr) {
			qemu_log_mask(LOG_XSILON, "FILTER ACCEPT: DA_EXT DST_EXT(0x%016llx) OURS(0x%016llx)\n",
					(long long unsigned int)dst_ext_addr, (long long unsigned int)our_ext_addr);
			return true;
		} else {
			qemu_log_mask(LOG_XSILON, "FILTER REJECT: DA_EXT DST_EXT(0x%016llx) OURS(0x%016llx)\n",
					(long long unsigned int)dst_ext_addr, (long long unsigned int)our_ext_addr);
			return false;
		}
	}

	/* Unknown destination addressing mode */
	qemu_log_mask(LOG_XSILON, "FILTER REJECT: DA_UNKNOWN\n");
	return false;
}

void
hs_rx_pkt_enter(void)
{
	/* Filter frame and if for us populate rx buffer and generate
	 * Rx Interrupt. Otherwise drop. */
log_hexdump(han.rx.data_ind->pktData, ntohs(han.rx.data_ind->psdu_len), han.rx.data_ind->pktData);

	if (hanadu_rx_frame_filter_accept()) {

		hanadu_rx_frame_fill_buffer();
		qemu_irq_pulse(han.trx_dev->rx_irq);

		/* If Ack Requests goto Tx Ack state, otherwise Idle */
		if (han.rx.data_ind->pktData[0] & IEEE802154_FC0_AR_BIT) {
			hanadu_state_change(&han_state_tx_ack);
		} else {
			free(han.rx.data_ind);
			han.rx.data_ind = NULL;
			hanadu_state_change(&han_state_idle);
		}
	} else {
		/* Drop frame */
		free(han.rx.data_ind);
		han.rx.data_ind = NULL;
		hanadu_state_change(&han_state_idle);
	}
}

void
hs_rx_pkt_handle_start_tx(void)
{
	/* It is possible to receive a tx start event whilst receiving a packet
	 * as we may have entered IFS, received a packet and transmitted an
	 * ack so in either of these states we need to latch the txstart. */
	han.mac.start_tx_latched = true;
}


/* ______________________________________________________________ HAN_STATE_CSMA
 */

static int
ipow(int base, int exp)
{
	int result = 1;

	while (exp) {
		if (exp & 1)
			result *= base;
		exp >>= 1;
		base *= base;
	}

	return result;
}


static void
start_backoff_timer(void)
{
	int backoff_usecs;

	if (han.mac.wait_free) {
		/* Check channel free every 500 usec */
		backoff_usecs = 500;
	} else {
		int backoff_usecs = (int)random();

		/* TODO: Ian is going to make the backoff period configurable.
		 * Using 10usec for the moment (the * 10) */
		backoff_usecs = ((backoff_usecs % (ipow(2, han.mac.be) - 1)) + 1) * 10;
	}
	assert(backoff_usecs > 0);
	start_timer(han.mac.backoff_timer, backoff_usecs * 1000);
}


static void
clear_channel_assessment(void)
{
	ssize_t rv;

	qemu_log_mask(LOG_XSILON, "CCA request cca_started=true\n");
	rv = netsim_tx_cca_req();
	han.mac.cca_started = true;
	/* TODO: Check return and log error */
	assert(rv > 0);
}


/*
 * We can enter this state under 2 conditions, 1 from Idle in which case we
 * are starting a transmission, the second is where we are currently trying
 * to transmit a packet but we have received a packet.  For the latter siuation
 * we transition to the Rx Pkt state and because tx_started is true transition
 * back to csma state
 */
void
hs_csma_enter(void)
{
	if (!han.mac.tx_started) {
		han.mac.tx_attempt = 0;
		han_mac_ack_retry_attempts_set(han.mac_dev, 0);
		han.mac.tx_started = true;
		assert(!han.mac.csma_started);
	}
	if (!han.mac.csma_started) {
		uint8_t txbuf;

		/* As per 802.15.4 spec set NB to 0 and BE to macMinBE */
		han.mac.nb = 0;
		han.mac.be = han_mac_min_be_get(han.mac_dev);
		han.mac.csma_started = true;

		txbuf = han_trxm_tx_mem_bank_select_get(han.trx_dev);
		assert(txbuf < 2);
		han.mac.ack_requested = han.tx.buf[txbuf].data[0] & IEEE802154_FC0_AR_BIT;
	}

	if (han.mac.be) {
		/* Now delay for random(2^BE-1) unit backoff periods */
		start_backoff_timer();
	} else {
		/* This is the case where macMinBE is set to 0 and as per spec
		 * we go straight to CCA. */
		clear_channel_assessment();
	}
}


void
hs_csma_exit(void)
{
	/* Stop Backoff timer if started */
	stop_timer(han.mac.backoff_timer);
	han.mac.backoff_timer_stopped = true;
}


void
hs_csma_handle_rx_pkt(struct netsim_data_ind_pkt *data_ind)
{
	assert(han.rx.data_ind == NULL);
	han.rx.data_ind = data_ind;
	if (!han.mac.cca_started)
		/* State change will stop back off timer */
		hanadu_state_change(&han_state_rx_pkt);
	else
		qemu_log_mask(LOG_XSILON, "Received data packet in CSMA but we are waiting for CCA result, latch and wait\n");
}

void
hs_csma_handle_cca_con(int cca_result)
{
	han.mac.cca_started = false;
	qemu_log_mask(LOG_XSILON, "hs_csma_handle_cca_con cca_started=false\n");

	/* TODO: What shall we do with the CCA result? for the moment we just
	 * lose it and the CCA procedure will be restarted with the same
	 * parameters. */
	if (han.rx.data_ind) {
		hanadu_state_change(&han_state_rx_pkt);
		return;
	}

	/* Channel Idle? */
	if (cca_result) {
		/* yes */
		han_mac_status_tx_timeout_occured_set(han.mac_dev, false);
		han.mac.csma_started = false;
		han.mac.wait_free = false;
		qemu_log_mask(LOG_XSILON, "Channel Clear\n");
		hanadu_state_change(&han_state_tx_pkt);
	} else {
		int macMaxBE;
		/* no */

		/* TODO: Do we updated register to reflect num back offs here or
		 * once we CSMA finishes */
		/* TODO: Find out from HW guys if num backoffs is accumulated
		 * for each retry or reflects just the last attempt. */
		han.mac.nb++;
		han.mac.be++;
		qemu_log_mask(LOG_XSILON, "Channel Busy NB=%d BE=%d\n", han.mac.nb, han.mac.be);
		assert(!han_mac_status_tx_timeout_occured_get(han.mac_dev));
		han_mac_status_backoff_attempts_set(han.mac_dev, han.mac.nb);
		macMaxBE = han_mac_max_be_get(han.mac_dev);
		if (han.mac.be > macMaxBE)
			han.mac.be = macMaxBE;

		if (han.mac.nb > han_mac_max_csma_backoffs_get(han.mac_dev)) {
			/* CSMA Failure (Tx Timeout) What happens now depends
			 * on the Timeout Strategy (TOS). */
			qemu_log_mask(LOG_XSILON, "CSMA Failure, TOS=%d", han_mac_timeout_strategy_get(han.mac_dev));
			switch(han_mac_timeout_strategy_get(han.mac_dev)) {
			case TOS_DISCARD_TX:
				/* Usual case - Tx failed, set tx timeout and
				 * num backoffs and then generate Tx Interrupt. */
				assert(han_trxm_tx_start_get(han.trx_dev));
				han_mac_status_tx_timeout_occured_set(han.mac_dev, true);
				han_mac_status_backoff_attempts_set(han.mac_dev, han.mac.nb);
				/* Generate Tx Interrupt */
				qemu_irq_pulse(han.trx_dev->tx_irq);
				han.mac.csma_started = false;
				han.mac.tx_started = false;
				hanadu_state_change(&han_state_idle);
				break;
			case TOS_TX_IMMEDIATE:
				/* TODO: Do we set tx_timeout_occured to false here? ask Ian */
				/* CCA failed, just go ahead and Tranmit */
				han.mac.csma_started = false;
				hanadu_state_change(&han_state_tx_pkt);
				break;
			case TOS_CCA_WAIT_THEN_TX:
				han.mac.wait_free = true;
				start_backoff_timer();
				break;
			default:
				abort();
			}
		} else {
			/* Now delay for random(2^BE-1) unit backoff periods */
			start_backoff_timer();
		}
	}
}

void
hs_csma_handle_backoff_timer_expires(void)
{
	/* This will call send and recv which are both async-signal-safe */
	clear_channel_assessment();
}


/* ____________________________________________________________ HAN_STATE_TX_PKT
 */

#if 0
/* TODO: Get number of milliseconds that the current transmission would take. */
static int
getTimeOnWire(void)
{
	return 4;
}
#endif

void
hs_tx_pkt_enter(void)
{
	/* Send buffer to powerline network simulator */
	netsim_tx_data_ind();

	han_trxm_tx_busy_set(han.trx_dev, true);

	/* Now we wait for tx done indication for 5 seconds */
	start_timer(han.mac.tx_timer, 5000000000);
}

void
hs_tx_pkt_handle_tx_done_ind(int tx_result)
{
	han_trxm_tx_busy_set(han.trx_dev, false);
	stop_timer(han.mac.tx_timer);

	qemu_log_mask(LOG_XSILON, "TxDoneInd(%d)\n", tx_result);

	if (han.mac.ack_requested)
		/* TODO: Do we only go into this state if (han_mac_ack_enable_get(han.mac_dev)) { */
		hanadu_state_change(&han_state_wait_rx_ack);
	else
		hanadu_state_change(&han_state_ifs);
}

void
hs_tx_pkt_handle_tx_timer_expires(void)
{
	/* Not received Tx Done Indication, shouldn't happend */
	qemu_log_mask(LOG_XSILON, "Tx Done Ind not received within 5 seconds.");
	abort();
}


/* _______________________________________________________ HAN_STATE_WAIT_RX_ACK
 */

static void
tx_attempt_failed(void)
{
	/* No ack received or seq numbers don't match, increase retry attempts */
	uint8_t max_retries;

	max_retries = han_mac_max_retries_get(han.mac_dev);

	han.mac.tx_attempt++;
	han_mac_ack_retry_attempts_set(han.mac_dev, han.mac.tx_attempt);

	/* Once we reach the Max retries we set the ack success register flag
	 * to false, retries has been set above so that leaves raising the
	 * Tx Interrupt so the driver can drop the packet and update it's stats.
	 */
	if (han.mac.tx_attempt >= max_retries) {
		han_mac_ack_success_set(han.mac_dev, false);

		qemu_irq_pulse(han.trx_dev->tx_irq);

		qemu_log_mask(LOG_XSILON, "Tx Packet failed as attempts(%d) >= MaxRetries(%d)\n", han.mac.tx_attempt, max_retries);
		han.mac.tx_started = false;
		hanadu_state_change(&han_state_idle);
	} else {
		/* As the Tx Attempt has not been acknowledged we need to try
		 * again and perform clear channel assessment again to try and
		 * gain access to the medium. */
		qemu_log_mask(LOG_XSILON, "Tx Packet failed but attempts(%d) < MaxRetries(%d)\n", han.mac.tx_attempt, max_retries);
		hanadu_state_change(&han_state_csma);
	}
}

void
hs_wait_rx_ack_enter(void)
{
	uint16_t wait_dur;

	/* These are 10usecs units */
	wait_dur = han_mac_ack_wait_dur_get(han.mac_dev);
	/* 10000 = 10 usecs in nanoseconds. */
	start_timer(han.mac.ack_wait_timer, wait_dur * 10000);

	/* TODO: We can't really guarantee that the ack will come back in this
	 * time?? Unless we revert to a discrete simulator we may have to
	 * increase this time artificially. */
}

void
hs_wait_rx_ack_handle_rx_pkt(struct netsim_data_ind_pkt *data_ind)
{
	uint8_t ack_seq_num;
	uint16_t ack_fc;

	stop_timer(han.mac.ack_wait_timer);

	/* TODO: Check it is an ack. */
	ack_fc = data_ind->pktData[0];
	ack_fc |= ((uint16_t)data_ind->pktData[1]) >> 8;
	ack_seq_num = data_ind->pktData[2];
	if (han_mac_ack_destination_get(han.mac_dev)) {
		/* Ack data goes to ack data register */
		han_mac_ack_rx_seq_num_set(han.mac_dev, ack_seq_num);
		han_mac_ack_rx_fc_set(han.mac_dev, ack_fc);

	} else {
		/* TODO: for the moment handle han_mac_ack_destination_get() set to register
		 * then for future implement buffer. */

	}

	if (han_mac_ack_seq_check_get(han.mac_dev)) {
		uint8_t tx_pkt_seq_num;

		if (han_trxm_tx_mem_bank_select_get(han.trx_dev) == 0)
			tx_pkt_seq_num = han.tx.buf[0].data[2];
		else
			tx_pkt_seq_num = han.tx.buf[1].data[2];

		if (tx_pkt_seq_num != ack_seq_num) {
			/* TODO: If seq check fails then we should do what we do in expires
			 * and either go back to CSMA or if max retries go to Idle.
			 * Check with HW numpties that this is the case. */
			tx_attempt_failed();
			qemu_log_mask(LOG_XSILON, "Ack Sequence Number mismatch TxPkt(%d) != Ack(%d)\n", tx_pkt_seq_num, ack_seq_num);
			return;
		} else {
			han_mac_ack_success_set(han.mac_dev, true);
		}
	} else {
		han_mac_ack_success_set(han.mac_dev, true);
	}



	/* TODO: IF all is well han_mac_ack_success_set(han.mac_dev, true); */

	free(data_ind);
	hanadu_state_change(&han_state_ifs);
}

void
hs_wait_rx_ack_wait_timer_expires(void)
{
	qemu_log_mask(LOG_XSILON, "Ack Wait Dur expires\n");

	tx_attempt_failed();
}


/* ____________________________________________________________ HAN_STATE_TX_ACK
 */

void
hs_tx_ack_enter(void)
{
	uint8_t seq_num;

	seq_num = han.rx.data_ind->pktData[2];
	/* Transmit Ack to Network Simulator then wait for Tx Done Ind */
	netsim_tx_ack_data_ind(seq_num);

}

void
hs_tx_ack_handle_start_tx(void)
{
	/* It is possible to receive a tx start event whilst transmitting the ack
	 * as we may have entered IFS, received a packet and transmitted an
	 * ack so in either of these states we need to latch the txstart. */
	qemu_log_mask(LOG_XSILON, "StartTx while in Tx Ack state, latch it.\n");
	han.mac.start_tx_latched = true;
}

void
hs_tx_ack_handle_tx_done_ind(int tx_result)
{
	/* Now we have finished with Last Received packet so free it */
	free(han.rx.data_ind);
	han.rx.data_ind = NULL;

	qemu_log_mask(LOG_XSILON, "TxDone while in Tx Ack state, goto idle.\n");
	/* TODO: We will makes acks instant for the moment, may change this to
	 * timer driven, if we stay with the current then we could potentially
	 * lose this state and just move to Rx Pkt.
	 */
	hanadu_state_change(&han_state_idle);
}

/* _______________________________________________________________ HAN_STATE_IFS
 */

void
hs_ifs_enter(void)
{
	uint16_t psdu_len;
	uint8_t ifs_period;

	if (han_trxm_tx_mem_bank_select_get(han.trx_dev) == 0)
		psdu_len = han_trxm_tx_psdu_len0_get(han.trx_dev);
	else
		psdu_len = han_trxm_tx_psdu_len1_get(han.trx_dev);

	if (psdu_len <= han_mac_max_sifs_frame_size_get(han.mac_dev))
		ifs_period = han_mac_sifs_period_get(han.mac_dev);
	else
		ifs_period = han_mac_lifs_period_get(han.mac_dev);
	assert(ifs_period > 0);

	han.mac.tx_started = false;

	/* Even though we have 2 paths into IFS state we can generate Tx
	 * interrupt here to catch both cases. */
	qemu_irq_pulse(han.trx_dev->tx_irq);

	start_timer(han.mac.ifs_timer, ifs_period * IFS_PERIOD_UNIT_NSECS);
}

void
hs_ifs_exit(void)
{
	stop_timer(han.mac.ifs_timer);
	han.mac.ifs_timer_stopped = true;
}

void
hs_ifs_handle_rx_pkt(struct netsim_data_ind_pkt *data_ind)
{
	assert(han.rx.data_ind == NULL);
	han.rx.data_ind = data_ind;
	assert (han.mac.cca_started);
	/* State change will stop IFS timer */
	hanadu_state_change(&han_state_rx_pkt);
}

void
hs_ifs_handle_start_tx(void)
{
	qemu_log_mask(LOG_XSILON, "StartTx whilst in IFS state, latching Tx.\n");
	han.mac.start_tx_latched = true;
}

void
hs_ifs_handle_ifs_timer_expires(void)
{
	hanadu_state_change(&han_state_idle);
}

/* __________________________________________________ HAN_STATE_RECONNECT_SERVER
 */

void
hs_reconnect_server_enter(void)
{
	netsim_comms_close();
	/* Stop all timers that have been started */
	stop_timer(han.mac.backoff_timer);
	han.mac.backoff_timer_stopped = true;
	stop_timer(han.mac.tx_timer);
	stop_timer(han.mac.ack_wait_timer);
	stop_timer(han.mac.ifs_timer);
	han.mac.ifs_timer_stopped = true;

	start_timer(han.mac.reconnect_timer, 2000000000);

}

void
hs_reconnect_server_exit(void)
{
	stop_timer(han.mac.reconnect_timer);
}

void
hs_reconnect_server_handle_start_tx(void)
{
	/* We need to toggle the tx interrupt to stop the driver from
	 * hanging itself. */
	han_mac_status_tx_timeout_occured_set(han.mac_dev, false);
	han_mac_status_backoff_attempts_set(han.mac_dev, 0);
	han_mac_ack_retry_attempts_set(han.mac_dev, 0);
	han_mac_ack_success_set(han.mac_dev, false);
	qemu_irq_pulse(han.trx_dev->tx_irq);
}

void
hs_reconnect_server_handle_event_cca_con(int cca_result)
{
	/* Can ignore */
}

void
hs_reconnect_server_handle_event_tx_done(int tx_result)
{
	/* doesn't really matter any more if we get here so ignore. :) */
}

void
hs_reconnect_server_handle_reg_req(void)
{
	/* TODO: Check return and log errors */
	netsim_tx_reg_con();
	hanadu_state_change(&han_state_idle);
}

void
hs_reconnect_server_ignore_timer_expiry(void)
{
	/* None of the other timers expiring mean anything so we can ignore
	 * in this state, only interested in reconnect timer. */
}

void
hs_reconnect_server_handle_reconnect_timer_expires(void)
{
	/* See if server is back up and running */
	qemu_log_mask(LOG_XSILON, "Trying to reconnect..\n");
	if (netsim_comms_connect() == 0) {
		netsim_rxmcast_init();
		/* TODO Check ret value */
	} else {
		start_timer(han.mac.reconnect_timer, 2000000000);
	}

}

/* ___________________________________________________ Main State Machine Thread
 */

static int
sm_setup_readfs(fd_set * read_fd_set)
{
	int fd_max = 0;
	/* Set up the read file descriptor set to include the server or client
	 * socket and the unblock socket pipe. */
	FD_ZERO(read_fd_set);

	/* TCP Control Socket for receiving messages */
	if (han.netsim.sockfd != -1) {
		FD_SET(han.netsim.sockfd, read_fd_set);
		fd_max = han.netsim.sockfd;
	}

	/* Multicast socket used to receive 802.15.4 traffic */
	if (han.netsim.rxmcast.sockfd != -1) {
		FD_SET(han.netsim.rxmcast.sockfd, read_fd_set);
		if (han.netsim.rxmcast.sockfd > fd_max)
			fd_max = han.netsim.rxmcast.sockfd;
	}

	/* Backoff Timer */
	FD_SET(han.mac.backoff_timer, read_fd_set);
	if (han.mac.backoff_timer > fd_max)
		fd_max = han.mac.backoff_timer;

	/* Tx Timer */
	FD_SET(han.mac.tx_timer, read_fd_set);
	if (han.mac.tx_timer > fd_max)
		fd_max = han.mac.tx_timer;

	/* IFS Timer */
	FD_SET(han.mac.ifs_timer, read_fd_set);
	if (han.mac.ifs_timer > fd_max)
		fd_max = han.mac.ifs_timer;

	/* IFS Timer */
	FD_SET(han.mac.ack_wait_timer, read_fd_set);
	if (han.mac.ack_wait_timer > fd_max)
		fd_max = han.mac.ack_wait_timer;

	/* Reconnect Timer */
	FD_SET(han.mac.reconnect_timer, read_fd_set);
	if (han.mac.reconnect_timer > fd_max)
		fd_max = han.mac.reconnect_timer;

	/* Start Tx Event */
	FD_SET(han.txstart_event.pipe_fds[EVENT_PIPE_FD_READ], read_fd_set);
	if (han.txstart_event.pipe_fds[EVENT_PIPE_FD_READ] > fd_max)
		fd_max = han.txstart_event.pipe_fds[EVENT_PIPE_FD_READ];

	/* Sys Info Available Event */
	FD_SET(han.sysinfo_available.pipe_fds[EVENT_PIPE_FD_READ], read_fd_set);
	if (han.sysinfo_available.pipe_fds[EVENT_PIPE_FD_READ] > fd_max)
		fd_max = han.sysinfo_available.pipe_fds[EVENT_PIPE_FD_READ];

	return fd_max;
}

static uint8_t rxbuf[256];

static void
hanadu_handle_events(int count, fd_set * read_fd_set)
{
	struct netsim_data_ind_pkt *data_ind = NULL;

	han.mac.backoff_timer_stopped = false;
	han.mac.ifs_timer_stopped = false;
	qemu_log_mask(LOG_XSILON, "%s: count=%d\n", __FUNCTION__, count);
	/* TODO: Handle case where x6losim has shutdown */

	/* If packet pending on receive multicast socket, read it and create
	 * data indication message.  We delay handling as we want to handle
	 * before CCA confirm messages but after Tx Done messages if both
	 * events are received at the same time. */
	if (han.netsim.rxmcast.sockfd != -1 && FD_ISSET(han.netsim.rxmcast.sockfd, read_fd_set)) {
		struct netsim_pkt_hdr *hdr;
		struct sockaddr_in cliaddr;
		socklen_t len;
		int n;

		qemu_log_mask(LOG_XSILON, "Event: RxDataPkt %d\n", count);
		do {
			len = sizeof(cliaddr);
			n = recvfrom(han.netsim.rxmcast.sockfd, rxbuf, 256, 0,
						 (struct sockaddr *)&cliaddr, &len);

			if (n == -1) {
				assert (errno != EAGAIN || errno != EWOULDBLOCK);
				if (errno == EINTR)
					continue;
				else
					abort();
			}
		} while (0);
		/* n should be number of bytes received, a return of 0 shouldn't
		 * happen as this is a UDP multicast socket so is connectionless. */

		/* We will receive data packets we send out as they are sent
		 * over the TCP control channel and the network channel then
		 * sends them over the multicast channel. */
		assert(n > sizeof(*hdr));
		hdr = (struct netsim_pkt_hdr *)rxbuf;
		assert(n == ntohs(hdr->len));
		if(!(ntohll(hdr->node_id) == han.netsim.node_id)) {
			data_ind = netsim_rx_tx_data_ind_msg((struct netsim_data_ind_pkt *)hdr);
		}
		count--;
	}


	if (han.netsim.sockfd != -1 && FD_ISSET(han.netsim.sockfd, read_fd_set)) {
		struct netsim_pkt_hdr *hdr;
		void * msg;
		int n;

		qemu_log_mask(LOG_XSILON, "Event: RxControlPkt %d\n", count);
		/* This will dynamically allocate msg */
		n = netsim_rx_read_msg(&msg);
		/* TODO: Handle -EPROTONOSUPPORT as this means the QEMU node
		 * is not compatible with the server. */
		if (n == -ESHUTDOWN) {
			han.state->handle_lost_server_conn();
		} else {
			if (n > 0) {
				hdr = msg;
				assert(hdr);
				switch(ntohs(hdr->msg_type)) {
				case MSG_TYPE_REG_REQ:
					/* This will extract the node id */
					netsim_rx_reg_req_msg(msg);
					if (han.state == NULL) {
						hanadu_state_change(&han_state_wait_sysinfo);
					} else {
						/* Received due to reconnect */
						han.state->handle_reg_req();
					}
					break;
				case MSG_TYPE_DEREG_REQ:
					/* TODO implement MSG_TYPE_DEREG_REQ handler */
					abort();
					break;
				case MSG_TYPE_DEREG_CON:
					/* TODO implement MSG_TYPE_DEREG_CON handler */
					abort();
					break;
				case MSG_TYPE_CCA_CON:
					if (data_ind) {
						han.state->handle_rx_pkt(data_ind);
						data_ind = NULL;
					}
					/* We must handle CCA con even if data indication has
					 * been received. The CSMA state will latch the rx packet
					 * but will wait for the CCA confirm before aborting
					 * CCA and going to Rx Pkt state. */
					han.state->handle_cca_con(netsim_rx_cca_con_msg(msg));
					break;
				case MSG_TYPE_TX_DONE_IND:
					han.state->handle_tx_done_ind(netsim_rx_tx_done_ind_msg(msg));
					break;
				case MSG_TYPE_REG_CON:
				case MSG_TYPE_CCA_REQ:
				case MSG_TYPE_TX_DATA_IND:
				default:
					abort();

				}
				free(msg);
			} else {
				qemu_log_mask(LOG_XSILON, "Failed to read msg\n");
				abort();
			}
		}
		count--;
	}

	if (data_ind) {
		han.state->handle_rx_pkt(data_ind);
		data_ind = NULL;
	}

	if (FD_ISSET(han.mac.backoff_timer, read_fd_set)) {
		uint64_t expirations;
		int n;

		qemu_log_mask(LOG_XSILON, "Event: Backoff Timer Expires %d\n", count);
		if (!han.mac.backoff_timer_stopped)
			assert(han.state->id == HAN_STATE_CSMA);
		do {
			n = read(han.mac.backoff_timer, &expirations, sizeof(expirations));
			if (n == -1 && errno == EINTR)
				continue;
			if (n == -1)
				qemu_log_mask(LOG_XSILON, "Read Fail: %s", strerror(errno));
		} while(0);
		if (n != -1) {
			assert(n == 8);
			assert(expirations == 1);
			/* We may get a case where the backoff timer has expired
			 * at the same time we receive a control or data message
			 * that moves the state out of CSMA and stops the timer.
			 * In this instance we process the expired timer but
			 * do not action the expiry function. */
			if (!han.mac.backoff_timer_stopped)
				han.state->handle_backoff_timer_expires();
		}
		count--;
	}

	if (FD_ISSET(han.mac.tx_timer, read_fd_set)) {
		uint64_t expirations;
		int n;

		qemu_log_mask(LOG_XSILON, "Event: Tx Timer Expires %d\n", count);
		assert(hanadu_state_is_transmitting());
		do {
			n = read(han.mac.tx_timer, &expirations, sizeof(expirations));
			if (n == -1 && errno == EINTR)
				continue;
			if (n == -1)
				qemu_log_mask(LOG_XSILON, "Read Fail: %s", strerror(errno));
		} while(0);
		if (n != -1) {
			assert(n == 8);
			assert(expirations == 1);
			han.state->handle_tx_timer_expires();
		}
		count--;
	}

	if (FD_ISSET(han.mac.ifs_timer, read_fd_set)) {
		uint64_t expirations;
		int n;

		qemu_log_mask(LOG_XSILON, "Event: IFS Timer Expires %d\n", count);
		do {
			n = read(han.mac.ifs_timer, &expirations, sizeof(expirations));
			if (n == -1 && errno == EINTR)
				continue;
			if (n == -1)
				qemu_log_mask(LOG_XSILON, "Read Fail: %s", strerror(errno));
		} while(0);
		if (n != -1) {
			assert(n == 8);
			assert(expirations == 1);
			/* IFS timer might expire at the same time we receive
			 * a packet in the IFS state so we check here. */
			if (!han.mac.ifs_timer_stopped)
				han.state->handle_ifs_timer_expires();
		}
		count--;
	}

	if (FD_ISSET(han.mac.ack_wait_timer, read_fd_set)) {
		uint64_t expirations;
		int n;

		qemu_log_mask(LOG_XSILON, "Event: Ack Wait Timer Expires %d\n", count);
		do {
			n = read(han.mac.ack_wait_timer, &expirations, sizeof(expirations));
			if (n == -1 && errno == EINTR)
				continue;
			if (n == -1)
				qemu_log_mask(LOG_XSILON, "Read Fail: %s", strerror(errno));
		} while(0);
		if (n != -1) {
			assert(n == 8);
			assert(expirations == 1);
			han.state->handle_ack_wait_timer_expires();
		}
		count--;
	}

	if (FD_ISSET(han.mac.reconnect_timer, read_fd_set)) {
		uint64_t expirations;
		int n;

		qemu_log_mask(LOG_XSILON, "Event: Reconnect Timer Expires %d\n", count);
		do {
			n = read(han.mac.reconnect_timer, &expirations, sizeof(expirations));
			if (n == -1 && errno == EINTR)
				continue;
			if (n == -1)
				qemu_log_mask(LOG_XSILON, "Read Fail: %s", strerror(errno));
		} while(0);
		if (n != -1) {
			assert(n == 8);
			assert(expirations == 1);
			han.state->handle_reconnect_timer_expires();
		}
		count--;
	}

	if (FD_ISSET(han.sysinfo_available.pipe_fds[EVENT_PIPE_FD_READ], read_fd_set)) {
		char tmp;
		int n;

		qemu_log_mask(LOG_XSILON, "Event: Sys Info Available %d\n", count);
		do {
			n = read(han.sysinfo_available.pipe_fds[EVENT_PIPE_FD_READ], &tmp, 1);
			if (n == -1 && errno == EINTR)
				continue;
			if (n == -1)
				qemu_log_mask(LOG_XSILON, "Read Fail: %s", strerror(errno));
		} while(0);
		if (n != -1) {
			assert(n == 1);
			if (han.state)
				han.state->handle_sysinfo_event();
			else
				han.sysinfo_avail_latch = true;
		}
		count --;
	}

	if (FD_ISSET(han.txstart_event.pipe_fds[EVENT_PIPE_FD_READ], read_fd_set)) {
		char tmp;
		int n;

		qemu_log_mask(LOG_XSILON, "Event: Tx Start %d\n", count);
		do {
			n = read(han.txstart_event.pipe_fds[EVENT_PIPE_FD_READ], &tmp, 1);
			if (n == -1 && errno == EINTR)
				continue;
			if (n == -1)
				qemu_log_mask(LOG_XSILON, "Read Fail: %s", strerror(errno));
		} while(0);
		if (n != -1) {
			assert(n == 1);
			han.state->handle_start_tx();
		}
		count --;
	}

	assert(count == 0);
}

/*
 * Bascially handles all the different events that occur in the State Machine
 * by utilising the unix methodology of everything is a file.  Timers use
 * timerfd, Sockets are file descriptors leaving events from the CPU such as
 * register writes using pipes.  We can then multiplex all these using select.
 * This avoids having to use multiple process/threads, signal handlers for timers
 * and trying to synchronise all this and avoid race conditions.
 * If 2 events occur simultaneously then after select finishes we can priortise
 * them avoiding possible race conditions.
 */
void *
hanadu_modem_model_thread(void *arg)
{
	int fdmax;
	int count;
	fd_set read_fd_set;

	/* Initial state is NULL, we should receive registration request which
	 * will trigger a state change to wait for sysinfo.
	 */
	han.state = NULL;

	for (;;) {

		/* Setup readfs set */
		fdmax = sm_setup_readfs(&read_fd_set);

		assert(fdmax > 0);
		/* Wait forever on it read fd's */
		count = select(fdmax+1, &read_fd_set, NULL, NULL, NULL);

		/* Process events */
		if(count == -1) {
			if (errno == EINTR) {
				/* Select system call was interrupted by a signal so restart
				 * the system call */
				continue;
			}
			qemu_log_mask(LOG_XSILON, "%s: select failed (%s)",
					__FUNCTION__, strerror(errno));
			abort();
		} else if(count == 0) {
			/* Timeout Shouldn't happen!!!! */
			abort();
		} else {
			hanadu_handle_events(count, &read_fd_set);
		}
	}

	close(han.netsim.sockfd);
	close(han.netsim.rxmcast.sockfd);
	han_event_close(&han.txstart_event);
	han_event_close(&han.sysinfo_available);
	close(han.mac.backoff_timer);
	close(han.mac.tx_timer);
	close(han.mac.ifs_timer);
	close(han.mac.ack_wait_timer);

	pthread_exit(NULL);
}

