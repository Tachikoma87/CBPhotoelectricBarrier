//
// Created by Tinkerer on 09.02.2021.
//
#include <android/log.h>
#include "StartingFlap.h"

StartingFlap::StartingFlap(void){

}//Constructor

StartingFlap::~StartingFlap(void){

}//Destructor

void StartingFlap::init(void){
    m_Latency = -1;
    m_LastPingTimestamp = 0;
    m_LastRaceStart = 0;
    m_LastStatusUpdateTimestamp = 0;

    m_BatteryCharge = 0;
    m_RelayBatteryCharge = 0;
    m_RSSI = 0;
    m_Latency = 0;
    m_ClockOffset = 0;
}//initialize

void StartingFlap::clear(void){

}//clear

void StartingFlap::update(uint32_t Timestamp){
    m_CurrentTime = Timestamp;
    m_InternalClock = Timestamp;

    // do we need to ping? We ping evey two seconds
    if(m_CurrentTime - m_LastPingTimestamp > 5050){
        // ping every 2 seconds
        BTMessagePing PingMsg;
        PingMsg.timestamp(m_InternalClock);

        MsgBuffer *pBuffer = new MsgBuffer();
        pBuffer->BufferSize = PingMsg.size();
        pBuffer->pBuffer = new uint8_t[pBuffer->BufferSize];
        PingMsg.toStream(pBuffer->pBuffer, pBuffer->BufferSize);
        m_MsgQueue.push_back(pBuffer);
        m_LastPingTimestamp = Timestamp;

        //__android_log_print(ANDROID_LOG_INFO, "StartingFlap", "Requesting Ping");
    }

    // do we need to make a status update?
    if(m_CurrentTime - m_LastStatusUpdateTimestamp > 3000){
        BTMessageStatus StatusMsg;
        StatusMsg.request(true);
        StatusMsg.androidClock(m_InternalClock);

        MsgBuffer *pBuffer = new MsgBuffer();
        pBuffer->init(StatusMsg.size());
        StatusMsg.toStream(pBuffer->pBuffer, pBuffer->BufferSize);
        m_MsgQueue.push_back(pBuffer);
        m_LastStatusUpdateTimestamp = m_CurrentTime;
    }

    switch(m_RequestStatus){
        case SRS_WAIT_REQ_ACK:{
            if(Timestamp - m_LastRequestUpdate > 250){
                // send out the staring request
                BTMessageStartSignal SigMsg;
                SigMsg.request(true);
                SigMsg.acknowledge(false);
                SigMsg.timestamp(0);
                MsgBuffer *pBuffer = new MsgBuffer();
                pBuffer->init(SigMsg.size());
                SigMsg.toStream(pBuffer->pBuffer, pBuffer->BufferSize);
                m_MsgQueue.push_back(pBuffer);
                m_LastRequestUpdate = Timestamp;
                __android_log_print(ANDROID_LOG_INFO, "StartingFlap", "Issued Bluetooth package start-request");
            }
        }break;
        case SRS_WAIT_SIGNAL:{
            //__android_log_print(ANDROID_LOG_INFO, "StartingFlap", "Waiting for start signal");
        }break;
        default: break;
    }

}//update

void StartingFlap::processMessage(BTMessagePing *pMsg){
    if(nullptr == pMsg) return;
    if(pMsg->request()) return; // we do not respond to ping requests
    m_Latency = m_InternalClock - pMsg->timestamp();
}//processMessage

void StartingFlap::processMessage(BTMessageStatus *pMsg){
    if(nullptr == pMsg) return;
    if(pMsg->request()) return;// we do not respond to requests

    if(!pMsg->request()){
        m_RSSI = pMsg->rssi();
        m_BatteryCharge = pMsg->batteryChargeDevice();
        m_RelayBatteryCharge = pMsg->batteryChargeRelay();
        m_Latency = m_InternalClock - pMsg->androidClock();
        int32_t Offset = pMsg->androidClock() - (pMsg->deviceClock() - m_Latency/2);
        if(m_ClockOffset == 0) m_ClockOffset = Offset;
        else if(m_ClockOffset - Offset > -100 && m_ClockOffset - Offset < 100) m_ClockOffset = (m_ClockOffset + Offset)/2;
        __android_log_print(ANDROID_LOG_INFO, "StartingFlap", "RSSI is %d", m_RSSI);
    }else{
        // should not happen
    }

}//processMessage

void StartingFlap::processMessage(BTMessageStartSignal *pMsg){
    if(nullptr == pMsg) return;
    if(pMsg->request() && pMsg->acknowledge()){
        // received confirmation for start signal
        m_RequestStatus = SRS_WAIT_SIGNAL;
    }else if(!pMsg->request() && !pMsg->acknowledge() && pMsg->timestamp() != 0){
        // we actually received the start signal
        //m_LastRaceStart = pMsg->timestamp();
        // \todo Compute correct starting time after status message is implemented
        m_LastRaceStart = pMsg->timestamp() + m_ClockOffset;
        m_RequestStatus = SRS_UNKNOWN;

        // send back configuration
        BTMessageStartSignal SigMsg;
        SigMsg = (*pMsg);
        SigMsg.acknowledge(true);
        MsgBuffer *pBuffer = new MsgBuffer();
        pBuffer->init(pMsg->size());
        SigMsg.toStream(pBuffer->pBuffer, pBuffer->BufferSize);
        m_MsgQueue.push_back(pBuffer);
        __android_log_print(ANDROID_LOG_INFO, "StartingFlap", "Received start signal! Compensated %d ms", m_InternalClock - m_LastRaceStart);
    }else{
        // this should not happen
        __android_log_print(ANDROID_LOG_ERROR, "StartingFlap", "Unhandled Bluetooth start signal message encountered");
    }
}//processMessage

uint32_t StartingFlap::fetchMessageBuffer(uint8_t **ppBuffer){
    if(nullptr == ppBuffer || m_MsgQueue.empty()) return 0;

    uint32_t Rval = 0;
    for(auto &i: m_MsgQueue){
        if(nullptr == i) continue;
        (*ppBuffer) = i->pBuffer;
        Rval = i->BufferSize;
        i->pBuffer = nullptr;
        delete i;
        i = nullptr;
        break;
    }
    if(0 == Rval) m_MsgQueue.clear();
    return Rval;
}//fetchMessageBuffer

uint8_t StartingFlap::batteryCharge(void)const{
    return m_BatteryCharge;
}//batteryCharge

uint8_t StartingFlap::relayBatteryCharge(void)const{
    return m_RelayBatteryCharge;
};//relayBatteryCharge

int8_t StartingFlap::rssi(void)const{
    return m_RSSI;
}//rssi

int32_t StartingFlap::latency(void)const{
    return m_Latency;
}

bool StartingFlap::startRace() {
    // on next update request will be send
    m_LastRequestUpdate = 0;
    m_RequestStatus = SRS_WAIT_REQ_ACK;
    return true;
}

uint32_t StartingFlap::raceStarted(bool Reset){
    uint32_t Rval = m_LastRaceStart;
    if(Reset) m_LastRaceStart = 0;
    return Rval;
}
