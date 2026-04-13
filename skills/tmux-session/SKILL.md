# Skill: tmux 多终端管理

## 用途

**所有终端操作必须通过 tmux 执行。** 不要直接在裸终端中运行编译、烧录、监控命令。

**何时使用：**
- 执行任何 idf.py 命令（build/flash/monitor）
- 运行串口监控
- 任何需要观察输出或等待完成的命令

**何时不使用：**
- 读写文件（直接用文件工具）
- git 操作（可直接执行）

## 前置条件

- `tmux` 已安装: `brew install tmux` (macOS) / `apt install tmux` (Linux)

## 操作步骤

### 1. 幂等创建项目会话

**每次工作开始前必须执行。** 会话已存在则跳过。

```bash
tmux has-session -t espcam 2>/dev/null || {
  tmux set-option -g history-limit 10000
  tmux new-session -d -s espcam
  tmux rename-window -t espcam:0 'build'
  tmux new-window -t espcam -n 'monitor'
  echo "[tmux] Session 'espcam' created"
}
tmux list-windows -t espcam -F '#{window_index}:#{window_name}'
```

### 2. 标准窗口

| 窗口 | 名称 | 用途 |
|------|------|------|
| 0 | build | 编译 + 烧录 |
| 1 | monitor | 串口监控 (idf.py monitor) |

### 3. 发命令 + 等完成 + 读退出码

**这是核心操作模式。** 每次执行命令都必须用此模式，不要 fire-and-forget。

```bash
# === 函数定义（每个 session 定义一次）===
tmux_exec() {
  local target="$1"
  local cmd="$2"
  local timeout="${3:-120}"
  local sentinel="__DONE_${RANDOM}__"

  tmux send-keys -t "$target" "$cmd; echo \"${sentinel}_EXIT_\$?\"" C-m

  local elapsed=0
  sleep 1
  while [ $elapsed -lt $timeout ]; do
    local output
    output=$(tmux capture-pane -t "$target" -p -S -1000)
    local match
    match=$(echo "$output" | grep -o "${sentinel}_EXIT_[0-9][0-9]*" | head -1)
    if [ -n "$match" ]; then
      local code
      code=$(echo "$match" | grep -o '[0-9]*$')
      echo "$output" | tail -30
      return "${code:-1}"
    fi
    sleep 2
    elapsed=$((elapsed + 2))
  done
  echo "[TIMEOUT] after ${timeout}s"
  return 124
}
```

### 4. 标准操作流程

#### 编译
```bash
tmux_exec "espcam:build" "idf.py build" 300
# 检查 $? — 0 表示成功
```

#### 烧录
```bash
# 先确认串口设备
SERIAL_PORT=$(ls /dev/cu.wchusbserial* /dev/cu.usbserial-* /dev/ttyUSB* 2>/dev/null | head -1)
tmux_exec "espcam:build" "idf.py -p $SERIAL_PORT flash" 120
```

#### 串口监控
```bash
# monitor 不会自己结束，用 send-keys 启动，用 capture-pane 读取
SERIAL_PORT=$(ls /dev/cu.wchusbserial* /dev/cu.usbserial-* /dev/ttyUSB* 2>/dev/null | head -1)
tmux send-keys -t espcam:monitor "idf.py -p $SERIAL_PORT monitor" C-m

# 等待几秒后读取输出
sleep 5
tmux capture-pane -t espcam:monitor -p -S -200 | tail -50
```

#### 停止监控
```bash
tmux send-keys -t espcam:monitor C-]  # idf.py monitor 的退出快捷键
# 如果不行
tmux send-keys -t espcam:monitor C-c
```

### 5. 读取编译/运行输出

```bash
# 最近 50 行（快速检查）
tmux capture-pane -t espcam:build -p -S -200 | tail -50

# 完整输出（定位编译错误）
tmux capture-pane -t espcam:build -p -S - > /tmp/build-output.txt
grep -n "error:" /tmp/build-output.txt

# 串口日志检查关键信息
tmux capture-pane -t espcam:monitor -p -S -500 | grep -iE "(wifi|connected|ip|error|panic|http|started)"
```

### 6. 超时/挂死恢复

```bash
# 发送 Ctrl+C
tmux send-keys -t espcam:build C-c
sleep 1

# 验证 shell 恢复
tmux send-keys -t espcam:build 'echo __READY__' C-m
sleep 1
tmux capture-pane -t espcam:build -p | grep -q __READY__ && echo "Shell OK" || echo "Shell stuck"

# 最后手段：杀窗口重建
tmux kill-window -t espcam:build
tmux new-window -t espcam -n 'build'
```
