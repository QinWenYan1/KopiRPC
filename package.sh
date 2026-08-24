#!/bin/bash
# KopiRPC 出包脚本(极简版): lib/ + USAGE + LICENSE → KopiRPC-<VERSION>.tar.gz
# 包内 4 件: include/(11 头文件) + lib/libKopiRPC.a + USAGE.md + LICENSE
# 前置: 先 ./autobuild.sh(产物 lib/ 现成);容器内运行
set -e

VERSION=0.1.0
PKG=KopiRPC-${VERSION}
ROOT=$(cd "$(dirname "$0")" && pwd)

# 防 macOS 的 bsdtar 把 ._ AppleDouble 垃圾文件打进包;Linux(容器)下本变量无害
export COPYFILE_DISABLE=1

# 前置检查: 库与头文件是 autobuild 的产出;usage.md 是包内容之一
[ -f "$ROOT/lib/libKopiRPC.a" ] || { echo "缺 lib/libKopiRPC.a —— 先跑 ./autobuild.sh"; exit 1; }
[ -d "$ROOT/lib/include" ]      || { echo "缺 lib/include/ —— 先跑 ./autobuild.sh"; exit 1; }
[ -f "$ROOT/docs/usage.md" ]    || { echo "缺 docs/usage.md(包内 USAGE.md 的单源)"; exit 1; }

# 临时目录拼包,脚本退出自动清理
STAGE=$(mktemp -d)
trap 'rm -rf "$STAGE"' EXIT
mkdir -p "$STAGE/$PKG/lib"

cp -r "$ROOT/lib/include" "$STAGE/$PKG/include"
cp "$ROOT/lib/libKopiRPC.a" "$STAGE/$PKG/lib/"
cp "$ROOT/docs/usage.md" "$STAGE/$PKG/USAGE.md"
cp "$ROOT/LICENSE" "$STAGE/$PKG/LICENSE"

rm -f "$ROOT/$PKG.tar.gz"
tar czf "$ROOT/$PKG.tar.gz" -C "$STAGE" "$PKG"

echo "打包完成: $ROOT/$PKG.tar.gz"
tar tzf "$ROOT/$PKG.tar.gz"
