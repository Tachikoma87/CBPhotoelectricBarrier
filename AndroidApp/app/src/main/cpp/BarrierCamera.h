//
// Created by Tinkerer on 07.03.2021.
//

#ifndef TF_CHAMPION_BARRIERCAMERA_H
#define TF_CHAMPION_BARRIERCAMERA_H

#include <inttypes.h>
#include <vector>
#include "BTCommunication.hpp"

class BarrierCamera {
public:
    BarrierCamera(void);
    ~BarrierCamera(void);

    void init(int8_t CameraID);

    void update(uint32_t Timestamp);

    void updatePreviewImage(void);
    bool getPreviewImage(uint32_t *pWidth, uint32_t *pHeight, uint8_t **ppImgData, bool *pGrayscale);

    uint16_t latency(void)const;
    uint8_t batteryCharge(void)const;
    uint32_t lastBarrierBreach(bool Reset);
    void startDetection(void);
    void stopDetection(void);
    bool isConnected(void)const;
    bool isDetecting(void)const;
    float sensitivity(void)const;
    void sensitivity(float Value);

    void getMeasurementLine(uint8_t ID, uint16_t *pStart, uint16_t *pEnd);
    void configMeasurementLines(uint16_t Start[4], uint16_t Ends[4]);

    uint32_t fetchMessageBuffer(uint8_t **ppBuffer);
    void processMessage(BTMessagePing *pMsg);
    void processMessage(BTMessageStatus *pMsg);
    void processMessage(BTMessageCommand *pMsg);
    void processMessage(BTMessageImage *pMsg);

    bool measurementPointsChanged(bool Reset = true);

private:
    struct MsgBuffer{
        uint8_t *pBuffer;
        uint32_t BufferSize;

        MsgBuffer(uint32_t Size = 0){
            pBuffer = nullptr;
            BufferSize = 0;
            if(Size != 0) init(Size);
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

    struct MeasurementLine{
        uint16_t Start;
        uint16_t End;
    };

    uint32_t m_InternalClock; ///< Up time of this device.
    int64_t m_DeviceClockOffset; ///< Difference between internal clock and device clock

    uint32_t m_LastPingSend; // pings are currently not used
    uint32_t m_LastMessageReceived;
    uint32_t m_LastStatusUpdateRequest;

    uint16_t m_Latency;

    bool m_Detecting; ///< Device status. If detection on do not use Bluetooth communication

    float m_Sensitivity;
    uint8_t m_BatteryCharge;

    std::vector<MsgBuffer*> m_MsgQueue;
    uint32_t m_PreviewImageWidth;
    uint32_t m_PreviewImageHeight;
    uint8_t *m_pPreviewImageData;
    int64_t m_LastBarrierBreach;
    bool m_PreviewImageGrayscaleMode; // does device capture grayscale or color images?

    MeasurementLine m_MeasurementLines[4];
    bool m_RetrievedConfig;
    bool m_MeasurementPointsChanged;

};

#endif //TF_CHAMPION_BARRIERCAMERA_H
