# MultiHostTestSuit

## 简介

MultiHostTestSuite中有3个可执行程序，分别是maner，worker和order。

### maner

maner全局有且只有1个，用于集中分发消息。运行maner时可以指定参数ip, p1, p2。

* ip: 指定maner所在的IP地址，默认127.0.0.1。
* p1: RPC端口，默认6021。
* p2: Multicast端口，默认6022。

ip、p1和p2共同决定唯一的mhts组。

> 直接在maner上输入 `:command`，可以直接将指令下发到每个worker执行。预留这个接口主要是为了检测maner和worker是否联通。

### worker

worker是分布在每台设备上的“工人”，一个worker负责拉起**一个进程**，**阻塞执行**，执行完毕后向order返回执行结果。执行结果包括：应用程序执行返回值，应用程序控制台输出等等。运行worker时可以指定参数ip, p1, p2, name, group, mode。

* ip: 指定maner所在的IP地址，默认127.0.0.1。
* p1: RPC端口，默认6021。
* p2: Multicast端口，默认6022。
* name: 该worker的名称。
* group: 用于给不同的worker分组，order可以按照组来分发指令。
* mode：工作模式，可以选择pipe或file，pipe遵循操作系统的管道缓存限制（默认4KB），适用于结果输出在限制范围内或没有写权限的环境；file没有管道缓存限制，能够存储的结果大小由磁盘剩余容量决定，需要当前环境的写权限。

### order

order用于向worker下发指令，等待所有的worker执行完成并返回结果后才会退出。运行order时可以指定参数ip, p1, p2, command, env, group, output, workers, timeout。

* ip: 指定maner所在的IP地址，默认127.0.0.1。
* p1: RPC端口，默认6021。
* p2: Multicast端口，默认6022。
* command: 需要下发的指令。
* env：指定应用程序运行时环境变量，如env="LD_PRELOAD=~/DDS/lib/libddsc.so;LD_LIBRARY_PATH=xxx"，多个环境变量之间使用 `;`隔开。
* group: 与command配合使用，一个command对应一个group。若未指定group，则只有默认group的worker会执行指令。若指定group为**all**，则所有的worker都会执行该指令。
* output: 存放所有worker执行结果的log文件。默认output.log。若指定output，则使用**安静模式**，不在控制台打印执行结果；否则将在控制台打印执行结果，且将结果保存在默认文件output.log中。
* workers: 执行指令的worker数量，该参数用于控制order在收到指定workers数量的结果才退出。默认1。
* timeout: command的执行超时时间，默认300s，与command配合使用，一个command对应一个timeout。如果在指定时间内order没有退出，则worker会向所有子进程发送SIGINT信号。
* delay：command的延迟执行时间，单位毫秒，默认0毫秒。一个command对应一个delay。

## 使用场景举例

一个简单的场景：获取两台设备上的文件系统使用情况 ` df -h`。

**设备1：192.168.10.54**

```bash
# device_1.sh
# start maner, specify maner ip
mhts_maner ip=192.168.10.54 &

# start worker, specify maner ip and its name
mhts_worker ip=192.168.10.54 name=worker1
```

**设备2：192.168.10.55**

```bash
# device_2.sh
# start worker, specify maner ip and its name
mhts_worker ip=192.168.10.54 name=worker2
```

**此时maner和worker已就绪，就等待order下发指令了。**

**在任意一台机器上能与54，55连通的机器上，执行：**

```bash
# start order, specify maner ip and total worker number
mhts_order ip=192.168.10.54 command="df -h" workers=2
```

**就可以获取到设备1和设备2上的文件系统使用情况了。**

## 综合分析

| 优点                       | 缺点             |
| -------------------------- | ---------------- |
| 功能丰富，能够满足日常需求 | 有一定学习成本   |
| 自动化脚本好维护           | 需要提前部署服务 |
| 支持跨平台                 |                  |



## V1.0——ChangeLog

实现测试套件。`7ec6ba7435cdaf057a673bd263b2941ffb13ca38`

## V1.1——ChangeLog

#### 优化多组worker执行

order以command为索引，每个order都可以指定执行worker的group，不指定的话则表示使用默认worker；当group为all时表示所有worker都要执行。

#### order实现安静运行模式

当order指定output时，使用安静模式，结果仅输出到指定的log文件；不指定output时输出到控制台，并写入output.log。

`83d9f1dffa72a643501d7b5e19e1848698b474c7`

## V1.2——ChangeLog

#### 支持指定运行时环境变量

order下发指令时，允许指定指令运行时的环境变量，order增加命令行参数env来指定环境变量，多个环境变量之间通过 `;`隔开。

`6314f3bb454b93068f3d4239f2985707c0f63355`

## V1.3——ChangeLog

order增加配置项delay，实现延迟执行子进程，具体见简介；

order修改timeout配置项含义，具体见简介。

`f77daa36eb6a8826ebc670cd630cab892e2607ac`
