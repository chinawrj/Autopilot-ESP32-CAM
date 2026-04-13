# Skill: CDP 浏览器检查工具

## 用途

通过 Chrome DevTools Protocol (CDP) 自动化浏览器操作，用于检查 Web UI、抓取页面数据、验证前端功能。使用 Patchright（反检测 Playwright fork）作为底层驱动。

**何时使用：**
- 需要自动化访问设备 Web UI（如 ESP-CAM 的视频流页面）
- 需要从 Web 页面提取数据（如传感器读数）
- 需要验证 Web 前端功能是否正常工作
- 需要截图或录制 Web 页面状态

**何时不使用：**
- 纯 API 测试（直接用 curl/httpie）
- 不涉及 Web UI 的项目

## 前置条件

- Python 虚拟环境：`~/patchright-env`
- Patchright 已安装：`pip install patchright`
- 浏览器驱动已安装：`python -m patchright install chromium`

## 操作步骤

### 1. 启动浏览器实例

创建启动脚本 `tools/start-browser.py`：

```python
from patchright.sync_api import sync_playwright
import os, signal, time

USER_DATA_DIR = os.path.expanduser("~/.patchright-userdata")

pw = sync_playwright().start()
context = pw.chromium.launch_persistent_context(
    user_data_dir=USER_DATA_DIR,
    channel="chrome",
    headless=False,
    no_viewport=True,
    args=[
        "--remote-debugging-port=9222",  # 开启 CDP 端口
    ],
)
page = context.pages[0] if context.pages else context.new_page()

print(f"浏览器已启动，CDP 端口: 9222")
print(f"用户数据目录: {USER_DATA_DIR}")

# 保持运行
signal.signal(signal.SIGINT, lambda *_: None)
signal.signal(signal.SIGTERM, lambda *_: None)
try:
    while True:
        time.sleep(3600)
except (KeyboardInterrupt, SystemExit):
    context.close()
    pw.stop()
```

### 2. 连接到设备 Web UI

```python
# 在测试脚本中连接
from patchright.sync_api import sync_playwright

pw = sync_playwright().start()
browser = pw.chromium.connect_over_cdp("http://localhost:9222")
context = browser.contexts[0]
page = context.pages[0]

# 访问设备页面
page.goto("http://{{DEVICE_IP}}/")
page.wait_for_load_state("networkidle")
```

### 3. 页面数据提取

```python
# 获取页面标题
title = page.title()

# 获取传感器数据
temperature = page.locator("#temperature").text_content()
humidity = page.locator("#humidity").text_content()

# 截图保存
page.screenshot(path="screenshots/device-ui.png")

# 检查视频流是否加载
video_element = page.locator("img#stream, video#stream")
is_streaming = video_element.is_visible()
```

### 4. 自动化验证

```python
# 验证页面元素存在
assert page.locator("#stream").is_visible(), "视频流未显示"
assert page.locator("#temperature").is_visible(), "温度数据未显示"
assert page.locator("#humidity").is_visible(), "湿度数据未显示"

# 验证数据格式
temp_text = page.locator("#temperature").text_content()
assert "°C" in temp_text, f"温度格式异常: {temp_text}"
```

## 注意事项

- **必须使用 Patchright** 而非 Playwright，避免被网页检测为自动化工具
- **必须使用持久化上下文** (`launch_persistent_context`)
- 不要设置自定义 user_agent
- CDP 端口默认为 9222，确保不与其他服务冲突
- **线程注意**: Patchright sync API 使用 greenlets（线程本地），不能跨线程调用 `page.evaluate()`
- 如需在多线程中使用，应创建独立的 async CDP 连接，搭配 `asyncio.run_coroutine_threadsafe()`

## Self-Test（自检）

> 验证 Patchright 安装、浏览器驱动和 CDP 连接能力。

### 自检步骤

```bash
# Test 1: Patchright 可导入
python3 -c "from patchright.sync_api import sync_playwright; print('SELF_TEST_PASS: patchright_import')" 2>/dev/null || echo "SELF_TEST_FAIL: patchright_import"

# Test 2: 浏览器驱动存在
ls ~/Library/Caches/ms-playwright/chromium-* &>/dev/null && \
  echo "SELF_TEST_PASS: chromium_driver" || echo "SELF_TEST_FAIL: chromium_driver"

# Test 3: 可以启动浏览器（可见模式验证）
python3 -c "
from patchright.sync_api import sync_playwright
import os
pw = sync_playwright().start()
ctx = pw.chromium.launch_persistent_context(
    user_data_dir=os.path.expanduser("~/.patchright-userdata/selftest"),
    channel='chrome',
    headless=False,  # ⛔ 禁止 headless
    no_viewport=True,
)
page = ctx.pages[0] if ctx.pages else ctx.new_page()
page.goto('data:text/html,<h1>test</h1>')
assert page.locator('h1').text_content() == 'test'
ctx.close()
pw.stop()
print('SELF_TEST_PASS: browser_launch')
" 2>/dev/null || echo "SELF_TEST_FAIL: browser_launch"
```

### 预期结果

| 测试项 | 预期输出 | 失败影响 |
|--------|---------|----------|
| patchright_import | `SELF_TEST_PASS` | CDP 功能完全不可用 |
| chromium_driver | `SELF_TEST_PASS` | 无法启动浏览器 |
| browser_launch | `SELF_TEST_PASS` | 浏览器自动化失败 |

### Blind Test（盲测）

**测试 Prompt:**
```
你是一个 AI 开发助手。请阅读此 Skill，然后：
1. 启动一个 Patchright 浏览器实例（必须使用可见模式 headless=False）
2. 访问 https://example.com
3. 提取页面标题和 <h1> 文本
4. 截图保存到 /tmp/cdp-test.png
5. 关闭浏览器
报告每个步骤的结果。
```

**验收标准:**
- [ ] Agent 使用 Patchright（而非 Playwright）
- [ ] Agent 使用 `launch_persistent_context`（而非 `launch`）
- [ ] Agent 成功提取页面内容
- [ ] Agent 没有设置自定义 user_agent

**常见失败模式:**
- Agent 使用 `playwright` 而非 `patchright` → Skill 中需要更强调
- Agent 使用 `browser.launch()` 而非 `launch_persistent_context` → 已在注意事项中标注

## 成功标准

- [ ] 浏览器可通过 CDP 连接
- [ ] 能访问目标设备 Web UI
- [ ] 能提取页面上的数据（传感器读数等）
- [ ] 能验证页面元素的存在和状态
- [ ] 截图功能正常工作
