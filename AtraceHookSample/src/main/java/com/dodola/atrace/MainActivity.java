package com.dodola.atrace;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.Toast;

public class MainActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        findViewById(R.id.button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                boolean b = Atrace.hasHacks();
                if (b) {
                    Atrace.enableSystrace();
                    Toast.makeText(MainActivity.this, "开启成功", Toast.LENGTH_SHORT).show();
                }
            }
        });
    }
}
