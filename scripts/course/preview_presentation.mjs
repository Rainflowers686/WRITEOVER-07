import fs from 'node:fs/promises';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
const modules=process.env.WRITEOVER_ARTIFACT_MODULES;
if(!modules) throw new Error('Set WRITEOVER_ARTIFACT_MODULES to the bundled runtime');
const {FileBlob,PresentationFile}=await import(pathToFileURL(path.join(modules,'@oai/artifact-tool/dist/artifact_tool.mjs')));
const deck=await PresentationFile.importPptx(await FileBlob.load(process.argv[2] || 'docs/course/output/WRITEOVER07_Course_Presentation_v2.pptx'));
await fs.mkdir('out/course-presentation/final-preview',{recursive:true});
for(let i=0;i<deck.slides.items.length;i++) {
  const preview=await deck.export({slide:deck.slides.items[i],format:'png',scale:1});
  await fs.writeFile(`out/course-presentation/final-preview/slide-${i+1}.png`,new Uint8Array(await preview.arrayBuffer()));
}
console.log(`FINAL_PPTX_RENDERED_SLIDES=${deck.slides.items.length}`);
