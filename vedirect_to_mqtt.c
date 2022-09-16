#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>
#include <systemd/sd-daemon.h>

#include <mosquitto.h>
#include <wiringSerial.h>
#include "vedirect.h"


// TODO: Parse returned messages, store data in intermediate form
// TODO: Add thread for publish to MQTT
// TODO: Only GETS right now.  Consider adding SET commands

static volatile int running = 1;
int fd;
struct mosquitto *mqtt;

pthread_t process_rx_thread;
pthread_t process_rq_thread;
pthread_t process_tx_thread;

pthread_mutex_t lock_request_list;

const char *mqtt_host = "192.168.43.57";
const unsigned int mqtt_port = 1883;
const char *bmv_topic_root = "bmv";
const unsigned int min_request_period_us = 50000; // Too fast and the BMV will miss requests

// TODO: Replace with dynamic FIFO/circular buffer
unsigned int request_list[32]; // Added to periodically, processed immediately with small delay between
unsigned int request_count = 0;

enum ReceiveState {
    GET_START_CHAR,
    GET_HEX_DATA,
    GET_TEXT_DATA,
};

struct VEPeriodicRequest {
    bool publish;
    const char *name;
    float  request_period_s;
    double last_update_s;
//    float publish_period_s;
};

// TODO: Add command line switch or similar to control the request list
// NOTE: Setting any of these less than about 2 seconds can prevent the automatic VE.Direct TEXT protocol from being output
struct VEPeriodicRequest periodic_request_list[] = {
    { true, "soc", 3, 0 },
    { true, "current_coarse", 3, 0 },
    { true, "consumed_ah", 3, 0 },
    { true, "main_voltage", 3, 0 },
};

double timestamp(void) {
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec + ( spec.tv_nsec / 1.0e9 );
}

unsigned int asciiHexToInt(char ch) {
  unsigned int num = 0;
  if( (ch >= '0') && (ch <= '9') ) {
    num = ch - '0';
  }
  else {
    switch(ch) {
      case 'A': case 'a': num = 10; break;
      case 'B': case 'b': num = 11; break;
      case 'C': case 'c': num = 12; break;
      case 'D': case 'd': num = 13; break;
      case 'E': case 'e': num = 14; break;
      case 'F': case 'f': num = 15; break;
      default: num = 0;
    }
  }

  return num;
}

// Expects message with leading ':' and trailing '\n' stripped
// Returns checksum to be appended
uint8_t CalculateChecksum(char *msg) {
    unsigned int sum = 0;
    bool first_flag = true;

    while(*msg != '\0') {
        if(first_flag) {
            sum = asciiHexToInt(*msg); // First byte is single character
            first_flag = false;
            msg++;
        }
        else {
            if(*(msg + 1) != '\0') {
                sum += asciiHexToInt(*msg) << 4;
                sum += asciiHexToInt(*(msg + 1));
                msg += 2;
            }
        }
    }

    return (0x55 - sum);
}

void BuildRequest(char *msg, uint16_t address) {
    char temp[50];
    sprintf(temp, "%c%0.2X%0.2X%0.2X", VE_CMD_GET, (uint8_t)(address & 0xFF), (uint8_t)(address >> 8), 0);
    sprintf(msg, ":%s%0.2X\n", temp, CalculateChecksum(temp));
}

void *ProcessUARTTransmitQueueThread(void *param) {
    char message_buffer[50];

    while(running) {
        while(request_count > 0) {
            BuildRequest(message_buffer, request_list[request_count - 1]);

            pthread_mutex_lock(&lock_request_list);
            request_count--;
            pthread_mutex_unlock(&lock_request_list);

            serialPrintf(fd, "%s", message_buffer);
            //printf("<<< <UART> %s\r\n", message_buffer);
            usleep(min_request_period_us);
        }
    }
}

void ParseTextMessage(char *msg_buf) {
    struct VEDirectTextMsg vedirect_msg;
    char *token;
    const char sep[2] = "\t";
    char reg_name[10];
    char value_string[34];
    long reg_value;
    char mqtt_topic[50];
    char mqtt_payload[50];

    //printf("ParseTextMessage(%s)\r\n", msg_buf); fflush(NULL);

    token = strtok(msg_buf, sep);
    if (token != NULL) {
        strcpy(reg_name, token);
    }

    token = strtok(NULL, sep);
    if (token != NULL) {
        strcpy(value_string, token);
        reg_value = atoi(value_string);
    }

    //DEBUG//printf("%s = %d\r\n", reg_name, reg_value); fflush(NULL);

    if( ve_lookup_by_text_name(&vedirect_msg, reg_name) ) {
        switch(vedirect_msg.type) {

            case VE_TYPE_TXT_FLOAT:
                //DEBUG//printf("--> Found %s = %0.3f\r\n", vedirect_msg.name, reg_value * vedirect_msg.multiplier); fflush(NULL);

                if( !strcmp(value_string, "---") ) {
                    sprintf(mqtt_payload, ""); // TODO: This might not be the best way to indicate invalid data
                }
                else {
                    sprintf(mqtt_payload, "%0.3f", reg_value * vedirect_msg.multiplier);
                }
                break;

            case VE_TYPE_TXT_INT:
                //DEBUG//printf("--> Found %s = %0.3f\r\n", vedirect_msg.name, reg_value * vedirect_msg.multiplier); fflush(NULL);

                if( !strcmp(value_string, "---") ) {
                    sprintf(mqtt_payload, ""); // TODO: This might not be the best way to indicate invalid data
                }
                else {
                    sprintf(mqtt_payload, "%ld", reg_value);
                }
                break;

            case VE_TYPE_TXT_BOOL:
                //DEBUG//printf("--> Found %s = %s\r\n", vedirect_msg.name, value_string); fflush(NULL);

                if( !strcmp(value_string, "ON") ) {
                    sprintf(mqtt_payload, "1");
                }
                else if( !strcmp(value_string, "OFF") ) {
                    sprintf(mqtt_payload, "0");
                }
                else {
                    sprintf(mqtt_payload, ""); // TODO: This might not be the best way to indicate invalid data
                }

                break;
        }

        sprintf(mqtt_topic, "%s/text/%s", bmv_topic_root, vedirect_msg.name);

        printf("<<< <MQTT> Publish %s = %s\r\n", mqtt_topic, mqtt_payload); fflush(NULL);
        mosquitto_publish(mqtt, NULL, mqtt_topic, strlen(mqtt_payload), mqtt_payload, 0, false);
    }
    else {
        //DEBUG//printf("--> NOT FOUND %s\r\n", reg_name); fflush(NULL);
    }

}

void ParseHexMessage(char *msg_buf) {
    double now;
    struct VEDirectHexMsg vedirect_msg;
    unsigned int msg_len;
    char c;
    unsigned int address = 0;
    uint8_t flags = 0;
    char string_data[33];
    double data = 0;
    char mqtt_topic[50];
    char mqtt_payload[50];

    //printf("ParseHexMessage(%s)\r\n", msg_buf);

    now = timestamp(); // Calculate/display time between request being sent and receipt of message
    msg_len = strlen(msg_buf);

    // Smallest valid message contains one command char plus four hex digts of address
    if(msg_len >= 5) {

        c = *msg_buf;

        // TODO: Verify checksum

        if(c == VE_RSP_GET) {
            msg_buf++;

            address = ( asciiHexToInt(*(msg_buf + 0)) << 4 ) +
                      ( asciiHexToInt(*(msg_buf + 1)) << 0 ) +
                      ( asciiHexToInt(*(msg_buf + 2)) << 12 ) +
                      ( asciiHexToInt(*(msg_buf + 3)) << 8 );

            if( ve_lookup_by_hex_address(&vedirect_msg, address) ) {

                //printf("Parsing data from %s [%0.4x], length = %d\r\n", vedirect_msg.name, address, msg_len);

                msg_buf += 4; // Advance to the start of flags

                if(msg_len >= 7) {
                    flags = ( asciiHexToInt(*(msg_buf + 0)) << 4 ) +
                            ( asciiHexToInt(*(msg_buf + 1)) << 0 );

                    //printf("Flags: %d\r\n", flags);

                    if(flags == 0) {
                        msg_buf += 2; // Point to the start of data bytes

                        switch(vedirect_msg.type) {
                            case VE_TYPE_NONE:
                                return;
                            break;

                            case VE_TYPE_UN8:
                                if(msg_len >= 9) {
                                    data = (uint8_t)( ( asciiHexToInt(*(msg_buf + 0)) << 4 ) +
                                                      ( asciiHexToInt(*(msg_buf + 1)) << 0 ) );
                                }

                            break;

                            case VE_TYPE_SN8:
                                if(msg_len >= 9) {
                                    data = (int8_t)( ( asciiHexToInt(*(msg_buf + 0)) << 4 ) +
                                                     ( asciiHexToInt(*(msg_buf + 1)) << 0 ) );
                                }
                            break;

                            case VE_TYPE_UN16:
                                if(msg_len >= 11) {
                                    data = (uint16_t)( ( asciiHexToInt(*(msg_buf + 0)) << 4  ) +
                                                       ( asciiHexToInt(*(msg_buf + 1)) << 0  ) +
                                                       ( asciiHexToInt(*(msg_buf + 2)) << 12 ) +
                                                       ( asciiHexToInt(*(msg_buf + 3)) << 8  ) );
                                }

                            break;

                            case VE_TYPE_SN16:
                                if(msg_len >= 11) {
                                    data = (int16_t)( ( asciiHexToInt(*(msg_buf + 0)) << 4  ) +
                                                      ( asciiHexToInt(*(msg_buf + 1)) << 0  ) +
                                                      ( asciiHexToInt(*(msg_buf + 2)) << 12 ) +
                                                      ( asciiHexToInt(*(msg_buf + 3)) << 8  ) );
                                }
                            break;

                            case VE_TYPE_UN24:
                                if(msg_len >= 13) {
                                    data = (uint32_t)( ( asciiHexToInt(*(msg_buf + 0)) << 4  ) +
                                                       ( asciiHexToInt(*(msg_buf + 1)) << 0  ) +
                                                       ( asciiHexToInt(*(msg_buf + 2)) << 12 ) +
                                                       ( asciiHexToInt(*(msg_buf + 3)) << 8  ) +
                                                       ( asciiHexToInt(*(msg_buf + 4)) << 20 ) +
                                                       ( asciiHexToInt(*(msg_buf + 5)) << 16 ) );
                                }
                            break;

                            //case VE_TYPE_SN24:
                            //break;

                            case VE_TYPE_UN32:
                                if(msg_len >= 15) {
                                    data = (uint32_t)( ( asciiHexToInt(*(msg_buf + 0)) << 4  ) +
                                                       ( asciiHexToInt(*(msg_buf + 1)) << 0  ) +
                                                       ( asciiHexToInt(*(msg_buf + 2)) << 12 ) +
                                                       ( asciiHexToInt(*(msg_buf + 3)) << 8  ) +
                                                       ( asciiHexToInt(*(msg_buf + 4)) << 20 ) +
                                                       ( asciiHexToInt(*(msg_buf + 5)) << 16 ) +
                                                       ( asciiHexToInt(*(msg_buf + 6)) << 28 ) +
                                                       ( asciiHexToInt(*(msg_buf + 7)) << 24 ) );
                                }
                            break;

                            case VE_TYPE_SN32:
                                if(msg_len >= 15) {
                                    data = (int32_t)( ( asciiHexToInt(*(msg_buf + 0)) << 4  ) +
                                                      ( asciiHexToInt(*(msg_buf + 1)) << 0  ) +
                                                      ( asciiHexToInt(*(msg_buf + 2)) << 12 ) +
                                                      ( asciiHexToInt(*(msg_buf + 3)) << 8  ) +
                                                      ( asciiHexToInt(*(msg_buf + 4)) << 20 ) +
                                                      ( asciiHexToInt(*(msg_buf + 5)) << 16 ) +
                                                      ( asciiHexToInt(*(msg_buf + 6)) << 28 ) +
                                                      ( asciiHexToInt(*(msg_buf + 7)) << 24 ) );
                                }
                            break;

                            case VE_TYPE_STR20:
                            break;

                            case VE_TYPE_STR32:
                            break;

                        }

                        data *= vedirect_msg.multiplier;
                        //printf("Parsing data from %s [%0.4X] = %0.3f\r\n", vedirect_msg.name, address, data);

                        // Find the entry in periodic request list to see whether it should be published
                        for (int i = 0; i < ( sizeof(periodic_request_list) / sizeof(struct VEPeriodicRequest) ); i++ ) {
                            if( !strcmp(periodic_request_list[i].name, vedirect_msg.name) ) {
                                if( periodic_request_list[i].publish ) {
                                    sprintf(mqtt_topic, "%s/hex/%s", bmv_topic_root, vedirect_msg.name);
                                    sprintf(mqtt_payload, "%0.2f", data);

                                    printf("<<< <MQTT> Publish %s = %s\r\n", mqtt_topic, mqtt_payload); fflush(NULL);
                                    mosquitto_publish(mqtt, NULL, mqtt_topic, strlen(mqtt_payload), mqtt_payload, 0, false);
                                }
                            }
                        }

                    }
                }
            }
        }
    }
}

void *ProcessReceiveThread(void *param) {
    static char message_buffer[50] = {0};
    static enum ReceiveState receive_state = GET_START_CHAR;
    char c;

    while(running) {

        if(serialDataAvail(fd)) {

            c = serialGetchar(fd);

            //DEBUG
            //if ( isprint(c) ) {
            //    printf( "%c", c );
            //}
            //else {
            //    printf( "<%0.2X>", (uint8_t)c );
            //}
            //
            //fflush(NULL);

            switch(receive_state) {

                case GET_START_CHAR:

                    if( c  == ':' ) {
                        //DEBUG//printf("<SOL>");
                        strncpy(message_buffer, "", sizeof(message_buffer));
                        receive_state = GET_HEX_DATA;
                    }
                    else if( c == '\n') {
                        strncpy(message_buffer, "", sizeof(message_buffer));
                        receive_state = GET_TEXT_DATA;
                    }
                break;

                case GET_HEX_DATA:

                    if( isprint(c) ) {
                        // Valid data, save/buffer it for later
                        //DEBUG//printf("%c", c);
                        strncat(message_buffer, &c, 1);
                    }
                    else if( c == '\n' ) {
                        //DEBUG//printf("<EOL>\r\n");
                        ParseHexMessage(message_buffer);
                        receive_state = GET_START_CHAR;
                    }
                    else if( c == ':' ) {
                        // ERROR - expected ETX before STX
                        //printf("<UART> ERROR: New packet began before previous packet finished\r\n");
                        strncpy(message_buffer, "", sizeof(message_buffer));
                    }
                break;

                case GET_TEXT_DATA:

                    if( c == '\r' ) {
                        //DEBUG//printf("<EOL>\r\n");
                        ParseTextMessage(message_buffer);
                        receive_state = GET_START_CHAR;
                    }
                    else if( c == '\n' ) {
                        //printf("<UART> ERROR: New packet began before previous packet finished\r\n");
                        strncpy(message_buffer, "", sizeof(message_buffer));
                    }
                    else if( c == ':' ) {
                        receive_state = GET_HEX_DATA;
                        strncpy(message_buffer, "", sizeof(message_buffer));
                    }
                    else {
                        strncat(message_buffer, &c, 1);
                    }
                break;

            }
        }
    }
}

void *ProcessVEDirectRequestThread(void *param) {
    double now;
    char message_buffer[50] = {0};
    struct VEDirectHexMsg vedirect_msg;

    while(running) {
        // TODO: Replace the static structure with a dynamic one
        for (int i = 0; i < ( sizeof(periodic_request_list) / sizeof(struct VEPeriodicRequest) ); i++ ) {

            now = timestamp();

            //printf("%0.6f> %s [%0.3f]\r\n", now, periodic_request_list[i].name, (now - periodic_request_list[i].last_update_s) );
            if( (now - periodic_request_list[i].last_update_s) >= periodic_request_list[i].request_period_s ) {

                //printf("REQUEST %s [%0.3f]\r\n", periodic_request_list[i].name, (now - periodic_request_list[i].last_update_s) );
                periodic_request_list[i].last_update_s = now;

                if( ve_lookup_by_hex_name(&vedirect_msg, periodic_request_list[i].name) ) {

                    pthread_mutex_lock(&lock_request_list);
                    if( request_count < (sizeof(request_list) - 1) ) {
                        request_count++;
                        request_list[request_count-1] = vedirect_msg.address;
                    }
                    pthread_mutex_unlock(&lock_request_list);

                }
                // TODO: else name is invalid and should be removed or user signalled
            }
        }
    }
}

void SignalHandler(int signum)
{
    running = 0;
}

int main ()
{
    int status;
    char client_id[30];
    char message_buffer[50] = {0};

    signal(SIGINT, SignalHandler);
    signal(SIGHUP, SignalHandler);
    signal(SIGTERM, SignalHandler);

    // SETUP UART
    if ((fd = serialOpen ("/dev/ttyS0", 19200)) < 0) {
            fprintf (stderr, "Unable to open serial device: %s\n", strerror(errno));
            return 1;
    }

    // SETUP MQTT
    mosquitto_lib_init();
    snprintf(client_id, sizeof(client_id)-1, "offgrid-daemon-%d", getpid());

    if( (mqtt = mosquitto_new(client_id, true, NULL)) != NULL ) { // TODO: Replace NULL with pointer to data structure 
        //mosquitto_connect_callback_set(mqtt, connect_callback);
        //mosquitto_message_callback_set(mqtt, message_callback);

        if( (mosquitto_connect(mqtt, mqtt_host, mqtt_port, 15)) == MOSQ_ERR_SUCCESS ) {
            //mosquitto_subscribe(mqtt, NULL, "og/#", 0);
            mosquitto_loop_start(mqtt);
        }
        else {
                    fprintf (stderr, "Unable to connect with MQTT broker (%s:%s): %s\n", mqtt_host, mqtt_port, strerror(errno));
        }
    }
    else {
                fprintf (stderr, "Unable to create MQTT client: %s\n", strerror(errno));
        return 1;
    }

    if(pthread_mutex_init(&lock_request_list, NULL) != 0) {
        fprintf (stderr, "Mutex initialization failed\n");
        return 1;
    }

    // TODO: Add error checking for thread creation
    pthread_create(&process_rx_thread, NULL, ProcessReceiveThread, NULL);
    pthread_create(&process_rq_thread, NULL, ProcessVEDirectRequestThread, NULL);
    pthread_create(&process_tx_thread, NULL, ProcessUARTTransmitQueueThread, NULL);

    serialFlush(fd);

    while(running) {

/*
        printf("\r\n");

        BuildRequest(message_buffer, 0xEEFF); // Consumed Ah x10
        serialPrintf(fd, "%s", message_buffer);

        usleep(min_request_period_us);

        BuildRequest(message_buffer, 0xED8F); // Current x10
        serialPrintf(fd, "%s", message_buffer);

        usleep(min_request_period_us);

        BuildRequest(message_buffer, 0xED8D); // Main voltage x100
        serialPrintf(fd, "%s", message_buffer);

        usleep(min_request_period_us);

        BuildRequest(message_buffer, 0x0FFF); // SOC x100
        serialPrintf(fd, "%s", message_buffer);

        usleep(min_request_period_us);
*/
        if(running && status) {
            mosquitto_reconnect(mqtt);
        }

        sleep(1);
        sd_notify(0, "WATCHDOG=1");
    }

    printf("Waiting for threads to terminate...\r\n");
    fflush(NULL);

    pthread_join(process_rx_thread, NULL);
    pthread_join(process_rq_thread, NULL);
    pthread_join(process_tx_thread, NULL);

    pthread_mutex_destroy(&lock_request_list);

    printf("...Threads terminated\r\n");
    fflush(NULL);

    mosquitto_loop_stop(mqtt, true);

    mosquitto_destroy(mqtt);
    mosquitto_lib_cleanup();

    // TODO: Any other cleanup actions?

    return (EXIT_SUCCESS);

}


