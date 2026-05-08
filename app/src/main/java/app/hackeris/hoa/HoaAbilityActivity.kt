package app.hackeris.hoa

import android.os.Bundle
import android.util.Log
import ohos.stage.ability.adapter.StageActivity

class HoaAbilityActivity : StageActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        val bundleName = intent.getStringExtra("BUNDLE_NAME") ?: "com.example.enjoyarkuix"
        val moduleName = intent.getStringExtra("MODULE_NAME") ?: "entry"
        val abilityName = intent.getStringExtra("ABILITY_NAME") ?: "DynamicHapAbility"

        val instanceName = "$bundleName:$moduleName:$abilityName:"
        Log.i(TAG, "========== HoaAbilityActivity onCreate START ==========")
        Log.i(TAG, "bundleName=$bundleName, moduleName=$moduleName, abilityName=$abilityName")
        Log.i(TAG, "instanceName=$instanceName")

        try {
            setInstanceName(instanceName)
            Log.i(TAG, "setInstanceName() OK")
        } catch (e: Exception) {
            Log.e(TAG, "setInstanceName() FAILED", e)
        }

        try {
            super.onCreate(savedInstanceState)
            Log.i(TAG, "super.onCreate() completed — ArkUI rendering surface should be created")
        } catch (e: UnsatisfiedLinkError) {
            Log.e(TAG, "FATAL: Native library link error during Activity onCreate", e)
        } catch (e: Exception) {
            Log.e(TAG, "ERROR: Activity onCreate failed", e)
        }

        Log.i(TAG, "========== HoaAbilityActivity onCreate END ==========")
    }

    override fun onResume() {
        Log.i(TAG, "onResume — UIAbility.onForeground() should fire")
        super.onResume()
    }

    override fun onPause() {
        Log.i(TAG, "onPause — UIAbility.onBackground() should fire")
        super.onPause()
    }

    override fun onDestroy() {
        Log.i(TAG, "onDestroy — UIAbility.onDestroy() should fire")
        super.onDestroy()
    }

    companion object {
        private const val TAG = "HOA.Ability"
    }
}
