#include "mcu.h"

#include <asm-generic/errno.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#define JUMP_HEADER         0x01
#define SCORE_HEADER        0x10
#define GAMEMODE_HEADER     0x11

static int serial_fd = -1;

/**
 * @brief Sends bytes to mcu
 * @param data Pointer to data bytes buffer
 * @param len Length of the message
 * @return Returns true if the sending was successful
 */
static bool send_bytes(const uint8_t *data, size_t len)
{
    if (serial_fd < 0 || !data || len == 0) {
        return false;
    }

    ssize_t written = write(serial_fd, data, len);
    if (written < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("MCU_Send: write fail");
        }
        return false;
    }
    return (size_t)written == len;
}

/**
 * @brief Initializes MCU UART communication
 * @return Returns true if the setup was successful
 */
bool MCU_Init(const char *device)
{
    if (serial_fd >= 0) {
        MCU_Close();
    }

    int mcu = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (mcu < 0) {
        perror("MCU_Init: Can't open port");
        return false;
    }

    struct termios tty;
    if (tcgetattr(mcu, &tty) != 0) {
        perror("MCU_Init: getattr fail");
        return false;
    }

    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    cfmakeraw(&tty);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(mcu, TCSANOW, &tty) != 0) {
        perror("MCU_Init: setattr fail");
        return false;
    }

    tcflush(mcu, TCIOFLUSH);
    serial_fd = mcu;
    return true;
}

/**
 * @brief Read the input buffer for jump message
 * @return Returns true if there was jump message
 */
bool MCU_Jump(void)
{
    if (serial_fd < 0) {
        return false;
    }

    uint8_t rx_buffer[64];
    ssize_t n = read(serial_fd, rx_buffer, sizeof(rx_buffer));

    if (n <= 0) {
        return false;
    }

    bool jump_triggered = false;
    for (ssize_t i = 0; i < n; i++) {
        if (rx_buffer[i] == JUMP_HEADER) {
            jump_triggered = true;
        }
    }

    return jump_triggered;
}

/**
 * @brief Sending score to the MCU
 * @param score Value of the score (0...9999)
 * @return Returns true if the sending was successful
 */
bool MCU_SendScore(uint16_t score)
{
    if (score > 9999) {
        score = 9999;
    }
    uint8_t packet[3] = {
        SCORE_HEADER,
        (uint8_t)((score >> 8) & 0xFF),
        (uint8_t)(score & 0xFF)
    };

    return send_bytes(packet, sizeof(packet));
}

/**
 * @brief Sending gamemode to the MCU
 * @param mode Gamemode value
 * @return Returns true if the sending was successful
 */
bool MCU_SendGamemode(Gamemode_t mode)
{
    uint8_t packet[2] = {
        GAMEMODE_HEADER,
        (uint8_t)mode
    };
    return send_bytes(packet, sizeof(packet));
}

/**
 * @brief Close the device
 */
void MCU_Close(void)
{
    if (serial_fd >= 0) {
        close(serial_fd);
        serial_fd = -1;
    }
}