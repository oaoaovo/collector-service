# collector-service MVP Implementation Plan

> 本实施计划基于 `v2版本设计文档` 制定，用于指导 AI 按阶段完成 C++17 mini 工业设备数据采集服务的开发。  
> 核心原则：不要一次性生成完整项目；每个 Phase 只实现一个明确目标，每个 Phase 都必须能编译、运行、验证。

---

## 0. 项目定位

本项目是一个 **工业设备数据采集服务 collector-service**，不是前端系统，也不是普通 WebSocket 聊天程序。

核心目标是实现以下链路：

```text
Device / DataPoint 配置
        ↓
DataTask 构造 query
        ↓
DriverClient 通过 WebSocket 发送 query
        ↓
MockDriverServer 模拟驱动返回 JSON
        ↓
DataTask 解析 response
        ↓
Resources 写入 SQLite
        ↓
HttpServer 提供查询接口
```

必须保留的架构思想：

```text
collector service 不直接生成采集数据
collector service 必须通过真实 WebSocket 与 driver service 通信
MockDriverServer 是假的驱动，但通信链路必须是真实 WebSocket
DataTask 是采集调度核心
SQLite 是本地数据存储
HttpServer 只提供服务接口，不包含前端
```

---

## 1. 技术栈约束

### 必选

```text
C++17
CMake
SQLite3
nlohmann/json
spdlog
cpp-httplib
Boost.Beast
yaml-cpp
std::thread
std::mutex
std::atomic_bool
```

### 建议约束

```text
HTTP 服务使用 cpp-httplib
WebSocket client/server 使用 Boost.Beast
JSON 统一使用 nlohmann/json
日志统一使用 spdlog
SQLite 访问封装在 SQLiteManager 内部
不要把 SQL 分散写在业务模块里
```

---

## 2. 目标目录结构

```text
collector-cpp/
│
├── README.md
│
├── CMakeLists.txt
│
├── config/
│   ├── devices.json
│   └── README.md
│
├── data/
│   ├── collector.sqlite
│   └── README.md
│
├── include/
│   ├── Logger.h
│   ├── SQLiteManager.h
│   ├── DataTask.h
│   ├── DriverClient.h
│   ├── MockDriverServer.h
│   ├── HttpServer.h
│   ├── Models.h
│   ├── ModelParser.h
│   └── README.md
│
├── src/
│   ├── main.cpp
│   ├── Logger.cpp
│   ├── SQLiteManager.cpp
│   ├── DataTask.cpp
│   ├── DriverClient.cpp
│   ├── MockDriverServer.cpp
│   ├── HttpServer.cpp
│   ├── ModelParser.cpp
│   └── README.md
│
├── scripts/
│   ├── init.sql
│   └── README.md
│
├── docs/
│   ├── README.md
│   ├── architecture.md
│   ├── websocket-protocol.md
│   ├── database-design.md
│   ├── phase-tracking.md
│   ├── ai-constitution.md
│   └── change-log.md
│
├── modules/
│   ├── SQLiteManager/
│   │   └── README.md
│   │
│   ├── DriverClient/
│   │   └── README.md
│   │
│   ├── MockDriverServer/
│   │   └── README.md
│   │
│   ├── DataTask/
│   │   └── README.md
│   │
│   ├── HttpServer/
│   │   └── README.md
│   │
│   └── ModelParser/
│       └── README.md
│
├── tests/
│   └── README.md
│
└── implementation-plan.md
```

---

## 3. 总体开发策略

# 3.1 AI Native 工程治理规则

本项目采用：

```text
README 分层 + 渐进披露（Progressive Disclosure）
```

方式进行 AI 协作开发。

---

## AI 必须遵守的规则

```text
1. AI 不允许一次性读取整个项目
2. AI 必须从根 README.md 开始逐层检索
3. AI 只能读取当前任务相关目录
4. AI 不允许修改未授权模块
5. AI 不允许提前实现未来 Phase
6. AI 不允许跳过验收步骤
7. 每次修改必须保持项目可编译
8. 每次代码修改必须同步更新对应 README.md
9. 每次新增类必须补充文档
10. 所有 WebSocket 通信必须是真实通信
11. MockDriverServer 不允许被普通 mock 替代
12. 不允许擅自重构项目架构
```

---

## README 的作用

项目中的 README.md：

不是普通说明文件。

而是：

```text
AI Navigation Layer
AI Context Router
AI Constitution Carrier
```

所有 AI 必须：

```text
从 README.md 开始理解项目。
```

---

## README 分层原则

每个目录必须包含 README.md。

目的：

```text
限制 AI 上下文范围
建立模块边界
降低误修改风险
```

每个 README.md 必须包含：

```text
1. 模块职责（Responsibility）
2. 模块边界（Boundary）
3. 当前状态（Current State）
4. 当前允许修改内容
5. 当前禁止修改内容
6. 设计原则（Design Rules）
7. 验收规则（Acceptance）
8. 修改历史（Change Log）
```

---

## 根 README.md 规则（最重要）

根 README.md：

是整个项目的 AI 总入口。

所有 AI 对话必须从根 README.md 开始。

---

## AI 检索路径（必须遵守）

如果任务涉及：

```text
数据库：
→ modules/SQLiteManager/README.md

采集逻辑：
→ modules/DataTask/README.md

WebSocket Client：
→ modules/DriverClient/README.md

WebSocket Server：
→ modules/MockDriverServer/README.md

HTTP 接口：
→ modules/HttpServer/README.md

YAML 模型：
→ modules/ModelParser/README.md
```

---

## 文档同步规则

任何代码修改：

必须同步更新：

```text
1. 当前模块 README.md
2. docs/change-log.md
3. docs/phase-tracking.md
```

否则视为：

```text
不完整修改
```

---



开发过程必须按 Phase 逐步推进。

每个 Phase 的要求：

```text
1. 只实现当前 Phase 范围内的功能
2. 不提前实现后续 Phase 的复杂功能
3. 每个 Phase 完成后必须有明确验证方式
4. 如果编译失败，先修复当前 Phase，不进入下一 Phase
```

---


# Phase 0：README 分层与 AI 工程治理初始化

## Goal

建立 AI Native 项目治理结构。

让 AI 可以：

```text
逐层理解项目
限制上下文范围
防止意图漂移
```

---

## Tasks

```text
1. 创建根 README.md
2. 创建 docs/README.md
3. 创建 docs/ai-constitution.md
4. 创建 docs/phase-tracking.md
5. 创建 docs/change-log.md
6. 为每个目录创建 README.md
7. 为每个核心模块创建 README.md
8. 在 README 中定义模块边界
9. 在 README 中定义设计原则
10. 在 README 中定义当前 Phase 范围
```

---

## Acceptance

```text
1. 所有目录包含 README.md
2. 根 README 可以作为 AI 总入口
3. AI 可以通过 README 逐层导航
4. 每个模块都有边界定义
5. 每个模块都有禁止行为说明
6. phase-tracking.md 可以记录开发阶段
7. change-log.md 可以记录历史修改
```

---

## Do Not

```text
不要实现业务功能
不要实现 SQLite
不要实现 WebSocket
不要实现 HTTP Server
```

---

# Phase 1：工程骨架 + Logger

## Goal

建立最小可编译 C++ 工程，完成项目目录、CMake、Logger 和 main.cpp。

## Scope

只做工程骨架，不实现数据库、WebSocket、HTTP 和采集逻辑。

## Tasks

```text
1. 创建 collector-cpp 目录结构
2. 创建 CMakeLists.txt
3. 创建 include/Models.h
4. 创建 include/Logger.h
5. 创建 src/Logger.cpp
6. 创建 src/main.cpp
7. main.cpp 中初始化 Logger
8. 程序启动后输出 collector service started
```

## Required Files

```text
CMakeLists.txt
include/Logger.h
src/Logger.cpp
include/Models.h
src/main.cpp
```

## Models.h 初始内容

需要先定义核心结构体：

```cpp
struct Device;
struct DataPoint;
struct ResourceRecord;
struct DeviceStatus;
```

完整字段按 v2 设计文档实现。

## Acceptance

执行：

```bash
mkdir build
cd build
cmake ..
cmake --build .
./collector-service
```

期望：

```text
程序成功启动
控制台输出 collector service started
无编译错误
无链接错误
```

## Do Not

```text
不要实现 SQLite
不要实现 WebSocket
不要实现 HTTP
不要实现 DataTask
```

---

# Phase 2：SQLiteManager + 数据库初始化

## Goal

实现 SQLiteManager，支持数据库初始化、建表、插入测试数据、读取 Device 和 DataPoint。

## Scope

只实现数据库基础能力，不实现采集线程和 WebSocket。

## Tasks

```text
1. 创建 scripts/init.sql
2. 实现 Device 表
3. 实现 DataPoint 表
4. 实现 Resources 表
5. 创建 SQLiteManager 类
6. SQLiteManager 构造时打开 data/collector.sqlite
7. 如果 data 目录不存在则自动创建
8. 执行 init.sql 初始化表
9. 提供 seedTestData() 插入测试设备和测试点位
10. 提供 getDevices()
11. 提供 getDataPointsByDeviceName(deviceName)
12. 提供 insertResources(records)
13. 提供 getLatestResources(deviceName)
```

## Required Files

```text
include/SQLiteManager.h
src/SQLiteManager.cpp
scripts/init.sql
```

## SQLite Tables

必须使用 v2 设计文档中的表结构：

```sql
Device
DataPoint
Resources
```

## Test Data

至少插入：

```text
Device:
Name = DeviceA
IpAddr = 127.0.0.1
Port = 9001
SampleInterval = 1000
Model = 一个简单 YAML 字符串
ModelVersion = v1
DriverPath = mock-driver

DataPoint:
DName = DeviceA
NamePath = /Position/temperature
ItemName = temperature
ItemDescription = 温度

DataPoint:
DName = DeviceA
NamePath = /Position/pressure
ItemName = pressure
ItemDescription = 压力
```

## Acceptance

程序启动后：

```text
data/collector.sqlite 被创建
Device 表存在
DataPoint 表存在
Resources 表存在
Device 表中至少一条记录
DataPoint 表中至少两条记录
控制台能打印读取到的 Device 和 DataPoint
```

## Do Not

```text
不要实现 DataTask 采集循环
不要实现 WebSocket 通信
不要实现 HTTP Server
```

---

# Phase 3：MockDriverServer WebSocket 服务端

## Goal

实现一个真实 WebSocket 服务端，用于模拟工业驱动 driver service。

## Scope

只实现 MockDriverServer，可以接收 query JSON 并返回 mock response JSON。

## Tasks

```text
1. 创建 MockDriverServer 类
2. 使用 Boost.Beast 实现 WebSocket server
3. 监听 127.0.0.1:9001
4. 接收客户端发送的 query JSON
5. 解析 cid
6. 解析 query 数组
7. 为每个 query path 生成模拟数据
8. 返回符合 v2 设计文档的 response JSON
9. 支持 start()
10. 支持 stop()
11. 服务运行在独立线程中
```

## Required Files

```text
include/MockDriverServer.h
src/MockDriverServer.cpp
```

## Input Query Format

```json
{
  "cid": "uuid-string",
  "query": [
    "/Position/temperature",
    "/Position/pressure"
  ]
}
```

## Output Response Format

```json
{
  "cid": "uuid-string",
  "machineConnected": true,
  "data": {
    "/Position/temperature": {
      "value": [2.0],
      "dev_time": 1766457708.8780046,
      "svr_time": 1766457708.8780046,
      "item_path": "/DV:mock/CP:mock/DT:temperature",
      "item_description": "温度",
      "item_name": "temperature"
    }
  }
}
```

## Acceptance

使用临时测试客户端或命令行工具连接：

```text
ws://127.0.0.1:9001
```

发送 query 后可以收到 JSON response。

## Do Not

```text
不要实现 DriverClient
不要实现 DataTask
不要写入 SQLite
不要实现 HTTP Server
```

---

# Phase 4：DriverClient WebSocket 客户端

## Goal

实现 DriverClient，通过真实 WebSocket 连接 MockDriverServer，发送 query 并接收 response。

## Scope

只实现 WebSocket 客户端，不接入 DataTask。

## Tasks

```text
1. 创建 DriverClient 类
2. 构造函数接收 host 和 port，或 ws url
3. 实现 connect()
4. 实现 sendQuery(json query)
5. 实现 close()
6. 使用 Boost.Beast WebSocket client
7. main.cpp 中临时测试：
   - 启动 MockDriverServer
   - 创建 DriverClient
   - 构造 query
   - 发送 query
   - 打印 response
```

## Required Files

```text
include/DriverClient.h
src/DriverClient.cpp
```

## Acceptance

运行程序后：

```text
MockDriverServer 正常启动
DriverClient 成功连接
DriverClient 发送 query
DriverClient 收到 response
response 中包含 machineConnected=true
response.data 中包含 /Position/temperature
```

## Do Not

```text
不要实现 DataTask
不要写入 Resources
不要实现 HTTP Server
```

---

# Phase 5：DataTask 单设备采集链路

## Goal

实现单设备完整采集链路：

```text
SQLite Device/DataPoint
        ↓
DataTask
        ↓
DriverClient WebSocket
        ↓
MockDriverServer
        ↓
response JSON
        ↓
Resources
```

## Scope

先只支持一台设备 DeviceA。

## Tasks

```text
1. 创建 DataTask 类
2. DataTask 持有 SQLiteManager 引用
3. DataTask 读取 Device 列表
4. DataTask 按设备读取 DataPoint
5. DataTask 为 DeviceA 构造 query
6. query.cid 使用简单 UUID 或时间戳字符串
7. query.query 使用 DataPoint.namePath
8. 调用 DriverClient 发送 query
9. 接收 response
10. 解析 response.data
11. 每个 data path 生成 ResourceRecord
12. ResourceRecord.Data 保存 value 的 JSON 字符串，例如 [2.0]
13. ResourceRecord.DevTime 取 dev_time
14. ResourceRecord.SvrTime 取 svr_time，没有则取当前时间
15. DataItemPath、DataItemName、DataItemDescription 优先取 response，没有则用 DataPoint 配置
16. 批量插入 Resources
17. 更新 DeviceStatus
```

## Required Files

```text
include/DataTask.h
src/DataTask.cpp
```

## DataTask Required Methods

```cpp
class DataTask {
public:
    void startAll();
    void stopAll();
    std::vector<DeviceStatus> getStatus();

private:
    void runDeviceTask(Device device);
    void processResponse(const Device& device,
                         const std::vector<DataPoint>& points,
                         const std::string& responseText);
};
```

## Acceptance

运行程序后：

```text
DataTask 可以读取 DeviceA
DataTask 可以读取 DeviceA 的 DataPoint
DataTask 可以构造 query
DriverClient 真实发送 query
MockDriverServer 返回 response
DataTask 解析 response
Resources 表新增 temperature 和 pressure 数据
控制台输出采集成功日志
```

## Do Not

```text
暂时不要实现多设备并发
暂时不要实现 HTTP Server
暂时不要实现复杂失败重试
```

---
# Phase 5.5：服务常驻与控制台命令模式

## Goal

让 collector-service 支持常驻运行，并允许在控制台输入 JSON 命令触发设备采集或设备管理操作。

## Scope

只新增服务生命周期管理和控制台命令入口；保留现有单次采集逻辑，不实现多设备线程调度。

## Tasks

```text
1. main.cpp 新增 --console 启动模式
2. --console 模式下初始化 SQLiteManager、DataTask、CommandProcessor
3. 启动 MockDriverServer，并保持 ws://127.0.0.1:9001 在线
4. 主线程进入 std::getline(std::cin, line) 控制台命令循环
5. 每输入一行 JSON，就调用 CommandProcessor::handle(line)
6. 打印 Command response
7. 支持 exit / quit 退出控制台循环
8. 支持空行跳过，避免误触发 unknown cmd
9. 捕获异常并打印错误日志，保证单条命令失败不导致进程退出
10. 支持 Ctrl+C 优雅退出
11. 退出时调用 MockDriverServer::stop()
12. 保留 --cmd / --cmd-file 单次命令模式
13. 保留无参数 Phase 5 手动验证模式
14. 更新 README 和 docs/phase-tracking.md
```

## Command Behavior

create_device
  控制台模式下可新增设备

update_device
  控制台模式下可更新设备

delete_device
  控制台模式下可删除设备

start_device_collect
  控制台模式下执行一次指定设备采集
  执行后服务不退出

stop_device_collect
  当前阶段仍返回已有逻辑，不控制线程


## Acceptance

1. 执行 .\build\collector-service.exe --console 后进程不退出
2. 控制台显示 collector service started
3. MockDriverServer 持续监听 ws://127.0.0.1:9001
4. 输入 start_device_collect JSON 后能完成一次采集
5. 采集后进程仍保持运行，可继续输入下一条命令
6. 输入 create/update/delete 设备命令后能返回 Command response
7. 输入 exit / quit 后服务优雅退出
8. Ctrl+C 后服务优雅退出
9. --cmd-file 仍保持执行一次后退出

---


# Phase 6：DataTask 多设备、多线程、状态管理

## Goal

在当前“单设备后台采集线程”的基础上，将 DataTask 扩展为多设备采集调度器。
每个在线设备独立一个采集线程，并维护每台设备自己的停止控制和运行状态。

## Current Baseline

当前阶段已经实现的内容：

```text
1. Device 表已经增加 Status 字段，用于表示设备是否在线
2. start_device_collect 已经会检查 Device.Status，只有在线设备可以启动采集
3. runDeviceTask(device) 已经是周期采集循环，不再是单次采集
4. 每轮采集流程已经包含：
   读取/初始化 DataPoint
   buildQuery
   DriverClient connect
   sendQuery
   processResponse
   更新状态
   按 SampleInterval sleep
5. sleep 已经拆成 100ms 小步长，期间持续检查停止标志
6. DataTask 当前已有一个后台 workerThread_
7. DataTask 当前已有一个全局 stopRequested_
8. DataTask 当前已有 activeDevices_ 和 getStatus() 雏形
```

## Scope

本阶段只扩展 DataTask 的多设备线程调度、停止控制和状态维护。

不实现 HttpServer。
不实现复杂重连策略。
不实现连接池。
不引入新的 Device.State 字段，统一使用当前已经存在的 Device.Status 字段。

## Tasks

```text
1. 将 DataTask 当前的单 workerThread_ 改为按设备名管理的线程表
   std::map<std::string, std::thread> workerThreads_

2. 将当前单一 stopRequested_ 改为每台设备一个停止标志
   std::map<std::string, std::shared_ptr<std::atomic_bool>> stopFlags_

3. 将当前 latest-only statuses_ 改为按设备名维护的状态表
   std::map<std::string, DeviceStatus> statuses_

4. activeDevices_ 可以保留，也可以用 workerThreads_ 的 key 判断设备是否正在运行

5. workerThreads_、stopFlags_、activeDevices_、statuses_ 必须使用 stateMutex_ 保护

6. DataTask 实现 startAll() 逻辑

7. startAll() 从 SQLiteManager::getDevices() 读取所有 Device

8. startAll() 只启动 Status == 1 的在线设备

9. startAll() 跳过已经在运行的设备，避免重复创建线程

10. 每个在线 Device 创建一个独立 std::thread

11. 每个线程只负责自己的 Device

12. 每个设备线程必须使用该设备自己的 IpAddr 和 Port 建立 WebSocket 连接

13. 每台设备按照自己的 SampleInterval 周期采集

14. 保留 100ms 小步长 sleep，保证 stop 时线程能快速退出

15. DataTask 实现 stopAll()

16. stopAll() 设置所有设备的 stop flag

17. stopAll() join 所有 joinable 的设备线程

18. stopAll() 在线程退出后清理 workerThreads_、stopFlags_、activeDevices_

19. 调整 startDeviceCollect(deviceName)

20. startDeviceCollect 必须拒绝 Status == 0 的离线设备

21. startDeviceCollect 重复启动同一设备时不能创建重复线程

22. startDeviceCollect 不能因为其他设备正在采集而失败

23. 调整 stopDeviceCollect(deviceName)

24. stopDeviceCollect 只设置指定设备的 stop flag

25. stopDeviceCollect 只 join 指定设备线程

26. stopDeviceCollect 只清理指定设备的线程状态

27. failCount 按设备分别记录连续失败次数

28. lastSuccessTime 按设备分别记录最近成功采集时间

29. 成功采集后 failCount 清零，error 清空

30. 采集失败后 failCount++

31. 连续失败 5 次后保留 error 字符串，并让该设备线程退出或标记为失败

32. DataTask 实现 getStatus()

33. getStatus() 返回每台设备的 DeviceStatus

34. DeviceStatus 中的 error 当前是 std::string，不要在 Phase 6 中改成 bool

35. CommandProcessor 新增 start_all 命令

36. start_all 调用 DataTask::startAll()

37. CommandProcessor 新增 stop_all 命令

38. stop_all 调用 DataTask::stopAll()

39. CommandProcessor 新增 get_status 命令

40. get_status 调用 DataTask::getStatus() 并序列化为 JSON

41. CommandProcessor 保留 start_device_collect 为启动指定设备线程

42. CommandProcessor 保留 stop_device_collect 为停止指定设备线程

43. main.cpp 增加 Ctrl+C 退出处理

44. Ctrl+C 退出时依次调用 DataTask::stopAll() 和 MockDriverServer::stop()

45. --cmd-file 仍可用于快速验证

46. --cmd-file start_device_collect 可以保持运行直到按 Enter 停止

47. --cmd-file start_all 可以保持运行直到按 Enter 停止

48. 更新 README、docs/phase-tracking.md、tests/README.md
```

## Command Behavior

start_all
  读取所有 Status == 1 的在线 Device
  为每台未运行的在线设备启动一个独立采集线程
  已经运行的设备不重复启动

stop_all
  停止所有采集线程
  join 所有设备线程
  清理线程表和停止标志

get_status
  返回所有已跟踪设备的状态
  状态包含 connected、machineConnected、failCount、lastSuccessTime、error

start_device_collect
  启动指定设备的持续采集线程
  只影响指定设备
  不影响其他正在采集的设备
  重复启动同一设备时不能创建重复线程

stop_device_collect
  停止指定设备的采集线程
  只影响指定设备
  不影响其他正在采集的设备

## Acceptance

插入多个 Device 后：

```text
1. 多个 Device 的 Status == 1 时，start_all 能为每台设备创建独立采集线程
2. Status == 0 的设备不会被 start_all 启动
3. 每台设备按照各自 SampleInterval 周期采集
4. 每台设备都只连接自己的 IpAddr 和 Port
5. Resources 表持续写入每台运行设备的采集数据
6. start_device_collect 能启动单个设备持续采集线程
7. start_device_collect 不会停止其他正在运行的设备
8. 重复启动同一设备不会创建重复线程
9. stop_device_collect 能停止指定设备线程
10. stop_device_collect 不影响其他正在运行的设备
11. stop_all 能停止并 join 所有设备线程
12. get_status 能返回每个已跟踪设备的状态
13. connected 能反映 WebSocket 是否连接成功
14. machineConnected 能反映驱动响应里的 machineConnected
15. failCount 能按设备记录连续失败次数
16. 连续失败 5 次后该设备 error 字符串非空，并且该设备采集线程退出或被标记失败
17. 成功采集后 failCount 清零
18. 成功采集后 error 清空
19. lastSuccessTime 在成功采集后更新
20. Ctrl+C 后 stopAll() 和 MockDriverServer::stop() 被调用，不出现线程悬挂
21. --cmd-file 单次命令模式仍可用于快速验证
22. --cmd-file start_device_collect 和 start_all 能持续运行直到按 Enter 停止
```

## Do Not

```text
不要实现复杂重连策略
不要实现 HttpServer
不要让多个设备线程共享同一个 DriverClient
不要实现连接池
```

---
# Phase 7：HttpServer 查询接口

## Goal

实现 HttpServer，对外暴露 Phase 6 的多设备采集控制能力和状态/资源查询能力。

## Scope

只实现服务接口，不实现前端页面，不实现文件上传。

## Tasks

```text
1. 新增 HttpServer 模块
2. HttpServer 启动后监听本地端口，例如 8080
3. POST /commands 接收 JSON 命令
4. /commands 内部调用 CommandProcessor::handle()
5. 支持通过 HTTP 调用 start_all
6. 支持通过 HTTP 调用 stop_all
7. 支持通过 HTTP 调用 start_device_collect
8. start_device_collect 必须支持按 device_name 启动指定设备采集线程
9. 支持通过 HTTP 调用 stop_device_collect
10. stop_device_collect 必须支持按 device_name 停止指定设备采集线程
11. GET /devices/status 返回 DataTask::getStatus()
12. GET /devices/{deviceName}/status 返回指定设备状态
13. GET /devices/{deviceName}/resources/latest 查询指定设备最新 Resources
14. main.cpp 新增 --http 或 --serve-http 模式
15. 更新 README 和接口示例

```

## Required Files

```text
include/HttpServer.h
src/HttpServer.cpp
```

## API Contract

### GET /health

Response:

```json
{
  "success": true,
  "service": "collector-service"
}
```

### GET /devices

Response:

```json
{
  "success": true,
  "data": []
}
```

### GET /status

Response:

```json
{
  "success": true,
  "data": []
}
```

### GET /latest?device=DeviceA

Response:

```json
{
  "success": true,
  "data": []
}
```

### POST /tasks/start

Response:

```json
{
  "success": true
}
```

### POST /tasks/stop

Response:

```json
{
  "success": true
}
```

## Acceptance

使用 curl 验证：

```bash
curl http://127.0.0.1:8080/health
curl http://127.0.0.1:8080/devices
curl http://127.0.0.1:8080/status
curl "http://127.0.0.1:8080/status?device=DeviceA"
curl "http://127.0.0.1:8080/latest?device=DeviceA"
curl -X POST http://127.0.0.1:8080/tasks/start
curl -X POST "http://127.0.0.1:8080/tasks/start?device=DeviceA"
curl -X POST http://127.0.0.1:8080/tasks/stop
curl -X POST "http://127.0.0.1:8080/tasks/stop?device=DeviceA"

```

所有接口返回 JSON。

## Do Not

```text
不实现文件上传
不实现前端页面
不实现用户认证
```

---


# Phase 8：Resources 清理与稳定性增强

## Goal

在不改变现有采集架构的前提下，增强 Resources 表的数据清理能力和采集链路的异常容错能力。

本阶段目标不是增加新业务功能，而是保证系统长时间运行时：

```text
Resources 不会无限增长
单设备采集失败不会拖垮整个服务
单个异常不会导致进程退出
错误能被记录、能反映到设备状态中
后续排查问题时有明确日志依据
```

## Scope

只增强已有链路：

```text
SQLiteManager
DataTask
DriverClient
MockDriverServer 返回异常场景下的处理
README / tests 文档中的稳定性验证说明
```

不改变 Phase 6 的多设备调度架构。
不新增 HttpServer。
不改变 Resources 表的核心字段含义。

## Design Rules

```text
1. 清理逻辑必须集中在 SQLiteManager 内部，不要把 DELETE SQL 分散到 DataTask
2. 清理 Resources 时必须使用事务，避免清理过程中数据库处于半更新状态
3. 清理策略按每个设备分别保留最新数据，不能让某一台设备的数据挤掉全部历史
4. 采集异常只影响对应设备，不应影响其他设备线程
5. DataTask 必须捕获采集循环内的异常，不能让异常穿透到线程入口之外
6. DriverClient 只负责明确抛出或返回连接/发送/接收错误，不实现复杂重连
7. 错误必须写日志，同时更新 DeviceStatus.error
8. 成功采集后必须清除该设备的 error 和 failCount
9. 连续失败达到阈值时只停止该设备线程，不退出整个进程
```

## Tasks

```text
1. SQLiteManager 增加 cleanupResources(maxRowsPerDevice)

2. cleanupResources(maxRowsPerDevice) 按 DeviceName 分组清理 Resources

3. 每个 DeviceName 只保留最新 maxRowsPerDevice 条 Resources

4. 最新数据判断规则：优先按 SvrTime DESC，其次按 Id DESC

5. 被删除的数据应是每个设备最旧的数据

6. cleanupResources 必须使用事务

7. cleanupResources 参数必须做保护：
   maxRowsPerDevice <= 0 时直接返回或使用安全默认值

8. SQLiteManager 增加 cleanupResourcesForDevice(deviceName, maxRows)
   用于单设备采集线程在本设备采集后清理自己的历史数据

9. DataTask 增加资源清理触发逻辑

10. DataTask 不要每轮都无条件全表清理

11. 建议触发策略：
    每台设备每 N 轮采集后调用一次 cleanupResourcesForDevice
    或每台设备距离上次清理超过 cleanupIntervalMs 后调用一次

12. 建议默认值：
    maxRowsPerDevice = 5000
    cleanupIntervalMs = 60000

13. 清理失败不能中断采集线程

14. 清理失败必须写 warn/error 日志

15. DriverClient connect 失败时必须返回 false 或抛出明确错误

16. DriverClient sendQuery 发送失败时必须抛出明确异常

17. DriverClient read response 失败时必须抛出明确异常

18. DriverClient close 失败只记录 warning，不应导致进程退出

19. DataTask 捕获 DriverClient connect/send/read/close 相关异常

20. DataTask 捕获 response 解析异常

21. DataTask 捕获 response 缺少 data 字段或缺少点位字段的异常

22. DataTask 捕获 SQLite insertResources 异常

23. DataTask 捕获 cleanupResourcesForDevice 异常

24. DataTask 每次失败后更新该设备 DeviceStatus：
    connected = false
    failCount++
    error = 失败原因

25. DataTask 每次成功后更新该设备 DeviceStatus：
    connected = true
    machineConnected = response.machineConnected
    failCount = 0
    error = ""
    lastSuccessTime = 当前时间

26. 连续失败 5 次后，该设备采集线程退出或被标记为失败

27. 连续失败 5 次不能导致整个 collector-service 进程退出

28. MockDriverServer 停止或端口不可用时，DataTask 必须只记录该设备失败

29. MockDriverServer 返回非法 JSON 时，DataTask 必须只记录该设备失败

30. MockDriverServer 返回合法 JSON 但 data 为空时，DataTask 必须只记录该设备失败

31. SQLite 数据库被锁定或写入失败时，DataTask 必须只记录该设备失败

32. 所有 catch 块不得吞掉错误，必须写入 Logger

33. 日志中必须包含 deviceName 和失败阶段，例如 connect、sendQuery、processResponse、insertResources、cleanupResources

34. 更新 README.md，说明 Resources 清理策略和默认保留数量

35. 更新 docs/phase-tracking.md，记录 Phase 8 已实现能力

36. 更新 tests/README.md，补充稳定性手工验证场景
```

## Cleanup Strategy

```text
默认策略：按设备保留最新 5000 条 Resources

清理范围：
只清理 Resources 表
不清理 Device 表
不清理 DataPoint 表

清理排序：
保留 SvrTime 最新的数据
如果 SvrTime 相同，则保留 Id 更大的数据

清理触发：
1. 单设备采集成功后，可以按间隔触发 cleanupResourcesForDevice(deviceName, maxRowsPerDevice)
2. stopAll() 不负责清理历史数据
3. startAll() 不负责清理历史数据
4. 程序启动时暂不做全表清理，避免启动阻塞

清理失败：
只写日志
不影响本轮采集结果
不导致线程退出
```

## Error Handling Behavior

DriverClient connect 失败：
  当前设备 failCount++
  connected = false
  error 写入连接失败原因
  本轮采集失败，进入下一轮或达到 5 次后退出该设备线程

DriverClient sendQuery / read 失败：
  当前设备 failCount++
  connected = false
  error 写入发送或接收失败原因

response JSON 异常：
  当前设备 failCount++
  connected 可保持 true
  error 写入解析失败原因

response data 缺失：
  当前设备 failCount++
  error 写入 response did not contain resource records

SQLite insertResources 失败：
  当前设备 failCount++
  error 写入数据库写入失败原因

cleanupResources 失败：
  只写日志 warning/error
  不增加 failCount
  不改变本轮采集成功状态

连续失败 5 次：
  当前设备 error 保留最后一次失败原因
  当前设备线程退出或标记 failed
  其他设备线程继续运行

## Acceptance

```text
1. Resources 超过 maxRowsPerDevice 后，每个设备只保留最新 maxRowsPerDevice 条数据
2. 多设备场景下，一个设备的数据清理不会删除另一个设备未超限的数据
3. cleanupResources 使用事务，清理失败不会破坏 Resources 表
4. MockDriverServer 停止时，DataTask 不崩溃
5. DriverClient 连接失败时，设备 failCount 增加
6. DriverClient 发送失败时，设备 failCount 增加
7. DriverClient 接收失败时，设备 failCount 增加
8. MockDriverServer 返回非法 JSON 时，DataTask 不崩溃
9. MockDriverServer 返回空 data 时，DataTask 不崩溃
10. SQLite insertResources 失败时，DataTask 不崩溃
11. cleanupResources 失败时，采集线程不退出
12. 所有异常都有日志记录
13. 日志中能看到 deviceName 和失败阶段
14. 成功采集后 failCount 清零，error 清空
15. 连续失败 5 次后，只停止或标记该设备，不影响其他设备
16. collector-service 长时间运行时 Resources 不会无限增长
```

## Manual Verification

```text
1. 创建两个在线设备，设置不同 SampleInterval
2. start_all 启动多设备采集
3. 观察 Resources 表持续写入
4. 将 maxRowsPerDevice 临时设置为较小值，例如 10
5. 验证每个 DeviceName 最多保留 10 条最新 Resources
6. 停止 MockDriverServer
7. 验证 DataTask 不崩溃，failCount 增加，error 有错误信息
8. 恢复 MockDriverServer
9. 验证成功采集后 failCount 清零，error 清空
10. 构造非法 response JSON
11. 验证 DataTask 不崩溃并记录日志
12. 模拟 SQLite 写入失败或数据库锁定
13. 验证进程不崩溃并记录日志
```

## Do Not

```text
不要实现复杂重连策略
不要实现 HttpServer
不要修改 Resources 核心表结构
不要因为 cleanupResources 失败而停止采集线程
不要因为单设备异常导致整个进程退出
不要让一个设备的清理删除其他设备未超限的数据
```

---
# Phase 9：README 和最终验收

## Goal

补充项目说明、运行方式、接口说明和最终验收步骤。

## Tasks

```text
1. 编写 README.md
2. 说明项目背景
3. 说明目录结构
4. 说明构建方式
5. 说明运行方式
6. 说明 HTTP API
7. 说明 WebSocket query/response 格式
8. 说明当前 MockDriverServer 的作用
9. 说明 ModelParser 的后续扩展方向
10. 说明如何查看 SQLite Resources 数据
```

## README 必须包含

```text
项目简介
技术栈
构建步骤
运行步骤
HTTP 接口
WebSocket 协议
数据库表结构
后续扩展方向
```

## Final Acceptance

最终项目必须满足：

```text
1. CMake 能成功构建
2. 程序能启动
3. SQLite 数据库能自动创建
4. Device 表有测试设备
5. DataPoint 表有测试点位
6. MockDriverServer 能监听 WebSocket 端口
7. DriverClient 能真实发送 query
8. DataTask 能定时采集
9. Resources 表能持续写入数据
10. /status 能看到设备状态
11. /latest?device=DeviceA 能看到最新数据
12. Ctrl+C 后程序能优雅退出
```

---

## AI Coding 使用方式

建议将本文件和 `v2版本设计文档` 一起提供给 AI 编程工具。

每次只发送一个 Phase，例如：

```text
请严格按照 implementation-plan.md 的 Phase 1 实现。
只实现 Phase 1，不要实现 Phase 2 或后续内容。
完成后请说明新增/修改的文件，以及如何编译运行。
```

完成 Phase 1 并验证后，再发送：

```text
请继续按照 implementation-plan.md 的 Phase 2 实现。
保留 Phase 1 已有代码，只补充 Phase 2。
不要实现 Phase 3 或后续内容。
```

---

## 重要约束（AI 必须严格遵守）

```text
1. 不允许一次性生成完整项目
2. 不允许跳过 Phase
3. 不允许提前实现未来功能
4. 不允许扫描整个仓库
5. 不允许修改未授权模块
6. 不允许擅自重构架构
7. 每个 Phase 必须保持可编译
8. 每个 Phase 必须有明确验收方式
9. WebSocket 通信必须真实发生
10. MockDriverServer 不能被普通函数 mock 替代
11. DataTask 必须通过 DriverClient 采集数据
12. Resources 数据必须来自 WebSocket response
13. HTTP 接口只用于查询和任务控制
14. 每次代码修改必须同步更新 README.md
15. 每次修改必须同步更新：
    - docs/change-log.md
    - docs/phase-tracking.md
16. AI 必须从 README.md 开始逐层检索项目
17. AI 只能读取当前任务相关目录
18. 每个目录必须维护 README.md
```
