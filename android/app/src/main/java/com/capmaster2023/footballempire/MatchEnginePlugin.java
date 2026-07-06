package com.capmaster2023.footballempire;

import android.util.Log;

import com.getcapacitor.JSObject;
import com.getcapacitor.Plugin;
import com.getcapacitor.PluginCall;
import com.getcapacitor.PluginMethod;
import com.getcapacitor.annotation.CapacitorPlugin;

import org.json.JSONObject;

@CapacitorPlugin(name = "MatchEngine")
public class MatchEnginePlugin extends Plugin {
    private static final String TAG = "MatchEnginePlugin";

    static {
        try {
            System.loadLibrary("datdb");
        } catch (Throwable t) {
            Log.e(TAG, "Impossible de charger libdatdb.so pour le moteur de match", t);
        }
    }

    private native String nativeStartMatch(String matchDataJson);
    private native String nativeTick(double deltaMinutes);
    private native String nativePause();
    private native String nativeResume();
    private native String nativeSetSpeed(double speed);
    private native String nativeUpdateTactics(String teamId, String tacticsJson);
    private native String nativeGetMatchState();
    private native String nativeStopMatch();

    @PluginMethod
    public void startMatch(PluginCall call) {
        try {
            JSObject data = call.getData();
            Object raw = data.has("matchData") ? data.opt("matchData") : data;
            String json = raw == null ? "{}" : raw.toString();
            resolveState(call, nativeStartMatch(json));
        } catch (Throwable t) {
            resolveError(call, t);
        }
    }

    @PluginMethod
    public void tick(PluginCall call) {
        try {
            double deltaTime = call.getDouble("deltaTime", 0.016);
            resolveState(call, nativeTick(deltaTime));
        } catch (Throwable t) {
            resolveError(call, t);
        }
    }

    @PluginMethod public void pause(PluginCall call) { try { resolveState(call, nativePause()); } catch (Throwable t) { resolveError(call, t); } }
    @PluginMethod public void resume(PluginCall call) { try { resolveState(call, nativeResume()); } catch (Throwable t) { resolveError(call, t); } }

    @PluginMethod
    public void setSpeed(PluginCall call) {
        try {
            double speed = call.getDouble("speed", 1.0);
            resolveState(call, nativeSetSpeed(speed));
        } catch (Throwable t) {
            resolveError(call, t);
        }
    }

    @PluginMethod
    public void updateTactics(PluginCall call) {
        try {
            String teamId = call.getString("teamId", "home");
            JSObject data = call.getData();
            Object raw = data.has("tactics") ? data.opt("tactics") : new JSObject();
            resolveState(call, nativeUpdateTactics(teamId, raw == null ? "{}" : raw.toString()));
        } catch (Throwable t) {
            resolveError(call, t);
        }
    }

    @PluginMethod public void getMatchState(PluginCall call) { try { resolveState(call, nativeGetMatchState()); } catch (Throwable t) { resolveError(call, t); } }
    @PluginMethod public void stopMatch(PluginCall call) { try { resolveState(call, nativeStopMatch()); } catch (Throwable t) { resolveError(call, t); } }

    private void resolveState(PluginCall call, String json) throws Exception {
        JSObject ret = new JSObject();
        ret.put("ok", true);
        ret.put("state", new JSONObject(json == null || json.isEmpty() ? "{}" : json));
        call.resolve(ret);
    }

    private void resolveError(PluginCall call, Throwable t) {
        Log.e(TAG, "Match engine call failed", t);
        JSObject ret = new JSObject();
        ret.put("ok", false);
        ret.put("error", t.getMessage());
        call.resolve(ret);
    }
}
