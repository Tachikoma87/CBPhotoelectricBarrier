//
// Created by Tinkerer on 03.02.2021.
//

#ifndef TF_CHAMPION_CPPCONTEXTMANAGER_H
#define TF_CHAMPION_CPPCONTEXTMANAGER_H

#include <vector>
#include <inttypes.h>
#include <android/log.h>

#include "StripPhoto.h"
#include "StartingFlap.h"
#include "BarrierCamera.h"


// this acts as a global manager class to
class CPPContextManager {
public:

    struct MessageBuffer{
        uint8_t *pBuffer;
        uint32_t BufferSize;
        uint32_t Pointer;

        MessageBuffer(){
            pBuffer = new uint8_t[64];
            BufferSize = 64;
            Pointer = 0;
        }

        ~MessageBuffer(){
            delete[] pBuffer;
            pBuffer = nullptr;
            Pointer = 0;
        }

        void write(uint8_t Char){
            if(Pointer == BufferSize){
                uint8_t *pNewBuffer = new uint8_t[BufferSize*2];
                memcpy(pNewBuffer, pBuffer, sizeof(uint8_t)*BufferSize);
                delete[] pBuffer;
                pBuffer = pNewBuffer;
                BufferSize *= 2;
            }
            pBuffer[Pointer] = Char;
            Pointer++;
        }

        void reset(void){
            Pointer = 0;
        }
    };

    CPPContextManager(void){
        m_pStripPhoto = new StripPhoto();
        m_pStartingFlap = new StartingFlap();

    }

    ~CPPContextManager(void){

    }

    StripPhoto *getStripPhotoInstance(int32_t ID){
        return m_pStripPhoto;
    }

    StartingFlap *getStartingFlap(int32_t ID){
        return m_pStartingFlap;
    }

    BarrierCamera *getBarrierCamera(int32_t ID){
        return m_BarrierCams[ID];
    }

    int32_t createBarrierCamera(void){
        int32_t Rval = m_BarrierCams.size();
        m_BarrierCams.push_back(new BarrierCamera());
        return Rval;
    }

    int32_t createMessageBuffer(void){
        int32_t Rval = m_MsgBuffers.size();
        m_MsgBuffers.push_back(new MessageBuffer());
        return Rval;
    }

    MessageBuffer *getMessageBuffer(int32_t BufferID){
        return m_MsgBuffers[BufferID];
    }

private:
    StripPhoto *m_pStripPhoto;
    StartingFlap *m_pStartingFlap;
    std::vector<BarrierCamera*> m_BarrierCams;
    std::vector<MessageBuffer*> m_MsgBuffers;

};


#endif //TF_CHAMPION_CPPCONTEXTMANAGER_H
