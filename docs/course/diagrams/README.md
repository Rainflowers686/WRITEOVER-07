# 图源与阅读说明

五份 Mermaid 源文件与课程报告配套，可在支持 Mermaid 的编辑器中修改和渲染。
WBS 是工作领域分解，用例图表达玩家动作和具体交互关系，类图保留主要生产依赖，
两个算法图分别解释 DDA 与存取档事务。

类图的虚线表示依赖，带三角箭头的实现关系对应 C++ 接口。Engine 的模块指针
是非拥有引用，实际寿命由组装点控制；不要把调度关联解释成 Engine 持有所有对象。
用例图中的“具体交互场景”不是声明所有交互都必须先使用凭证。

技术权威分别为 composition_root.cpp、engine.h、tower_campaign_runtime.h、
raycaster.cpp、save.cpp 和 player_save.h。该图是主要架构解释，不是全量自动 UML。
