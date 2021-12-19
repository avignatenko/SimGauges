#pragma once

#include "si_base.h"
#include "si_input_buffer.h"
#include "si_output_buffer.h"

#define SIM_EXTERN_API_VERSION        7 // Current API version

#define SIM_EXTERN_MESSAGE_PORT_PACKET_LEN                                           64 // Number of bytes in a sim extern message packet
#define SIM_EXTERN_MESSAGE_PORT_INT_PACKET_LEN   (SIM_EXTERN_MESSAGE_PORT_PACKET_LEN/sizeof(int32_t)) // Number of integers in a sim extern message packet
#define SIM_EXTERN_MESSAGE_PORT_FLOAT_PACKET_LEN (SIM_EXTERN_MESSAGE_PORT_PACKET_LEN/sizeof(float)) // Number of floats in a sim extern message packet

// This size calculation might not be accurate all of the time, but it works for now I guess...
#define SIM_EXTERN_MAX_MESSAGE_SIZE (uint8_t)sizeof(union SimExternalMessageUnion)

#ifdef __cplusplus
extern "C"{
#endif

enum SimExternMessageType {
	SIM_EXTERN_MESSAGE_TYPE_UNKNOWN = 0,
	SIM_EXTERN_MESSAGE_TYPE_REQUEST_DEVICE_INFORMATION = 1,
	SIM_EXTERN_MESSAGE_TYPE_DEVICE_INFORMATION = 2,
	SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_HOST_TO_DEVICE_BYTES = 23,
	SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_DEVICE_TO_HOST_BYTES = 24,
	SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_HOST_TO_DEVICE_INTEGERS = 25,
	SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_DEVICE_TO_HOST_INTEGERS = 26,
	SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_HOST_TO_DEVICE_FLOATS = 27,
	SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_DEVICE_TO_HOST_FLOATS = 28,
	SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_HOST_TO_DEVICE_STRING = 29,
	SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_DEVICE_TO_HOST_STRING = 30,
	SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_HOST_TO_DEVICE_EMPTY = 31,
	SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_DEVICE_TO_HOST_EMPTY = 32,
	SIM_EXTERN_MESSAGE_TYPE_MESSAGE_PORT_PRINT = 33
};

enum SimExternDeviceType {
	SIM_EXTERN_DEVICE_TYPE_UNKNOWN = 0,
	SIM_EXTERN_DEVICE_TYPE_HW_PORT = 1,
	SIM_EXTERN_DEVICE_TYPE_ARDUINO_2560 = 2,
	SIM_EXTERN_DEVICE_TYPE_ARDUINO_UNO = 3,
	SIM_EXTERN_DEVICE_TYPE_ARDUINO_LEONARDO = 4,
	SIM_EXTERN_DEVICE_TYPE_ARDUINO_MICRO = 5,
	SIM_EXTERN_DEVICE_TYPE_ARDUINO_NANO = 6,
	SIM_EXTERN_DEVICE_TYPE_KNOBSTER_V2 = 7,
	SIM_EXTERN_DEVICE_TYPE_ARDUINO_NANO_EVERY = 8,
	SIM_EXTERN_DEVICE_TYPE_ESP32 = 9,
	SIM_EXTERN_DEVICE_TYPE_NODE_MCU = 10,
	SIM_EXTERN_DEVICE_TYPE_ESP8266 = 11,
	SIM_EXTERN_DEVICE_TYPE_TEENSY_2_0 = 12,
	SIM_EXTERN_DEVICE_TYPE_TEENSY_PP_2_0 = 13,
	SIM_EXTERN_DEVICE_TYPE_TEENSY_LC = 14,
	SIM_EXTERN_DEVICE_TYPE_TEENSY_3_2 = 15,
	SIM_EXTERN_DEVICE_TYPE_TEENSY_3_5 = 16,
	SIM_EXTERN_DEVICE_TYPE_TEENSY_3_6 = 17,
	SIM_EXTERN_DEVICE_TYPE_TEENSY_4_0 = 18,
	SIM_EXTERN_DEVICE_TYPE_TEENSY_4_1 = 19,
	SIM_EXTERN_DEVICE_TYPE_ARDUINO_DUE = 20,
	SIM_EXTERN_DEVICE_TYPE_RPI_PICO = 21,
	SIM_EXTERN_DEVICE_TYPE_MAX = 22
};

enum SimExternDeviceMode {
	SIM_EXTERN_DEVICE_MODE_UNKNOWN = 0,
	SIM_EXTERN_DEVICE_MODE_PIN = 1,
	SIM_EXTERN_DEVICE_MODE_MESSAGE_PORT = 2,
	SIM_EXTERN_DEVICE_MODE_KNOBSTER = 3,
	SIM_EXTERN_DEVICE_MODE_MAX = 4
};

enum SimExternMessagePortPrintLogLevel {
	SIM_EXTERN_MESSAGE_PORT_LOG_LEVEL_UNKNOWN = 0,
	SIM_EXTERN_MESSAGE_PORT_LOG_LEVEL_TRACE = 1,
	SIM_EXTERN_MESSAGE_PORT_LOG_LEVEL_DEBUG = 2,
	SIM_EXTERN_MESSAGE_PORT_LOG_LEVEL_INFO = 3,
	SIM_EXTERN_MESSAGE_PORT_LOG_LEVEL_WARN = 4,
	SIM_EXTERN_MESSAGE_PORT_LOG_LEVEL_ERROR = 5,
	SIM_EXTERN_MESSAGE_PORT_LOG_LEVEL_MAX = 6
};

enum SimExternStepperMotorDirection {
	SIM_EXTERN_STEPPER_MOTOR_DIRECTION_UNKNOWN = 0,
	SIM_EXTERN_STEPPER_MOTOR_DIRECTION_FASTEST = 1,
	SIM_EXTERN_STEPPER_MOTOR_DIRECTION_SLOWEST = 2,
	SIM_EXTERN_STEPPER_MOTOR_DIRECTION_CW      = 3,
	SIM_EXTERN_STEPPER_MOTOR_DIRECTION_CCW     = 4,
    SIM_EXTERN_STEPPER_MOTOR_DIRECTION_ENDLESS_CW = 5,
    SIM_EXTERN_STEPPER_MOTOR_DIRECTION_ENDLESS_CCW = 6,
	SIM_EXTERN_STEPPER_MOTOR_DIRECTION_STOP = 7,
	SIM_EXTERN_STEPPER_MOTOR_DIRECTION_MAX = 8
};

struct SimExternMessageBase {
	enum SimExternMessageType type;
};

struct SimExternPortMessageBase {
	struct SimExternMessageBase base;
	uint16_t message_id;
};

struct SimExternPortMessageBytesBase {
	struct SimExternPortMessageBase base;
	uint8_t len;
	uint8_t buffer[SIM_EXTERN_MESSAGE_PORT_PACKET_LEN];
};

struct SimExternPortMessageIntegersBase {
	struct SimExternPortMessageBase base;
	uint8_t len;
	int32_t buffer[SIM_EXTERN_MESSAGE_PORT_INT_PACKET_LEN];
};

struct SimExternPortMessageFloatsBase {
	struct SimExternPortMessageBase base;
	uint8_t len;
	float buffer[SIM_EXTERN_MESSAGE_PORT_FLOAT_PACKET_LEN];
};

struct SimExternPortMessageStringBase {
	struct SimExternPortMessageBase base;
	uint8_t len;
	char buffer[SIM_EXTERN_MESSAGE_PORT_PACKET_LEN+1];
};

/*
	Device information
*/
struct SimExternRequestDeviceInformationMessage {
	struct SimExternMessageBase base;
};

struct SimExternDeviceInformationMessage {
	struct SimExternMessageBase base;

	uint8_t api_version;

	enum SimExternDeviceType type;
	enum SimExternDeviceMode mode;

	uint8_t channel;

	uint8_t nr_pins;
	uint8_t nr_digital_pins;
	uint8_t nr_analog_pins;
	uint8_t nr_groups;
};

/*
	Message (used mainly for external lib)
*/
struct SimExternMessagePortHostToDeviceMessage {
	struct SimExternPortMessageBase base;
};
struct SimExternMessagePortDeviceToHostMessage {
	struct SimExternPortMessageBase base;
};
struct SimExternMessagePortHostToDeviceBytesMessage {
	struct SimExternPortMessageBytesBase base;
};
struct SimExternMessagePortDeviceToHostBytesMessage {
	struct SimExternPortMessageBytesBase base;
};
struct SimExternMessagePortHostToDeviceIntegersMessage {
	struct SimExternPortMessageIntegersBase base;
};
struct SimExternMessagePortDeviceToHostIntegersMessage {
	struct SimExternPortMessageIntegersBase base;
};
struct SimExternMessagePortHostToDeviceFloatsMessage {
	struct SimExternPortMessageFloatsBase base;
};
struct SimExternMessagePortDeviceToHostFloatsMessage {
	struct SimExternPortMessageFloatsBase base;
};
struct SimExternMessagePortHostToDeviceStringMessage {
	struct SimExternPortMessageStringBase base;
};
struct SimExternMessagePortDeviceToHostStringMessage {
	struct SimExternPortMessageStringBase base;
};

struct SimExternMessagePortPrintMessage {
	struct SimExternMessageBase base;

	uint8_t len;
	char buffer[SIM_EXTERN_MESSAGE_PORT_PACKET_LEN+1];

	enum SimExternMessagePortPrintLogLevel level;
};

// Make sure to add all available messages in this union
// The union is used to determine the size of the biggest message object
union SimExternalMessageUnion {
	struct SimExternDeviceInformationMessage device_info;
	struct SimExternMessagePortHostToDeviceBytesMessage message_port_host_to_device_bytes_message;
	struct SimExternMessagePortDeviceToHostBytesMessage message_port_device_to_host_bytes_message;
	struct SimExternMessagePortHostToDeviceIntegersMessage message_port_host_to_device_integers_message;
	struct SimExternMessagePortDeviceToHostIntegersMessage message_port_device_to_host_integers_message;
	struct SimExternMessagePortHostToDeviceFloatsMessage message_port_host_to_device_floats_message;
	struct SimExternMessagePortDeviceToHostFloatsMessage message_port_device_to_host_floats_message;
	struct SimExternMessagePortHostToDeviceStringMessage message_port_host_to_device_string_message;
	struct SimExternMessagePortDeviceToHostStringMessage message_port_device_to_host_string_message;
	struct SimExternMessagePortPrintMessage message_port_print_message;
};

enum SiResult sim_extern_shared_create_message(const struct SiInputBuffer* input_buffer, const uint8_t len, struct SimExternMessageBase* message);
enum SiResult sim_extern_shared_push_message(const struct SimExternMessageBase* message, const struct SiOutputBuffer* output_buffer);

#ifdef __cplusplus
}
#endif
