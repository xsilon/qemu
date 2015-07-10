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

void hs_idle_enter(void);
void hs_idle_handle_rx_pkt(struct netsim_data_ind_pkt *data_ind);
void hs_idle_handle_start_tx(void);

void hs_csma_enter(void);
void hs_csma_exit(void);
void hs_csma_handle_cca_con(int cca_result);
void hs_csma_handle_backoff_timer_expires(void);

void hs_tx_pkt_enter(void);
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

static struct han_state han_state_idle = {
	.id = HAN_STATE_IDLE,
	.enter = hs_idle_enter,
	.exit = NULL,
	.handle_rx_pkt = hs_idle_handle_rx_pkt,
	.handle_start_tx = hs_idle_handle_start_tx,
	.handle_cca_con = invalid_event_cca_con,
	.handle_tx_done_ind = invalid_event,
	.handle_backoff_timer_expires = invalid_event,
	.handle_tx_timer_expires = invalid_event,
	.handle_ifs_timer_expires = invalid_event,
};

#if 0
static struct han_state han_state_rx_pkt = {
	.id = HAN_STATE_RX_PKT,
	.enter = NULL,
	.exit = NULL,
	.handle_rx_pkt = invalid_event_rx_pkt,
	.handle_start_tx = invalid_event,
	.handle_cca_con = invalid_event_cca_con,
	.handle_tx_done_ind = invalid_event,
	.handle_backoff_timer_expires = invalid_event,
	.handle_tx_timer_expires = invalid_event,
	.handle_ifs_timer_expires = invalid_event,
};
#endif
static struct han_state han_state_csma = {
	.id = HAN_STATE_CSMA,
	.enter = hs_csma_enter,
	.exit = hs_csma_exit,
	.handle_rx_pkt = invalid_event_rx_pkt,
	.handle_start_tx = invalid_event,
	.handle_cca_con = hs_csma_handle_cca_con,
	.handle_tx_done_ind = invalid_event,
	.handle_backoff_timer_expires = hs_csma_handle_backoff_timer_expires,
	.handle_tx_timer_expires = invalid_event,
	.handle_ifs_timer_expires = invalid_event,
};
static struct han_state han_state_tx_pkt = {
	.id = HAN_STATE_TX_PKT,
	.enter = hs_tx_pkt_enter,
	.exit = NULL,
	.handle_rx_pkt = invalid_event_rx_pkt,
	.handle_start_tx = invalid_event,
	.handle_cca_con = invalid_event_cca_con,
	.handle_tx_done_ind = invalid_event,
	.handle_backoff_timer_expires = invalid_event,
	.handle_tx_timer_expires = hs_tx_pkt_handle_tx_timer_expires,
	.handle_ifs_timer_expires = invalid_event,
};
static struct han_state han_state_wait_rx_ack = {
	.id = HAN_STATE_WAIT_RX_ACK,
	.enter = NULL,
	.exit = NULL,
	.handle_rx_pkt = invalid_event_rx_pkt,
	.handle_start_tx = invalid_event,
	.handle_cca_con = invalid_event_cca_con,
	.handle_tx_done_ind = invalid_event,
	.handle_backoff_timer_expires = invalid_event,
	.handle_tx_timer_expires = invalid_event,
	.handle_ifs_timer_expires = invalid_event,
};
#if 0
static struct han_state han_state_tx_ack = {
	.id = HAN_STATE_TX_ACK,
	.enter = NULL,
	.exit = NULL,
	.handle_rx_pkt = invalid_event_rx_pkt,
	.handle_start_tx = invalid_event,
	.handle_cca_con = invalid_event_cca_con,
	.handle_tx_done_ind = invalid_event,
	.handle_backoff_timer_expires = invalid_event,
	.handle_tx_timer_expires = invalid_event,
	.handle_ifs_timer_expires = invalid_event,
};
#endif

static struct han_state han_state_ifs = {
	.id = HAN_STATE_IFS,
	.enter = hs_ifs_enter,
	.exit = NULL,
	.handle_rx_pkt = invalid_event_rx_pkt,
	.handle_start_tx = hs_ifs_handle_start_tx,
	.handle_cca_con = invalid_event_cca_con,
	.handle_tx_done_ind = invalid_event,
	.handle_backoff_timer_expires = invalid_event,
	.handle_tx_timer_expires = invalid_event,
	.handle_ifs_timer_expires = hs_ifs_handle_ifs_timer_expires,
};


bool
hanadu_state_is_listening(void)
{
	return (han.state->id == HAN_STATE_IDLE
		|| han.state->id == HAN_STATE_RX_PKT
		|| han.state->id == HAN_STATE_CSMA
		|| han.state->id == HAN_STATE_WAIT_RX_ACK);
}

bool
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

void
hanadu_state_change(struct han_state *new_state)
{
	pthread_mutex_lock(&han.sm_mutex);
	if (han.state != NULL)
		if (han.state->exit != NULL)
			han.state->exit();
	han.state = new_state;
	if (han.state->enter != NULL)
		han.state->enter();
	pthread_mutex_unlock(&han.sm_mutex);
}

struct han_state *
hanadu_state_idle_get(void)
{
	return &han_state_idle;
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

/* QEMU Context: QEMU thread */
void
backoff_timer_signal_handler(int sig, siginfo_t *si, void *uc)
{
	/* We are in a signal hander, no non-async-signal-safe calls */

	assert(han.state->id == HAN_STATE_CSMA);
	han.state->handle_backoff_timer_expires();
}

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
	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 0;
	ts.it_value.tv_nsec = backoff_usecs * 1000;

	timer_settime(han.mac.backoff_timer, 0, &ts, NULL);
}

static void
clear_channel_assessment(void)
{
	int cca_result;
	ssize_t rv;

	rv = netsim_tx_cca_req();
	/* TODO: Check return and log error */
	assert(rv > 0);
	cca_result = netsim_rx_cca_con_msg();
	han.state->handle_cca_con(cca_result);

}

/*
 * QEMU Context: CPU thread.
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

/*
 * QEMU Context: QEMU Thread (backoff_timer_signal_handler)
 * This is called from our signal handler for the backoff timer so no calls
 * to non-async-signal-safe functions like printf etc. */
void
hs_csma_handle_backoff_timer_expires(void)
{
	/* This will call send and recv which are both async-signal-safe */
	clear_channel_assessment();
}

/* ____________________________________________________________ HAN_STATE_TX_PKT
 */

void
tx_timer_signal_handler(int sig, siginfo_t *si, void *uc)
{
	/* We are in a signal hander, no non-async-signal-safe calls */

	assert(hanadu_state_is_transmitting());
	han.state->handle_tx_timer_expires();
}

/* TODO: Get number of milliseconds that the current transmission would take. */
static int
getTimeOnWire(void)
{
	return 4;
}


static void
start_tx_timer(void)
{
	struct itimerspec ts;

	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 0;
	ts.it_value.tv_nsec = getTimeOnWire() * 1000000;

	assert(ts.it_value.tv_nsec > 0);
	timer_settime(han.mac.tx_timer, 0, &ts, NULL);
}

/*
 * QEMU Context: BOTH
 * QEMU thread as we will get called from backoff timer expires
 * which will send CCA req, receive confirm and if result is channel clear
 * change state to 'Tx Pkt' ie this state so we end up here in enter.
 * CPU Thread as we transition to Tx Pkt state from Idle if MAC is bypassed.
 * So no stalling the CPU thread which means we need to be careful about how
 * we wait for the NetSim to send the packet as it will wait for the nominal
 * time on the wire.
 */
void
hs_tx_pkt_enter(void)
{
	/* Send buffer to powerline network simulator */
	hanadu_tx_buffer_to_netsim();

	/* TODO: Ensure this gets cleared */
	han_trxm_tx_busy_set(han.trx_dev, true);

	/* Now we wait for tx done indication */
	start_tx_timer();
}

/*
 * QEMU Context: QEMU Thread (tx_timer_signal_handler)
 */
void
hs_tx_pkt_handle_tx_timer_expires(void)
{
	netsim_rx_tx_done_ind_msg();
	if (han.mac.ack_requested)
		hanadu_state_change(&han_state_wait_rx_ack);
	else
		hanadu_state_change(&han_state_ifs);
}


/* _______________________________________________________ HAN_STATE_WAIT_RX_ACK
 */

/* ____________________________________________________________ HAN_STATE_TX_ACK
 */

/* _______________________________________________________________ HAN_STATE_IFS
 */

void
ifs_timer_signal_handler(int sig, siginfo_t *si, void *uc)
{
	/* We are in a signal hander, no non-async-signal-safe calls */
	han.state->handle_ifs_timer_expires();
}

static void
start_ifs_timer(void)
{
	struct itimerspec ts;
	uint16_t psdu_len;
	uint8_t ifs_period;

	if (han_trxm_tx_mem_bank_select_get(han.trx_dev) == 0) {
		psdu_len = han_trxm_tx_psdu_len0_get(han.trx_dev);
	} else {
		psdu_len = han_trxm_tx_psdu_len1_get(han.trx_dev);
	}
	if (psdu_len <= han_mac_max_sifs_frame_size_get(han.mac_dev))
		ifs_period = han_mac_sifs_period_get(han.mac_dev);
	else
		ifs_period = han_mac_lifs_period_get(han.mac_dev);

	assert(ifs_period > 0);
	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 0;
	ts.it_value.tv_nsec = ifs_period * IFS_PERIOD_UNIT_NSECS;

	timer_settime(han.mac.ifs_timer, 0, &ts, NULL);
}

/*
 * QEMU Context: QEMU Thread (for Tx Pkt No AR)
 * Don't know about Wait Rx Ack yet??
 */
void
hs_ifs_enter(void)
{
	han_trxm_tx_busy_set(han.trx_dev, false);
	han.mac.tx_started = false;

	/* Even though we have 2 paths into IFS state we can generate Tx
	 * interrupt here to catch both cases. */
	qemu_irq_pulse(han.trx_dev->tx_irq);

	start_ifs_timer();
}

/*
 * QEMU Context: CPU thread (han_trxm_mem_region_write)
 */
void
hs_ifs_handle_start_tx(void)
{
	han.mac.start_tx_latched = true;
}

/*
 * QEMU Context: QEMU Thread (ifs_timer_signal_handler)
 */
void
hs_ifs_handle_ifs_timer_expires(void)
{
	hanadu_state_change(&han_state_idle);
}
