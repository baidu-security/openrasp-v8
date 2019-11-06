package org.sample;

import java.util.*;
import com.jsoniter.output.JsonStream;
import org.apache.commons.lang3.RandomStringUtils;
import com.baidu.openrasp.v8.ByteArrayOutputStream;

public class Context extends com.baidu.openrasp.v8.Context {

  public String getString(String key) {
    return RandomStringUtils.randomAscii(1, 1024 * 4);
  }

  public byte[] getObject(String key) {
    HashMap<String, Object> obj = new HashMap<String, Object>();
    obj.put("os", "Linux");
    int rand = new Random().nextInt(10);
    for (int i = 0; i < rand; i++) {
      obj.put(RandomStringUtils.randomAlphanumeric(2, 10), RandomStringUtils.randomAscii(0, 1024 * 4));
      obj.put(RandomStringUtils.randomAlphanumeric(2, 10), Math.random());
    }
    ByteArrayOutputStream data = new ByteArrayOutputStream();
    JsonStream.serialize(obj, data);
    data.write(0);
    return data.getByteArray();
  }

  public byte[] getBuffer(String key) {
    return getObject(key);
  }

}