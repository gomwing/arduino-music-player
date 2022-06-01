#ifndef pgmspace_h
#define pgmspace_h

#define __ATTR_PROGMEM__
#define PROGMEM __ATTR_PROGMEM__
typedef unsigned char uint8_t;
static inline char pgm_read_byte(const uint8_t *addr)
//static inline char pgm_read_byte(size_t addr)
{
	char* ptr = (char*)addr;
	return *ptr;	// gomwing modefied 2022.6.1
}

#define memcpy_P	memcpy

#endif