package com.crossforge.tfchampion;


import android.Manifest;
import android.app.Activity;
import android.app.AsyncNotedAppOp;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.Camera;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureFailure;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.TotalCaptureResult;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.Image;
import android.media.ImageReader;
import android.media.MediaRecorder;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Environment;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Parcel;
import android.os.SystemClock;
import android.provider.MediaStore;
import android.renderscript.Allocation;
import android.renderscript.Element;
import android.renderscript.RenderScript;
import android.renderscript.ScriptIntrinsicYuvToRGB;
import android.renderscript.Type;
import android.se.omapi.Session;
import android.util.Log;
import android.util.Range;
import android.util.Size;
import android.view.PixelCopy;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.TextureView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.math.MathUtils;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.sql.Time;
import java.util.Collections;


/**
 * This class is used to grab and process image/video data required for evaluation
 *
 *
 */
public class OpticalRecorder  {

    public enum SessionType{
        TYPE_UNKNOWN,
        TYPE_PREVIEW,
        TYPE_CALIBRATION,
        TYPE_RACE,
    }

    public static abstract class RecorderstateCallback{
        enum RecorderMessage{
            MSG_CAMERA_OPENED,
            MSG_CALIBRATION_FINISHED,
            MSG_RECORDING_STARTED,
            MSG_RECORDING_FINISHED,
        }

        void recorderStateCallback(RecorderMessage Msg, SessionType ActiveSession){

        }
    }//StateCallback

    // some static identifiers
    private static final int CAMERA_REQUEST_CODE = 301;
    private static final String mCameraBackgroundThreadName = "tfchampion_camera_background_thread";

    // Management stuff
    Activity mActivity; ///< Activity that created this instance
    Context mApplicationContext;

    // Camera managment stuff
    CameraManager mCameraMgr;
    HandlerThread mCameraBackgroundThread;
    Handler mCameraBackgroundHandler;


    HandlerThread mPixelCopyThread = null;
    Handler mPixelCopyHandler = null;
    Bitmap mPixelCopyBmp = null;
    boolean mCaptureCompleted = false;

    // actual camera stuff
    CameraDevice mActiveCamera = null;
    CameraDevice.StateCallback mCameraStateCallback;
    CameraCaptureSession mCameraCaptureSession;
    CameraCaptureSession.CaptureCallback mCameraCaptureCallback = null;
    CaptureRequest mCaptureRequest = null;
    private int mCaptureWidth = 800;
    private int mCaptureHeight = 600;
    private Range<Integer> mFPS;

    private long mLastFPSOutput = 0;
    private int mFPSCount = 0;
    private int mCurrentFPS = 0;

    private Range<Integer> mCaptureFPSRange;

    private TextureView mCameraTexView = null;
    private TextureView.SurfaceTextureListener mTVListener = null;

    Size[] mCameraAvailableResolutions = null;
    Range<Integer>[] mCameraAvailableFPSRanges = null;

    Context mContext = null;

    private float mSFCamSensitivity = 1.30f;
    private int mSFCamDiscardThreshold = 500; // in milliseconds


    // data types we need to transform the image to RGB and extract the data we really need
    private Bitmap mBmp;
    RenderScript mRS = null;
    Allocation mTargetAllocation = null;
    Allocation mSourceAllocation = null;
    ScriptIntrinsicYuvToRGB m_YuvToRGBScript;

    private long mCalibrationStarted = 0; // if calibration started != 0 it is calibration mode

    private int mActiveStripPhotoID = -1;

    private boolean mIsCalibrated = false;

    private RecorderstateCallback mListener = null;

    private SessionType mSessionType = SessionType.TYPE_UNKNOWN;

    private ImageReader mImgReader = null;

    private long mLastImageCapture;

    private boolean mGrabingImage = false;

    OpticalRecorder(){

    }//Constructor

    protected void finalize(){
        destroyCameraBackgroundThread();
    }


    void init(@NonNull Activity ParentActivity, @NonNull Context C, RecorderstateCallback Callback){
        mActivity = ParentActivity;
        mCameraMgr = (CameraManager)mActivity.getSystemService(Context.CAMERA_SERVICE);
        mContext = C;
        mListener = Callback;


        // we need camera permission for this class to work properly
        if(ActivityCompat.checkSelfPermission(mActivity, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(mActivity, new String[]{Manifest.permission.CAMERA}, CAMERA_REQUEST_CODE);
            return;
        }

        if(null == mCameraMgr){
            Log.e("OpticalRecorder", "Failed to get camera manager!");
            return;
        }

        // create background thread which handles the camera
        createCameraBackgroundThread();

        // create camera state callback
        mCameraStateCallback = new CameraDevice.StateCallback(){
            @Override
            public void onOpened(CameraDevice CamDev){
                mActiveCamera = CamDev;
                if(null != mListener) mListener.recorderStateCallback(RecorderstateCallback.RecorderMessage.MSG_CAMERA_OPENED, mSessionType);
                Log.i("OpticalRecorder", "Camera opened ...");
            }

            @Override
            public void onClosed(@NonNull CameraDevice camera) {
                mActiveCamera.close();
                mActiveCamera = null;
                super.onClosed(camera);
            }

            @Override
            public void onDisconnected(@NonNull CameraDevice camera) {
                mActiveCamera.close();
                mActiveCamera = null;
            }

            @Override
            public void onError(@NonNull CameraDevice camera, int error) {
                mActiveCamera.close();
                mActiveCamera = null;
                Log.e("OpticalRecorder", "A camera error occurred: "+ Integer.toString(error));
            }
        };//CameraDevice.StateCallback

        mCameraCaptureCallback = new CameraCaptureSession.CaptureCallback() {
            @Override
            public void onCaptureStarted(@NonNull CameraCaptureSession session, @NonNull CaptureRequest request, long timestamp, long frameNumber) {
                super.onCaptureStarted(session, request, timestamp, frameNumber);
            }

            @Override
            public void onCaptureProgressed(@NonNull CameraCaptureSession session, @NonNull CaptureRequest request, @NonNull CaptureResult partialResult) {
                super.onCaptureProgressed(session, request, partialResult);
            }

            @Override
            public void onCaptureCompleted(@NonNull CameraCaptureSession session, @NonNull CaptureRequest request, @NonNull TotalCaptureResult result) {
                super.onCaptureCompleted(session, request, result);

                long Timestamp = SystemClock.uptimeMillis();

                // estimate FPS
                if(Timestamp - mLastFPSOutput > 1000){
                    mCurrentFPS = mFPSCount;
                    mLastFPSOutput = Timestamp;
                    mFPSCount = 0;
                    Log.i("OpticalRecorder", "Capture FPS: " + mCurrentFPS);

                }else{
                    mFPSCount++;
                }

                if(null == mPixelCopyBmp) mPixelCopyBmp = Bitmap.createBitmap(mCaptureWidth, mCaptureHeight, Bitmap.Config.ARGB_8888);

                int width = 47;
                int x = mCaptureWidth/2 - width/2;
                int y = 0;
                int height = mCaptureHeight;
                int[] Pixels = new int[width*height];

                if(mSessionType != SessionType.TYPE_PREVIEW){
                    mCameraTexView.getBitmap(mPixelCopyBmp).getPixels(Pixels, 0, width, x, y, width, height);
                    addStrip(0, Pixels, width, height, Timestamp);
                }

                if(mSessionType == SessionType.TYPE_CALIBRATION && (Timestamp - mCalibrationStarted) > 3000){
                    try{
                        mCameraCaptureSession.stopRepeating();
                    }catch(CameraAccessException e){
                        e.printStackTrace();
                    }
                }

            }//onCaptureCompleted

            @Override
            public void onCaptureFailed(@NonNull CameraCaptureSession session, @NonNull CaptureRequest request, @NonNull CaptureFailure failure) {
                super.onCaptureFailed(session, request, failure);
            }

            @Override
            public void onCaptureSequenceCompleted(@NonNull CameraCaptureSession session, int sequenceId, long frameNumber) {
                super.onCaptureSequenceCompleted(session, sequenceId, frameNumber);
                Log.i("OpticalRecorder", "Capture sequence completed");

                if(mSessionType == SessionType.TYPE_CALIBRATION){
                    calibrateWithExistingData(mActiveStripPhotoID);
                    mCalibrationStarted = 0;
                    mIsCalibrated = true;
                    if(null != mListener) mListener.recorderStateCallback(RecorderstateCallback.RecorderMessage.MSG_CALIBRATION_FINISHED, mSessionType);
                }
                if(null != mListener) mListener.recorderStateCallback(RecorderstateCallback.RecorderMessage.MSG_RECORDING_FINISHED, mSessionType);
                mSessionType = SessionType.TYPE_UNKNOWN;
                mCameraCaptureSession = null;
                mCaptureRequest = null;
            }

            @Override
            public void onCaptureSequenceAborted(@NonNull CameraCaptureSession session, int sequenceId) {
                super.onCaptureSequenceAborted(session, sequenceId);
            }

            @Override
            public void onCaptureBufferLost(@NonNull CameraCaptureSession session, @NonNull CaptureRequest request, @NonNull Surface target, long frameNumber) {
                super.onCaptureBufferLost(session, request, target, frameNumber);
            }

        };//CameraCaptureCallback

        initializeStripPhotoCamera(mActiveStripPhotoID, mSFCamSensitivity, mSFCamDiscardThreshold);

    }//init

    private void createCameraBackgroundThread(){
        if(null != mCameraBackgroundThread){
            mCameraBackgroundThread = new HandlerThread(mCameraBackgroundThreadName);
            mCameraBackgroundThread.start();
            mCameraBackgroundHandler = new Handler(mCameraBackgroundThread.getLooper());
        }
    }//createCameraBackgroundThread

    private void destroyCameraBackgroundThread(){
        if(null == mCameraBackgroundThread) return;
        mCameraBackgroundThread.quitSafely();
        mCameraBackgroundThread = null;
        mCameraBackgroundHandler = null;
    }//destroyCameraBackgroundThread


    public boolean openCamera(boolean BackCamera){
        String SelectedCameraID = null;

        // find suitable camera
        try{
            int Facing = (BackCamera) ? CameraCharacteristics.LENS_FACING_BACK : CameraCharacteristics.LENS_FACING_FRONT;
            for(String CameraID : mCameraMgr.getCameraIdList()){
                CameraCharacteristics Characteristics = mCameraMgr.getCameraCharacteristics(CameraID);
                if(Characteristics.get(CameraCharacteristics.LENS_FACING) == Facing){
                    SelectedCameraID = CameraID;
                    break; // now we have a suitable camera
                }
            }
        }catch(CameraAccessException e){
            e.printStackTrace();
        }

        // found suitable camera
        if(null == SelectedCameraID){
            Log.i("OpticalRecorder", "No suitable camera found");
            return false;
        }

        // retrieve available resolutions and FPS
        try{
            CameraCharacteristics Characteristics = mCameraMgr.getCameraCharacteristics(SelectedCameraID);
            StreamConfigurationMap SM = Characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            mCameraAvailableResolutions = SM.getOutputSizes(Allocation.class);
            mCameraAvailableFPSRanges = Characteristics.get(CameraCharacteristics.CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES);
        }catch(CameraAccessException e){
            e.printStackTrace();
        }

        if(null == mCameraBackgroundThread) createCameraBackgroundThread();

        // open selected camera
        // also starts capture session if camera was opened successfully
        try{
            if(ActivityCompat.checkSelfPermission(mActivity, Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) {
                mCameraMgr.openCamera(SelectedCameraID, mCameraStateCallback, mCameraBackgroundHandler);
            }
        }catch(CameraAccessException e){
            e.printStackTrace();
            return false;
        }

        return true;
    }//openCamera

    public boolean isCameraReady(){
        return (null != mActiveCamera);
    }

    public int fps(){
        return mCurrentFPS;
    }

    public int captureWidth(){
        return mCaptureWidth;
    }

    public int captureHeight(){
        return mCaptureHeight;
    }

    private Size selectOptimalCaptureSize(Size Res){
        if(Res.getWidth() < 0) Res = new Size(10000, 10000);

        Size Rval = mCameraAvailableResolutions[0];
        int Deviation =  Math.abs(Res.getWidth() - Rval.getWidth());
        for(Size i: mCameraAvailableResolutions){
            int temp = Math.abs(Res.getWidth() - i.getWidth());
            if(temp < Deviation){
                Rval = i;
                Deviation = temp;
            }
        }
        return Rval;
    }

    public boolean isRecording(){
        return (mSessionType != SessionType.TYPE_UNKNOWN);
    }

    private Range<Integer> selectBestFPSRange(int FPS){
        Range<Integer> Rval = mCameraAvailableFPSRanges[0];
        if(FPS < 0) FPS = 60;
        int Deviation = Math.abs(Rval.getUpper() - FPS);
        for(Range<Integer> i: mCameraAvailableFPSRanges){
            int temp = Math.abs(i.getUpper() - FPS);
            if(temp < Deviation){
                Rval = i;
                Deviation = temp;
            }
        }//for[ranges]
        return Rval;
    }//selectBestFPSRange


    public void startCaptureSession(TextureView Target, SessionType SType, Size Resolution, int FPS) {
        if (null == mActiveCamera) return;

        // if camera is already running we just need to start capturing data and calibrate when time is right
        if(SType == SessionType.TYPE_CALIBRATION && mSessionType == SessionType.TYPE_PREVIEW){
            mCalibrationStarted = SystemClock.uptimeMillis();
            mSessionType = SessionType.TYPE_CALIBRATION;
            return;
        }else if(SType == SessionType.TYPE_CALIBRATION || SType == SessionType.TYPE_PREVIEW){
            Resolution = selectOptimalCaptureSize(Resolution);
            mCaptureFPSRange = selectBestFPSRange(FPS);
            mCaptureWidth = Resolution.getWidth();
            mCaptureHeight = Resolution.getHeight();
        }

        Resolution = new Size(mCaptureWidth, mCaptureHeight);

        Log.i("OpticalRecorder", "Staring capturing session with resolution " + Integer.toString(Resolution.getWidth()) + ":" + Integer.toString(Resolution.getHeight())
                + "@ " + Integer.toString(mCaptureFPSRange.getLower()) + " - " + Integer.toString(mCaptureFPSRange.getUpper()) + " FPS");

        mCameraTexView = Target;
        mCameraTexView.setSurfaceTextureListener(mTVListener);
        SurfaceTexture SurfaceTex = mCameraTexView.getSurfaceTexture();
        if(null == SurfaceTex){
            Log.e("OpticalRecorder", "Camera texture view has no surface!");
            return;
        }
        SurfaceTex.setDefaultBufferSize(Resolution.getWidth(), Resolution.getHeight());
        Surface TargetSurf = new Surface(SurfaceTex);
        mCameraTargetSurface = TargetSurf;
        mCaptureCompleted = false;

        try {
            mSessionType = SType;
            CaptureRequest.Builder CaptureRequestBuilder = mActiveCamera.createCaptureRequest(mActiveCamera.TEMPLATE_PREVIEW);
            CaptureRequestBuilder.set(CaptureRequest.CONTROL_MODE, CaptureRequest.CONTROL_MODE_AUTO);
            CaptureRequestBuilder.set(CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE, mCaptureFPSRange);
            CaptureRequestBuilder.addTarget(TargetSurf);

            mActiveCamera.createCaptureSession(Collections.singletonList(TargetSurf), new CameraCaptureSession.StateCallback() {
                @Override
                public void onConfigured(@NonNull CameraCaptureSession session) {
                    mCaptureRequest = CaptureRequestBuilder.build();
                    mCameraCaptureSession = session;

                    startRecording(mActiveStripPhotoID, SystemClock.uptimeMillis());

                    try{
                        mCameraCaptureSession.setRepeatingRequest(mCaptureRequest, mCameraCaptureCallback, mCameraBackgroundHandler);
                        if(mSessionType == SessionType.TYPE_CALIBRATION){
                            mCalibrationStarted = SystemClock.uptimeMillis();
                        }
                        if(null != mListener) mListener.recorderStateCallback(RecorderstateCallback.RecorderMessage.MSG_RECORDING_STARTED, mSessionType);
                    }catch(CameraAccessException e){
                        e.printStackTrace();
                    }
                }
                @Override
                public void onConfigureFailed(@NonNull CameraCaptureSession session) {
                    Log.e("OpticalRecorder", "Starting camera capture session failed!");
                    mSessionType = SessionType.TYPE_UNKNOWN;
                }
            }, mCameraBackgroundHandler);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }//startCaptureSession

    private Surface mCameraTargetSurface = null;

    public void stopCaptureSession(){
        if(mCameraCaptureSession != null){
            try{
                mCameraCaptureSession.stopRepeating();
                mCameraCaptureSession = null;
                mCaptureCompleted = true;
            }catch(CameraAccessException e){
                e.printStackTrace();
            }
        }
    }

    public void buildFinishPhoto(){
        buildStripPhoto(mActiveStripPhotoID);
    }//buildFinishPhoto

    public Bitmap retrieveFinishPhoto(){
        int[] ColorData = retrieveStripPhotoColorData(mActiveStripPhotoID);
        Size Dims = retrieveFinishPhotoDimensions();

        if(ColorData == null) return Bitmap.createBitmap(800, 600, Bitmap.Config.ARGB_8888);

        return Bitmap.createBitmap(ColorData, Dims.getWidth(), Dims.getHeight(), Bitmap.Config.ARGB_8888);
    }//retrieveFinishPhoto

    public int[] retrieveFinishPhotoTimestamps(){
        return retrieveStripPhotoTimestamps(mActiveStripPhotoID);
    }

    public Size retrieveFinishPhotoDimensions(){
        int[] Dims = retrieveStripPhotoDimensions(mActiveStripPhotoID);
        return new Size(Dims[0], Dims[1]);
    }

    public boolean isCalibrated(){
        return mIsCalibrated;
    }

    private native int createStripPhotoCamera();
    private native void releaseStripPhotoCamera(int CameraID);
    private native void initializeStripPhotoCamera(int CameraID, float Sensitivity, int DiscardThreshold);
    private native void startRecording(int CameraID, long Timestamp);
    /**
     *
     * @param CameraID
     * @param ColorData
     * @param Width
     * @param Height
     * @param Timestamp
     * @return True if light barrier was triggered, false otherwise.
     */
    private native void addStrip(int CameraID, int[] ColorData, int Width, int Height, long Timestamp);
    private native void calibrateWithExistingData(int CameraID);
    private native void buildStripPhoto(int CameraID);

    private native int[] retrieveStripPhotoDimensions(int CameraID);
    private native int[] retrieveStripPhotoColorData(int CameraID);
    private native int[] retrieveStripPhotoTimestamps(int CameraID);



}//OpticalRecorder
