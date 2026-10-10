// 只读解析历史路径；新输出固定留在work，正式证据与原hash不重写。
import nativeFs from 'node:fs/promises';
import {statSync} from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const outputs=new Set();
function relativeWithin(directory,absolute){
 const relative=path.relative(path.join(root,directory),absolute);
 return relative==='..'||relative.startsWith('..'+path.sep)||path.isAbsolute(relative)?null:relative;
}
export function archiveInput(value){
 if(typeof value!=='string')return value;
 const absolute=path.resolve(value);if(outputs.has(absolute))return absolute;
 const relative=relativeWithin('work',absolute);
 if(relative!==null)for(const directory of ['tools','verification']){
  const candidate=path.join(root,directory,relative);
  if(statSync(candidate,{throwIfNoEntry:false})?.isFile())return candidate;
 }
 const toolRelative=relativeWithin('tools',absolute);
 if(toolRelative!==null&&!statSync(absolute,{throwIfNoEntry:false}))for(const directory of ['verification','work']){
  const candidate=path.join(root,directory,toolRelative);
  if(statSync(candidate,{throwIfNoEntry:false})?.isFile())return candidate;
 }
 return absolute;
}
export function archiveWork(url){return path.join(root,'work',path.basename(path.dirname(fileURLToPath(url))));}
function output(value){
 const absolute=path.resolve(value),relative=path.relative(path.join(root,'work'),absolute);
 if(relative==='..'||relative.startsWith('..'+path.sep)||path.isAbsolute(relative))throw Error('研究工具新输出必须位于work: '+absolute);
 outputs.add(absolute);
 return absolute;
}
export const archiveFs={...nativeFs,
 readFile:(file,...args)=>nativeFs.readFile(archiveInput(file),...args),
 stat:(file,...args)=>nativeFs.stat(archiveInput(file),...args),
 writeFile:(file,...args)=>nativeFs.writeFile(output(file),...args),
 copyFile:(from,to,...args)=>nativeFs.copyFile(archiveInput(from),output(to),...args),
 mkdir:(file,...args)=>nativeFs.mkdir(output(file),...args),
 mkdtemp:(file,...args)=>nativeFs.mkdtemp(output(file),...args),
 rm:(file,...args)=>nativeFs.rm(output(file),...args)
};
