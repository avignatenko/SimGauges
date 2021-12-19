#include "si_input_buffer.h"

#include "si_network.h"
#include <stdlib.h>
#include <string.h>

#define MODE2_HEADER_SIZE 2 // Start tag and payload len
#define MODE2_START_BYTE 0xFF

struct SiInputBuffer* si_input_buffer_mode2_init(struct SiInputBuffer* input_buffer, struct SiCircularData* circular_data_buffer, void(*new_message_callback)(const struct SiInputBuffer* input_buffer, const uint8_t len, const void* tag), const void* tag) {
	input_buffer->mode = SI_INPUT_BUFFER_MODE2;

	input_buffer->mode2_new_message_callback = new_message_callback;
	input_buffer->mode2_circular_data_buffer = circular_data_buffer;
	input_buffer->mode2_tag = tag;

	return input_buffer;
}

void si_input_buffer_mode2_set_callback(struct SiInputBuffer* input_buffer, void(*new_message_callback)(const struct SiInputBuffer* input_buffer, const uint8_t len, const void* tag), const void* tag) {
	input_buffer->mode2_new_message_callback = new_message_callback;
	input_buffer->mode2_tag = tag;
}

enum SiResult si_input_buffer_mode2_push_data(const struct SiInputBuffer* input_buffer, const uint8_t* data, const uint8_t len) {
	if (si_circular_data_free(input_buffer->mode2_circular_data_buffer) < len) {
		return SI_ERROR;
	}
	else {
		uint8_t i;
		for (i = 0; i < len; i++) {
			si_circular_push(input_buffer->mode2_circular_data_buffer, data[i]);
		}
		return SI_OK;
	}
}

void si_input_buffer_mode2_evaluate(const struct SiInputBuffer* input_buffer) {
	uint8_t len;
	
	// The start of the circular buffer should always contain the start of a message
	// We will purge all non start byte data from the start of the buffer
	while (si_circular_data_available(input_buffer->mode2_circular_data_buffer) > 0) {
		uint8_t byte = 0;
		si_circular_peek(input_buffer->mode2_circular_data_buffer, 0, &byte);

		if (byte == MODE2_START_BYTE) {
			// The buffer now indeed starts with a message start tag, we are done
			break;
		}
		else {
			// Remove this byte from the buffer
			si_circular_forward(input_buffer->mode2_circular_data_buffer, 1);
		}
	}

	// Check if we can find any full message in the buffer
	// There might be more then one
	while (si_circular_data_available(input_buffer->mode2_circular_data_buffer) >= 2) {
		// Check if we have the start byte
		uint8_t start_byte;
		uint8_t payload_len_byte;

		si_circular_peek(input_buffer->mode2_circular_data_buffer, 0, &start_byte);
		si_circular_peek(input_buffer->mode2_circular_data_buffer, 1, &payload_len_byte);

		if ((start_byte == MODE2_START_BYTE) && (payload_len_byte != MODE2_START_BYTE)) {

			const uint8_t expected_message_len = (MODE2_HEADER_SIZE + payload_len_byte);

			// Check if the circular buffer is even big enough to house this message
			if (expected_message_len > si_circular_data_total_size(input_buffer->mode2_circular_data_buffer)) {
				si_circular_forward(input_buffer->mode2_circular_data_buffer, si_circular_data_available(input_buffer->mode2_circular_data_buffer));
			}

			// if the buffer contains at least '2 + payload len' number of bytes, we should have a complete message!
			if (si_circular_data_available(input_buffer->mode2_circular_data_buffer) >= expected_message_len) {

				// Count number of 0xFF in payload
				uint8_t i;
				for (i = 1; i < payload_len_byte; i++) {
					uint8_t prev_byte = 0;
					uint8_t byte = 0;

					si_circular_peek(input_buffer->mode2_circular_data_buffer, MODE2_HEADER_SIZE + i - 1, &prev_byte);
					si_circular_peek(input_buffer->mode2_circular_data_buffer, MODE2_HEADER_SIZE + i, &byte);

					if ((prev_byte == MODE2_START_BYTE) && (byte == MODE2_START_BYTE)) {
						// Poll away one of the 0xFF padding bytes
						si_circular_poll(input_buffer->mode2_circular_data_buffer, MODE2_HEADER_SIZE + i, NULL);

						// Payload just became one byte shorter, since we removed it
						payload_len_byte--;
					}
				}

				// Skip start and len bytes
				si_circular_forward(input_buffer->mode2_circular_data_buffer, MODE2_HEADER_SIZE);

				// Let the application know we have new data
				if (input_buffer->mode2_new_message_callback != NULL) {
					input_buffer->mode2_new_message_callback(input_buffer, payload_len_byte, input_buffer->mode2_tag);
				}

				// Skip payload bytes
				si_circular_forward(input_buffer->mode2_circular_data_buffer, payload_len_byte);
			}
			else {
				// There is still an uncompleted message in the buffer
				// We break from the while above and wait for new data to arrive
				break;
			}
		}
		else {
			// Illegal start sequence
			// Remove one of these bytes from the input buffer
			si_circular_forward(input_buffer->mode2_circular_data_buffer, 1);
		}
	}

	// We will find the last start tag starting from the back so we will find the last message
	// Any data before this last start tag is corrupted (since it was not removed by the code above)
	len = si_circular_data_available(input_buffer->mode2_circular_data_buffer);
	if (len > 3) {
		uint8_t i;
		for (i = (len - 2); i > 0; i--) {
			uint8_t prev_prev_byte = 0;
			uint8_t prev_byte = 0;
			uint8_t byte = 0;

			si_circular_peek(input_buffer->mode2_circular_data_buffer, i - 2, &prev_prev_byte);
			si_circular_peek(input_buffer->mode2_circular_data_buffer, i - 1, &prev_byte);
			si_circular_peek(input_buffer->mode2_circular_data_buffer, i, &byte);

			if ((prev_prev_byte != MODE2_START_BYTE) && (prev_byte == MODE2_START_BYTE) && (byte != MODE2_START_BYTE)) {
				// We have found the latest start tag!

				// Remove any data before this start tag (if there is any)
				if ((i-1) > 0) {
					si_circular_forward(input_buffer->mode2_circular_data_buffer, i-1);
				}
			}
		}
	}
}

void si_input_buffer_flush(struct SiInputBuffer* input_buffer) {
	if (input_buffer->mode == SI_INPUT_BUFFER_MODE2) {
		si_circular_forward(input_buffer->mode2_circular_data_buffer, si_circular_data_available(input_buffer->mode2_circular_data_buffer));
	}
}

void si_input_buffer_close(struct SiInputBuffer* input_buffer) {
	assert(input_buffer != NULL);
    // TODO: Implement
}
