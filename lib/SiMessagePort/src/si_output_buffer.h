#pragma once

#include <stdint.h>

#include "si_circular_data.h"

#define SI_OUTPUT_BUFFER_MODE2_HEADER_SIZE 2

enum SiOutputBufferMode {
	SI_OUTPUT_BUFFER_MODE1, // Each message has a unsigned int (4 bytes) proceeding it. No CRC checking
	SI_OUTPUT_BUFFER_MODE2, // Each message has a unique starting byte and a 1 byte len (max payload is 254 bytes)
    SI_OUTPUT_BUFFER_MODE3  // Each message starts with 0x00, and ends with 0xFF
};

#ifndef ARDUINO_ARCH_AVR
struct SiOutputBufferMessage {
	uint8_t* data;
	uint32_t len;
	uint32_t pos;
};
#endif

struct SiOutputBuffer {
	enum SiOutputBufferMode mode;

	// Specific for mode 2 and 3
	struct SiCircularData* circular_data_buffer;
};

void si_output_buffer_mode2_init(struct SiOutputBuffer* output_buffer, struct SiCircularData* circulair_data_buffer);
enum SiResult si_output_buffer_mode2_push_data(const struct SiOutputBuffer* output_buffer, uint8_t* data, uint8_t len);
uint8_t si_output_buffer_mode2_peek_data(struct SiOutputBuffer* output_buffer, uint8_t* data, uint8_t len);
enum SiResult si_output_buffer_mode2_forward_data(struct SiOutputBuffer* output_buffer, uint8_t len);

void si_output_buffer_flush(struct SiOutputBuffer* output_buffer);
void si_output_buffer_close(struct SiOutputBuffer* output_buffer);
