package com.riwutz.debugmasklab;

import android.app.Activity;
import android.os.Bundle;
import android.provider.Settings;
import android.graphics.Typeface;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.lang.reflect.Method;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

public class MainActivity extends Activity {
    private TextView output;

    private static final String[] PROPS = new String[] {
            "sys.usb.config",
            "persist.sys.usb.config",
            "sys.usb.state",
            "init.svc.adbd",
            "sys.usb.ffs.ready",
            "sys.usb.ffs.adb.ready",
            "service.adb.tcp.port"
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        int pad = dp(14);
        root.setPadding(pad, pad, pad, pad);

        TextView title = new TextView(this);
        title.setText("RIWUTZ DebugMask Tester");
        title.setTextSize(20f);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        root.addView(title);

        TextView hint = new TextView(this);
        hint.setText("Package: com.riwutz.debugmasklab\nBandingkan IN-PROCESS vs GLOBAL. ADB PC harus tetap aktif.");
        hint.setTextSize(13f);
        hint.setPadding(0, dp(6), 0, dp(10));
        root.addView(hint);

        Button refresh = new Button(this);
        refresh.setText("REFRESH TEST");
        root.addView(refresh);

        ScrollView scroll = new ScrollView(this);
        output = new TextView(this);
        output.setTextSize(12f);
        output.setTypeface(Typeface.MONOSPACE);
        output.setTextIsSelectable(true);
        output.setPadding(0, dp(10), 0, dp(20));
        scroll.addView(output);
        root.addView(scroll, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                0,
                1f));

        refresh.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                render();
            }
        });

        setContentView(root);
        render();
    }

    private void render() {
        StringBuilder sb = new StringBuilder();
        sb.append("=== RIWUTZ DEBUGMASK TESTER ===\n");
        sb.append("Time: ").append(new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.US).format(new Date())).append('\n');
        sb.append("SDK: ").append(android.os.Build.VERSION.SDK_INT).append('\n');
        sb.append("Release: ").append(android.os.Build.VERSION.RELEASE).append("\n\n");

        sb.append("=== IN-PROCESS SystemProperties ===\n");
        boolean reflectionOk = true;
        String inConfig = null;
        String inAdbd = null;
        for (String key : PROPS) {
            String value = getSystemPropertyInProcess(key);
            if (value.startsWith("<reflection failed:")) reflectionOk = false;
            if ("sys.usb.config".equals(key)) inConfig = value;
            if ("init.svc.adbd".equals(key)) inAdbd = value;
            sb.append(key).append('=').append(value).append('\n');
        }

        sb.append("\n=== GLOBAL getprop subprocess ===\n");
        String globalConfig = null;
        String globalAdbd = null;
        for (String key : PROPS) {
            String value = getGlobalProperty(key);
            if ("sys.usb.config".equals(key)) globalConfig = value;
            if ("init.svc.adbd".equals(key)) globalAdbd = value;
            sb.append(key).append('=').append(value).append('\n');
        }

        sb.append("\n=== Settings.Global (real provider state) ===\n");
        sb.append("adb_enabled=").append(getGlobalSetting("adb_enabled")).append('\n');
        sb.append("development_settings_enabled=").append(getGlobalSetting("development_settings_enabled")).append('\n');
        sb.append("adb_wifi_enabled=").append(getGlobalSetting("adb_wifi_enabled")).append('\n');

        sb.append("\n=== RESULT ===\n");
        if (!reflectionOk) {
            sb.append("IN-PROCESS reflection unavailable on this ROM.\n");
            sb.append("Modul belum bisa dinilai dari APK tester ini.\n");
        } else {
            boolean globalAdbAlive = containsAdb(globalConfig) || "running".equals(globalAdbd);
            boolean processLooksMasked = !containsAdb(inConfig) && "stopped".equals(inAdbd);
            if (globalAdbAlive && processLooksMasked) {
                sb.append("MASK ACTIVE: YES\n");
                sb.append("Per-app view berbeda dari kondisi global.\n");
                sb.append("ADB global masih aktif.\n");
            } else {
                sb.append("MASK ACTIVE: NOT CONFIRMED\n");
                sb.append("Periksa log module setelah membuka tester.\n");
            }
        }

        sb.append("\nCatatan: module LAB saat ini hanya memask SystemProperties.\n");
        sb.append("Settings.Global sengaja tetap menampilkan kondisi provider sebenarnya.\n");

        output.setText(sb.toString());
    }

    private String getSystemPropertyInProcess(String key) {
        try {
            Class<?> cls = Class.forName("android.os.SystemProperties");
            Method method = cls.getDeclaredMethod("get", String.class, String.class);
            method.setAccessible(true);
            Object value = method.invoke(null, key, "");
            return value == null ? "" : String.valueOf(value);
        } catch (Throwable t) {
            Throwable cause = t.getCause() != null ? t.getCause() : t;
            return "<reflection failed:" + cause.getClass().getSimpleName() + ">";
        }
    }

    private String getGlobalProperty(String key) {
        Process process = null;
        try {
            process = new ProcessBuilder("/system/bin/getprop", key)
                    .redirectErrorStream(true)
                    .start();
            BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
            String line = reader.readLine();
            process.waitFor();
            return line == null ? "" : line.trim();
        } catch (Throwable t) {
            return "<getprop failed:" + t.getClass().getSimpleName() + ">";
        } finally {
            if (process != null) process.destroy();
        }
    }

    private String getGlobalSetting(String key) {
        try {
            int value = Settings.Global.getInt(getContentResolver(), key, -999);
            return value == -999 ? "<unavailable>" : String.valueOf(value);
        } catch (Throwable t) {
            return "<failed:" + t.getClass().getSimpleName() + ">";
        }
    }

    private boolean containsAdb(String value) {
        if (value == null) return false;
        String[] parts = value.split(",");
        for (String part : parts) {
            if ("adb".equals(part.trim())) return true;
        }
        return false;
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }
}
