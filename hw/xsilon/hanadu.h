
#ifndef _HANADU_QDEV_INC
#define _HANADU_QDEV_INC

#include "hw/sysbus.h"
#include "qom/object.h"
#include "qemu/fifo8.h"
#include "hanadu-defs-qdev.h"
#include "x6losim_interface.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>

#define BACKOFF_TIMER_SIG		(SIGRTMAX)
#define TX_TIMER_SIG			(SIGRTMAX-1)
#define IFS_TIMER_SIG			(SIGRTMAX-2)

#define TYPE_HANADU_TRXM "xlnx,transceiver-1.00.a"
#define TYPE_HANADU_AFE "xlnx,afe-controller-2.00.a"
#define TYPE_HANADU_PWR "xlnx,powerup-controller-1.00.a"
#define TYPE_HANADU_MAC "xlnx,traffic-monitor-1.00.a"
#define TYPE_HANADU_TXB "xlnx,tx-buffer-3.00.a"
#define TYPE_HANADU_RXB "xlnx,rx-buffer-3.00.a"
#define TYPE_HANADU_HWVERS "xlnx,hardware-version-control-2.00.a"
#define TYPE_HANADU_SYSINFO "xlnx,system-info-1.00.a"


#define HANADU_TRXM_DEV(obj) \
     OBJECT_CHECK(struct han_trxm_dev, (obj), TYPE_HANADU_TRXM)
#define HANADU_AFE_DEV(obj) \
     OBJECT_CHECK(struct han_afe_dev, (obj), TYPE_HANADU_AFE)
#define HANADU_PWR_DEV(obj) \
     OBJECT_CHECK(struct han_pwr_dev, (obj), TYPE_HANADU_PWR)
#define HANADU_MAC_DEV(obj) \
     OBJECT_CHECK(struct han_mac_dev, (obj), TYPE_HANADU_MAC)
#define HANADU_TXB_DEV(obj) \
     OBJECT_CHECK(struct han_txb_dev, (obj), TYPE_HANADU_TXB)
#define HANADU_RXB_DEV(obj) \
     OBJECT_CHECK(struct han_rxb_dev, (obj), TYPE_HANADU_RXB)
#define HANADU_HWVERS_DEV(obj) \
     OBJECT_CHECK(struct han_hwvers_dev, (obj), TYPE_HANADU_HWVERS)
#define HANADU_SYSINFO_DEV(obj) \
     OBJECT_CHECK(struct han_sysinfo_dev, (obj), TYPE_HANADU_SYSINFO)

typedef int (*mem_region_write_fnp)(void *opaque, hwaddr addr, uint64_t value, unsigned size);
typedef int (*mem_region_read_fnp)(void *opaque, hwaddr addr, unsigned size, uint64_t *value);

struct han_trxm_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
    mem_region_write_fnp mem_region_write;
    mem_region_read_fnp mem_region_read;
    qemu_irq rx_irq;
    qemu_irq rx_fail_irq;
    qemu_irq tx_irq;
    bool rx_irq_state;
    bool tx_irq_state;

    struct han_regmap_trxm regs;

    uint32_t c_rxmem;
    uint32_t c_txmem;
};

struct han_afe_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
    mem_region_write_fnp mem_region_write;
    mem_region_read_fnp mem_region_read;

    struct han_regmap_afe regs;
};

struct han_pwr_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
    mem_region_write_fnp mem_region_write;
    mem_region_read_fnp mem_region_read;

    struct han_regmap_pwr regs;
};

struct han_mac_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
    mem_region_write_fnp mem_region_write;
    mem_region_read_fnp mem_region_read;

    struct han_regmap_mac regs;
};

struct han_txb_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
};

struct han_rxb_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
};

struct han_hwvers_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
    mem_region_write_fnp mem_region_write;
    mem_region_read_fnp mem_region_read;

    struct han_regmap_hwvers regs;
};

struct han_sysinfo_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
};

struct system_info_reg {
	char os[32];
	char os_version[32];
	char nodename[64];
};

struct txbuf {
	uint8_t data[128];
};
struct rxbuf {
	uint8_t data[128];
};

struct hanadu {
	uint8_t dip_board;
	uint8_t dip_afe;

	/* State Machine Mutex */
	pthread_mutex_t sm_mutex;
	struct han_state * state;

	struct netsim {
		struct {
			pthread_t threadinfo;
			int sockfd;
			struct sockaddr_in addr;
		} rxmcast;
		int sockfd;
		struct sockaddr_in server_addr;
		struct sockaddr_in client_addr;
		char * addr;
		int port;
		bool initialised;
		uint64_t node_id;
	} netsim;
	struct han_rx {
		struct rxbuf buf[4];
		unsigned bufs_avail_bitmap;
		Fifo8 nextbuf;
		Fifo8 proc;
		bool enabled;
	} rx;
	struct han_tx {
		struct txbuf buf[2];
		bool enabled;
	} tx;
	struct han_mac {
		/* Flag to indicate that we have started a transmission. */
		bool tx_started;
		/* Set when TOS is tx when channel next idle */
		bool wait_free;
		/* For the current transmission is the AR bit set */
		bool ack_requested;
		/* Used if start_tx is set when in IFS */
		bool start_tx_latched;
		/* Number of times CSMA algorithm was required to back off for
		 * current transmission attempt */
		int nb;
		/* current backoff exponent */
		int be;

		/* Timer for random backoff for CSMA state. */
		timer_t backoff_timer;
		/* Timer for transmissions.  NetSim will wait for the time it
		 * would take for the packet to be transmitted before sending
		 * it out on the multicast channel and sending the tx done ind. */
		timer_t tx_timer;
		/* Interframe Spacing timer */
		timer_t ifs_timer;
	} mac;
	struct han_ad9865 {
		uint32_t regs[5];
	} ad9865;
	struct han_trxm_dev *trx_dev;
	struct han_mac_dev *mac_dev;
	struct han_pwr_dev *pwr_dev;
	struct han_afe_dev *afe_dev;
	struct system_info_reg sysinfo;
	uint8_t mac_addr[8];
};

extern struct hanadu han;

/* ________________________________________________________________ Hanadu Comms
 */

ssize_t
netsim_tx_reg_con(void);
ssize_t
netsim_tx_cca_req(void);

void
netsim_rx_reg_req_msg(void);
int
netsim_rx_cca_con_msg(void);
int
netsim_rx_tx_done_ind_msg(void);
void
netsim_rx_tx_data_ind_msg(struct netsim_data_ind_pkt *data_ind);

void
hanadu_tx_buffer_to_netsim(void);
void
hanadu_rx_buffer_from_netsim(struct han_trxm_dev *s, struct netsim_pkt_hdr *rx_pkt);
void *
netsim_rxthread(void *arg);

/* ________________________________________________________ Hanadu State Machine
 */

enum han_state_enum {
	HAN_STATE_IDLE = 0,
	HAN_STATE_RX_PKT,
	HAN_STATE_CSMA,
	HAN_STATE_TX_PKT,
	HAN_STATE_WAIT_RX_ACK,
	HAN_STATE_TX_ACK,
	HAN_STATE_IFS
};

typedef void (* han_state_enter)(void);
typedef void (* han_state_exit)(void);
typedef void (* han_state_handle_rx_pkt)(struct netsim_data_ind_pkt *data_ind);
typedef void (* han_state_handle_start_tx)(void);
typedef void (* han_state_handle_cca_con)(int);
typedef void (* han_state_handle_tx_done_ind)(void);
typedef void (* han_state_handle_backoff_timer_expires)(void);
typedef void (* han_state_handle_tx_timer_expires)(void);
typedef void (* han_state_handle_ifs_timer_expires)(void);

struct han_state {
	enum han_state_enum id;

	han_state_enter enter;
	han_state_exit exit;
	han_state_handle_rx_pkt handle_rx_pkt;
	han_state_handle_start_tx handle_start_tx;
	han_state_handle_cca_con handle_cca_con;
	han_state_handle_tx_done_ind handle_tx_done_ind;
	han_state_handle_backoff_timer_expires handle_backoff_timer_expires;
	han_state_handle_tx_timer_expires handle_tx_timer_expires;
	han_state_handle_ifs_timer_expires handle_ifs_timer_expires;
};

void
backoff_timer_signal_handler(int sig, siginfo_t *si, void *uc);
void
tx_timer_signal_handler(int sig, siginfo_t *si, void *uc);
void
ifs_timer_signal_handler(int sig, siginfo_t *si, void *uc);

bool
hanadu_state_is_listening(void);
bool
hanadu_state_is_transmitting(void);
void
hanadu_state_change(struct han_state *new_state);
struct han_state *
hanadu_state_idle_get(void);

#endif
