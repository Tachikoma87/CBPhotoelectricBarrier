package com.crossforge.tfchampion;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothSocket;
import android.util.Log;

import androidx.annotation.NonNull;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Calendar;
import java.util.UUID;

public class BluetoothComThread extends Thread{
    public static abstract class BluetoothComThreadListener{
        public void onConnectionEstablished(){

        }

        public void onUpdate(){

        }

        public void onDataReceived(byte[] Data, int ByteCount){

        }
    }//Callback


    private final UUID mUUID = UUID.fromString("00001101-0000-1000-8000-00805f9b34fb");
    private BluetoothDevice mBTDevice = null;
    private BluetoothSocket mBTSocket = null;
    private InputStream mInputStream = null;
    private OutputStream mOutputStream = null;
    private boolean mClosingRequest = false;
    private boolean mIsConnected = false;

    private byte[] mSendBuffer = null;

    private BluetoothComThreadListener mCallback = null;

    public void sendData(byte[] Data){
        mSendBuffer = Data;
    }

    public void closeConnection(){
        mClosingRequest = true;
    }

    void init(@NonNull BluetoothDevice BTDevice, @NonNull BluetoothComThreadListener Callback){
        mBTDevice = BTDevice;
        mIsConnected = false;
        mClosingRequest = false;
        mCallback = Callback;
    }//initialize

    @Override
    public void run() {

        Log.i("BluetoothComThread", "Starting BT connection");
        // create socket
        try{
            mBTSocket = mBTDevice.createRfcommSocketToServiceRecord(mUUID);
        }catch(IOException e){
            e.printStackTrace();
            mBTSocket = null;
        }

        try{
            mBTSocket.connect();
        }catch(IOException e){
            try{
                mBTSocket.close();
            }catch(IOException e2){
                e2.printStackTrace();
            }
            return;
        }

        Log.i("BluetoothComThread", "Socket opened");

        try{
            mInputStream = mBTSocket.getInputStream();
            mOutputStream = mBTSocket.getOutputStream();
        }catch(IOException StreamExcept){
            StreamExcept.printStackTrace();
        }

        Log.i("RelayCommunication", "Connection established");
        mIsConnected = true;
        mCallback.onConnectionEstablished();

        while(!mClosingRequest){
            if(!mBTSocket.isConnected()){
                mClosingRequest = true;
                mIsConnected = false;
                continue;
            }

            // update
            mCallback.onUpdate();

            try{
                // do we need to send bluetooth data?
                if(null != mSendBuffer){
                    mOutputStream.write(mSendBuffer, 0, mSendBuffer.length);
                    mSendBuffer = null;
                }
                // did we receive bluetooth data?
                int AvailableBytes = mInputStream.available();
                if(AvailableBytes > 0){
                    byte[] ReceiveBuffer = new byte[AvailableBytes];
                    int ByteCount = mInputStream.read(ReceiveBuffer, 0, AvailableBytes);
                    mCallback.onDataReceived(ReceiveBuffer, ByteCount);
                }
            }catch(IOException IOExcept){
                IOExcept.printStackTrace();
                mClosingRequest = true;
            }

            // sleep for a while so we don't burn through too much CPU time
            try{
                Thread.sleep(1);
            }catch(InterruptedException InterruptExcept){
                InterruptExcept.printStackTrace();
            }
        }//while[!closing]

        try{
            mBTSocket.close();
            mBTSocket = null;
            Log.i("FlapCommunication", "Bluettoth socket closed");
        }catch(IOException IOExcept){
            IOExcept.printStackTrace();
        }

        mIsConnected = false;
        mInputStream = null;
        mOutputStream = null;
        mSendBuffer = null;
    }//run
}//BTComThread
