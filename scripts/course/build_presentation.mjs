// Editable course deck. Supply the bundled runtime and skill paths explicitly.
import fs from 'node:fs/promises';
import path from 'node:path';
import { pathToFileURL } from 'node:url';
const repo = process.cwd();
const modules = process.env.WRITEOVER_ARTIFACT_MODULES;
const skill = process.env.WRITEOVER_PRESENTATION_SKILL;
const python = process.env.WRITEOVER_ARTIFACT_PYTHON;
if (!modules || !skill || !python) throw new Error('Set the three WRITEOVER artifact runtime paths');
process.env.RUNTIME_NODE_MODULES = modules;
process.env.RUNTIME_NODE = process.execPath;
process.env.RUNTIME_PYTHON = python;
const {Presentation, PresentationFile} = await import(pathToFileURL(path.join(modules, '@oai/artifact-tool/dist/artifact_tool.mjs')));
const {finalizePresentation} = await import(pathToFileURL(path.join(skill, 'container_tools/artifact_tool_utils.mjs')));
const build = path.join(repo, 'out/course-presentation');
const output = path.join(repo, 'docs/course/output');
await fs.mkdir(build, {recursive:true});
await fs.mkdir(output, {recursive:true});
const pres = Presentation.create({slideSize:{width:1280,height:720}});
const font = 'Microsoft YaHei';
const colors = {bg:'#0C151C',text:'#E8EEF0',muted:'#B7C4C9',accent:'#ADCBC3'};
function text(slide, value, x, y, w, h, size=30, color=colors.text, bold=false) {
  const item=slide.shapes.add({geometry:'textbox',position:{left:x,top:y,width:w,height:h},fill:'none',line:{fill:'none',width:0}});
  item.text=value;
  item.text.style={typeface:font,fontSize:size,color,bold,autoFit:'none'};
  return item;
}
function slide(title, note) {
  const s=pres.slides.add();s.background.fill=colors.bg;
  text(s,title,64,38,1152,80,46,colors.text,true);
  s.speakerNotes.textFrame.setText(note);
  return s;
}
async function picture(s, relative, x,y,w,h) {
  s.images.add({blob:new Uint8Array(await fs.readFile(path.join(repo,relative))),contentType:'image/png',alt:'Production character-cell export of WRITEOVER-07',fit:'contain',position:{left:x,top:y,width:w,height:h}});
}
function box(s,label,x,y,w=320,h=100) {
  const b=s.shapes.add({geometry:'rect',position:{left:x,top:y,width:w,height:h},fill:'#152832',line:{fill:colors.accent,width:2}});
  b.text=label;b.text.style={typeface:font,fontSize:28,color:colors.text,alignment:'center',verticalAlignment:'middle',autoFit:'none'};
}
function arrow(s,x,y,w=64) {
  s.shapes.add({geometry:'rightArrow',position:{left:x,top:y,width:w,height:30},fill:colors.accent,line:{fill:'none',width:0}});
}
let s=slide('WRITEOVER-07','课程展示。游戏为单人字符第一人称探索与战斗。图来自 production character-cell export，不是原生终端截图。来源：docs/production/evidence/chapter01_creative_polish/optimization_v2_20260915/lift_front.png');
text(s,'字符构成的第一人称世界',64,126,1140,64,38,colors.accent);
await picture(s,'docs/production/evidence/chapter01_creative_polish/optimization_v2_20260915/lift_front.png',250,220,780,390);
text(s,'C++17 单机课程设计',64,638,1120,44,26,colors.muted);

s=slide('调查与通行','来源：data/scenes/recovery_scene.json，src/app/tower_campaign_runtime.cpp。目录的41层是设定，不是41个可玩关卡。讲解约30秒。');
text(s,'玩家在 B1 醒来，追查记录与权限。',64,152,1120,62,36);
text(s,'19 个可玩房间，流程抵达屋顶。',64,260,1120,62,36);
text(s,'调查、合作与武力留下不同后果。',64,368,1120,62,36);
text(s,'三种结局由实际取得的事实决定。',64,476,1120,62,36);
text(s,'设施目录中的 41 层属于世界设定。',64,610,1120,46,26,colors.muted);

s=slide('一条交互怎样留下后果','来源：composition_root.cpp，src/systemic/systemic.cpp，src/app/perception_feed.h。框与箭头是可编辑机制示意。演示时使用已经彩排的真实摄像头或非致命路线，不手改事实。');
box(s,'玩家操作设备',64,190);arrow(s,397,224);
box(s,'世界事实或系统事件',480,190);arrow(s,813,224);
box(s,'后续门禁与 NPC 读取',896,190);
text(s,'字幕和感知消息解释已发生的事。',64,390,1120,58,34);
text(s,'关闭消息不会取消摄像头、身体或警戒后果。',64,480,1120,92,32);

s=slide('引擎调度与应用组装','来源：include/writeover/core/engine.h；composition_root.cpp RegisterModule 调用。Engine 不拥有具体模块，组装点控制寿命。固定模拟顺序 Input、Player、World、AI、Narrative，Render 单独呈现。');
box(s,'RunComposition\n创建并连接模块',64,180,350,120);arrow(s,426,224,60);
box(s,'Engine\n固定 120 Hz 模拟',500,180,340,120);arrow(s,852,224,60);
box(s,'IEngineModule\n抽象更新接口',926,180,290,120);
text(s,'Input   Player   World   AI   Narrative',70,374,1140,60,34,colors.accent);
text(s,'RenderFrame 独立呈现，不用显示帧率推进剧情。',64,488,1120,100,32);

s=slide('DDA 射线与字符投影','来源：src/render/raycaster.cpp CastColumnRay，character_renderer.cpp。角点容差内两轴同时推进，高度边界生成遮挡段，整墙终止。每列遍历所穿过的格子，输出仍为CharCell。');
box(s,'下一条网格边界',64,190);arrow(s,397,224);
box(s,'比较两侧高度与墙体',480,190);arrow(s,813,224);
box(s,'遮挡段与投影',896,190);
text(s,'墙体决定空间，手工字符资产表现人物和武器。',64,390,1120,100,34);
text(s,'人物朝向由观察关系选取，侧面有独立姿态。',64,518,1120,88,30,colors.muted);

s=slide('存档失败时保留旧状态','来源：src/core/save.cpp，src/common/io.cpp，src/app/player_save.h，composition_root.cpp staged load/rollback。外层schema1，生产七节。主槽位和resume分别写入，不是跨文件事务。');
box(s,'解析与校验',64,180);arrow(s,397,214);
box(s,'临时状态预检',480,180);arrow(s,813,214);
box(s,'提交或回滚',896,180);
text(s,'保存先写临时文件，再原子替换目标。',64,378,1120,72,34);
text(s,'不预先删除旧存档，不把半截负载当作旧格式。',64,480,1120,100,32);
text(s,'成功读取后清除未来的暂态提示。',64,610,1120,45,26,colors.muted);

s=slide('中英文与可恢复的操作','来源：presentation_text.h，text_layout.h，player_product.h，product_keys.h。截图来源：docs/course/assets/case-file-zh.png，真实生产导出，非原生终端验收。');
await picture(s,'docs/course/assets/case-file-zh.png',570,150,640,410);
text(s,'同一进度切换语言',64,170,470,70,32);
text(s,'中文按显示列排版',64,286,470,70,32);
text(s,'改键需检查与确认',64,402,470,70,32);
text(s,'窗口过小暂停游玩',64,518,470,70,32);

s=slide('验证的分工','来源：scripts/run_qa.ps1，.github/workflows/ci.yml，docs/engineering/TEST_STRATEGY.md。本页解释覆盖，不把待完成的最终CI声明为PASS。当前已执行本地245项单元，正式结果以最终测试报告为准。');
text(s,'FAST_REQUIRED',64,158,540,60,36,colors.accent,true);
text(s,'主线、产品、存档、包和原负载性能',64,238,1140,65,32);
text(s,'EXTENDED',64,350,540,60,36,colors.accent,true);
text(s,'恢复路线、场景矩阵和全部结局组合',64,430,1140,65,32);
text(s,'终端观感、音频与首次试玩由真人填写。',64,590,1140,64,30,colors.muted);

s=slide('跨平台与课堂演示','来源：CMakePresets.json，cmake/toolchains/aarch64-linux-gnu.cmake，docs/course/DEMO_5_MINUTES.md。Linux ARM64 为交叉链接和ELF检查，没有鲲鹏真机运行证据。');
text(s,'Windows 负责完整产品回归。',64,152,1140,68,34);
text(s,'Linux 与 macOS 分别检查原生构建和运行。',64,254,1140,80,32);
text(s,'ARM64 检查交叉链接与 ELF 架构。',64,372,1140,70,32);
text(s,'五分钟演示使用准备好的进度展示上层与结局。',64,502,1140,100,32,colors.accent);

s=slide('成员分工与交换测试','来源：课程大纲第16至18页，docs/course/MEMBER_RESPONSIBILITIES.md。姓名学号和实际职责必须由本人确认。代码50分、功能50分由对方小组填写，不用自动化结果代填。');
text(s,'姓名、学号与实际职责待成员确认。',64,174,1140,90,34);
text(s,'代码质量 50 分，功能体验 50 分。',64,310,1140,80,34);
text(s,'每人准备解释一个模块、一条后果链和一次失败恢复。',64,446,1140,110,32);
text(s,'真人评分和试玩记录不能由自动测试代填。',64,620,1140,46,26,colors.muted);

const draft=path.join(build,'candidate.pptx');
await (await PresentationFile.exportPptx(pres)).save(draft);
for(let i=0;i<pres.slides.items.length;i++) {
  const blob=await pres.export({slide:pres.slides.items[i],format:'png',scale:1});
  await fs.writeFile(path.join(build,`slide-${i+1}.png`),new Uint8Array(await blob.arrayBuffer()));
}
await finalizePresentation({workspaceDir:repo,candidatePath:draft,
  finalPath:path.join(output,process.argv[2] || 'WRITEOVER07_Course_Presentation_v2.pptx'),
  pythonExecutable:python,
  integrityValidatorPath:path.join(skill,'container_tools/inspect_presentation_package_integrity.py'),
  layoutValidatorPath:path.join(skill,'container_tools/inspect_presentation_layout_geometry.py'),
  layoutArgs:['--expected-slide-size-emu','12192000,6858000','--validate-heading-fit'],
  fontPolicy:{basis:'design',families:[font]},verifyArtifactToolImport:true,
  receiptPath:path.join(build,(process.argv[2] || 'WRITEOVER07_Course_Presentation_v2.pptx')+'.validation.json')});
console.log('COURSE_DECK_EXPORTED=YES HUMAN_POWERPOINT_ACCEPTANCE=NOT_CLAIMED');
