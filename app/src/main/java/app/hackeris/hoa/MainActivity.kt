package app.hackeris.hoa

import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        Log.i(TAG, "========== HOA MainActivity START ==========")
        Log.i(TAG, "Device: ${android.os.Build.MODEL}, SDK: ${android.os.Build.VERSION.SDK_INT}")
        Log.i(TAG, "ABI: ${android.os.Build.SUPPORTED_ABIS.toList()}")

        // Check native libs availability
        val nativeLibs = listOf(
            "libarkui_android.so",
            "libhilog.so",
            "libarkui_componentsnapshot.so",
            "libarkui_focuscontroller.so"
        )
        val nativeLibDir = applicationInfo.nativeLibraryDir
        Log.i(TAG, "nativeLibraryDir: $nativeLibDir")
        for (lib in nativeLibs) {
            val exists = java.io.File(nativeLibDir, lib).exists()
            Log.i(TAG, "  $lib: ${if (exists) "FOUND" else "MISSING"}")
        }

        // Check assets
        val assetItems = listOf(
            "arkui-x/entry/ets/modules.abc",
            "arkui-x/entry/module.json",
            "arkui-x/entry/resources.index"
        )
        Log.i(TAG, "Asset check:")
        for (item in assetItems) {
            val exists = try { assets.open(item).close(); true } catch (_: Exception) { false }
            Log.i(TAG, "  $item: ${if (exists) "FOUND" else "MISSING"}")
        }

        val statusText = findViewById<TextView>(R.id.status_text)
        val launchButton = findViewById<Button>(R.id.launch_button)

        val sb = StringBuilder()
        sb.appendLine("HOA — HarmonyOS on Android")
        sb.appendLine()
        sb.appendLine("Device: ${android.os.Build.MODEL}")
        sb.appendLine("Android SDK: ${android.os.Build.VERSION.SDK_INT}")
        sb.appendLine("ABI: ${android.os.Build.SUPPORTED_ABIS.firstOrNull()}")
        sb.appendLine()

        // Build runtime status summary
        val allLibsPresent = nativeLibs.all { lib ->
            java.io.File(nativeLibDir, lib).exists()
        }
        val allAssetsPresent = assetItems.all { item ->
            try { assets.open(item).close(); true } catch (_: Exception) { false }
        }

        sb.appendLine("Runtime: ${if (allLibsPresent) "OK" else "INCOMPLETE"}")
        sb.appendLine("Bytecode: ${if (allAssetsPresent) "OK" else "INCOMPLETE"}")
        sb.appendLine(".abc version: 12.0.6.0")
        sb.appendLine()
        sb.appendLine("Tap 'Launch HAP' to start ArkUI sample.")

        statusText.text = sb.toString()

        launchButton.setOnClickListener {
            Log.i(TAG, "Launching HoaAbilityActivity")
            val intent = Intent(this, HoaAbilityActivity::class.java).apply {
                putExtra("BUNDLE_NAME", "com.example.enjoyarkuix")
                putExtra("MODULE_NAME", "entry")
                putExtra("ABILITY_NAME", "DynamicHapAbility")
            }
            startActivity(intent)
        }

        Log.i(TAG, "========== HOA MainActivity END ==========")
    }

    companion object {
        private const val TAG = "HOA.Main"
    }
}
