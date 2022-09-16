LDFLAGS = -lwiringPi -lmosquitto -lsystemd -lpthread

#.PHONY: all clean

all : vedirect_to_mqtt

vedirect_to_mqtt : vedirect_to_mqtt.o
	${CXX} $^ -o $@ ${LDFLAGS}

clean :
	-rm -f *.o vedirect_to_mqtt

install : all
	-systemctl stop vedirect_to_mqtt
	chown root:root ./vedirect_to_mqtt.service ./vedirect_to_mqtt
	chmod 664 ./vedirect_to_mqtt.service
	cp ./vedirect_to_mqtt.service /etc/systemd/system/
	cp ./vedirect_to_mqtt /usr/local/lib/
	systemctl daemon-reload
	systemctl enable vedirect_to_mqtt
	systemctl restart vedirect_to_mqtt
