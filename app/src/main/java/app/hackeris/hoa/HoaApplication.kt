package app.hackeris.hoa

import android.util.Log
import ohos.stage.ability.adapter.StageApplication

class HoaApplication : StageApplication() {
    override fun onCreate() {
        Log.i(TAG, "========== HOA Application onCreate START ==========")
        Log.i(TAG, "Process: ${android.os.Process.myPid()}")
        try {
            super.onCreate()
            Log.i(TAG, "StageApplication.onCreate() completed successfully")
        } catch (e: UnsatisfiedLinkError) {
            Log.e(TAG, "FATAL: Failed to load native library", e)
            Log.e(TAG, "  Message: ${e.message}")
            Log.e(TAG, "  Check: Are .so files in lib/arm64-v8a/? Is extractNativeLibs=true?")
        } catch (e: Exception) {
            Log.e(TAG, "FATAL: StageApplication.onCreate() failed", e)
        }
        Log.i(TAG, "========== HOA Application onCreate END ==========")
    }

    companion object {
        private const val TAG = "HOA.App"
    }
}
