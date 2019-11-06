package org.sample;

public class Logger implements com.baidu.openrasp.v8.Logger {
  public void log(String msg) {
    if (msg.length() > 1024) {
      msg = msg.substring(0, 256) + "..." + msg.substring(msg.length() - 256, msg.length());
    }
    System.out.println(msg);
  }
}