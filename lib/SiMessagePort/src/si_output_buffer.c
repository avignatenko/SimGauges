#include "si_output_buffer.h"
#include "si_network.h"

#include <string.h>

#define LEN_MODE_LEN_SIZE 4 // Number of bytes of the len prefix
#define MODE2_HEADER_SIZE 2 // Start tag and payload len
#define MODE3_OVERHEAD_SIZE 2 // Start tag and end tag

void si_output_buffer_mode2_init(struct SiOutputBuffer* output_buffer, struct SiCircularData* circulair_data_buffer) {
	output_buffer->mode = SI_OUTPUT_BUFFER_MODE2;
	output_buffer->circular_data_buffer = circulair_data_buffer;
}

enum SiResult si_output_buffer_mode2_push_data(const struct SiOutputBuffer* output_buffer, uint8_t* data, uint8_t len) {
	uint8_t required_message_len = 0;

	required_message_len += MODE2_HEADER_SIZE; // Start and len byte
	required_message_len += len; // bytes required for the data

	// Every data byte that is 0xFF should get another 0xFF
	for(uint8_t i = 0; i < len; i++) {
		if (data[i] == 0xFF) {
			required_message_len++;
		}
	}

	// Push actual bytes into circular buffer
	if (si_circular_data_free(output_buffer->circular_data_buffer) >= required_message_len) {
		si_circular_push(output_buffer->circular_data_buffer, 0xFF); // Start byte
		si_circular_push(output_buffer->circular_data_buffer, (required_message_len - MODE2_HEADER_SIZE)); // Len byte

		for (uint8_t i = 0; i < len; i++) {
			si_circular_push(output_buffer->circular_data_buffer, data[i]);

			// Push another 0xFF when the data is 0xFF
			if (data[i] == 0xFF) {
				si_circular_push(output_buffer->circular_data_buffer, 0xFF);
			}
		}

		return SI_OK;
	}

	return SI_ERROR;
}

uint8_t si_output_buffer_mode2_peek_data(struct SiOutputBuffer* output_buffer, uint8_t* data, uint8_t len) {
	uint8_t bytes_read = 0;

	for (uint8_t i = 0; i < len; i++) {
		if (si_circular_peek(output_buffer->circular_data_buffer, i, data + i) == SI_ERROR) {
			break;
		}
		bytes_read++;
	}

	return bytes_read;
}

enum SiResult si_output_buffer_mode2_forward_data(struct SiOutputBuffer* output_buffer, uint8_t len) {
	return si_circular_forward(output_buffer->circular_data_buffer, len);
}

void si_output_buffer_flush(struct SiOutputBuffer* output_buffer) {
	switch (output_buffer->mode) {
	case SI_OUTPUT_BUFFER_MODE2:
	case SI_OUTPUT_BUFFER_MODE3:
		si_circular_forward(output_buffer->circular_data_buffer, si_circular_data_available(output_buffer->circular_data_buffer));
		break;

	default:
		break;
	}
}

void si_output_buffer_close(struct SiOutputBuffer* output_buffer) {
    // TODO: Implement
}