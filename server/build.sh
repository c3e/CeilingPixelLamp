if [ ! -d "mosquitto-1.4.9" ]; then
	tar -xvf mosquitto-1.4.9.tar.gz;
	cd mosquitto-1.4.9
	cmake . 
	make
	cd ..
fi	

g++ -levent -lpthread -I./mosquitto-1.4.9/lib/ -lmosquitto gason.cpp panels.cpp
