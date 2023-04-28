package com.crossforge.tfchampion;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;

import com.google.android.material.tabs.TabLayout;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.os.SystemClock;
import android.util.Log;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;
import android.widget.LinearLayout.LayoutParams;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.Set;
import java.util.Timer;
import java.util.TimerTask;

public class FlyingRunMain extends AppCompatActivity{
    private class Result{
        public int RaceID = -1;
        public TableRow Row = null;
        public int RaceDuration = 0;
        public float TrackLength = 0.0f;
        public float Speed = 0.0f;
    }

    FrameLayout mContentView = null; ///< Contains the main tab content

    TFCBarrierCamera mBarrierCams[] = {null, null}; ///< The two barrier cameras
    Timer mUserInterfaceUpdateTimer = new Timer();
    Activity mActivity = this;

    boolean mOngoingRace = false; ///< Is race currently running

    ArrayList<Result> mResults = new ArrayList<>(); ///< List of results

    private LinearLayout buildDivider(boolean Horizontal){
        LinearLayout Rval = new LinearLayout(getApplicationContext());
        if(Horizontal){
            Rval.setLayoutParams(new TableRow.LayoutParams(TableRow.LayoutParams.MATCH_PARENT, (int)TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 2, getResources().getDisplayMetrics())));
            Rval.setOrientation(LinearLayout.HORIZONTAL);
        }else{
            Rval.setLayoutParams(new TableRow.LayoutParams((int)TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 2, getResources().getDisplayMetrics()), TableRow.LayoutParams.MATCH_PARENT));
            Rval.setOrientation(LinearLayout.VERTICAL);
            int MarginVal = (int)TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 10, getResources().getDisplayMetrics());
            ((ViewGroup.MarginLayoutParams)Rval.getLayoutParams()).setMargins(MarginVal,0,MarginVal,0);
        }

        Rval.setBackgroundColor(Color.BLUE);

        return Rval;
    }

    private void buildResultTable(TableLayout View){
        View.removeAllViews();

        // header
        TableRow Tr = new TableRow(getApplicationContext());
        Tr.setLayoutParams(new TableLayout.LayoutParams(TableRow.LayoutParams.MATCH_PARENT, TableRow.LayoutParams.WRAP_CONTENT));
        Tr.addView(buildDivider(false));
        Tr.addView(buildDivider(true));
        Tr.addView(buildDivider(false));
        Tr.addView(buildDivider(true));
        Tr.addView(buildDivider(false));
        Tr.addView(buildDivider(true));
        Tr.addView(buildDivider(false));
        View.addView(Tr);

        Tr = new TableRow(getApplicationContext());
        TextView tvRunIDHeader = new TextView(getApplicationContext());
        TextView tvTimeHeader = new TextView(getApplicationContext());
        TextView tvSpeed = new TextView(getApplicationContext());

        tvRunIDHeader.setText("Nr.");
        tvRunIDHeader.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24);
        tvRunIDHeader.setGravity(android.view.View.TEXT_ALIGNMENT_CENTER);
        tvRunIDHeader.setTextColor(Color.BLACK);
        tvTimeHeader.setText("Zeit");
        tvTimeHeader.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24);
        tvTimeHeader.setTextColor(Color.BLACK);
        tvSpeed.setText("Geschwindigkeit");
        tvSpeed.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24);
        tvSpeed.setTextColor(Color.BLACK);

        Tr.addView(buildDivider(false));
        Tr.addView(tvRunIDHeader);
        Tr.addView(buildDivider(false));
        Tr.addView(tvTimeHeader);
        Tr.addView(buildDivider(false));
        Tr.addView(tvSpeed);
        Tr.addView(buildDivider(false));

        View.addView(Tr);

        Tr = new TableRow(getApplicationContext());
        Tr.addView(buildDivider(false));
        Tr.addView(buildDivider(true));
        Tr.addView(buildDivider(false));
        Tr.addView(buildDivider(true));
        Tr.addView(buildDivider(false));
        Tr.addView(buildDivider(true));
        Tr.addView(buildDivider(false));

        View.addView(Tr);

        int c = 0;
        for(int i=mResults.size()-1; i >= 0; --i){

            TableLayout T = (TableLayout)mResults.get(i).Row.getParent();
            if(null != T && T != View) T.removeView(mResults.get(i).Row);

            View.addView(mResults.get(i).Row);
            c++;
            if(c%5 == 0 && i != 0) {
                Tr = new TableRow(getApplicationContext());
                Tr.addView(buildDivider(false));
                Tr.addView(buildDivider(true));
                Tr.addView(buildDivider(false));
                Tr.addView(buildDivider(true));
                Tr.addView(buildDivider(false));
                Tr.addView(buildDivider(true));
                Tr.addView(buildDivider(false));
                View.addView(Tr);
            }
        }
        Tr = new TableRow(getApplicationContext());
        Tr.addView(buildDivider(false));
        Tr.addView(buildDivider(true));
        Tr.addView(buildDivider(false));
        Tr.addView(buildDivider(true));
        Tr.addView(buildDivider(false));
        Tr.addView(buildDivider(true));
        Tr.addView(buildDivider(false));
        View.addView(Tr);


    }

    long mBarrierBreach1 = 0;
    long mBarrierBreach2 = 0;

    TimerTask mRaceClockTimerTask = null;
    Runnable mRaceClockTask = new Runnable() {
        @Override
        public void run() {
            Button BtnCmd = (Button) findViewById(R.id.btn_flying_run_cmd);

            long RunDuration = SystemClock.uptimeMillis() - mBarrierBreach1;
            int Seconds = (int) RunDuration / 1000;
            int Residual = (int) RunDuration - (Seconds * 1000);
            String ClockText = Integer.toString(Seconds) + "," + Integer.toString(Residual / 10) + " s";
            BtnCmd.setText(ClockText);
        }
    };//Runnable

    public void setCameraUIData(int CameraID){
        int btnCameraConfigID = (CameraID == 0) ? R.id.btn_flying_run_config_lb1_cmd: R.id.btn_flying_run_config_lb2_cmd;
        int tvCameraNameID = (CameraID == 0) ? R.id.tv_flying_run_cam1_name : R.id.tv_flying_run_cam2_name;
        int tvCameraNameConfigID = (CameraID == 0) ? R.id.tv_flying_run_config_lb1_name : R.id.tv_flying_run_config_lb2_name;
        int editSensitivityID = (CameraID == 0) ? R.id.edit_flying_run_config_lb1_sensitivity : R.id.edit_flying_run_config_lb2_sensitivity;

        Button btnCameraConfigCmd = (Button)findViewById(btnCameraConfigID);
        EditText editSensitivity = (EditText)findViewById(editSensitivityID);
        TextView tvCameraName = (TextView)findViewById(tvCameraNameID);
        TextView tvCameraConfigName = (TextView)findViewById(tvCameraNameConfigID);
        float Sensitivity = mBarrierCams[CameraID].cameraData().Sensitivity;

        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if(null != btnCameraConfigCmd){
                    btnCameraConfigCmd.setText("Update");
                    btnCameraConfigCmd.setEnabled(true);
                }
                if(null != editSensitivity){
                    editSensitivity.setText(Float.toString(Sensitivity));
                }
                if(null != tvCameraName){
                    tvCameraName.setText(mBarrierCams[CameraID].cameraName());
                }
                if(null != tvCameraConfigName){
                    tvCameraConfigName.setText(mBarrierCams[CameraID].cameraName());
                }

                if(null != mBarrierCams[0] && null != mBarrierCams[1]){
                    Button BtnCmd = (Button)findViewById(R.id.btn_flying_run_cmd);
                    if(BtnCmd != null) BtnCmd.setEnabled(true);
                }
            }//run[change UI]
        });
    }

    TFCBarrierCamera.BarrierCameraListener mCameraListener = new TFCBarrierCamera.BarrierCameraListener() {
        @Override
        public void onConnectionEstablished(int CameraID) {
            super.onConnectionEstablished(CameraID);
            mBarrierCams[CameraID].changeSensitivity(20.0f);
            setCameraUIData(CameraID);

        }//onConnectionEstablished

        @Override
        public void onBarrierBreach(int CameraID, int Timestamp) {
            super.onBarrierBreach(CameraID, Timestamp);

            if(CameraID == 0){
                mBarrierBreach1 = Timestamp;
                mRaceClockTimerTask = new TimerTask() {
                    @Override
                    synchronized public void run() {
                        runOnUiThread(mRaceClockTask);
                    }
                };//TimerTask
                mUserInterfaceUpdateTimer.scheduleAtFixedRate(mRaceClockTimerTask, 0, 10);
            }else if(mBarrierBreach1 == 0){
                mBarrierCams[1].startDetection();
            }else{
                mBarrierBreach2 = Timestamp;
                if(null != mRaceClockTimerTask){
                    mRaceClockTimerTask.cancel();
                    mRaceClockTimerTask = null;
                }

                mOngoingRace = false;
                addResult((int)(mBarrierBreach2 - mBarrierBreach1));

                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Button btnCmd = (Button)findViewById(R.id.btn_flying_run_cmd);
                        btnCmd.setText("Start");
                    }
                });

            }
        }
    };//CameraListener


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_flying_run_main);

        mContentView = (FrameLayout)findViewById(R.id.lay_flying_run_content);
        setupTabs();

        TabLayout T = (TabLayout)findViewById(R.id.tab_flying_run_main);
        T.getTabAt(0).select();

        Runnable UIUpdateTasks = new Runnable(){
            @Override
            public void run() {

                TFCBarrierCamera.BarrierCameraData Data = null;

                for(int i=0; i < 2; ++i) {
                    if (mBarrierCams[i] == null) continue;
                    Data = mBarrierCams[i].cameraData();

                    int tvLatencyID = (i == 0) ? R.id.tv_flying_run_cam1_ping : R.id.tv_flying_run_cam2_ping;
                    int tvStateID = (i == 0) ? R.id.tv_flying_run_cam1_state : R.id.tv_flying_run_cam2_state;
                    int ivPreviewID = (i == 0) ? R.id.iv_flying_run_config_lb1_preview : R.id.iv_flying_run_config_lb2_preview;
                    int tvBatteryChargeID = (i == 0) ? R.id.tv_flying_run_cam1_battery : R.id.tv_flying_run_cam2_battery;
                    int tvCameraNameID = (i == 0) ? R.id.tv_flying_run_cam1_name : R.id.tv_flying_run_cam2_name;


                    // update data
                    TextView tvLatency = (TextView) findViewById(tvLatencyID);
                    TextView tvStatus = (TextView) findViewById(tvStateID);
                    TextView tvBattery = (TextView)findViewById(tvBatteryChargeID);
                    TextView tvCameraName = (TextView)findViewById(tvCameraNameID);


                    if (null != tvLatency) tvLatency.setText(Integer.toString(Data.Latency) + "ms");
                    if(null != tvBattery) tvBattery.setText(Integer.toString(Data.BatteryCharge) + "%");
                    if (!Data.Connected && null != tvStatus) {
                        tvStatus.setText("Nicht verbunden");
                    } else if (Data.Detecting && null != tvStatus) {
                        tvStatus.setText("Scharf");
                    } else if(null != tvStatus){
                        tvStatus.setText("Bereit");
                    }
                    if(null != tvCameraName) tvCameraName.setText(mBarrierCams[i].cameraName());

                    if(Data.MeasurePointsChanged) {
                        int editMeasurePoint1StartID = (i == 0) ? R.id.edit_flying_run_config_lb1_mp1_from : R.id.edit_flying_run_config_lb2_mp1_from;
                        int editMeasurePoint1EndID = (i == 0) ? R.id.edit_flying_run_config_lb1_mp1_to : R.id.edit_flying_run_config_lb2_mp1_to;
                        int editMeasurePoint2StartID = (i == 0) ? R.id.edit_flying_run_config_lb1_mp2_from : R.id.edit_flying_run_config_lb2_mp2_from;
                        int editMeasurePoint2EndID = (i == 0) ? R.id.edit_flying_run_config_lb1_mp2_to : R.id.edit_flying_run_config_lb2_mp2_to;
                        int editMeasurePoint3StartID = (i == 0) ? R.id.edit_flying_run_config_lb1_mp3_from : R.id.edit_flying_run_config_lb2_mp3_from;
                        int editMeasurePoint3EndID = (i == 0) ? R.id.edit_flying_run_config_lb1_mp3_to : R.id.edit_flying_run_config_lb2_mp3_to;
                        int editMeasurePoint4StartID = (i == 0) ? R.id.edit_flying_run_config_lb1_mp4_from : R.id.edit_flying_run_config_lb2_mp4_from;
                        int editMeasurePoint4EndID = (i == 0) ? R.id.edit_flying_run_config_lb1_mp4_to : R.id.edit_flying_run_config_lb2_mp4_to;
                        int editSensitivityID = (i == 0) ? R.id.edit_flying_run_config_lb1_sensitivity : R.id.edit_flying_run_config_lb2_sensitivity;

                        EditText editMeasurePoint1Start = findViewById(editMeasurePoint1StartID);
                        EditText editMeasurePoint1End = findViewById(editMeasurePoint1EndID);
                        EditText editMeasurePoint2Start = findViewById(editMeasurePoint2StartID);
                        EditText editMeasurePoint2End = findViewById(editMeasurePoint2EndID);
                        EditText editMeasurePoint3Start = findViewById(editMeasurePoint3StartID);
                        EditText editMeasurePoint3End = findViewById(editMeasurePoint3EndID);
                        EditText editMeasurePoint4Start = findViewById(editMeasurePoint4StartID);
                        EditText editMeasurePoint4End = findViewById(editMeasurePoint4EndID);
                        EditText editSensitivity = (EditText)findViewById(editSensitivityID);

                        if (null != editMeasurePoint1Start)
                            editMeasurePoint1Start.setText(Short.toString(Data.MeasurePoint1Start));
                        if (null != editMeasurePoint1End)
                            editMeasurePoint1End.setText(Short.toString(Data.MeasurePoint1End));
                        if (null != editMeasurePoint2Start)
                            editMeasurePoint2Start.setText(Short.toString(Data.MeasurePoint2Start));
                        if (null != editMeasurePoint2End)
                            editMeasurePoint2End.setText(Short.toString(Data.MeasurePoint2End));
                        if (null != editMeasurePoint3Start)
                            editMeasurePoint3Start.setText(Short.toString(Data.MeasurePoint3Start));
                        if (null != editMeasurePoint3End)
                            editMeasurePoint3End.setText(Short.toString(Data.MeasurePoint3End));
                        if (null != editMeasurePoint4Start)
                            editMeasurePoint4Start.setText(Short.toString(Data.MeasurePoint4Start));
                        if (null != editMeasurePoint4End)
                            editMeasurePoint4End.setText(Short.toString(Data.MeasurePoint4End));
                        if(null != editSensitivity)
                            editSensitivity.setText(Float.toString(Data.Sensitivity));

                        Data.MeasurePointsChanged = false;
                    }

                    // update preview image
                    Bitmap PrevImg = Data.buildPreviewBitmap(true);
                    if (null != PrevImg) {
                        ImageView ivCam = (ImageView) findViewById(ivPreviewID);
                        if (ivCam != null) ivCam.setImageBitmap(PrevImg);
                    }
                }//for[2 cameras]

            }//run
        };

        mUserInterfaceUpdateTimer.scheduleAtFixedRate(new TimerTask(){
            @Override
            synchronized public void run() {
                runOnUiThread(UIUpdateTasks);
            }}, 1000, 1050);

    }//onCreate

    @Override
    protected void onResume() {
        super.onResume();
    }

    private void addResult(int Duration){

        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {

                Result Res = new Result();

                EditText editTrackLength = (EditText)findViewById(R.id.edit_flying_run_distance);
                String TrackString = editTrackLength.getText().toString();
                if(TrackString.isEmpty()) TrackString = "10";

                Res.TrackLength = Float.parseFloat(TrackString);
                Res.RaceDuration = Duration;

                int RaceTimeSeconds = Duration / 1000;
                int Residual = Duration % 1000;
                int RaceTime100ms = Residual/100;
                Residual = Residual - RaceTime100ms * 100;
                int RaceTime10ms = Residual/10;
                int RaceTime1ms = Residual - RaceTime10ms * 10;

                // round value if necessary
                if(RaceTime1ms >= 5){
                    if(RaceTime100ms == 9 && RaceTime10ms == 9){
                        RaceTimeSeconds += 1;
                        RaceTime100ms = 0;
                        RaceTime10ms = 0;
                    }else if(RaceTime10ms == 9){
                        RaceTime100ms += 1;
                        RaceTime10ms = 0;
                    }else{
                        RaceTime10ms += 1;
                    }
                }

                if(Res.TrackLength > 0){
                    Res.Speed = 1000.0f * Res.TrackLength / Duration;
                }

                String TimeString = Integer.toString(RaceTimeSeconds) + "," + Integer.toString(RaceTime100ms) + Integer.toString(RaceTime10ms) + " s";
                String SpeedString = String.format("%.2f m/s", Res.Speed);

                Res.Row = new TableRow(getApplicationContext());
                Res.Row.setLayoutParams(new TableLayout.LayoutParams(TableRow.LayoutParams.MATCH_PARENT, TableRow.LayoutParams.WRAP_CONTENT));
                TextView tvRunID = new TextView(getApplicationContext());
                TextView tvTime = new TextView(getApplicationContext());
                TextView tvSpeed = new TextView(getApplicationContext());

                tvRunID.setText(Integer.toString(mResults.size()));
                tvRunID.setTextColor(Color.BLACK);
                tvRunID.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
                tvTime.setText(TimeString);
                tvTime.setTextColor(Color.BLACK);
                tvTime.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
                tvSpeed.setText(SpeedString);
                tvSpeed.setTextColor(Color.BLACK);
                tvSpeed.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);

                Res.Row.addView(buildDivider(false));
                Res.Row.addView(tvRunID);
                Res.Row.addView(buildDivider(false));
                Res.Row.addView(tvTime);
                Res.Row.addView(buildDivider(false));
                Res.Row.addView(tvSpeed);
                Res.Row.addView(buildDivider(false));

                mResults.add(Res);

                TableLayout layResult = (TableLayout)findViewById(R.id.lay_flying_run_results);
                buildResultTable(layResult);
            }
        });
    }//addResult

    private void setupTabs(){
        TabLayout T = (TabLayout)findViewById(R.id.tab_flying_run_main);
        T.addOnTabSelectedListener(new TabLayout.OnTabSelectedListener(){

            @Override
            public void onTabSelected(@NonNull TabLayout.Tab tab) {
                if(mContentView != null) mContentView.removeAllViews();
                switch(tab.getPosition()){
                    case 0:{
                        getLayoutInflater().inflate(R.layout.flying_run_capture, mContentView, true);
                        mActivity.runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                if(null != mBarrierCams[0] && null != mBarrierCams[1]){
                                    Button BtnCmd = (Button)findViewById(R.id.btn_flying_run_cmd);
                                    if(BtnCmd != null) BtnCmd.setEnabled(true);
                                }
                                if(mBarrierCams[0] != null) {
                                    mBarrierCams[0].cameraData().MeasurePointsChanged = true;
                                    setCameraUIData(0);
                                }
                                if(mBarrierCams[1] != null){
                                    mBarrierCams[1].cameraData().MeasurePointsChanged = true;
                                    setCameraUIData(1);
                                }
                                if(!mResults.isEmpty()) {
                                    TableLayout layResult = (TableLayout) findViewById(R.id.lay_flying_run_results);
                                    buildResultTable(layResult);
                                }

                            }//run[change UI]
                        });
                    }break;
                    case 1:{
                        getLayoutInflater().inflate(R.layout.flying_run_config, mContentView, true);
                        mActivity.runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                if(null != mBarrierCams[0] && null != mBarrierCams[1]){
                                    Button BtnCmd = (Button)findViewById(R.id.btn_flying_run_cmd);
                                    if(BtnCmd != null) BtnCmd.setEnabled(true);
                                }
                                if(mBarrierCams[0] != null) {
                                    mBarrierCams[0].cameraData().MeasurePointsChanged = true;
                                    setCameraUIData(0);
                                }
                                if(mBarrierCams[1] != null){
                                    mBarrierCams[1].cameraData().MeasurePointsChanged = true;
                                    setCameraUIData(1);
                                }

                            }//run[change UI]
                        });
                    }
                    default: break;
                }//Switch[tab]
            }//onTabSelected

            @Override
            public void onTabReselected(TabLayout.Tab tab) {
                if(mContentView.getChildCount() == 0) onTabSelected(tab);
            }

            @Override
            public void onTabUnselected(TabLayout.Tab tab) {

            }
        });//add[OnTabSelectedListener]

    }//setupTabs


    public void onBtnCmdFlyingRunConfigLB1Clicked(View v){
        Button CmdBtn = (Button)findViewById(R.id.btn_flying_run_config_lb1_cmd);
        if(null == mBarrierCams[0]){
            // list available devices
            LinearLayout Lay = (LinearLayout)findViewById(R.id.lay_flying_run_config_lb1_devices);
            listTFCBarrierCameraDevices(Lay, CmdBtn, 0);
        }else{
            // update camera data
            EditText MP1Start = findViewById(R.id.edit_flying_run_config_lb1_mp1_from);
            EditText MP2Start = findViewById(R.id.edit_flying_run_config_lb1_mp2_from);
            EditText MP3Start = findViewById(R.id.edit_flying_run_config_lb1_mp3_from);
            EditText MP4Start = findViewById(R.id.edit_flying_run_config_lb1_mp4_from);
            EditText MP1End = findViewById(R.id.edit_flying_run_config_lb1_mp1_to);
            EditText MP2End = findViewById(R.id.edit_flying_run_config_lb1_mp2_to);
            EditText MP3End = findViewById(R.id.edit_flying_run_config_lb1_mp3_to);
            EditText MP4End = findViewById(R.id.edit_flying_run_config_lb1_mp4_to);

            short Starts[] = new short[4];
            short Ends[] = new short[4];
            if(!MP1Start.getText().toString().isEmpty()) Starts[0] = Short.parseShort(MP1Start.getText().toString());
            if(!MP2Start.getText().toString().isEmpty()) Starts[1] = Short.parseShort(MP2Start.getText().toString());
            if(!MP3Start.getText().toString().isEmpty()) Starts[2] = Short.parseShort(MP3Start.getText().toString());
            if(!MP4Start.getText().toString().isEmpty()) Starts[3] = Short.parseShort(MP4Start.getText().toString());
            if(!MP1End.getText().toString().isEmpty()) Ends[0] = Short.parseShort(MP1End.getText().toString());
            if(!MP2End.getText().toString().isEmpty()) Ends[1] = Short.parseShort(MP2End.getText().toString());
            if(!MP3End.getText().toString().isEmpty()) Ends[2] = Short.parseShort(MP3End.getText().toString());
            if(!MP4End.getText().toString().isEmpty()) Ends[3] = Short.parseShort(MP4End.getText().toString());

            Log.i("FlyingRunMain", "Sending Data: " + Short.toString(Starts[0]) + "-" + Short.toString(Ends[0]));

            mBarrierCams[0].configMeasurementLines(Starts, Ends);

            // update sensitivity
            EditText Cam1Sensitivity = (EditText)findViewById(R.id.edit_flying_run_config_lb1_sensitivity);
            String Value = Cam1Sensitivity.getText().toString();
            if(!Value.isEmpty()){
                float NewSensitivity = Float.parseFloat(Value);
                float OldSensitivity = mBarrierCams[0].cameraData().Sensitivity;

                if(NewSensitivity > OldSensitivity - 0.01 && NewSensitivity < OldSensitivity + 0.01){

                }else{
                    mBarrierCams[0].changeSensitivity(NewSensitivity);
                }
            }
        }
    }

    public void onBtnCmdFlyingRunConfigLB2Clicked(View v){
        Button CmdBtn = (Button)findViewById(R.id.btn_flying_run_config_lb2_cmd);
        if(null == mBarrierCams[1]){
            // list available devices
            LinearLayout Lay = (LinearLayout)findViewById(R.id.lay_flying_run_config_lb2_devices);
            listTFCBarrierCameraDevices(Lay, CmdBtn, 1);
        }else{
            // update camera data
            EditText MP1Start = findViewById(R.id.edit_flying_run_config_lb2_mp1_from);
            EditText MP2Start = findViewById(R.id.edit_flying_run_config_lb2_mp2_from);
            EditText MP3Start = findViewById(R.id.edit_flying_run_config_lb2_mp3_from);
            EditText MP4Start = findViewById(R.id.edit_flying_run_config_lb2_mp4_from);
            EditText MP1End = findViewById(R.id.edit_flying_run_config_lb2_mp1_to);
            EditText MP2End = findViewById(R.id.edit_flying_run_config_lb2_mp2_to);
            EditText MP3End = findViewById(R.id.edit_flying_run_config_lb2_mp3_to);
            EditText MP4End = findViewById(R.id.edit_flying_run_config_lb2_mp4_to);

            short Starts[] = new short[4];
            short Ends[] = new short[4];
            if(!MP1Start.getText().toString().isEmpty()) Starts[0] = Short.parseShort(MP1Start.getText().toString());
            if(!MP2Start.getText().toString().isEmpty()) Starts[1] = Short.parseShort(MP2Start.getText().toString());
            if(!MP3Start.getText().toString().isEmpty()) Starts[2] = Short.parseShort(MP3Start.getText().toString());
            if(!MP4Start.getText().toString().isEmpty()) Starts[3] = Short.parseShort(MP4Start.getText().toString());
            if(!MP1End.getText().toString().isEmpty()) Ends[0] = Short.parseShort(MP1End.getText().toString());
            if(!MP2End.getText().toString().isEmpty()) Ends[1] = Short.parseShort(MP2End.getText().toString());
            if(!MP3End.getText().toString().isEmpty()) Ends[2] = Short.parseShort(MP3End.getText().toString());
            if(!MP4End.getText().toString().isEmpty()) Ends[3] = Short.parseShort(MP4End.getText().toString());

            mBarrierCams[1].configMeasurementLines(Starts, Ends);

            // update sensitivity
            EditText Cam2Sensitivity = (EditText)findViewById(R.id.edit_flying_run_config_lb2_sensitivity);
            String Value = Cam2Sensitivity.getText().toString();
            if(!Value.isEmpty()){
                float NewSensitivity = Float.parseFloat(Value);
                float OldSensitivity = mBarrierCams[1].cameraData().Sensitivity;

                if(NewSensitivity > OldSensitivity - 0.01 && NewSensitivity < OldSensitivity + 0.01){

                }else{
                    mBarrierCams[1].changeSensitivity(NewSensitivity);
                }
            }
        }
    }



    public void onIVPreviewClicked(View v){
        ImageView iv = (ImageView)v;
        Bitmap bmp = ((BitmapDrawable)iv.getDrawable()).getBitmap();
        TFCUtilities.storeImage(this, bmp, "Preview" + System.currentTimeMillis() + ".jpg");
    }

    public void onBtnPreviewFlyingRunConfigLB1Clicked(View v){
        if(null != mBarrierCams[0]){
            mBarrierCams[0].updatePreviewImage();
        }else{
            Log.i("FylingRunMain", "Barrier camera 0 not connected!");
        }
    }

    public void onBtnPreviewFlyingRunConfigLB2Clicked(View v){
        if(null != mBarrierCams[1]){
            mBarrierCams[1].updatePreviewImage();
        }else{
            Log.i("FylingRunMain", "Barrier camera 1 not connected!");
        }
    }

    public void onBtnCmdClicked(View v){
        if(!mOngoingRace){
            if(mBarrierCams[0] != null) mBarrierCams[0].startDetection();
            if(mBarrierCams[1] != null) mBarrierCams[1].startDetection();

            Button BtnCmd = (Button)v;
            BtnCmd.setText("Bereit ...");
            mBarrierBreach1 = 0;
            mBarrierBreach2 = 0;

            mOngoingRace = true;
        }else{
            if(mBarrierCams[0] != null) mBarrierCams[0].stopDetection();
            if(mBarrierCams[1] != null) mBarrierCams[1].stopDetection();
            mBarrierBreach1 = 0;
            mBarrierBreach2 = 0;

            if(null != mRaceClockTimerTask){
                mRaceClockTimerTask.cancel();
                mRaceClockTimerTask = null;
            }
            mOngoingRace = false;
            ((Button)v).setText("Start");
        }
    }

    private void listTFCBarrierCameraDevices(LinearLayout L, Button CmdButton, int CameraID){
        TFCUtilities.startBluetooth(this);

        Set<BluetoothDevice> PairedBTDevs = BluetoothAdapter.getDefaultAdapter().getBondedDevices();

        View.OnClickListener Listener = new View.OnClickListener(){
            @Override
            public void onClick(View v) {
                TextView tv = (TextView)v;
                BluetoothDevice BTDev = null;
                for(BluetoothDevice i: PairedBTDevs){
                    if(i.getName().equals(tv.getText())){
                        BTDev = i;
                        break;
                    }
                }

                if(null == mBarrierCams[CameraID]) mBarrierCams[CameraID] = new TFCBarrierCamera();
                mBarrierCams[CameraID].init(FlyingRunMain.this, CameraID, BTDev, mCameraListener);

                L.removeAllViews();
                CmdButton.setEnabled(false);
                CmdButton.setText("Verbinde...");
            }
        };//OnClickListener

        L.removeAllViews();

        for(BluetoothDevice i : PairedBTDevs){
            if(i.getName().contains("T&F")){
                Log.i("TFCBarrierCamera", "Found T&F Barrier Camera: " + i.getName());

                TextView tv = new TextView(getApplicationContext());
                tv.setText(i.getName());
                tv.setGravity(View.TEXT_ALIGNMENT_CENTER);
                tv.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
                tv.setOnClickListener(Listener);
                tv.setTextColor(Color.BLACK);
                L.addView(tv, new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
            }
        }//for[Paired Devices]
    }//listTFCBarrierCameraDevices

}//FlyingRunMain