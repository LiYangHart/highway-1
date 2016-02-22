#include <peripheral/skywire.h>
#include <peripheral/virtual_com.h>
#include <peripheral/i2c_spi_bus.h>
#include <task/skywire_task.h>
#include <string.h>
#include <fat_sl.h>

char buffer[514];

/**
 * Tokenize a response buffer which is delimited by \r\n.
 *
 * This method modifies the input buffer by inserting \0 at the delimiter.
 */
char*
tokenize_res(ResConfig* res, char* prev_token) {
	char *c, *d;
	char *end;

	// Note the end of the buffer.
	end = res->Buffer + res->Length - 1;

	// Seek to the end of the current token.
	if (prev_token == NULL) {
		c = res->Buffer;
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

uint8_t
activate_pdp_context(ResConfig* res) {
	/**
	 * Check the state of PDP context 1.
	 */
	if (   skywire_at("AT#SGACT?\r\n")                != SKYWIRE_OK
		|| skywire_res(res, pred_ends_with, "OK\r\n") != SKYWIRE_OK) {
		return 0;
	} else {
		uint8_t cid, stat;
		char* line = NULL;
		while ((line = tokenize_res(res, line)) != NULL) {
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
	if (   skywire_at("AT+CGDCONT=1,\"IP\",\"internet.com\"\r\n") != SKYWIRE_OK
		|| skywire_res(res, pred_ends_with, "OK\r\n")             != SKYWIRE_OK
		|| skywire_at("AT#SGACT=1,1\r\n")                         != SKYWIRE_OK
		|| skywire_res(res, pred_ends_with, "OK\r\n")             != SKYWIRE_OK) {
		return 0;
	}

	return 1;
}

/**
 * Initialize the peripherals and state for this task.
 */
uint8_t
skywire_task_setup(ResConfig* res) {
	vcp_init();
	skywire_init();

	/* Execute the activation sequence - this takes about 15 seconds but is
	   required to enable the Skywire modem. */
	skywire_activate();

	/**
	 * Disable echo.
	 * Disable flow control.
	 */
	if (   skywire_at("ATE0\r\n")                     != SKYWIRE_OK
		|| skywire_res(res, pred_ends_with, "OK\r\n") != SKYWIRE_OK
		|| skywire_at("AT&K=0\r\n")                   != SKYWIRE_OK
		|| skywire_res(res, pred_ends_with, "OK\r\n") != SKYWIRE_OK) {
		trace_printf("skywire_task: stage1 failed\n");
		return 0;
	}

	if (!activate_pdp_context(res)) {
		trace_printf("skywire_task: failed to activate PDP 1\n");
		return 0;
	}

	return 1; // OK
}

/**
 * Enter a terminal mode to interact with the Skywire modem directly.
 */
void
skywire_terminal() {
	char c;

	// Enter terminal mode if VCP receives an ESC key.
	if (vcp_count() > 0 && (c = vcp_getc()) == '\033') {
		trace_printf("skywire_task: entering terminal mode\n");

		for (;;) {
			/*
			uint8_t b;
			while () {
				b = vcp_getc();

				// Echo characters input to the terminal.
				HAL_UART_Transmit(vcp_handle(), &b, 1, 1000);

				if (b == '\n') {
					b = '\r';
					HAL_UART_Transmit(skywire_handle(), &b, 1, 0);
					b = '\n';
					HAL_UART_Transmit(skywire_handle(), &b, 1, 0);
				} else {
					HAL_UART_Transmit(skywire_handle(), &b, 1, 0);
				}
			}

			while(skywire_count() > 0) {
				b = skywire_getc();
				HAL_UART_Transmit(vcp_handle(), &b, 1, 1000);
			}

			vTaskDelay(1);
			*/
		}
	}
}

/**
 * Construct a manifest of files to be sent to the server.
 */
Manifest*
get_manifest() {
	char* buffer;
	uint8_t i = 0;

	Manifest* head = NULL;
	Manifest* node = NULL;

	/* Collect all of the new images. */
	F_FIND xFindStruct;
	if (f_findfirst("CAP*.JPG", &xFindStruct) == F_NO_ERROR) {
		do {
			Manifest* entry = (Manifest*)pvPortMalloc(sizeof(Manifest));
			strncpy(entry->Name, xFindStruct.filename, 64);
			entry->Size = xFindStruct.filesize;
			entry->Next = NULL;
			trace_printf("found: %s\n", entry->Name);

			if (head == NULL) {
				head = entry;
				node = entry;
			} else {
				node->Next = entry;
				node = entry;
			}

			// Send up to 5 images.
			if (++i >= 2) {
				break;
			}
		} while(f_findnext( &xFindStruct ) == F_NO_ERROR);
	}

	return head;
}

void
free_manifest(Manifest* manifest) {
	Manifest* next = NULL;
	while (manifest != NULL) {
		next = manifest->Next;

		if (f_delete(manifest->Name) == F_NO_ERROR) {
			trace_printf("deleting %s\n", manifest->Name);
		}

		vPortFree(manifest);
		manifest = next;
	}
}

/**
 * Use a POST to upload the manifest to the server.
 */
void
post_manifest(ResConfig* res, Manifest* manifest) {
	if (   skywire_at("AT#SD=1,0,80,\"79ed7953.ngrok.com\"\r\n") != SKYWIRE_OK
		|| skywire_res(res, pred_ends_with, "CONNECT\r\n")       != SKYWIRE_OK) {
		trace_printf("http req failed\n");
	} else {
		char chunk_header[16] = { 0 }, item_header[32] = { 0 };

		while (skywire_count() > 0) {
			trace_printf("%c", skywire_getc());
		}

		// HTTP Header
		skywire_at("POST /data HTTP/1.1\r\n");
		skywire_at("Host: 79ed7953.ngrok.com\r\n");
		skywire_at("Content-Type: application/octet-stream\r\n");
		skywire_at("Transfer-Encoding: chunked\r\n");
		skywire_at("Connection: close\r\n\r\n");

		// POST Body
		while (manifest != NULL) {
			// Start of chunk.
			snprintf(chunk_header, 16, "%x\r\n", 32);
			skywire_at(chunk_header);

			// Manifest header
			snprintf(item_header, 32, "%s,%d\r\n", manifest->Name, manifest->Size);
			skywire_write(item_header, 0, 32);
			skywire_at("\r\n");
			trace_printf("manifest header: %s", item_header);

			// Write file
			F_FILE* hManifestItem = f_open(manifest->Name, "r");
			if (hManifestItem == NULL) {
				trace_printf("failed to open file");
				break;
			} else {
				/* Start of chunk. */
				snprintf(chunk_header, 16, "%x\r\n", manifest->Size);
				skywire_at(chunk_header);
				trace_printf("chunk: %s", chunk_header);

				uint32_t bytes = 0;
				while (bytes < manifest->Size) {
					uint32_t read = f_read(buffer, 1, 128, hManifestItem);
					bytes += read;
					skywire_write(buffer, 0, read);
				}
				f_close(hManifestItem);
				hManifestItem = NULL;

				/* End of chunk. */
				skywire_at("\r\n");
			}

			manifest = manifest->Next;
		}

		/* Trailing chunk and termination. */
		skywire_at("0\r\n\r\n");
		trace_printf("POST out\n");
	}
}

void
parse_response(ResConfig* res) {
	/**
	 * Expect a 200 OK response.
	 * Skip the headers by looking for "\r\n\r\n".
	 * Look for no carrier when the connection is closed.
	 */
	if (   skywire_res(res, pred_ends_with, "HTTP/1.1 200 OK\r\n") != SKYWIRE_OK
		|| skywire_res(res, pred_ends_with, "\r\n\r\n")            != SKYWIRE_OK
		|| skywire_res(res, pred_ends_with, "NO CARRIER\r\n")      != SKYWIRE_OK) {
		return;
	}

	uint8_t sl = 0;
	char* line = NULL;
	while ((line = tokenize_res(res, line)) != NULL) {
		if (sscanf(line, "SL=%d,EOM", &sl) == 1) {
			trace_printf("speed limit is %d\n", sl);
		}
	}
}

/**
 *
 */
void
skywire_task(void* pvParameters) {
	memset(buffer, '\0', 514);

	ResConfig res;
	res.Buffer = buffer;
	res.Length = 512;
	res.Timeout = 10000;

	/* Initialize the peripherals and state for this task. */
	if (!skywire_task_setup(&res)) {
		trace_printf("skywire_task: setup failed\n");
		vTaskDelete(NULL);
		return;
	} else {
		trace_printf("skywire_task: started\n");
	}

	// Enter terminal mode if the user input an ESC key while the task was
	// starting.
	skywire_terminal();

	for (;;) {
		/* Obtain exclusive access to the SD card. */
		if (xSemaphoreTake(xSpiSemaphore, 0) != pdTRUE) {
			vTaskDelay(100);
			continue;
		}

		// Get a manifest of file to send to the server.
		Manifest* manifest = get_manifest();
		if (manifest != NULL) {
			//trace_printf("got_manifest: %s\n", manifest->Name);
			post_manifest(&res, manifest);
			parse_response(&res);
		}

		free_manifest(manifest);

		xSemaphoreGive(xSpiSemaphore);

		// Sleep for 1 minute.
		vTaskDelay(30000);
	}

}
