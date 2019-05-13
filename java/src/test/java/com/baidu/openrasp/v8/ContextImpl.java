package com.baidu.openrasp.v8;

public class ContextImpl implements Context {

  public String getPath() {
    return "fine";
  }

  public String getMethod() {
    return "fine";
  }

  public String getUrl() {
    return "fine";
  }

  public String getQuerystring() {
    return "fine";
  }

  public String getAppBasePath() {
    return "fine";
  }

  public String getProtocol() {
    return "fine";
  }

  public String getRemoteAddr() {
    return "fine";
  }

  public byte[] getBody(int[] size) {
    String data = "[\"fine\"]";
    size[0] = data.getBytes().length;
    return data.getBytes();
  }

  public byte[] getJson(int[] size) {
    String data = "[\"fine\"]";
    size[0] = data.getBytes().length;
    return data.getBytes();
  }

  public byte[] getHeader(int[] size) {
    String data = "[\"fine\"]";
    size[0] = data.getBytes().length;
    return data.getBytes();
  }

  public byte[] getParameter(int[] size) {
    String data = "[\"fine\"]";
    size[0] = data.getBytes().length;
    return data.getBytes();
  }

  public byte[] getServer(int[] size) {
    String data = "[\"fine\"]";
    size[0] = data.getBytes().length;
    return data.getBytes();
  }
}