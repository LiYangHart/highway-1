#include <peripheral/skywire.h>
#include <peripheral/virtual_com.h>
#include <peripheral/i2c_spi_bus.h>
#include <task/skywire_task.h>
#include <task/beacon_task.h>
#include <hayes.h>
#include <stdio.h>
#include <string.h>
#include <fat_sl.h>
#include "diag/Trace.h"

/* Buffer for the Skywire modem communication. */
char buffer[512];

/**
 * Activate the PDP context for socket communication with the Internet.
 */
uint8_t
activate_pdp_context(ATDevice* dev) {
	/**
	 * Check the state of PDP context 1.
	 */
	if (   hayes_at(dev, "AT#SGACT?\r\n")                 != HAYES_OK
		|| hayes_res(dev, pred_ends_with, "OK\r\n", 1000) != HAYES_OK) {
		return 0;
	} else {
		int cid, stat;
		char* line = NULL;
		while ((line = tokenize_res(dev, line)) != NULL) {
			/* Look for the state of PDP context 1. */
			if ((sscanf(line, "#SGACT: %d,%d", &cid, &stat) == 2) && (cid == 1)) {
				if (stat == 1) {
					/* Context is already active. */
					return 1;
				} else {
					/* Proceed to activation step. */
					break;
				}
			}
		}
	}

	/**
	 * Input the APN for the Rogers network.
	 * Activate PDP context 1.
	 */
	if (   hayes_at(dev, "AT+CGDCONT=1,\"IP\",\"internet.com\"\r\n") != HAYES_OK
		|| hayes_res(dev, pred_ends_with, "OK\r\n", 1000)            != HAYES_OK
		|| hayes_at(dev, "AT#SGACT=1,1\r\n")                         != HAYES_OK
		|| hayes_res(dev, pred_ends_with, "OK\r\n", 10000)           != HAYES_OK) {
		return 0;
	}

	return 1;
}

/**
 * Initialize the peripherals and state for this task.
 */
uint8_t
skywire_task_setup(ATDevice* dev) {
	vcp_init();
	skywire_init();

	/* Execute the activation sequence - this takes about 15 seconds but is
	   required to enable the Skywire modem. */
	skywire_activate();

	/**
	 * Disable echo.
	 * Disable flow control.
	 */
	if (   hayes_at(dev, "ATE0\r\n")                      != HAYES_OK
		|| hayes_res(dev, pred_ends_with, "OK\r\n", 1000) != HAYES_OK
		|| hayes_at(dev, "AT&K=0\r\n")                    != HAYES_OK
		|| hayes_res(dev, pred_ends_with, "OK\r\n", 1000) != HAYES_OK) {
		trace_printf("skywire_task: stage1 failed\n");
		return 0;
	}

	if (!activate_pdp_context(dev)) {
		trace_printf("skywire_task: failed to activate PDP 1\n");
		return 0;
	}

	return 1; // OK
}

// POST manifest ---------------------------------------------------------------

/**
 * Add a file to the manifest.
 */
uint8_t
add_to_manifest(Attachment** head, Attachment** tail, char* file) {
	/* Get the size of the attachment. */
	uint32_t length;
	if ((length = f_filelength(file)) == 0) {
		return 0;
	}

	/* Allocate an entry for the manifest. */
	Attachment* attachment;
	attachment = (Attachment*)pvPortMalloc(sizeof(Attachment));
	attachment->length = length;
	attachment->next = NULL;
	strncpy(attachment->name, file, 64);

	/* Add the attachment to the manifest. */
	if (*head == NULL) {
		*head = attachment;
		*tail = *head;
	} else {
		(*tail)->next = attachment;
		*tail = attachment;
	}

	return 1;
}

/**
 * Construct a manifest of files to be sent to the server.
 * The manifest will contain data.log and any attached images.
 */
uint8_t
get_manifest(Attachment** manifest) {
	F_FILE* pxLog;
	if ((pxLog = f_open("data.log", "r")) == NULL) {
		trace_printf("skywire_task: failed to open data.log\n");
		return 0;
	}

	/* Add data.log to the start of the manifest. */
	Attachment* tail = NULL;
	if (!add_to_manifest(manifest, &tail, "data.log")) {
		goto error;
	}

	/* Parse the data log and look for attachments. */
	char line[128];
	while (f_eof(pxLog) == 0) {
		for (uint8_t i = 0; i < 128; ++i) {
			char c = f_getc(pxLog);
			if (c == '\n') {
				line[i] = '\0';
				break;
			} else {
				line[i] = c;
			}
		}

		if (memcmp(line, "FILE:", 5) == 0) {
			int tick;
			char file[64];
			if (sscanf((line + 5), "{\"tick\":%d,\"file\":\"%[^\"]\"}", &tick, file) == 2) {
				if (!add_to_manifest(manifest, &tail, file)) {
					goto error;
				}
			}

		}
	}

	f_close(pxLog);
	return 1;

error:
	if (pxLog != NULL) { f_close(pxLog); }
	return 0;
}

/**
 * Delete all files in the manifest.
 * Free all memory associated with the manifest.
 */
void
free_manifest(Attachment* manifest, uint8_t deleteFiles) {
	Attachment* next = NULL;
	while (manifest != NULL) {
		next = manifest->next;

		/* Delete the file from the SD card. */
		if (deleteFiles && (f_delete(manifest->name) == F_NO_ERROR)) {
			trace_printf("skywire_task: removed %s\n", manifest->name);
		}

		/* Free memory associated with this attachment. */
		vPortFree(manifest);

		manifest = next;
	}
}

/**
 * Use a POST to upload the manifest to the server.
 * Write slowly until we can support software flow control.
 *
 * Each HTTP chunk contains one attachment.
 * The first 32 bytes of the chunk encode the attachment length and filename.
 * Therefore, each HTTP chunk length is the attachment length plus 32 bytes.
 */
uint8_t
post_manifest(ATDevice* dev, Attachment* manifest) {
	if (   hayes_at(dev, "AT#SD=1,0,80,\"" NGROK_TUNNEL "\"\r\n") != HAYES_OK
		|| hayes_res(dev, pred_ends_with, "CONNECT\r\n", 10000)   != HAYES_OK) {
		trace_printf("skywire_task: open socket data failed\n");
		return 0;
	}

	trace_printf("skywire_task: starting POST\n");

	/* Output the HTTP header - specify POST with chunked encoding. */
	hayes_at(dev,
			"POST /data HTTP/1.1\r\n" \
			"Host: " NGROK_TUNNEL "\r\n" \
			"Content-Type: application/octet-stream\r\n" \
			"Transfer-Encoding: chunked\r\n" \
			"Connection: close\r\n\r\n"
	);

	while (manifest != NULL) {
		// Write file
		F_FILE* pxFile = f_open(manifest->name, "r");
		if (pxFile == NULL) {
			trace_printf("failed to open file");
			continue;
		}

		/* Write the start of the HTTP chunk. */
		int length = manifest->length;
		snprintf(dev->buffer, 32, "%x\r\n", 32 +length);
		hayes_at(dev, dev->buffer);

		/* Write the attachment header. */
		memset(dev->buffer, '\0', 32);
		snprintf(dev->buffer, 32, "%s,%d\r\n", manifest->name, length);
		hayes_write(dev, (uint8_t*)dev->buffer, 0, 32);

		/* Write the attachment. */
		uint32_t written = 0;
		while (written < manifest->length) {
			uint32_t read = f_read(dev->buffer, 1, 128, pxFile);
			written += read;
			hayes_write(dev, (uint8_t*)dev->buffer, 0, read);
		}

		f_close(pxFile);
		pxFile = NULL;

		/* Write the end of the HTTP chunk. */
		hayes_at(dev, "\r\n");
		manifest = manifest->next;
	}

	/* Write the trailing HTTP chunk and termination. */
	hayes_at(dev, "0\r\n\r\n");
	return 1;
}

// Task ------------------------------------------------------------------------

uint8_t
parse_response(ATDevice* dev, uint8_t* speedLimit) {
	/**
	 * Expect a 200 OK response.
	 * Skip the headers by looking for "\r\n\r\n".
	 * Look for no carrier when the connection is closed.
	 */
	if (   hayes_res(dev, pred_ends_with, "200 OK\r\n", 20000)    != HAYES_OK
		|| hayes_res(dev, pred_ends_with, "\r\n\r\n", 10000)       != HAYES_OK
		|| hayes_res(dev, pred_ends_with, "NO CARRIER\r\n", 10000) != HAYES_OK) {
		trace_printf("skywire_task: response not as expected\n");
		return 0;
	}

	int sl = 0;
	char* line = NULL;
	while ((line = tokenize_res(dev, line)) != NULL) {
		if (sscanf(line, "SL=%d,EOM", &sl) == 1) {
			//adding line to see if incoming SL value is good
			if (sl >= 1 && sl <= 255){
				*speedLimit = sl;
				return 1;
			}
		}
	}
	*speedLimit = 100;
	return 0;
}

/**
 *
 */
void
skywire_task(void* pvParameters) {
	/* Prepare an ATDevice to interface with the Skywire modem. */
	ATDevice dev;
	dev.api.count = skywire_count;
	dev.api.getc = skywire_getc;
	dev.api.write = skywire_write;
	dev.buffer = buffer;
	dev.length = 512;

	/* Initialize the peripherals and state for this task. */
	if (!skywire_task_setup(&dev)) {
		trace_printf("skywire_task: setup failed\n");
		vTaskDelete(NULL);
		return;
	} else {
		trace_printf("skywire_task: started\n");
	}

	for (;;) {
		/* Wait for exclusive access to the SPI bus. */
		while (!spi_take()) {
			vTaskDelay(100);
		}

		/* Get a manifest of files to POST to the server. */
		Attachment* manifest;
		uint8_t deleteFiles = 0;
		if (get_manifest(&manifest)) {
			/* POST the files in the manifest to the server. */
			if (post_manifest(&dev, manifest)) {
				/* Parse the HTTP response from the server. */
				SLUpdate slUpdate;
				if (parse_response(&dev, &slUpdate.limit)) {
					/* Server returned 200 OK - safe to delete the local data. */
					deleteFiles = 1;

					/* Post the updated speed limit to the beacon task. */
					xQueueSend(xSLUpdatesQueue, (void*)&slUpdate, 0);
				}
			}
		}

		/* Delete all of the files POSTed to the server */
		free_manifest(manifest, deleteFiles);

		/* Release exclusive access to the SPI bus. */
		spi_give();

		// Sleep for 1 minute.
		vTaskDelay(30000);
	}

}
