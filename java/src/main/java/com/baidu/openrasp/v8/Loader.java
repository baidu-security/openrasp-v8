package com.baidu.openrasp.v8;

import com.baidu.openrasp.nativelib.NativeLoader;

public class Loader {
    private static boolean isLoad = false;

    public synchronized static void load() throws Exception {
        if (isLoad) {
            return;
        }
        NativeLoader.loadLibrary("openrasp_v8_java");
        isLoad = true;
    }
}
