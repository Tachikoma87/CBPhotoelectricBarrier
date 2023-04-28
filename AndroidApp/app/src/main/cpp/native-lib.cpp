#include <jni.h>
#include <string>
#include <android/log.h>
#include "CPPContextManager.h"

struct Color{
    static uint8_t red(jint Val){
        return (uint8_t)((Val >> 16) & 0xFF);
    }//red

    static uint8_t green(jint Val){
        return (uint8_t)(((Val >> 8) & 0xFF));
    }//green

    static uint8_t blue(jint Val){
        return (uint8_t)((Val >> 0) & 0xFF);
    }//blue

    static uint8_t alpha(jint Val){
        return (uint8_t)((Val >> 24) & 0xFF);
    }//alpha

    static int32_t buildFromRGBA(uint8_t R, uint8_t G, uint8_t B, uint8_t A){
        int32_t Rval = ((int32_t)A) << 24;
        Rval |= ((int32_t)R) << 16;
        Rval |= ((int32_t)G) << 8;
        Rval |= ((int32_t)B) << 0;
        return Rval;
    }//buildFromRGBA
};

struct BTMessageBuffer{
    uint8_t *pBuffer;
    uint32_t Pointer;

    BTMessageBuffer(){
        pBuffer = new uint8_t[512];
        Pointer = 0;
    }

    ~BTMessageBuffer(){
        delete[] pBuffer;
        pBuffer = nullptr;
        Pointer = 0;
    }

    void write(uint8_t Char){
        pBuffer[Pointer++] = Char;
    }
};

// global manager instance
CPPContextManager g_Context;
BTMessageBuffer g_BTBuffer;

extern "C"{

    // Below here is all strip photo camera interfacing stuff

    JNIEXPORT void JNICALL Java_com_crossforge_tfchampion_OpticalRecorder_initializeStripPhotoCamera(JNIEnv *env, jobject j, jint CameraID, jfloat Sensitivity, jint DiscardThreshold){
        g_Context.getStripPhotoInstance(CameraID)->init(Sensitivity, DiscardThreshold);
    }

    JNIEXPORT void JNICALL Java_com_crossforge_tfchampion_OpticalRecorder_startRecording(JNIEnv *env, jobject j, jint CameraID, jlong Timestamp){
        g_Context.getStripPhotoInstance(CameraID)->startRecording(Timestamp);
    }

    JNIEXPORT void JNICALL Java_com_crossforge_tfchampion_OpticalRecorder_calibrateWithExistingData(JNIEnv *env, jobject j, jint CameraID){
        g_Context.getStripPhotoInstance(CameraID)->calibrate(true);
    }//calibrateWithExistingData


    JNIEXPORT void JNICALL Java_com_crossforge_tfchampion_OpticalRecorder_addStrip(JNIEnv* env, jobject j, int32_t CameraID, jintArray pColorData, int32_t Width, int32_t Height, jlong Timestamp){
        // convert pColorArray to RGB uint8_T color array
        jint *pBody = env->GetIntArrayElements(pColorData, JNI_FALSE);

        uint8_t *pRGBData = new uint8_t[Width*Height*3];
        for(int32_t y = 0; y < Height; ++y){
            for(int32_t x = 0; x < Width; ++x){
                int32_t Index = (y*Width + x) * 3;
                int32_t OrigIndex = (y*Width) + x;
                pRGBData[Index + 0] = Color::red(pBody[OrigIndex]);
                pRGBData[Index + 1] = Color::green(pBody[OrigIndex]);
                pRGBData[Index + 2] = Color::blue(pBody[OrigIndex]);
            }
        }
        g_Context.getStripPhotoInstance(CameraID)->addStrip(pRGBData, Width, Height, Timestamp);
        env->ReleaseIntArrayElements(pColorData, pBody, 0);
        delete[] pRGBData;
    }//addStrip

    JNIEXPORT void JNICALL Java_com_crossforge_tfchampion_OpticalRecorder_buildStripPhoto(JNIEnv *env, jobject j, jint CameraID){
        g_Context.getStripPhotoInstance(CameraID)->buildStripPhoto();
    }

    JNIEXPORT jintArray JNICALL Java_com_crossforge_tfchampion_OpticalRecorder_retrieveStripPhotoDimensions(JNIEnv *env, jobject j, jint CameraID){
        int32_t Width = 0;
        int32_t Height = 0;
        g_Context.getStripPhotoInstance(CameraID)->retrieveStripPhotoSize(&Width, &Height);

        jintArray Rval = env->NewIntArray(2);
        jint *pBuffer = env->GetIntArrayElements(Rval, JNI_FALSE);
        pBuffer[0] = Width;
        pBuffer[1] = Height;
        env->ReleaseIntArrayElements(Rval, pBuffer, 0);
        return Rval;
    }

    JNIEXPORT jintArray JNICALL Java_com_crossforge_tfchampion_OpticalRecorder_retrieveStripPhotoColorData(JNIEnv* env, jobject j, jint CameraID) {
        uint8_t *pData = nullptr;
        int32_t Width = 0;
        int32_t Height = 0;

        g_Context.getStripPhotoInstance(CameraID)->retrieveStripPhotoSize(&Width, &Height);
        g_Context.getStripPhotoInstance(CameraID)->retrieveStripPhotoColorData(&pData, false);
        if(pData == nullptr) return nullptr;

        jintArray  ColorData = env->NewIntArray(Width*Height);
        jint *pBuffer = env->GetIntArrayElements(ColorData, JNI_FALSE);
        // now build the image data
        for(uint32_t y = 0; y < Height; ++y){
            for(uint32_t x = 0; x < Width; ++x){
                uint32_t Index = (y * Width + x) * 3;
                pBuffer[y * Width + x] = Color::buildFromRGBA(pData[Index+0], pData[Index+1], pData[Index+2], 255);
            }//for[all rows]
        }//for[all columns]

        // do not delete image data (we did not request a copy)
        pData = nullptr;
        env->ReleaseIntArrayElements(ColorData, pBuffer, 0);
        __android_log_print(ANDROID_LOG_INFO, "StripPhoto", "Strip photo created and returned. Dims: %dx%d", Width, Height);
        return ColorData;
    }//buildStripPhoto

    JNIEXPORT jintArray JNICALL Java_com_crossforge_tfchampion_OpticalRecorder_retrieveStripPhotoTimestamps(JNIEnv *env, jobject j, jint CameraID){
        int32_t Width = 0;
        int32_t Height = 0;
        int32_t *pTimestamps = nullptr;
        g_Context.getStripPhotoInstance(CameraID)->retrieveStripPhotoSize(&Width, &Height);
        g_Context.getStripPhotoInstance(CameraID)->retrieveStripPhotoTimestamps(&pTimestamps, false);

        jintArray Rval = env->NewIntArray(Width);
        jint *pBuffer = env->GetIntArrayElements(Rval, JNI_FALSE);
        for(int32_t i=0; i < Width; ++i) pBuffer[i] = pTimestamps[i];
        env->ReleaseIntArrayElements(Rval, pBuffer, 0);
        return Rval;
    }

    // below here is all starting flap interfacing stuff
    JNIEXPORT void JNICALL Java_com_crossforge_tfchampion_FlapCommunication_initStartingFlap(JNIEnv *env, jobject j){
        g_Context.getStartingFlap(0)->init();
    }

    JNIEXPORT jbyteArray JNICALL Java_com_crossforge_tfchampion_FlapCommunication_updateFlap(JNIEnv *env, jobject j, jobject Data, jlong Timestamp){
        StartingFlap *pFlap = g_Context.getStartingFlap(0);
        jbyteArray Rval = nullptr;
        pFlap->update(Timestamp);

        uint8_t *pOutBuffer = nullptr;
        uint32_t OutBufferSize = pFlap->fetchMessageBuffer(&pOutBuffer);
        if(OutBufferSize > 0){
            // write to out buffer
            Rval = env->NewByteArray(OutBufferSize);
            env->SetByteArrayRegion(Rval, 0, (jsize)OutBufferSize, (const jbyte*)pOutBuffer);
            delete[] pOutBuffer;
            pOutBuffer = nullptr;
        }

        jclass C = env->GetObjectClass(Data);
        jfieldID FlapChargeID = env->GetFieldID(C, "mFlapBatteryCharge", "I");
        jfieldID RelayChargeID = env->GetFieldID(C, "mRelayBatteryCharge", "I");
        jfieldID RssiID = env->GetFieldID(C, "mRSSI", "I");
        jfieldID LatencyID = env->GetFieldID(C, "mLatency", "I");
        jfieldID LastRaceStartId = env->GetFieldID(C, "mLastRaceStart", "J");

        env->SetIntField(Data, FlapChargeID, (jint)pFlap->batteryCharge());
        env->SetIntField(Data, RelayChargeID, (jint)pFlap->relayBatteryCharge());
        env->SetIntField(Data, RssiID, (jint)pFlap->rssi());
        env->SetIntField(Data, LatencyID, (jint)pFlap->latency());
        env->SetLongField(Data, LastRaceStartId, (jlong)pFlap->raceStarted());

        return Rval;
    }//updateFlap

    void processBluetoothPackage(){
        // a full package arrived
        BTMessage::BTPackageType Type = BTMessage::fetchPackageTypeFromStream(g_BTBuffer.pBuffer, g_BTBuffer.Pointer);
        switch(Type){
            case BTMessage::BTPCK_PING:{
                BTMessagePing PingIn;
                PingIn.fromStream(g_BTBuffer.pBuffer, g_BTBuffer.Pointer);
                g_Context.getStartingFlap(0)->processMessage(&PingIn);
            }break;
            case BTMessage::BTPCK_STARTSIGNAL:{
                BTMessageStartSignal SigIn;
                SigIn.fromStream(g_BTBuffer.pBuffer, g_BTBuffer.Pointer);
                g_Context.getStartingFlap(0)->processMessage(&SigIn);
            }break;
            case BTMessage::BTPCK_STATUS:{
                BTMessageStatus StatusIn;
                StatusIn.fromStream(g_BTBuffer.pBuffer, g_BTBuffer.Pointer);
                g_Context.getStartingFlap(0)->processMessage(&StatusIn);
                __android_log_print(ANDROID_LOG_INFO, "Native-Lib", "Processed status Bluettoth message");
            }break;
            default: break;
        }//switch[BT message type]
        // reset input stream
        g_BTBuffer.Pointer = 0;
    }//processBluetoothPackage

    JNIEXPORT void JNICALL Java_com_crossforge_tfchampion_FlapCommunication_receiveBluetoothData(JNIEnv *env, jobject j, jbyteArray Data){
        int32_t MsgSize = env->GetArrayLength(Data);
        jbyte *pData = env->GetByteArrayElements(Data, 0);
        for(int32_t i=0; i < MsgSize; ++i){
            g_BTBuffer.write(pData[i]);
            if(pData[i] == BTMessage::cmdCharacter()){
                processBluetoothPackage();
            }
        }

        env->ReleaseByteArrayElements(Data, pData, 0);
    }

    JNIEXPORT jboolean JNICALL Java_com_crossforge_tfchampion_FlapCommunication_issueStartRequest(JNIEnv *env, jobject j){
        StartingFlap *pFlap = g_Context.getStartingFlap(0);
        return (jboolean)pFlap->startRace();
    }


/*
 * values of Signature (*sig):
 * jclass DataHolder = env->GetObjectClass(Data);
 * jfieldID ReceiveStreamID = env->GetFieldID(DataHolder, "mReceiveBuffer", "[B");
 * jobject ReceiveStreamObj = env->GetObjectField(Data, ReceiveStreamID);
Z boolean
B byte
C char
S short
I int
J long
F float
D double

For non-primitive types, the signature is of form

L fully-qualified-class ;

For arrays, [ is added.
 */

    JNIEXPORT jbyteArray JNICALL Java_com_crossforge_tfchampion_TFCBarrierCamera_updateBarrierCamera(JNIEnv *env, jobject j, jint CameraID, jobject CameraData, jlong Timestamp){
        BarrierCamera *pCam = g_Context.getBarrierCamera(CameraID);
        pCam->update((uint32_t)Timestamp);

        uint8_t *pBTOutBuffer = nullptr;
        uint32_t ByteCount = pCam->fetchMessageBuffer(&pBTOutBuffer);

        jbyteArray Rval = nullptr;

        if(ByteCount > 0){
            Rval = env->NewByteArray(ByteCount);
            env->SetByteArrayRegion(Rval, 0, ByteCount, (const jbyte*)pBTOutBuffer);
        }

        delete[] pBTOutBuffer;
        pBTOutBuffer = nullptr;

        // update data
        jclass DataClass = env->GetObjectClass(CameraData);

        jfieldID LatencyID = env->GetFieldID(DataClass, "Latency", "I");
        jfieldID ConnectedID = env->GetFieldID(DataClass, "Connected", "Z");
        jfieldID DetectingID = env->GetFieldID(DataClass, "Detecting", "Z");
        jfieldID SensitivityID = env->GetFieldID(DataClass, "Sensitivity", "F");
        jfieldID BatteryChargeID = env->GetFieldID(DataClass, "BatteryCharge", "I");
        env->SetIntField(CameraData, LatencyID, pCam->latency());
        env->SetBooleanField(CameraData, ConnectedID, pCam->isConnected());
        env->SetBooleanField(CameraData, DetectingID, pCam->isDetecting());
        env->SetFloatField(CameraData, SensitivityID, pCam->sensitivity());
        env->SetIntField(CameraData, BatteryChargeID, pCam->batteryCharge());

        if(pCam->measurementPointsChanged()){
            jfieldID MeasurePoint1StartID = env->GetFieldID(DataClass, "MeasurePoint1Start", "S");
            jfieldID MeasurePoint1EndID = env->GetFieldID(DataClass, "MeasurePoint1End", "S");
            jfieldID MeasurePoint2StartID = env->GetFieldID(DataClass, "MeasurePoint2Start", "S");
            jfieldID MeasurePoint2EndID = env->GetFieldID(DataClass, "MeasurePoint2End", "S");
            jfieldID MeasurePoint3StartID = env->GetFieldID(DataClass, "MeasurePoint3Start", "S");
            jfieldID MeasurePoint3EndID = env->GetFieldID(DataClass, "MeasurePoint3End", "S");
            jfieldID MeasurePoint4StartID = env->GetFieldID(DataClass, "MeasurePoint4Start", "S");
            jfieldID MeasurePoint4EndID = env->GetFieldID(DataClass, "MeasurePoint4End", "S");
            jfieldID MeasurePointChanged = env->GetFieldID(DataClass, "MeasurePointsChanged", "Z");

            uint16_t Start, End;
            pCam->getMeasurementLine(0, &Start, &End);
            env->SetShortField(CameraData, MeasurePoint1StartID, Start);
            env->SetShortField(CameraData, MeasurePoint1EndID, End);
            pCam->getMeasurementLine(1, &Start, &End);
            env->SetShortField(CameraData, MeasurePoint2StartID, Start);
            env->SetShortField(CameraData, MeasurePoint2EndID, End);
            pCam->getMeasurementLine(2, &Start, &End);
            env->SetShortField(CameraData, MeasurePoint3StartID, Start);
            env->SetShortField(CameraData, MeasurePoint3EndID, End);
            pCam->getMeasurementLine(3, &Start, &End);
            env->SetShortField(CameraData, MeasurePoint4StartID, Start);
            env->SetShortField(CameraData, MeasurePoint4EndID, End);

            env->SetBooleanField(CameraData, MeasurePointChanged, true);
        }



        uint32_t BreachTimestamp = pCam->lastBarrierBreach(true);
        if(BreachTimestamp != 0){
            jfieldID BarrierBreachId = env->GetFieldID(DataClass, "LastBarrierBreach", "I");
            env->SetIntField(CameraData, BarrierBreachId, (int32_t)BreachTimestamp);
        }
        return Rval;
    }

    JNIEXPORT void JNICALL Java_com_crossforge_tfchampion_TFCBarrierCamera_configMeasurementLines(JNIEnv *env, jobject j, jint CameraID, jshortArray Starts, jshortArray Ends){
        jshort *pStarts = env->GetShortArrayElements(Starts, 0);
        jshort *pEnds = env->GetShortArrayElements(Ends, 0);

        BarrierCamera *pCam = g_Context.getBarrierCamera(CameraID);
        pCam->configMeasurementLines((uint16_t*)pStarts, (uint16_t*)pEnds);

        env->ReleaseShortArrayElements(Starts, pStarts, 0);
        env->ReleaseShortArrayElements(Ends, pEnds, 0);
    }

    JNIEXPORT jshortArray JNICALL Java_com_crossforge_tfchampion_TFCBarrierCamera_retrievePreviewImage(JNIEnv *env, jobject j, jint CameraID, jobject CameraData){
        BarrierCamera *pCam = g_Context.getBarrierCamera(CameraID);
        jshortArray Rval = nullptr;
        // preview image
        uint32_t Width;
        uint32_t Height;
        uint8_t *pImgData;
        bool Grayscale;
        if(pCam->getPreviewImage(&Width, &Height, &pImgData, &Grayscale)){
            jclass DataClass = env->GetObjectClass(CameraData);
            jfieldID WidthID = env->GetFieldID(DataClass, "PrevImgWidth", "I");
            jfieldID HeightID = env->GetFieldID(DataClass, "PrevImgHeight", "I");
            jfieldID GrayscaleID = env->GetFieldID(DataClass, "PrevImgGrayscale", "Z");
            env->SetIntField(CameraData, WidthID, (jint)Width);
            env->SetIntField(CameraData, HeightID, (jint)Height);
            env->SetBooleanField(CameraData, GrayscaleID, (jboolean)Grayscale);

            int16_t *pD = nullptr;
            if(Grayscale){
                pD = new int16_t[Width*Height];
                for(int i=0; i < Width*Height; ++i) pD[i] = pImgData[i];
                Rval = env->NewShortArray(Width*Height);
                env->SetShortArrayRegion(Rval, 0, Width*Height, (const jshort*)pD);
            }else{
                pD = new int16_t[Width*Height*3];
                for(int i=0; i < Width*Height*3; ++i) pD[i] = pImgData[i];
                Rval = env->NewShortArray(Width*Height*3);
                env->SetShortArrayRegion(Rval, 0, Width*Height*3, (const jshort*)pD);
            }


            delete[] pImgData;
            delete[] pD;
            pImgData = nullptr;
            pD = nullptr;
        }

        return Rval;
    }

    void processBluetoothPackageBarrierCamera(CPPContextManager::MessageBuffer *pBuffer, BarrierCamera *pCamera){
        switch(BTMessage::fetchPackageTypeFromStream(pBuffer->pBuffer, pBuffer->Pointer)){
            case BTMessage::BTPCK_PING:{
                BTMessagePing PingMsg;
                PingMsg.fromStream(pBuffer->pBuffer, pBuffer->Pointer);
                pCamera->processMessage(&PingMsg);
            }break;
            case BTMessage::BTPCK_STATUS:{
                BTMessageStatus StatusMsg;
                StatusMsg.fromStream(pBuffer->pBuffer, pBuffer->Pointer);
                pCamera->processMessage(&StatusMsg);
            }break;
            case BTMessage::BTPCK_COMMAND:{
                BTMessageCommand CmdMsg;
                CmdMsg.fromStream(pBuffer->pBuffer, pBuffer->Pointer);
                pCamera->processMessage(&CmdMsg);
            }break;
            case BTMessage::BTPCK_IMAGE:{
                BTMessageImage ImgMsg;
                ImgMsg.fromStream(pBuffer->pBuffer, pBuffer->Pointer);
                pCamera->processMessage(&ImgMsg);
            }break;
            default:{

            }break;
        }//switch[Message type]

        pBuffer->reset();

    }//processBarrierCamera

    JNIEXPORT void JNICALL Java_com_crossforge_tfchampion_TFCBarrierCamera_receiveBTData(JNIEnv *env, jobject j, jint CameraID, jbyteArray Data, jint ByteCount, jint BufferID){
        CPPContextManager::MessageBuffer *pBTBuffer = g_Context.getMessageBuffer(BufferID);
        jbyte *pData = env->GetByteArrayElements(Data, 0);

        for(uint32_t i=0; i < ByteCount; ++i){
            pBTBuffer->write(pData[i]);
            if(pData[i] == BTMessage::cmdCharacter()){
                processBluetoothPackageBarrierCamera(pBTBuffer, g_Context.getBarrierCamera(CameraID));
            }
        }

    }//receiveBTData

    JNIEXPORT jint JNICALL Java_com_crossforge_tfchampion_TFCBarrierCamera_createBTBuffer(JNIEnv *env, jobject j){
        return g_Context.createMessageBuffer();
    }//createBTBuffer

    JNIEXPORT jint JNICALL Java_com_crossforge_tfchampion_TFCBarrierCamera_createBarrierCamera(JNIEnv *env, jobject j){
        return g_Context.createBarrierCamera();
    }

    JNIEXPORT void JNICALL Java_com_crossforge_tfchampion_TFCBarrierCamera_updatePreviewImage(JNIEnv *env, jobject j, jint CameraID){
        BarrierCamera *pCam = g_Context.getBarrierCamera(CameraID);
        if(nullptr != pCam) pCam->updatePreviewImage();
    }

    JNIEXPORT void JNICALL Java_com_crossforge_tfchampion_TFCBarrierCamera_startDetection(JNIEnv *env, jobject j, jint CameraID){
        BarrierCamera *pCam = g_Context.getBarrierCamera(CameraID);
        if(nullptr != pCam) pCam->startDetection();
    }

    JNIEXPORT void JNICALL Java_com_crossforge_tfchampion_TFCBarrierCamera_stopDetection(JNIEnv *env, jobject j, jint CameraID){
        BarrierCamera *pCam = g_Context.getBarrierCamera(CameraID);
        if(nullptr != pCam) pCam->stopDetection();
    }

    JNIEXPORT void JNICALL Java_com_crossforge_tfchampion_TFCBarrierCamera_changeSensitivity(JNIEnv *env, jobject j, jint CameraID, jfloat Value){
        BarrierCamera *pCam = g_Context.getBarrierCamera(CameraID);
        if(nullptr != pCam) pCam->sensitivity(Value);
    }

}//extern "C"