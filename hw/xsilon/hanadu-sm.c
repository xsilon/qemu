/*
 * This file contains the main implementation of the Hanadu HW modem device's
 * State Machine as documented by the Papyrus Model in x6losim.
 *
 * Author Martin Townsend
 *        skype: mtownsend1973
 *        email: martin.townsend@xsilon.com
 *               mtownsend1973@gmail.com
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
};

typedef void (* han_state_enter)(void);
typedef void (* han_state_exit)(void);
typedef void (* han_state_handle_sysinfo_event)(void);
typedef void (* han_state_handle_rx_pkt)(struct netsim_data_ind_pkt *data_ind);
typedef void (* han_state_handle_start_tx)(void);
typedef void (* han_state_handle_cca_con)(int);
typedef void (* han_state_handle_tx_done_ind)(int);
typedef void (* han_state_handle_backoff_timer_expires)(void);
typedef void (* han_state_handle_tx_timer_expires)(void);
typedef void (* han_state_handle_ifs_timer_expires)(void);

struct han_state {
	enum han_state_enum id;

	han_state_enter enter;
	han_state_exit exit;
	han_state_handle_sysinfo_event handle_sysinfo_event;
	han_state_handle_rx_pkt handle_rx_pkt;
	han_state_handle_start_tx handle_start_tx;
	han_state_handle_cca_con handle_cca_con;
	han_state_handle_tx_done_ind handle_tx_done_ind;
	han_state_handle_backoff_timer_expires handle_backoff_timer_expires;
	han_state_handle_tx_timer_expires handle_tx_timer_expires;
	han_state_handle_ifs_timer_expires handle_ifs_timer_expires;
};

void hs_wait_sysinfo_handle_sysinfo_event(void);

void hs_idle_enter(void);
void hs_idle_handle_rx_pkt(struct netsim_data_ind_pkt *data_ind);
void hs_idle_handle_start_tx(void);

void hs_csma_enter(void);
void hs_csma_exit(void);
void hs_csma_handle_cca_con(int cca_result);
void hs_csma_handle_backoff_timer_expires(void);

void hs_tx_pkt_enter(void);
void hs_tx_pkt_handle_tx_done_ind(int tx_result);
void hs_tx_pkt_handle_tx_timer_expires(void);

void hs_ifs_enter(void);
void hs_ifs_handle_start_tx(void);
void hs_ifs_handle_ifs_timer_expires(void);

static void
invalid_event(void)
{
	abort();
}
static void
invalid_event_cca_con(int cca_result)
{
	abort();
}
static void
invalid_event_rx_pkt(struct netsim_data_ind_pkt *data_ind)
{
	abort();
}
static void
invalid_event_tx_done(int tx_done_result)
{
	abort();
}

static struct han_state han_state_wait_sysinfo = {
	.id = HAN_STATE_WAIT_SYSINFO,
	.enter = NULL,
	.exit = NULL,
	.handle_sysinfo_event = hs_wait_sysinfo_handle_sysinfo_event,
	.handle_rx_pkt = invalid_event_rx_pkt,
	.handle_start_tx = invalid_event,
	.handle_cca_con = invalid_event_cca_con,
	.handle_tx_done_ind = invalid_event_tx_done,
	.handle_backoff_timer_expires = invalid_event,
	.handle_tx_timer_expires = invalid_event,
	.handle_ifs_timer_expires = invalid_event,

};

static struct han_state han_state_idle = {
	.id = HAN_STATE_IDLE,
	.enter = hs_idle_enter,
	.exit = NULL,
	.handle_sysinfo_event = invalid_event,
	.handle_rx_pkt = hs_idle_handle_rx_pkt,
	.handle_start_tx = hs_idle_handle_start_tx,
	.handle_cca_con = invalid_event_cca_con,
	.handle_tx_done_ind = invalid_event_tx_done,
	.handle_backoff_timer_expires = invalid_event,
	.handle_tx_timer_expires = invalid_event,
	.handle_ifs_timer_expires = invalid_event,
};

#if 0
static struct han_state han_state_rx_pkt = {
	.id = HAN_STATE_RX_PKT,
	.enter = NULL,
	.exit = NULL,
	.handle_sysinfo_event = invalid_event,
	.handle_rx_pkt = invalid_event_rx_pkt,
	.handle_start_tx = invalid_event,
	.handle_cca_con = invalid_event_cca_con,
	.handle_tx_done_ind = invalid_event_tx_done,
	.handle_backoff_timer_expires = invalid_event,
	.handle_tx_timer_expires = invalid_event,
	.handle_ifs_timer_expires = invalid_event,
};
#endif
static struct han_state han_state_csma = {
	.id = HAN_STATE_CSMA,
	.enter = hs_csma_enter,
	.exit = hs_csma_exit,
	.handle_sysinfo_event = invalid_event,
	.handle_rx_pkt = invalid_event_rx_pkt,
	.handle_start_tx = invalid_event,
	.handle_cca_con = hs_csma_handle_cca_con,
	.handle_tx_done_ind = invalid_event_tx_done,
	.handle_backoff_timer_expires = hs_csma_handle_backoff_timer_expires,
	.handle_tx_timer_expires = invalid_event,
	.handle_ifs_timer_expires = invalid_event,
};
static struct han_state han_state_tx_pkt = {
	.id = HAN_STATE_TX_PKT,
	.enter = hs_tx_pkt_enter,
	.exit = NULL,
	.handle_sysinfo_event = invalid_event,
	.handle_rx_pkt = invalid_event_rx_pkt,
	.handle_start_tx = invalid_event,
	.handle_cca_con = invalid_event_cca_con,
	.handle_tx_done_ind = hs_tx_pkt_handle_tx_done_ind,
	.handle_backoff_timer_expires = invalid_event,
	.handle_tx_timer_expires = hs_tx_pkt_handle_tx_timer_expires,
	.handle_ifs_timer_expires = invalid_event,
};
static struct han_state han_state_wait_rx_ack = {
	.id = HAN_STATE_WAIT_RX_ACK,
	.enter = NULL,
	.exit = NULL,
	.handle_sysinfo_event = invalid_event,
	.handle_rx_pkt = invalid_event_rx_pkt,
	.handle_start_tx = invalid_event,
	.handle_cca_con = invalid_event_cca_con,
	.handle_tx_done_ind = invalid_event_tx_done,
	.handle_backoff_timer_expires = invalid_event,
	.handle_tx_timer_expires = invalid_event,
	.handle_ifs_timer_expires = invalid_event,
};
#if 0
static struct han_state han_state_tx_ack = {
	.id = HAN_STATE_TX_ACK,
	.enter = NULL,
	.exit = NULL,
	.handle_sysinfo_event = invalid_event,
	.handle_rx_pkt = invalid_event_rx_pkt,
	.handle_start_tx = invalid_event,
	.handle_cca_con = invalid_event_cca_con,
	.handle_tx_done_ind = invalid_event_tx_done,
	.handle_backoff_timer_expires = invalid_event,
	.handle_tx_timer_expires = invalid_event,
	.handle_ifs_timer_expires = invalid_event,
};
#endif

static struct han_state han_state_ifs = {
	.id = HAN_STATE_IFS,
	.enter = hs_ifs_enter,
	.exit = NULL,
	.handle_sysinfo_event = invalid_event,
	.handle_rx_pkt = invalid_event_rx_pkt,
	.handle_start_tx = hs_ifs_handle_start_tx,
	.handle_cca_con = invalid_event_cca_con,
	.handle_tx_done_ind = invalid_event_tx_done,
	.handle_backoff_timer_expires = invalid_event,
	.handle_tx_timer_expires = invalid_event,
	.handle_ifs_timer_expires = hs_ifs_handle_ifs_timer_expires,
};

int
han_event_init(struct han_event * event)
{
	return pipe2(event->pipe_fds, O_NONBLOCK | O_CLOEXEC);
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

/* TODO: Eventually these may have to be protected by a mutex as we have 2
 * different threads accessing the han.state variable.  */
#if 0
static enum han_state_enum
hanadu_state_get(void)
{
	return han.state->id;
}
#endif

static void
hanadu_state_change(struct han_state *new_state)
{
	if (han.state)
		fprintf(stderr, "Leaving %s\n", han_state_str[han.state->id]);
	if (han.state != NULL)
		if (han.state->exit != NULL)
			han.state->exit();
	han.state = new_state;
	fprintf(stderr, "Entering %s\n", han_state_str[han.state->id]);
	if (han.state->enter != NULL)
		han.state->enter();
}

/* ______________________________________________________ HAN_STATE_WAIT_SYSINFO
 */

void
hs_wait_sysinfo_handle_sysinfo_event(void)
{
	/* TODO: Check return and log errors */
	netsim_tx_reg_con();
	hanadu_state_change(&han_state_idle);
}

/* ______________________________________________________________ HAN_STATE_IDLE
 */

void
hs_idle_enter(void)
{
	/* Check so see if tx start has been set which may have happened whilst
	 * we were in IFS state. */
	if (han.mac.start_tx_latched) {
		assert(han_trxm_tx_start_get(han.trx_dev));

		hanadu_state_change(&han_state_tx_pkt);
		han.mac.start_tx_latched = false;
	}
}

void
hs_idle_handle_rx_pkt(struct netsim_data_ind_pkt *data_ind)
{

}

/*
 * QEMU Context: CPU thread (han_trxm_mem_region_write)
 */
void
hs_idle_handle_start_tx(void)
{
	fprintf(stderr, "IDLE: Handle Tx Start\n");

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

/* ______________________________________________________________ HAN_STATE_CSMA
 */
/*  */
//hanadu_tx_buffer_to_netsim();
//hanadu_state_change(&han_state_idle);


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
	struct itimerspec ts;

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
	fprintf(stderr, "CCA Backoff %d usecs\n", backoff_usecs);
	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 0;
	ts.it_value.tv_nsec = backoff_usecs * 1000;

	timerfd_settime(han.mac.backoff_timer, 0, &ts, NULL);
}

static void
clear_channel_assessment(void)
{
	ssize_t rv;

	fprintf(stderr, "CCA\n");
	rv = netsim_tx_cca_req();
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
		uint8_t txbuf;

		/* As per 802.15.4 spec set NB to 0 and BE to macMinBE */
		han.mac.nb = 0;
		han.mac.be = han_mac_min_be_get(han.mac_dev);
		han.mac.tx_started = true;

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
	/* Ensure wait free is cleared */
	han.mac.wait_free = false;
}

void
hs_csma_handle_cca_con(int cca_result)
{
	fprintf(stderr, "CCA CON [%d]\n", cca_result);
	/* Channel Idle? */
	if (cca_result) {
		/* yes */
		han_mac_status_tx_timeout_occured_set(han.mac_dev, false);
		hanadu_state_change(&han_state_tx_pkt);
	} else {
		int macMaxBE;
		/* no */

		/* TODO: Do we updated register to reflect num back offs here or
		 * once we CSMA finishes */
		han.mac.nb++;
		han.mac.be++;
		assert(!han_mac_status_tx_timeout_occured_get(han.mac_dev));
		han_mac_status_backoff_attempts_set(han.mac_dev, han.mac.nb);
		macMaxBE = han_mac_max_be_get(han.mac_dev);
		if (han.mac.be > macMaxBE)
			han.mac.be = macMaxBE;

		if (han.mac.nb > han_mac_max_csma_backoffs_get(han.mac_dev)) {
			/* CSMA Failure (Tx Timeout) What happens now depends
			 * on the Timeout Strategy (TOS). */
			switch(han_mac_timeout_strategy_get(han.mac_dev)) {
			case TOS_DISCARD_TX:
				/* Usual case - Tx failed, set tx timeout and
				 * num backoffs and then generate Tx Interrupt. */
				assert(!han_trxm_tx_start_get(han.trx_dev));
				han_mac_status_tx_timeout_occured_set(han.mac_dev, true);
				han_mac_status_backoff_attempts_set(han.mac_dev, han.mac.nb);
				/* Generate Tx Interrupt */
				fprintf(stderr, "TX INTERRUPT: FAIL\n");
				qemu_irq_pulse(han.trx_dev->tx_irq);
				hanadu_state_change(&han_state_idle);
				break;
			case TOS_TX_IMMEDIATE:
				/* TODO: Do we set tx_timeout_occured to false here? ask Ian */
				/* CCA failed, just go ahead and Tranmit */
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


static void
start_tx_timer(void)
{
	struct itimerspec ts;

	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 2;
	ts.it_value.tv_nsec = 0;

	timerfd_settime(han.mac.tx_timer, 0, &ts, NULL);
}

static void
stop_tx_timer(void)
{
	struct itimerspec ts;

	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 0;
	ts.it_value.tv_nsec = 0;

	timerfd_settime(han.mac.tx_timer, 0, &ts, NULL);
}

void
hs_tx_pkt_enter(void)
{
	/* Send buffer to powerline network simulator */
	netsim_tx_data_ind();

	han_trxm_tx_busy_set(han.trx_dev, true);

	/* Now we wait for tx done indication */
	start_tx_timer();
}

void
hs_tx_pkt_handle_tx_done_ind(int tx_result)
{
	han_trxm_tx_busy_set(han.trx_dev, false);
	stop_tx_timer();
	if (han.mac.ack_requested)
		hanadu_state_change(&han_state_wait_rx_ack);
	else
		hanadu_state_change(&han_state_ifs);
}

void
hs_tx_pkt_handle_tx_timer_expires(void)
{
	/* Not received Tx Done Indication */
	abort();
}


/* _______________________________________________________ HAN_STATE_WAIT_RX_ACK
 */

/* ____________________________________________________________ HAN_STATE_TX_ACK
 */

/* _______________________________________________________________ HAN_STATE_IFS
 */

static void
start_ifs_timer(void)
{
	struct itimerspec ts;
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
	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 0;
	ts.it_value.tv_nsec = ifs_period * IFS_PERIOD_UNIT_NSECS;

	timerfd_settime(han.mac.ifs_timer, 0, &ts, NULL);
}

void
hs_ifs_enter(void)
{
	han.mac.tx_started = false;

	/* Even though we have 2 paths into IFS state we can generate Tx
	 * interrupt here to catch both cases. */
	qemu_irq_pulse(han.trx_dev->tx_irq);
	fprintf(stderr, "TX INTERRUPT: GOOD\n");

	start_ifs_timer();
}

void
hs_ifs_handle_start_tx(void)
{
	han.mac.start_tx_latched = true;
}

void
hs_ifs_handle_ifs_timer_expires(void)
{
	hanadu_state_change(&han_state_idle);
}

/* ___________________________________________________ Main State Machine Thread
 */

static int
sm_setup_readfs(fd_set * read_fd_set)
{
	int fd_max;
	/* Set up the read file descriptor set to include the server or client
	 * socket and the unblock socket pipe. */
	FD_ZERO(read_fd_set);

	/* TCP Control Socket for receiving messages */
	FD_SET(han.netsim.sockfd, read_fd_set);
	fd_max = han.netsim.sockfd;

	/* Multicast socket used to receive 802.15.4 traffic */
	FD_SET(han.netsim.rxmcast.sockfd, read_fd_set);
	if (han.netsim.rxmcast.sockfd > fd_max)
		fd_max = han.netsim.rxmcast.sockfd;

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
	bool event_handled = false;

	if (FD_ISSET(han.netsim.rxmcast.sockfd, read_fd_set)) {
		struct netsim_pkt_hdr *hdr;
		struct netsim_data_ind_pkt *data_ind;
		struct sockaddr_in cliaddr;
		socklen_t len;
		int n;

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
		if(!(ntohll(hdr->node_id) == han.netsim.node_id)) {
			data_ind = netsim_rx_tx_data_ind_msg((struct netsim_data_ind_pkt *)hdr);
			han.state->handle_rx_pkt(data_ind);
			event_handled = true;
		}
		count--;
	}

	/* TODO: What to do if we receive a packet and something on the control
	 * channel. */

	if (FD_ISSET(han.netsim.sockfd, read_fd_set)) {
		struct netsim_pkt_hdr *hdr;
		void * msg;

		/* This will dynamically allocate msg */
		msg = netsim_rx_read_msg();
		hdr = msg;
		assert(hdr);
		switch(ntohs(hdr->msg_type)) {
		case MSG_TYPE_REG_REQ:
			assert(han.state == NULL);
			netsim_rx_reg_req_msg(msg);
			hanadu_state_change(&han_state_wait_sysinfo);
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
		event_handled = true;
		count--;
	}

	if (FD_ISSET(han.mac.backoff_timer, read_fd_set)) {
		uint64_t expirations;
		int n;

		assert(han.state->id == HAN_STATE_CSMA);
		do {
			n = read(han.mac.backoff_timer, &expirations, sizeof(expirations));
			if (n == -1 && errno == EINTR)
				continue;
		} while(0);
		assert(n == 8);
		assert(expirations == 1);
		han.state->handle_backoff_timer_expires();
		event_handled = true;
		count--;
	}

	if (FD_ISSET(han.mac.tx_timer, read_fd_set)) {
		uint64_t expirations;
		int n;

		assert(hanadu_state_is_transmitting());
		do {
			n = read(han.mac.tx_timer, &expirations, sizeof(expirations));
			if (n == -1 && errno == EINTR)
				continue;
		} while(0);
		assert(n == 8);
		assert(expirations == 1);
		han.state->handle_tx_timer_expires();

		event_handled = true;
		count--;
	}

	if (FD_ISSET(han.mac.ifs_timer, read_fd_set)) {
		uint64_t expirations;
		int n;

		do {
			n = read(han.mac.ifs_timer, &expirations, sizeof(expirations));
			if (n == -1 && errno == EINTR)
				continue;
		} while(0);
		assert(n == 8);
		assert(expirations == 1);
		han.state->handle_ifs_timer_expires();
		event_handled = true;
		count--;
	}

	if (FD_ISSET(han.sysinfo_available.pipe_fds[EVENT_PIPE_FD_READ], read_fd_set)) {
		char tmp;
		int n;

		do {
			n = read(han.sysinfo_available.pipe_fds[EVENT_PIPE_FD_READ], &tmp, 1);
			if (n == -1 && errno == EINTR)
				continue;
		} while(0);
		assert(n == 1);
		han.state->handle_sysinfo_event();
		event_handled = true;
		count --;
	}

	if (FD_ISSET(han.txstart_event.pipe_fds[EVENT_PIPE_FD_READ], read_fd_set)) {
		char tmp;
		int n;

		fprintf(stderr, "TX START EVENT");
		do {
			n = read(han.txstart_event.pipe_fds[EVENT_PIPE_FD_READ], &tmp, 1);
			if (n == -1 && errno == EINTR)
				continue;
		} while(0);
		assert(n == 1);
		han.state->handle_start_tx();
		event_handled = true;
		count --;
	}

	//TODO: Remove, just here to get rid of compiler warning
	if (event_handled) {

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
			abort();
		} else if(count == 0) {
			/* Timeout Shouldn't happen!!!! */
			abort();
		} else {
			hanadu_handle_events(count, &read_fd_set);
		}
	}

	//TODO: Cleanup all file descriptors ???
	close(han.netsim.rxmcast.sockfd);
	pthread_exit(NULL);
}
