//
// Created by Tinkerer on 03.02.2021.
//

#include "StripPhoto.h"
#include <android/log.h>

StripPhoto::StripPhoto(void){
    m_StartTime = 0;
    m_Sensitivity = 1.25f;
    m_DiscardThreshold = 1000; // 0.5 seconds
    m_NoiseThreshold = 0;
    m_pBaseLineStrip = nullptr;
}//Constructor

StripPhoto::~StripPhoto(){
    clear();
}//Destructor

void StripPhoto::init(float Sensitivity, int32_t DiscardThreshold){
    clear();
    m_Sensitivity = Sensitivity;
    m_DiscardThreshold = DiscardThreshold;
    delete m_pBaseLineStrip;
    m_pBaseLineStrip = nullptr;
}//initialize

void StripPhoto::startRecording(int32_t StartTime) {
    for(auto &i: m_Strips) delete i;
    m_Strips.clear();
    m_StartTime = StartTime;
    m_LastBarrierTrigger = 0;
}//StartTime

void StripPhoto::clear(void){
    for(auto &i: m_Strips) delete i;
    m_Strips.clear();
    m_StartTime = 0;
    m_LastBarrierTrigger = 0;
    m_NoiseThreshold = 0;
    m_DiscardThreshold = 500; // default 500ms
    m_Sensitivity = 1.25; // default Sensitivity
}//clear

void StripPhoto::addStrip(uint8_t *pData, int32_t Width, int32_t Height, int64_t Timestamp){
    Strip *pStrip = new Strip();
    // initialize the new strip
    pStrip->init(pData, Width, Height, Timestamp);
    // if it was not too long ago that the barrier was triggered, the new strip is definetly valid
    m_Strips.push_back(pStrip);

    if(Timestamp - m_LastBarrierTrigger < m_DiscardThreshold){
        pStrip->State = true;
    }

    // we need at least two strips to do some useful computations
    if(m_Strips.size() <=  1 || nullptr == m_pBaseLineStrip) return;

    //const float BarrierError = computeBarrierErrorValue(m_pBaseLineStrip, pStrip);
    const float BarrierError = computeBarrierErrorValue(m_Strips[m_Strips.size()-1], m_Strips[m_Strips.size()-2]);
    if(BarrierError > (m_Sensitivity * m_NoiseThreshold) ){
        pStrip->LightBarrierTriggered = true;
        m_LastBarrierTrigger = Timestamp;

        // log barrier trigger, but not too often
        if(Timestamp - m_LastBarrierTrigger > 500)  __android_log_print(ANDROID_LOG_INFO, "StripPhoto", "Barrier triggered: %f %f", BarrierError, m_Sensitivity * m_NoiseThreshold);

        // go back in time and set valid till discard threshold
        for(uint32_t i=m_Strips.size() - 1; i > 0; --i){
            Strip *pS = m_Strips[i];
            if(pS->State) break; // at this point in time and further back we know the state
            if(Timestamp - pS->Timestamp < m_DiscardThreshold) pS->State = true;
        }
    }else {
        // check if we can discard some data (only if already calibrated)
        for(uint32_t i = m_Strips.size() - 1; i > 0; --i){
            Strip *pS = m_Strips[i];
            if(pS->State) break; // we already know state at this point
            // discard data if barrier trigger has been longer than threshold
            if( (Timestamp - pS->Timestamp) > m_DiscardThreshold){
                delete[] pS->pData;
                pS->pData = nullptr;
                pS->State = true;
            }
        }//for[strips back in time]
    }
}//addStrip

void StripPhoto::calibrate(bool DeleteCurrentStrips){
    __android_log_print(ANDROID_LOG_INFO, "StripPhoto", "Staring Calibration");

    if(m_Strips.size() == 0) return;

    m_pBaseLineStrip = new Strip();
    uint32_t BaselineID = m_Strips.size()/2;
    m_pBaseLineStrip->init(m_Strips[BaselineID]->pData, m_Strips[BaselineID]->Width, m_Strips[BaselineID]->Height, 0);

    m_NoiseThreshold = 0.0f;
    int32_t ValidSamples = 0;
    for(uint32_t i=1; i < m_Strips.size(); ++i){
        float Error = computeBarrierErrorValue(i-1, i);
        if(Error > 0.0f){
            ValidSamples++;
            m_NoiseThreshold += Error;
        }
    }
    m_NoiseThreshold /= (float)ValidSamples;
    if(DeleteCurrentStrips){
        for(auto &i: m_Strips) delete i;
        m_Strips.clear();
    }
    __android_log_print(ANDROID_LOG_INFO, "StripPhoto", "Calibration finished. Noise threshold now %f from %d strips and sensitivity %f.", m_NoiseThreshold, ValidSamples, m_Sensitivity);
}//calibrate

float StripPhoto::computeBarrierErrorValue(int32_t StripIDLeft, int32_t StripIDRight){
    float Rval = 0.0f;
    if(StripIDLeft < 0 || StripIDRight >= m_Strips.size()) return Rval;
    if(StripIDRight < 0 || StripIDRight >= m_Strips.size()) return Rval;

    const Strip *pLeft = m_Strips[StripIDLeft];
    const Strip *pRight = m_Strips[StripIDRight];

    const int32_t Mid = pLeft->Width/2;
    const int32_t Min = Mid - 3;
    const int32_t Max = Mid + 3;
    int32_t PixelCount = 0;


    for(int32_t i=0; i < pLeft->Height; ++i){
        for(int32_t k = Min; k < Max; ++k){
            const int32_t Index = (i*pLeft->Width*3) + (k * 3);
            const int32_t RedDiff = abs(pLeft->pData[Index + 0] - pRight->pData[Index + 0]);
            const int32_t GreenDiff = abs(pLeft->pData[Index + 1] - pRight->pData[Index + 1]);
            const int32_t BlueDiff = abs(pLeft->pData[Index + 2] - pRight->pData[Index + 2]);
            float PixelError = sqrtf(RedDiff * RedDiff + GreenDiff * GreenDiff + BlueDiff*BlueDiff);
            Rval += PixelError;
            PixelCount++;
        }//for[columns]
    }//for[Height]

    Rval /= (float)PixelCount;
    return Rval;
}//computeErrorValue

float StripPhoto::computeBarrierErrorValue(Strip *pLeft, Strip *pRight){
    float Rval = 0.0f;
    if(nullptr == pLeft || nullptr == pRight) return Rval;

    const int32_t Mid = pLeft->Width/2;

    for(int32_t i=0; i < pLeft->Height; ++i){
        const int32_t Index = (i*pLeft->Width*3) + (Mid * 3);
        const int32_t RedDiff = abs(pLeft->pData[Index + 0] - pRight->pData[Index + 0]);
        const int32_t GreenDiff = abs(pLeft->pData[Index + 1] - pRight->pData[Index + 1]);
        const int32_t BlueDiff = abs(pLeft->pData[Index + 2] - pRight->pData[Index + 2]);
        float PixelError = sqrtf(RedDiff * RedDiff + GreenDiff * GreenDiff + BlueDiff*BlueDiff);
        Rval += PixelError;
    }//for[Height]

    Rval /= (float)pLeft->Height;
    return Rval;
}//computeErrorValue

void StripPhoto::buildStripPhoto(void){
    // collect valid strips
    std::vector<Strip*> FinalStrips;
    m_StripPhoto.clear();
    generateFinalStrips(&FinalStrips);
    if(FinalStrips.size() == 0) return;

    // size of final image?
    uint32_t Width = 0;
    for(auto i: FinalStrips) Width += i->Width;
    uint32_t Height = FinalStrips[0]->Height;

    __android_log_print(ANDROID_LOG_INFO, "StripPhoto", "Building strip photo with resolution %d x %d.", Width, Height);

    uint8_t *pData = new uint8_t[Width*Height*3]; // final image data
    int32_t *pTimestamps = new int32_t[Width]; // timestamps for each pixel, "precise" to the millisecond :-P
    memset(pData, 0, Width*Height*3 * sizeof(uint8_t));
    memset(pTimestamps, 0, Width*sizeof(int32_t));
    uint32_t CurrentX = 0; // x cursor (width)

    // now we copy the image data
    for(uint32_t i=0; i < FinalStrips.size(); ++i){
        Strip *pS = FinalStrips[i];
        for(uint32_t y = 0; y < pS->Height; ++y){
            const int32_t RowIndex = y * Width * 3; // start of row (final image)
            for(uint32_t x=0; x < pS->Width; ++x){
                const int32_t Index = RowIndex + (CurrentX + x) * 3; // start of pixel (final image)
                const int32_t StripIndex = (y * pS->Width + x) * 3; // current pixel (strip)
                pData[Index + 0] = pS->pData[StripIndex + 0];
                pData[Index + 1] = pS->pData[StripIndex + 1];
                pData[Index + 2] = pS->pData[StripIndex + 2];
            }//for[rows]
        }//for[all columns]

        const float Step = pS->Width / (float)(pS->RelativeTimeInterval[1] - pS->RelativeTimeInterval[0]);

        for(uint32_t x = 0; x < pS->Width; ++x){
            pTimestamps[CurrentX + x] = pS->RelativeTimeInterval[0] + (int32_t)round(Step * x);
        }//for[all columns]

        CurrentX += pS->Width;
    }//for[all strips]

    // now create the timings table
    __android_log_print(ANDROID_LOG_INFO, "StripPhoto", "Strip photo generation finished.");

    m_StripPhoto.Height = Height;
    m_StripPhoto.Width = Width;
    m_StripPhoto.pColorData = pData;
    m_StripPhoto.pTimestamps = pTimestamps;

    // delete generated final strips
    for(auto &i: FinalStrips) delete i;
    FinalStrips.clear();

}//buildStripPhoto

void StripPhoto::buildInterpolatedStrip(Strip *pPast, Strip *pPresent, Strip *pInterpolated){
    int32_t TimeInterval = pPresent->Timestamp - pPast->Timestamp;

    // required pixels for each strip are 500px * Interval/1000.0 if we cover each second with 500px
    int32_t Width = (int32_t)(round(1.0 * TimeInterval)); // that is the number of columns we have to generate
    //__android_log_print(ANDROID_LOG_INFO, "StripPhoto", "Generating Strip with width %d @ %d ms", Width, TimeInterval);
    int32_t Height = pPast->Height;

    uint8_t *pColumnPast = new uint8_t[Height*3];
    uint8_t *pColumnPresent = new uint8_t[Height*3];
    uint8_t *pColumnInterpolated = new uint8_t[Height*3];

    pInterpolated->init(nullptr, Width, Height, 0);
    pInterpolated->RelativeTimeInterval[0] = pPast->Timestamp - m_StartTime;
    pInterpolated->RelativeTimeInterval[1] = pPresent->Timestamp - m_StartTime;

    const int32_t PastMid =pPast->Width/2 + 1;
    const int32_t PresentMid = pPresent->Width/2 + 1;

    const int32_t SampleWidth = 32;
    const int32_t HalfSampleWidth = SampleWidth/2;

    // always init first and last column
    pPast->getColumn(pColumnPast, PastMid);
    pPresent->getColumn(pColumnPresent, PresentMid);
    pInterpolated->setColumn(pColumnPast, 0);
    pInterpolated->setColumn(pColumnPresent, Width-1);

    float Factor = SampleWidth/(float)Width; // steps factor
    for(int32_t k = 1; k < Width; ++k){
        // we have 44 lines in total (22 from past and 22 from present)
        float RelativeIndexPast = (k-1) * Factor;
        float RelativeIndexPresent = k * Factor;

        int32_t IndexPast = (int32_t)round(RelativeIndexPast);
        int32_t IndexPresent = (int32_t)round(RelativeIndexPresent);

        // retrieve the correct columns
        if(IndexPast < HalfSampleWidth) pPast->getColumn(pColumnPast, PastMid + IndexPast);
        else pPresent->getColumn(pColumnPast, PresentMid - HalfSampleWidth + (IndexPast - HalfSampleWidth));

        if(IndexPresent < HalfSampleWidth) pPast->getColumn(pColumnPresent, PastMid + IndexPresent);
        else pPresent->getColumn(pColumnPresent, PresentMid - HalfSampleWidth + (IndexPresent - HalfSampleWidth));

        // create interpolated column
        // 0.5 is not correct, compute correct Alpha from relative indices
        const float Alpha = 0.5f;
        for(int32_t t = 0; t < Height; ++t){
            pColumnInterpolated[t*3 + 0] = (uint8_t)round(Alpha * pColumnPast[t * 3 + 0] + (1.0 - Alpha) * pColumnPresent[t*3 + 0]);
            pColumnInterpolated[t*3 + 1] = (uint8_t)round(Alpha * pColumnPast[t * 3 + 1] + (1.0 - Alpha) * pColumnPresent[t*3 + 1]);
            pColumnInterpolated[t*3 + 2] = (uint8_t)round(Alpha * pColumnPast[t * 3 + 2] + (1.0 - Alpha) * pColumnPresent[t*3 + 2]);
        }

        // and store
        pInterpolated->setColumn(pColumnInterpolated, k);
    }//for[width]

    delete[] pColumnPast;
    delete[] pColumnPresent;
    delete[] pColumnInterpolated;
}

void StripPhoto::generateFinalStrips(std::vector<Strip*> *pFinalStrips){
    if(nullptr == pFinalStrips) __android_log_print(ANDROID_LOG_ERROR, "StripPhoto::collectFinalStrips", "Nulpointer exception (pFinalStrips)");
    if(m_Strips.size() == 0) return;

    // generate Strips from past strip timestamp to present strip timestamp
    for(uint32_t i=1; i < m_Strips.size(); ++i){
       Strip *pPast = m_Strips[i-1];
       Strip *pPresent = m_Strips[i];
       // we skip if one of the two was discarded
       if(nullptr == pPast->pData || nullptr == pPresent->pData) continue;

       Strip *pInterpolated = new Strip();
       buildInterpolatedStrip(pPast, pPresent, pInterpolated);
       pFinalStrips->push_back(pInterpolated);

    }//for[all strips]

}//generateFinalStrips


void StripPhoto::retrieveStripPhotoColorData(uint8_t **ppColorData, bool CopyData){
    if(nullptr == ppColorData) return;
    if(CopyData) memcpy(*ppColorData, m_StripPhoto.pColorData, m_StripPhoto.Width * m_StripPhoto.Height * 3 * sizeof(uint8_t));
    else (*ppColorData) = m_StripPhoto.pColorData;
}
void StripPhoto::retrieveStripPhotoTimestamps(int32_t **ppTimestamps, bool CopyData){
    if(nullptr == ppTimestamps) return;
    if(CopyData) memcpy(*ppTimestamps, m_StripPhoto.pTimestamps, m_StripPhoto.Width * sizeof(int32_t));
    else (*ppTimestamps) = m_StripPhoto.pTimestamps;
}

void StripPhoto::retrieveStripPhotoSize(int32_t *pWidth, int32_t *pHeight){
    if(nullptr != pWidth) (*pWidth) = m_StripPhoto.Width;
    if(nullptr != pHeight) (*pHeight) = m_StripPhoto.Height;
}