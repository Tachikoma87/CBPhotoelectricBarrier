#include <Arduino.h>

#ifdef FLAP
#include "StartingFlap.h"

StartingFlap SFlap;

void setup() {
    #ifdef DEBUG
    Serial.begin(115200);
    #endif 

    SFlap.init();
}//setup

void loop() {
    SFlap.update();
}//loop
#endif


#ifdef RELAY
#include "SignalRelay.h"

SignalRelay Relay;

void setup(){
    #ifdef DEBUG
    Serial.begin(115200);
    #endif 

    Relay.init();
}//setup


void loop(){
    Relay.update();

    
}//loop

#endif


#ifdef OPTICALBARRIER
#include "OpticalBarrier.h"

OpticalBarrier Barrier;

void setup(void){
    #ifdef DEBUG
    Serial.begin(115200);
    #endif

    Barrier.init(1);
}//setup

void loop(){
    Barrier.update();
}//loop

#endif