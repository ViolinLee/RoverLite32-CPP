#include <Arduino.h>
#include <PinDefines.h>
#include <controller_index.h>
#include <Wifi.h>
#include <ESPAsyncWebServer.h>
#include <motor.h>
#include <camera.h>
#include <MecanumWheelRobot.h>
#include <iostream>
#include <sstream>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


const char* ssid = "RoverLite";
const char* password = "leesophia666";

AsyncWebServer server(80);
AsyncWebSocket wsCameraStream("/stream");
AsyncWebSocket wsRoverCmd("/cmd");
TaskHandle_t StreamTaskHandle;
uint32_t streamClientId = 0;
uint8_t lampState = 0;
uint8_t lastPercentage = 100;
uint8_t batPercentage = 100;
const TickType_t xDelay1ms = pdMS_TO_TICKS( 1 );


// OV2640 Camera
Camera ov2640;

// Motors
Motor *motorFL = new Motor(1); 
Motor *motorFR = new Motor(-1);
Motor *motorBL = new Motor(1);
Motor *motorBR = new Motor(1);

// Robot
MecanumWheelRobot roverLite(0.0465, 0.02626, 0.0295);

// Function Declaration
void handleRoot(AsyncWebServerRequest *request);
void handleNotFound(AsyncWebServerRequest *request);
void notifyClients();
void onRoverCmdWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void onStreamWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void batteryMonitorLoop();
void sendCameraImage();
void streamTask( void * parameter );

void setup() {
    Serial.begin(115200);
    Serial.printf("maxTurnOmega: %d vel_Max: %d\n", roverLite.maxTurnOmega, roverLite.vel_Max);

    pinMode(BJT_LAMP, OUTPUT);
    digitalWrite(BJT_LAMP, LOW);

    motorFL->initialize(FRONTAL_LEFT_MOTOR_A, 0, FRONTAL_LEFT_MOTOR_B, 1);
    motorFR->initialize(FRONTAL_RIGHT_MOTOR_A, 2, FRONTAL_RIGHT_MOTOR_B, 3);  //78冲突
    motorBL->initialize(BACK_LEFT_MOTOR_A, 4, BACK_LEFT_MOTOR_B, 5);
    motorBR->initialize(BACK_RIGHT_MOTOR_A, 6, BACK_RIGHT_MOTOR_B, 7);

    roverLite.attachMotor(motorFL, motorFR, motorBL, motorBR);
    roverLite.standBy();

    float isCameraInitialized = ov2640.initialize();
    if (!isCameraInitialized) {while(1) {}}

    WiFi.softAP(ssid, password);

    // ov2640.startCameraServer();
    Serial.print("Camera Ready! AP IP address: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", HTTP_GET, handleRoot);
    server.onNotFound(handleNotFound);
        
    wsCameraStream.onEvent(onStreamWebSocketEvent);
    server.addHandler(&wsCameraStream);

    wsRoverCmd.onEvent(onRoverCmdWebSocketEvent);
    server.addHandler(&wsRoverCmd);

    server.begin();
    Serial.println("HTTP server started");

    xTaskCreatePinnedToCore(streamTask, "streamTask",  10000,  NULL,  1, &StreamTaskHandle, 1);
    Serial.println("Streaming task begin!");
}


void loop() {
    //wsCameraStream.cleanupClients();
    //wsRoverCmd.cleanupClients();
    // sendCameraImage();

}

void sendCameraImage() {
    if (streamClientId == 0) {
        return;
    }

    camera_fb_t * fb = esp_camera_fb_get(); // ov2640.camera_fb_get();
    if (!fb) {
        Serial.println("Frame buffer could not be acqured");
        return;
    } else {} //Serial.println("Frame buffer is acqured");}

    wsCameraStream.binary(streamClientId, fb->buf, fb->len);
    esp_camera_fb_return(fb);  //ov2640.camera_fb_return(fb);
    // Serial.println("Frame empty!");

    while (true) {
        AsyncWebSocketClient *cp = wsCameraStream.client(streamClientId);
        if (!cp || !(cp->queueIsFull())) {
            break;
        }
        // delay(1);
    }
}

void handleRoot(AsyncWebServerRequest *request) {
    // request->send_P(200, "text/html", (const char*)controller_index_html_gz);
    // Serial.println("handle root!");
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", controller_index_html_gz, controller_index_html_gz_len);
    response->addHeader("Content-Encoding","gzip");
    request->send(response);
}

void handleNotFound(AsyncWebServerRequest *request) {
    request->send_P(404, "text/plain", "File Not Found");
}

void notifyClients() {
    const uint8_t size = JSON_OBJECT_SIZE(1);
    StaticJsonDocument<size> json;
    json["percentage"] = batPercentage;

    char buffer[17];
    size_t len = serializeJson(json, buffer);
    wsRoverCmd.textAll(buffer, len);
}

void onRoverCmdWebSocketEvent(AsyncWebSocket *server, 
                              AsyncWebSocketClient *client, 
                              AwsEventType type,
                              void *arg, 
                              uint8_t *data, 
                              size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            roverLite.standBy();  
            break;
        case WS_EVT_DATA:
            AwsFrameInfo *info;
            info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                /* std::string cmdData = "";
                cmdData.assign((char*)data, len);
                std::istringstream ss(cmdData);
                std::string L_XPos, L_YPos, R_XPos, R_YPos, Lamp_SW;
                Serial.printf("LXPos [%s] LYPos [%s] RXPos [%s] R_YPos [%s] Lamp [%s]\n", 
                              L_XPos.c_str(), L_YPos.c_str(), R_XPos.c_str(), R_YPos.c_str(), Lamp_SW.c_str());

                int L_X = atoi(L_XPos.c_str());
                int L_Y = atoi(L_YPos.c_str());
                int R_X = atoi(R_XPos.c_str());
                int R_Y = atoi(R_YPos.c_str()); */

                StaticJsonDocument<128> json;
                DeserializationError err = deserializeJson(json, data);
                if (err) {
                    Serial.print(F("deserializeJson() failed with code "));
                    Serial.println(err.c_str());
                    return;
                }
                
                if (json.containsKey("Lamp_SW")) {
                    int8_t Lamp_SW = json["Lamp_SW"];

                    if (lampState != Lamp_SW) {
                        lampState = Lamp_SW; 
                        digitalWrite(BJT_LAMP, lampState);
                        Serial.printf("receive Lamp_SW: %d", Lamp_SW);
                    }
                } else {
                    int8_t L_X = json["L_XPos"];
                    int8_t L_Y = json["L_YPos"];
                    int8_t R_X = json["R_XPos"];
                    R_X = -1 * R_X;
                    L_X = -1 * L_X;
                    // Serial.printf("receive L_X L_Y R_X: %d, %d, %d", L_X, L_Y, R_X);
                    // Serial.println();

                    roverLite.parseCommand(L_X, L_Y, R_X);
                    roverLite.move();
                }
                
                if (abs(lastPercentage - batPercentage) > 3) {
                    lastPercentage = batPercentage;
                    notifyClients();
                }
            }
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
        default:
            break;
    }
}

void onStreamWebSocketEvent(AsyncWebSocket *server, 
                            AsyncWebSocketClient *client, 
                            AwsEventType type,
                            void *arg, 
                            uint8_t *data, 
                            size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            streamClientId = client->id();
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            streamClientId = 0;
            break;
        case WS_EVT_DATA:
            break;
        case WS_EVT_PONG:
            break;
        case WS_EVT_ERROR:
            break;
        default:
            break;
    }
}

void streamTask( void * parameter ) {     
  while ( true ){
    sendCameraImage();
    vTaskDelay( xDelay1ms );
  }
  vTaskDelete(StreamTaskHandle);
}

void batteryMonitorLoop() {
    int batValue = 0;
    float adcVoltage = 0;
    float batVoltage = 0;

    batValue = analogRead(BAT_ADC);
    // Serial.println(batValue);
    adcVoltage = batValue / 4096.0 * 2.6;
    // Serial.println(adcVoltage);
    batVoltage = adcVoltage * 3;
    Serial.printf("Battery Voltage: [%f]", batVoltage);
    
    batPercentage = batVoltage * 100 / 8.4;

    delay(10000);
}