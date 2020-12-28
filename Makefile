LDFLAGS = -lwiringPi -lmosquitto -lpthread

#.PHONY: all clean

all : vebus_to_mqtt

vebus_to_mqtt : vebus_to_mqtt.o
	${CXX} $^ -o $@ ${LDFLAGS}

clean :
	-rm -f *.o vebus_to_mqtt

install : all
	-systemctl stop vebus_to_mqtt
	chown root:root ./vebus_to_mqtt.service ./vebus_to_mqtt
	chmod 664 ./vebus_to_mqtt.service
	cp ./vebus_to_mqtt.service /etc/systemd/system/
	cp ./vebus_to_mqtt /usr/local/lib/
	systemctl daemon-reload
	systemctl enable vebus_to_mqtt
	systemctl restart vebus_to_mqtt

