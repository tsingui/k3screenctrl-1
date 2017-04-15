#include "frame_tx.h"
#include "common.h"
#include "serial_port.h"
#include "frame_tx.h"
#include "gpio.h"
#include "mem_util.h"
#include "mcu_proto.h"
#include "handlers.h"

#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <getopt.h>

/*
 * Detected on rising edge of RESET GPIO.
 * Low = Run app from ROM
 * High = Enter download mode and wait for a new app
 */
#define SCREEN_BOOT_MODE_GPIO 7

/*
 * Resets the screen on rising edge
 */
#define SCREEN_RESET_GPIO 8

static void print_buf(const unsigned char* buf, int len) {
    printf("RCVD %d bytes\n", len);

    for (int i=0;i<len;i++) {
        printf("0x%hhx ", buf[i]);
    }
    printf("\n");
};

static void frame_handler(const unsigned char* frame, int len) {
    if (frame[0] != PAYLOAD_HEADER) {
        syslog(LOG_WARNING, "frame with unknown type received: %hhx\n", frame[0]);
        return;
    }

    extern RESPONSE_HANDLER g_response_handlers[];
    for (RESPONSE_HANDLER* handler = &g_response_handlers[0]; handler != NULL; handler++) {
        if (handler->type == frame[1]) {
            handler->handler(frame + 2, len - 2); /* Start from payload content */
            return;
        }
    }

    syslog(LOG_WARNING, "frame with unknown response type received: %hhx\n", frame[1]);
    print_buf(frame, len);
}

static int screen_initialize(int skip_reset) {
    mask_memory_byte(0x1800c1c1, 0xf0, 0); /* Enable UART2 in DMU */

    if (!skip_reset) {
        if (gpio_export(SCREEN_BOOT_MODE_GPIO) == FAILURE ||
            gpio_export(SCREEN_RESET_GPIO) == FAILURE) {
            syslog(LOG_ERR, "Could not export GPIOs\n");
            return FAILURE;
        }

        if (gpio_set_value(SCREEN_BOOT_MODE_GPIO, 0) == FAILURE ||
            gpio_set_value(SCREEN_RESET_GPIO, 0) == FAILURE ||
            gpio_set_value(SCREEN_RESET_GPIO, 1) == FAILURE) {
            syslog(LOG_ERR, "Could not reset screen\n");
            return FAILURE;
        }
    }

    if (serial_setup("/dev/ttyS1") == FAILURE) {
        syslog(LOG_ERR, "Could not setup serial transport\n");
        return FAILURE;
    }

    return SUCCESS;
}

int main() {
    const unsigned char data[] = {0x30, 0x01};
    const unsigned char d2[] = {0x30, 0x09, 0x4b, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x56, 0x32, 0x31, 0x2e, 0x34, 0x2e, 0x33, 0x33, 0x2e, 0x32, 0x31, 0x37, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x38, 0x3a, 0x43, 0x38, 0x3a, 0x45, 0x39, 0x3a, 0x46, 0x45, 0x3a, 0x43, 0x36, 0x3a, 0x36, 0x43, 0x00};
    const unsigned char d3[] = {0x30, 0x07, 0x01, 0x00, 0x00, 0x00, 0x40, 0x50, 0x48, 0x49, 0x43, 0x4f, 0x4d, 0x4d, 0x5f, 0x36, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x50, 0x48, 0x49, 0x43, 0x4f, 0x4d, 0x4d, 0x5f, 0x36, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x50, 0x48, 0x49, 0x43, 0x4f, 0x4d, 0x4d, 0x5f, 0x47, 0x75, 0x65, 0x73, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    const unsigned char d35[] = {0x30, 0x06, 0x01, 0x00, 0x00, 0x00, 0x40, 0x4c, 0xe3, 0x0d, 0x80, 0x5a, 0xb2, 0x27};
    const unsigned char d4[] = {0x30, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    const unsigned char d5[] = {0x30, 0x05, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
    const unsigned char d6[] = {0x30, 0x04, 0x03, 0x00, 0x00, 0x00};
    static struct option long_options[] = {
        {"skip-reset", no_argument, NULL, 's'},
        {"host-script", required_argument, NULL, 0},
        {"wifi-script", required_argument, NULL, 0},
        {"switchport-script", required_argument, NULL, 0},
        {"usb-script", required_argument, NULL, 0},
        {"wan-script", required_argument, NULL, 0},
        {"basic-info-script", required_argument, NULL, 0},
        {0, 0, 0, 0}
    };

    if (screen_initialize(1) == FAILURE) {
        return -EIO;
    }

    serial_set_pollin_callback(frame_notify_serial_recv);
    frame_set_received_callback(frame_handler);
    frame_send(data, sizeof(data));
    frame_send(d2, sizeof(d2));
    frame_send(d3, sizeof(d3));
    frame_send(d35, sizeof(d35));
    frame_send(d4, sizeof(d4));
    frame_send(d5, sizeof(d5));
    frame_send(d6, sizeof(d6));
    serial_start_poll_loop();
}
