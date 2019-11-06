package org.sample;

import java.util.*;
import com.jsoniter.output.JsonStream;
import org.apache.commons.lang3.RandomStringUtils;
import com.baidu.openrasp.v8.ByteArrayOutputStream;

public class Params {

  private static Random random = new Random();

  private static String[] types = { "sql", "command", "readFile", "writeFile", "directory" };

  public static String GetType() {
    return types[random.nextInt(types.length)];
  }

  public static byte[] GetParams(String type) {
    HashMap<String, Object> obj = new HashMap<String, Object>();
    if (type.equals("sql")) {
      obj.put("query", RandomStringUtils.randomAscii(1, 1024 * 4));
      obj.put("server", "mysql");
    } else if (type.equals("command")) {
      obj.put("command", RandomStringUtils.randomAscii(1, 1024 * 1024 * 4));
    } else if (type.equals("readFile") || type.equals("writeFile") || type.equals("directory")) {
      obj.put("path", RandomStringUtils.randomAscii(1, 1024 * 4));
      obj.put("realpath", RandomStringUtils.randomAscii(1, 1024 * 4));
    }
    ByteArrayOutputStream data = new ByteArrayOutputStream();
    JsonStream.serialize(obj, data);
    data.write(0);
    return data.getByteArray();
  }

}