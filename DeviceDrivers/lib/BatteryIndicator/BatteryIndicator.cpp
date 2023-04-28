#include "BatteryIndicator.h"

namespace ArduForge{

    BatteryIndicator::BatteryIndicator(void){
        m_RefVoltage = 0.0f;
        m_R1 = 0;
        m_R2 = 0;
        m_BatteryCount = 0;
        m_BatteryPin = 0xFF;
    }//Constructor

    BatteryIndicator::~BatteryIndicator(void){
        end();
    }//Destructor


    void BatteryIndicator::begin(uint8_t BatteryPin, uint8_t BatteryCount, uint32_t R1, uint32_t R2, BatteryType Type, float ReferenceVoltage){
        /*#ifdef ESP32
        adcAttachPin(m_BatteryPin);
        analogReadResolution(12);
        analogSetAttenuation(ADC_11db);
        #endif*/
        m_BatteryPin = BatteryPin;
        m_BatteryCount = BatteryCount;
        m_R1 = (float)R1;
        m_R2 = (float)R2;
        m_RefVoltage = ReferenceVoltage;
        m_Type = Type;

        switch(m_Type){
            case TYPE_NIMH:{
                m_VoltageMin = 1.05f;
                m_VoltageMax = 1.30f;
            }break;
            case TYPE_LIPO:{
                m_VoltageMin = 3.1f;
                m_VoltageMax = 4.1f;
            }break;
            default: break;
        }//switch[type]

    }//begin

    void BatteryIndicator::end(void){

    }//end

    float BatteryIndicator::read(void){
        #ifdef ESP32
        const float Resolution = 4095.0f;
        #else
        const float Resolution = 1023.0f;
        #endif

        // read voltage at voltage divider
        const float Reading = m_RefVoltage * analogRead(m_BatteryPin)/Resolution;
        // calculate original voltage V from Reading U1: U1 = (V*R2) / (R1 + R2) <=> V = (U1 * (R1+R2)) / R2
        return (m_R1 == 0 || m_R2 == 0) ? Reading : Reading * (m_R1+m_R2) / m_R2;
    }//read

    uint8_t BatteryIndicator::charge(void){
        const float Voltage = read() / (float)m_BatteryCount;
        if(Voltage < m_VoltageMin) return 0;
        // Mapping from [VoltageMin , VoltageMax] -> [0,100]
        uint8_t Charge = (uint8_t) ((Voltage - m_VoltageMin)/(m_VoltageMax - m_VoltageMin) * 100.0f);
        if(Charge < 0) Charge = 0;
        else if(Charge > 100) Charge = 100;
        return Charge;
    }//charge

    BatteryIndicator::BatteryType BatteryIndicator::type(void)const{
        return m_Type;
    }//type


}//name-space