package com.baidu.openrasp.v8;

public interface Context {

    public String getPath();

    public String getMethod();

    public String getUrl();

    public String getQuerystring();

    public String getAppBasePath();

    public String getProtocol();

    public String getRemoteAddr();

    public String getRequestId();

    public byte[] getBody(int[] size);

    public byte[] getJson(int[] size);

    public byte[] getHeader(int[] size);

    public byte[] getParameter(int[] size);

    public byte[] getServer(int[] size);

}