//
// Created by Tinkerer on 03.02.2021.
//

#ifndef TF_CHAMPION_STRIPPHOTO_H
#define TF_CHAMPION_STRIPPHOTO_H

#include <inttypes.h>
#include <vector>
#include <Eigen/Eigen>

class StripPhoto {
public:
    StripPhoto(void);
    ~StripPhoto();

    void init(float Sensitivity, int32_t DiscardThreshold);
    void startRecording(int32_t StartTime);
    void clear(void);

    void addStrip(uint8_t *pData, int32_t Width, int32_t Height, int64_t Timestamp);

    void calibrate(bool DeleteCurrentStrips);
    void buildStripPhoto(void);

    void retrieveStripPhotoColorData(uint8_t **ppColorData, bool CopyData);
    void retrieveStripPhotoTimestamps(int32_t **ppTimestamps, bool CopyData);
    void retrieveStripPhotoSize(int32_t *pWidth, int32_t *pHeight);
    

private:

    struct Strip{
        int32_t Width;  ///< Width of the strip
        int32_t Height; ///< Height of the strip
        uint8_t *pData; ///< Actual image data (RGB values)
        int64_t Timestamp; ////< Timestamp at CENTER of the image
        bool LightBarrierTriggered;
        bool State; ///< False if state is unknown.
        int32_t RelativeTimeInterval[2]; ///< Relative time interval the interpolated strip covers

        Strip(void){
            Width = 0;
            Height = 0;
            pData = nullptr;
            Timestamp = 0;
            LightBarrierTriggered = false;
            State = false;
        }

        ~Strip(void){
            clear();
        }

        void init(uint8_t *pData, int32_t Width, int32_t Height, int64_t Timestamp){
            clear();
            this->pData = new uint8_t[Width*Height*3];
            if(pData != nullptr) memcpy(this->pData, pData, Width*Height*3*sizeof(uint8_t));
            this->Width = Width;
            this->Height = Height;
            this->Timestamp = Timestamp;
            this->LightBarrierTriggered = false;
            this->State = false;
        }//initialize

        void clear(void){
            Width = 0;
            Height = 0;
            delete[] pData;
            pData = nullptr;
            Timestamp = 0;
            LightBarrierTriggered = false;
            State = false;
        }//clear

        void getColumn(uint8_t *pData, int32_t ColumnID)const{
            if(nullptr == pData) return;

            for(int32_t i=0; i< Height; ++i){
                int32_t Index = (i*Width + ColumnID)*3;
                pData[i*3 + 0] = this->pData[Index + 0];
                pData[i*3 + 1] = this->pData[Index + 1];
                pData[i*3 + 2] = this->pData[Index + 2];
            }
        }//retrieveColumn

        void setColumn(const uint8_t *pData, int32_t ColumnID){
            if(nullptr == pData) return;

            for(int32_t i=0; i < Height; ++i){
                int32_t Index = (i*Width + ColumnID)*3;
                this->pData[Index + 0] = pData[i*3 + 0];
                this->pData[Index + 1] = pData[i*3 + 1];
                this->pData[Index + 2] = pData[i*3 + 2];
            }
        }
    };//Strip

    struct TimedStripPhoto{
        uint8_t *pColorData;
        int32_t *pTimestamps; ///< Relative timestamps in milliseconds, one for each columns so length equals Width
        uint32_t Width;
        uint32_t Height;

        TimedStripPhoto(){
            pColorData = nullptr;
            pTimestamps = nullptr;
            Width = 0;
            Height = 0;
        }

        ~TimedStripPhoto(){
            clear();
        }

        void clear(){
            delete[] pColorData;
            delete[] pTimestamps;
            pColorData = nullptr;
            pTimestamps = nullptr;
            Width = 0;
            Height = 0;
        }
    };

    float computeBarrierErrorValue(int32_t StripIDLeft, int32_t StripIDRight);
    float computeBarrierErrorValue(Strip *pLeft, Strip *pRight);

    void generateFinalStrips(std::vector<Strip*> *pFinalStrips);
    void buildInterpolatedStrip(Strip *pPast, Strip *pPresent, Strip *pInterpolated);

    int64_t m_StartTime; ///< Start of the heat. Required to compute relative time
    int32_t m_DiscardThreshold; ///< Discarding data threshold in milliseconds
    int32_t m_LastBarrierTrigger;
    std::vector<Strip*> m_Strips;
    float m_NoiseThreshold;
    float m_Sensitivity;

    TimedStripPhoto m_StripPhoto;

    Strip *m_pBaseLineStrip;
};


#endif //TF_CHAMPION_STRIPPHOTO_H
