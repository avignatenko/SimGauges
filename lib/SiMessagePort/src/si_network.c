#include "si_network.h"

#include <string.h>

#define IS_BIG_ENDIAN (*(uint16_t *)"\0\xff" < 0x100)

int16_t si_network_int16_to_network(int16_t value) {
	uint16_t temp = htons(*((uint16_t*)&value));
	return *(int16_t*)&temp;
}

int16_t si_network_int16_to_host(int16_t value) {
	uint16_t temp = ntohs(*((uint16_t*)&value));
	return *(int16_t*)&temp;
}

int32_t si_network_int32_to_network(int32_t value) {
	uint32_t temp = htonl(*((uint32_t*)&value));
	return *(int32_t*)&temp;
}

int32_t si_network_int32_to_host(int32_t value) {
	uint32_t temp = ntohl(*((uint32_t*)&value));
	return *(int32_t*)&temp;
}

float si_network_float(uint8_t* x) {
    union {
        uint8_t  byte[4];
        uint32_t integer;
        float   floating;
    } ret;

    memcpy(ret.byte, x, 4);

#ifdef IS_BIG_ENDIAN
    ret.integer = ntohl(ret.integer);
#endif

    return ret.floating;
}

float si_network_swap_float(float value) {
    union v {
        float       f;
        uint32_t    i;
    };

    union v val;

    val.f = value;
    val.i = htonl(val.i);

    return val.f;
}
