package com.baidu.openrasp.v8;
import java.util.*;
import com.jsoniter.spi.JsoniterSpi;
import com.jsoniter.extra.Base64Support;
import com.jsoniter.output.JsonStream;
import com.jsoniter.JsonIterator;
import com.jsoniter.any.Any;
public class ContextImpl implements Context {

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

  public byte[] getBody(int[] size) {
    List<String> list = new ArrayList<String>();
    list.add("test 中文 & 😊");
    ByteArrayOutputStream data = new ByteArrayOutputStream();
    JsonStream.serialize(list, data);
    size[0] = data.size();
    return data.getByteArray();
  }

  public byte[] getJson(int[] size) {
    List<String> list = new ArrayList<String>();
    list.add("test 中文 & 😊");
    ByteArrayOutputStream data = new ByteArrayOutputStream();
    JsonStream.serialize(list, data);
    size[0] = data.size();
    return data.getByteArray();
  }

  public byte[] getHeader(int[] size) {
    List<String> list = new ArrayList<String>();
    list.add("test 中文 & 😊");
    ByteArrayOutputStream data = new ByteArrayOutputStream();
    JsonStream.serialize(list, data);
    size[0] = data.size();
    return data.getByteArray();
  }

  public byte[] getParameter(int[] size) {
    List<String> list = new ArrayList<String>();
    list.add("test 中文 & 😊");
    ByteArrayOutputStream data = new ByteArrayOutputStream();
    JsonStream.serialize(list, data);
    size[0] = data.size();
    return data.getByteArray();
  }

  public byte[] getServer(int[] size) {
    List<String> list = new ArrayList<String>();
    list.add("test 中文 & 😊");
    ByteArrayOutputStream data = new ByteArrayOutputStream();
    JsonStream.serialize(list, data);
    size[0] = data.size();
    return data.getByteArray();
  }
}