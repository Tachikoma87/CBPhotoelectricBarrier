//
// Created by Tinkerer on 09.02.2021.
//

#ifndef TF_CHAMPION_STARTINGFLAP_H
#define TF_CHAMPION_STARTINGFLAP_H

#include <inttypes.h>
#include <vector>
#include "BTCommunication.hpp"

/**
 * Physically existing starting flap. It is always assumed that communication is handled by bluetooth
 *
 */
class StartingFlap {
public:

    StartingFlap(void);
    ~StartingFlap(void);

    void init(void);
    void clear(void);
    void update(uint32_t Timestamp);

    void processMessage(BTMessagePing *pMsg);
    void processMessage(BTMessageStatus *pMsg);
    void processMessage(BTMessageStartSignal *pMsg);

    bool startRace(void);
    uint32_t raceStarted(bool Reset = true);

    uint32_t fetchMessageBuffer(uint8_t **ppBuffer);

    uint8_t batteryCharge(void)const;
    uint8_t relayBatteryCharge(void)const;
    int8_t rssi(void)const;
    int32_t latency(void)const;

private:
    struct MsgBuffer{
        uint8_t *pBuffer;
        uint32_t BufferSize;

        MsgBuffer(void){
            pBuffer = nullptr;
            BufferSize = 0;
        }

        ~MsgBuffer(){
            clear();
        }

        void init(uint32_t Size){
            clear();
            pBuffer = new uint8_t[Size];
            BufferSize = Size;
        }

        void clear(){
            delete[] pBuffer;
            pBuffer = nullptr;
            BufferSize = 0;
        }

    };

    enum StartRequestStatus{
        SRS_UNKNOWN,
        SRS_WAIT_REQ_ACK, // we wait for the flap to confirm our starting request
        SRS_WAIT_SIGNAL, // we wait for the actual start signal
    };

    uint8_t m_BatteryCharge;
    uint8_t m_RelayBatteryCharge;
    int8_t m_RSSI;
    int32_t m_Latency;
    uint32_t m_InternalClock; // current time
    uint32_t m_ClockOffset;

    uint32_t m_LastPingTimestamp;
    uint32_t m_LastStatusUpdateTimestamp;
    std::vector<MsgBuffer*> m_MsgQueue;
    uint32_t m_CurrentTime;

    uint32_t m_LastRequestUpdate;
    StartRequestStatus m_RequestStatus;
    uint32_t m_LastRaceStart;

};


#endif //TF_CHAMPION_STARTINGFLAP_H
