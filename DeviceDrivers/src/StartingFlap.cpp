
#ifndef ESP32
#include "StartingFlap.h"
#include <avr/sleep.h>


StartingFlap::StartingFlap(void){
    
}//constructor

StartingFlap::~StartingFlap(void){

}//destructor

void StartingFlap::init(void){
    #ifdef DEBUG
    Serial.print("Initializing starting flap... \n");
    #endif

    // initialize transceiver
    #ifdef DEBUG
    Serial.print("\tInitializing CC1101 Transceiver...");
    #endif
    m_Transceiver.begin(CC1101FlapAddress, Gdo0Pin);
    m_Transceiver.outputPowerLevel(10);
    m_Transceiver.receiveMode();
    
    #ifdef DEBUG
    // communication working or did something go wrong?
    if(m_Transceiver.deviceAddress() != CC1101FlapAddress) Serial.print("failed\n");
    else Serial.print("success\n");
    #endif

    // initializing RSSI Led
    m_RSSILed.begin(RSSIRedPin, RSSIGreenPin, RSSIBluePin, true);
    #ifdef DEBUG
    Serial.print("\tRSSI Led initialized...done\n");
    #endif

    // initialize hall sensor
    pinMode(HallPowerPin, OUTPUT);
    digitalWrite(HallPowerPin, HIGH);
    m_HallSensor.begin(HallPin, false);
    #ifdef DEBUG
    Serial.print("\tHall sensor initialized...done\n");
    #endif 

    pinMode(StartingSignalLedPin, OUTPUT);
    pinMode(BatteryLedPin, OUTPUT);
    digitalWrite(StartingSignalLedPin, LOW);
    digitalWrite(BatteryLedPin, LOW);  
    m_BatterySensor.begin(BatteryReadPin, 2, 47000, 47000, ArduForge::BatteryIndicator::TYPE_LIPO, 4.75f);
    updateBattery();
    #ifdef DEBUG
    Serial.print("\tBattery led and starting signal led initialized...done\n");
    #endif
    #ifdef DEBUG
    Serial.print("Starting flap initialized!\n");
    #endif

    memset(m_MsgBuffer, 0, sizeof(m_MsgBuffer));
    updateRSSI(0);
    m_FlapStatus = STATE_UNKNOWN;
}//initialize

void StartingFlap::clear(void){

}//clear

void StartingFlap::update(void){
    // if a message is available we have to process it
    if(m_Transceiver.dataAvailable()){
        uint8_t Receiver = 0x00;
        uint8_t Sender = 0x00;
        m_Transceiver.receiveData(m_MsgBuffer, &Receiver, &Sender, &m_RSSI);
        m_Transceiver.receiveMode();
        m_LastMessageTimestamp = millis();
        updateRSSI(m_RSSI);

        switch(CC1101Message::cmdFromStream(m_MsgBuffer)){
            case CC1101Message::CMD_PING:{
                MsgPing Msg;
                Msg.fromStream(m_MsgBuffer);
                processMsg(&Msg);
            }break;
            case CC1101Message::CMD_STATUS:{
                MsgStatus Msg;
                Msg.fromStream(m_MsgBuffer);
                processMsg(&Msg);
            }break;
            case CC1101Message::CMD_START_SIGNAL:{
                MsgStartSignal Msg;
                Msg.fromStream(m_MsgBuffer);
                processMsg(&Msg);
            }break;
            default:{
                #ifdef DEBUG
                Serial.print("Invalid message with CMD-ID: "); Serial.print(CC1101Message::cmdFromStream(m_MsgBuffer)); Serial.print(" received!\n");
                #endif
            }break;
        }//switch[Message]
    }//if[CC1101 data available]


    if( ((millis() - m_LastMessageTimestamp) > 5100 || m_RSSI == 0) && (millis() - m_LastPing) > 250 ){
        // ping signal relay
        MsgPing Ping;
        m_LastPing = millis();
        Ping.request(true);
        Ping.timestamp(m_LastPing);
        uint8_t DataSize = Ping.toStream(m_MsgBuffer);
        m_Transceiver.sendData(CC1101RelayAddress, m_MsgBuffer, DataSize);
        m_Transceiver.receiveMode();
        #ifdef DEBUG
        Serial.print("Pinging signal relay ...\n");
        #endif    
    }

    // did we loose signal completely?
    if(m_RSSI != 0 && (millis() - m_LastMessageTimestamp) > 10000){
        // we lost the signal
        updateRSSI(0);
    }

    // check battery (every 5 minutes)
    if( (millis() - m_LastBatteryCheck) > (uint32_t)(30 * 1000L)){
        updateBattery();
    }

    // what's going on with our flap?
    if(m_FlapStatus == STATE_WAITFOROPEN && 1 == m_HallSensor.read(0)){
        // make sure next state is not activated by accident
        delay(1000);
        m_FlapStatus = STATE_GO;
        digitalWrite(StartingSignalLedPin, HIGH);
        #ifdef DEBUG
        Serial.print("Starting flap armed!\n");
        #endif
    }else if(m_FlapStatus == STATE_GO && 0 == m_HallSensor.read(0)){ 
        MsgStartSignal Msg;
        Msg.request(false);
        Msg.acknowledge(false);
        Msg.timestamp(millis());
        uint8_t PayloadSize = Msg.toStream(m_MsgBuffer);
        m_Transceiver.sendData(CC1101RelayAddress, m_MsgBuffer, PayloadSize);
        m_Transceiver.receiveMode();
        m_FlapStatus = STATE_WAITFORACK;
        m_LastAckCheck = millis();
        digitalWrite(StartingSignalLedPin, LOW);   
        updateRSSI(m_RSSI);
        #ifdef DEBUG
        Serial.print("Go!\n");
        #endif
    }else if(m_FlapStatus == STATE_WAITFORACK && millis() - m_LastAckCheck > 250){
        // no acknowledge yet, send start signal again
        MsgStartSignal Msg;
        Msg.request(false);
        Msg.acknowledge(false);
        Msg.timestamp(millis());
        uint8_t PayloadSize = Msg.toStream(m_MsgBuffer);
        m_Transceiver.sendData(CC1101RelayAddress, m_MsgBuffer, PayloadSize);
        m_Transceiver.receiveMode();
        m_FlapStatus = STATE_WAITFORACK;
        m_LastAckCheck = millis();
        #ifdef DEBUG
        Serial.print("Need confirmation!");
        #endif
    }

    if(m_FlapStatus == STATE_UNKNOWN || m_FlapStatus == STATE_OPEN || m_FlapStatus == STATE_CLOSED) enterSleepMode();

}//update

void StartingFlap::processMsg(MsgStartSignal *pMsg){
    if(nullptr == pMsg) return;

    if(pMsg->request() && pMsg->timestamp() == 0){
        // here we go, let's get started
        // is flap open or do we have to wait for it to open?
        if(0 == m_HallSensor.read(0)) m_FlapStatus = STATE_WAITFOROPEN;
        else{
            m_FlapStatus = STATE_GO;
            digitalWrite(StartingSignalLedPin, HIGH);
        }

        // let's send back the ack that we received the starting signal
        pMsg->request(true);
        pMsg->timestamp(0);
        pMsg->acknowledge(true);
        uint8_t PayloadSize = pMsg->toStream(m_MsgBuffer);
        m_Transceiver.sendData(CC1101RelayAddress, m_MsgBuffer, PayloadSize);
        m_Transceiver.receiveMode();
        // turn RSSI Led white
        updateRSSI(m_RSSI);
        #ifdef DEBUG
        Serial.print("Got starting signal.\n");
        #endif
    }else if(!pMsg->request() && pMsg->acknowledge() && pMsg->timestamp() != 0){
        // we received confirmation, that starting signal has arrived
        m_FlapStatus = (0 == m_HallSensor.read(0)) ? STATE_CLOSED : STATE_OPEN;
        #ifdef DEBUG
        Serial.print("Received starting signal Ack!");
        #endif
    }

}//processMsg

void StartingFlap::processMsg(MsgStatus *pMsg){
    if(nullptr == pMsg) return;
    if(pMsg->request()){
        pMsg->request(false);
        pMsg->charge(m_BatterySensor.charge());
        pMsg->internalClock(millis());
        pMsg->rssi(m_RSSI);
        uint8_t PayloadSize = pMsg->toStream(m_MsgBuffer);
        m_Transceiver.sendData(CC1101RelayAddress, m_MsgBuffer, PayloadSize);
        m_Transceiver.receiveMode();
        #ifdef DEBUG
        Serial.print("Answered to status request\n");
        #endif
    }else{
        // should not happen here
    }

}//processMsg

void StartingFlap::processMsg(MsgPing *pMsg){
    if(nullptr == pMsg) return;

    if(pMsg->request()){
        MsgPing R;
        R.request(false);
        R.timestamp(pMsg->timestamp());
        uint8_t DataSize = R.toStream(m_MsgBuffer);
        m_Transceiver.sendData(CC1101RelayAddress, m_MsgBuffer, DataSize);
        m_Transceiver.receiveMode();
        #ifdef DEBUG
        Serial.print("Sent ping back\n");
        #endif
    }else{
        #ifdef DEBUG
        // got a ping back
        uint32_t Latency = millis() - pMsg->timestamp();
        Serial.print("Got ping back with latency of "); Serial.print(Latency); Serial.print(" ms\n");
        #endif
    }

}//processMsg

void StartingFlap::updateRSSI(int8_t RSSI){
    m_RSSI = RSSI;
    if(0 == m_RSSI){
        // no connection at al
        m_RSSILed.color(255, 0, 0);
    }else if(m_RSSI > -60){
        // goo quality signal
        m_RSSILed.color(0, 255, 0);
    }else if (m_RSSI > -90){
        // medium to bad signal
        const float Alpha = -(m_RSSI+60.0)/30.0;
        uint8_t Green = (uint8_t)(Alpha * 0  + (1.0 - Alpha)*200);
        m_RSSILed.color(255, Green, 0);
    }else{
        m_RSSILed.color(255, 0, 0);
    }

    // if we want to start the race and signal quality if sufficient, we turn RSSI white
    if(STATE_GO == m_FlapStatus || STATE_WAITFOROPEN == m_FlapStatus) m_RSSILed.color(255, 255, 255);

}//updateRSSI

void StartingFlap::updateBattery(void){
    if(m_BatterySensor.charge() < 15){
        digitalWrite(BatteryLedPin, HIGH);
    }else{
        digitalWrite(BatteryLedPin, LOW);
    }

    m_LastBatteryCheck = millis();
}//updateBattery

void StartingFlap::enterSleepMode(void){
    attachInterrupt(digitalPinToInterrupt(StartingFlap::Gdo0Pin), StartingFlap::transceiverInterrupt , HIGH);
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();
    sleep_mode();
    sleep_disable();
}//enterSleepMode

void StartingFlap::transceiverInterrupt(void){
    detachInterrupt(digitalPinToInterrupt(StartingFlap::Gdo0Pin));
}//transceiverInterrupt

#endif //ESP32