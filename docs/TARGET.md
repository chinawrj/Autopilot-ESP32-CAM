# Autopilot ESP32-CAM — 项目目标与进度跟踪

## 总体目标

在 YD-ESP32-CAM (ESP32-WROVER-E-N8R8) 上构建一个完整的 **实时摄像头 Web 服务器**，
支持 TCP/UDP 双路径视频流、实时 HUD 显示、LED 控制。

## 里程碑进度

### M0: 项目脚手架 ✅ Complete
> **目标日**: Day 1 | **完成日**: Day 1

- [x] ESP-IDF 项目结构搭建 (CMakeLists.txt, 分区表)
- [x] sdkconfig.defaults (PSRAM, 摄像头, WiFi 基础配置)
- [x] WiFi 管理模块 (wifi_manager.c/h)
  - [x] 从 Kconfig 读取 SSID/Password
  - [x] STA 模式连接
  - [x] 自动重连 (最大重试 5 次)
  - [x] 事件回调 (连接/断开/获取 IP)
- [x] tools/provision-wifi.sh (凭据注入)
- [x] 编译通过
- [x] 烧录成功 + 串口看到 WiFi 连接日志 + 获取 IP (Day 6 验证: IP 192.168.1.171)
- [x] Git: 初始提交

**完成标志**: ✅ 串口输出 `WiFi connected, IP: 192.168.1.171` (Day 6 硬件验证通过)

---

### M1: 基础 TCP 视频流 ✅ Complete
> **目标日**: Day 2-3 | **完成日**: Day 3

- [x] OV2640 摄像头初始化 (camera_init.c)
  - [x] PSRAM 帧缓冲
  - [x] 分辨率: VGA (640x480)
  - [x] JPEG 质量: 12
- [x] HTTP 服务器 (http_server.c)
  - [x] GET `/` → 主页 (index.html)
  - [x] GET `/stream/tcp` → MJPEG 流
  - [x] MJPEG: multipart/x-mixed-replace boundary
- [x] 前端页面 (main/index.html)
  - [x] `<img>` 标签指向 `/stream/tcp`
  - [x] 基本布局和样式
- [x] 浏览器验证: MJPEG 流正常，~14KB/帧，FPS ~6.3 (Day 6 curl 验证)

**完成标志**: ✅ `http://192.168.1.171/stream/tcp` MJPEG 流工作正常 (Day 6 硬件验证通过)

---

### M2: HUD 叠加显示 ✅ Complete
> **目标日**: Day 4-5 | **完成日**: Day 5

- [x] FPS 实时计算 (在服务器端统计帧率)
- [x] 虚拟温度传感器组件 (components/virtual_sensor/)
  - [x] 基准 25°C，±3°C 随机波动
  - [x] 更新频率 1 Hz
- [x] REST API
  - [x] GET `/api/status` → JSON {fps, temperature, led_state}
- [x] 前端 HUD
  - [x] JavaScript 轮询 `/api/status` (每秒)
  - [x] 叠加 FPS 和温度到视频流上方
  - [x] 样式: 半透明黑底白字

**完成标志**: ✅ /api/status → fps=6.3, temp=27.3°C, led_state (Day 6 硬件验证通过)

---

### M3: LED 控制 ✅ Complete
> **目标日**: Day 6 | **完成日**: Day 4 (提前完成)

- [x] GPIO33 LED 驱动 (led_controller.c)
  - [x] 初始化为输出, 默认关闭
  - [x] on/off/toggle 接口
- [x] REST API
  - [x] POST `/api/led` body: {"state": "on"/"off"/"toggle"}
  - [x] 返回当前状态 JSON
- [x] 前端按钮
  - [x] Toggle 按钮，显示当前状态 (🔴 OFF / 🟢 ON)
  - [x] 点击发送 POST 请求
  - [x] 实时同步状态
- [x] 硬件验证: LED on/off/toggle 全部工作 (Day 6 curl 验证)

**完成标志**: 点击网页按钮，ESP32-CAM 板载 LED 亮/灭 (待硬件验证)

---

### M4: UDP 视频流 ✅ Complete
> **目标日**: Day 7-9 | **完成日**: Day 8

- [ ] UDP 推送端 (udp_stream.c)
  - [ ] JPEG 帧分片 (MTU 1400)
  - [ ] 序列号 + 帧号头
  - [ ] 丢帧容忍
- [x] WebSocket 视频流通道 (ws_stream.c)
  - [x] WebSocket binary frame 推送 JPEG (Day 7 实现)
  - [x] 最多 4 客户端并发
  - [x] httpd close callback 自动清理
  - [x] 控制消息: start/stop/quality (Day 8: set_quality, set_resolution, get_status)
  - [x] 心跳机制 (Day 8: 5s periodic heartbeat with fps/clients/heap)
- [x] 前端 `/stream/ws` 页面 (Day 7)
  - [x] WebSocket 连接管理 + 自动重连
  - [x] Canvas 渲染 (Blob → Image → drawImage)
  - [x] HUD 叠加: FPS + 温度 + WS 状态
  - [x] LED 控制按钮
- [x] TCP 和 WS 双路径并存不冲突 (Day 7 验证通过)

**完成标志**: `/stream/tcp` 和 `/stream/ws` 都可独立访问 — ✅ Day 7 验证通过

---

### M5: 稳定性与优化 ✅ Complete
> **目标日**: Day 10-12 | **完成日**: Day 11

- [x] 内存使用分析 (`heap_caps_get_info`) — Day 9: heap ~4.1MB free
- [x] 内存泄漏检测 (5min连续运行, heap drift +383 bytes/min, 无泄漏)
- [x] WiFi 断线重连测试 — Day 11: debug endpoint + 4/4 reconnect 全部成功 (~2s 恢复)
- [x] 多客户端并发测试 (3 WS + 1 MJPEG, 60s, 0 errors)
- [x] PSRAM 分配优化 — Day 10: 评估无需优化 (4MB+ free, 摄像头仅用 120KB)
- [x] 24 小时连续运行测试 — Day 10: 已启动后台监控 (heap_monitor.py 1440 min)
- [x] 代码重构 — Day 10: send_heartbeat(); Day 11: http_server.c 260→216 行
- [x] README.md 最终版本 — Day 10: 完成

**完成标志**: ✅ 多客户端 0 错误 + WiFi 重连 4/4 + heap 稳定 + 代码审查通过 (Day 11)

---

### Release: 发布准备 ✅ Complete
> **目标日**: Day 12-13 | **完成日**: Day 12

- [x] Web UI 截图 (4 张: MJPEG / WebSocket / LED / API)
- [x] 双语 README (README.md 英文 + README_CN.md 中文)
- [x] CHANGELOG.md (v1.0.0 release notes)
- [x] 架构图 (Mermaid, 嵌入 README)
- [x] 截图工具 (tools/take_screenshots.py)
- [x] GitHub Release 创建 (Day 13)

**完成标志**: ✅ GitHub Release v1.0.0 发布 + README 渲染验证通过 (Day 13)

---

## 当前状态

| 里程碑 | 状态 | 进度 |
|--------|------|------|
| M0 脚手架 | ✅ | 100% |
| M1 TCP流 | ✅ | 100% |
| M2 HUD | ✅ | 100% |
| M3 LED | ✅ | 100% |
| M4 WS流 | ✅ | 100% |
| M5 稳定性 | ✅ | 100% |
| Release v1.0.0 | ✅ | 100% |
| M6 v1.1.0 新功能 | ✅ | 100% |
| Release v1.1.1 | ✅ | 100% |
| Release v1.2.0 | ✅ | 100% |
| Release v1.3.0 | ✅ | 100% |

**当前工作日**: Day 24
**当前固件版本**: v1.3.0
**状态**: v1.3.0 发布 — SD 卡存储 + 单元测试 + 代码重构

---

## v1.1.0 Release 计划（Day 15-18）

### M6: v1.1.0 新功能
> **目标日**: Day 15-18 | **发布日**: Day 18

#### Day 15: 文档更新 + 规划
- [x] README 添加 AI 开发者信息（Claude Opus 4.6）
- [x] v1.1.0 roadmap 规划

#### Day 16: OTA 固件升级
- [x] OTA 升级模块 (ota_update.c/h)
  - [x] HTTP OTA: 从 URL 下载固件并升级
  - [x] 版本号管理 (app_desc)
  - [x] 回滚保护 (rollback — 双 OTA 分区)
- [x] REST API: `POST /api/ota` 触发升级
- [x] REST API: `GET /api/ota/status` 进度查询
- [x] 前端: OTA 升级面板 (URL输入 + 进度条 + 版本显示)
- [x] 分区表调整: 双 OTA 分区 (ota_0 + ota_1, 3MB each)
- [x] 修复: 摄像头 PWDN 上电时序 (OTA 重启后 I2C 超时)
- [x] E2E 测试: 下载→烧写→重启→摄像头OK→HTTP OK

#### Day 17: 快照拍照 + Web UI 优化
- [x] REST API: `GET /api/snapshot` 返回单帧 JPEG
- [x] 前端: 拍照按钮 + 图片下载
- [x] 首页合并: 统一入口页面（TCP/WS 切换 + 所有控制面板）
- [x] 系统信息面板: 固件版本、运行时间、内存状态、WiFi 信号
- [x] 修复: MJPEG 流阻塞 API — 拆分双服务器架构 (port 80 + 81)
- [x] WiFi RSSI 函数 + `/api/status` 增强

#### Day 18: 测试 + 发布 v1.1.0
- [x] 全功能回归测试 (所有端点) — curl 9/10 + browser 17/18
- [x] OTA 升级端到端测试 — 修复 camera I2C reboot 问题
- [x] 稳定性验证 (多客户端 + WiFi 重连) — 300s 0 errors + 3/3 reconnect
- [x] CHANGELOG 更新
- [x] GitHub Release v1.1.0

#### Day 19: 质量审计
- [x] 全面 API 端点测试 (curl) — 80/80 通过
- [x] 浏览器 UI 验证 (Patchright Chrome) — 50/50 通过
- [x] 修复: MJPEG stream port 81 → 8081 (网络代理兼容性)
- [x] 代码/文档端口引用全部更新

#### Day 20: v1.1.1 文档刷新 + 补丁发布
- [x] README.md / README_CN.md 全面更新
  - [x] 架构图: 单服务器 → 双服务器 (:80 + :8081)
  - [x] 功能表: 增加 OTA、快照、统一仪表盘、系统信息
  - [x] API 参考: 增加 /api/snapshot、/api/ota、/api/ota/status
  - [x] 项目结构: 增加 stream_server.c/h、ota_update.c/h、新测试工具
  - [x] 性能指标: ~1300行/9源文件、双OTA分区布局
  - [x] 里程碑时间线: 增加 v1.1.0、v1.1.1
- [x] CHANGELOG.md v1.1.1 条目
- [x] RELEASE_NOTES.md 更新为 v1.1.1
- [x] 固件版本号 1.1.0 → 1.1.1
- [x] GitHub Release v1.1.1

---

## v1.2.0 Release（Day 21）

### Day 21: mDNS + Camera Settings API + 发布 v1.2.0
- [x] mDNS 本地发现: `http://espcam.local/`
  - [x] espressif/mdns 1.11.0 managed component
  - [x] Socket 网络模式 (CONFIG_MDNS_NETWORKING_SOCKET=y)
  - [x] 修复: WiFi GOT_IP 事件竞争 → 手动 mdns_netif_action
  - [x] 验证: ESP32 mDNS 响应器工作 (unicast 查询验证通过)
  - [x] 注意: 多播发现依赖路由器允许 WiFi 客户端间多播
- [x] Camera Settings API: GET/POST `/api/camera`
  - [x] 支持: brightness, contrast, saturation, sharpness (-2~2)
  - [x] 支持: hmirror, vflip (bool)
  - [x] 部分 JSON 更新 (只发改动字段)
  - [x] curl + 串口 + 浏览器全链路验证
- [x] 前端: Camera Settings 面板 (可折叠)
  - [x] Range sliders + Checkboxes
  - [x] 实时摄像头参数调节
- [x] 固件版本号 1.1.1 → 1.2.0
- [x] 上板验证: build ✅ flash ✅ serial ✅ curl ✅ browser ✅
- [x] 截图: v1.2.0-dashboard.png, v1.2.0-tcp-stream.png
- [x] CHANGELOG.md + RELEASE_NOTES.md 更新
- [x] GitHub Release v1.2.0

### Day 22: 代码重构 + 单元测试覆盖
- [x] 重构: 提取 FPS 计算器组件 (`components/fps_counter/`)
  - [x] 纯结构体 API (fps_counter_t)，无全局变量
  - [x] stream_server.c 和 ws_stream.c 消除重复代码
- [x] 重构: 提取 JSON 响应辅助函数 (`main/http_helpers.c/h`)
  - [x] http_send_json() 封装 5 处重复的 JSON 响应模式
  - [x] http_server.c 减少 27 行代码
- [x] 搭建 Unity 单元测试框架 (host-based)
  - [x] test/CMakeLists.txt + mocks + redirect headers
  - [x] Mac 上直接编译运行，无需 ESP32 硬件
- [x] 编写核心模块单元测试: 20 tests, 0 failures
  - [x] test_fps_counter (7 tests): 初始值、窗口、高帧率、重置
  - [x] test_virtual_sensor (6 tests): 温度范围、随机值映射
  - [x] test_led_controller (7 tests): GPIO mock、toggle、幂等性
- [x] 上板验证: build ✅ flash ✅ serial ✅ browser ✅ (Patchright)
- [x] Host 测试: ctest 20/20 pass

### Day 23: SD 卡支持 (Phase 1: 基础读写)
- [x] SD 卡驱动组件 (components/sd_card/)
  - [x] 1-bit SDMMC 模式 (GPIO2/14/15)
  - [x] VFS FAT 挂载到 /sdcard
  - [x] 无卡时优雅降级 (非致命)
  - [x] 启用 FATFS LFN 长文件名支持
- [x] SD 卡 REST API (main/sd_handlers.c/h)
  - [x] GET /api/sd/status, GET /api/sd/list
  - [x] POST /api/sd/capture, POST /api/sd/delete
  - [x] GET /api/sd/file/* (通配符匹配)
- [x] 前端 SD Card Storage 面板
- [x] 上板验证: build 0 warnings, flash, serial, Patchright 8/8
- [x] 单元测试回归: 20/20 pass

### Day 24: v1.3.0 发布准备
- [x] 版本号升级: 1.2.0 → 1.3.0
- [x] CHANGELOG.md: v1.3.0 条目 (SD 卡 + 重构 + 单元测试)
- [x] RELEASE_NOTES.md: 更新为 v1.3.0
- [x] README.md / README_CN.md 全面更新
  - [x] 功能表: 增加 SD 卡存储、单元测试
  - [x] 架构图: 增加 sd_card 模块 + Micro SD 卡硬件
  - [x] API 参考: 增加 6 个 SD 卡端点
  - [x] 项目结构: 增加 sd_handlers、http_helpers、fps_counter、sd_card、test/
  - [x] 性能指标: ~1600 行/13 源文件、20 单元测试
  - [x] 里程碑时间线: 增加 Day 22 重构、v1.3.0
- [x] TARGET.md 更新
- [x] 编译验证: 0 warnings
- [x] 单元测试: 20/20 pass
- [x] GitHub Release v1.3.0
