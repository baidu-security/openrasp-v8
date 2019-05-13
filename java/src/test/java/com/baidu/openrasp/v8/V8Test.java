package com.baidu.openrasp.v8;

import static org.junit.Assert.*;
import org.junit.*;
import java.util.*;

public class V8Test {

  @BeforeClass
  public static void Initialize() {
    assertTrue(V8.Initialize());
  }

  @AfterClass
  public static void Dispose() {
    assertTrue(V8.Dispose());
  }

  @Test
  public void InitializeAgain() {
    assertTrue(V8.Initialize());
  }

  @Test
  public void ExecuteScript() throws Exception {
    assertTrue(V8.CreateSnapshot("{}", new Object[0]));
    assertEquals(V8.ExecuteScript("23333", "6666"), "23333");
  }

  @Test
  public void CreateSnapshot() {
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js", "const plugin = new RASP('test')" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray()));
  }

  @Test
  public void Check() {
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js",
        "const plugin = new RASP('test')\nplugin.register('request', (params) => {\nif (params.timeout) { for(;;) {} }\nreturn params\n})" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray()));
    String params;
    params = "{\"action\":\"ignore\"}";
    assertNull(V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true));
    params = "{\"action\":\"log\"}";
    assertEquals(V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true),
        "[{\"action\":\"log\",\"message\":\"\",\"name\":\"test\",\"confidence\":0}]");
    params = "{\"action\":\"block\"}";
    assertEquals(V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true),
        "[{\"action\":\"block\",\"message\":\"\",\"name\":\"test\",\"confidence\":0}]");
    params = "{\"timeout\":true}";
    assertEquals(V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true),
        "[{\"action\":\"log\",\"message\":\"Javascript plugin execution timeout\"}]");
  }

  @Test
  public void PluginLog() {
    V8.SetPluginLogger(new Logger() {
      @Override
      public void log(String msg) {
        assertEquals(msg, "23333");
      }
    });
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js", "console.log(23333)" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray()));
    V8.SetPluginLogger(null);
  }

  @Test
  public void Context() {
    V8.SetPluginLogger(new Logger() {
      @Override
      public void log(String msg) {
        assertEquals(msg,
            "{\"json\":[\"fine\"],\"server\":[\"fine\"],\"body\":{},\"appBasePath\":\"fine\",\"remoteAddr\":\"fine\",\"protocol\":\"fine\",\"method\":\"fine\",\"querystring\":\"fine\",\"path\":\"fine\",\"parameter\":[\"fine\"],\"header\":[\"fine\"],\"url\":\"fine\"}");
      }
    });
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js",
        "const plugin = new RASP('test')\nplugin.register('request', (params, context) => console.log(JSON.stringify(context)))" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray()));
    String params = "{\"action\":\"ignore\"}";
    V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true);
    V8.SetPluginLogger(null);
  }
}