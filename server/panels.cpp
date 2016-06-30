#include <stdexcept>
#include <iostream>
#include <memory>
#include <chrono>
#include <string.h>
#include <thread>
#include <cstdint>
#include <vector>
#include <evhttp.h>
#include "gason.h"

//Can
#include <linux/can.h>
#include <linux/can/raw.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

//MQTT
#include <stdio.h>
#include <mosquitto.h>

uint16_t to565( uint32_t );
inline void send_panel ( uint8_t , uint32_t *);
void mqtt_propagate ( uint8_t, uint8_t *, uint32_t *, uint8_t );
inline void mqtt_send(char *, uint16_t );

class tile {
	public:
		tile( uint8_t,uint8_t,uint8_t);
		tile ( uint8_t);
		tile ( );
		uint8_t panel[16][3] = {0};
		bool white;
		uint8_t orientation;
		void set_pixel(uint8_t, uint8_t,uint8_t, uint8_t,uint8_t);
		void set_panel(uint8_t (*)[16][3]);
		void set_panel(uint32_t (*)[16]);
		uint8_t * compare(uint32_t *);
		void set_panel(uint32_t * rgb);
};

void tile::set_pixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b ) {
	panel[x*4+y][0] = r;
	panel[x*4+y][1] = g;
	panel[x*4+y][2] = b;
}

void tile::set_panel(uint8_t (*rgb)[16][3]){
	for ( int i = 0; i<16; i ++){
		panel[i][0] = *rgb[i][0];
		panel[i][1] = *rgb[i][1];
		panel[i][2] = *rgb[i][2];
	}
}

void tile::set_panel(uint32_t (*rgb)[16]){
	for ( int i = 0; i<16; i ++){
		panel[i][0] = (*rgb[i] & 0xFF0000) >> 16;
		panel[i][1] = (*rgb[i] & 0xFF00) >> 8;
		panel[i][2] = (*rgb[i] & 0xFF);
	}
}

void tile::set_panel(uint32_t * rgb){
	for ( int i = 0; i<16; i ++){
		panel[i][0] = (rgb[i] & 0xFF0000) >> 16;
		panel[i][1] = (rgb[i] & 0xFF00) >> 8;
		panel[i][2] = (rgb[i] & 0xFF);
	}
}

tile::tile ( uint8_t r,uint8_t g,uint8_t b){
	white = false;
	for ( uint8_t i = 0 ; i < 16 ; i++){
		panel[i][0] = r;
		panel[i][1] = g;
		panel[i][2] = b;
	}
}

uint8_t * tile::compare ( uint32_t * input ){
	static uint8_t a [17];
	uint8_t p = 0;
	for ( uint8_t i = 1; i<16; i++){
		if (   (input[i] & 0xFF) != panel[i][0] 
			 || input[i] & 0xFF00 >> 8 != panel[i][1]
			 || input[i] & 0xFF0000 >> 16 != panel[i][2])
			a[p++] = i;
	}
	a[0] = p;
	return p == 0 ? NULL : a; //probably faulty
}

tile::tile ( uint8_t w){
	white = true;
}

tile::tile ( ){
	//fun stuff
}

bool MQTT_STARTED = false;
bool CAN_STARTED = false;
struct mosquitto * mosq = NULL;
tile ceiling[64];
int can_socket;
//tile ceiling[64];

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

uint8_t can_send_pixel4( uint8_t update_mode, uint8_t panel_address, uint32_t *rgb, uint8_t* pixel_address){
	uint8_t nbytes;
	struct can_frame frame;

	frame.can_id  = panel_address << 3 + update_mode + (pixel_address[0] << 25) + (pixel_address[1] << 21) + (pixel_address[2] << 17) + pixel_address[3] << 13;
	frame.can_dlc = 2;
	for ( uint8_t i = 0; i< 4; i++){
		uint16_t p = to565(rgb[i]);
		frame.data[i*2] = p >> 8;
		frame.data[i*2+1] = p & 0xFF; 
	}

	//
	// Mögliche Fehler Quelle: send statt write !!
	// 
	//

	nbytes = send(can_socket, &frame, sizeof(struct can_frame),0);

	printf("Wrote %d bytes on CAN Bus\n", nbytes);
	
	return 0;

}

uint16_t to565( uint32_t i){
	return ((i & 0xF80000) >> 8 ) + ((i & 0xFC00) >> 5 ) + (i & 0xFF >> 3);  
}


// sends 1 pixel
uint8_t can_send_pixel1( uint8_t update_mode, uint8_t panel_address, uint8_t r, uint8_t g, uint8_t b, uint8_t pixel_address){
	
	uint8_t nbytes;
	struct can_frame frame;

	frame.can_id  = panel_address << 3 + update_mode + (pixel_address << 25) + (pixel_address << 21) + (pixel_address << 17) + (r << 9 );
	frame.can_dlc = 2;
	frame.data[0] = b;
	frame.data[1] = g;
	//
	// Mögliche Fehler Quelle: send statt write !!
	// 
	//

	nbytes = send(can_socket, &frame, sizeof(struct can_frame),0);

	printf("Wrote %d bytes on CAN Bus\n", nbytes);
	
	return 0;
}
// sends 2 pixel
uint8_t can_send_pixel2( uint8_t update_mode, uint8_t panel_address, uint8_t *r, uint8_t* g, uint8_t* b,uint8_t* pixel_address ){
	
	uint8_t nbytes;
	struct can_frame frame;

	frame.can_id  = panel_address << 3 + update_mode + (pixel_address[0] << 25) + (pixel_address[1] << 21) + (pixel_address[1] << 17) + (r[0] << 9 );
	frame.can_dlc = 5;
	frame.data[0] = g[0];
	frame.data[1] = b[0];
	frame.data[2] = r[1];
	frame.data[3] = g[1];
	frame.data[4] = b[1];
	//frame.data[5] = r[1];
	//frame.data[6] = g[1];
	//frame.data[7] = b[1];

	//
	// Mögliche Fehler Quelle: send statt write !!
	// 
	//

	nbytes = send(can_socket, &frame, sizeof(struct can_frame),0);

	printf("Wrote %d bytes on CAN Bus\n", nbytes);
	
	return 0;
}
// guess what ..
uint8_t can_send_pixel3( uint8_t update_mode, uint8_t panel_address, uint8_t* pixel_address, uint8_t* r, uint8_t* g, uint8_t* b ){
	
	uint8_t nbytes;
	struct can_frame frame;

	frame.can_id  = r[0] + g[0]*0x100 + b[0]*0x10000 + ( pixel_address[0] )*0x1000000;
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
// sends n pixel
uint8_t can_send_pixeln( uint8_t update_mode, uint8_t panel_address, uint32_t *rgb, uint8_t *pixel_address, size_t n){
	uint8_t r[n];
	uint8_t g[n];
	uint8_t b[n];
	for ( uint8_t i = 0; i<n; i++){
		r[i] = rgb[i] & 0xFF0000 >> 16;
		g[i] = rgb[i] & 0xFF00 >> 8;
		b[i] = rgb[i] & 0xFF,rgb[1] & 0xFF;
	}
	switch (n){
		case 1:
			return can_send_pixel1( update_mode, panel_address, rgb[0] & 0xFF0000 >> 16, rgb[0] & 0xFF00 >> 8, rgb[0] & 0xFF, pixel_address[0]);

		case 2:
			can_send_pixel2( update_mode, panel_address, r, g, b, pixel_address);
			break;
		case 3:
			can_send_pixel3( update_mode, 	panel_address, r, g, b, pixel_address);
			break;
	}
	return 1;
}

//
// called by api 
// issued from a web request
// update 1 pixel with panel id and x,y coord on panel
//  
char * update_pixel(uint8_t id, uint8_t x, uint8_t y, char *buffer ){
	
	// values for gason library 
	char * end = buffer + strlen(buffer);
	JsonValue value;
	JsonAllocator allocator;

	// parsing values to store
	uint32_t store[3];
		
	// actually parsing 
	uint8_t status = jsonParse(buffer, &end, &value, allocator);	
	if (status != JSON_OK && value.getTag() != JSON_ARRAY) {
    	fprintf(stderr, "%s at %zd\n", jsonStrError(status), end - buffer);
    	return NULL;
	}
	uint8_t j = 0;
	for (auto i : value) {
		store[j] = uint8_t(i->value.toNumber());
		j++;
    }

    // needed for propagating values
    uint8_t addr = y*4+x;
    uint32_t s = store[0] + (store[1] << 8) + (store[2] << 16);
    
    printf("Pixel Update at id: %i at ( %i,%i ) with (r: %i, g: %i, b: %i)\n",id,x,y,store[0],store[1],store[2]);
    
    // check boundaries before updating ( or segfault )
    if ( x< 4 && y < 4 && id < 64){
    	can_send_pixel1(4,id,store[0],store[1], store[2], y*4+x);
    	mqtt_propagate(id,&addr,&s,1 );
    	ceiling[id].set_pixel(x,y,store[0],store[1],store[2]);
    }
}


//
// update a whole panel 
// calls send_panel 
//
char * update_panel(uint8_t id, char *buffer ){
	
	char * end = buffer + strlen(buffer);
	JsonValue value;
	JsonAllocator allocator;

	uint32_t store[16];
	uint8_t status = jsonParse(buffer, &end, &value, allocator);	
	
	if (status != JSON_OK && value.getTag() != JSON_ARRAY) {
    	fprintf(stderr, "%s at %zd\n", jsonStrError(status), end - buffer);
    	return NULL;
	}
	
	printf("Panel with id %i updated with:\n", id );
	
	uint8_t j = 0;
	for (auto i : value) {
		store[j] = uint32_t(i->value.toNumber());
		printf("(%lu)[%lu,%lu,%lu]\n", store[j],store[j] & 0xFF, (store[j] & 0xFF00) >> 8, (store[j] & 0xFF0000) >> 16);
		j++;
    }

    //Sending CAN Packets and send updates to mqtt channel

    if ( id < 64)
    	send_panel( id, store );
}

//
// sends panel updates to mqtt channel
// and CAN Bus
//
inline void send_panel ( uint8_t id, uint32_t * store){

	// p[0] contains size of array
	uint8_t * p = ceiling[id].compare(store);

	// check if CAN is initialized
	if ( CAN_STARTED && p != NULL){
		
		uint8_t m = p[0] - p[0]%4;
		uint8_t i = 1;

		// sending in most efficient packets (4pixel pp)
		for ( ; i+3 < m; i=i+4){
			// dont create temporary array if addresses are usccsessive values
			if (  (p[i+1] - p[i]) - (p[i+3] - p[i+2]) == 0)
				can_send_pixel4(4, id, &store[p[i]], &p[i]);
			else {
				uint32_t store4[4];
				for ( uint8_t j = 0; j< 4; j++)
					store4[j] = store[p[j+i]];
				can_send_pixel4(4, id, store4, &p[i]);
			}
		}
		uint32_t store3[3];
		for ( uint8_t j = 0; j< 3; j++)
			store3[j] = store[p[j+i]];
		can_send_pixeln( 4, id, store3, &p[i], p[0]%4 );
	}

	// update mqtt values with p offset
	if ( MQTT_STARTED && p != NULL){
		mqtt_propagate(id, p+sizeof(uint8_t), store, p[0]);
	}

	// update internal representation
	if ( p != NULL ){
		ceiling[id].set_panel(store);
	}
}

char * fetch_panel(uint8_t id){
	printf("Fetching Panel with id %i\n", id);
	char * buffer = (char * )malloc(sizeof(char)*300);
	memset( buffer, '\0', 150);
	sprintf(buffer, "[");
	char * p = buffer + 1; 
	for ( int i = 0; i < 15; i++ ){
		sprintf(p,"0x%06X,", ceiling[id].panel[i][0] * 0x10000 + ceiling[id].panel[i][1] * 0x100 + ceiling[id].panel[i][2]);
		p += 9;
	}
	sprintf(p,"0x%06X]", ceiling[id].panel[15][0] * 0x10000 + ceiling[id].panel[15][1] * 0x100 + ceiling[id].panel[15][2]);
	return buffer;
}

// 
// http api
// 

char * api( char * p, evhttp_request * req){
	char * ret = NULL;

	std::string s(p);
	std::string delimiter = "/";


	//Need to find a better Solution than this!
	std::string args[10];
	uint8_t i=0;
	uint8_t pos = 0;
	std::string token;
	while ((pos = s.find(delimiter)) != std::string::npos) {
    	token = s.substr(0, pos);
    	args[i] = token;
   	 	s.erase(0, pos + delimiter.length());
   	 	i++;
	}
	if ( !s.empty() ) 
		args[i] = s;
	//Seriously!

	for ( int k = 0; k < 10 ; k++){
		printf("%i: %s\n",k,args[k].c_str() );
	}

	if ( strcmp( args[1].c_str(), "api") != 0)
		return NULL;

	uint8_t len = evbuffer_get_length(evhttp_request_get_input_buffer(req));
	struct evbuffer *in_evb = evhttp_request_get_input_buffer(req);
    char data[len+1];
	evbuffer_copyout(in_evb, data, len);

	//This is also bad!
	if ( strncmp( args[2].c_str(),"set", 3) == 0){
		if ( args[3].compare("panel") == 0 ){
			ret = update_panel(atoi(args[4].c_str()), data);
		} else if ( args[3].compare("pixel") == 0){
			ret = update_pixel(atoi( args[4].c_str()),atoi( args[5].c_str()),atoi( args[6].c_str()), data);
		}
	} else if ( strncmp(args[2].c_str(), "get", 3 ) == 0){
		if ( strncmp(args[3].c_str(),"panel",5) == 0 ){
			ret = fetch_panel(atoi ( args[4].c_str() ) );
		}
	} 
	return ret;
}

//
// handler function for mqtt
// called on MQTT connect event
// subscribes to channel
//

void on_connect(struct mosquitto *mosq, void *userdata, int result){
	if(!result){
		mosquitto_subscribe(mosq, NULL, "#", 2);
	}else{
		fprintf(stderr, "Connect failed\n");
	}
}

//
// parse json from mqtt channel
// calls send_panel
//

void json_parse ( char * buffer){
	char * end = buffer + strlen(buffer);
	JsonValue value;
	JsonValue *n;
	JsonAllocator allocator;

	uint8_t status = jsonParse(buffer, &end, &value, allocator);	
	if (status != JSON_OK && value.getTag() != JSON_OBJECT) {
    	fprintf(stderr, "%s at %zd\n", jsonStrError(status), end - buffer);
	}

	uint32_t store[16];
	uint8_t index[16];
	uint8_t x,y;
	uint8_t count = 0;
	for (auto i : value) {
		if ( *(i -> key) == 'x' )
			x = i -> value.toNumber();
    	if ( *(i -> key) == 'y' )
			y = i -> value.toNumber();
		if ( *(i -> key) == 'p' ){
			//n = i -> value.toNode();
			// only works with modified gason library
			for ( auto j : i -> value ){
				store[atoi(j->key)] = (uint32_t) j->value.toNumber();
				index[count++] = atoi(j->key); 
			}
    	}
	}

	send_panel( y*8+x, store );
}

//
// propagates update through mqtt
//

void mqtt_propagate ( uint8_t id, uint8_t *addr, uint32_t * store, uint8_t n){

	char out[205];
	sprintf(out, "items:{\"x\":%i\",y\":%i\",pixels\":[", id%8, id/8);
	if ( n == 16 ){
		for (int i = 0; i< 16 ; i++ ){
			sprintf(out, "%i:\"%06X\",", i, store[i]);
		} 
	} else {
		for ( int i = 0; i< n; i++){
			sprintf(out, "%i:\"%06X\",", addr[i], store[i]);
		}
	}
	sprintf(out, "]}");
	mqtt_send( out, 11*n+27+2);
	
}

// 
// helper function for sending stuff
//

inline void mqtt_send(char * buffer, uint16_t len ){
	int mid = 0;
	if ( MQTT_STARTED )
		mosquitto_publish( 	mosq, &mid, "#", len, buffer, 0, false ); 	 
}

// 
// handler function for mqtt
//

void on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message){
	
	if ( message->payloadlen)
		json_parse((char *)userdata);
}

int main(){
	
	// HTTP api
	char const SrvAddress[] = "0.0.0.0";
	std::uint16_t const SrvPort = 5555;
	int const SrvThreadCount = 4;

	// CAN initialization
	can_socket = can_init();
	if ( can_socket > -1 )
		CAN_STARTED = true;
	
	// MQTT initialization
	mosquitto_lib_init();
	mosq = mosquitto_new(NULL, true, NULL);
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_message_callback_set(mosq, on_message);
	
	if( mosquitto_connect(mosq, "docker.chaospott.de", 9001, 60) ){
		printf("Unable to connect to MQTT f00\n");
	} else {
		MQTT_STARTED = true;
		mosquitto_loop_start(mosq);
	}

	//
	// handler function for evhttp server
	// calls api()
	//
	void (*OnRequest)(evhttp_request *, void *) = [] (evhttp_request *req, void *)
	{
		char * ret;
		auto *OutBuf = evhttp_request_get_output_buffer(req);
		if (!OutBuf)
			return;
		char *path = req->uri;
		if ( ( ret = api(path, req) ) != NULL ){
			evbuffer_add(OutBuf, ret, 150);
		} else {
			evbuffer_add_printf(OutBuf, "Ok");
		}
		evhttp_send_reply(req, HTTP_OK, "", OutBuf);
		if ( ret != NULL)
			free(ret);
	};

	std::exception_ptr InitExcept;
	bool volatile IsRun = true;
	evutil_socket_t Socket = -1;
	auto ThreadFunc = [&] ()
	{
		try
		{
			std::unique_ptr<event_base, decltype(&event_base_free)> EventBase(event_base_new(), &event_base_free);
			if (!EventBase)
				throw std::runtime_error("Failed to create new base_event.");
			std::unique_ptr<evhttp, decltype(&evhttp_free)> EvHttp(evhttp_new(EventBase.get()), &evhttp_free);
			if (!EvHttp)
				throw std::runtime_error("Failed to create new evhttp.");
				evhttp_set_gencb(EvHttp.get(), OnRequest, nullptr);
			if (Socket == -1)
			{
				auto *BoundSock = evhttp_bind_socket_with_handle(EvHttp.get(), SrvAddress, SrvPort);
				if (!BoundSock)
					throw std::runtime_error("Failed to bind server socket.");
				if ((Socket = evhttp_bound_socket_get_fd(BoundSock)) == -1)
					throw std::runtime_error("Failed to get server socket for next instance.");
			}
			else
			{
				if (evhttp_accept_socket(EvHttp.get(), Socket) == -1)
					throw std::runtime_error("Failed to bind server socket for new instance.");
			}
			for ( ; IsRun ; )
			{
				event_base_loop(EventBase.get(), EVLOOP_NONBLOCK);
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
		catch (...)
		{
			InitExcept = std::current_exception();
		}
	};
	auto ThreadDeleter = [&] (std::thread *t) { IsRun = false; t->join(); delete t; };
	typedef std::unique_ptr<std::thread, decltype(ThreadDeleter)> ThreadPtr;
	typedef std::vector<ThreadPtr> ThreadPool;
	ThreadPool Threads;
	for (int i = 0 ; i < SrvThreadCount ; ++i)
	{
		ThreadPtr Thread(new std::thread(ThreadFunc), ThreadDeleter);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		if (InitExcept != std::exception_ptr())
		{
			IsRun = false;
			std::rethrow_exception(InitExcept);
		}
		Threads.push_back(std::move(Thread));
	}
	std::cout << "Press Enter to quit." << std::endl;
	std::cin.get();
	IsRun = false;
	
	return 0;
}
