/*
 * QEMU model of Xsilon Hanadu 802.15.4 transceiver.
 *
 * Copyright (c) 2014 Martin Townsend <martin.townsend@xsilon.com>
 *
 */
#include "hw/sysbus.h"
#include "qemu/log.h"
#include "qemu/fifo.h"
#include "hanadu.h"
#include "hanadu-inl.h"
#include "xsilon.h"
#include "x6losim_interface.h"

#include <assert.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define NETSIM_DEFAULT_PORT					 (12304)
#define NETSIM_DEFAULT_ADDR					 (INADDR_LOOPBACK)
#define NETSIM_PKT_LEN						  (256)


enum han_state {
	HAN_STATE_IDLE = 0,
	HAN_STATE_RX,
	HAN_STATE_TX_CSMA,
	HAN_STATE_TX_BACKOFF,
	HAN_STATE_TX,
};

struct txbuf {
	uint8_t data[128];
};
struct rxbuf {
	uint8_t data[128];
};

static struct hanadu {
	uint8_t dip_board;
	uint8_t dip_afe;

	enum han_state state;

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
	} netsim;
	struct han_rx {
		struct rxbuf buf[4];
		unsigned bufs_avail_bitmap;
		Fifo nextbuf;
		Fifo proc;
		bool enabled;
	} rx;
	struct han_tx {
		struct txbuf buf[2];
		bool enabled;
	} tx;
	struct han_mac_dev *mac_dev;
	uint8_t mac_addr[8];
} han;


void
hanadu_set_default_dip_board(uint8_t dip)
{
	han.dip_board = dip;
}

void
hanadu_set_default_dip_afe(uint8_t dip)
{
	han.dip_afe = dip;
}

void
hanadu_set_netsim_addr(const char * addr)
{
	han.netsim.addr = strdup(addr);
}

void
hanadu_set_netsim_port(int port)
{
	han.netsim.port = port;
}

static void
hanadu_tx_buffer_to_netsim(struct han_trxm_dev *s, uint8_t * txbuf,
						   uint16_t len, uint8_t repcode)
{
	/* Send data to netsim */
	if(han.netsim.initialised) {
		uint8_t * netsim_pkt = (uint8_t *)malloc(NETSIM_PKT_LEN);
		struct netsim_pkt_hdr * hdr = (struct netsim_pkt_hdr *)netsim_pkt;
		uint8_t * data = netsim_pkt + NETSIM_PKT_HDR_SZ;

		memset(hdr, 0, NETSIM_PKT_HDR_SZ);
		memcpy(hdr->source_addr, han.mac_addr, sizeof(hdr->source_addr));
		hdr->psdu_len = len;
		hdr->rep_code = repcode;
		/* @todo use this fields */
		hdr->cca_mode = 0;
		hdr->tx_power = 0;
		memcpy(data, txbuf, len);
		sendto(han.netsim.sockfd,netsim_pkt, NETSIM_PKT_LEN, 0,
			   (struct sockaddr *)&han.netsim.server_addr,
			   sizeof(han.netsim.server_addr));
		free(netsim_pkt);
	}
	/* @todo set timer for receiving confirmation of packet successfully sent */
	qemu_set_irq(s->tx_irq, 1);
	s->tx_irq_state = 1;
}

static void
hanadu_rx_buffer_from_netsim(struct han_trxm_dev *s, struct netsim_pkt_hdr *rx_pkt)
{
	int i;
	uint8_t fifo_used;
	uint8_t *rxbuf;
	uint8_t * data;

	/* first up get next available buffer */
	i=0;
	while(!(han.rx.bufs_avail_bitmap & (1<<i)) && i < 4)
		i++;
	if(i == 4) {
		/* Overflow */
		han_trxm_rx_mem_bank_overflow_set(s, true);
	} else {
		/* Clear the buffer from the bitmap as we are going to use it. */
		han.rx.bufs_avail_bitmap &= ~(1<<i);
		han_trxm_rx_mem_bank_overflow_set(s, false);
		assert(i>=0 && i<4);
		assert(!fifo_is_full(&han.rx.nextbuf));
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
		fifo_push8(&han.rx.nextbuf, i);
		fifo_used = (uint8_t)fifo_num_used(&han.rx.nextbuf);
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
	if(!s->tx_irq_state) {
		qemu_set_irq(s->rx_irq, 1);
		s->rx_irq_state = 1;
	}
}

static void *
netsim_rxthread(void *arg)
{
	struct ip_mreq mreq;
	int rv;
	uint8_t *rxbuf;
	struct han_trxm_dev *s = arg;
	int optval;

	han.netsim.rxmcast.sockfd = socket(AF_INET,SOCK_DGRAM,0);
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
	han.netsim.rxmcast.addr.sin_port=htons(22411);
	rv = bind(han.netsim.rxmcast.sockfd,
			  (struct sockaddr *)&han.netsim.rxmcast.addr,
			  sizeof(han.netsim.rxmcast.addr));
	if (rv) {
		perror("Error binding rx multicast socket");
		close(han.netsim.rxmcast.sockfd);
		exit(EXIT_FAILURE);
	}
	mreq.imr_multiaddr.s_addr = inet_addr("224.1.1.1");
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
		struct sockaddr_in cliaddr;
		socklen_t len;
		int n;
		struct netsim_pkt_hdr * hdr;

		len = sizeof(cliaddr);
		n = recvfrom(han.netsim.rxmcast.sockfd, rxbuf, 256, 0 /*MSG_DONTWAIT*/,
					 (struct sockaddr *)&cliaddr, &len);

		if (n == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
		} else if (n == 0) {
			continue;
		}

		hdr = (struct netsim_pkt_hdr *)rxbuf;
		/* First check to see if it's the packet we last sent, ie from us. */
		if(memcmp(han.mac_addr, hdr->source_addr, sizeof(han.mac_addr)) == 0) {
			/* Assert Tx Done interrupt if packet has been sent or CSMA fails or if Ack
			 * requested the max retries has exceeded. */
//			qemu_set_irq(s->tx_irq, 1);
//			s->tx_irq_state = 1;
		} else {
			hanadu_rx_buffer_from_netsim(s, hdr);
		}
	}
	free(rxbuf);
	close(han.netsim.rxmcast.sockfd);
	pthread_exit(NULL);
}

static void
netsim_init(struct han_trxm_dev *s)
{
	int rv;

	han.netsim.sockfd=socket(AF_INET,SOCK_DGRAM,0);
	if (han.netsim.sockfd == -1) {
		perror("Failed to create netsim socket");
		exit(EXIT_FAILURE);
	}

	bzero(&han.netsim.server_addr,sizeof(han.netsim.server_addr));
	han.netsim.server_addr.sin_family = AF_INET;
	if (han.netsim.addr)
		han.netsim.server_addr.sin_addr.s_addr=inet_addr(han.netsim.addr);
	else
		han.netsim.server_addr.sin_addr.s_addr=htonl(NETSIM_DEFAULT_ADDR);
	if(han.netsim.port)
		han.netsim.server_addr.sin_port=htons(han.netsim.port);
	else
		han.netsim.server_addr.sin_port=htons(NETSIM_DEFAULT_PORT);
	han.netsim.initialised = true;

	rv = pthread_create(&han.netsim.rxmcast.threadinfo, NULL, netsim_rxthread, s);
	if(rv)
		exit(EXIT_FAILURE);
}


/* __________________________________________________________ Hanadu Transceiver
 */

static void
han_trxm_tx_enable_changed(uint32_t value, void *hw_block)
{
	if(value) {
		assert(han.tx.enabled == false);
		han.tx.enabled = true;
	} else {
		assert(han.tx.enabled == true);
		han.tx.enabled = false;
	}
}

static void
han_trxm_rx_enable_changed(uint32_t value, void *hw_block)
{
	if(value) {
		assert(han.rx.enabled == false);
		han.rx.enabled = true;
	} else {
		assert(han.rx.enabled == true);
		han.rx.enabled = false;
	}
}

static void
han_trxm_tx_start_changed(uint32_t value, void *hw_block)
{
	struct han_trxm_dev *s = HANADU_TRXM_DEV(hw_block);
	uint16_t psdu_len;
	uint8_t rep_code;
	uint8_t * buf;

	if(value) {
		/* Start transmission */
		if(han_trxm_tx_mem_bank_select_get(s) == 0) {
			psdu_len = han_trxm_tx_psdu_len0_get(hw_block);
			rep_code = han_trxm_tx_rep_code0_get(hw_block);
			buf = han.tx.buf[0].data;
		} else {
			psdu_len = han_trxm_tx_psdu_len1_get(hw_block);
			rep_code = han_trxm_tx_rep_code1_get(hw_block);
			buf = han.tx.buf[1].data;
		}
		/* Send buffer to powerline network simulator */
		hanadu_tx_buffer_to_netsim(s, buf, psdu_len, rep_code);
	} else {
		/* Tx finished, we use this to de-assert the interrupt line */
		if(s->tx_irq_state) {
			qemu_set_irq(s->tx_irq, 0);
			s->tx_irq_state = 0;
		}
	}
}

static int
han_trxm_rx_next_membank_to_proc_read(uint32_t *value_out, void *hw_block)
{
	struct han_trxm_dev *s = HANADU_TRXM_DEV(hw_block);
	uint8_t next_membank, fifo_used;

	assert(!fifo_is_empty(&han.rx.nextbuf));
	next_membank = fifo_pop8(&han.rx.nextbuf);
	han_trxm_rx_mem_bank_next_to_process_set(s, next_membank);
	fifo_push8(&han.rx.proc, next_membank);

	/* The buffer would be moved from nextbuf FIFO to proc FIFO */
	fifo_used = (uint8_t)fifo_num_used(&han.rx.nextbuf);
	han_trxm_rx_nextbuf_fifo_wr_level_set(s, fifo_used);
	han_trxm_rx_nextbuf_fifo_rd_level_set(s, fifo_used);
	fifo_used = (uint8_t)fifo_num_used(&han.rx.proc);
	han_trxm_rx_proc_fifo_wr_level_set(s, fifo_used);
	han_trxm_rx_proc_fifo_rd_level_set(s, fifo_used);

	/* We aren't going to terminate the read so return 0 so it's handled by
	 * the caller which will go ahead and return the next bank bank to process */
	return 0;
}

static void
han_trxm_rx_clear_membank_full_changed(uint32_t value, void *hw_block, uint8_t membank)
{
	struct han_trxm_dev *s = HANADU_TRXM_DEV(hw_block);
	/* Should be called twice, as the bit has to be pulsed by the driver */

	if(value) {
		uint8_t membank_processed, fifo_used;

		/* when high adjust fifo levels. */
		assert(!fifo_is_empty(&han.rx.proc));
		membank_processed = fifo_pop8(&han.rx.proc);
		assert(membank == membank_processed);
		fifo_used = (uint8_t)fifo_num_used(&han.rx.proc);
		han_trxm_rx_proc_fifo_wr_level_set(s, fifo_used);
		han_trxm_rx_proc_fifo_rd_level_set(s, fifo_used);

	} else {
		/* when low deassert irq if still set */
		assert((han.rx.bufs_avail_bitmap & (1 << membank)) == 0);
		han.rx.bufs_avail_bitmap |= 1 << membank;
		if(s->rx_irq_state) {
			qemu_set_irq(s->rx_irq, 0);
			s->rx_irq_state = 0;
		}
	}
}

static void
han_trxm_rx_clear_membank_full0_changed(uint32_t value, void *hw_block)
{
	struct han_trxm_dev *s = HANADU_TRXM_DEV(hw_block);

	han_trxm_rx_clear_membank_full_changed(value, hw_block, 0);
	/* Clear the actual flag in the membank status register */
	han_trxm_rx_mem_bank_full0_flag_set(s, 0);
}
static void
han_trxm_rx_clear_membank_full1_changed(uint32_t value, void *hw_block)
{
	struct han_trxm_dev *s = HANADU_TRXM_DEV(hw_block);

	han_trxm_rx_clear_membank_full_changed(value, hw_block, 1);
	/* Clear the actual flag in the membank status register */
	han_trxm_rx_mem_bank_full1_flag_set(s, 0);
}
static void
han_trxm_rx_clear_membank_full2_changed(uint32_t value, void *hw_block)
{
	struct han_trxm_dev *s = HANADU_TRXM_DEV(hw_block);

	han_trxm_rx_clear_membank_full_changed(value, hw_block, 2);
	/* Clear the actual flag in the membank status register */
	han_trxm_rx_mem_bank_full2_flag_set(s, 0);
}
static void
han_trxm_rx_clear_membank_full3_changed(uint32_t value, void *hw_block)
{
	struct han_trxm_dev *s = HANADU_TRXM_DEV(hw_block);

	han_trxm_rx_clear_membank_full_changed(value, hw_block, 3);
	/* Clear the actual flag in the membank status register */
	han_trxm_rx_mem_bank_full3_flag_set(s, 0);
}

static void
han_trxm_rx_clear_membank_oflow_changed(uint32_t value, void *hw_block)
{
	struct han_trxm_dev *s = HANADU_TRXM_DEV(hw_block);

	if(!value) {
		/* Clear the actual flag in the membank status register */
		han_trxm_rx_mem_bank_overflow_set(s, 0);
		if(s->rx_irq_state) {
			qemu_set_irq(s->rx_irq, 0);
			s->rx_irq_state = 0;
		}
	}
}

static Property han_trxm_properties[] = {
	DEFINE_PROP_UINT32("rxmem", struct han_trxm_dev, c_rxmem, 0x10000),
	DEFINE_PROP_UINT32("txmem", struct han_trxm_dev, c_txmem, 0x10000),
//	DEFINE_NIC_PROPERTIES(XilinxAXIEnet, conf),
	DEFINE_PROP_END_OF_LIST(),
};

static const MemoryRegionOps han_trxm_mem_region_ops = {
	.read = han_trxm_mem_region_read,
	.write = han_trxm_mem_region_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
};


static void han_trxm_realize(DeviceState *dev, Error **errp)
{
//	struct han_trxm_dev *s = HANADU_TRXM_DEV(dev);
//	Error *local_err = NULL;

	return;

//realize_fail:
//	if (!*errp) {
//		*errp = local_err;
//	}
}

static void han_trxm_reset(DeviceState *dev)
{
//	struct han_trxm_dev *s = HANADU_TRXM_DEV(dev);
}


static void han_trxm_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = han_trxm_realize;
	dc->props = han_trxm_properties;
	dc->reset = han_trxm_reset;
}

static void han_trxm_instance_init(Object *obj)
{
	struct han_trxm_dev *s = HANADU_TRXM_DEV(obj);
	SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

	memory_region_init_io(&s->iomem, OBJECT(s), &han_trxm_mem_region_ops, s,
						  "hantrxm", 0x10000);
	sysbus_init_mmio(sbd, &s->iomem);

	sysbus_init_irq(sbd, &s->rx_irq);
	sysbus_init_irq(sbd, &s->rx_fail_irq);
	sysbus_init_irq(sbd, &s->tx_irq);

	s->mem_region_write = NULL;

	/* Override Register/Bitfield changed functions */
	s->regs.reg_read.trx_rx_next_membank_to_proc_read_cb = han_trxm_rx_next_membank_to_proc_read;
	s->regs.field_changed.tx_enable_changed = han_trxm_tx_enable_changed;
	s->regs.field_changed.rx_enable_changed = han_trxm_rx_enable_changed;
	s->regs.field_changed.tx_start_changed = han_trxm_tx_start_changed;
	s->regs.field_changed.rx_clear_membank_full0_changed = han_trxm_rx_clear_membank_full0_changed;
	s->regs.field_changed.rx_clear_membank_full1_changed = han_trxm_rx_clear_membank_full1_changed;
	s->regs.field_changed.rx_clear_membank_full2_changed = han_trxm_rx_clear_membank_full2_changed;
	s->regs.field_changed.rx_clear_membank_full3_changed = han_trxm_rx_clear_membank_full3_changed;
	s->regs.field_changed.rx_clear_membank_oflow_changed = han_trxm_rx_clear_membank_oflow_changed;

	/* Setup callbacks to capture filter reg changes */
	netsim_init(s);
}

/* __________________________________________________________________ Hanadu AFE
 */

static const MemoryRegionOps han_afe_mem_region_ops = {
	.read = han_afe_mem_region_read,
	.write = han_afe_mem_region_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
};

static void han_afe_realize(DeviceState *dev, Error **errp)
{
//	struct han_afe_dev *s = HANADU_AFE_DEV(dev);
//	Error *local_err = NULL;

	return;

//realize_fail:
//	if (!*errp) {
//		*errp = local_err;
//	}
}

static void han_afe_reset(DeviceState *dev)
{
//	struct han_afe_dev *s = HANADU_AFE_DEV(dev);
}


static void han_afe_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = han_afe_realize;
	dc->props = NULL;
	dc->reset = han_afe_reset;
}

static void han_afe_instance_init(Object *obj)
{
	struct han_afe_dev *s = HANADU_AFE_DEV(obj);
	SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

	memory_region_init_io(&s->iomem, OBJECT(s), &han_afe_mem_region_ops, s,
						  "hanafe", 0x10000);
	sysbus_init_mmio(sbd, &s->iomem);
	s->mem_region_write = NULL;
}

/* __________________________________________________________________ Hanadu PWR
 */

static const MemoryRegionOps han_pwr_mem_region_ops = {
	.read = han_pwr_mem_region_read,
	.write = han_pwr_mem_region_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
};

static void han_pwr_realize(DeviceState *dev, Error **errp)
{
//	struct han_pwr_dev *s = HANADU_PWR_DEV(dev);
//	Error *local_err = NULL;

	return;

//realize_fail:
//	if (!*errp) {
//		*errp = local_err;
//	}
}

static void han_pwr_reset(DeviceState *dev)
{
	struct han_pwr_dev *s = HANADU_PWR_DEV(dev);

	s->regs.pup_dips1 = han.dip_board;
	s->regs.pup_dips2 = han.dip_afe;
}


static void han_pwr_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = han_pwr_realize;
	dc->props = NULL;
	dc->reset = han_pwr_reset;
}

static void han_pwr_instance_init(Object *obj)
{
	struct han_pwr_dev *s = HANADU_PWR_DEV(obj);
	SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

	memory_region_init_io(&s->iomem, OBJECT(s), &han_pwr_mem_region_ops, s,
						  "hanpwr", 0x10000);
	sysbus_init_mmio(sbd, &s->iomem);
	s->mem_region_write = NULL;
}

/* __________________________________________________________________ Hanadu MAC
 */

static int
han_mac_filter_ea_upper_changed(uint32_t value)
{
	han.mac_addr[0] = (uint8_t)((value >> 24) & 0xff);
	han.mac_addr[1] = (uint8_t)((value >> 16) & 0xff);
	han.mac_addr[2] = (uint8_t)((value >> 8) & 0xff);
	han.mac_addr[3] = (uint8_t)(value & 0xff);

	return 0;
}

static int
han_mac_filter_ea_lower_changed(uint32_t value)
{
	han.mac_addr[4] = (uint8_t)((value >> 24) & 0xff);
	han.mac_addr[5] = (uint8_t)((value >> 16) & 0xff);
	han.mac_addr[6] = (uint8_t)((value >> 8) & 0xff);
	han.mac_addr[7] = (uint8_t)(value & 0xff);

	return 0;
}

static const MemoryRegionOps han_mac_mem_region_ops = {
	.read = han_mac_mem_region_read,
	.write = han_mac_mem_region_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
};

static void han_mac_realize(DeviceState *dev, Error **errp)
{
//	struct han_mac_dev *s = HANADU_MAC_DEV(dev);
//	Error *local_err = NULL;

	return;

//realize_fail:
//	if (!*errp) {
//		*errp = local_err;
//	}
}

static void han_mac_reset(DeviceState *dev)
{
//	struct han_mac_dev *s = HANADU_MAC_DEV(dev);
}


static void han_mac_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = han_mac_realize;
	dc->props = NULL;
	dc->reset = han_mac_reset;
}

static void han_mac_instance_init(Object *obj)
{
	struct han_mac_dev *s = HANADU_MAC_DEV(obj);
	SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

	memory_region_init_io(&s->iomem, OBJECT(s), &han_mac_mem_region_ops, s,
						  "hanmac", 0x10000);
	sysbus_init_mmio(sbd, &s->iomem);
	s->mem_region_write = NULL;

	han.mac_dev = s;
	s->regs.reg_changed.mac_filter_ea_upper_changed_cb = han_mac_filter_ea_upper_changed;
	s->regs.reg_changed.mac_filter_ea_lower_changed_cb = han_mac_filter_ea_lower_changed;
}

/* __________________________________________________________________ Hanadu TXB
 */

static uint64_t
han_txb_mem_region_read(void *opaque, hwaddr addr, unsigned size)
{
	uint8_t * buf;

	assert(size == 1);
	if(addr < 0x1000) {
		addr >>= 2;
		buf = han.tx.buf[0].data + addr;
	} else {
		addr -= 0x1000;
		addr >>= 2;
		buf = han.tx.buf[1].data + addr;
	}
	assert(addr <= 128);

	return (uint64_t)*buf;
}

static void
han_txb_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
	uint8_t * buf;

	assert(size == 1);
	if(addr < 0x1000) {
		addr >>= 2;
		buf = han.tx.buf[0].data + addr;
	} else {
		addr -= 0x1000;
		addr >>= 2;
		buf = han.tx.buf[1].data + addr;
	}
	assert(addr <= 128);

	*buf = (uint8_t)value;
}

static const MemoryRegionOps han_txb_mem_region_ops = {
	.read = han_txb_mem_region_read,
	.write = han_txb_mem_region_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
};

static void han_txb_realize(DeviceState *dev, Error **errp)
{
//	struct han_txb_dev *s = HANADU_TXB_DEV(dev);
//	Error *local_err = NULL;

	return;

//realize_fail:
//	if (!*errp) {
//		*errp = local_err;
//	}
}

static void han_txb_reset(DeviceState *dev)
{
//	struct han_txb_dev *s = HANADU_TXB_DEV(dev);
}


static void han_txb_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = han_txb_realize;
	dc->props = NULL;
	dc->reset = han_txb_reset;
}

static void han_txb_instance_init(Object *obj)
{
	struct han_txb_dev *s = HANADU_TXB_DEV(obj);
	SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

	memory_region_init_io(&s->iomem, OBJECT(s), &han_txb_mem_region_ops, s,
						  "hantxb", 0x10000);
	sysbus_init_mmio(sbd, &s->iomem);
}

/* __________________________________________________________________ Hanadu RXB
 */

static uint64_t
han_rxb_mem_region_read(void *opaque, hwaddr addr, unsigned size)
{
	uint8_t * buf;

	assert(size == 1);
	if(addr < 0x1000) {
		addr >>= 2;
		buf = han.rx.buf[0].data + addr;
	} else if(addr < 0x2000) {
		addr -= 0x1000;
		addr >>= 2;
		buf = han.rx.buf[1].data + addr;
	} else if(addr < 0x3000) {
		addr -= 0x2000;
		addr >>= 2;
		buf = han.rx.buf[2].data + addr;
	} else if(addr < 0x4000) {
		addr -= 0x3000;
		addr >>= 2;
		buf = han.rx.buf[3].data + addr;
	} else {
		assert(addr >= 0x4000);
	}
	assert(addr <= 128);

	return (uint64_t)*buf;
}

static void
han_rxb_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
	/* Shouldn't be writing to receive memories from the driver */
	assert(1==0);
}

static const MemoryRegionOps han_rxb_mem_region_ops = {
	.read = han_rxb_mem_region_read,
	.write = han_rxb_mem_region_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
};

static void han_rxb_realize(DeviceState *dev, Error **errp)
{
//	struct han_rxb_dev *s = HANADU_RXB_DEV(dev);
//	Error *local_err = NULL;

	return;

//realize_fail:
//	if (!*errp) {
//		*errp = local_err;
//	}
}

static void han_rxb_reset(DeviceState *dev)
{
//	struct han_rxb_dev *s = HANADU_RXB_DEV(dev);
}


static void han_rxb_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = han_rxb_realize;
	dc->props = NULL;
	dc->reset = han_rxb_reset;
}

static void han_rxb_instance_init(Object *obj)
{
	struct han_rxb_dev *s = HANADU_RXB_DEV(obj);
	SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

	memory_region_init_io(&s->iomem, OBJECT(s), &han_rxb_mem_region_ops, s,
						  "hanrxb", 0x10000);
	sysbus_init_mmio(sbd, &s->iomem);
}

/* __________________________________________________________________ Hanadu HWV
 */

/* Override the auto generated functions */
uint64_t
han_hwvers_mem_region_read(void *opaque, hwaddr addr, unsigned size)
{
	addr >>= 2;

	switch (addr) {
	case 0:
	case 1:
	case 2:
		return 9999;
	case 3:
		return 1;
	}

	return 0;
}

void
han_hwvers_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
	/* This is a read only register block */
}

static const MemoryRegionOps han_hwvers_mem_region_ops = {
	.read = han_hwvers_mem_region_read,
	.write = han_hwvers_mem_region_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
};

static void han_hwvers_realize(DeviceState *dev, Error **errp)
{
//	struct han_hwv_dev *s = HANADU_HWV_DEV(dev);
//	Error *local_err = NULL;

	return;

//realize_fail:
//	if (!*errp) {
//		*errp = local_err;
//	}
}

static void han_hwvers_reset(DeviceState *dev)
{
//	struct han_hwv_dev *s = HANADU_HWV_DEV(dev);
}


static void han_hwvers_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = han_hwvers_realize;
	dc->props = NULL;
	dc->reset = han_hwvers_reset;
}

static void han_hwvers_instance_init(Object *obj)
{
	struct han_hwvers_dev *s = HANADU_HWVERS_DEV(obj);
	SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

	memory_region_init_io(&s->iomem, OBJECT(s), &han_hwvers_mem_region_ops, s,
						  "hanhwv", 0x10000);
	sysbus_init_mmio(sbd, &s->iomem);
	s->mem_region_write = NULL;
}

/* _________________________________________________________ Hanadu Registration
 */

static const TypeInfo han_trxm_info = {
	.name  = TYPE_HANADU_TRXM,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size  = sizeof(struct han_trxm_dev),
	.class_init = han_trxm_class_init,
	.instance_init = han_trxm_instance_init,
};

static const TypeInfo han_afe_info = {
	.name  = TYPE_HANADU_AFE,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size  = sizeof(struct han_afe_dev),
	.class_init = han_afe_class_init,
	.instance_init = han_afe_instance_init,
};

static const TypeInfo han_pwr_info = {
	.name  = TYPE_HANADU_PWR,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size  = sizeof(struct han_pwr_dev),
	.class_init = han_pwr_class_init,
	.instance_init = han_pwr_instance_init,
};

static const TypeInfo han_mac_info = {
	.name  = TYPE_HANADU_MAC,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size  = sizeof(struct han_mac_dev),
	.class_init = han_mac_class_init,
	.instance_init = han_mac_instance_init,
};

static const TypeInfo han_txb_info = {
	.name  = TYPE_HANADU_TXB,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size  = sizeof(struct han_txb_dev),
	.class_init = han_txb_class_init,
	.instance_init = han_txb_instance_init,
};

static const TypeInfo han_rxb_info = {
	.name  = TYPE_HANADU_RXB,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size  = sizeof(struct han_rxb_dev),
	.class_init = han_rxb_class_init,
	.instance_init = han_rxb_instance_init,
};

static const TypeInfo han_hwv_info = {
	.name  = TYPE_HANADU_HWVERS,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size  = sizeof(struct han_hwvers_dev),
	.class_init = han_hwvers_class_init,
	.instance_init = han_hwvers_instance_init,
};

static void han_register_types(void)
{
	fifo_create8(&han.rx.nextbuf, 4);
	fifo_create8(&han.rx.proc, 4);
	han.rx.bufs_avail_bitmap = 0x0f;

	type_register_static(&han_trxm_info);
	type_register_static(&han_afe_info);
	type_register_static(&han_pwr_info);
	type_register_static(&han_mac_info);
	type_register_static(&han_txb_info);
	type_register_static(&han_rxb_info);
	type_register_static(&han_hwv_info);
}

type_init(han_register_types)


