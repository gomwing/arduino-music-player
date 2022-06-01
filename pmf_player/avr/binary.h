#define __attribute__(x) 

static inline int pgm_read_dword(const uint8_t *p) {
	int* ptr = (int*)p;
	return *ptr;
} 

static inline short pgm_read_word(const uint8_t* p) {
	short* ptr = (short*)p;
	return *ptr;
}
#define ARDUINO_ARCH_AVR
//if ((pgm_read_word(pmf_file + pmfcfg_offset_version) & 0xfff0) != pmf_file_version)
typedef unsigned char uint8_t;

//char inline pgm_read_byte(const char *p) {
//	return *p;
//}