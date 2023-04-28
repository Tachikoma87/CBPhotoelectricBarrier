#include "SignalRelay.h"
#ifdef ESP32
#include "BTCommunication.hpp"
#include <driver/adc.h>

SignalRelay::SignalRelay(void){
  
}//Constructor

SignalRelay::~SignalRelay(void){

}//Destructor

void SignalRelay::init(void){
    #ifdef DEBUG
    Serial.print("Staring initialization of signal relay...\n");
    #endif 

    // battery led
    // important note: The ADC of the ESP32 is calibrated internally and not very reliable out of the box. Calibrating seems to be a sophisticated procedure. The 1.07 scaling factor
    // seems to be working fine and was found by trial and error (comparing read value with that of a multimeter)
    m_BatterySensor.begin(BatteryReadPin, 2, 47000, 22000, ArduForge::BatteryIndicator::TYPE_LIPO, 3.3f*1.07f);
    m_LastBatteryCheck = 0;
    pinMode(BatteryLedPin, OUTPUT);
    digitalWrite(BatteryLedPin, LOW);
    #ifdef DEBUG
    Serial.print("\tBattery sensor and pin initialized...\n");
    #endif

    // initialize transceiver
    #ifdef DEBUG
    Serial.print("\tInitializing CC1101...");
    #endif
    m_Transceiver.begin(CC1101RelayAddress, Gdo0Pin, ArduForge::CC1101::F_433, 0xFF, SS, MOSI, MISO, SCK);
    m_Transceiver.outputPowerLevel(10);
    m_Transceiver.receiveMode();

    #ifdef DEBUG
    if(m_Transceiver.deviceAddress() != CC1101RelayAddress) Serial.print("failed\n");
    else Serial.print("done\n");
    #endif
     
    // RSSI Led
    m_RSSILed.begin(RSSIRedPin, RSSIGreenPin, RSSIBluePin);
    m_RSSILed.color(0,0,0);
    #ifdef DEBUG
    Serial.print("\tRSSI RGB led initialized...\n");
    #endif

    
    #ifdef DEBUG
    Serial.print("\tFiring up bluetooth...");
    #endif
    m_BTMsgPointer = 0;
    memset(m_BTMsgBuffer, 0, sizeof(m_BTMsgBuffer));
    m_BTSerial.begin("T&F Flap Relay");
    pinMode(BluetoothLed, OUTPUT);
    digitalWrite(BluetoothLed, LOW);
    #ifdef DEBUG
    Serial.print("done\n");
    #endif


    #ifdef DEBUG
    Serial.print("Signal relay initialized!\n");
    #endif

    m_LastMsgTimestamp = 0;
    updateRSSI(0);


}//initialize

void SignalRelay::clear(void){

}//clear

uint16_t SignalRelay::update(void){
    // if a CC1101 message is available we have to process it
    if(m_Transceiver.dataAvailable()){
        uint8_t Receiver = 0x00;
        uint8_t Sender = 0x00;
        const uint8_t DataSize = m_Transceiver.receiveData(m_MsgBuffer, &Receiver, &Sender, &m_RSSI);
        m_Transceiver.receiveMode();
        m_LastMsgTimestamp = millis();
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


    if( ((millis() - m_LastMsgTimestamp) > 5000 || m_RSSI == 0) && (millis() - m_LastPing) > 250 ){
        // ping signal relay
        MsgPing Ping;
        m_LastPing = millis();
        Ping.request(true);
        Ping.timestamp(m_LastPing);
        uint8_t DataSize = Ping.toStream(m_MsgBuffer);
        m_Transceiver.sendData(CC1101FlapAddress, m_MsgBuffer, DataSize);
        m_Transceiver.receiveMode();
        #ifdef DEBUG
        Serial.print("Pinging starting flap ...\n");
        #endif    
    }

    if(m_RSSI != 0 && (millis() - m_LastMsgTimestamp) > 10000){
        // we lost the signal
        updateRSSI(0);
    }

    if(millis() - m_LastBTCheck > 2000){
        m_LastBTCheck = millis();
        digitalWrite(BluetoothLed, m_BTSerial.hasClient() ? HIGH : LOW);
    }

    if(millis() - m_LastBatteryCheck > 30000){
        m_LastBatteryCheck = millis();
        uint8_t Charge = m_BatterySensor.charge();
        if(Charge < 15) digitalWrite(BatteryLedPin, HIGH);
        else digitalWrite(BatteryLedPin, LOW);
   
        #ifdef DEBUG
        float Voltage = m_BatterySensor.read();
        uint16_t Reading = analogRead(BatteryReadPin);
        Serial.print("Battery: "); Serial.print(Charge); Serial.print("% ("); Serial.print(Voltage); Serial.print("V)"); Serial.print(" / "); Serial.print(Reading); Serial.print("\n");
        #endif
    }

    // got something on the Bluetooth channel?
    while(m_BTSerial.available()){
        m_BTMsgBuffer[m_BTMsgPointer] = m_BTSerial.read();
        m_BTMsgPointer++;
        if(m_BTMsgBuffer[m_BTMsgPointer-1] == BTMessage::cmdCharacter()){
            // package arrived
            processBluetoothPackage();
            // reset message pointer
            m_BTMsgPointer = 0;
        }
    }//while[read BT stream]

    return 0;
}//update

void SignalRelay::processBluetoothPackage(void){
    // package arrived 
    BTMessage::BTPackageType Type = BTMessage::fetchPackageTypeFromStream(m_BTMsgBuffer, m_BTMsgPointer);
    switch(Type){
        case BTMessage::BTPCK_PING:{
            BTMessagePing PingIn;
            PingIn.fromStream(m_BTMsgBuffer, m_BTMsgPointer);
            BTMessagePing PingOut = PingIn;
            PingOut.request(false);
            PingOut.timestamp(PingIn.timestamp() - m_Ping);
            uint32_t MsgSize = PingOut.toStream(m_BTMsgBuffer, sizeof(m_BTMsgBuffer));
            m_BTSerial.write(m_BTMsgBuffer, MsgSize);
        }break;
        case BTMessage::BTPCK_STARTSIGNAL:{
            // repack to CC1101 message and relay to flap
            BTMessageStartSignal SigIn;
            SigIn.fromStream(m_BTMsgBuffer, m_BTMsgPointer);

            MsgStartSignal SigOut;
            SigOut.request(SigIn.request());
            SigOut.acknowledge(SigIn.acknowledge());
            SigOut.timestamp(SigIn.timestamp());
            uint8_t PayloadSize = SigOut.toStream(m_MsgBuffer);
            m_Transceiver.sendData(CC1101FlapAddress, m_MsgBuffer, PayloadSize);
            m_Transceiver.receiveMode();
            #ifdef DEBUG
            Serial.print("Relayed Bluetooth start signal package\n");
            #endif

        }break;
        case BTMessage::BTPCK_STATUS:{
            BTMessageStatus StatusIn;
            StatusIn.fromStream(m_BTMsgBuffer, m_BTMsgPointer);

            MsgStatus StatusOut;
            StatusOut.request(StatusIn.request());
            StatusOut.remoteClock(StatusIn.androidClock());

            uint8_t PayloadSize = StatusOut.toStream(m_MsgBuffer);
            m_Transceiver.sendData(CC1101FlapAddress, m_MsgBuffer, PayloadSize);
            m_Transceiver.receiveMode();
            #ifdef DEBUG
            Serial.print("Relayed status request package\n");
            #endif
        }break;
        default:{
            #ifdef DEBUG
            Serial.print("Got unknown Bluetooth package type: "); Serial.print(Type); Serial.print("\n");
            #endif
        } break;
    }//switch[package type]

}//processBluetoothPackage


void SignalRelay::processMsg(MsgStatus *pMsg){
    if(nullptr == pMsg) return;

    if(!pMsg->request()){
        // translate to Bluetooth message and relay
        BTMessageStatus MsgOut;
        MsgOut.request(pMsg->request());
        MsgOut.rssi(pMsg->rssi());
        MsgOut.androidClock(pMsg->remoteClock());
        MsgOut.deviceClock(pMsg->internalClock());
        MsgOut.batteryChargeRelay(m_BatterySensor.charge());
        MsgOut.batteryChargeDevice(pMsg->charge());

        uint8_t *pBuffer = new uint8_t[MsgOut.size()];
        uint8_t MsgSize = MsgOut.toStream(pBuffer, MsgOut.size());
        m_BTSerial.write(pBuffer, MsgSize);
        delete[] pBuffer;
    }else{
        // should not happen
    }

}//processMsg

void SignalRelay::processMsg(MsgPing *pMsg){
    if(nullptr == pMsg) return;

    if(pMsg->request()){
        MsgPing R;
        R.request(false);
        R.timestamp(pMsg->timestamp());
        uint8_t DataSize = R.toStream(m_MsgBuffer);
        m_Transceiver.sendData(CC1101FlapAddress, m_MsgBuffer, DataSize);
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

void SignalRelay::processMsg(MsgStartSignal *pMsg){
    if(nullptr == pMsg) return;
    // repack and relay to connected Bluetooth device
    BTMessageStartSignal SigOut;
    SigOut.request(pMsg->request());
    SigOut.acknowledge(pMsg->acknowledge());
    SigOut.timestamp(pMsg->timestamp());

    uint8_t *pOutBuffer = new uint8_t[SigOut.size()];
    uint32_t MsgSize = SigOut.toStream(pOutBuffer, sizeof(m_BTMsgBuffer));
    m_BTSerial.write(pOutBuffer, MsgSize);
    delete[] pOutBuffer;
    
}//processMsg

void SignalRelay::updateRSSI(int8_t RSSI){
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
}//updateRSSIColor


#endif //ESP32