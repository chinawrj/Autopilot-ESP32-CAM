# Skill: 每日迭代开发

## 用途

驱动 Agent 以"每天一轮"的节奏推进项目。每次 Agent 被调用视为一个工作日。

## 工作日流程

### 1. 开始 (Morning)

```bash
# 检查当前进度
cat docs/TARGET.md
ls docs/daily-logs/

# 确定当前日期编号 (最大已有编号 + 1)
NEXT_DAY=$(ls docs/daily-logs/day-*.md 2>/dev/null | sort -V | tail -1 | grep -oE '[0-9]+' | tail -1)
NEXT_DAY=$((${NEXT_DAY:-0} + 1))
DAY=$(printf "day-%03d" $NEXT_DAY)
```

### 2. 计划

从 `docs/daily-logs/TEMPLATE.md` 复制模板，填写:
- 昨日回顾 (看上一个 day-NNN.md)
- 今日 2-3 个目标 + 验收标准
- 风险识别

### 3. 执行

每个任务的循环:
```
编写代码 → idf.py build → idf.py flash → idf.py monitor → 浏览器验证
    ↑                                                          │
    └──────────── 修复问题 ◀────────────────────────────────────┘
```

**规则:**
- 测试失败 → 必须修复后再继续下一任务
- 每个任务完成 → 立即 git commit
- 编译警告 → 当天必须消除

### 4. 结束 (Evening)

- 更新 `docs/daily-logs/day-NNN.md` 完成状态
- 更新 `docs/TARGET.md` 里程碑 checkbox
- git commit + push

### 5. 代码重构（自适应触发）

重构**不是固定周期**，而是在每日结束时根据**代码健康度指标**决定是否触发。

#### 触发条件（满足任一即触发次日重构日）

| 指标 | 阈值 | 检查方式 |
|------|------|---------|
| 编译警告数 | ≥ 3 | `idf.py build 2>&1 \| grep -c "warning:"` |
| 单文件行数 | ≥ 250 行 | `wc -l main/*.c components/**/*.c` |
| 单函数行数 | ≥ 40 行 | 人工/Agent 审查 |
| 重复代码块 | ≥ 2 处相似片段 | `grep -rn` 或 Agent 识别 |
| 连续功能开发天数 | ≥ 4 天无重构 | 检查 daily-logs |
| TODO/FIXME 累积数 | ≥ 5 | `grep -rn "TODO\|FIXME" main/ components/` |
| 内存泄漏趋势 | free heap 持续下降 | 对比最近 3 天 heap 数据 |

#### 重构日规则

- 🔧 **不增加新功能**，仅做代码改善
- 优先级: 编译警告 > 大文件拆分 > 重复代码提取 > 命名规范 > 注释完善
- 重构完成后必须: `idf.py build` 零警告 + 功能回归验证
- 在 daily-log 中记录重构内容和前后对比指标

#### 每日晚间健康度检查模板

```bash
# 在每天 Evening Review 时执行
echo "=== Code Health Check ==="
echo "Warnings: $(idf.py build 2>&1 | grep -c 'warning:' || echo 0)"
echo "Large files (>250 lines):"
find main/ components/ -name '*.c' -exec awk 'END{if(NR>250)print NR, FILENAME}' {} \;
echo "TODOs: $(grep -rn 'TODO\|FIXME' main/ components/ 2>/dev/null | wc -l)"
echo "Free heap: check serial monitor"
```

如果任一指标超过阈值，在 daily-log 的"明日计划"中标记: `⚠️ 触发重构日`


## 前置条件

- Git 已初始化
- `docs/TARGET.md` 存在（里程碑定义）
- `docs/daily-logs/TEMPLATE.md` 存在

## Self-Test（自检）

```bash
#!/bin/bash
SKILL="skills/daily-iteration/SKILL.md"

[ -f "$SKILL" ] && echo "SELF_TEST_PASS: skill_md_exists" || echo "SELF_TEST_FAIL: skill_md_exists"
grep -q "Morning" "$SKILL" && echo "SELF_TEST_PASS: has_morning_section" || echo "SELF_TEST_FAIL: has_morning_section"
grep -q "Evening" "$SKILL" && echo "SELF_TEST_PASS: has_evening_section" || echo "SELF_TEST_FAIL: has_evening_section"
grep -q "idf.py build" "$SKILL" && echo "SELF_TEST_PASS: has_build_step" || echo "SELF_TEST_FAIL: has_build_step"
grep -q "git commit" "$SKILL" && echo "SELF_TEST_PASS: has_commit_step" || echo "SELF_TEST_FAIL: has_commit_step"
grep -q "重构" "$SKILL" && echo "SELF_TEST_PASS: has_refactor_trigger" || echo "SELF_TEST_FAIL: has_refactor_trigger"

# 检查模板目录
[ -d "docs/daily-logs" ] && echo "SELF_TEST_PASS: daily_logs_dir" || echo "SELF_TEST_FAIL: daily_logs_dir"
```

### Blind Test（盲测）

**场景描述:**
AI Agent 首次接手项目，需要开始 Day 1 的开发工作。

**测试 Prompt:**
> 这是我 ESP32-CAM 项目的第一天开发。请按照 daily-iteration skill 的流程，生成 Day 1 的工作计划。项目目标是搭建 WiFi + HTTP 摄像头流媒体服务器。

**验收标准:**
- [ ] 生成了包含 Morning/Evening 的完整日志文档
- [ ] 目标具体可执行（不是模糊的"学习 XX"）
- [ ] 包含编译→烧录→监控→验证的执行循环
- [ ] 每个任务完成后要求 git commit

**常见失败模式:**
- Agent 跳过计划直接写代码 → Skill 需强调先写日志
- Agent 不做上板验证 → 需与 hw-test-verify skill 联动

## 成功标准

- [ ] 按 Morning → 执行 → Evening 流程完成一天
- [ ] 每个任务有明确的验收标准
- [ ] daily-log 文件已生成并记录
- [ ] git commit 至少一次
