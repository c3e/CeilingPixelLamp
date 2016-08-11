#include <stdexcept>
#include <iostream>
#include <memory>
#include <chrono>
#include <string.h>
#include <thread>
#include <cstdint>
#include <vector>
#include <evhttp.h>
#include "libs/gason.h"

//Can
#include "libs/can.h"

//MQTT
#include <stdio.h>
#include <mosquitto.h>

//uint16_t to565( uint32_t );
inline void send_panel ( uint8_t , uint32_t *);
void mqtt_propagate ( uint8_t, uint8_t *, uint32_t *, uint8_t );
inline void mqtt_send(char *, uint16_t );

class tile {
	public:
		tile( uint8_t,uint8_t,uint8_t);
		tile ( uint8_t);
		tile ( );
		uint8_t panel[16][3];
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
	for ( uint8_t i = 0; i<16; i++){
		if (   (input[i] & 0xFF) != panel[i][2] 
			 || ((input[i] & 0xFF00) >> 8 ) != panel[i][1]
			 || ((input[i] & 0xFF0000) >> 16 ) != panel[i][0] ){
			a[p+1] = i;
			p++;
		}
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

uint8_t o1[16] = {3,2,1,0,4,5,6,7,11,10,9,8,12,13,14,15};
uint8_t o2[16] = {0,7,8,15,1,6,9,14,2,5,10,13,3,4,11,12};
uint8_t o3[16] = {15,14,13,12,8,9,10,11,7,6,5,4,0,1,2,3};
uint8_t o4[16] = {12,11,4,3,13,10,5,2,14,9,6,1,15,8,7,0};

uint8_t orient( uint8_t val, uint8_t o ){
	switch (o) {
		case 0:
		return o1[val];
		break;
		case 1:
		return o2[val];
		break;
		case 2:
		return o3[val];
		break;
		case 3:
		return o3[val];
		break;
	}
	return 0;
}

inline void reorder( uint8_t orientation, uint8_t * addr ){
	uint8_t * n = (uint8_t *)malloc(sizeof(uint8_t)*addr[0]);
	switch (orientation) {
		case 0: 
			for ( uint8_t i =0; i < addr[0]; i++)
				n[i] = o1[addr[i+1]];
			break;
		case 1: 
			for ( uint8_t i =0; i < addr[0]; i++)
				n[i] = o2[addr[i+1]];
			break;
		case 2: 
			for ( uint8_t i =0; i < addr[0]; i++)
				n[i] = o3[addr[i+1]];
			break;
		case 3: 
			for ( uint8_t i =0; i < addr[0]; i++)
				n[i] = o4[addr[i+1]];
			break;
	}
	//free(addr);
	addr = n;
}

char * update_white( uint8_t id, uint8_t pwm){
	can_send_white( id, pwm, 1, -1);
	printf("White id: %i, pwm: %i", id, pwm);
	return NULL;
}

char * update_on(uint8_t id){
	can_send_white(id, 0, 1, -1);
	printf("Panel %i is now ON!", id);
	return NULL;
}

char * update_off(uint8_t id){
	can_send_white(id, 0, 0, -1);
	printf("Panel %i is now OFF!", id);
	return NULL;
}


char * update_orientation ( uint8_t id, uint8_t o){
	ceiling[id].orientation = o;
	//can_send_white(id, 0, 1, o);
	return NULL;
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
	uint32_t store[3] = {0};
		
	// actually parsing 
	uint8_t status = jsonParse(buffer, &end, &value, allocator);	
	if (status != JSON_OK && value.getTag() != JSON_ARRAY) {
    	fprintf(stderr, "%s at %zd\n", jsonStrError(status), end - buffer);
    	return NULL;
	}
	uint8_t j = 0;
	for (auto i : value) {
		if (j<3)
			store[j] = uint8_t(i->value.toNumber());
		j++;
    }

    // needed for propagating values
    uint8_t addr = y*4+x;
    uint32_t s = store[0] + (store[1] << 8) + (store[2] << 16);
    
    printf("Pixel Update at id: %i at ( %i,%i ) with (r: %i, g: %i, b: %i)\n",id,x,y,store[0],store[1],store[2]);
    
    // check boundaries before updating ( or segfault )
    if ( x< 4 && y < 4 && id < 64){
    	can_send_pixel1(0,id,store[0],store[1], store[2], orient(y*4+x+1, ceiling[id].orientation));
    	mqtt_propagate(id,&addr,&s,1 );
    	ceiling[id].set_pixel(x,y,store[0],store[1],store[2]);
    }

    return NULL;
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
		if (j<16)
			store[j] = uint32_t(i->value.toNumber());
		printf("(%lu)[%lu,%lu,%lu]\n", store[j],(store[j] & 0xFF0000) >> 16, (store[j] & 0xFF00) >> 8, (store[j] & 0xFF));
		j++;
    }

    //Sending CAN Packets and send updates to mqtt channel

    if ( id < 64)
    	send_panel( id, store );

    return NULL;
}

//
// sends panel updates to mqtt channel
// and CAN Bus
//
inline void send_panel ( uint8_t id, uint32_t * store){

	// p[0] contains size of array
	// check if there are any differences to our internal data model
	uint8_t * p = ceiling[id].compare(store);

	// reorder due to individiual rotation of panels
	// takes care of length in first value
	//reorder(ceiling[id].orientation, p);

	// check if CAN is initialized
	if ( CAN_STARTED && p != NULL){
		uint32_t store4[4];
		uint8_t m = p[0] - p[0]%4;
		uint8_t i = 1;
		printf("%i values changed!\n", p[0]);
		// sending in most efficient packets (4pixel pp)
		for ( ; i+3 <= m; i=i+4){
			// dont create temporary array if addresses are succsessive values
			printf("Sent 4p\n");
			for ( uint8_t j = 0; j< 4; j++)
				store4[j] = store[p[j+i]];
			can_send_pixel4(4, id, store4, &p[i]);
		}
		
		if ( p[0] % 4 != 0){
			uint32_t store3[3];
			for ( uint8_t j = 0; j< 3; j++)
				store3[j] = store[p[j+i]];
			can_send_pixeln( 4, id, store3, &p[i], p[0]%4 );
		}
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
	while ((pos = s.find(delimiter)) != std::string::npos && i < 10) {
    	token = s.substr(0, pos);
    	args[i] = token;
   	 	s.erase(0, pos + delimiter.length());
   	 	i++;
	}
	if ( !s.empty() ) 
		args[i] = s;
	//Seriously! ie. Something that is actually safe!

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
		} else if ( args[3].compare("white") == 0){
			ret = update_white( atoi (args[4].c_str() ), atoi(args[5].c_str()));
		} else if ( args[3].compare("on") == 0){
			ret = update_on(atoi (args[4].c_str()));
		} else if ( args[3].compare("off") == 0){
			ret = update_off(atoi (args[4].c_str()));
		} else if ( args[3].compare("orient") == 0){
			ret = update_orientation( atoi (args[4].c_str() ), atoi(args[5].c_str()) );
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
		printf("connected to channel #");
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
