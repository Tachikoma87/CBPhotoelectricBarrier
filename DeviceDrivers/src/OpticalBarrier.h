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
#ifndef TFCHAMPION_OPTICALBARRIER_H
#define TFCHAMPION_OPTICALBARRIER_H
#ifdef ESP32

#include <Arduino.h>
#include <OpticalLightBarrierESP32.h>
#include <BatteryIndicator.h>
#include "BTCommunication.hpp"
#include "BluetoothSerial.h"

class OpticalBarrier{
public:
    OpticalBarrier(void);
    ~OpticalBarrier(void);

    void init(uint8_t CameraID);
    void clear(void);
    void update(void);

private:
    static void barrierCallback(ArduForge::OpticalLightBarrierESP32::BarrierMsg Msg, void *pUserParam);
    void processBluetoothPackage(void);
    void ledColor(uint8_t Red, uint8_t Green, uint8_t Blue);

    ArduForge::OpticalLightBarrierESP32 m_Barrier;
    BluetoothSerial m_BTSerial;
    ArduForge::BatteryIndicator m_Battery;

    uint8_t m_BTMsgBuffer[64];
    uint8_t m_BTBufferPointer;
    uint8_t m_BTMsgBufferOut[64];
    uint8_t m_BatteryCharge;

    const uint8_t m_RedLEDPin = 12;
    const uint8_t m_GreenLEDPin = 13;
    const uint8_t m_BlueLEDPin = 15;
    const uint8_t m_BatteryReadPin = 14;

    uint32_t m_LastBarrierBreach;
    uint32_t m_LastBarrierBreachSent;



};//OpticalBarrier


#endif //ESP32
#endif // Header-Guard