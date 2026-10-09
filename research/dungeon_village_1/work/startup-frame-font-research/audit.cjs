// 本机只读证据摘要：不导出源码正文、账户/存档或动态进程数据。
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const root=path.resolve(__dirname,'../..');
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const sourceWindows={
 'work/decompiled/sources/d/a.java':[[474,500],[2761,2804]],
 'work/decompiled/sources/kairo/android/ui/n.java':[[1,172]],
 'work/decompiled/sources/kairo/android/ui/o.java':[[167,244],[1013,1049]],
 'work/decompiled/sources/kairo/android/i/h.java':[[44,83],[125,141]]
};
const sources=Object.entries(sourceWindows).map(([p,windows])=>{
 const b=fs.readFileSync(path.join(root,p)),lines=b.toString('utf8').replace(/\r\n/g,'\n').split('\n');
 return {path:p,bytes:b.length,sha256:sha(b),windows:windows.map(([first,last])=>{
  if(first<1||last>lines.length)throw Error('源码窗口越界 '+p);
  return {first,last,normalized_utf8_sha256:sha(Buffer.from(lines.slice(first-1,last).join('\n')+'\n'))};
 })};
});
const assets=['common/img.inf','common/seb.inf','common/wnd_back.png','common/wnd_bar.png',
 'common/wnd_conner.png','common/wnd_conner.seb'].map(p=>{
 const b=fs.readFileSync(path.join(root,'assets/original',p));
 return {path:'assets/original/'+p,bytes:b.length,sha256:sha(b),
  ...(p.endsWith('.png')?{width:b.readUInt32BE(16),height:b.readUInt32BE(20)}:{})};
});
const b=fs.readFileSync(path.join(root,'assets/original/common/wnd_conner.seb'));
if(b.readInt16BE(0)!==1||b.readInt16BE(2)!==4||b.readInt16BE(4)!==4||b.length!==88)
 throw Error('原角SEB结构已变');
const corners=Array.from({length:4},(_,i)=>{
 const v=Array.from({length:10},(_,j)=>b.readInt16BE(8+i*20+j*2));
 return {frame:v[0],image:v[1],crop:v.slice(2,6),offset:v.slice(6,8),flip:v.slice(8,10)};
});
const output={date:'2026-10-09',scope:'固定APK1.0.8窗口图元与Android字体局部静态证据；无Steam动态认证',
 sources,assets,corners,sourceAssetBytes:assets.reduce((n,a)=>n+a.bytes,0),
 outputConsumer:'startup_world_visuals现有套件；可选760x930 CPU图元拼图；根集中验收'};
fs.writeFileSync(path.join(__dirname,'EVIDENCE.json'),JSON.stringify(output,null,2)+'\n');
console.log(JSON.stringify({sources:sources.length,assets:assets.length,bytes:output.sourceAssetBytes,corners:corners.length}));
