//
// Created by david on 4/29/21.
//

#ifndef _SIO_H_
#define _SIO_H_
#include <stdint.h>

int sio_init( char * device);
int sio_write(int fd, const uint8_t *buf, size_t size);
int sio_read(int fd, uint8_t *buf, size_t size);

#endif //_SIO_H_
