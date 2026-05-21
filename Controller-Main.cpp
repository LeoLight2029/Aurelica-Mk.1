#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(1);

//espnow protocol
typedef struct struct_message{
    int buttonComb;
    float x;
    float y;
} struct_message;

uint8_t receiverAddress[] = {0x78, 0x1C, 0x3C, 0xE5, 0x59, 0x7C};
struct_message dataToSend;
esp_now_peer_info_t peerInfo;

void sendData(const uint8_t*mac_addr, esp_now_send_status_t status) {
    //add espnow fail indicator here
}

// led 
#define rPin  17
#define gPin 18
#define bPin 19

#define rChannel 0
#define gChannel 1
#define bChannel 2

#define pwm 5000
#define resolution 8

void setColor (int r, int g, int b){
    ledcWrite(rChannel, r);
    ledcWrite(gChannel, g);
    ledcWrite(bChannel, b);
}

//buttons
int buttonPin[4]={14, 27, 26, 25};
bool buttonBool[4];

//accelerometer
float xNeutral = 0;
float yNeutral = 0;
float zNeutral = 0;
#define sensitivity 1.5 //callibrate this later


void setup() {
    Serial.begin(115200);
    Wire.begin(21,22);

    //esp now protocol
    WiFi.mode(WIFI_STA);

    if(esp_now_init() != ESP_OK) {
        Serial.println("Error initilizing ESPNOW");
        return;

    }

    esp_now_register_send_cb(sendData);

    memcpy(peerInfo.peer_addr, receiverAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if(esp_now_add_peer(&peerInfo) != ESP_OK){
        Serial.println("Failed to add peer");
        return;

    }


    //led
    ledcSetup(rChannel, pwm, resolution);
    ledcSetup(gChannel, pwm, resolution);
    ledcSetup(bChannel, pwm, resolution);

    ledcAttachPin(rPin, rChannel);
    ledcAttachPin(gPin, gChannel);
    ledcAttachPin(bPin, bChannel);
    
    //buttons
    for(int i=0;i<4; i++){
        pinMode(buttonPin[i], INPUT_PULLDOWN);
    }
   
    //accelerometer
    if(!accel.begin()){
        setColor(255,0,0);
        while(1);
    }

    accel.setRange(ADXL345_RANGE_4_G);

    //accelerometer callibration first time
    setColor(0,255,0);
    delay(3000);
    sensors_event_t event;
    accel.getEvent(&event);

    xNeutral=event.acceleration.x;
    yNeutral=event.acceleration.y;
    zNeutral=event.acceleration.z;
    setColor(255,255,255);
}

void loop() {
    //buttons
    for(int i =0;i<4;i++){
        buttonBool[i]=digitalRead(buttonPin[i]);
    }
    int buttonComb = buttonBool[0]*1000+buttonBool[1]*100+buttonBool[2]*10+buttonBool[3];

    //acelerometer
    sensors_event_t event;
    accel.getEvent(&event);

    float xCalibrated = event.acceleration.z - zNeutral;
    float yCalibrated = event.acceleration.y - yNeutral;

    bool xChange = false;
    bool yChange = false;

    //button controls
    if(buttonComb==1001){
        setColor(0,255,0);
        delay(1000);
        sensors_event_t event;
        accel.getEvent(&event);

        xNeutral=event.acceleration.x;
        yNeutral=event.acceleration.y;
        zNeutral=event.acceleration.z;
        setColor(255,255,255);
    }

    dataToSend.buttonComb = buttonComb;
    dataToSend.x= xCalibrated;
    dataToSend.y = yCalibrated;
    esp_now_send(receiverAddress, (uint8_t *) &dataToSend, sizeof(dataToSend));

    delay(30);
}
