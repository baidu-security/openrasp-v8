# openrasp-v8 for go

## Build

Firstly, make sure the git and git-lfs have been installed.

Clone this project and enter go directory

```
git clone https://github.com/baidu-security/openrasp-v8.git
cd go
```

Build C++ dependencies

```
mkdir build
cmake -S . -B build
cmake --build build
```

Build go package

```
go build
```

Run tests

```
go test
```

## Install

Go get without building and installing

```
go get -d github.com/baidu-security/openrasp-v8/go
```

Enter the source directory

```
cd $GOPATH/src/github.com/baidu-security/openrasp-v8/go
```

Follow the build guide above and then,

Install package

```
go install
```

## Caveat

The v8 static library is tracked by git-lfs in this repository, if you can not install the git-lfs, you can download the libv8 mananly, and tell cmake where can find the libv8_monolith.a

```
cmake -S . -B build -DCMAKE_PREFIX_PATH=path-to-libv8
```