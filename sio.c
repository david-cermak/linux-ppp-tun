#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include "errno.h"
#include <stdint.h>
#include <unistd.h>

int sio_init( char * device)
{
    struct termios tty;
    int fd = open( device, O_RDWR | O_NOCTTY | O_NONBLOCK );
    if (fd == -1) {
        printf("Failed to open serial port %s\n", device);
        printf("errno:%i :%s\n", errno, strerror(errno));
        return -1;
    }
    if (tcgetattr(fd, &tty) != 0) {
        printf("errno:%i :%s\n", errno, strerror(errno));
        return -1;
    }
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE; // Clear all the size bits, then use one of the statements below
    tty.c_cflag |= CS8; // 8 bits per byte (most common)
    tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
    tty.c_cc[VTIME] = 0;
    tty.c_cc[VMIN] = 0;
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    cfsetspeed(&tty, B115200);
    ioctl(fd, TCSETS, &tty);
    return fd;
}

int sio_write(int fd, const uint8_t *buf, size_t size)
{
    return write(fd, buf, size);
}

int sio_read(int fd, uint8_t *buf, size_t size)
{
    return read(fd, buf, size);
}
