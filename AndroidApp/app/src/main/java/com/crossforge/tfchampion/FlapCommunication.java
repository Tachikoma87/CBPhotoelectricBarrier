package com.crossforge.tfchampion;

import android.Manifest;
import android.app.Activity;
import android.bluetooth.*;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.SystemClock;
import android.util.Log;
import android.view.PixelCopy;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Set;
import java.util.UUID;

public class FlapCommunication {

    public static abstract class StartingFlapListener{

        public void onStartSignalReceived(long Timestamp){

        }
    }//

    public class FlapData{
        public int mLatency; /// current latency to the starting flap (-1 if disconnected)
        public int mRSSI; ///< Receiver signal strength indicator
        public int mFlapBatteryCharge; /// Battery charge in percent
        public int mRelayBatteryCharge; /// Relay Battery charge in percent
        public boolean mIsConnected = false;
        public long mLastRaceStart = 0;
    }

    Activity mParentActivity = null;
    BluetoothComThread mBTThread = null;
    BluetoothComThread.BluetoothComThreadListener mBTThreadListener = null;

    StartingFlapListener mListener = null;

    private FlapData mFlapData = new FlapData();

    FlapCommunication(){

    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
    }

    public void setListener(StartingFlapListener Listener){
        mListener = Listener;
    }

    public void init(@NonNull Activity ParentActivity, BluetoothDevice BTDevice){
        mParentActivity = ParentActivity;
        mBTThread = new BluetoothComThread();
        createBTThreadListener();
        mBTThread.init(BTDevice, mBTThreadListener);
        mBTThread.start();
        initStartingFlap();
    }//initialize

    public boolean isConnected(){
        return mFlapData.mIsConnected;
    }

    public FlapData getFlapData(){
        return mFlapData;
    }

    private native void initStartingFlap(); // initializes the starting flap
    private native byte[] updateFlap(FlapData DataStorage, long Timestamp); // perform regular update
    private native void receiveBluetoothData(byte[] Buffer);
    public native boolean issueStartRequest();

    private long mLastDataUpdate = 0;


    private void createBTThreadListener(){
        mBTThreadListener = new BluetoothComThread.BluetoothComThreadListener() {
            @Override
            public void onConnectionEstablished() {
                super.onConnectionEstablished();
                mFlapData.mIsConnected = true;
            }

            @Override
            public void onUpdate() {
                super.onUpdate();

                byte[] SendBuffer = updateFlap(mFlapData, SystemClock.uptimeMillis());
                if(null != SendBuffer) mBTThread.sendData(SendBuffer);

                if(mFlapData.mLastRaceStart != 0 && mListener != null){
                    mListener.onStartSignalReceived(mFlapData.mLastRaceStart);
                    mFlapData.mLastRaceStart = 0;
                }

                if(SystemClock.uptimeMillis() - mLastDataUpdate > 2000){
                    mLastDataUpdate = SystemClock.uptimeMillis();
                }
            }

            @Override
            public void onDataReceived(byte[] Data, int ByteCount) {
                super.onDataReceived(Data, ByteCount);
                receiveBluetoothData(Data);
            }
        };//BluetoothComThreadListener
    }//createBTThreadListener

}//RelayCommunication
