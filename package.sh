#!/bin/bash
# KopiRPC 出包脚本: 把框架打成可分发的 tar.gz
# 产物: KopiRPC-<VERSION>.tar.gz = include/(全量头文件) + lib/libKopiRPC.a + USAGE.md + LICENSE
# 前置: 先跑 ./autobuild.sh(本脚本只打包不编译)
# 注意: example 编不编与本包无关(产物只取自 lib/),无需为此改动 CMakeLists
set -e

VERSION=0.1.0
PKG=KopiRPC-${VERSION}
ROOT=$(cd "$(dirname "$0")" && pwd)

# macOS 的 bsdtar 会把 ._ AppleDouble 垃圾文件打进包,Linux 下本变量无害
export COPYFILE_DISABLE=1

# 前置检查: 库与头文件副本都在 lib/ 下(autobuild.sh 的产出)
[ -f "$ROOT/lib/libKopiRPC.a" ] || { echo "缺 lib/libKopiRPC.a —— 先跑 ./autobuild.sh"; exit 1; }
[ -d "$ROOT/lib/include" ]      || { echo "缺 lib/include/ —— 先跑 ./autobuild.sh"; exit 1; }
[ -f "$ROOT/docs/usage.md" ]    || { echo "缺 docs/usage.md(包内使用文档的单源)"; exit 1; }

# 临时目录拼装包结构,脚本退出时自动清理
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
