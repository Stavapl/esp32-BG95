#ifndef ESP32_BG95_H
#define ESP32_BG95_H

#include <Arduino.h>
#include <Time.h>
#include <TimeLib.h>
#include "mbedtls/md.h"

#include "editable_macros.h"

#define GSM 1
#define GPRS 2
#define NB 3
#define CATM1 4
#define AUTO 5

#define NOT_REGISTERED 0
#define REGISTERED 1
#define CONNECTING 2
#define DENIED 3
#define UNKNOWN 4
#define ROAMING 5

#define MQTT_STATE_DISCONNECTED 0
#define MQTT_STATE_INITIALIZING 1
#define MQTT_STATE_CONNECTING 2
#define MQTT_STATE_CONNECTED 3
#define MQTT_STATE_DISCONNECTING 4

// PRIORITY TYPES
#define PRIORITY_GNSS 0
#define PRIORITY_WWAN 1

// CONSTANTS
#define AT_WAIT_RESPONSE 10 // milis
#define AT_TERMINATOR '\n'	// \n

#define MAX_SMS 10

#define now_us esp_timer_get_time()
#define TIMEIT(func) do { int64_t s=now_us; func; int64_t d=(now_us-s); ESP_LOGE("TIMEIT", #func " took %fs", d/1000.0/1000.0); } while(0);

class MODEMBGXX
{
public:
	HardwareSerial *log_output = &Serial;
	HardwareSerial *modem = &Serial2;

	MODEMBGXX(){};
	/*
	 * @serial_modem - Serial port for modem connection
	 */
	MODEMBGXX(HardwareSerial *serial_modem)
	{
		modem = serial_modem;
	};
	/*
	 * @serial_modem - Serial port for modem connection
	 * @serial_log - Serial port for logs
	 */
	MODEMBGXX(HardwareSerial *serial_modem, HardwareSerial *serial_log)
	{
		modem = serial_modem;
		log_output = serial_log;
	};

	/*
	 * call it to initialize state machine
	 */
	bool init(uint8_t radio, uint16_t cops, uint8_t pwkey);
	/*
	 * call it to initialize serial port
	 */
	void init_port(uint32_t baudrate, uint32_t config);
	void init_port(uint32_t baudrate, uint32_t serial_config, uint8_t tx_pin, uint8_t rx_pin);
	/*
	 * call it to disable serial port
	 */
	void disable_port();
	/*
	 * switch off and on
	 */
	bool powerCycle();
	/*
	 * setup APN configuration
	 *
	 * @cid - 0-16, limited to MAX_CONNECTIONS
	 *
	 * returns true if succeed
	 */
	bool setup(uint8_t cid, String apn, String username, String password);

	/*
	 * Set error message format (AT+CMEE)
	 * 
	 * n = 0: Disable result code
	 * n = 1: Enable result code and use numeric values
	 * n = 2: Enable result code and use verbose values
	 */
	bool set_error_message_format(int n);

	bool set_ssl(uint8_t ssl_cid);

	//
	/*
	 * check for pending commands and received data
	 */
	bool loop(uint32_t loop = 10);

	// --- MODEM static registered numbers ---
	String get_imei();
	String get_ccid();
	String get_imsi();
	String get_subscriber_number(uint16_t wait = 3000);

	/*
	 * freeRTOS - safe function
	 * return last retrieved rssi
	 */
	int16_t rssi(); // return last read value
	/*
	 * freeRTOS - safe function
	 * return tech in use
	 */
	String technology(); // return tech in use
	/*
	 * freeRTOS - safe function
	 * return tech in use - use it to check if modem is registered in a tower cell
	 */
	int8_t get_actual_mode();

	// --- CONTEXT ---
	/*
	 * get IP of a context
	 */
	String get_ip(uint8_t cid = 1);
	/*
	 * check if modem is connected to apn
	 */
	bool apn_connected(uint8_t cid = 1);
	/*
	 * check if modem has IP
	 */
	bool has_context(uint8_t cid = 1);
	/*
	 * open context
	 */
	bool open_pdp_context(uint8_t cid = 1);
	/*
	 * close context
	 */
	bool close_pdp_context(uint8_t cid = 1);
	/*
	 * returns the state of context id
	 */
	String check_context_state(uint8_t contextID);
	/*
	 * returns the state of connection id
	 */
	String check_connection_state(uint8_t connectionID);

	// --- SMS ---
	/*
	 * check if there is some function to deal with sms
	 */
	bool sms_check_handler();
	/*
	 * pass callback for sms
	 */
	bool sms_handler(void (*handler)(uint8_t, String, String));
	/*
	 * send sms
	 */
	bool sms_send(String origin, String message);
	/*
	 * remove sms
	 */
	bool sms_remove(uint8_t index);

	// --- TCP ---
	void tcp_set_callback_on_close(void (*callback)(uint8_t clientID));
	bool tcp_connect(uint8_t clientID, String host, uint16_t port, uint16_t wait = 10000);
	bool tcp_connect(uint8_t contextID, uint8_t clientID, String host, uint16_t port, uint16_t wait = 10000);
	bool tcp_connect_ssl(uint8_t contextID, uint8_t sslClientID, uint8_t clientID, String host, uint16_t port, uint16_t wait = 10000);
	bool tcp_connected(uint8_t clientID);
	bool tcp_close(uint8_t clientID);
	bool tcp_send(uint8_t clientID, const char *data, uint16_t size);
	uint16_t tcp_recv(uint8_t clientID, char *data, uint16_t size);
	uint16_t tcp_has_data(uint8_t clientID);
	void tcp_check_data_pending();

	// --- CLOCK ---
	/*
	 * use it to get network clock
	 */
	bool get_clock(tm *t); // get unix timestamp
	/*
	 * get timezone difference
	 */
	int32_t get_tz();
	/*
	 * update system clock
	 * uses get_clock function
	 */
	void update_sys_clock();
	// --- LOCATION ---
	/*
	 * get info from near cells
	 */
	String scan_cells();
	/*
	 * get gps position
	 */
	String get_position();

	/*
	 * set priority mode (GNSS vs WWAN) (AT+QGPSCFG="priority",priority_type,save)
	 *
	 * priority_type: one of PRIORITY_GNSS, PRIORITY_WWAN 
	 * save: true if save to NVRAM
	 */
	bool set_priority_mode(int priority_type, bool save = false);

	// --- MQTT ---
	void MQTT_init(bool (*callback)(uint8_t clientID, String topic, String payload));
	bool MQTT_setup(uint8_t clientID, uint8_t contextID, String willTopic, String willPayload);
	bool MQTT_set_ssl(uint8_t clientID, uint8_t contextID, uint8_t sslClientID);
	bool MQTT_connect(uint8_t clientID, const char *uid, const char *user, const char *pass, const char *host, uint16_t port = 1883, uint8_t cleanSession = 1);
	bool MQTT_connected(uint8_t clientID);
	int8_t MQTT_disconnect(uint8_t clientID);
	bool MQTT_subscribeTopic(uint8_t clientID, uint16_t msg_id, String topic, uint8_t qos);
	bool MQTT_subscribeTopics(uint8_t clientID, uint16_t msg_id, String topic[], uint8_t qos[], uint8_t len);
	int8_t MQTT_unSubscribeTopic(uint8_t clientID, uint16_t msg_id, String topic[], uint8_t len);
	int8_t MQTT_publish(uint8_t clientID, uint16_t msg_id, uint8_t qos, uint8_t retain, String topic, String msg);
	void MQTT_readAllBuffers(uint8_t clientID);

	// --- HTTP ---
	bool HTTP_config(uint8_t contextID=1);
	void HTTP_GET_download(String url, String filename, 
		void (*pending_callback)(int16_t http_status, size_t content_length),
		void (*finished_callback)(void),
		void (*failed_callback)(void));

	// --- FILE ---
	void FILE_get_chunk(String filename, char *buf, size_t size, size_t offset, size_t* read_bytes);

	void log_status();
private:
	struct SMS
	{
		bool used;
		uint8_t index;
		char origin[20];
		char msg[256];
	};

	// configurations
	struct Modem
	{
		uint8_t pwkey;
		bool ready;
		bool did_config;
		bool sim_ready;
		uint8_t radio;
		uint16_t cops;
		bool force;
		char tech_string[16];
		uint8_t technology;
	};

	struct APN
	{
		char name[64];
		uint8_t contextID; // context id 1-16
		bool active;
		bool connected;
		uint32_t retry;
		char ip[15];
	};

	struct TCP
	{
		char server[64];
		uint16_t port;
		uint8_t contextID; // context id 1-16
		uint8_t connectID; // connect id 0-11
		bool ssl;
		uint8_t sslClientID;
		uint8_t socket_state;
		bool active;
		bool connected;
	};

	struct MQTT
	{
		char host[64];
		uint8_t contextID; // index for TCP tcp[] 1-16, limited to MAX_CONNECTIONS
		uint8_t clientID;  // client id 0-5 (limited to MAX_MQTT_CONNECTIONS)
		uint8_t socket_state;
		bool active;
		bool connected;
	};

	Modem op = {
		/* pwkey */ 0,
		/* ready */ false,
		/* did_config */ false,
		/* sim_ready */ false,
		/* radio */ 0,
		/* cops */ 0,
		/* force */ false,
		/* tech_string */ "",
		/* technology */ 0
	};

	// State
	String state;
	// IMEI
	String imei;
	// IP address
	String ip_address;
	// pending texts
	SMS message[MAX_SMS];

	int8_t mqtt_buffer[5] = {-1, -1, -1, -1, -1}; // index of msg to read

	APN apn[MAX_CONNECTIONS];
	TCP tcp[MAX_TCP_CONNECTIONS];
	MQTT mqtt[MAX_MQTT_CONNECTIONS];
	MQTT mqtt_previous[MAX_MQTT_CONNECTIONS];

	mbedtls_md_context_t ctx;

	int32_t tz = 0;

	uint8_t cereg; // Unsolicited LTE commands
	uint8_t cgreg; // Unsolicited GPRS commands

	// --- TCP ---
	// size of each buffer
	uint16_t buffer_len[MAX_TCP_CONNECTIONS];
	// data pending of each connection
	bool data_pending[MAX_TCP_CONNECTIONS];
	// validity of each connection state
	uint32_t connected_until[MAX_TCP_CONNECTIONS];
	// last connection start
	uint32_t connected_since[MAX_TCP_CONNECTIONS];
	// data buffer for each connection
	char buffers[MAX_TCP_CONNECTIONS][CONNECTION_BUFFER];
	// --- --- ---

	uint32_t rssi_until = 20000;
	uint32_t loop_until = 0;
	uint32_t ready_until = 15000;

	// last rssi
	int16_t rssi_last = 99;

	// validity of sms check
	uint32_t sms_until = 60000;

	void (*sms_handler_func)(uint8_t, String, String) = NULL;

	uint32_t next_retry = 0;
	uint32_t clock_sync_timeout = 0;

	bool mqtt_pool = false;
	uint32_t mqtt_pool_timeout = 0;
	/*
	 * check if modem is ready (if it's listening for AT commands)
	 */
	bool ready();
	/*
	 * wait for at response
	 */
	bool wait_modem_to_init();
	/*
	 * switch modem on
	 */
	void switchOn();
	/*
	 * switch modem off
	 */
	bool switch_radio_off();
	/*
	 * register on network
	 */
	bool enable_pdp(uint8_t cid);
	/*
	 * deregister from network
	 */
	bool disable_pdp(uint8_t cid);
	/*
	 * reset state machine
	 */
	bool reset();
	/*
	 * configure base settings like ECHO mode and multiplex, check for sim card
	 */
	bool config();
	/*
	 * configure base settings like ECHO mode and multiplex
	 */
	bool configure_radio_mode(uint8_t radio, uint16_t cops, bool force = false);

	// --- TCP ---
	void tcp_read_buffer(uint8_t index, uint16_t wait = 100);

	// --- NETWORK STATE ---
	int16_t get_rssi();
	void get_state(); // get network state

	// --- CLOCK ---
	void sync_clock_ntp(bool force = false); // private

	void check_commands();

	// --- MQTT ---
	bool MQTT_open(uint8_t clientID, const char *host, uint16_t port);
	bool MQTT_isOpened(uint8_t clientID, const char *host, uint16_t port);
	bool MQTT_close(uint8_t clientID);
	void MQTT_checkConnection();
	bool _MQTT_check_in_progress = false;
	void MQTT_readMessages(uint8_t clientID);

	// check for new SMS messages
	void check_sms();

	// process pending SMS messages
	void process_sms(uint8_t index);

	// Read and parse data from modem serial port
	String check_messages();

	String parse_command_line(String line, bool set_data_pending = true);
	void read_data(uint8_t index, String command, uint16_t bytes);

	// run a command and check if it matches an OK or ERROR result String
	bool check_command(String command, String ok_result, uint32_t wait = 5000);
	bool check_command(String command, String ok_result, String error_result, uint32_t wait = 5000);
	bool check_command_no_ok(String command, String ok_result, uint32_t wait = 5000);
	bool check_command_no_ok(String command, String ok_result, String error_result, uint32_t wait = 5000);

	// send a command (or data for that matter)
	void send_command(uint8_t *command, uint16_t size);
	void send_command(String command, bool mute = false);

	String get_command(String command, uint32_t timeout = 300);
	String get_command(String command, String filter, uint32_t timeout = 300);
	String get_command_critical(String command, String filter, uint32_t timeout = 300);
	String get_command_no_ok(String command, String filter, uint32_t timeout = 300);
	String get_command_no_ok_critical(String command, String filter, uint32_t timeout = 300);
	String mqtt_message_received(String line);

	String _HTTP_response_received(String line);
	String _HTTP_file_downloaded(String line);
	String _HTTP_file_download_error(String line);
	bool _HTTP_request_in_progress = false;
	size_t _HTTP_download_content_length = 0;
	String _HTTP_download_filename;

	bool wait_command(String command, uint32_t timeout = 300);

	// debugging
	void log(String text);
	String date();
	String pad2(int value);
	boolean isNumeric(String str);

	int str2hex(String str);
};

#endif
