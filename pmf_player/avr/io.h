#ifndef _AVR_IO_H_
#define _AVR_IO_H_

#ifdef _MSC_VER
#define __AVR_ATmega328P__
#include <stdint.h>

#define sei()
#define cli()

#pragma warning(disable:4068)

static char SREG;

//#define __ASSEMBLER__
__declspec( selectany ) char virtual_addr[65536];
#define _MMIO_BYTE(mem_addr) (*(volatile uint8_t *)(&virtual_addr[mem_addr]))
#define _MMIO_WORD(mem_addr) (*(volatile uint16_t *)(&virtual_addr[mem_addr]))
#define _MMIO_DWORD(mem_addr) (*(volatile uint32_t *)(&virtual_addr[mem_addr]))


#endif

#include <avr/sfr_defs.h>

#if defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328__)
#  include <avr/iom328p.h>
#endif

#ifndef _MSC_VER
	#include <avr/portpins.h>
	#include <avr/common.h>
	#include <avr/version.h>

	#if __AVR_ARCH__ >= 100
	#  include <avr/xmega.h>
	#endif

	/* Include fuse.h after individual IO header files. */
	#include <avr/fuse.h>

	/* Include lock.h after individual IO header files. */
	#include <avr/lock.h>
#endif

#endif /* _AVR_IO_H_ */
