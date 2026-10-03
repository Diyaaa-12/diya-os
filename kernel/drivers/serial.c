#include <stdint.h>
#include "serial.h"

#define COM1 0x3F8

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void serial_init(void)
{
    outb(COM1 + 1, 0x00); /* disable UART interrupts — we're not handling them yet */
    outb(COM1 + 3, 0x80); /* enable DLAB (Divisor Latch Access Bit) to set baud rate */
    outb(COM1 + 0, 0x03); /* divisor low byte  -> 38400 baud */
    outb(COM1 + 1, 0x00); /* divisor high byte */
    outb(COM1 + 3, 0x03); /* 8 bits, no parity, one stop bit; also clears DLAB */
    outb(COM1 + 2, 0xC7); /* enable FIFO, clear it, 14-byte threshold */
    outb(COM1 + 4, 0x0B); /* IRQs disabled for now, RTS/DSR set (required for the UART to transmit) */
}

static int transmit_empty(void)
{
    /* Line Status Register, bit 5 = transmitter holding register empty */
    return inb(COM1 + 5) & 0x20;
}

void serial_write_char(char c)
{
    while (!transmit_empty()) {
        /* wait until the UART is ready for the next byte */
    }
    outb(COM1, (uint8_t)c);
}

void serial_write(const char *str)
{
    while (*str) {
        if (*str == '\n') {
            serial_write_char('\r'); /* terminals expect CRLF, not bare LF */
        }
        serial_write_char(*str);
        str++;
    }
}
void serial_write_hex(uint64_t value)
{
    const char *digits = "0123456789abcdef";
    char buf[17];
    buf[16] = '\0';

    for (int i = 15; i >= 0; i--) {
        buf[i] = digits[value & 0xF];
        value >>= 4;
    }

    serial_write(buf);
}