#ifndef _INC_HANADU_DEFS_H
#define _INC_HANADU_DEFS_H

#define IEEE802154_FC0_AR_BIT				(0x20)
/* An IFS period is 6.5625 microseconds */
#define IFS_PERIOD_UNIT_NSECS				(6562)

enum tso_enum {
	TOS_DISCARD_TX = 0,
	TOS_TX_IMMEDIATE = 1,
	TOS_CCA_WAIT_THEN_TX = 2
};

#endif
