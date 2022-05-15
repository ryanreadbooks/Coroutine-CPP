# Coroutine-CPP
### 这是一个使用C++基于ucontext的协程实现
##### 实现如下功能
* 协程类
* 协程执行器
* 协程调度器

##### 实现思路
1. 封装ucontext的基本操作成Coroutine类;
2. 每个线程定义一个协程执行器CoExecutor, 可以执行多个协程
3. 在协程调度器CoScheduler中开启多个CoExecutor来执行多个协程; CoScheduler中开启一个调度线程来调度不同CoExecutor中的协程对象.




### 构建
```shell
mkdir build
cd build
cmake ..
make
```



### 代码结构

├── src
│   ├── coexecutor.cpp
│   ├── coexecutor.h
│   ├── containers.hpp
│   ├── coroutine.cpp
│   ├── coroutine.h
│   ├── coscheduler.cpp
│   ├── coscheduler.h
│   ├── mutexes.cpp
│   ├── mutexes.h
│   ├── nocopyable.h
│   ├── singleton.hpp
│   ├── thread.cpp
│   ├── thread.h
│   ├── threadpool.cpp
│   ├── threadpool.h
│   ├── utils.cpp
│   └── utils.h
└── tests

