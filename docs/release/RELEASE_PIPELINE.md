# 发布流程

## 2026-09-15：发布已验证 Windows 候选包

这次是文档与既有包发布，不执行构建、测试、回放或重新打包。产品版本保持 `PRODUCT_VERSION` 中的 `0.1.0-complete-campaign-candidate`。

1. 对照包内 version.json、现有验证回执和 ZIP SHA-256。
2. 提交玩家文档及发布记录到 main，使用 [skip ci]，不改工作流。
3. 创建不存在的 candidate-0.1.0-complete-campaign-20260915 标签，目标为包实现提交 d2654f8974922a1e8db9d23e1ef99819cb5f5524。
4. 创建新的 GitHub Pre-release，上传原 ZIP、中文玩家指南和校验文件。
5. 只读核对发布属性、标签目标、附件大小／摘要与远端 main。保留旧标签、旧发布和原始验证证据。

标签不使用 v 前缀，因此不会触发旧 v* 自动打包流程。这条发布路径由用户明确要求“发布上一轮已验证包且本步不跑测试”而采用，不表示取消未来代码变更的测试要求。

当前版本与来源见 [VERSIONING.md](VERSIONING.md)，面向玩家的发布正文见 [本版说明](RELEASE_NOTES_0.1.0-complete-campaign-candidate.md)。

## 既有打包工具

PRODUCT_VERSION 是包版本来源。CMake 读取数值前缀，打包工具把完整值写进 version.json、README 和通常的归档文件名。

scripts/package_windows.ps1、scripts/package_linux.sh、scripts/package_macos.sh 负责平台构建与打包。运行前应先阅读脚本参数及当前门槛；不要在只授权文档的任务中运行它们。SkipBuild 不等于跳过所有包检查。

运行时包包含程序、编译内容、字符资产及元数据，不应含用户存档、作者源码、测试输出或凭据。原有 package_smoke.py 会在独立目录中验证内容查找和用户数据分离。

## 旧自动发布流程的限制

.github/workflows/release.yml 监听 v* 标签及手动触发。虽然它核对标签是否匹配 PRODUCT_VERSION，归档名、上传路径、校验命令、发布标题和说明文件仍写死了旧 0.1.0-pvs01-gold。

因此它目前不是新战役版本的可用自动发布入口。未来要发布重新构建的多平台包，应单独修正这些引用并验证，不要直接推新 v* 标签碰运气，也不要移动旧标签。

本次没有改动该工作流。Windows 签名、macOS 签名／公证仍未配置；ARM64 交叉链接通过不能替代真机试玩。
