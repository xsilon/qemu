#ifndef _XSILON_INC
#define _XSILON_INC

#include <stdint.h>

void
hanadu_set_default_dip_board(uint8_t dip);

void
hanadu_set_default_dip_afe(uint8_t dip);

void
hanadu_set_netsim_addr(const char * addr);

void
hanadu_set_netsim_port(int port);

#endif
