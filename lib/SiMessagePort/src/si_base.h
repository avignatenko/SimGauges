#ifndef SI_BASE_H
#define SI_BASE_H

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include <assert.h>

#ifdef WIN
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef int SI_BOOL;

#define SI_TRUE    1
#define SI_FALSE   0

enum SiResult {
    SI_ERROR,
    SI_OK
};

#define SI_BASE_PI		 3.141592	// PI
#define SI_BASE_PI180	 0.017453	// PI / 180
#define SI_BASE_180PI	57.295780	// 180 / PI

// Convert degrees to radians and vice versa.
#define SI_BASE_DEGREES_TO_RADIANS(x) (x * SI_BASE_PI180)
#define SI_BASE_RADIANS_TO_DEGREES(x) (x * SI_BASE_180PI)

#define SI_BIT(B) (1 << B)
#define SI_BIT_SET(R, B) (R |= (1 << (B)))
#define SI_BIT_CLR(R, B) (R &= ~(1 << (B)))
#define SI_BIT_TOGGLE(R, B) (R ^= (1 << (B)))
#define SI_BIT_PUT(R, B, V) (V ? SI_BIT_SET(R, (B)) : SI_BIT_CLR(R, (B)) )
#define SI_BIT_GET(R, B) (R & (1 << (B)) ? SI_TRUE : SI_FALSE)

#define SI_MIN(a,b) (((a)<(b))?(a):(b))
#define SI_MAX(a,b) (((a)>(b))?(a):(b))
#define SI_CAP(val, min, max) (((val)<(min))?(min):((val)>(max)?(max):(val)))

#define SI_TRANSACTION_STATE_IN_PROGRESS 0
#define SI_TRANSACTION_STATE_DONE        1
#define SI_TRANSACTION_STATE_FAILED      2

#define SI_TRANSACTION_ERROR_CHAR_SIZE 512

#ifdef __cplusplus
#ifdef WIN
#define SI_EXPORT extern "C" __declspec(dllexport)
#endif
#ifdef MAC
#define SI_EXPORT extern "C" __attribute__((visibility("default")))
#endif
#ifdef LIN
#define SI_EXPORT extern "C" __attribute__((visibility("default")))
#endif
#else
#ifdef WIN
#define SI_EXPORT __declspec(dllexport)
#endif
#ifdef MAC
#define SI_EXPORT __attribute__((visibility("default")))
#endif
#ifdef LIN
#define SI_EXPORT __attribute__((visibility("default")))
#endif
#endif

struct SiTransactionResponse {
	int state;
	float percentage; 
	char error[SI_TRANSACTION_ERROR_CHAR_SIZE];

	int bytes_total;

    void* tag;
};

struct SiTransactionResponse* si_base_create_transaction_response(int state, const char* error, float percentage, int bytes_total, void* tag);
void si_base_load_transaction_response(struct SiTransactionResponse* response, int state, const char* error, float percentage, int bytes_total, void* tag);

void si_base_free_transaction_response(struct SiTransactionResponse* response);

#ifdef __cplusplus
}
#endif

#endif