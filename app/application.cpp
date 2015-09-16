#include <user_config.h>
#include <user_interface.h>
#include <SmingCore/SmingCore.h>

// Put you SSID and Password here
#define WIFI_SSID "access"
#define WIFI_PWD ""
#define WIFI_CONF_FILE "wifi_conf"

#define page "<html> <body> <form action='.' method='post'>  SSID: <input type='text' name='ssid'><br>  Password: <input type='text' name='password'><br>  <input type='submit' value='Submit'></form></body></html>"


#define _FOUR_SWITCH_MODE 0


void onMessageReceived(String topic, String message); // Forward declaration for our callback


#ifdef FOUR_SWITCH_MODE
    const int latchPin = 0;
    //Pin connected to clock pin (SH_CP) of 74HC595
    const int clockPin = 2;
    ////Pin connected to Data in (DS) of 74HC595

    const int dataPin = 3;

    int register_state = 0;

#else
    int switches[] = {0, 2};
#endif

HttpClient hc;

Timer procTimer;



HttpServer server;

bool httpInProgress = false;

#define DEBUG

#define RF_DATA_PIN    2



#define MAX_CODE_CYCLES 4

#define SHORT_DELAY       380
#define NORMAL_DELAY      500
#define SIGNAL_DELAY     1500

#define SYNC_DELAY       2650
#define EXTRASHORT_DELAY 3000
#define EXTRA_DELAY     10000

enum {
  PLUG_A = 0,
  PLUG_B,
  PLUG_C,
  PLUG_D,
  PLUG_MASTER,
  PLUG_LAST
};

unsigned long signals[PLUG_LAST][2][MAX_CODE_CYCLES] = {
  { /*A*/
    {0b101111000001000101011100, 0b101100010110110110101100, 0b101110101110010001101100, 0b101101000101010100011100},
    {0b101101010010011101111100, 0b101111100011110000101100, 0b101111110111001110001100, 0b101110111000101110111100}
  },
  { /*B*/
    {0b101101110100001000110101, 0b101101101010100111100101, 0b101110011101111000000101, 0b101100100000100011110101},
    {0b101111011001101011010101, 0b101100111011111101000101, 0b101110001100011010010101, 0b101100001111000011000101}
  },
  { /*C*/
    {0b101101010010011101111110, 0b101111100011110000101110, 0b101111110111001110001110, 0b101110111000101110111110},
    {0b101110101110010001101110, 0b101101000101010100011110, 0b101111000001000101011110, 0b101100010110110110101110}
  },
  { /*D*/
    {0b101111011001101011010111, 0b101100111011111101000111, 0b101100001111000011000111, 0b101110001100011010010111},
    {0b101101110100001000110111, 0b101100100000100011110111, 0b101101101010100111100111, 0b101110011101111000000111}
  },
  { /*MASTER*/
    {0b101111100011110000100010, 0b101110111000101110110010, 0b101101010010011101110010, 0b101111110111001110000010},
    {0b101111000001000101010010, 0b101101000101010100010010, 0b101110101110010001100010, 0b101100010110110110100010}
  },
};

boolean       onOff;
unsigned char plug;
unsigned char swap;



void loop() {


}


void sendSync(){
  digitalWrite(RF_DATA_PIN, HIGH);
  delayMicroseconds(SHORT_DELAY);
  digitalWrite(RF_DATA_PIN, LOW);
  delayMicroseconds(SYNC_DELAY - SHORT_DELAY);
}

void sendValue(boolean value, unsigned int base_delay){
  unsigned long d = value? SIGNAL_DELAY - base_delay : base_delay;
  digitalWrite(RF_DATA_PIN, HIGH);
  delayMicroseconds(d);
  digitalWrite(RF_DATA_PIN, LOW);
  delayMicroseconds(SIGNAL_DELAY - d);
}

void longSync(){
  digitalWrite(RF_DATA_PIN, HIGH);
  delayMicroseconds(EXTRASHORT_DELAY);
  digitalWrite(RF_DATA_PIN, LOW);
  delayMicroseconds(EXTRA_DELAY - EXTRASHORT_DELAY);
}

void ActivatePlug(unsigned char PlugNo, boolean On) {
  if( PlugNo < PLUG_LAST ) {

    digitalWrite(RF_DATA_PIN,LOW);
    delayMicroseconds(1000);

    unsigned long signal = signals[PlugNo][On][swap];

    swap++;
    swap%=MAX_CODE_CYCLES;

    for (unsigned char i=0; i<4; i++) { // repeat 1st signal sequence 4 times
      sendSync();
      for (unsigned char k=0; k<24; k++) { //as 24 long and short signals, this loop sends each one and if it is long, it takes it away from total delay so that there is a short between it and the next signal and viceversa
        sendValue(bitRead(signal, 23-k),SHORT_DELAY);
      }
    }
    for (unsigned char i=0; i<4; i++) { // repeat 2nd signal sequence 4 times with NORMAL DELAY
      longSync();
      for (unsigned char k=0; k<24; k++) {
        sendValue(bitRead(signal, 23-k),NORMAL_DELAY);
      }
    }

  }
}






String mqttName(){
    String name;
    int id = system_get_chip_id();
    name = "SmartController-";
    name  += id;
    return name;
}


String commandTopic(){
    String topic;
    int id = system_get_chip_id();
    topic = "/smart_plug_work/SmartPlug-";
    topic  = topic + id;
    return topic;
}

class ReconnectingMqttClient: public MqttClient{
    using MqttClient::MqttClient; // Inherit Base's constructors.

    void onError(err_t err)  {
        close();
        connect(mqttName());
        subscribe(commandTopic());
        return;
    }

};

ReconnectingMqttClient mqtt("dmarkey.mooo.com", 8000, onMessageReceived);

void printResponse(HttpClient& hc, bool success){
    Serial.print(hc.getResponseString());
}





void processSwitchcmd(JsonObject& obj){
    int switch_num = obj["socketnumber"];

    bool state = obj["state"];
    ActivatePlug(switch_num-1, !state);


}


void setTaskStatus(JsonObject& obj, int status){

    char post_data[256];
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["controller_id"] = system_get_chip_id();
    root["task_id"] = obj["task_id"];
    root["status"] = status;

    root.printTo(post_data, sizeof(post_data));
    mqtt.publish("/admin/task_status", post_data);
    //queue_web_request("http://dmarkey.com:8080/controller_task_status/", post_data, "application/json");
}

void beaconComplete(){

    StaticJsonBuffer<200> jsonBuffer;
    char data[200];
    JsonObject& root = jsonBuffer.createObject();
    root["controller_id"] = system_get_chip_id();
    root.printTo(data, sizeof(data));
    mqtt.publish("/admin/beacon", data);
    //queue_web_request("http://dmarkey.com:8080/controller_task_status/", post_data, "application/json");
}


// Callback for messages, arrived from MQTT server
void onMessageReceived(String topic, String message)
{

	Serial.print(topic);
	if (topic == commandTopic()){
	    StaticJsonBuffer<200> jsonBuffer;
        const char *json = message.c_str();
        JsonObject& root = jsonBuffer.parseObject((char *)json);

        const char * command = root["name"];
        setTaskStatus(root, 2);

        if (strcmp(command, "sockettoggle") != -1){
                processSwitchcmd(root);
                setTaskStatus(root, 3);

        }


	}

}




void processBeaconResponse(HttpClient& hc, bool success){
    Serial.print(hc.getResponseString());
    if (success){
        mqtt.connect(mqttName());
        mqtt.subscribe(commandTopic());
        beaconComplete();
    }



    //processing_web = false;
    //process_web();
}


void beaconFunc(){
    String post_data;
    post_data = "model=SmartController-4RF&controller_id=";
    post_data += system_get_chip_id();
    post_data += "\r\n";
    //queue_web_request("http://dmarkey.com:8080/controller_ping_create/", post_data);
    hc.setPostBody(post_data);
    hc.downloadString("http://dmarkey.com:8080/controller_ping_create/", processBeaconResponse);
}



// Will be called when WiFi station was connected to AP


void connectOk()
{
	Serial.println("I'm CONNECTED");
    beaconFunc();
	// Run MQTT client

}

void writeConf(String SSID, String Pwd){
    String buf;
    char cstring[100];
    buf = SSID + "\n" + Pwd;
    buf.toCharArray(cstring, 100);
    fileSetContent(WIFI_CONF_FILE, cstring);
}


void restart(){
    System.restart();
}

void onIndex(HttpRequest &request, HttpResponse &response)
{
	String ssid = request.getPostParameter("ssid");
	String password = request.getPostParameter("password");

	if (ssid == ""){
        response.sendString(page);
        return;
	}
	else{
        writeConf(ssid, password);
        response.sendString("Success");
        procTimer.initializeMs(1000, restart).start();
	}

}

// Will be called when WiFi station timeout was reached
void connectFail()
{
    String SSID;
    SSID = "Smarthome-";
    SSID = SSID + system_get_chip_id();
    WifiAccessPoint.config(SSID,"", AUTH_OPEN, false, 2, 2000);
    WifiAccessPoint.enable(true);
    server.listen(80);
	server.addPath("/", onIndex);
	WifiStation.enable(false);

	Serial.println("Fallback WIFI mode.");

	// .. some you code for device configuration ..
}


void init()
{

    pinMode(RF_DATA_PIN, OUTPUT);

	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(true); // Debug output to serial


	//wifi_station_set_hostname("MyEsp8266");

	String wifiSSID, wifiPassword, tmp;
	file_t wifi_file;



	if(!fileExist(WIFI_CONF_FILE)){
        writeConf(WIFI_SSID, WIFI_PWD);

	}
	else{
        tmp = fileGetContent(WIFI_CONF_FILE);
        Serial.println("WIFI_CONFIG");
        Serial.println(tmp);
        int newline = tmp.indexOf('\n');
        wifiSSID = tmp.substring(0, newline);
        wifiPassword = tmp.substring(newline+1, tmp.length());
        Serial.println(wifiSSID);
        Serial.println(wifiPassword);
        //wifiSSID = fileRead(wifi_file);


	}

	WifiStation.config(wifiSSID, wifiPassword);
	WifiStation.enable(true);
	WifiAccessPoint.enable(false);

	// Run our method when station was connected to AP (or not connected)
	WifiStation.waitConnection(connectOk, 20, connectFail); // We recommend 20+ seconds for connection timeout at start
}
