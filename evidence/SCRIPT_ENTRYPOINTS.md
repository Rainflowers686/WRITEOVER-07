# bootstrap / build / test / bench / smoke / package 入口说明

所有脚本位于 `repo_seed/scripts/`。从 `repo_seed/` 根目录调用（当前工作目录 = 仓库根）。

```powershell
# 1) 首次：验证工具链 + 编译内容 + configure debug
pwsh -File scripts/bootstrap.ps1

# 2) 构建
pwsh -File scripts/build.ps1                # debug
pwsh -File scripts/build.ps1 -Preset release

# 3) 测试（CTest 单元测试）
pwsh -File scripts/test.ps1                 # ctest --preset debug

# Gate 入口（文档 + 可执行检查；未实现项报告 NOT_READY，绝不 hardcoded PASS）
pwsh -File scripts/test.ps1 -Gate R0
pwsh -File scripts/test.ps1 -Gate Integration

# 4) 冒烟（build + tests + mapc + app --smoke）
pwsh -File scripts/smoke.ps1

# 5) 基准（结构校验 + 真实采样）
pwsh -File scripts/bench.ps1

# 6) 治理检查（forbidden / deps / public header drift）
pwsh -File scripts/contract_check.ps1

# 7) 打包 Release 目录 + zip（9/15 前按 RELEASE_GATE 使用）
pwsh -File scripts/package.ps1
```

## 直接命令（等价，供 CI / 报告引用）

```powershell
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
cmake --preset release
cmake --build --preset release
python tools/contentc/contentc.py --data-dir data --out-dir data
.\out\build\debug\Debug\mapc.exe data/rooms/room_01_calibration.woc
.\out\build\debug\Debug\writeover_app.exe --smoke --data-dir data
.\out\build\debug\Debug\writeover_bench.exe
powershell -ExecutionPolicy Bypass -File tools/contract_check\check_forbidden.ps1
powershell -ExecutionPolicy Bypass -File tools/contract_check\check_deps.ps1
powershell -ExecutionPolicy Bypass -File tools/contract_check\check_public_headers.ps1
```

## 约定

- 单元测试执行器：`writeover_tests.exe`（自研 harness，无外部框架）。
- `writeover_app --smoke`：真实终端后端 → 60 sim ticks → raycaster 渲染带 →
  原子写 `saves/smoke.wo07` → 恢复控制台 → exit 0。
- 所有 gate 对"未来内容"只做两项：可查项真查；未实现项报 `NOT_READY`。