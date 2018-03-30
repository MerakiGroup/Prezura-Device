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
int state_pin = 0;
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
const char *ssid = "SLT";
const char *password = "1997ashan";

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
    pinMode(state_pin, INPUT);

    // Initialize Serial communication
    Serial.begin(115200);

    //Initializing the wifi connection
    connectWifi();

    //Initializing the web socket connection
    connectWebSocket();

    sendState();
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
 * Check if the flag exist on the base
 * @return true if the flag exist and false otherwise
 * */
bool hasFlag() {

    if(digitalRead(state_pin) == HIGH){
        return true;
    }

    return false;
}

/**
 * Connecting to the wifi network
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
 * Initializing the socket connection with the host
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
 * This method will indicate if flag state
 * using the led
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

    if (hasFlag()) {
        digitalWrite(led_pin, LOW);
    } else {
        digitalWrite(led_pin, HIGH);
    }

}

/**
 * This method will send flag state to the server
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
 * Send data to the server using web socket protocol
 * */
void sendDataWS(){

    String json;

    if (hasFlag()) {
        json = "{\"id\": 1, \"state\": true}";
    } else {
        json = "{\"id\": 1, \"state\": false}";
    }

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
 * Send data to the server using HTTP protocol
 *
 * TODO implement
 * */
void sendDataHTTP(){

}

void log(String message){
    Serial.println(message);
}