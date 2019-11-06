package com.baidu.openrasp.v8;

import org.scijava.nativelib.NativeLoader;

public class V8 {

    private static Logger logger = null;

    private static StackGetter stackGetter = null;

    private static boolean isLoad = false;

    public synchronized static native boolean Initialize(int isolate_pool_size);

    public synchronized static native boolean Dispose();

    public synchronized static native boolean CreateSnapshot(String config, Object[] plugins, String version);

    public static native byte[] Check(String type, byte[] params, int params_size, Context context, long free_memory,
            int timeout);

    public static native String ExecuteScript(String source, String filename) throws Exception;

    public synchronized static void Load() throws Exception {
        if (isLoad) {
            return;
        }
        NativeLoader.loadLibrary("openrasp_v8_java");
        isLoad = true;
    }

    public synchronized static boolean Initialize() {
        return Initialize(Runtime.getRuntime().availableProcessors());
    }

    public static byte[] Check(String type, byte[] params, int params_size, Context context, boolean new_request,
            int timeout) {
        return Check(type, params, params_size, context, -1, timeout);
    }

    public static void Log(String msg) {
        if (logger != null) {
            logger.log(msg.replaceAll("\n$", ""));
        }
    }

    public static void SetLogger(Logger logger) {
        V8.logger = logger;
    }

    public static byte[] GetStack() {
        return stackGetter != null ? stackGetter.get() : null;
    }

    public static void SetStackGetter(StackGetter stackGetter) {
        V8.stackGetter = stackGetter;
    }

    public static long GetFreeMemory() {
        return Runtime.getRuntime().freeMemory();
    }

    public static void Gc() {
        System.gc();
    }
}
