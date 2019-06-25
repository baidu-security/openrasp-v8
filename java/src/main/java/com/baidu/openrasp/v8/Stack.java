package com.baidu.openrasp.v8;

public abstract class Stack {

  private static Stack instance = null;

  public static void setInstance(Stack instance) {
    Stack.instance = instance;
  }

  public static byte[] getStack() {
    return Stack.instance != null ? Stack.instance.get() : null;
  }

  public abstract byte[] get();

}