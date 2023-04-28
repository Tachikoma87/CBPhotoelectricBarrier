package com.crossforge.tfchampion;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.SystemClock;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.util.Range;
import android.util.Size;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.TextureView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.HorizontalScrollView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.view.ViewGroup.LayoutParams;
import android.widget.TextView;
import android.widget.Toast;


import com.google.android.material.tabs.TabLayout;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Set;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.TimeUnit;

public class SprintMain extends AppCompatActivity {

    private static final String KEY_LAST_TAB_SELECTED = "Last_Selected_Tab";

    private FlapCommunication mRelayCom = null;

    private ViewGroup mContentView = null;
    private int mLastSelectedTab = 0;

    OpticalRecorder mRecorder = null;

    private Size mRecordingSize = new Size(1080, 720);
    //private int mResultHeight = 0;

    private OpticalRecorder.RecorderstateCallback mRecorderCallback = null;

    private Timer mUserInterfaceUpdateTimer = new Timer();
    private Timer mRaceTimer = null;
    private long mRaceStartTime = 0;

    private boolean mMeasureMode = false;
    private FinishPhoto mFinishPhoto = null;
    private ArrayList<ResultView> mResultViews;

    private FlapCommunication.StartingFlapListener mStartingFlapListener = null;

    private int mTemporaryResultLinePosition = 0;
    private long mLastResultLineUpdate = 0;
    private float mResultDisplayScale = 1.0f;
    private int mLastUpdatedRV = -1;

    private TFCBarrierCamera.BarrierCameraListener mLightbarrierListener = null;

    private TFCBarrierCamera mLightbarrier = null;
    int mRaceStopTimestamp = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_sprint_main);

        mContentView = findViewById(R.id.lay_sprint_content);
        setupTabs();
        TabLayout T = (TabLayout)findViewById(R.id.tab_sprint_main);
        // restore old tab
        if(null != savedInstanceState){
            T.getTabAt(savedInstanceState.getInt(KEY_LAST_TAB_SELECTED)).select();
        }else{
            T.getTabAt(0).select();
        }

        mRecorderCallback = new OpticalRecorder.RecorderstateCallback() {
            @Override
            void recorderStateCallback(RecorderMessage Msg, OpticalRecorder.SessionType SType) {
                super.recorderStateCallback(Msg, SType);

                TextureView TV = (TextureView)findViewById(R.id.tv_sprint_capture);;
                Button CmdButton = (Button)findViewById(R.id.btn_sprint_capture_cmd);

                if(Msg == RecorderMessage.MSG_CAMERA_OPENED){
                    CmdButton.setText(getString(R.string.sprint_capture_btn_cmd_calibrate));
                    mRecorder.startCaptureSession(TV, OpticalRecorder.SessionType.TYPE_PREVIEW, mRecordingSize, -1);
                }else if(Msg == RecorderMessage.MSG_CALIBRATION_FINISHED){
                    CmdButton.setText(getString(R.string.sprint_capture_btn_cmd_start_race));

                }else if(Msg == RecorderMessage.MSG_RECORDING_FINISHED && SType == OpticalRecorder.SessionType.TYPE_RACE){
                    mFinishPhoto = null;
                    TabLayout T = (TabLayout) findViewById(R.id.tab_sprint_main);
                    T.selectTab(T.getTabAt(1));
                }else if(Msg == RecorderMessage.MSG_RECORDING_STARTED && SType == OpticalRecorder.SessionType.TYPE_RACE){
                    CmdButton.setText(getString(R.string.sprint_capture_btn_cmd_stop));
                }
            }
        };//RecorderStateCallback

        Runnable UIUpdateTasks = new Runnable(){
            @Override
            public void run() {
                if(mRelayCom != null){
                    FlapCommunication.FlapData Data = mRelayCom.getFlapData();
                    TextView tvLatency = (TextView) findViewById(R.id.tv_sprint_capture_latency);
                    TextView tvRssi = (TextView) findViewById(R.id.tv_sprint_capture_rssi);
                    TextView tvBattery = (TextView)findViewById(R.id.tv_sprint_capture_battery);
                    TextView tvBatteryRelay = (TextView)findViewById(R.id.tv_sprint_capture_battery_relay);
                    if (null != tvLatency)
                        tvLatency.setText("Latency: " + Integer.toString(Data.mLatency) + "ms");
                    if (null != tvRssi)
                        tvRssi.setText("RSSI: " + Integer.toString(Data.mRSSI) + "dBm");
                    if(null != tvBattery)
                        tvBattery.setText("Battery (Flap): " + Integer.toString(Data.mFlapBatteryCharge) + "%");
                    if(null != tvBatteryRelay)
                        tvBatteryRelay.setText("Battery (Relay): " + Integer.toString(Data.mRelayBatteryCharge) + "%");
                }//if[mRelay]

                if(mRecorder.isRecording()){
                    TextView tvFPS = (TextView)findViewById(R.id.tv_sprint_capture_fps);
                    if(null != tvFPS) tvFPS.setText("Capturing@" + Integer.toString(mRecorder.fps()) + "FPS");
                }

            }
        };

        mUserInterfaceUpdateTimer.scheduleAtFixedRate(new TimerTask(){
            @Override
            synchronized public void run() {
                runOnUiThread(UIUpdateTasks);
            }}, 1000, 1000);

        mStartingFlapListener = new FlapCommunication.StartingFlapListener(){
            @Override
            public void onStartSignalReceived(long Timestamp) {

                Log.i("SprintMain", "Want to start race!");
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        startRace(Timestamp);
                    }
                });
            }
        };//StartingFlapListener

        mUserInterfaceUpdateTimer.scheduleAtFixedRate(new TimerTask(){
            @Override
            synchronized public void run() {
                runOnUiThread(UIUpdateTasks);
        }}, 1000, 1000);


        mLightbarrierListener = new TFCBarrierCamera.BarrierCameraListener() {
            @Override
            public void onConnectionEstablished(int CameraID) {
                super.onConnectionEstablished(CameraID);
                Log.i("SprintMain", "Connection to Lightbarrier established!");
                mLightbarrier.changeSensitivity(1.0f);
            }

            @Override
            public void onBarrierBreach(int CameraID, int Timestamp) {
                super.onBarrierBreach(CameraID, Timestamp);
                Log.i("SprintMain", "Barrier Breached");

                mRaceStopTimestamp = Timestamp;
                mRaceTimer.cancel();
                mRaceTimer = null;

                int RaceDuration = (int)(mRaceStopTimestamp - mRaceStartTime);
                int Seconds = RaceDuration/1000;
                RaceDuration = RaceDuration % 1000;
                int DSeconds = RaceDuration / 100;
                RaceDuration = RaceDuration % 100;
                int HSeconds = RaceDuration / 10;

                String Timestring = String.format("%d,%d%ds", Seconds, DSeconds, HSeconds);

                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Button Cmd = (Button)findViewById(R.id.btn_sprint_capture_cmd);
                        TextView tvRaceClock = (TextView)findViewById(R.id.tv_sprint_capture_clock);
                        Cmd.setText("Start");
                        tvRaceClock.setText(Timestring);
                    }
                });


            }
        };//LightbarrierListener
    }//onCreate

    @Override
    protected void onSaveInstanceState(@NonNull Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putInt(KEY_LAST_TAB_SELECTED, mLastSelectedTab);
    }

    // Makes sure the tabs are working correctly and that they can change view as requested.
    private void setupTabs(){
        // retrieve tabs context
        TabLayout T = (TabLayout) findViewById(R.id.tab_sprint_main);
        T.addOnTabSelectedListener(new TabLayout.OnTabSelectedListener(){
            @Override
            public void onTabSelected(TabLayout.Tab tab) {
                if(null != mContentView) mContentView.removeAllViews();

                switch(tab.getPosition()){
                    case 0:{
                        getLayoutInflater().inflate(R.layout.sprint_capture, mContentView, true);
                        Button Cmd = (Button)findViewById(R.id.btn_sprint_capture_cmd);
                        /*TextureView TV = (TextureView)findViewById(R.id.tv_sprint_capture);
                        TV.setVisibility(View.VISIBLE);*/
                        mLastSelectedTab = 0;
                        if(null != mLightbarrier){
                            if(null != Cmd) Cmd.setText("Starte Rennen");
                        }
                        else if(null != mRecorder && mRecorder.isCameraReady() && mRecorder.isCalibrated()){
                            if(null != Cmd) Cmd.setText(R.string.sprint_capture_btn_cmd_start_race);
                        }
                    }break;
                    case 1:{
                        getLayoutInflater().inflate(R.layout.sprint_result, mContentView, true);
                        mLastSelectedTab = 1;
                        HorizontalScrollView ScrollView = (HorizontalScrollView)findViewById(R.id.hsv_sprint_result);
                        ScrollView.setOnTouchListener(new View.OnTouchListener(){
                            @Override
                            public boolean onTouch(View v, MotionEvent event) {
                                long Millis = SystemClock.uptimeMillis();
                                if(mMeasureMode && Millis - mLastResultLineUpdate > 50){
                                    int x = (int)event.getX();
                                    int y = (int)event.getY();
                                    int ScrollX = findViewById(R.id.hsv_sprint_result).getScrollX();

                                    mTemporaryResultLinePosition = (int)Math.floor((ScrollX + x) / mResultDisplayScale);
                                    updateTemporaryResultLine();

                                    mLastResultLineUpdate = Millis;
                                }

                                return mMeasureMode;
                            }
                        });
                        showRaceResult();
                    }break;
                    case 2:{
                        getLayoutInflater().inflate(R.layout.sprint_flap, mContentView, true);

                        // search for available devices if necessary
                        if(mRelayCom == null) onFlapSearchButtonClicked(null);
                        if(mLightbarrier == null) onFinisherCamSearchButtonClicked(null);
                        else{
                            TextView tvName = (TextView)findViewById(R.id.tv_sprint_finisher_cam_name);
                            EditText editSensitivity = (EditText)findViewById(R.id.edit_sprint_lightbarrier_sensitivity);

                            tvName.setText(mLightbarrier.cameraName());

                            float Sensitivity = mLightbarrier.cameraData().Sensitivity;
                            int Val = (int)(-1 * ((Sensitivity*10) - 100));
                            editSensitivity.setText(Integer.toString(Val));

                            editSensitivity.addTextChangedListener(new TextWatcher() {
                                @Override
                                public void beforeTextChanged(CharSequence s, int start, int count, int after) {

                                }

                                @Override
                                public void onTextChanged(CharSequence s, int start, int before, int count) {
                                    String Text = s.toString();
                                    if(!Text.isEmpty()){
                                        int Value = Integer.parseInt(Text);
                                        float Sensitivity = (100 - Value)/10.0f;
                                        mLightbarrier.changeSensitivity(Sensitivity);
                                    }
                                }

                                @Override
                                public void afterTextChanged(Editable s) {

                                }
                            });
                        }

                        mLastSelectedTab = 2;
                    }break;

                    default:{
                        Log.e("SprintMain", "Unhandled tab item selected!");
                        mLastSelectedTab = 0;
                    }
                }
            }

            @Override
            public void onTabUnselected(TabLayout.Tab tab) {

            }

            @Override
            public void onTabReselected(TabLayout.Tab tab) {
                if(mContentView.getChildCount() == 0) onTabSelected(tab);
            }
        });// addOnTabSelectedListener
    }//setupTabs

    @Override
    protected void onResume() {
        super.onResume();

        if (null == mRecorder) {
            mRecorder = new OpticalRecorder();
            mRecorder.init(this, getApplicationContext(), mRecorderCallback);
        }
    }

    public void cmdButtonCaptureClicked(View v){
        TextureView TV = (TextureView) findViewById(R.id.tv_sprint_capture);
        Button BtnCmd = (Button)findViewById(R.id.btn_sprint_capture_cmd);
        ImageView ivFinishLineIndicator = (ImageView)findViewById(R.id.iv_finish_line_marker);

        if(null != mLightbarrier) {
            if(mRaceTimer != null){
                mRaceTimer.cancel();
                mRaceTimer.cancel();
                mRaceTimer = null;
                BtnCmd.setText("Start");
                mLightbarrier.stopDetection();
            }
            else if(null == mRelayCom || !mRelayCom.isConnected()){
                // no starting flap connected, just start the race on button press
                startRace(SystemClock.uptimeMillis());
            }else{
                if(!mRelayCom.issueStartRequest()) Log.e("SprintMain", "Start request could not be issued!");
                // issue start request to starting flap
                TextView tvClock = (TextView)findViewById(R.id.tv_sprint_capture_clock);
                tvClock.setText("Bereit ...");
                BtnCmd.setText("Cancel Race");
            }
        }
        else{
            if(!mRecorder.isCameraReady()) {
                mRecorder.openCamera(true);
                // show finish line indicator
                if(null != ivFinishLineIndicator) ivFinishLineIndicator.setVisibility(View.VISIBLE);
            }else if(!mRecorder.isCalibrated()){
                mRecorder.startCaptureSession(TV, OpticalRecorder.SessionType.TYPE_CALIBRATION, mRecordingSize, -1);
                BtnCmd.setText(getText(R.string.sprint_capture_btn_cmd_calibrating));
                if(null != ivFinishLineIndicator) ivFinishLineIndicator.setVisibility(View.GONE);
            }else  if(mRecorder.isRecording()){
                BtnCmd.setText(getString(R.string.sprint_capture_btn_cmd_generating_photofinish));
                mRecorder.stopCaptureSession();
                mRaceTimer.cancel();
                mRaceTimer = null;
            }else if(null == mRelayCom || !mRelayCom.isConnected()){
                // no starting flap connected, just start the race on button press
                startRace(SystemClock.uptimeMillis());

            }else{
                if(!mRelayCom.issueStartRequest()) Log.e("SprintMain", "Start request could not be issued!");
                // issue start request to starting flap
                TextView tvClock = (TextView)findViewById(R.id.tv_sprint_capture_clock);
                tvClock.setText("Bereit ...");
                BtnCmd.setText("Cancel Race");
            }
        }//else[no Lightbarrier]
    }

    private void startRace(long Timestamp){
        if(mRaceTimer != null){
            Log.i("SprintMain", "Attempting to start race again!");
            return;
        }

        mRaceStartTime = Timestamp;
        TextureView TV = (TextureView) findViewById(R.id.tv_sprint_capture);
        if(mRecorder.isCameraReady())  mRecorder.startCaptureSession(TV, OpticalRecorder.SessionType.TYPE_RACE, mRecordingSize, -1);
        if(null != mLightbarrier) mLightbarrier.startDetection();

        if(mRaceTimer == null) mRaceTimer = new Timer();
        mRaceTimer.scheduleAtFixedRate(new TimerTask(){
            @Override
            public void run() {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        TextView tvClock = (TextView)findViewById(R.id.tv_sprint_capture_clock);
                        int Dur = (int)(SystemClock.uptimeMillis() - mRaceStartTime);
                        int Residual = (Dur%1000)/10;
                        String T = String.format("%d,%2ds",
                                TimeUnit.MILLISECONDS.toSeconds(Dur),
                                Residual);
                        if(null != tvClock) tvClock.setText(T);
                    }
                });
            }
        }, 0, 10);
    }

    private void showRaceResult(){
        // if we don't already have the finish photo, we need to retrieve it and need to perform some basic operations on it
        if(mFinishPhoto == null){
            mRecorder.buildFinishPhoto();

            mFinishPhoto = new FinishPhoto();
            mFinishPhoto.mRawOriginal = mRecorder.retrieveFinishPhoto();
            mFinishPhoto.mRawTimings = mRecorder.retrieveFinishPhotoTimestamps();
            Log.i("StripPhoto", "Image returned with dims " + Integer.toString(mFinishPhoto.mRawOriginal.getWidth()) + "x" + Integer.toString(mFinishPhoto.mRawOriginal.getHeight()));

            // now we need build the scale (timings positioned on buttom line)
            mFinishPhoto.buildScale(500);
        }

        // generate views dynamically
        // there seems to be a maximum size for the image you can apply, but 2000px (width) per view  works fine
        LinearLayout LayResult = (LinearLayout)findViewById(R.id.lay_sprint_result);
        LayResult.removeAllViews();

        // build the image views to display the result
        mResultViews = new ArrayList<ResultView>();

        int CurrentX = 0;
        int IDs = 0;

        mResultDisplayScale =  (float)(getResources().getDisplayMetrics().heightPixels) / (float)(mFinishPhoto.mRawOriginal.getHeight());
        mResultDisplayScale *= 2.0/3.0;

        LayoutParams Params = new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
        int FinishPhotoWidth = mFinishPhoto.mRawOriginal.getWidth();
        while(CurrentX < FinishPhotoWidth){
            ResultView RV = new ResultView();
            RV.mImageView = new ImageView(getApplicationContext());
            RV.mImageView.setVisibility(View.VISIBLE);
            int ImgWidth = Math.min(1080, FinishPhotoWidth - CurrentX);
            RV.mRange = new Range<Integer>(CurrentX, CurrentX  + ImgWidth);
            Bitmap Bmp = mFinishPhoto.buildAugmentedPhoto(CurrentX, ImgWidth);
            RV.mID = IDs;
            LayResult.addView(RV.mImageView, Params);
            RV.updateContent(Bmp);
            mResultViews.add(RV);

            CurrentX += ImgWidth;
            IDs++;
        }
    }//cmdResultButtonClicked

    public void onAddMeasurementLineClick(View v){
        HorizontalScrollView ScrollView = (HorizontalScrollView)findViewById(R.id.hsv_sprint_result);
        Button BtnMeasureLine = (Button)findViewById(R.id.btn_sprint_result_addLine);


        if(mMeasureMode){
            BtnMeasureLine.setText("Place Measurement");
            mMeasureMode = false;
            mFinishPhoto.addFinisher(mTemporaryResultLinePosition);
            updateTemporaryResultLine();
            mTemporaryResultLinePosition = -1;
            updateTemporaryResultLine();
        }else{
            BtnMeasureLine.setText("Finish Placement");
            mMeasureMode = true;

            int ScrollX = (int)Math.floor(ScrollView.getScrollX());
            Rect R = new Rect();
            ScrollView.getWindowVisibleDisplayFrame(R);

            mTemporaryResultLinePosition = (int)Math.floor( (ScrollX + (R.right - R.left)*0.5) / mResultDisplayScale);
            updateTemporaryResultLine();
        }
    }

    public void onFlapSearchButtonClicked(View v){
        Set<BluetoothDevice> PairedBTDevs = BluetoothAdapter.getDefaultAdapter().getBondedDevices();

        LinearLayout DevList = (LinearLayout)findViewById(R.id.lay_sprint_flap_devices);

        View.OnClickListener Listener = new View.OnClickListener(){
            @Override
            public void onClick(View v) {
                TextView tv = (TextView)v;
                Toast.makeText(getApplicationContext(), "Verbinde mit " + tv.getText(), Toast.LENGTH_LONG).show();
                //mRelayCom.connect(tv.getText().toString());
                if(null == mRelayCom) {
                    mRelayCom = new FlapCommunication();
                    // find device
                    for(BluetoothDevice i: PairedBTDevs){
                        if(i.getName().equals(tv.getText())){
                            mRelayCom.init(SprintMain.this, i);
                            mRelayCom.setListener(mStartingFlapListener);
                        }
                    }
                }
            }
        };//Listener

        for(BluetoothDevice i: PairedBTDevs){
            Log.i("SprintMain", "Found BT Device " + i);
            TextView tv = new TextView(getApplicationContext());
            tv.setText(i.getName());
            tv.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
            tv.setGravity(Gravity.CENTER);
            tv.setOnClickListener(Listener);
            tv.setTextColor(Color.BLACK);
            DevList.addView(tv, new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
        }
    }

    public void onFinisherCamSearchButtonClicked(View v){
        Set<BluetoothDevice> PairedBTDevs = BluetoothAdapter.getDefaultAdapter().getBondedDevices();

        LinearLayout DevList = (LinearLayout)findViewById(R.id.lay_sprint_finisher_cam_devices);

        View.OnClickListener Listener = new View.OnClickListener(){
            @Override
            public void onClick(View v) {
                TextView tv = (TextView)v;
                Toast.makeText(getApplicationContext(), "Verbinde mit " + tv.getText(), Toast.LENGTH_LONG).show();
                if(null == mLightbarrier){
                    mLightbarrier = new TFCBarrierCamera();
                    for(BluetoothDevice i: PairedBTDevs){
                        if(i.getName().equals(tv.getText())){
                            mLightbarrier.init(SprintMain.this, 0, i, mLightbarrierListener);
                        }
                    }
                }
            }
        };//Listener

        for(BluetoothDevice i: PairedBTDevs){
            Log.i("SprintMain", "Found BT Device " + i);
            TextView tv = new TextView(getApplicationContext());
            tv.setText(i.getName());
            tv.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
            tv.setGravity(Gravity.CENTER);
            tv.setTextColor(Color.BLACK);
            tv.setOnClickListener(Listener);
            DevList.addView(tv, new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
        }
    }

    public void onConnectFlapButtonClicked(View v){


    }//onConnectFlapButtonClicked

    void updateTemporaryResultLine(){
        for(ResultView RV: mResultViews){
            if(RV.mRange.getLower() <= mTemporaryResultLinePosition && RV.mRange.getUpper() >= mTemporaryResultLinePosition){
                Bitmap Bmp = mFinishPhoto.buildAugmentedPhoto(RV.mRange.getLower(), RV.mRange.getUpper() - RV.mRange.getLower()).copy(Bitmap.Config.ARGB_8888, true);
                float Timestamp = mFinishPhoto.mRawTimings[mTemporaryResultLinePosition] / 1000.0f;
                mFinishPhoto.augmentMeasurmentLine(Bmp, mTemporaryResultLinePosition - RV.mRange.getLower(), Timestamp, (int)(Bmp.getHeight()*0.95f), Color.GREEN, 3);
                RV.updateContent(Bmp);

                if(mLastUpdatedRV != -1 && mLastUpdatedRV != RV.mID){
                    Range<Integer> R = mResultViews.get(mLastUpdatedRV).mRange;
                    Bmp = mFinishPhoto.buildAugmentedPhoto(R.getLower(), R.getUpper() - R.getLower());
                    mResultViews.get(mLastUpdatedRV).updateContent(Bmp);
                }
                mLastUpdatedRV = RV.mID;
            }
        }//for[result views
    }


    class ResultView{
        public ImageView mImageView; // display element
        public Range<Integer> mRange; // from pixel to pixel
        int mID = -1;

        public void updateContent(Bitmap Bmp){
            int w = (int)Math.floor(Bmp.getWidth() * mResultDisplayScale);
            int h = (int)Math.floor(Bmp.getHeight() * mResultDisplayScale);
            Bitmap ScaledBmp = Bitmap.createScaledBitmap(Bmp, w, h, true);
            mImageView.setImageBitmap(ScaledBmp);
        }//updateContent
    }

    class FinishPhoto{

        public Bitmap mRawOriginal = null;
        public int[] mRawTimings = null;

        private ArrayList<Integer> mScalePositions = new ArrayList<Integer>();
        private ArrayList<Float> mScaleTimings = new ArrayList<Float>();

        private ArrayList<Integer> mFinisherPositions = new ArrayList<>();
        private ArrayList<Float> mFinisherTimings = new ArrayList<>();

        // resolution in milliseconds
        public void buildScale(int Resolution){
            int NextStamp = 0;
            for(int i=0; i < mRawTimings.length; ++i){
                if(mRawTimings[i] >= NextStamp){
                    float Stamp = 0.05f * (mRawTimings[i] / 50);
                    mScaleTimings.add(Stamp);
                    mScalePositions.add(i);
                    NextStamp = Resolution*(mRawTimings[i]/Resolution) + Resolution;
                }
            }//for[raw timings]
        }//buildScale

        public void addFinisher(int Position){
            mFinisherPositions.add(Position);
            float Stamp = (float)mRawTimings[Position]/1000.0f;
            mFinisherTimings.add(Stamp);
        }//

        public Bitmap buildAugmentedPhoto(int Start, int Width){
            // create the bitmap
            Bitmap Bmp = Bitmap.createBitmap(mRawOriginal, Start, 0, Width, mRawOriginal.getHeight()).copy(Bitmap.Config.ARGB_8888, true);

            // augment scale
            int LineLength = (int)(Math.ceil(Bmp.getHeight()*0.2f));
            for(int i = 0; i < mScalePositions.size(); ++i){
                int Pos = mScalePositions.get(i);
                if(Pos >= Start && Pos < Start + Width){
                    // add line
                    augmentMeasurmentLine(Bmp, Pos - Start, mScaleTimings.get(i), LineLength, Color.BLUE, 1);
                }
            }

            for(int i=0; i < mFinisherPositions.size(); ++i){
                int Pos = mFinisherPositions.get(i);
                if(Pos >= Start && Pos < Start + Width){
                    augmentMeasurmentLine(Bmp, Pos - Start, mFinisherTimings.get(i), (int)(Bmp.getHeight() * 0.95f), Color.CYAN, 3);
                }
            }

            return Bmp;
        }//buildAugmentedPhoto

        public void augmentMeasurmentLine(Bitmap Bmp, int PositionX, float Timestamp, int LineLength, int LineColor, int DecimalPlaces){
            Canvas C = new Canvas(Bmp);
            Paint p = new Paint();
            p.setColor(LineColor);
            int TextSize = (int)Math.ceil(28 * Bmp.getHeight()/720.0f);
            p.setTextSize(TextSize);
            p.setTextAlign(Paint.Align.CENTER);
            p.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC_OVER));

            // draw the text
            String Format = "%.1fs";
            if(DecimalPlaces == 1)  Format = "%.1fs";
            else if(DecimalPlaces == 2) Format = "%.2fs";
            else Format = "%.3fs";
            C.drawText(String.format(Format, Timestamp), Math.min(PositionX + 3, Bmp.getWidth()), Bmp.getHeight() - 3, p);

            int To = Bmp.getHeight()  - 22 - 5;
            int From = To - LineLength;
            for(int k = From; k < To; ++k) Bmp.setPixel(PositionX, k, LineColor);
        }//augmentMeasurementLine

    }//FinishPhoto



}//SprintMain
