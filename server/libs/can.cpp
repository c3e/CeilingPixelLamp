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

#include "can.h"

extern int can_socket;

uint16_t to565( uint32_t i){
	return ((i & 0xF80000) >> 8 ) + ((i & 0xFC00) >> 5 ) + (i & 0xFF >> 3);  
}

int can_init() {

	int s;
	struct sockaddr_can addr;
	struct ifreq ifr;

	std::string ifname("can0");

	if((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Error while opening Can Socket");
		return -1;
	}

	strcpy(ifr.ifr_name, ifname.c_str());
	ioctl(s, SIOCGIFINDEX, &ifr);
	
	addr.can_family  = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex; 

	printf("%s at index %d\n", ifname.c_str(), ifr.ifr_ifindex);

	if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Error while binding Can Socket");
		return -2;
	}

	return s;
}

// sends n pixel
uint8_t can_send_pixeln( uint8_t update_mode, uint8_t panel_address, uint32_t *rgb, uint8_t *pixel_address, size_t n){
	uint8_t r[n];
	uint8_t g[n];
	uint8_t b[n];
	for ( uint8_t i = 0; i<n; i++){
		r[i] = (rgb[i] & 0xFF0000) >> 16;
		g[i] = (rgb[i] & 0xFF00) >> 8;
		b[i] = rgb[i] & 0xFF;
	}
	switch (n){
		case 1:
			return can_send_pixel1( update_mode, panel_address, (rgb[0] & 0xFF0000) >> 16, (rgb[0] & 0xFF00) >> 8, rgb[0] & 0xFF, pixel_address[0]);

		case 2:
			can_send_pixel2( update_mode, panel_address, r, g, b, pixel_address);
			break;
		case 3:
			can_send_pixel3( update_mode, 	panel_address, r, g, b, pixel_address);
			break;
	}
	return 1;
}


uint8_t can_send_pixel4( uint8_t update_mode, uint8_t panel_address, uint32_t *rgb, uint8_t* pixel_address){
	uint8_t nbytes;
	struct can_frame frame;

	frame.can_id  = ((panel_address) << 3) + update_mode + (pixel_address[0] << 25) + (pixel_address[1] << 21) + (pixel_address[2] << 17) + (pixel_address[3] << 13) + 0x80000000;
	frame.can_dlc = 2;
	for ( uint8_t i = 0; i< 4; i++){
		uint16_t p = to565(rgb[i]);
		frame.data[i*2] = p >> 8;
		frame.data[i*2+1] = p & 0xFF; 
	}

	nbytes = send(can_socket, &frame, sizeof(struct can_frame),0);

	printf("Wrote %d bytes on CAN Bus\n", nbytes);
	
	return 0;

}

// sends 1 pixel
uint8_t can_send_pixel1( uint8_t update_mode, uint8_t panel_address, uint8_t r, uint8_t g, uint8_t b, uint8_t pixel_address){
	
	uint8_t nbytes;
	struct can_frame frame;
	memset ( frame.data, '\0', sizeof(uint8_t) * 8);

	frame.can_id  = (panel_address << 3) + update_mode + (pixel_address << 25) + (r << 9 ) + 0x80000000;
	frame.can_dlc = 2;
	frame.data[0] = b;
	frame.data[1] = g;

	printf("ID: %lu", frame.can_id);

	nbytes = send(can_socket, &frame, sizeof(struct can_frame),0);

	printf("Wrote %d bytes on CAN Bus\n", nbytes);
	
	return 0;
}
// sends 2 pixel
uint8_t can_send_pixel2( uint8_t update_mode, uint8_t panel_address, uint8_t *r, uint8_t* g, uint8_t* b,uint8_t* pixel_address ){
	
	uint8_t nbytes;
	struct can_frame frame;
	memset ( frame.data, '\0', sizeof(uint8_t) * 8);


	frame.can_id  = (panel_address << 3) + update_mode + (pixel_address[0] << 25) + (pixel_address[1] << 21) + (pixel_address[1] << 17) + (r[0] << 9 ) + + 0x80000000;
	frame.can_dlc = 5;
	frame.data[0] = g[0];
	frame.data[1] = b[0];
	frame.data[2] = r[1];
	frame.data[3] = g[1];
	frame.data[4] = b[1];

	nbytes = send(can_socket, &frame, sizeof(struct can_frame),0);

	printf("Wrote %d bytes on CAN Bus\n", nbytes);
	
	return 0;
}
// guess what ..
uint8_t can_send_pixel3( uint8_t update_mode, uint8_t panel_address, uint8_t* pixel_address, uint8_t* r, uint8_t* g, uint8_t* b ){
	
	uint8_t nbytes;
	struct can_frame frame;

	frame.can_id  = r[0] + g[0]*0x100 + b[0]*0x10000 + ( pixel_address[0] )*0x1000000 + 0x80000000;
	frame.can_dlc = 8;
	frame.data[0] = panel_address * 4 + update_mode;
	frame.data[1] = pixel_address[1] + pixel_address[2] * 0x10;
	frame.data[2] = r[1];
	frame.data[3] = g[1];
	frame.data[4] = b[1];
	frame.data[5] = r[2];
	frame.data[6] = g[2];
	frame.data[7] = b[2];
	 
	//
	// Mögliche Fehler Quelle: send statt write !!
	// 
	//

	nbytes = send(can_socket, &frame, sizeof(struct can_frame),0);

	printf("Wrote %d bytes on CAN Bus\n", nbytes);
	
	return 0;
}

void can_send_white(uint8_t id, uint8_t pwm, uint8_t activate, int orientation){
	uint8_t nbytes;
	struct can_frame frame;

	/* old 
	frame.can_id = 3;
	frame.can_dlc = 8;
	if ( ! activate == 1) 
		frame.can_id += 8; //activation sequence is a zero at bit index 4 in the id
	memset ( frame.data, '\0', sizeof(uint8_t) * 8);
	frame.data[0] = id << 2;
	frame.data[4] = pwm;
	*/

	frame.can_id = 3 + ( id << 3 ) + 0x80000000;
	if ( activate == 1 )
		frame.can_id += 0x200;
	if ( orientation >= 0 )
		frame.can_id = frame.can_id + 0x1000 + (orientation << 10);
	frame.can_id += (pwm << 13);
	frame.can_dlc = 0;


	nbytes = send(can_socket, &frame, sizeof(struct can_frame),0);

	printf("Sent White Packet!");
}