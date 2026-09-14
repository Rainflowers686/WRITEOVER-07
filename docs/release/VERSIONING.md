# 版本与发布来源

## 当前版本

`PRODUCT_VERSION` 仍为 `0.1.0-complete-campaign-candidate`，与已验证 ZIP 内的 `version.json` 一致。本次只完善文档和发布既有包，没有通过改版本文字暗示重新编译。

| 项目 | 值 |
|---|---|
| 产品版本 | 0.1.0-complete-campaign-candidate |
| 发布类型 | Windows x64 Pre-release |
| 标签 | candidate-0.1.0-complete-campaign-20260915 |
| 包实现提交／标签目标 | d2654f8974922a1e8db9d23e1ef99819cb5f5524 |
| 已通过 CI 的交付提交 | d5ef249adf54431e605603c3436000cc39421c36 |
| 已通过 CI 的运行 | [34872278294](https://github.com/Rainflowers686/WRITEOVER-07/actions/runs/34872278294) |
| ZIP | WRITEOVER-07-audit-candidate.zip |
| ZIP 字节数 | 609629 |

实现提交与已通过 CI 的交付提交，在 src、include、data、tests、tools、scripts、.github 和 CMakeLists.txt 上一致。后续文档提交不重新定义该二进制包的来源。

发布 SHA-256 见 [校验文件](SHA256SUMS_complete-campaign-candidate.txt)。ZIP 不变，中文玩家指南单独作为附件发布。仓库中的玩家说明模板更新只影响未来打包，不追改旧 ZIP。

## 标签与版本的区别

本次使用 `candidate-…` 标签，明确表示发布已验证的候选包。它指向包内记录的实现提交，不是文档提交。

旧自动发布流程监听 `v*`，但其中下载名和发布说明仍写死 PVS-01。本次不运行、不修改该流程，也不冒充完成了新的三平台构建。未来若恢复自动发布，先校正文档中列出的过时部分并单独验证。

旧 `v0.1.0-pvs01-gold` 标签及预发布保留。“Gold”是当时 PVS 范围的历史名称，不表示当前完整游戏已经完成真人验收。

## 状态

实现已达到 `READY_FOR_DSV4_1F_FINAL_AUDIT`。公开候选包供试玩，不等于 Product Gold、正式 1.0、课堂最终交付或人类验收通过。

本次文档和发布整理按用户要求不跑构建、单元测试、回放或 benchmark。提交使用 `[skip ci]`，不会把上一轮 CI 写成本轮新测试。

## 许可

当前仓库未附开源许可证。公开源码和提供试玩下载不应被解释为任意再分发或商业使用授权。第三方内容说明见 [THIRD_PARTY_NOTICES.txt](THIRD_PARTY_NOTICES.txt)。
