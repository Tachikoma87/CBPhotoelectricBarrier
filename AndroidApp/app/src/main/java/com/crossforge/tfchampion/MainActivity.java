package com.crossforge.tfchampion;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //Intent toSprintIntent = new Intent(MainActivity.this, SprintMain.class);
        //startActivity(toSprintIntent);

        //Intent toFlyingRunIntent = new Intent(MainActivity.this, FlyingRunMain.class);
        //startActivity(toFlyingRunIntent);
    }//onCreate

    public void onBtnSprintClicked(View v){
        Intent toSprintIntent = new Intent(MainActivity.this, SprintMain.class);
        startActivity(toSprintIntent);
    }

    public void onBtnFlyingRunClicked(View v){
        Intent toFlyingRunIntent = new Intent(MainActivity.this, FlyingRunMain.class);
        startActivity(toFlyingRunIntent);
    }

}