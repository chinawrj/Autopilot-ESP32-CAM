#!/bin/bash
# 探测包含指定模块的 Python 解释器
# 用法: source skills/_common/detect-python.sh
#       PYTHON=$(detect_python "patchright.sync_api")

detect_python() {
  local module="${1:-patchright.sync_api}"

  # 1. 当前 python3
  if python3 -c "import $module" 2>/dev/null; then
    echo "python3"
    return 0
  fi

  # 2. 环境变量指定
  if [ -n "$PATCHRIGHT_PYTHON" ] && "$PATCHRIGHT_PYTHON" -c "import $module" 2>/dev/null; then
    echo "$PATCHRIGHT_PYTHON"
    return 0
  fi

  # 3. 自动探测常见路径
  local search_paths=(
    "$HOME/patchright-env/bin/python3"
    "$HOME/.venv/bin/python3"
    "$HOME/venv/bin/python3"
  )

  local dir="$PWD"
  for i in 1 2 3; do
    search_paths+=("$dir/.venv/bin/python3" "$dir/venv/bin/python3")
    dir=$(dirname "$dir")
  done

  for p in "${search_paths[@]}"; do
    if [ -x "$p" ] && "$p" -c "import $module" 2>/dev/null; then
      echo "$p"
      return 0
    fi
  done

  return 1
}
