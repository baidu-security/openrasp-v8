package com.baidu.openrasp.v8;

public class CrashReporter {
    public synchronized static native void install(String url, String appid, String appSecret, String raspid);
}
