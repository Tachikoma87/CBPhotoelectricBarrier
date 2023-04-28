//
// Created by Tinkerer on 09.02.2021.
//

#ifndef BTCOMMUNICATION_HPP
#define BTCOMMUNICATION_HPP

#include <inttypes.h>

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <memory.h>
#endif

class BTMessage{
public:
    enum BTPackageType: uint8_t{
        BTPCK_UNKNOWN = 0, // void or invalid package
        BTPCK_PING,
        BTPCK_STATUS,
        BTPCK_STARTSIGNAL,
        BTPCK_COMMAND,
        BTPCK_IMAGE,
    };


    static BTPackageType fetchPackageTypeFromStream(uint8_t *pData, uint32_t BufferSize){
        const char MagicPackageHeader[3] = {'T', 'F', 'C'};
        if(BufferSize < 4) return BTPCK_UNKNOWN;
        BTPackageType Rval = BTPCK_UNKNOWN;
        // is it a valid T&F Champion BT package?
        for(uint8_t i=0; i < sizeof(MagicPackageHeader); ++i){
            if(pData[i] != MagicPackageHeader[i]) return Rval;
        }
        Rval = (BTPackageType)pData[3];
        return Rval;
    }//fetchCmdType


    // written at the end of command stream so its clear that package is finished
    static uint8_t cmdCharacter(void){
        return '#';
    }

    bool request(void){
        return m_Request;
    }

    void request(bool Request){
        m_Request = Request;
    }

    BTPackageType type(void)const{
        return m_PackageType;
    }

    virtual uint32_t toStream(uint8_t *pData, uint32_t BufferSize){
        if(nullptr == pData) return 0;
        if(BufferSize < size()) return 0;
        const char MagicPackageHeader[3] = {'T', 'F', 'C'};
        uint32_t Rval = 0;
        memcpy(&pData[Rval], MagicPackageHeader, sizeof(MagicPackageHeader)); Rval += sizeof(MagicPackageHeader);
        memcpy(&pData[Rval], &m_PackageType, sizeof(BTPackageType)); Rval += sizeof(BTPackageType);
        memcpy(&pData[Rval], &m_Request, sizeof(bool)); Rval += sizeof(bool);
        return Rval;
    }//toStream

    virtual uint32_t fromStream(uint8_t *pData, uint32_t BufferSize){
        if(nullptr == pData) return 0;
        if(BufferSize < size()) return 0;
        uint32_t Rval = 0;
        m_PackageType = fetchPackageTypeFromStream(pData, BufferSize);
        if(BTPCK_UNKNOWN != m_PackageType){
            // now we can fetch the actual content
            Rval = sizeof(uint8_t)*3;
            memcpy(&m_PackageType, &pData[Rval], sizeof(BTPackageType)); Rval += sizeof(BTPackageType);
            memcpy(&m_Request, &pData[Rval], sizeof(bool)); Rval += sizeof(bool);
        }
        return Rval;
    }//fromStream

    virtual uint32_t size(void){
        uint32_t Rval = 0;
        Rval += 3 * sizeof(char);
        Rval += sizeof(BTPackageType);
        Rval += sizeof(bool);
        return Rval;
    }//size

protected:
    BTMessage(void){
        m_Request = false;
        m_PackageType = BTPCK_UNKNOWN;
    };

    ~BTMessage(void){};

    bool m_Request; // request or answer
    BTPackageType m_PackageType;
    uint8_t m_CmdChar;

};//BTMessage

class BTMessagePing: public BTMessage{
public:
    BTMessagePing(){
        m_PackageType = BTPCK_PING;
        m_Request = false;
        m_CmdChar = cmdCharacter();
    }

    ~BTMessagePing(){

    }

    uint32_t timestamp(void)const{
        return m_Timestamp;
    }

    void timestamp(uint32_t Timestamp){
        m_Timestamp = Timestamp;
    }

    virtual uint32_t toStream(uint8_t *pData, uint32_t BufferSize) override {
        if(nullptr == pData || BufferSize < size()) return 0;
        uint32_t Rval = BTMessage::toStream(pData, BufferSize);
        memcpy(&pData[Rval], &m_Timestamp, sizeof(uint32_t)); Rval += sizeof(uint32_t);
        memcpy(&pData[Rval], &m_CmdChar, sizeof(uint8_t)); Rval += sizeof(uint8_t);
        return Rval;
    }

    virtual uint32_t fromStream(uint8_t *pData, uint32_t BufferSize) override{
        if(nullptr == pData || BufferSize < size()) return 0;
        uint32_t Rval = BTMessage::fromStream(pData, BufferSize);
        memcpy(&m_Timestamp, &pData[Rval], sizeof(uint32_t)); Rval += sizeof(uint32_t);
        return Rval;
    }

    virtual uint32_t size(void) override {
        uint32_t Rval = BTMessage::size();
        Rval += sizeof(uint32_t);
        Rval += sizeof(uint8_t); // cmd Char
        return Rval;
    }
protected:
    uint32_t m_Timestamp;
};// BTMessagePing


/**
 * Queries status of connected devices and performs time sync
 */
class BTMessageStatus: public BTMessage{
public:
    enum DeviceState: uint8_t{
        STATE_UNKNOWN = 0,
        STATE_OPEN, ///< wings are open
        STATE_READY, ///< wings are open and start will be performed soon
        STATE_CLOSED, ///< closed
    };

    BTMessageStatus(void){
        m_PackageType = BTPCK_STATUS;
        m_Request = false;
        m_BatterChargeDevice = 255;
        m_BatteryChargeRelay = 255;
        m_RSSI = 0;
        m_DeviceState = STATE_UNKNOWN;
        m_AndroidClock = 0;
        m_DeviceClock = 0;
        m_CmdChar = cmdCharacter();
    }//Status

    ~BTMessageStatus(){

    }

    uint8_t batteryChargeDevice(void)const{
        return m_BatterChargeDevice;
    }

    void batteryChargeDevice(uint8_t Charge){
        m_BatterChargeDevice = Charge;
    }

    uint8_t batteryChargeRelay(void)const{
        return m_BatteryChargeRelay;
    }

    void batteryChargeRelay(uint8_t Charge){
        m_BatteryChargeRelay = Charge;
    }

    void rssi(int8_t Rssi){
        m_RSSI = Rssi;
    }

    int8_t rssi(void)const{
        return m_RSSI;
    }

    DeviceState deviceState(void)const{
        return m_DeviceState;
    }

    void deviceState(DeviceState State){
        m_DeviceState = State;
    }

    uint32_t androidClock(void)const{
        return m_AndroidClock;
    }

    void androidClock(uint32_t Clock){
        m_AndroidClock = Clock;
    }

    uint32_t deviceClock(void)const{
        return m_DeviceClock;
    }

    void deviceClock(uint32_t Clock){
        m_DeviceClock = Clock;
    }


    virtual uint32_t toStream(uint8_t *pData, uint32_t BufferSize)override {
        if(BufferSize < size()) return 0;
        uint32_t Rval = BTMessage::toStream(pData, BufferSize);
        memcpy(&pData[Rval], &m_BatterChargeDevice, sizeof(uint8_t));   Rval += sizeof(uint8_t);
        memcpy(&pData[Rval], &m_BatteryChargeRelay, sizeof(uint8_t));   Rval += sizeof(uint8_t);
        memcpy(&pData[Rval], &m_RSSI, sizeof(int8_t));                  Rval += sizeof(int8_t);
        memcpy(&pData[Rval], &m_DeviceState, sizeof(DeviceState));      Rval += sizeof(DeviceState);
        memcpy(&pData[Rval], &m_AndroidClock, sizeof(uint32_t));        Rval += sizeof(uint32_t);
        memcpy(&pData[Rval], &m_DeviceClock, sizeof(uint32_t));         Rval += sizeof(uint32_t);
        memcpy(&pData[Rval], &m_CmdChar, sizeof(uint8_t));              Rval += sizeof(uint8_t);
        return Rval;
    }

    virtual uint32_t fromStream(uint8_t *pData, uint32_t BufferSize)override {
        uint32_t Rval = BTMessage::fromStream(pData, BufferSize);
        memcpy(&m_BatterChargeDevice, &pData[Rval], sizeof(uint8_t));   Rval += sizeof(uint8_t);
        memcpy(&m_BatteryChargeRelay, &pData[Rval], sizeof(uint8_t));   Rval += sizeof(uint8_t);
        memcpy(&m_RSSI, &pData[Rval], sizeof(int8_t));                  Rval += sizeof(int8_t);
        memcpy(&m_DeviceState, &pData[Rval], sizeof(DeviceState));      Rval += sizeof(DeviceState);
        memcpy(&m_AndroidClock, &pData[Rval], sizeof(uint32_t));        Rval += sizeof(uint32_t);
        memcpy(&m_DeviceClock, &pData[Rval], sizeof(uint32_t));         Rval += sizeof(uint32_t);
        return Rval;
    }

    virtual uint32_t size(void)override {
        uint32_t Rval = BTMessage::size();
        Rval += sizeof(uint8_t);
        Rval += sizeof(uint8_t);
        Rval += sizeof(int8_t);
        Rval += sizeof(uint8_t);
        Rval += sizeof(uint32_t);
        Rval += sizeof(uint32_t);
        Rval += sizeof(uint8_t); // Cmd Char
        return Rval;
    }

protected:
    uint8_t m_BatterChargeDevice; ///< Charge of the device.
    uint8_t m_BatteryChargeRelay; ///< Charge of relay device (if any).
    int8_t m_RSSI; ///< Receive signal strength indicator at the device
    DeviceState m_DeviceState; ///< Current device state
    uint32_t m_AndroidClock; ///< Up time in milliseconds on the android device
    uint32_t m_DeviceClock; ///< Up time in milliseconds on the start flap device
};


class BTMessageStartSignal: public BTMessage{
public:
    BTMessageStartSignal(){
        m_PackageType = BTPCK_STARTSIGNAL;
        m_Request = false;
        m_CmdChar = cmdCharacter();
    }

    ~BTMessageStartSignal(){

    }

    uint32_t timestamp(void)const{
        return m_Timestamp;
    }

    void timestamp(uint32_t Timestamp){
        m_Timestamp = Timestamp;
    }

    bool acknowledge(void)const{
        return m_Acknowledge;
    }

    void acknowledge(bool Ack){
        m_Acknowledge = Ack;
    }

    virtual uint32_t toStream(uint8_t *pData, uint32_t BufferSize)override {
        if(BufferSize < size()) return 0;
        uint32_t Rval = BTMessage::toStream(pData, BufferSize);
        memcpy(&pData[Rval], &m_Timestamp, sizeof(uint32_t)); Rval += sizeof(uint32_t);
        memcpy(&pData[Rval], &m_Acknowledge, sizeof(bool)); Rval += sizeof(bool);
        memcpy(&pData[Rval], &m_CmdChar, sizeof(uint8_t)); Rval += sizeof(uint8_t);
        return Rval;
    }

    virtual uint32_t fromStream(uint8_t *pData, uint32_t BufferSize)override {
        if(BufferSize < size()) return 0;
        uint32_t Rval = BTMessage::fromStream(pData, BufferSize);
        memcpy(&m_Timestamp, &pData[Rval], sizeof(uint32_t)); Rval += sizeof(uint32_t);
        memcpy(&m_Acknowledge, &pData[Rval], sizeof(bool));  Rval += sizeof(bool);
        return Rval;
    }

    virtual uint32_t size(void) override{
        uint32_t Rval = BTMessage::size();
        Rval += sizeof(uint32_t);
        Rval += sizeof(bool);
        Rval += sizeof(uint8_t); // Cmd Char
        return Rval;
    }

protected:
    uint32_t m_Timestamp; ///<  Timestamp of starting flap when clap was closed (to annihilate signal travel delay)
    bool m_Acknowledge; ///< Is this an acknowledge package?
};

class BTMessageImage: public BTMessage{
public:
    enum PixelFormat: uint8_t{
        FMT_UNKNOWN = 0,
        FMT_GRAYSCALE,
        FMT_RGB565,
        FMT_RGB888,
        FMT_JPEG,
    };

    BTMessageImage(void){
        m_Request = true;
        m_PackageType = BTPCK_IMAGE;
        m_Width = 0;
        m_Height = 0;
        m_DataSize = 0;
        m_PixelFormat = FMT_UNKNOWN;
        m_pImgData = nullptr;
    }

    ~BTMessageImage(void){
        clear();
    }

    void init(uint32_t Width, uint32_t Height, uint8_t *pData,  PixelFormat Fmt, uint32_t DataSize, bool Copy = true){
        clear();
        if(Copy){
            m_pImgData = new uint8_t[DataSize];
            memcpy(m_pImgData, pData, DataSize);
        }else{
            m_pImgData = pData;
        }
        m_Width = Width;
        m_Height = Height;
        m_PixelFormat = Fmt;
        m_DataSize = DataSize;
    }

    void clear(void){

    }

    uint32_t width(void)const{
        return m_Width;
    }

    uint32_t height(void)const{
        return m_Height;
    }

    uint8_t *imageData(void){
        return m_pImgData;
    }

    uint32_t imageSize(void)const{
        return m_DataSize;
    }

    PixelFormat format(void)const{
        return m_PixelFormat;
    }

    uint32_t toStream(uint8_t *pBuffer, uint32_t BufferSize){
        if(BufferSize < size()) return 0;
        uint32_t Rval = BTMessage::toStream(pBuffer, BufferSize);
        memcpy(&pBuffer[Rval], &m_Width, sizeof(uint32_t));             Rval += sizeof(uint32_t);
        memcpy(&pBuffer[Rval], &m_Height, sizeof(uint32_t));            Rval += sizeof(uint32_t);
        memcpy(&pBuffer[Rval], &m_DataSize, sizeof(uint32_t));          Rval += sizeof(uint32_t);
        memcpy(&pBuffer[Rval], &m_PixelFormat, sizeof(PixelFormat));    Rval += sizeof(PixelFormat);
        if(m_DataSize > 0) memcpy(&pBuffer[Rval], m_pImgData, m_DataSize); Rval += m_DataSize;
        memcpy(&pBuffer[Rval], &m_CmdChar, sizeof(uint8_t));            Rval += sizeof(uint8_t);
        return Rval;
    }//toStream

    uint32_t fromStream(uint8_t *pBuffer, uint32_t BufferSize){
        if(nullptr == pBuffer || BufferSize < size()) return 0;
        clear();
        uint32_t Rval = BTMessage::fromStream(pBuffer, BufferSize);
        memcpy(&m_Width, &pBuffer[Rval], sizeof(uint32_t));     Rval += sizeof(uint32_t);
        memcpy(&m_Height, &pBuffer[Rval], sizeof(uint32_t));    Rval += sizeof(uint32_t);
        memcpy(&m_DataSize, &pBuffer[Rval], sizeof(uint32_t));  Rval += sizeof(uint32_t);
        memcpy(&m_PixelFormat, &pBuffer[Rval], sizeof(PixelFormat));    Rval += sizeof(PixelFormat);
        if(m_DataSize > 0){
            m_pImgData = new uint8_t[m_DataSize];
            memcpy(m_pImgData, &pBuffer[Rval], m_DataSize);
            Rval += m_DataSize;
        }
        return Rval;
    }//fromStream

    uint32_t size(void){
        uint32_t Rval = BTMessage::size();
        Rval += sizeof(uint32_t);
        Rval += sizeof(uint32_t);
        Rval += sizeof(uint32_t);
        Rval += sizeof(PixelFormat);
        Rval += m_DataSize; // actual image data
        Rval += sizeof(uint8_t); //Cmd char
        return Rval;
    }

protected:
    uint32_t m_Width;
    uint32_t m_Height;
    uint32_t m_DataSize;
    PixelFormat m_PixelFormat;
    uint8_t *m_pImgData;
};

class BTMessageCommand: public BTMessage{
public:
    enum CmdCode: uint8_t{
        CMD_UNKNOWN = 0,
        CMD_BARRIERCAM_BREACH,
        CMD_BARRIERCAM_START_DETECTION,
        CMD_BARRIERCAM_STOP_DETECTION,
        CMD_BARRIERCAM_CALIBRATE,
        CMD_CHANGE_SENSITIVITY,
        CMD_CONFIG_MEASUREMENT_POINTS,
    };

    BTMessageCommand(void){
        m_Request = true;
        m_PackageType = BTPCK_COMMAND;
        m_CmdCode = CMD_UNKNOWN;
        m_CmdChar = cmdCharacter();
        m_PayloadSize = 0;
        m_pPayload = nullptr;
    }//Constructor

    ~BTMessageCommand(void){

    }//Destructor

    void cmdCode(CmdCode Code){
        m_CmdCode = Code;
    }

    CmdCode cmdCode(void)const{
        return m_CmdCode;
    }

    void setPayload(uint8_t *pData, uint32_t DataSize){
        delete [] m_pPayload;
        m_pPayload = nullptr;
        m_PayloadSize = 0;
        if(nullptr != pData && DataSize > 0){
            m_pPayload = new uint8_t[DataSize];
            memcpy(m_pPayload, pData, DataSize),
            m_PayloadSize = DataSize;
        }
    }//payload

    void getPayload(uint8_t *pBuffer, uint32_t BufferSize)const{
        if(nullptr == pBuffer || m_pPayload == nullptr) return;
        memcpy(pBuffer, m_pPayload, m_PayloadSize);
    }

    uint32_t getPayloadSize(void)const{
        return m_PayloadSize;
    }

    uint32_t toStream(uint8_t *pBuffer, uint32_t BufferSize) override {
        if(pBuffer == nullptr || BufferSize < size()) return 0;
        uint32_t Rval = BTMessage::toStream(pBuffer, BufferSize);
        memcpy(&pBuffer[Rval], &m_CmdCode, sizeof(CmdCode));        Rval += sizeof(CmdCode);
        memcpy(&pBuffer[Rval], &m_PayloadSize, sizeof(uint32_t));   Rval += sizeof(uint32_t);
        if(nullptr != m_pPayload){
            memcpy(&pBuffer[Rval], m_pPayload, m_PayloadSize);
            Rval += m_PayloadSize;
        }
        memcpy(&pBuffer[Rval], &m_CmdChar, sizeof(uint8_t));    Rval += sizeof(uint8_t);
        return Rval;
    }//toStream

    uint32_t fromStream(uint8_t *pBuffer, uint32_t BufferSize) override{
        if(pBuffer == nullptr || BufferSize < size()) return 0;
        delete[] m_pPayload;
        m_pPayload = nullptr;
        m_PayloadSize = 0;

        uint32_t Rval = BTMessage::fromStream(pBuffer, BufferSize);
        memcpy(&m_CmdCode, &pBuffer[Rval], sizeof(CmdCode));    Rval += sizeof(CmdCode);
        memcpy(&m_PayloadSize, &pBuffer[Rval], sizeof(uint32_t));   Rval += sizeof(uint32_t);
        if(m_PayloadSize > 0){
            m_pPayload = new uint8_t[m_PayloadSize];
            memcpy(m_pPayload, &pBuffer[Rval], m_PayloadSize);
            Rval += m_PayloadSize;
        }
        return Rval;
    }//FromStream

    uint32_t size(void) override{
        uint32_t Rval = BTMessage::size();
        Rval += sizeof(CmdCode);
        Rval += sizeof(uint32_t);
        Rval += m_PayloadSize;
        Rval += sizeof(uint8_t); // Cmd Char
        return Rval;
    }//size

protected:
    CmdCode m_CmdCode;  ///< The command code
    uint8_t *m_pPayload; ///< Additional data
    uint32_t m_PayloadSize; ///< Size of additional data
};

#endif
