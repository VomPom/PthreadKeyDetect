package com.vompom.pthread_available

import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import com.vompom.pthread_key_detect.PthreadKeyDetect
import com.vompom.pthread_key_detect.PthreadKeyDetect.PthreadKeyCallback

class MainActivity : AppCompatActivity() {
    companion object {
        private const val TAG = "PthreadKeyDetect"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        init()
    }

    private fun init() {
        PthreadKeyDetect.hook()
        PthreadKeyDetect.setPthreadKeyCallback(object : PthreadKeyCallback {
            override fun onPthreadCreate(ret: Int, keyIndex: Int, soPath: String?, backtrace: String?) {
                log(ret, keyIndex, soPath, backtrace)
            }

            override fun onPthreadDelete(ret: Int, keyIndex: Int, soPath: String?, backtrace: String?) {
                log(ret, keyIndex, soPath, backtrace)
            }
        })
        findViewById<TextView>(R.id.tv_max).text = "PTHREAD_KEYS_MAX: ${PthreadKeyDetect.max()}"
        findViewById<Button>(R.id.btn_refresh).setOnClickListener { refresh() }
        findViewById<Button>(R.id.btn_full).setOnClickListener {
            PthreadKeyDetect.useUp()
            refresh()
        }
        refresh()
    }

    private fun log(ret: Int, keyIndex: Int, soPath: String?, backtrace: String?) {
        Log.e(TAG, " ret:$ret keySeq:$keyIndex soPath:$soPath backtrace:\n$backtrace")
    }

    private fun refresh() {
        findViewById<TextView>(R.id.tv_available).text = "AVAILABLE: ${PthreadKeyDetect.available()}"
    }
}