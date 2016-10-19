#include "lib/ws2812.h"

void WS2812_init()
{
    // Configure UART1
    // Set baudrate of UART1 to 3200000
    WRITE_PERI_REG(UART_CLKDIV(1), UART_CLK_FREQ / 3200000);
    // Set UART Configuration No parity / 6 DataBits / 1 StopBits / Invert TX
    WRITE_PERI_REG(UART_CONF0(1), UART_TXD_INV | (1 << UART_STOP_BIT_NUM_S) | (1 << UART_BIT_NUM_S));

    // WS2812 Power-On-Reset (GPIO2 low for 50 us)
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(2), GPIO_LOW);

    // Waits 50us to simulate a reset
    os_delay_us(50);

    // Redirect UART1 to GPIO2
    // Disable GPIO2
    GPIO_REG_WRITE(GPIO_ENABLE_W1TC_ADDRESS, BIT2);
    // Enable Function 2 for GPIO2 (U1TXD)
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);
}

void WS2812_write(const uint8 *buffer, uint8 length)
{
    // Data are sent LSB first, with a start bit at 0, an end bit at 1 and all inverted
    // 0b00110111 => 110111 => [0]111011[1] => 10001000 => 00
    // 0b00000111 => 000111 => [0]111000[1] => 10001110 => 01
    // 0b00110100 => 110100 => [0]001011[1] => 11101000 => 10
    // 0b00000100 => 000100 => [0]001000[1] => 11101110 => 11
    // Array declared as static const to avoid runtime generation
    // But declared in ".data" section to avoid read penalty from FLASH
    static const uint8_t _uartData[4] = { 0b00110111, 0b00000111, 0b00110100, 0b00000100 };

    const uint8_t *end  = buffer + length;

    do {
        // If something to send for first buffer and enough room
        // in FIFO buffer (we wants to write 4 bytes, so less than
        // 124 in the buffer)
        if (buffer < end
                && (((READ_PERI_REG(UART_STATUS(1)) >> UART_TXFIFO_CNT_S)
                        & UART_TXFIFO_CNT) <= 124)) {
            uint8_t value = *buffer++;

            // Fill the buffer
            WRITE_PERI_REG(UART_FIFO(1), _uartData[(value >> 6) & 3]);
            WRITE_PERI_REG(UART_FIFO(1), _uartData[(value >> 4) & 3]);
            WRITE_PERI_REG(UART_FIFO(1), _uartData[(value >> 2) & 3]);
            WRITE_PERI_REG(UART_FIFO(1), _uartData[(value >> 0) & 3]);
        }
    } while (buffer < end);

    os_delay_us(50);
}
