package com.smwl.nativecatch;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;

import com.smwl.nativecatch.databinding.ActivityMainBinding;
import com.smwl.smsdk.app.MyNativeCrashHandler;

import java.io.File;
import java.io.FileReader;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'nativecatch' library on application startup.
    static {
        System.loadLibrary("nativecatch");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        TextView tv = binding.sampleText;
//        tv.setText(stringFromJNI());

        MyNativeCrashHandler.init(this);
        MyNativeCrashHandler.checkAndReportCrash(this);

        tv.setOnClickListener(view -> {
            MyNativeCrashHandler.triggerCrash();
        });
    }

}