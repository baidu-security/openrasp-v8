package com.baidu.openrasp;

import static org.junit.Assert.*;
import org.junit.*;
import com.baidu.openrasp.v8.*;

public class NativePatchesTest {

  @BeforeClass
  public static void Initialize() throws Exception {
    V8.Load();
  }

  @Test
  public void GetNetworkInterfaces() {
    assertNotNull(NativePatches.GetNetworkInterfaces());
  }

}