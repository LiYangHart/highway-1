#include <hayes.h>
#include <string.h>

/**
 * Output a buffer to the Hayes compatible device.
 * Calls the underlying write API method.
 */
uint8_t
hayes_write(ATDevice* dev, uint8_t* buffer, uint8_t start, uint8_t length) {
	return dev->api.write(buffer, start, length);
}

/**
 * Output a string to the Hayes compatible device.
 * Remember to add \r\n to the end of the command.
 */
uint8_t
hayes_at(ATDevice* dev, char* command) {
	return dev->api.write((uint8_t*)command, 0, strlen(command));
}

/**
 * Read characters into the buffer until the given predicate is satisfied.
 */
uint8_t
hayes_res(ATDevice* dev, RES_PREDICATE test, void* param, uint16_t timeout) {
	dev->read = 0;
	for (uint16_t d = 0; d < timeout; d += 10) {
		while (dev->api.count() > 0) {
			if (dev->read == dev->length) {
				return HAYES_ERROR;
			} else {
				dev->buffer[dev->read++] = dev->api.getc();
				if (test(dev, param)) {
					return HAYES_OK;
				}
			}
		}

		vTaskDelay(10);
	}

	return HAYES_TIMEOUT;
}

/**
 * Tokenize a response buffer which is delimited by \r\n.
 *
 * This method modifies the buffer by inserting \0 at the delimiter.
 */
char*
tokenize_res(ATDevice* dev, char* prev_token) {
	char *c, *d, *end;

	// Note the end of the buffer.
	end = dev->buffer + dev->length - 1;

	// Seek to the end of the current token.
	if (prev_token == NULL) {
		c = dev->buffer;
	} else {
		c = prev_token;
		c += strlen(prev_token);
		c += 2; /* \r\n */
	}

	if (c >= end) {
		return NULL;
	}

	// Find the end of the line and convert to null termination.
	d = c;
	while (d < end) {
		if (*d == '\r' && *(d + 1) == '\n') {
			*d = '\0';
			*(d + 1) = '\0';
			return c;
		}
		++d;
	}

	return NULL;
}

// Response predicate functions ------------------------------------------------

/**
 * True if the buffer ends with the string given by 'param', otherwise false.
 */
uint8_t
pred_ends_with(ATDevice* dev, void* param) {
	char* param_ends_with = (char*)param;
	uint8_t param_length = strlen(param_ends_with);

	if (dev->read < param_length) {
		return 0;
	}

	char* in_buffer = dev->buffer + (dev->read - param_length);

	return strncmp(in_buffer, param_ends_with, param_length) == 0;
}
