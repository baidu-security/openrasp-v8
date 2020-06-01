package com.baidu.openrasp.v8;

import java.util.*;
import com.jsoniter.output.JsonStream;

public class ContextImpl extends Context {

  public String getString(String key) {
    if (key.equals("path"))
      return getPath();
    if (key.equals("method"))
      return getMethod();
    if (key.equals("url"))
      return getUrl();
    if (key.equals("querystring"))
      return getQuerystring();
    if (key.equals("appBasePath"))
      return getAppBasePath();
    if (key.equals("protocol"))
      return getProtocol();
    if (key.equals("remoteAddr"))
      return getRemoteAddr();
    if (key.equals("requestId"))
      return getRequestId();
    return "";
  }

  public byte[] getObject(String key) {
    if (key.equals("json"))
      return getJson();
    if (key.equals("header"))
      return getHeader();
    if (key.equals("parameter"))
      return getParameter();
    if (key.equals("server"))
      return getServer();
    return "{}".getBytes();
  }

  public byte[] getBuffer(String key) {
    if (key.equals("body"))
      return getBody();
    return "{}".getBytes();
  }

  public String getPath() {
    return "test 中文 & 😊";
  }

  public String getMethod() {
    return "test 中文 & 😊";
  }

  public String getUrl() {
    return "test 中文 & 😊";
  }

  public String getQuerystring() {
    return "test 中文 & 😊";
  }

  public String getAppBasePath() {
    return "test 中文 & 😊";
  }

  public String getProtocol() {
    return "test 中文 & 😊";
  }

  public String getRemoteAddr() {
    return "test 中文 & 😊";
  }

  public String getRequestId() {
    return "";
  }

  public byte[] getBody() {
    ByteArrayOutputStream data = new ByteArrayOutputStream();
    data.write((byte) 0);
    data.write((byte) 1);
    data.write((byte) 2);
    data.write((byte) 3);
    return data.toByteArray();
  }

  public byte[] getJson() {
    List<String> list = new ArrayList<String>();
    list.add("test 中文 & 😊");
    ByteArrayOutputStream data = new ByteArrayOutputStream();
    JsonStream.serialize(list, data);
    data.write(0);
    return data.getByteArray();
  }

  public byte[] getHeader() {
    List<String> list = new ArrayList<String>();
    list.add("test 中文 & 😊");
    ByteArrayOutputStream data = new ByteArrayOutputStream();
    JsonStream.serialize(list, data);
    data.write(0);
    return data.getByteArray();
  }

  public byte[] getParameter() {
    List<String> list = new ArrayList<String>();
    list.add("test 中文 & 😊");
    ByteArrayOutputStream data = new ByteArrayOutputStream();
    JsonStream.serialize(list, data);
    data.write(0);
    return data.getByteArray();
  }

  public byte[] getServer() {
    List<String> list = new ArrayList<String>();
    list.add("test 中文 & 😊");
    ByteArrayOutputStream data = new ByteArrayOutputStream();
    JsonStream.serialize(list, data);
    data.write(0);
    return data.getByteArray();
  }

}