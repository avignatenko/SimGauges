#include "sim_extern_shared.h"

#include "si_network.h"

enum SiResult sim_extern_shared_create_message(const struct SiInputBuffer* input_buffer, const uint8_t len, struct SimExternMessageBase* message) {
	uint8_t byte;

	if (len == 0) {
		return SI_ERROR;
	}

	struct SiCircularData* circular_data = input_buffer->mode2_circular_data_buffer;

	si_circular_peek(circular_data, 0, &byte);

	// Every message has a message id
	message->type = (enum SimExternMessageType) byte;

	switch (message->type) {
		case SIM_EXTERN_MESSAGE_TYPE_REQUEST_DEVICE_INFORMATION:
			return (len == 1) ? SI_OK : SI_ERROR;

		case SIM_EXTERN_MESSAGE_TYPE_DEVICE_INFORMATION: {
			struct SimExternDeviceInformationMessage* device_information = (struct SimExternDeviceInformationMessage*) message;

			si_circular_peek(circular_data, 1, &device_information->api_version);
			si_circular_peek(circular_data, 2, &device_information->channel);
			si_circular_peek(circular_data, 3, &byte);
			device_information->type = (enum SimExternDeviceType) byte;

			si_circular_peek(circular_data, 4, &device_information->nr_pins);
			si_circular_peek(circular_data, 5, &device_information->nr_digital_pins);
			si_circular_peek(circular_data, 6, &device_information->nr_analog_pins);
			si_circular_peek(circular_data, 7, &device_information->nr_groups);

			if (len == 9) {
				si_circular_peek(circular_data, 8, &byte);
				device_information->mode = (enum SimExternDeviceMode) byte;
			}
			else {
				device_information->mode = SIM_EXTERN_DEVICE_MODE_PIN;
			}

			return ( (len == 8) || (len == 9) ) ? SI_OK : SI_ERROR;
		}
		
        case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_HOST_TO_DEVICE_EMPTY:
        case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_DEVICE_TO_HOST_EMPTY: {
            struct SimExternPortMessageBase* message_message = (struct SimExternPortMessageBase*) message;

            si_circular_peek(circular_data, 1, &byte);
            message_message->message_id = ((uint16_t)byte * 256);
            si_circular_peek(circular_data, 2, &byte);
            message_message->message_id += ((uint16_t)byte);

            return (len == 3) ? SI_OK : SI_ERROR;
        }

		case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_HOST_TO_DEVICE_BYTES:
		case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_DEVICE_TO_HOST_BYTES: {
			struct SimExternPortMessageBytesBase* message_message = (struct SimExternPortMessageBytesBase*) message;

			si_circular_peek(circular_data, 1, &byte);
			message_message->base.message_id = ((uint16_t)byte * 256);
			si_circular_peek(circular_data, 2, &byte);
			message_message->base.message_id += ((uint16_t)byte);

			si_circular_peek(circular_data, 3, &message_message->len);

			for(uint8_t i = 0; (i < message_message->len) && (i < SIM_EXTERN_MESSAGE_PORT_PACKET_LEN); i++) {
				si_circular_peek(circular_data, 4 + i, message_message->buffer + i);
			}
			
			return (len == (4 + message_message->len) ) ? SI_OK : SI_ERROR;
		}

		case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_HOST_TO_DEVICE_STRING:
		case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_DEVICE_TO_HOST_STRING: {
			struct SimExternPortMessageStringBase* message_message = (struct SimExternPortMessageStringBase*) message;

			si_circular_peek(circular_data, 1, &byte);
			message_message->base.message_id = ((uint16_t)byte * 256);
			si_circular_peek(circular_data, 2, &byte);
			message_message->base.message_id += ((uint16_t)byte);

			si_circular_peek(circular_data, 3, &message_message->len);

            uint8_t msg_len = (uint8_t) SI_MIN(SIM_EXTERN_MESSAGE_PORT_PACKET_LEN, message_message->len);
			for(uint8_t i = 0; i < msg_len; i++) {
				si_circular_peek(circular_data, 4 + i, (uint8_t*) (message_message->buffer + i) );
			}
            message_message->buffer[msg_len] = '\0';
			
			return (len == (4 + message_message->len) ) ? SI_OK : SI_ERROR;
		}

		case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_HOST_TO_DEVICE_INTEGERS:
		case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_DEVICE_TO_HOST_INTEGERS: {
			struct SimExternPortMessageIntegersBase* message_message = (struct SimExternPortMessageIntegersBase*) message;

			si_circular_peek(circular_data, 1, &byte);
			message_message->base.message_id = ((uint16_t)byte * 256);
			si_circular_peek(circular_data, 2, &byte);
			message_message->base.message_id += ((uint16_t)byte);

			si_circular_peek(circular_data, 3, &message_message->len);

			for(uint8_t i = 0; (i < message_message->len) && (i < SIM_EXTERN_MESSAGE_PORT_INT_PACKET_LEN); i++) {
				int32_t value = 0;
				si_circular_peek(circular_data, 4 + (i * 4) + 0, &byte);
				value = byte;
				si_circular_peek(circular_data, 4 + (i * 4) + 1, &byte);
				value |= ((int32_t)byte << 8);
				si_circular_peek(circular_data, 4 + (i * 4) + 2, &byte);
				value |= ((int32_t)byte << 16);
				si_circular_peek(circular_data, 4 + (i * 4) + 3, &byte);
				value |= ((int32_t)byte << 24);

				message_message->buffer[i] = SI_NETWORK_INT32_TO_NETWORK(value);
			}
			
			return (len == (4 + (message_message->len * 4) ) ) ? SI_OK : SI_ERROR;
		}

		case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_HOST_TO_DEVICE_FLOATS:
		case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_DEVICE_TO_HOST_FLOATS: {
			struct SimExternPortMessageFloatsBase* message_message = (struct SimExternPortMessageFloatsBase*) message;

			si_circular_peek(circular_data, 1, &byte);
			message_message->base.message_id = ((uint16_t)byte * 256);
			si_circular_peek(circular_data, 2, &byte);
			message_message->base.message_id += ((uint16_t)byte);

			si_circular_peek(circular_data, 3, &message_message->len);

			for(uint8_t i = 0; (i < message_message->len) && (i < SIM_EXTERN_MESSAGE_PORT_INT_PACKET_LEN); i++) {
                uint8_t value[4];
                si_circular_peek(circular_data, 4 + (i * 4) + 0, &byte);
                value[0] = byte;
                si_circular_peek(circular_data, 4 + (i * 4) + 1, &byte);
                value[1] = byte;
                si_circular_peek(circular_data, 4 + (i * 4) + 2, &byte);
                value[2] = byte;
                si_circular_peek(circular_data, 4 + (i * 4) + 3, &byte);
                value[3] = byte;

                message_message->buffer[i] = si_network_float(value);
			}
			
			return (len == (4 + (message_message->len * 4) ) ) ? SI_OK : SI_ERROR;
		}

		case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_PRINT: {
			struct SimExternMessagePortPrintMessage* print_message = (struct SimExternMessagePortPrintMessage*) message;

			si_circular_peek(circular_data, 1, &byte);
			print_message->level = (enum SimExternMessagePortPrintLogLevel) byte;

			si_circular_peek(circular_data, 2, &print_message->len);

            uint8_t msg_len = (uint8_t)SI_MIN(SIM_EXTERN_MESSAGE_PORT_PACKET_LEN, print_message->len);
			for(uint8_t i = 0; i < msg_len; i++) {
				si_circular_peek(circular_data, 3 + i, (uint8_t*) print_message->buffer + i);
			}
			print_message->buffer[msg_len] = '\0';
			
			return (len == (3 + print_message->len) ) ? SI_OK : SI_ERROR;
		}

		default:
			break;
	}

	return SI_ERROR;
}

enum SiResult sim_extern_shared_push_message(const struct SimExternMessageBase* message, const struct SiOutputBuffer* output_buffer) {
	uint8_t buffer[SIM_EXTERN_MAX_MESSAGE_SIZE];
	uint8_t len = 0;

	// First byte is always the message type
	buffer[0] = (uint8_t) message->type;

	switch (message->type) {
	case SIM_EXTERN_MESSAGE_TYPE_REQUEST_DEVICE_INFORMATION: {
		len = 1;
		break;
	}

	case SIM_EXTERN_MESSAGE_TYPE_DEVICE_INFORMATION: {
		struct SimExternDeviceInformationMessage* device_information = (struct SimExternDeviceInformationMessage*) message;

		buffer[1] = device_information->api_version;
		buffer[2] = device_information->channel;
		buffer[3] = (uint8_t) device_information->type;
		buffer[4] = device_information->nr_pins;
		buffer[5] = device_information->nr_digital_pins;
		buffer[6] = device_information->nr_analog_pins;
		buffer[7] = device_information->nr_groups;
		buffer[8] = (uint8_t) device_information->mode;
		len = 9;
		break;
	}

    case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_HOST_TO_DEVICE_EMPTY:
    case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_DEVICE_TO_HOST_EMPTY: {
        struct SimExternPortMessageBase* message_message = (struct SimExternPortMessageBase*) message;

        buffer[1] = (uint8_t)((message_message->message_id >> 8) & 0xFF);
        buffer[2] = (uint8_t)((message_message->message_id >> 0) & 0xFF);

        len = 3;
        break;
    }

	case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_HOST_TO_DEVICE_BYTES:
	case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_DEVICE_TO_HOST_BYTES: {
		struct SimExternPortMessageBytesBase* message_message = (struct SimExternPortMessageBytesBase*) message;

		buffer[1] = (uint8_t)((message_message->base.message_id >> 8) & 0xFF);
		buffer[2] = (uint8_t)((message_message->base.message_id >> 0) & 0xFF);

		buffer[3] = message_message->len;
		for(uint8_t i = 0; i < message_message->len; i++) {
			buffer[4+i] = message_message->buffer[i];
		}

		len = 4 + message_message->len;
		break;
	}

	case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_HOST_TO_DEVICE_STRING:
	case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_DEVICE_TO_HOST_STRING: {
		struct SimExternPortMessageStringBase* message_message = (struct SimExternPortMessageStringBase*) message;

		buffer[1] = (uint8_t)((message_message->base.message_id >> 8) & 0xFF);
		buffer[2] = (uint8_t)((message_message->base.message_id >> 0) & 0xFF);

		buffer[3] = message_message->len;
		for(uint8_t i = 0; i < message_message->len; i++) {
			buffer[4+i] = message_message->buffer[i];
		}

		len = 4 + message_message->len;
		break;
	}

	case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_HOST_TO_DEVICE_INTEGERS:
	case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_DEVICE_TO_HOST_INTEGERS: {
		struct SimExternPortMessageIntegersBase* message_message = (struct SimExternPortMessageIntegersBase*) message;

		buffer[1] = (uint8_t)((message_message->base.message_id >> 8) & 0xFF);
		buffer[2] = (uint8_t)((message_message->base.message_id >> 0) & 0xFF);

		buffer[3] = message_message->len;
		for(uint8_t i = 0; i < message_message->len; i++) {
			int32_t* int32_buffer = (int32_t*) (buffer + (4 + (i*4)));
			*int32_buffer = SI_NETWORK_INT32_TO_NETWORK(message_message->buffer[i]);
		}

		len = 4 + (message_message->len * 4);
		break;
	}

	case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_HOST_TO_DEVICE_FLOATS:
	case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_DEVICE_TO_HOST_FLOATS: {
		struct SimExternPortMessageFloatsBase* message_message = (struct SimExternPortMessageFloatsBase*) message;

		buffer[1] = (uint8_t)((message_message->base.message_id >> 8) & 0xFF);
		buffer[2] = (uint8_t)((message_message->base.message_id >> 0) & 0xFF);

		buffer[3] = message_message->len;
		for(uint8_t i = 0; i < message_message->len; i++) {
			float* float_buffer = (float*) (buffer + (4 + (i*4)));
            *float_buffer = si_network_swap_float(message_message->buffer[i]);
		}

		len = 4 + (message_message->len * 4);
		break;
	}

	case SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_PRINT: {
		struct SimExternMessagePortPrintMessage* print_message = (struct SimExternMessagePortPrintMessage*) message;

		buffer[1] = (uint8_t)print_message->level;
		buffer[2] = print_message->len;
		for(uint8_t i = 0; i < print_message->len; i++) {
			buffer[3+i] = (uint8_t) print_message->buffer[i];
		}

		len = 3 + print_message->len;
		break;
	}

	default:
		return SI_ERROR;
	}

	// Try to push this message into the output buffer
	return si_output_buffer_mode2_push_data(output_buffer, buffer, len);
}
