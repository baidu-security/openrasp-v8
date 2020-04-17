# openrasp-v8 

![CI](https://github.com/baidu-security/openrasp-v8/workflows/CI/badge.svg)

Google V8 JavaScript engine with OpenRASP builtins, and bridges that are used to embed V8 within languages which are supported by [OpenRASP](https://github.com/baidu/openrasp).

Currently supported languages:

- PHP
- Java
- Go

## Features

### OpenRASP attack detect engine in JavaScript

Full JavaScript runtime context of OpenRASP attack detect engine, with simple C++ interfaces and helpers.

Integrating openrasp-v8 in other languages only need several lines.

### Lexical analyzer

Flex lexical analyzer is embeded in openrasp-v8 to tokenize sql and command strings. Just call `RASP.sql_tokenize` or `RASP.cmd_tokenize`

### HTTP request

The http request method is `RASP.request`. It accepts a configure object and returns a promise. Just like the axios.

### Platform independent

You can build and use it on different OS types, different OS versions and different OS archs.

For convenience of build, we have built some third-party libraries to static libraries:

- Linux x64
- Linux x86
- Linux musl
- OSX x64
- Windows x64
- Windows x86

For compatibility of Linux, most libraries will be staticly linked into openrasp-v8, even libc++. You can even use centos6 sysroot to force linker to link old version glibc.

### Support multiple languages

We will continuely add more languages to our support list.

The language support briges are not just some C++ ports, but also the language specified native interfaces. Such as jni fo Java and cgo for Go.

## Build

We use CMake to generate the files needed by your build tool (GNU make, Visual Studio, Ninja, etc.) for building openrasp-v8 and its language ports.

For example:

```shell
mkdir build
cd build
cmake -DENABLE_LANGUAGES=all ..
make
make test
```

The openrasp-v8 specified cmake variables:

- ENABLE_LANGUAGES

  Cmake list of languages to build, or all for building all (base, php, java, go)

- BUILD_TESTING
  
  Boolean option for whether the tests will be built

- BUILD_COVERAGE

  Boolean option for whether the coverage will be built
