#!/usr/bin/env bash
# 一键开发环境（不用记 docker 命令）
#
#   ./dev.sh                    进入 Linux 开发容器（交互，exit 退出）
#   ./dev.sh "cmake -B build"   在容器里执行一条命令就退出（适合快速编译）
#
set -euo pipefail
cd "$(dirname "$0")"

# 1. 确保 Linux 虚拟机在跑，没跑就启动
if ! colima status >/dev/null 2>&1; then
  echo "→ 启动 Linux 虚拟机（首次约 20 秒）..."
  colima start
fi

# 2. 把当前目录挂到容器的 /work，然后进 bash（或执行传入的命令）
if [ $# -eq 0 ]; then
  echo "→ 进入开发容器（输入 exit 退出）"
  exec docker run --rm -it -v "$PWD":/work -w /work frame-dev bash
else
  exec docker run --rm -v "$PWD":/work -w /work frame-dev bash -c "$*"
fi
