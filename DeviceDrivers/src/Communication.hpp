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
#ifndef TFELECTRONIC_COMMUNICATION_H
#define TFELECTRONIC_COMMUNICATION_H

#include <string.h>
#include <inttypes.h>

const uint8_t CC1101RelayAddress = 0x0B;
const uint8_t CC1101FlapAddress = 0x0A;

class CC1101Message{
public:
    enum CmdCode: uint8_t{
        CMD_UNKNOWN = 0x00,
        CMD_PING,
        CMD_STATUS,
        CMD_START_SIGNAL
    };//CmdCodes

    static CmdCode cmdFromStream(uint8_t *pStream){
        if(nullptr == pStream) return CMD_UNKNOWN;
        return (CmdCode)pStream[0];
    }//cmdFromStream

    bool request(void)const{
        return m_Request;
    }//request

    void request(bool Request){
        m_Request = Request;
    }//request

    virtual uint8_t toStream(uint8_t *pBuffer) = 0;
    virtual uint8_t fromStream(uint8_t *pBuffer) = 0;
    
protected:
    CC1101Message(void){
        m_CmdCode = CMD_UNKNOWN;
    }
    ~CC1101Message(void){
        m_CmdCode = CMD_UNKNOWN;
    }

    uint8_t m_CmdCode; ///< Command code identifier
    bool m_Request;

};//CC1101Message


class MsgStatus: public CC1101Message{
public:
    MsgStatus(void){
        m_CmdCode = CMD_STATUS;
        m_Request = false;
        m_Charge = 0;
        m_Rssi = 0;
        m_InternalClock = 0;
        m_RemoteClock = 0;
    }//Constructor

    ~MsgStatus(void){
        m_CmdCode = CMD_STATUS;
        m_Request = true;
        m_Charge = 0;
    }//Destructor


    uint8_t charge(void)const{
        return m_Charge;
    }//capacity

    void charge(uint8_t C){
        m_Charge = C;
    }//charge

    int8_t rssi(void)const{
        return m_Rssi;
    }

    void rssi(int8_t Rssi){
        m_Rssi = Rssi;
    }

    uint32_t internalClock(void)const{
        return m_InternalClock;
    }

    void internalClock(uint32_t Timestamp){
        m_InternalClock = Timestamp;
    }

    uint32_t remoteClock(void)const{
        return m_RemoteClock;
    }

    void remoteClock(uint32_t Timestamp){
        m_RemoteClock = Timestamp;
    }

    uint8_t toStream(uint8_t *pBuffer){
        uint8_t Rval = 0; // number of written bytes
        memcpy(&pBuffer[Rval], &m_CmdCode, sizeof(uint8_t));    Rval += sizeof(uint8_t);
        memcpy(&pBuffer[Rval], &m_Request, sizeof(bool));       Rval += sizeof(bool);
        memcpy(&pBuffer[Rval], &m_Charge, sizeof(uint8_t));     Rval += sizeof(uint8_t);
        memcpy(&pBuffer[Rval], &m_Rssi, sizeof(int8_t));        Rval += sizeof(int8_t);
        memcpy(&pBuffer[Rval], &m_InternalClock, sizeof(uint32_t)); Rval += sizeof(uint32_t);
        memcpy(&pBuffer[Rval], &m_RemoteClock, sizeof(uint32_t));   Rval += sizeof(uint32_t);
        return Rval;
    }//toStream

    uint8_t fromStream(uint8_t *pBuffer){
        uint8_t Rval = 0;
        memcpy(&m_CmdCode, &pBuffer[Rval], sizeof(uint8_t));    Rval += sizeof(uint8_t);
        memcpy(&m_Request, &pBuffer[Rval], sizeof(bool));       Rval += sizeof(bool);
        memcpy(&m_Charge, &pBuffer[Rval], sizeof(uint8_t));     Rval += sizeof(uint8_t);
        memcpy(&m_Rssi, &pBuffer[Rval], sizeof(int8_t));        Rval += sizeof(int8_t);
        memcpy(&m_InternalClock, &pBuffer[Rval], sizeof(uint32_t)); Rval += sizeof(uint32_t);
        memcpy(&m_RemoteClock, &pBuffer[Rval], sizeof(uint32_t));   Rval += sizeof(uint32_t);
        return Rval;
    }//fromStream

protected:
    uint8_t m_Charge; ///< Remaining charge (in percent)
    int8_t m_Rssi; ///< Rssi value
    uint32_t m_InternalClock; ///< Internal clock (up time)
    uint32_t m_RemoteClock; ///< Clock timing of caller (for time sync)
};//MsgBattery

class MsgPing: public CC1101Message{
public:
    MsgPing(void){
        m_CmdCode = CMD_PING;
    }//Constructor

    ~MsgPing(void){

    }//Destructor

    uint32_t timestamp(void)const{
        return m_Timestamp;
    }//timestamp

    void timestamp(uint32_t Millis){
        m_Timestamp = Millis;
    }//timestamp

    uint8_t toStream(uint8_t *pBuffer){
        uint8_t Rval = 0;
        memcpy(&pBuffer[Rval], &m_CmdCode, sizeof(uint8_t));    Rval += sizeof(uint8_t);
        memcpy(&pBuffer[Rval], &m_Request, sizeof(bool));       Rval += sizeof(bool);
        memcpy(&pBuffer[Rval], &m_Timestamp, sizeof(uint32_t)); Rval += sizeof(uint32_t);
        return Rval;
    }//toStream

    uint8_t fromStream(uint8_t *pBuffer){
        uint8_t Rval = 0;
        memcpy(&m_CmdCode, &pBuffer[Rval], sizeof(uint8_t));    Rval += sizeof(uint8_t);
        memcpy(&m_Request, &pBuffer[Rval], sizeof(bool));       Rval += sizeof(bool);
        memcpy(&m_Timestamp, &pBuffer[Rval], sizeof(uint32_t)); Rval += sizeof(uint32_t);
        return Rval;
    }//fromStream

protected:
    uint32_t m_Timestamp; ///< Timestamp in milliseconds
};//MsgPin

class MsgStartSignal: public CC1101Message{
public:
    
    MsgStartSignal(void){
        m_CmdCode = CMD_START_SIGNAL;

    }//Constructor

    ~MsgStartSignal(void){

    }//Destructor

    uint32_t timestamp(void)const{
        return m_Timestamp;
    }

    void timestamp(uint32_t Stamp){
        m_Timestamp = Stamp;
    }

    bool acknowledge(void)const{
        return m_Acknowledge;
    }

    void acknowledge(bool Ack){
        m_Acknowledge = Ack;
    }

    uint8_t toStream(uint8_t *pBuffer){
        uint8_t Rval = 0;
        memcpy(&pBuffer[Rval], &m_CmdCode, sizeof(uint8_t));    Rval += sizeof(uint8_t);
        memcpy(&pBuffer[Rval], &m_Request, sizeof(bool));       Rval += sizeof(bool);
        memcpy(&pBuffer[Rval], &m_Acknowledge, sizeof(bool));   Rval += sizeof(bool);
        memcpy(&pBuffer[Rval], &m_Timestamp, sizeof(uint32_t)); Rval += sizeof(uint32_t);
        return Rval;
    }//toStream

    uint8_t fromStream(uint8_t *pBuffer){
        uint8_t Rval = 0;
        memcpy(&m_CmdCode, &pBuffer[Rval], sizeof(uint8_t));    Rval += sizeof(uint8_t);
        memcpy(&m_Request, &pBuffer[Rval], sizeof(bool));       Rval += sizeof(bool);
        memcpy(&m_Acknowledge, &pBuffer[Rval], sizeof(bool));   Rval += sizeof(bool);
        memcpy(&m_Timestamp, &pBuffer[Rval], sizeof(uint32_t)); Rval += sizeof(uint32_t);
        return Rval;
    }//fromStream

protected:
    bool m_Acknowledge;
    uint32_t m_Timestamp;
};//MsgStartSignal

#endif