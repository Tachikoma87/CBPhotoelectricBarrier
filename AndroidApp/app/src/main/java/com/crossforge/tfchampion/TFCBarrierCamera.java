package com.crossforge.tfchampion;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.media.Image;
import android.os.SystemClock;
import android.util.Log;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import java.util.Set;

public class TFCBarrierCamera extends BluetoothComThread.BluetoothComThreadListener{

    public static abstract class BarrierCameraListener{
        public void onConnectionEstablished(int CameraID){

        }

        public void onBarrierBreach(int CameraID, int Timestamp){

        }
    }//BarrierCameraListener

    private Activity mParentActivity = null;

    private int mCameraID = 0;
    private int mInternalCameraID = 0;
    private BluetoothComThread mBTThread = new BluetoothComThread();
    private int mBTBufferID = -1;
    private BarrierCameraData mCameraData = new BarrierCameraData();
    private BarrierCameraListener mListener = null;

    private String mCameraName = "";

    public TFCBarrierCamera(){

    }//Constructor

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
    }

    public void init(@NonNull Activity ParentActivity, int CameraID, @NonNull BluetoothDevice BTDev, @NonNull BarrierCameraListener Listener){
        mParentActivity = ParentActivity;
        mInternalCameraID = createBarrierCamera();
        mCameraID = CameraID;
        mBTBufferID = createBTBuffer();
        mListener = Listener;

        mCameraData.Connected = false;
        mCameraName = BTDev.getName();

        mBTThread.init(BTDev, this);
        mBTThread.start();
    }//initialize

    public void changeSensitivity(float Value){
        changeSensitivity(mCameraID, Value);
    }

    // Handle bluetooth communication
    @Override
    public void onConnectionEstablished() {
        super.onConnectionEstablished();
        Log.i("TFCBarrierCamera", "Connection to Camera established!");
        mListener.onConnectionEstablished(mCameraID);
    }

    @Override
    public void onDataReceived(byte[] Data, int ByteCount) {
        super.onDataReceived(Data, ByteCount);
        receiveBTData(mInternalCameraID, Data, ByteCount, mBTBufferID);
    }

    @Override
    public void onUpdate() {
        super.onUpdate();
        byte[] Msg = updateBarrierCamera(mInternalCameraID, mCameraData, SystemClock.uptimeMillis());
        if(Msg != null) mBTThread.sendData(Msg);

        short[] ImgData = retrievePreviewImage(mInternalCameraID, mCameraData);
        if(ImgData != null) mCameraData.PrevImgData = ImgData;

        if(mCameraData.LastBarrierBreach != 0){
            mListener.onBarrierBreach(mCameraID, mCameraData.LastBarrierBreach);
            mCameraData.LastBarrierBreach = 0;
        }
    }

    public void startDetection(){
        startDetection(mInternalCameraID);
    }
    public void stopDetection(){stopDetection(mInternalCameraID);}

    public void updatePreviewImage(){
        updatePreviewImage(mInternalCameraID);
    }//

    public BarrierCameraData cameraData(){
        return mCameraData;
    }

    public String cameraName(){
        return mCameraName;
    }

    public void configMeasurementLines(short Starts[], short Ends[]){
        configMeasurementLines(mCameraID, Starts, Ends);
    }

    private native int createBTBuffer();
    private native int createBarrierCamera();

    private native byte[] updateBarrierCamera(int CameraID, BarrierCameraData CameraData, long Millis);
    private native void updatePreviewImage(int CameraID);
    private native short[] retrievePreviewImage(int CameraID, BarrierCameraData CameraData);
    private native void receiveBTData(int CameraID, byte[] Data, int ByteCount, int BTBufferID);

    private native void startDetection(int CameraID);
    private native void stopDetection(int CameraID);

    private native void changeSensitivity(int CameraID, float Sensitivity);
    private native void configMeasurementLines(int CameraID, short Starts[], short Ends[]);



    public class BarrierCameraData{
        boolean Connected;
        boolean Detecting;

        float Sensitivity;
        int BatteryCharge; // [0, 100]

        int Latency;
        int PrevImgWidth;
        int PrevImgHeight;
        boolean PrevImgGrayscale;
        short[] PrevImgData;

        int LastBarrierBreach;

        short MeasurePoint1Start = 0;
        short MeasurePoint1End = 0;
        short MeasurePoint2Start = 0;
        short MeasurePoint2End = 0;
        short MeasurePoint3Start = 0;
        short MeasurePoint3End = 0;
        short MeasurePoint4Start = 0;
        short MeasurePoint4End = 0;
        boolean MeasurePointsChanged = false;

        Bitmap buildPreviewBitmap(boolean Clear){
            if(null == PrevImgData) return null;
            // bitmap must be flipped horizontally and rotated -90 degree
            Bitmap Rval = Bitmap.createBitmap(PrevImgHeight, PrevImgWidth, Bitmap.Config.ARGB_8888);
            for(int y = 1; y <= PrevImgHeight; ++y){
                for(int x = 0; x < PrevImgWidth; ++x){
                    if(PrevImgGrayscale){
                        int Index = (PrevImgHeight-y) * PrevImgWidth + x;
                        int c = PrevImgData[Index];
                        Rval.setPixel(y-1, x, Color.argb(255, c, c, c));
                    }else {
                        int Index = (PrevImgHeight-y)*3 * PrevImgWidth + x*3;
                        int r = PrevImgData[Index + 0];
                        int g = PrevImgData[Index + 1];
                        int b = PrevImgData[Index + 2];
                        Rval.setPixel(y-1, x, Color.argb(255, r, g, b));
                    }
                }
            }

            if(Clear){
                PrevImgData = null;
                PrevImgWidth = 0;
                PrevImgHeight = 0;
            }

            return Rval;
        }//buildPreviewBitmap

    }//BarrierCameraData

}//TFCBarrierCamera
