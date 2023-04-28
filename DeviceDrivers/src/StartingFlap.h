/*****************************************************************************\
*                                                                           *
* File(s):                                          *
*                                                                           *
* Content:           *
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
#ifndef TFCHAMPION_STARTINGFLAP_H
#define TFCHAMPION_STARTINGFLAP_H

#include <Arduino.h>
#include <CC1101.h>
#include <RGBLed.h>
#include <BatteryIndicator.h>
#include <BinarySensor.h>
#include "Communication.hpp"

class StartingFlap{
public:
    enum PinConfig: uint8_t{
        Gdo0Pin = 2,
        HallPin = 3,
        RSSIRedPin = 4,
        RSSIGreenPin = 5,
        RSSIBluePin = 6,
        BatteryLedPin = 7,
        BatteryReadPin = 2,
        StartingSignalLedPin = 8,
        HallPowerPin = 9
    };
    
    StartingFlap(void);
    ~StartingFlap(void);

    void init(void);
    void clear(void);

    void update(void);
 
private:
    enum State: uint8_t{
        STATE_UNKNOWN = 0,
        STATE_WAITFOROPEN,
        STATE_OPEN,
        STATE_GO, // race starts on next closing of flap
        STATE_WAITFORACK, // we still need to confirm that our starting signal arrived
        STATE_CLOSED,
    };//State

    void processMsg(MsgStartSignal *pMsg);
    void processMsg(MsgStatus *pMsg);
    void processMsg(MsgPing *pMsg);

    void updateRSSI(int8_t RSSI);
    void updateBattery(void);

    static void transceiverInterrupt(void);
    void enterSleepMode(void);

    ArduForge::CC1101 m_Transceiver; ///< 433 MHz Communication transceiver
    ArduForge::RGBLed m_RSSILed; ///< Receiver Strength Signal Indicator (RSSI) Led. Shows the signal strength by color
    ArduForge::BatteryIndicator m_BatterySensor; ///< Battery indicator measure battery level.
    ArduForge::BinarySensor m_HallSensor;

    int8_t m_RSSI;
    uint32_t m_LastMessageTimestamp;
    uint32_t m_LastPing;
    uint32_t m_LastBatteryCheck;
    uint32_t m_LastAckCheck;
    State m_FlapStatus;
    uint8_t m_MsgBuffer[64];
};//StartingFlap

#endif