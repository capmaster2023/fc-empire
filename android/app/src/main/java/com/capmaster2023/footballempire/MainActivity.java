package com.capmaster2023.footballempire;

import android.os.Bundle;
import com.getcapacitor.BridgeActivity;

public class MainActivity extends BridgeActivity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        registerPlugin(DatDbPlugin.class);
        registerPlugin(MatchEnginePlugin.class);
        super.onCreate(savedInstanceState);
    }
}
