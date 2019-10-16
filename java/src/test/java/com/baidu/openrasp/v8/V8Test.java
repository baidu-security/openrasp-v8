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

  static String log = null;

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
        log = msg;
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
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js",
        "const plugin = new RASP('test')\nplugin.register('request', params => console.log(params))" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray(), "1.2.3"));
    String params = "{\"action\":\"ignore\"}";
    assertNull(V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 100));
    assertEquals("{ action: 'ignore', stack: [ 1, 2, 3, 4 ] }", log);
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
      byte[] rst = V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 200);
      Any any = JsonIterator.deserialize(rst).asList().get(0);
      assertEquals("log", any.toString("action"));
      assertEquals("", any.toString("message"));
      assertEquals("test", any.toString("name"));
      assertEquals(0, any.toInt("confidence"));
    }
    {
      String params = "{\"action\":\"block\"}";
      byte[] rst = V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 200);
      Any any = JsonIterator.deserialize(rst).asList().get(0);
      assertEquals("block", any.toString("action"));
      assertEquals("", any.toString("message"));
      assertEquals("test", any.toString("name"));
      assertEquals(0, any.toInt("confidence"));
    }
    {
      String params = "{\"timeout\":true}";
      byte[] rst = V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), false, 200);
      Any any = JsonIterator.deserialize(rst).asList().get(0);
      assertEquals("exception", any.toString("action"));
      assertEquals("Javascript plugin execution timeout", any.toString("message"));
    }
  }

  @Test
  public void PluginLog() {
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js", "console.log(23333)" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray(), "1.2.3"));
    assertEquals("23333", log);
  }

  @Test
  public void Context() {
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js",
        "const plugin = new RASP('test')\nplugin.register('request', (params, context) => console.log(JSON.stringify(context)))" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray(), "1.2.3"));
    String params = "{\"action\":\"ignore\"}";
    byte[] result = V8.Check("request", params.getBytes(), params.getBytes().length, new ContextImpl(), true, 200);
    assertNull(result);
    assertEquals(
        "{\"body\":{},\"header\":[\"test 中文 & 😊\"],\"parameter\":[\"test 中文 & 😊\"],\"server\":[\"test 中文 & 😊\"],\"json\":[\"test 中文 & 😊\"],\"requestId\":\"test 中文 & 😊\",\"appBasePath\":\"test 中文 & 😊\",\"remoteAddr\":\"test 中文 & 😊\",\"protocol\":\"test 中文 & 😊\",\"querystring\":\"test 中文 & 😊\",\"url\":\"test 中文 & 😊\",\"method\":\"test 中文 & 😊\",\"path\":\"test 中文 & 😊\"}",
        log);
  }

  @Test
  public void Unicode() throws Exception {
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js",
        "console.log('test 中文 & 😊'); const plugin = new RASP('test'); plugin.register('request', params => { console.log(params.message); return params; })" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray(), "1.2.3"));
    assertEquals("test 中文 & 😊", log);
    Map<String, Object> params = new HashMap<String, Object>();
    params.put("action", "log");
    params.put("message", "test 中文 & 😊");
    ByteArrayOutputStream data = new ByteArrayOutputStream();
    JsonStream.serialize(params, data);
    byte[] result = V8.Check("request", data.getByteArray(), data.size(), new ContextImpl(), true, 200);
    assertEquals("test 中文 & 😊", log);
    Any any = JsonIterator.deserialize(result);
    assertEquals(any.asList().get(0).toString("message"), "test 中文 & 😊");
    assertEquals(V8.ExecuteScript("console.log('test 中文 & 😊'); 'test 中文 & 😊';", "test"), "test 中文 & 😊");
    assertEquals("test 中文 & 😊", log);
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

  public class Task implements Callable<String> {
    public int id;

    public Task(int id) {
      this.id = id;
    }

    @Override
    public String call() throws Exception {
      Map<String, Object> params = new HashMap<String, Object>();
      ByteArrayOutputStream data = new ByteArrayOutputStream();
      params.put("flag", id);
      JsonStream.serialize(params, data);
      String msg = new String(V8.Check("request", data.getByteArray(), data.size(), new ContextImpl(), true, 200));
      if (!msg.contains("timeout")) {
        return msg;
      }
      Thread.sleep(id / 20);
      return new String(V8.Check("requestEnd", data.getByteArray(), data.size(), new ContextImpl(), false, 200));
    }
  }

  /**
   * 测试在多线程并发请求下 每个isolate是否能够正常工作 timeout是否正常工作 被缓存的context是否正确
   * 
   * @throws Exception
   */
  @Test
  public void ParallelCheck() throws Exception {
    List<String[]> scripts = new ArrayList<String[]>();
    scripts.add(new String[] { "test.js", "const plugin = new RASP('test');\n"
        + "plugin.register('request', (params, context) => { context.flag = params.flag; while(true); })\n"
        + "plugin.register('requestEnd', (params, context) => { return {action: 'log', message: context.flag == params.flag ? 'ok' : `${context.flag} ${params.flag}`}; })\n" });
    assertTrue(V8.CreateSnapshot("{}", scripts.toArray(), "1.2.3"));
    ExecutorService service = Executors.newFixedThreadPool(20);
    List<Future<String>> futs = new ArrayList<Future<String>>();
    for (int i = 0; i < 50; i++) {
      Future<String> fut = service.submit(new Task(i));
      futs.add(fut);
    }
    service.shutdown();
    for (Future<String> fut : futs) {
      assertEquals("[{\"action\":\"log\",\"message\":\"ok\",\"name\":\"test\",\"confidence\":0}]", fut.get());
    }
    assertTrue(service.awaitTermination(10, TimeUnit.SECONDS));
  }
}