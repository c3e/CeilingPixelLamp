# apt-get install libssl-dev libc-ares-dev libc-ares2 ibevent-dev libevent-2.0-5 libuu-dev libuu0
# libssl-dev -- openssl
# libc-ares-dev libc-ares2 -- c-ares
# libevent-dev libevent-2.0-5 -- libev
# libuu-dev libuu0
# cmake
#

cd libs
if [ ! -d "mosquitto-1.4.9" ]; then
	tar -xvf mosquitto-1.4.9.tar.gz;
	cd mosquitto-1.4.9
	cmake . 
	sudo make install
	cd ..
fi	
cd ..
g++ -std=c++11 -levent -lpthread -lmosquitto libs/gason.cpp libs/can.cpp panels.cpp
# vim: ts=2 sw=2 et:
