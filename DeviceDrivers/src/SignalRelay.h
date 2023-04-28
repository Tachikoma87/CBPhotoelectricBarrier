/*****************************************************************************\
*                                                                           *
* File(s):                                           *
*                                                                           *
* Content:            *
*                                                                   *
*                                                                           *
*                                                                           *
* Author(s): Tom Uhlmann                                                    *
*                                                                           *
*                                                                           *
* The file(s) mentioned above are provided as is under the terms of the     *
* MIT License without any warranty or guaranty to work properly.            *
* For additional license, copyright and contact/support issues see the      *
* supplied documentation.                                                   *
*                                                                           *
\****************************************************************************/
#ifndef TFCHAMPION_SIGNALRELAY_H
#define TFCHAMPION_SIGNALRELAY_H

#ifdef ESP32

#include <Arduino.h>
#include <CC1101.h>
#include <RGBLed.h>
#include <BatteryIndicator.h>
#include <BluetoothSerial.h>
#include "Communication.hpp"

class SignalRelay{
public:
    const uint8_t RSSIRedPin = 25;
    const uint8_t RSSIGreenPin = 26;
    const uint8_t RSSIBluePin = 27;
    const uint8_t BatteryLedPin = 14;
    const uint8_t BatteryReadPin = 33;
    const uint8_t BluetoothLed = 15;
    const uint8_t Gdo0Pin = 2;

    SignalRelay(void);
    ~SignalRelay(void);

    void init(void);
    void clear(void);

    uint16_t update(void);


private:
    void processMsg(MsgStatus *pMsg);
    void processMsg(MsgPing *pMsg);
    void processMsg(MsgStartSignal *pMsg);

    void updateRSSI(int8_t RSSI);

    void processBluetoothPackage(void);

    ArduForge::CC1101 m_Transceiver;
    ArduForge::RGBLed m_RSSILed;
    ArduForge::BatteryIndicator m_BatterySensor;
    BluetoothSerial m_BTSerial;

    int8_t m_RSSI; ///< RSSI of last arrived message
    uint32_t m_LastMsgTimestamp; ///< Time of last message arrival
    uint32_t m_LastBTCheck;
    bool m_PackageAvailable; ///< Set by interrupt if data from CC1101 available
    uint8_t m_MsgBuffer[32]; ///< Message buffer for CC1101
    uint8_t m_BTMsgBuffer[128]; ///< Msg buffer for bluetooth
    uint8_t m_BTMsgPointer; ///< pointer to write next character read from bluetooth stream
    uint16_t m_Ping; ///< Estimate signal latency
    uint32_t m_LastPing;
    uint32_t m_LastBatteryCheck;


    uint32_t m_LastStartSignal;
};//SignalRelay

#endif  // ESP32
#endif // Guard macro