package com.baidu.openrasp.v8;

import static org.junit.Assert.*;
import org.junit.*;
import java.util.*;
import com.jsoniter.output.JsonStream;
import com.jsoniter.JsonIterator;
import com.jsoniter.any.Any;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.Callable;

public class V8Test {

  @BeforeClass
  public static void Initialize() throws Exception {
    V8.Load();
    Context.setStringKeys(
        new String[] { "path", "method", "url", "querystring", "protocol", "remoteAddr", "appBasePath", "requestId" });
    Context.setObjectKeys(new String[] { "json", "server", "parameter", "header" });
    Context.setBufferKeys(new String[] { "body" });
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
  public void Check() throws Exception {
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js",
        "const plugin = new RASP('test')\nplugin.register('request', (params) => {\nif (params.timeout) { for(;;) {} }\nreturn params\n})" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray()));
    String params;
    params = "{\"action\":\"ignore\"}";
    assertNull(V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 100));
    params = "{\"action\":\"log\"}";
    assertArrayEquals(V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 100),
        "[{\"action\":\"log\",\"message\":\"\",\"name\":\"test\",\"confidence\":0}]".getBytes("UTF-8"));
    params = "{\"action\":\"block\"}";
    assertArrayEquals(V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 100),
        "[{\"action\":\"block\",\"message\":\"\",\"name\":\"test\",\"confidence\":0}]".getBytes("UTF-8"));
    params = "{\"timeout\":true}";
    assertArrayEquals(V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 100),
        "[{\"action\":\"exception\",\"message\":\"Javascript plugin execution timeout\"}]".getBytes("UTF-8"));
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
            "{\"body\":{},\"header\":[\"test 中文 & 😊\"],\"parameter\":[\"test 中文 & 😊\"],\"server\":[\"test 中文 & 😊\"],\"json\":[\"test 中文 & 😊\"],\"requestId\":\"test 中文 & 😊\",\"appBasePath\":\"test 中文 & 😊\",\"remoteAddr\":\"test 中文 & 😊\",\"protocol\":\"test 中文 & 😊\",\"querystring\":\"test 中文 & 😊\",\"url\":\"test 中文 & 😊\",\"method\":\"test 中文 & 😊\",\"path\":\"test 中文 & 😊\"}");
      }
    });
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js",
        "const plugin = new RASP('test')\nplugin.register('request', (params, context) => console.log(JSON.stringify(context)))" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray()));
    String params = "{\"action\":\"ignore\"}";
    byte[] result = V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 100);
    assertNull(result);
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
    byte[] result = V8.Check("request", data.getByteArray(), data.size(), new ContextImpl(), true, 100);
    Any any = JsonIterator.deserialize(result);
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
    assertArrayEquals(V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 400),
        "[{\"action\":\"exception\",\"message\":\"Javascript plugin execution timeout\"}]".getBytes("UTF-8"));
  }

  @Test
  public void ParallelCheck() throws Exception {
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js",
        "const plugin = new RASP('test')\nplugin.register('request', (params) => {\nif (params.timeout) { for(;;) {} }\nreturn params\n})" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray()));
    Callable<byte[]> task = new Callable<byte[]>() {
      @Override
      public byte[] call() {
        String params = "{\"action\":\"ignore\",\"timeout\":true}";
        return V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 100);
      }
    };
    ExecutorService service = Executors.newCachedThreadPool();
    List<Future<byte[]>> futs = new ArrayList<Future<byte[]>>();
    for (int i = 0; i < 100; i++) {
      Future<byte[]> fut = service.submit(task);
      futs.add(fut);
    }
    service.shutdown();
    for (Future<byte[]> fut : futs) {
      byte[] rst = fut.get();
      assertArrayEquals(rst,
          "[{\"action\":\"exception\",\"message\":\"Javascript plugin execution timeout\"}]".getBytes("UTF-8"));
    }
    assertTrue(service.awaitTermination(10, TimeUnit.SECONDS));
  }
}