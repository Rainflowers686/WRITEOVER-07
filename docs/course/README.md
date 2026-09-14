# 课程交付入口

- [课程设计报告](COURSE_REPORT.md)：可编辑 Markdown，技术说明依据实际源代码。
- [测试报告与组间评价](TEST_REPORT.md)：机器证据与待真人填写的评分分开。
- [五份 Mermaid 图源](diagrams/README.md)：WBS、用例、主要类关系和两个算法图。
- [十页课堂 PPT](output/WRITEOVER07_Course_Presentation_v2.pptx)：文字和示意图可编辑，备注附来源。
- [五分钟演示路线](DEMO_5_MINUTES.md)：预算、话术和失败退路，不是实测通关时间。
- [真人验收清单](HUMAN_PLAYTEST_CHECKLIST.md)：终端、语言、音频、改键和首次试玩。
- [成员职责确认](MEMBER_RESPONSIBILITIES.md)：姓名学号和实际贡献由成员填写。

课程大纲保留在本目录。第 17 页要求组间测试分别给代码与功能各 50 分，不能由
自动化结果代填。第 9 页要求 Windows 到 ARM 目标的编译方法说明；当前 Linux
ARM64 CI 证明交叉链接和 ELF 架构，不证明鲲鹏真机部署。

PPT 构建源在 `scripts/course/build_presentation.mjs`。它使用本次提供的制品运行库，
不会修改游戏架构。重新导出要选择新文件名以保留已有结果。实际交付 PPT 已从
成品重新导入并渲染十页，检查了字形、边界与可编辑对象；未声称在 PowerPoint
桌面软件中完成现场投影验收。
