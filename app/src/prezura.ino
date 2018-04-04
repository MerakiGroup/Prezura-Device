#include <ESP8266WiFi.h>
#include <WebSocketClient.h>

// Use WiFiClient class to create TCP connections
WiFiClient client;
WebSocketClient webSocketClient;

/**
 * indicator led pin
 * */
int led_pin = 2;

/**
 * flag state input pin
 * */
int pressureSensors[] = new int {1, 2, 3, 4};

bool last_state = false;
int blink_interval = 100;

/**
 * Last connection success state with the server
 * */
bool lastConnState = false;
long lastRequestedTime = -1;
int request_interval = 5000;

/**
 * Wifi network configuration
 * */
const char *ssid = ""; // Network SSID Here
const char *password = ""; // Network Password Here

/**
 * Backend host details
 * */

char path[] = "/";
char host[] = "192.168.1.2";
int port = 7171;

/**
 * Blinking state and last blink time to blink when not connected
 * to the server
 * */
long lastBlinkTime = -1;
bool lastBlinkStatus = true;

/**
 * Sync state with the server configurations
 * */
long lastSyncTime = -1;
int sync_interval = 750;

void setup() {

    // Setting up pin modes
    pinMode(led_pin, OUTPUT);

    // Setting up pin mode for pressure sensors
    for(int i = 0; i < pressureSensors.length; i++){
        pinMode(pressureSensors[i], INPUT);
    }

    // Initialize Serial communication
    Serial.begin(115200);

    //Initializing the wifi connection
    connectWifi();

    //Initializing the web socket connection
    connectWebSocket();
}

void loop() {

    indicate();

    /*
     * Try to reconnect if connection failed
     * */
    if(!lastConnState){
        tryReconnect();
        return;
    }

    sendState();
}

/**
 * Connecting to the wifi network
 * Trying to connect to the wifi network with a 500ms
 * interval
 * */
void connectWifi() {

    log("Connecting to the wifi network");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    log("connected");

}

/**
 * Initializing the web socket connection with the host
 *
 * Security: This websocket connection will use plain websocket
 * protocol ws to communicate with the server, using wss will
 * be more secure
 * */
void connectWebSocket(){

    // Connect to the websocket server
    if (client.connect(host, port)) {
        Serial.println("Connected");
    } else {
        Serial.println("Connection failed.");
        lastConnState = false;
        return;
    }

    // Handshake with the server
    webSocketClient.path = path;
    webSocketClient.host = host;

    if (webSocketClient.handshake(client)) {
        Serial.println("Handshake successful");
    } else {
        Serial.println("Handshake failed.");
        lastConnState = false;
        return;
    }

    lastConnState = true;

}

/**
 * This is the indicator of the network connectivity, the blink status
 * will be override if connection attempt is failed
 * */
void indicate() {

    /*
     * blink the light if connection is failed and
     * last blink time is more than the interval
     * */
    if(!lastConnState){

        if(millis() - lastBlinkTime > blink_interval){

            lastBlinkTime = millis();

            if(lastBlinkStatus){
                digitalWrite(led_pin, LOW);
            }else{
                digitalWrite(led_pin, HIGH);
            }

            lastBlinkStatus = !lastBlinkStatus;
        }

        return;
    }

}

/**
 * Try to reconnect to the wifi network on fail
 * */
void tryReconnect() {

    if(!lastConnState){
        if(millis() - lastRequestedTime > request_interval){
            lastRequestedTime = millis();
            connectWebSocket();
        }
    }

}

/**
 * Send state if synchronization interval is passed
 * */
void sendState(){

    if(millis() - lastSyncTime > sync_interval){
        lastSyncTime = millis();
        sendDataWS();
    }

}

/**
 * Prepare pressure sensors analog voltage read as a JSON array
 *
 * @return a json string contain voltage reads of pressure sensors
 * */
String prepareData() {

    String json = "[";

    for(int i = 0 ; i < pressureSensors.lenght; i++){

        json = json + anaglogRead(pressureSensors[i]);

        if(i != pressureSensors.length - 1){
            json += ",";
        }

    }

    json += "]";

    return json;

}

/**
 * Send data to the server using web socket protocol
 * */
void sendDataWS(){

    String json = prepareData();

    log("Sending the state");

    String data;

    if (client.connected()) {

        webSocketClient.getData(data);

        if (data.length() > 0) {
            Serial.print("Received data: ");
            Serial.println(data);
        }

        data = json;
        webSocketClient.sendData(data);

    } else {
        Serial.println("Client disconnected.");
        lastConnState = false;
    }

}

/**
 * Log required information to the serial port
 * for testing purposes
 * */
void log(String message){
    Serial.println(message);
}