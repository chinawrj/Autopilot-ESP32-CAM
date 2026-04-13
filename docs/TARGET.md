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

### M4: UDP 视频流 ⬜ Not Started
> **目标日**: Day 7-9

- [ ] UDP 推送端 (udp_stream.c)
  - [ ] JPEG 帧分片 (MTU 1400)
  - [ ] 序列号 + 帧号头
  - [ ] 丢帧容忍
- [ ] WebSocket 控制通道
  - [ ] 客户端发送: {action: "start_udp", port: XXXX}
  - [ ] 服务端回送: {status, fps, frame_count}
  - [ ] 心跳机制
- [ ] 前端 `/stream/udp` 页面
  - [ ] WebSocket 连接管理
  - [ ] UDP 帧接收 + 重组 (WebRTC DataChannel 或 WebSocket fallback)
  - [ ] Canvas 渲染
- [ ] TCP 和 UDP 双路径并存不冲突

**完成标志**: `/stream/tcp` 和 `/stream/udp` 都可独立访问

---

### M5: 稳定性与优化 ⬜ Not Started
> **目标日**: Day 10-12

- [ ] 内存使用分析 (`heap_caps_get_info`)
- [ ] 内存泄漏检测 (连续运行 1 小时，heap 不持续增长)
- [ ] WiFi 断线重连测试 (模拟路由器重启)
- [ ] 多客户端并发测试 (至少 2 个浏览器同时观看)
- [ ] PSRAM 分配优化
- [ ] 24 小时连续运行测试
- [ ] 代码重构 (消除重复，提取公共模块)
- [ ] README.md 最终版本

**完成标志**: 24 小时无崩溃 + 代码审查通过

---

## 当前状态

| 里程碑 | 状态 | 进度 |
|--------|------|------|
| M0 脚手架 | ⬜ | 0% |
| M1 TCP流 | ⬜ | 0% |
| M2 HUD | ⬜ | 0% |
| M3 LED | ⬜ | 0% |
| M4 UDP流 | ⬜ | 0% |
| M5 稳定性 | ⬜ | 0% |

**当前工作日**: Day 0 (未开始)
**下一步**: 启动 Day 1 → M0 项目脚手架
