/**
 *
 * Riech-O-Mat Backend for the National Instruments NI USB-6501
 *
 * Based on the NI USB-6501 library by Marc Schütz
 * https://github.com/schuetzm/ni-usb-6501
 *
 * Switches device to all-output mode, then
 * sets p0.0, p1.0, p1.1, ..., p1.7 to given bit string.
 *
 * Usage:
 * ./X 011110000
 *
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <usb.h>

#define VENDOR_ID 0x3923
#define PRODUCT_ID 0x718a

#define EP_IN 0x81
#define EP_OUT 0x01

#define TIMEOUT 1000

#define PACKET_HEADER_LEN 4
#define DATA_HEADER_LEN 4

#define PROTOCOL_ERROR (-EPROTO)
#define BUFFER_TOO_SMALL (-ENOSPC)


int niusb6501_is_success(size_t len, const void* buffer) {
	static const char success_packet[8] = "\x00\x08\x01\x00\x00\x00\x00\x02";

	if (len != sizeof success_packet) return 0;
	return !memcmp(buffer, success_packet, sizeof success_packet);
}


int niusb6501_send_request(struct usb_dev_handle* usb_handle, uint8_t cmd, size_t request_len, const void* request, size_t* result_len, void* result) {
	uint8_t buffer[256];
	int status;

	if (request_len > 255-(PACKET_HEADER_LEN + DATA_HEADER_LEN)) {
		fprintf(stderr, "request too long (%lu > %d bytes)\n", request_len, 255-(PACKET_HEADER_LEN + DATA_HEADER_LEN));
		return -EINVAL;
	}

	buffer[0] = 0x00;
	buffer[1] = 0x01;
	buffer[2] = 0x00;
	buffer[3] = request_len + PACKET_HEADER_LEN + DATA_HEADER_LEN;
	buffer[4] = 0x00;
	buffer[5] = request_len + DATA_HEADER_LEN;
	buffer[6] = 0x01;
	buffer[7] = cmd;

	memcpy(&buffer[PACKET_HEADER_LEN + DATA_HEADER_LEN], request, request_len);

	status = usb_bulk_write(usb_handle, EP_OUT, buffer, request_len + PACKET_HEADER_LEN + DATA_HEADER_LEN, TIMEOUT);
	if (status < 0) return status;

	status = usb_bulk_read(usb_handle, EP_IN, buffer, sizeof buffer, TIMEOUT);
	if (status < 0) return status;
	if (status < PACKET_HEADER_LEN) return PROTOCOL_ERROR;
	if (status-PACKET_HEADER_LEN > *result_len) return BUFFER_TOO_SMALL;
	*result_len = status-PACKET_HEADER_LEN;
	memcpy(result, &buffer[PACKET_HEADER_LEN], status-PACKET_HEADER_LEN);

	return 0;
}

/*
 * Returns info on up to [size] NI USB-6501 devices
 */
size_t niusb6501_list_devices(struct usb_device* devices[], size_t size) {
	struct usb_device* usb_dev;
	struct usb_bus* usb_bus;
	size_t count = 0;

	usb_init();
	usb_find_busses();
	usb_find_devices();

	for (usb_bus = usb_busses; usb_bus; usb_bus = usb_bus->next) {
		for (usb_dev = usb_bus->devices; usb_dev; usb_dev = usb_dev->next) {
			if ((usb_dev->descriptor.idVendor == VENDOR_ID) && (usb_dev->descriptor.idProduct == PRODUCT_ID)) {
				if (count < size) devices[count++] = usb_dev;
			}
		}
	}

	return count;
}


struct usb_dev_handle* niusb6501_open_device(struct usb_device* device) {
	return usb_open(device);
}


int niusb6501_close_device(struct usb_dev_handle* handle) {
	return usb_close(handle);
}


int niusb6501_write_port(struct usb_dev_handle* handle, uint8_t port, uint8_t value) {
	uint8_t result[8];
	size_t result_len = sizeof result;
	uint8_t request[12] = "\x02\x10\x00\x00\x00\x03\x00\x00\x03\x00\x00\x00";
	int status;

	request[6] = port;
	request[9] = value;

	status = niusb6501_send_request(handle, 0x0f, sizeof request, request, &result_len, result);
	if (status < 0) return status;
	if (!niusb6501_is_success(result_len, result)) return PROTOCOL_ERROR;
	return 0;
}


/*
 * Sets the I/O mode for the ports (0 bit -> input / 1 bit -> output).
 */
int niusb6501_set_io_mode(struct usb_dev_handle* handle, uint8_t port0, uint8_t port1, uint8_t port2) {
	uint8_t result[8];
	size_t result_len = sizeof result;
	uint8_t request[16] = "\x02\x10\x00\x00\x00\x05\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00";
	int status;

	request[6] = port0;
	request[7] = port1;
	request[8] = port2;

	status = niusb6501_send_request(handle, 0x12, sizeof request, request, &result_len, result);
	if (status < 0) return status;
	if (!niusb6501_is_success(result_len, result)) return PROTOCOL_ERROR;
	return 0;
}


int main(int argc, char** argv) {
	struct usb_device* dev;
	struct usb_dev_handle* handle;

	/* open first matching device */
	if (niusb6501_list_devices(&dev, 1) != 1) {
		fprintf(stderr, "Device not found\n");
		return ENODEV;
	}
	handle = niusb6501_open_device(dev);
	if (handle == NULL) {
		fprintf(stderr, "Unable to open the USB device: %s\n", strerror(errno));
		return errno;
	}

	/* Read '1's and '0's from command line arg; the bits read are: p0.0, p1.0, p1.1, ... p1.7 */
	uint8_t p0 = 0x00;
	uint8_t p1 = 0x00;
	char* cp = argv[1];
	int bitpos = 0;
	uint8_t bit = ((*cp++ == '1') ? 1 : 0);
	p0 = p0 ^ bit;
	while (*cp) {
		uint8_t bit = ((*cp++ == '1') ? 1 : 0);
		p1 = p1 ^ (bit << bitpos++);
	}

	/* switch to all-output */
	niusb6501_set_io_mode(handle, 0xff, 0xff, 0xff);

	/* set ports 0 and 1 */
	int status;
	status = niusb6501_write_port(handle, 0, p0);
	if (status) {
		fprintf(stderr, "error writing to port 0: %s\n", strerror(-status));
		return 1;
	}
	status = niusb6501_write_port(handle, 1, p1);
	if (status) {
		fprintf(stderr, "error writing to port 1: %s\n", strerror(-status));
		return 1;
	}

	/* done */
	niusb6501_close_device(handle);

	return 0;
}
