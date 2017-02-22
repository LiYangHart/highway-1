/**
 * Methods for working with Hayes command and response codes.
 */
#ifndef _HAYES_H_
#define _HAYES_H_

#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Error codes for API functions. */
#define HAYES_OK      0
#define HAYES_ERROR   1
#define HAYES_TIMEOUT 2

/* Default command or response timeout. */
#define HAYES_DEFAULT_TIMEOUT 1000

/*  */
typedef struct _HayesApi {
	uint8_t (*write)(uint8_t* buffer, uint16_t start, uint16_t length);
	uint16_t (*count)();
	uint8_t (*get_char)();
} HayesApi;

/*  */
typedef struct _ATDevice {
	HayesApi api;
	uint8_t * buffer;
	uint16_t length;
	uint16_t read;
} ATDevice;

/* Predicate functions for hayes_res() */
typedef uint8_t (*RES_PREDICATE)(ATDevice * dev, void * param);

/* Function protoypes */
uint8_t hayes_at(ATDevice * dev, char * command);
uint8_t hayes_write(ATDevice * dev, uint8_t * buffer, uint8_t start, uint8_t length);
uint8_t hayes_res(ATDevice * dev, RES_PREDICATE test, void * param, uint16_t timeout);
char* tokenize_res(ATDevice * dev, char * prev_token);

/* Predicate function prototypes */
uint8_t pred_ends_with(ATDevice* dev, void* param);

#ifdef __cplusplus
}
#endif

#endif /* _HAYES_H_ */

