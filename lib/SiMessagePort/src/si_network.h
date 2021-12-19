#pragma once

#include "si_base.h"

// We expect all embedded systems to be little endian
#define htons(x) ( ((x)<< 8 & 0xFF00) | \
                   ((x)>> 8 & 0x00FF) )
#define ntohs(x) htons(x)

#define htonl(x) ( ((x)<<24 & 0xFF000000UL) | \
                   ((x)<< 8 & 0x00FF0000UL) | \
                   ((x)>> 8 & 0x0000FF00UL) | \
                   ((x)>>24 & 0x000000FFUL) )
#define ntohl(x) htonl(x)


int16_t si_network_int16_to_network(int16_t value);
int16_t si_network_int16_to_host(int16_t value);

int32_t si_network_int32_to_network(int32_t value);
int32_t si_network_int32_to_host(int32_t value);

double si_network_double_to_network(double value);
double si_network_double_to_host(double value);

float si_network_float(uint8_t* x);
float si_network_swap_float(float value);

#define SI_NETWORK_CHAR_TO_HOST(network_value) (network_value)
#define SI_NETWORK_CHAR_TO_NETWORK(host_value)  (host_value)

#define SI_NETWORK_UINT16_TO_HOST(network_value) (uint16_t) ntohs(network_value)
#define SI_NETWORK_UINT16_TO_NETWORK(host_value) (uint16_t) htons(host_value)

#define SI_NETWORK_UINT32_TO_HOST(network_value) (uint32_t) ntohl(network_value)
#define SI_NETWORK_UINT32_TO_NETWORK(host_value) (uint32_t) htonl(host_value)

#define SI_NETWORK_INT16_TO_HOST(network_value) ( si_network_int16_to_host(network_value) )
#define SI_NETWORK_INT16_TO_NETWORK(host_value) ( si_network_int16_to_network(host_value) )

#define SI_NETWORK_INT32_TO_HOST(network_value) ( si_network_int32_to_host(network_value) )
#define SI_NETWORK_INT32_TO_NETWORK(host_value) ( si_network_int32_to_network(host_value) )

#define SI_NETWORK_DOUBLE_TO_HOST(network_value)  ( si_network_double_to_host(network_value)  )
#define SI_NETWORK_DOUBLE_TO_NETWORK(host_value)  ( si_network_double_to_network(host_value)  )
