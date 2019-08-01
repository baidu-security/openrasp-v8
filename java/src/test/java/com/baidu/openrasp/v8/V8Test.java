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
    V8.SetLogger(new Logger() {
      @Override
      public void log(String msg) {
        System.out.println(msg);
      }
    });
    V8.SetStackGetter(new StackGetter() {
      @Override
      public byte[] get() {
        return "[1,2,3,4]".getBytes();
      }
    });
    assertTrue(V8.Initialize());
    try {
      V8.ExecuteScript("2333", "6666");
      fail();
    } catch (Exception e) {
      assertNotNull(e);
    }
    assertNull(V8.Check(null, null, 0, null, true, 100));
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
    assertTrue(V8.CreateSnapshot("{}", new Object[0], "1.2.3"));
    assertEquals(V8.ExecuteScript("23333", "6666"), "23333");
    try {
      V8.ExecuteScript("aaaa.a()", "exception");
    } catch (Exception e) {
      assertNotNull(e);
    }
  }

  @Test
  public void GetStack() throws Exception {
    V8.SetLogger(new Logger() {
      @Override
      public void log(String msg) {
        assertEquals("{ action: 'ignore', stack: [ 1, 2, 3, 4 ] }", msg);
      }
    });
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js",
        "const plugin = new RASP('test')\nplugin.register('request', params => console.log(params))" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray(), "1.2.3"));
    String params = "{\"action\":\"ignore\"}";
    assertNull(V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 100));
  }

  @Test
  public void CreateSnapshot() {
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js", "const plugin = new RASP('test')" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray(), "1.2.3"));
  }

  @Test
  public void Check() throws Exception {
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js",
        "const plugin = new RASP('test')\nplugin.register('request', (params) => {\nif (params.timeout) { for(;;) {} }\nreturn params\n})" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray(), "1.2.3"));
    {
      String params = "{\"action\":\"ignore\"}";
      assertNull(V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 100));
    }
    {
      String params = "{\"action\":\"log\"}";
      byte[] rst = V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 100);
      Any any = JsonIterator.deserialize(rst).asList().get(0);
      assertEquals("log", any.toString("action"));
      assertEquals("", any.toString("message"));
      assertEquals("test", any.toString("name"));
      assertEquals(0, any.toInt("confidence"));
    }
    {
      String params = "{\"action\":\"block\"}";
      byte[] rst = V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 100);
      Any any = JsonIterator.deserialize(rst).asList().get(0);
      assertEquals("block", any.toString("action"));
      assertEquals("", any.toString("message"));
      assertEquals("test", any.toString("name"));
      assertEquals(0, any.toInt("confidence"));
    }
    {
      String params = "{\"timeout\":true}";
      byte[] rst = V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), false, 100);
      Any any = JsonIterator.deserialize(rst).asList().get(0);
      assertEquals("exception", any.toString("action"));
      assertEquals("Javascript plugin execution timeout", any.toString("message"));
    }
  }

  @Test
  public void PluginLog() {
    V8.SetLogger(new Logger() {
      @Override
      public void log(String msg) {
        assertEquals(msg, "23333");
      }
    });
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js", "console.log(23333)" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray(), "1.2.3"));
    V8.SetLogger(null);
  }

  @Test
  public void Context() {
    V8.SetLogger(new Logger() {
      @Override
      public void log(String msg) {
        assertEquals(msg,
            "{\"body\":{},\"header\":[\"test 中文 & 😊\"],\"parameter\":[\"test 中文 & 😊\"],\"server\":[\"test 中文 & 😊\"],\"json\":[\"test 中文 & 😊\"],\"requestId\":\"test 中文 & 😊\",\"appBasePath\":\"test 中文 & 😊\",\"remoteAddr\":\"test 中文 & 😊\",\"protocol\":\"test 中文 & 😊\",\"querystring\":\"test 中文 & 😊\",\"url\":\"test 中文 & 😊\",\"method\":\"test 中文 & 😊\",\"path\":\"test 中文 & 😊\"}");
      }
    });
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js",
        "const plugin = new RASP('test')\nplugin.register('request', (params, context) => console.log(JSON.stringify(context)))" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray(), "1.2.3"));
    String params = "{\"action\":\"ignore\"}";
    byte[] result = V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 100);
    assertNull(result);
    V8.SetLogger(null);
  }

  @Test
  public void Unicode() throws Exception {
    V8.SetLogger(new Logger() {
      @Override
      public void log(String msg) {
        assertEquals(msg, "test 中文 & 😊");
      }
    });
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js",
        "console.log('test 中文 & 😊'); const plugin = new RASP('test'); plugin.register('request', params => { console.log(params.message); return params; })" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray(), "1.2.3"));
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

  @Test(timeout = 800)
  public void Timeout() throws Exception {
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js",
        "const plugin = new RASP('test')\nplugin.register('request', (params) => {\nfor(;;) {}\n})" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray(), "1.2.3"));
    String params = "{\"action\":\"ignore\"}";
    assertArrayEquals(V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 400),
        "[{\"action\":\"exception\",\"message\":\"Javascript plugin execution timeout\"}]".getBytes("UTF-8"));
  }

  @Test
  public void ParallelCheck() throws Exception {
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js",
        "const plugin = new RASP('test')\nplugin.register('request', (params) => {\nif (params.timeout) { for(;;) {} }\nreturn params\n})" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray(), "1.2.3"));
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