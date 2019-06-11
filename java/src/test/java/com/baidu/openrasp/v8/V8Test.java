package com.baidu.openrasp.v8;

import static org.junit.Assert.*;
import org.junit.*;
import java.util.*;
import com.jsoniter.spi.JsoniterSpi;
import com.jsoniter.extra.Base64Support;
import com.jsoniter.output.JsonStream;
import com.jsoniter.JsonIterator;
import com.jsoniter.any.Any;

public class V8Test {

  @BeforeClass
  public static void Initialize() throws Exception {
    V8.Load();
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
    assertNull(V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 100));
    params = "{\"action\":\"log\"}";
    assertEquals(V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 100),
        "[{\"action\":\"log\",\"message\":\"\",\"name\":\"test\",\"confidence\":0}]");
    params = "{\"action\":\"block\"}";
    assertEquals(V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 100),
        "[{\"action\":\"block\",\"message\":\"\",\"name\":\"test\",\"confidence\":0}]");
    params = "{\"timeout\":true}";
    assertEquals(V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 100),
        "[{\"action\":\"exception\",\"message\":\"Javascript plugin execution timeout\"}]");
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
            "{\"json\":[\"test 中文 & 😊\"],\"server\":[\"test 中文 & 😊\"],\"body\":{},\"appBasePath\":\"test 中文 & 😊\",\"remoteAddr\":\"test 中文 & 😊\",\"protocol\":\"test 中文 & 😊\",\"method\":\"test 中文 & 😊\",\"querystring\":\"test 中文 & 😊\",\"path\":\"test 中文 & 😊\",\"parameter\":[\"test 中文 & 😊\"],\"header\":[\"test 中文 & 😊\"],\"url\":\"test 中文 & 😊\"}");
      }
    });
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js",
        "const plugin = new RASP('test')\nplugin.register('request', (params, context) => console.log(JSON.stringify(context)))" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray()));
    String params = "{\"action\":\"ignore\"}";
    V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 100);
    V8.SetPluginLogger(null);
  }

  @Test
  public void Unicode() throws Exception {
    V8.SetPluginLogger(new Logger() {
      @Override
      public void log(String msg) {
        assertEquals(msg, "test 中文 & 😊");
      }
    });
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js",
        "console.log('test 中文 & 😊'); const plugin = new RASP('test'); plugin.register('request', params => { console.log(params.message); return params; })" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray()));
    Map<String, Object> params = new HashMap<String, Object>();
    params.put("action", "log");
    params.put("message", "test 中文 & 😊");
    ByteArrayOutputStream data = new ByteArrayOutputStream();
    JsonStream.serialize(params, data);
    String result = V8.Check("request", data.getByteArray(), data.size(), new ContextImpl(), true, 100);
    assertEquals(result, "[{\"action\":\"log\",\"message\":\"test 中文 & 😊\",\"name\":\"test\",\"confidence\":0}]");
    Any any = JsonIterator.deserialize(result.getBytes("UTF-8"));
    assertEquals(any.asList().get(0).toString("message"), "test 中文 & 😊");
    assertEquals(V8.ExecuteScript("console.log('test 中文 & 😊'); 'test 中文 & 😊';", "test"), "test 中文 & 😊");
  }

  @Test(timeout = 500)
  public void Timeout() throws Exception {
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js",
        "const plugin = new RASP('test')\nplugin.register('request', (params) => {\nfor(;;) {}\n})" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray()));
    String params = "{\"action\":\"ignore\"}";
    assertEquals(V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 400),
        "[{\"action\":\"exception\",\"message\":\"Javascript plugin execution timeout\"}]");
  }
}