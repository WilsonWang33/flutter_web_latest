package com.smwl.smsdk.app;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.lang.ref.WeakReference;

/**
 * MyNativeCrashHandler.java
 */
public class MyNativeCrashHandler {

    private static final String CRASH_FILE_NAME = "native_crash_info.txt";

    static {
        System.loadLibrary("nativecatch");  // 加载 Native 库
    }

    static WeakReference<Context> contextWeakReference;

    // Native 初始化方法（后续在 Native 层实现）
    public static native void initNativeCrashHandler(String filePath);
    public static native void triggerCrash();

    // Native 崩溃时的回调方法（后续在 Native 层调用）
    public static void onNativeCrash(String errorMsg) {
        Log.e("NativeCrash", "崩溃信息: " + errorMsg);
        // 将 errorMsg 保存到文件或上报服务器
        if(contextWeakReference!=null&&contextWeakReference.get()!=null){
            SharedPreferences sp = contextWeakReference.get().getSharedPreferences("crash", Context.MODE_PRIVATE);
            sp.edit().putBoolean("SP_SDK_OCCUR_CRASH", false).commit();
            sp.edit().putString("err_msg", errorMsg).commit();
            /*File crashFile = new File(contextWeakReference.get().getFilesDir(), CRASH_FILE_NAME);
            try {
                crashFile.createNewFile();
                try (BufferedReader reader = new BufferedReader(new FileReader(crashFile))) {
                    String crashInfo = reader.readLine();
                    if (crashInfo == null) {
                        SharedPreferences sp = contextWeakReference.get().getSharedPreferences("crash", Context.MODE_PRIVATE);
                        sp.edit().putString("crash", errorMsg).apply();
                    }
                }
            } catch (IOException e) {
                Log.e("NativeCrash", "写入崩溃文件失败", e);
            }*/
        }
    }

    // 初始化方法（在 Application 中调用）
    public static void init(Context context) {
        // 获取应用私有目录路径
        File crashFile = new File(context.getFilesDir(), CRASH_FILE_NAME);
        initNativeCrashHandler(crashFile.getAbsolutePath());
        contextWeakReference = new WeakReference<>(context);
    }

    // 检查并上报崩溃信息
    public static void checkAndReportCrash(Context context) {
        File crashFile = new File(context.getFilesDir(), CRASH_FILE_NAME);
        if (crashFile.exists()) {
            try (BufferedReader reader = new BufferedReader(new FileReader(crashFile))) {
                String crashInfo = reader.readLine();
                if (crashInfo != null) {
                    Log.e("NativeCrash", "上次崩溃信息: " + crashInfo);
//                    reportCrashToServer(crashInfo);
                }
                crashFile.delete();
            } catch (IOException e) {
                Log.e("NativeCrash", "读取崩溃文件失败", e);
            }
        }
    }
}