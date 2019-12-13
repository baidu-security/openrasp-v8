/*
 * Copyright (c) 2005, 2014, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

package org.sample;

import org.openjdk.jmh.annotations.*;
import org.apache.commons.io.FileUtils;
import java.util.concurrent.TimeUnit;
import java.util.ArrayList;
import com.baidu.openrasp.v8.V8;
import com.baidu.openrasp.v8.ByteArrayOutputStream;

@Fork(1)
@Threads(1000)
@Warmup(iterations = 1, time = 10, timeUnit = TimeUnit.SECONDS)
@Measurement(iterations = 2000, time = 10, timeUnit = TimeUnit.SECONDS)
@State(Scope.Benchmark)
public class MyBenchmark {

    public static Context context;

    @Setup(Level.Trial)
    public void setupTrial() throws Exception {
        V8.Load();
        Context.setStringKeys(new String[] { "path", "method", "url", "querystring", "protocol", "remoteAddr",
                "appBasePath", "requestId" });
        Context.setObjectKeys(new String[] { "json", "server", "parameter", "header" });
        Context.setBufferKeys(new String[] { "body" });
        V8.SetLogger(new Logger());
        V8.SetStackGetter(new StackGetter());
        V8.Initialize(2);
        String plugin = FileUtils.readFileToString(org.openjdk.jmh.util.FileUtils.extractFromResource("/plugin.js"),
                "UTF-8");
        ArrayList<String[]> scripts = new ArrayList<String[]>();
        scripts.add(new String[] { "test.js", plugin });
        V8.CreateSnapshot("{}", scripts.toArray(), "1.2.3");
    }

    @TearDown(Level.Trial)
    public void tearDownTrial() throws Exception {
        for (int i = 0; i < 10; i++) {
            V8.Check("requestEnd", "{}".getBytes(), 2, context, 100);
        }
    }

    @Setup(Level.Iteration)
    public void setupIteration() throws Exception {
        context = new Context();
        V8.Check("request", "{}".getBytes(), 2, context, 100);
    }

    @TearDown(Level.Iteration)
    public void tearDownIteration() throws Exception {
        V8.Check("requestEnd", "{}".getBytes(), 2, context, 100);
    }

    @Benchmark
    public void testCheck() {
        String type = Params.GetType();
        byte[] params = Params.GetParams(type);
        V8.Check(type, params, 0, context, 1000);
    }
}
