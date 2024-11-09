package com.vompom.pthread_key_detect;

import android.os.Build;
import android.os.Looper;

import androidx.annotation.RequiresApi;

public final class PthreadKeyDetect {
    private static PthreadKeyCallback sPthreadKeyCallback;

    static {
        System.loadLibrary("pthread_key_detect");
    }

    /**
     * Start hook native 'pthread_key_delete' and 'pthread_key_delete' functions.
     */
    public static native void hook();

    /**
     * Native level functions of 'pthread_key_delete' and 'pthread_key_delete' will be used
     * to create the maximum number of keys for detection.
     *
     * @return the maximum pthread_can_t count that the app's current process can be created in current status.
     */
    public static native int available();

    /**
     * A delayed way to get the available count, it will be executed when the APP is stable .
     *
     * @param callback same like {@link PthreadKeyDetect#available}.
     */
    @RequiresApi(api = Build.VERSION_CODES.M)
    public static void available(PthreadKeyCountCallback callback) {
        Looper.getMainLooper().getQueue().addIdleHandler(() -> {
            callback.available(available());
            return false;
        });
    }

    /**
     * Returns the maximum pthread_key_t count that the device supports creating
     * which define in bionic/libc/include/limits.h
     *
     * @return PTHREAD_KEYS_MAX
     */
    public static native int max();

    /**
     * Use up all pthread_key_t, can test extreme cases, how your app works
     */
    public static native void useUp();

    /**
     * When native c++ call pthread_key_create or pthread_key_delete,
     * there will be a callback.
     *
     * @param type      0-> pthread_key_create 1-> pthread_key_delete.
     * @param ret       the result of the function call.
     * @param keyIndex  key's index.
     * @param soPath    which .so file call the above two functions.
     * @param backtrace call stack.
     */
    private static void pthreadKeyCallback(int type, int ret, int keyIndex, String soPath, String backtrace) {
        if (sPthreadKeyCallback != null) {
            if (type == 0) {
                sPthreadKeyCallback.onPthreadCreate(ret, keyIndex, soPath, backtrace);
            } else if (type == 1) {
                sPthreadKeyCallback.onPthreadDelete(ret, keyIndex, soPath, backtrace);
            }
        }
    }

    public static void setPthreadKeyCallback(PthreadKeyCallback callback) {
        sPthreadKeyCallback = callback;
    }

    public interface PthreadKeyCountCallback {
        void available(int count);
    }

    public interface PthreadKeyCallback {
        void onPthreadCreate(int ret, int keyIndex, String soPath, String backtrace);

        void onPthreadDelete(int ret, int keyIndex, String soPath, String backtrace);
    }
}