#ifdef ESP32

#include "OpticalBarrier.h"
#include "soc/rtc_cntl_reg.h"

#ifdef DEBUG
#define ON_DEBUG(Statement) Statement
#else
#define ON_DEBUG(Statement)
#endif

void bluetoothCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param ){
    //ON_DEBUG(Serial.print("Bluetooth event!\n"));
}


OpticalBarrier::OpticalBarrier(void){

}//Constructor

OpticalBarrier::~OpticalBarrier(void){
    clear();
}//Destructor

void OpticalBarrier::init(uint8_t CameraID){
    ON_DEBUG(Serial.print("Starting to initialize Optical Barrier device...\n"));

    m_LastBarrierBreachSent = 0;
    m_LastBarrierBreach = 0;

    pinMode(m_RedLEDPin, OUTPUT);
    pinMode(m_GreenLEDPin, OUTPUT);
    pinMode(m_BlueLEDPin, OUTPUT);
    digitalWrite(m_RedLEDPin, LOW);
    digitalWrite(m_GreenLEDPin, LOW);
    digitalWrite(m_BlueLEDPin, LOW);

    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector

    // unfortunately we can read battery only once on startup, since WiFi/Bluetooth messes us analog readings
    ON_DEBUG(Serial.print("Starting battery indicator..."));
    analogReadResolution(12);
    m_Battery.begin(m_BatteryReadPin, 2, 47000, 22000, ArduForge::BatteryIndicator::TYPE_LIPO, 3.30f*1.034);
    ON_DEBUG(Serial.print("done\n"));
    m_BatteryCharge = m_Battery.charge();


    ON_DEBUG(Serial.printf("Battery charge: %d%% (%.2fV)\n", m_BatteryCharge, m_Battery.read()));

    m_Battery.end();

    ON_DEBUG(Serial.print("Firing up Bluetooth..."));

    char BTName[64];
    sprintf(BTName, "T&F Lightbarrier %d", CameraID);
    if(m_BTSerial.begin(BTName)){
        ON_DEBUG(Serial.printf("done. Bluetooth name now %s\n", BTName));
        m_BTSerial.register_callback(bluetoothCallback);
    }else{
        ON_DEBUG(Serial.print("failed\n"));
    }

     ON_DEBUG(Serial.print("Starting Optical Light Barrier..."));
    m_Barrier.begin(barrierCallback, this, 25.0f, 0, false, false, false);
    m_Barrier.setCalibrationParams(false, 2000);

    ON_DEBUG(Serial.print("done\n"));

    m_BTBufferPointer = 0;
    memset(m_BTMsgBuffer, 0, sizeof(m_BTMsgBuffer));

    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1); // enable brownout detector

}//initialize

void OpticalBarrier::clear(void){
    
}//clear



void OpticalBarrier::update(void){

    if(!m_BTSerial.hasClient()){
        ledColor(0, 0, 255);
    }
    while(m_BTSerial.available()){
        m_BTMsgBuffer[m_BTBufferPointer] = m_BTSerial.read();
        m_BTBufferPointer++;

        if(m_BTMsgBuffer[m_BTBufferPointer-1] == BTMessage::cmdCharacter()){
            //package arrived
            processBluetoothPackage();
            //reset message pointer
            m_BTBufferPointer = 0;
        }

    }//while[read BT stream]

    if(m_LastBarrierBreach != 0 && (millis() - m_LastBarrierBreachSent) > 250){
        BTMessageCommand CmdMsg;
        CmdMsg.request(false);
        CmdMsg.cmdCode(BTMessageCommand::CMD_BARRIERCAM_BREACH);
        uint32_t Timestamp = m_LastBarrierBreach;
        CmdMsg.setPayload((uint8_t*)&Timestamp, sizeof(uint32_t));
        uint32_t Payload = CmdMsg.toStream(m_BTMsgBufferOut, sizeof(m_BTMsgBufferOut));
        m_BTSerial.write(m_BTMsgBufferOut, Payload);
        m_LastBarrierBreachSent = millis();
    }

    delay(1);
    
}//update

void OpticalBarrier::processBluetoothPackage(void){
    switch(BTMessage::fetchPackageTypeFromStream(m_BTMsgBuffer, m_BTBufferPointer)){
        case BTMessage::BTPCK_PING:{
            BTMessagePing PingMsg;
            PingMsg.fromStream(&m_BTMsgBuffer[0], m_BTBufferPointer);
            PingMsg.request(false);

            ON_DEBUG(Serial.printf("Sending back ping! %d\n", PingMsg.timestamp()));

            // return to sender
            uint32_t MsgSize = PingMsg.toStream(m_BTMsgBuffer, sizeof(m_BTMsgBuffer));
            m_BTSerial.write(m_BTMsgBuffer, MsgSize);

        }break;
        case BTMessage::BTPCK_IMAGE:{
            BTMessageImage ImgMsgIn;
            ImgMsgIn.fromStream(m_BTMsgBuffer, m_BTBufferPointer);
            if(ImgMsgIn.request()){
                uint32_t Start = millis();
                ON_DEBUG(Serial.print("Sending current image...\n"));

                uint32_t ImgSize = m_Barrier.imageSize();
                uint8_t *pImgData = (uint8_t*)ps_malloc(ImgSize);
                uint32_t Width = 0;
                uint32_t Height = 0;
                m_Barrier.captureImage(&Width, &Height, pImgData, true);

                // we can not send command characters or it will break the pipeline
                for(int i=0; i < ImgSize; ++i){
                    if(pImgData[i] == BTMessage::cmdCharacter()) pImgData[i] = BTMessage::cmdCharacter()-1;
                }
                       
                ON_DEBUG(Serial.print("Image retrieved \n"));
                BTMessageImage ImgMsg;
                
                ImgMsg.init(Width, Height, pImgData, (ImgSize == Width*Height) ? BTMessageImage::FMT_GRAYSCALE : BTMessageImage::FMT_RGB888, ImgSize, false);

                ON_DEBUG(Serial.print("Image Size: "); Serial.print(ImgSize); Serial.print(" bytes\n"));
                ON_DEBUG(Serial.print("Image message size: "); Serial.print(ImgMsg.size()); Serial.print(" bytes\n"));

                uint8_t *pBuffer = (uint8_t*)ps_malloc((size_t)ImgMsg.size());
                uint32_t BytesToSend = ImgMsg.toStream(pBuffer, ImgMsg.size());

                ON_DEBUG(Serial.print("Sending image data ...\n"));
                uint32_t BytesSend = 0;
                while(BytesSend < BytesToSend){
                    uint16_t PckSize = min(4096U, BytesToSend - BytesSend);
                    m_BTSerial.write(&pBuffer[BytesSend], PckSize);
                    BytesSend += PckSize;
                    // we need a small delay or we run out of memory while sending (TX allocation in Bluetooth sending will fail)
                    delay(50);
                }
                    
                free(pBuffer);
                free(pImgData);
                pBuffer = nullptr;
                ImgMsg.clear();

                ON_DEBUG(Serial.printf("Image was sent in %d ms\n", (uint32_t)(millis() - Start)));
            }
        }break;
        case BTMessage::BTPCK_COMMAND:{
            BTMessageCommand CmdMsg;
            CmdMsg.fromStream(m_BTMsgBuffer, m_BTBufferPointer);
            switch(CmdMsg.cmdCode()){
                case BTMessageCommand::CMD_BARRIERCAM_CALIBRATE:{
                    m_Barrier.calibrate();
                    ledColor(255, 255, 255);
                }break;
                case BTMessageCommand::CMD_BARRIERCAM_START_DETECTION:{
                    m_LastBarrierBreach = 0;
                    m_LastBarrierBreachSent = 0;
                    m_Barrier.startDetection();        
                }break;
                case BTMessageCommand::CMD_BARRIERCAM_STOP_DETECTION:{
                    m_LastBarrierBreach = 0;
                    m_LastBarrierBreachSent = 0;
                    m_Barrier.stopDetection();
                }break;
                case BTMessageCommand::CMD_CHANGE_SENSITIVITY:{
                    float Sensitivity = 0.0f;
                    CmdMsg.getPayload((uint8_t*)&Sensitivity, sizeof(float));
                    m_Barrier.sensitivity(Sensitivity);

                    CmdMsg.request(false);
                    uint32_t Payload = CmdMsg.toStream(m_BTMsgBuffer, sizeof(m_BTMsgBuffer));
                    m_BTSerial.write(m_BTMsgBuffer, Payload);

                    ON_DEBUG(Serial.printf("Sensitivity changed to %.2f\n", Sensitivity));

                }break;
                case BTMessageCommand::CMD_CONFIG_MEASUREMENT_POINTS:{
                    // retrieve measurement points config (4*2 values)
                    uint16_t Data[8];
                    
                    if(CmdMsg.request()){
                        CmdMsg.getPayload((uint8_t*)Data, sizeof(uint16_t)*8);
                        m_Barrier.clearMeasureLines();
                        for(int i=0; i< 4; ++i){
                            if(Data[i*2+0] != 0 || Data[i*2+1] != 0) 
                                m_Barrier.addMeasureLine(Data[i*2+0], Data[i*2+1]);
                        }//for[four lines]
                    }

                    CmdMsg.request(false);
                    memset(Data, 0, sizeof(uint16_t)*8);
                    for(uint8_t i=0; i < m_Barrier.measureLineCount(); ++i){
                        uint16_t Start = 0;
                        uint16_t End = 0;
                        m_Barrier.getMeasureLine(i, &Start, &End);
                        ON_DEBUG(Serial.print("Measure Line: "); Serial.print(Start); Serial.print(":"); Serial.print(End); Serial.print("\n"));
                        Data[i*2+0] = Start;
                        Data[i*2+1] = End;
                    }
                    CmdMsg.setPayload((uint8_t*)Data, sizeof(uint16_t)*8);
                    uint32_t Payload = CmdMsg.toStream(m_BTMsgBuffer, sizeof(m_BTMsgBuffer));
                    m_BTSerial.write(m_BTMsgBuffer, Payload);

                    ON_DEBUG(Serial.print("Sent measurement points config.\n"));
                    
                }break;
                case BTMessageCommand::CMD_BARRIERCAM_BREACH:{
                    if(!CmdMsg.request()){
                        // got confirmation that host received breach information
                        m_LastBarrierBreach = 0;
                        m_LastBarrierBreachSent = 0;
                    }
                }break;
                default: break;
            }
        }break;
        case BTMessage::BTPCK_STATUS:{
            BTMessageStatus StatusMsg;
            StatusMsg.fromStream(m_BTMsgBuffer, m_BTBufferPointer);
            if(StatusMsg.request()){
                StatusMsg.request(false);
                StatusMsg.batteryChargeDevice(m_BatteryCharge);
                StatusMsg.deviceClock(millis());

                uint32_t Payload = StatusMsg.toStream(m_BTMsgBufferOut, sizeof(m_BTMsgBufferOut));
                m_BTSerial.write(m_BTMsgBufferOut, Payload);
            }
        }break;
        default: break;
    }//switch[Message]

}//processBluetoothPackage

void OpticalBarrier::ledColor(uint8_t Red, uint8_t Green, uint8_t Blue){
    digitalWrite(m_RedLEDPin, Red);
    digitalWrite(m_GreenLEDPin, Green);
    digitalWrite(m_BlueLEDPin, Blue);
}

void OpticalBarrier::barrierCallback(ArduForge::OpticalLightBarrierESP32::BarrierMsg Msg, void *pUserParam){
    OpticalBarrier *pParent = (OpticalBarrier*)pUserParam;

    switch(Msg){
        case ArduForge::OpticalLightBarrierESP32::BMSG_BARRIER_TRIGGERED:{
            ON_DEBUG(Serial.print("Barrier triggered. Sending notification.\n"));
            // sending Bluetooth Signal

            for(uint8_t i = 0; i < pParent->m_Barrier.measureLineCount(); ++i){
                // only send notification if all measurement lines were triggered
                if(!pParent->m_Barrier.isLineTriggered(i)) return;
            }          
            pParent->m_LastBarrierBreach = millis();
            pParent->m_LastBarrierBreachSent = 0;

            pParent->m_Barrier.stopDetection();
        }break;
        case ArduForge::OpticalLightBarrierESP32::BMSG_CALIBRATION_FINISHED:{
            ON_DEBUG(Serial.print("Calibration done\n"););

            BTMessageCommand CmdMsg;
            if(pParent->m_BTSerial.hasClient()){
                ON_DEBUG(Serial.print("Sending calibration finished notification!\n"));
                CmdMsg.request(false);
                CmdMsg.cmdCode(BTMessageCommand::CMD_BARRIERCAM_CALIBRATE);
                float Noise = pParent->m_Barrier.noise(0);
                CmdMsg.setPayload((uint8_t*)&Noise, sizeof(float));
                uint32_t Payload = CmdMsg.toStream(pParent->m_BTMsgBufferOut, sizeof(pParent->m_BTMsgBufferOut));
                pParent->m_BTSerial.write(pParent->m_BTMsgBufferOut, Payload);
            }

        }break;
        case ArduForge::OpticalLightBarrierESP32::BMSG_INITIALIZATION_FINISHED:{
            pParent->ledColor(0, 0, 255);
        }break;
        case ArduForge::OpticalLightBarrierESP32::BMSG_INITIALIZATION_FAILED:{
            pParent->ledColor(255, 255, 0);
        }break;
        case ArduForge::OpticalLightBarrierESP32::BMSG_DETECTION_STARTED:{
            pParent->ledColor(0, 255, 0);
            BTMessageCommand CmdMsg;
            CmdMsg.request(false);
            CmdMsg.cmdCode(BTMessageCommand::CMD_BARRIERCAM_START_DETECTION);
            uint32_t Payload = CmdMsg.toStream(pParent->m_BTMsgBufferOut, sizeof(pParent->m_BTMsgBufferOut));
            pParent->m_BTSerial.write(pParent->m_BTMsgBufferOut, Payload);
            ON_DEBUG(Serial.print("Notified about start of detection!\n"));

        }break;
        case ArduForge::OpticalLightBarrierESP32::BMSG_DETECTION_STOPPED:{
            pParent->ledColor(255, 0, 0);
            BTMessageCommand CmdMsg;
            CmdMsg.request(false);
            CmdMsg.cmdCode(BTMessageCommand::CMD_BARRIERCAM_STOP_DETECTION);
            uint32_t Payload = CmdMsg.toStream(pParent->m_BTMsgBufferOut, sizeof(pParent->m_BTMsgBufferOut));
            pParent->m_BTSerial.write(pParent->m_BTMsgBufferOut, Payload);
        }break;
        case ArduForge::OpticalLightBarrierESP32::BMSG_IDLING:{
            delay(1);
        }break;
        default:{
            ON_DEBUG(Serial.print("Received unhandled barrier message\n"));
        }break;
    }//switch[Msg]
}//barrierCallback


#endif //ESP32