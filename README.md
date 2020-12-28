# VE.Direct to MQTT Bridging Service

This is a service for bridging Victron Energy's VE.Direct protocol to MQTT.  Originally written and tested using Raspberry Pi 3B+ with Victron BMV-702.  With enough interest and the help of others, this may be expanded to interface with other devices in the same family.

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

Requires mosquitto runtime and development libraries (websockets not used)
	> sudo apt-get update
	> sudo apt-get upgrade
	> sudo apt-get install mosquitto
	> sudo apt-get install libmosquitto-dev
	> sudo apt-get install mosquitto-clients

	Ensuring that the mosquitto MQTT broker is functioning properly is beyond the scope of this document.  There are plenty of other resources, but do make sure it's woring before proceeding.

### Installing

On a Raspberry Pi 3B+, make sure to disable the serial shell using the configuration tool [Interface Options --> Serial, enable the port but not the shell]
	> sudo raspi-config

Connect the Raspberry Pi's serial port with the device's VE.Direct port.  MAKE SURE THE VOLTAGES ARE COMPATIBLE.  VE.Direct ports are not all the same voltage!  The BMV-702 used initially was 3.3V so this works with the Raspberry Pi's voltage.

    Raspberry Pi            VE.Direct
      GND                     GND
      Rx                      Tx
      TX                      Rx

Get a copy of the project, build and install it
	> cd ~
	> git clone https://github/com/TheWandrr/vebus_to_mqtt
	> sudo make install	

Use the mosquitto_sub tool to verify that MQTT data is being published.  You could also use anything else that subscribes to a MQTT broker, like a smartphone app.
	> mosquitto_sub -t 'bmv/#' -v

You should see something like the following repeating at about 1-2 second intervals:

	bmv/hex/soc 65.46
	bmv/hex/main_voltage 12.95
	bmv/hex/consumed_ah -68.30
	bmv/hex/current_coarse -12.20
	bmv/hex/soc 65.44
	bmv/text/max_discharge -160.896
	bmv/text/last_discharge -68.268
	bmv/text/average_discharge -38.254
	bmv/text/num_cycles 77
	bmv/text/num_full_discharge 0
	bmv/text/cumulative_ah -14869.411
	bmv/text/min_voltage 0.002
	bmv/text/max_voltage 14.533
	bmv/text/time_since_full_charge 24821
	bmv/text/num_auto_sync 101
	bmv/text/num_low_volt_alarm 0
	bmv/text/num_high_volt_alarm 0
	bmv/text/energy_discharged 192.270
	bmv/text/energy_charged 171.130
	bmv/text/id 0
	bmv/text/main_voltage 12.905
	bmv/text/current_fine -15.468
	bmv/text/power -200
	bmv/text/consumed_ah -68.268
	bmv/text/soc 65.400
	bmv/text/ttg 127
	bmv/text/alarm_state 0
	bmv/text/relay_state 0
	bmv/text/alarm_reason 0
	bmv/text/sw_version 308
	bmv/text/max_discharge -160.896
	bmv/text/last_discharge -68.272
	bmv/text/average_discharge -38.254
	bmv/text/num_cycles 77
	bmv/text/num_full_discharge 0
	bmv/text/cumulative_ah -14869.415
	bmv/text/min_voltage 0.002
	bmv/text/max_voltage 14.533
	bmv/text/time_since_full_charge 24822
	bmv/text/num_auto_sync 101
	bmv/text/num_low_volt_alarm 0
	bmv/text/num_high_volt_alarm 0
	bmv/text/energy_discharged 192.270
	bmv/text/energy_charged 171.130

## Contributing

## Versioning

## Authors

* **TheWandrr** - *Initial work* - [PurpleBooth](https://github.com/TheWandrr)

See also the list of [contributors](https://github.com/TheWandrr/contributors) who participated in this project.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments


