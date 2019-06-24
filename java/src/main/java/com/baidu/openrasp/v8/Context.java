package com.baidu.openrasp.v8;

public abstract class Context {

    public synchronized static native void setStringKeys(String[] keys);

    public synchronized static native void setObjectKeys(String[] keys);

    public synchronized static native void setBufferKeys(String[] keys);

    public abstract String getString(String key);

    public abstract byte[] getObject(String key);

    public abstract byte[] getBuffer(String key);

}