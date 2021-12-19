#ifndef SI_INPUT_BUFFER_H
#define SI_INPUT_BUFFER_H

#include <stdint.h>

#include "si_base.h"
#include "si_circular_data.h"

#define SI_INPUT_BUFFER_MODE2_HEADER_SIZE 2

#ifdef __cplusplus
extern "C"{
#endif

enum SiInputBufferMode {
	SI_INPUT_BUFFER_MODE1, // Each message has a unsigned int (4 bytes) proceeding it. No CRC checking
	SI_INPUT_BUFFER_MODE2, // Each message has a unique starting byte and a 1 byte len (max payload is 254 bytes)
    SI_INPUT_BUFFER_MODE3  // Each message starts with 0x00 and ends with 0xFF and is exact 25 bytes in lenght
};

enum SiInputBufferLenModeState {
	LEN_MODE_STATE_WAITING_FOR_LEN,
	LEN_MODE_STATE_WAITING_FOR_DATA
};

enum SiInputBufferStartLenModeState {
	START_LEN_MODE_STATE_WAITING_FOR_START_TAG,
	START_LEN_MODE_STATE_WAITING_FOR_LEN,
	START_LEN_MODE_STATE_WAITING_FOR_PAYLOAD
};

struct SiInputBuffer {
	enum SiInputBufferMode mode;

	struct SiCircularData* mode2_circular_data_buffer;
	const void* mode2_tag;
	void(*mode2_new_message_callback)(const struct SiInputBuffer* input_buffer, const uint8_t len, const void* tag);
};

// Mode 2
struct SiInputBuffer* si_input_buffer_mode2_init(struct SiInputBuffer* input_buffer, struct SiCircularData* circular_data_buffer, void(*new_message_callback)(const struct SiInputBuffer* input_buffer, const uint8_t len, const void* tag), const void* tag);
void si_input_buffer_mode2_set_callback(struct SiInputBuffer* input_buffer, void(*new_message_callback)(const struct SiInputBuffer* input_buffer, const uint8_t len, const void* tag), const void* tag);
enum SiResult si_input_buffer_mode2_push_data(const struct SiInputBuffer* input_buffer, const uint8_t* data, const uint8_t len);
void si_input_buffer_mode2_evaluate(const struct SiInputBuffer* input_buffer);

void si_input_buffer_flush(struct SiInputBuffer* input_buffer);
void si_input_buffer_close(struct SiInputBuffer* input_buffer);

#ifdef __cplusplus
}
#endif

#endif