package com.crossforge.tfchampion;

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.os.Environment;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Set;

public class TFCUtilities {
    public static final int EXTERNAL_STORAGE_REQUEST_CODE = 301;
    public static final int BLUETOOTH_GRANT_REQUEST_CODE = 311;
    public static final int BLUETOOTH_ACTIVATION_REQUEST_CODE = 312;


    static public boolean storeImage(Activity ParentActivity, Bitmap Bmp, String Filename){
        if(ActivityCompat.checkSelfPermission(ParentActivity, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(ParentActivity, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, EXTERNAL_STORAGE_REQUEST_CODE);
            return false;
        }

        boolean Rval = true;

        String path = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES).toString();
        File f = new File(path + "/" + Filename);

        try {
            f.createNewFile();
            FileOutputStream ostream = new FileOutputStream(f);
            Bmp.compress(Bitmap.CompressFormat.JPEG, 90, ostream);
            ostream.close();
            Log.d("TFCUtilities", "Stored iamge to " + path + "/" + Filename);
        } catch (IOException e) {
            e.printStackTrace();
            Rval = false;
        }

        return Rval;
    }//storeImage



    static public BluetoothAdapter startBluetooth(@NonNull Activity RequestingActivity){
        if(PackageManager.PERMISSION_GRANTED != ActivityCompat.checkSelfPermission(RequestingActivity, Manifest.permission.BLUETOOTH)){
            ActivityCompat.requestPermissions(RequestingActivity, new String[]{Manifest.permission.BLUETOOTH}, BLUETOOTH_GRANT_REQUEST_CODE);
            return null;
        }

        BluetoothAdapter Rval = BluetoothAdapter.getDefaultAdapter();
        if(null == Rval){
            Log.e("TFCUtilities", "Bluetooth adapter not available");
        }
        else if(!Rval.isEnabled()){
            Intent EnableBTIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            RequestingActivity.startActivityForResult(EnableBTIntent, BLUETOOTH_ACTIVATION_REQUEST_CODE);
            Log.i("TFCUtilities", "Requesting to start bluetooth");
        }

        return Rval;
    }//startBluetooth


}//TFCUtilities
