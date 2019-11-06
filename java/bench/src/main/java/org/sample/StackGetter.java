package org.sample;

import java.util.*;
import com.jsoniter.output.JsonStream;
import org.apache.commons.lang3.RandomStringUtils;
import com.baidu.openrasp.v8.ByteArrayOutputStream;

public class StackGetter implements com.baidu.openrasp.v8.StackGetter {

  public byte[] get() {
    LinkedList<String> list = new LinkedList<String>();
    for (int i = 0; i < 10; i++) {
      list.add(RandomStringUtils.randomAscii(1, 1024 * 4));
    }
    ByteArrayOutputStream data = new ByteArrayOutputStream();
    JsonStream.serialize(list, data);
    data.write(0);
    return data.getByteArray();
  }

}