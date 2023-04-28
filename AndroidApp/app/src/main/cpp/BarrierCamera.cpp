//
// Created by Tinkerer on 07.03.2021.
//

#include <android/log.h>
#include "BarrierCamera.h"


BarrierCamera::BarrierCamera(void){
    m_LastPingSend = 0;
    m_Latency = 0;
    m_InternalClock = 0;
    m_LastMessageReceived = 0;
    m_Detecting = false;
    m_BatteryCharge = 0;

    m_pPreviewImageData = nullptr;
    m_PreviewImageHeight = 0;
    m_PreviewImageWidth = 0;

    m_LastBarrierBreach = 0;
    m_LastStatusUpdateRequest = 0;
    m_DeviceClockOffset = 0;

}//Constructor

BarrierCamera::~BarrierCamera(void){

}//Destructor

void BarrierCamera::init(int8_t CameraID){
    m_RetrievedConfig = false;

}//init

void BarrierCamera::update(uint32_t Timestamp){
    m_InternalClock = Timestamp;

    if(!m_RetrievedConfig){
        BTMessageCommand CmdMsg;
        CmdMsg.request(false);
        CmdMsg.cmdCode(BTMessageCommand::CMD_CONFIG_MEASUREMENT_POINTS);

        MsgBuffer *pMsg = new MsgBuffer();
        pMsg->init(CmdMsg.size());
        CmdMsg.toStream(pMsg->pBuffer, pMsg->BufferSize);
        m_MsgQueue.push_back(pMsg);
        m_RetrievedConfig = true;
        m_MeasurementPointsChanged = true;
    }

   if(!m_Detecting && m_InternalClock - m_LastStatusUpdateRequest > 1464){
       m_LastStatusUpdateRequest = m_InternalClock;

       BTMessageStatus StatusMsg;
       StatusMsg.request(true);
       StatusMsg.androidClock(m_InternalClock);

       MsgBuffer *pMsgBuf = new MsgBuffer(StatusMsg.size());
       StatusMsg.toStream(pMsgBuf->pBuffer, pMsgBuf->BufferSize);
       m_MsgQueue.push_back(pMsgBuf);
   }

    // do we need to send a ping?
    /*if(m_InternalClock - m_LastPingSend > 1750){
        // prepare ping and send
        m_LastPingSend = m_InternalClock;

        BTMessagePing PingMsg;
        PingMsg.request(true);
        PingMsg.timestamp(m_InternalClock);

        MsgBuffer *pMsgBuf = new MsgBuffer(PingMsg.size());
        PingMsg.toStream(pMsgBuf->pBuffer, pMsgBuf->BufferSize);
        m_MsgQueue.push_back(pMsgBuf);
    }*/

}//update


uint32_t BarrierCamera::fetchMessageBuffer(uint8_t **ppBuffer){
    if(nullptr == ppBuffer) return 0;

    uint32_t Rval = 0;
    for(auto &i: m_MsgQueue){
        if(i != nullptr){
            (*ppBuffer) = new uint8_t[i->BufferSize];
            memcpy(*ppBuffer, i->pBuffer, sizeof(uint8_t)*i->BufferSize);
            Rval = i->BufferSize;
            delete i;
            i = nullptr;
            break;
        }
    }//for[Message queue]

    if(Rval == 0) m_MsgQueue.clear();

    return Rval;
}//fetchMessageBuffer

uint16_t BarrierCamera::latency(void)const{
    return m_Latency;
}//latency

uint8_t BarrierCamera::batteryCharge() const {
    return m_BatteryCharge;
}

bool BarrierCamera::isConnected(void)const{
    return m_Detecting || (m_InternalClock - m_LastMessageReceived < 6000);
}//isConnected

bool BarrierCamera::isDetecting(void)const{
    return m_Detecting;
}

uint32_t BarrierCamera::lastBarrierBreach(bool Reset){
    uint32_t Rval = m_LastBarrierBreach;
    if(Reset) m_LastBarrierBreach = 0;
    return Rval;
}//lastBarrierBreach

void  BarrierCamera::startDetection(void){
    BTMessageCommand StartCmdMsg;
    StartCmdMsg.request(true);
    StartCmdMsg.cmdCode(BTMessageCommand::CMD_BARRIERCAM_START_DETECTION);

    MsgBuffer *pMsgBuffer = new MsgBuffer();
    pMsgBuffer->init(StartCmdMsg.size());
    StartCmdMsg.toStream(pMsgBuffer->pBuffer, pMsgBuffer->BufferSize);
    m_MsgQueue.push_back(pMsgBuffer);

}//startDetection

void BarrierCamera::stopDetection(void){
    BTMessageCommand StopCmdMsg;
    StopCmdMsg.request(true);
    StopCmdMsg.cmdCode(BTMessageCommand::CMD_BARRIERCAM_STOP_DETECTION);

    MsgBuffer *pMsgBuffer = new MsgBuffer();
    pMsgBuffer->init(StopCmdMsg.size());
    StopCmdMsg.toStream(pMsgBuffer->pBuffer, pMsgBuffer->BufferSize);
    m_MsgQueue.push_back(pMsgBuffer);
}//stop detection


void BarrierCamera::processMessage(BTMessagePing *pMsg){
    if(nullptr == pMsg) return;
    if(pMsg->request()) return; // we do not respond to requests

    m_Latency = m_InternalClock - pMsg->timestamp();
    m_LastMessageReceived = m_InternalClock;

}//processMessage

void BarrierCamera::processMessage(BTMessageStatus *pMsg){
    if(nullptr == pMsg) return;
    if(pMsg->request()) return; // we do not respond to requests

    uint32_t Latency = m_InternalClock - pMsg->androidClock();
    if(Latency > 500) return; //this message can not be valid

    if(m_DeviceClockOffset == 0){
        m_DeviceClockOffset = int64_t(pMsg->androidClock()) - (int64_t(pMsg->deviceClock()) - int64_t(m_Latency/2));
        __android_log_print(ANDROID_LOG_INFO, "BarrierCamera", "Clock offset is %d ms", m_DeviceClockOffset);
    }

    m_BatteryCharge = pMsg->batteryChargeDevice();
    m_Latency = Latency;
    m_LastMessageReceived = m_InternalClock;

}//processMessage

void BarrierCamera::processMessage(BTMessageCommand *pMsg){
    if(nullptr == pMsg) return;
    m_LastMessageReceived = m_InternalClock;
    switch(pMsg->cmdCode()){
        case BTMessageCommand::CMD_BARRIERCAM_BREACH:{
            // payload is milliseconds
            int32_t RemoteTimestamp = 0;
            pMsg->getPayload((uint8_t*)&RemoteTimestamp, sizeof(int32_t));

            // only take valid values
            if(abs(m_InternalClock - (RemoteTimestamp + m_DeviceClockOffset)) > 2500 ) return;

            if(RemoteTimestamp > 0 && m_Detecting) {
                m_LastBarrierBreach = RemoteTimestamp + m_DeviceClockOffset;

                __android_log_print(ANDROID_LOG_INFO, "BarrierCamera","Barrier breach at %lld (Compensated: %lld ms)\n", m_LastBarrierBreach, int64_t(m_InternalClock) - m_LastBarrierBreach);

                // after receiving a breach we need to confirm correct values received
                BTMessageCommand ConfirmationMsg;
                ConfirmationMsg.request(false);
                ConfirmationMsg.cmdCode(BTMessageCommand::CMD_BARRIERCAM_BREACH);

                MsgBuffer *pMsgBuffer = new MsgBuffer();
                pMsgBuffer->init(ConfirmationMsg.size());
                ConfirmationMsg.toStream(pMsgBuffer->pBuffer, pMsgBuffer->BufferSize);
                m_MsgQueue.push_back(pMsgBuffer);

                m_Detecting = false;
            }
        }break;
        case BTMessageCommand::CMD_BARRIERCAM_CALIBRATE:{
            float Noise = 0.0f;
            pMsg->getPayload((uint8_t*)&Noise, sizeof(float));
            __android_log_print(ANDROID_LOG_INFO, "BarrierCamera", "Calibration finished and noise is now %.5f\n", Noise);
        }break;
        case BTMessageCommand::CMD_BARRIERCAM_START_DETECTION:{
            if(!pMsg->request()) m_Detecting = true;
            __android_log_print(ANDROID_LOG_INFO, "BarrierCamera", "Detection started");
        }break;
        case BTMessageCommand::CMD_BARRIERCAM_STOP_DETECTION:{
            if(!pMsg->request()) m_Detecting = false;
        }break;
        case BTMessageCommand::CMD_CHANGE_SENSITIVITY:{
            if(!pMsg->request()){
                // we got confirmation of change
                pMsg->getPayload((uint8_t*)&m_Sensitivity, sizeof(float));
                __android_log_print(ANDROID_LOG_INFO, "BarrierCamera", "Sensitivity changed to %f", m_Sensitivity);
            }
        }break;
        case BTMessageCommand::CMD_CONFIG_MEASUREMENT_POINTS:{
            if(!pMsg->request()){
                uint16_t Data[8];
                pMsg->getPayload((uint8_t*)Data, sizeof(uint16_t)*8);
                for(uint8_t i=0; i < 4; ++i){
                    m_MeasurementLines[i].Start = Data[i*2+0];
                    m_MeasurementLines[i].End = Data[i*2+1];
                    __android_log_print(ANDROID_LOG_INFO, "BarrierCamera", "Measurement point %d: %d : %d", i, m_MeasurementLines[i].Start, m_MeasurementLines[i].End);
                }
                __android_log_print(ANDROID_LOG_INFO, "BarrierCamera", "Received measurement points config");
                m_MeasurementPointsChanged = true;
            }
        }
        default: break;
    }//switch[cmd code]

}//processMessage

void BarrierCamera::processMessage(BTMessageImage *pMsg){
    if(nullptr == pMsg) return;
    __android_log_print(ANDROID_LOG_INFO, "BarrierCamera", "Received Image with dims %d x %d (%d bytes)\n", pMsg->width(), pMsg->height(), pMsg->imageSize());

    m_PreviewImageHeight = pMsg->height();
    m_PreviewImageWidth = pMsg->width();
    m_PreviewImageGrayscaleMode = (pMsg->format() == BTMessageImage::FMT_GRAYSCALE) ? true : false;
    delete[] m_pPreviewImageData;
    m_pPreviewImageData = new uint8_t[pMsg->imageSize()];
    memcpy(m_pPreviewImageData, pMsg->imageData(), pMsg->imageSize());
}

void BarrierCamera::updatePreviewImage(void){
    BTMessageImage ImgMsg;
    ImgMsg.request(true);

    MsgBuffer *pMsgBuf = new MsgBuffer();
    pMsgBuf->init(ImgMsg.size());
    ImgMsg.toStream(pMsgBuf->pBuffer, pMsgBuf->BufferSize);
    m_MsgQueue.push_back(pMsgBuf);
    __android_log_print(ANDROID_LOG_INFO, "BarrierCamera", "Requesting updated image");

}//updatePreviewImage

bool BarrierCamera::getPreviewImage(uint32_t *pWidth, uint32_t *pHeight, uint8_t **ppImgData, bool *pGrayscale){
    if(nullptr == m_pPreviewImageData) return false;
    (*pWidth) = m_PreviewImageWidth;
    (*pHeight) = m_PreviewImageHeight;
    (*ppImgData) = m_pPreviewImageData;
    (*pGrayscale) = m_PreviewImageGrayscaleMode;

    m_PreviewImageWidth = 0;
    m_PreviewImageHeight = 0;
    m_pPreviewImageData = nullptr;
    return true;
}//getPreviewImage

float BarrierCamera::sensitivity(void)const{
    return m_Sensitivity;
}//sensitivity

void BarrierCamera::sensitivity(float Value){
    BTMessageCommand CmdMsg;
    CmdMsg.request(true);
    CmdMsg.cmdCode(BTMessageCommand::CMD_CHANGE_SENSITIVITY);
    CmdMsg.setPayload((uint8_t*)&Value, sizeof(float));

    MsgBuffer *pMsg = new MsgBuffer();
    pMsg->init(CmdMsg.size());
    CmdMsg.toStream(pMsg->pBuffer, pMsg->BufferSize);
    m_MsgQueue.push_back(pMsg);

    __android_log_print(ANDROID_LOG_INFO, "BarrierCamera", "Attempting to change Sensitivity");

}//changeSensitivity

void BarrierCamera::getMeasurementLine(uint8_t ID, uint16_t *pStart, uint16_t *pEnd){
    if(ID >= 4) {
        __android_log_print(ANDROID_LOG_ERROR, "BarrierCamera", "Measurement line ID out of bounds %d", ID);
        return;
    }
    (*pStart) = m_MeasurementLines[ID].Start;
    (*pEnd) = m_MeasurementLines[ID].End;
}//getMeasurementLine

void BarrierCamera::configMeasurementLines(uint16_t Start[4], uint16_t Ends[4]){
    uint16_t Data[8];
    for(int i=0; i < 4; ++i){
        Data[i*2 + 0] = Start[i];
        Data[i*2 + 1] = Ends[i];
    }
    BTMessageCommand CmdMsg;
    CmdMsg.request(true);
    CmdMsg.cmdCode(BTMessageCommand::CMD_CONFIG_MEASUREMENT_POINTS);
    CmdMsg.setPayload((uint8_t*)Data, sizeof(uint16_t)*8);

    MsgBuffer *pMsg = new MsgBuffer();
    pMsg->init(CmdMsg.size());
    CmdMsg.toStream(pMsg->pBuffer, pMsg->BufferSize);
    m_MsgQueue.push_back(pMsg);

    __android_log_print(ANDROID_LOG_INFO, "BarrierCamera", "Attempting to change measurement points");
}//configMeasurementLines

bool BarrierCamera::measurementPointsChanged(bool Reset)  {
    bool Rval = m_MeasurementPointsChanged;
    if(Reset) m_MeasurementPointsChanged = false;
    return Rval;
}

