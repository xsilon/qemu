
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
#include <byteswap.h>

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


#define EVENT_PIPE_FD_READ		(0)
#define EVENT_PIPE_FD_WRITE		(1)
struct han_event
{
	int pipe_fds[2];
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
	pthread_t sm_threadinfo;
	struct han_state * state;
	struct han_event txstart_event;
	struct han_event sysinfo_available;

	struct netsim {
		struct {
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
		int backoff_timer;
		/* Timer for transmissions.  NetSim will wait for the time it
		 * would take for the packet to be transmitted before sending
		 * it out on the multicast channel and sending the tx done ind. */
		int tx_timer;
		/* Interframe Spacing timer */
		int ifs_timer;
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
/* TODO: return ssize_t */
void
netsim_tx_data_ind(void);

void *
netsim_rx_read_msg(void);
void
netsim_rx_reg_req_msg(struct netsim_to_node_registration_req_pkt *req_req);
int
netsim_rx_cca_con_msg(struct netsim_to_node_cca_con_pkt * cca_con);
int
netsim_rx_tx_done_ind_msg(struct netsim_to_node_tx_done_ind_pkt *tx_done_ind);
struct netsim_data_ind_pkt *
netsim_rx_tx_data_ind_msg(struct netsim_data_ind_pkt *data_ind);
void
netsim_rxmcast_init(void);



/* ________________________________________________________ Hanadu State Machine
 */


int
han_event_init(struct han_event * event);
void
han_event_signal(struct han_event * event);


void *
hanadu_modem_model_thread(void *arg);

#endif
