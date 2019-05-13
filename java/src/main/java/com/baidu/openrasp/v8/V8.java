package com.baidu.openrasp.v8;

import org.scijava.nativelib.NativeLoader;

public class V8 {

    public static Logger logger = null;

    public synchronized static native boolean Initialize();

    public synchronized static native boolean Dispose();

    public synchronized static native boolean CreateSnapshot(String config, Object[] plugins);

    public static native String Check(String type, byte[] params, int params_size, Context context,
            boolean new_request);

    public static native String ExecuteScript(String source, String filename) throws Exception;

    static {
        if (System.getProperty("java.vm.name").contains("JRockit")) {
            System.err.println("OpenRASP does not support Oracle JRockit JDK! Please contact us for more information.");
            System.exit(1);
        }
        try {
            NativeLoader.loadLibrary("openrasp_v8_java");
        } catch (Exception e) {
            System.err.println(e);
            System.exit(1);
        }
    }

    public static void PluginLog(String msg) {
        if (logger != null) {
            logger.log(msg.replaceAll("\n$", ""));
        }
    }

    public static void SetPluginLogger(Logger logger) {
        V8.logger = logger;
    }
}
