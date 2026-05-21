#include <Arduino.h>
#include <AccelStepper.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

//espnow protocol
typedef struct struct_message{
    int buttonComb;
    float x;
    float y;
} struct_message;

struct_message receivedData;

void dataReceived(const uint8_t * mac, const uint8_t *incomingData, int len){
    memcpy(&receivedData, incomingData, sizeof(receivedData));
}

#define sensitivity 1.5

//stepper motor
#define StepPin 5
#define DirectionPin 4
#define microStep 8
AccelStepper stepper(AccelStepper::DRIVER, StepPin, DirectionPin);

//servo motor
#define servoNumber 5
#define PWMMin 150
#define PWMMax 600
#define servoDelay 10 // adjust later
unsigned long lastServoUpdate = 0;
#define servoStep 1 //adjust later

int servoPins[servoNumber] = {0, 1, 2, 3, 4};
int servoCurrentPWM[servoNumber] = {450, 200, 550, 375, 500};

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

void setup() {
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        return;
        //add fail indicator
    }

    esp_now_register_recv_cb(dataReceived);

    Serial.begin(115200);
    Wire.begin(21, 22);
    pwm.begin();
    pwm.setPWMFreq(50);

    for (int i=0; i<=4; i++){
        pwm.setPWM(servoPins[i], 0, servoCurrentPWM[i]);
    }

    //adjust the below
    stepper.setMaxSpeed(20000);
    stepper.setAcceleration(40000);
    stepper.setCurrentPosition(0);
    stepper.setMinPulseWidth(2);

}

void loop() {
    bool activity = false;

    //reset
    if(receivedData.buttonComb == 1111){
        servoCurrentPWM[0]=450;
        servoCurrentPWM[1]=200;
        servoCurrentPWM[2]=550;
        servoCurrentPWM[3]=375;
        servoCurrentPWM[4]=500;
    }

    //base movement
    if(receivedData.x > sensitivity && activity == false && receivedData.buttonComb == 1110){
        stepper.setSpeed(20000);
        activity = true;
    }
    else if(receivedData.x < -sensitivity && activity == false && receivedData.buttonComb == 1110){
        stepper.setSpeed(-20000);
        activity = true;
    }
    else{
        stepper.setSpeed(0);
    }

    if(millis() - lastServoUpdate >= servoDelay){
        lastServoUpdate = millis();

        //shoulder
        if(receivedData.x >sensitivity && activity == false && receivedData.buttonComb == 1000){
            servoCurrentPWM[0] += servoStep;
            activity = true;
        }
        else if(receivedData.x < -sensitivity && activity == false && receivedData.buttonComb == 1000){
            servoCurrentPWM[0] -= servoStep;
            activity = true;
        }

        //elbow
        if(receivedData.x >sensitivity && activity == false && receivedData.buttonComb == 100){
            servoCurrentPWM[1] += servoStep;
            activity = true;
        }
        else if(receivedData.x < -sensitivity && activity == false && receivedData.buttonComb == 100){
            servoCurrentPWM[1] -= servoStep;
            activity = true;  
        }

        //wrist
        if(receivedData.x >sensitivity && activity == false && receivedData.buttonComb == 10){
            servoCurrentPWM[2] += servoStep;
            activity = true;
        }
        else if(receivedData.x < -sensitivity && activity == false && receivedData.buttonComb == 10){
            servoCurrentPWM[2] -= servoStep;
            activity = true;  
        }

        //hand
        if(receivedData.x >sensitivity && activity == false && receivedData.buttonComb == 1){
            servoCurrentPWM[3] += servoStep;
            activity = true;
        }
        else if(receivedData.x < -sensitivity && activity == false && receivedData.buttonComb == 1){
            servoCurrentPWM[3] -= servoStep;
            activity = true; 
        }

        //gripper
        if(receivedData.x >sensitivity && activity == false && receivedData.buttonComb == 1100){
            servoCurrentPWM[4] += servoStep;
            activity = true; 
        }
        else if(receivedData.x < -sensitivity && activity == false && receivedData.buttonComb == 1100){
            servoCurrentPWM[4] -= servoStep;
            activity = true;
        }
    }

    stepper.runSpeed();
    for (int i=0; i<=4; i++){
        servoCurrentPWM[i] = constrain(servoCurrentPWM[i], PWMMin, PWMMax);
        pwm.setPWM(i, 0, servoCurrentPWM[i]);
    }
}