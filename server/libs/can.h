#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <stdexcept>
#include <iostream>

extern int can_socket;

uint16_t to565( uint32_t );
int can_init();
uint8_t can_send_pixel4( uint8_t , uint8_t , uint32_t *, uint8_t* );
uint8_t can_send_pixel1( uint8_t , uint8_t , uint8_t , uint8_t , uint8_t , uint8_t );
uint8_t can_send_pixel2( uint8_t , uint8_t , uint8_t *, uint8_t* , uint8_t* ,uint8_t* );
uint8_t can_send_pixel3( uint8_t , uint8_t , uint8_t* , uint8_t* , uint8_t* , uint8_t*  );
uint8_t can_send_pixeln( uint8_t , uint8_t, uint32_t *, uint8_t *, size_t );
void can_send_white(uint8_t , uint8_t , uint8_t, int);

