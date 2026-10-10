const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 只保存具名源窗口/字段/资源摘要；不运行原DLL、不落盘反汇编实现。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto'),cp=archivePaths.require('child_process');
if(process.argv.length!==2)throw Error('no arguments');
const root=path.resolve(archivePaths.workDir,'../..'),read=p=>fs.readFileSync(path.join(root,p)),sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=read('DungeonVillageEXE/GameAssembly.dll'),meta=read('DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat');
const indexBytes=read('work/persistence-replay-analysis/exe/methods.json'),index=JSON.parse(indexBytes);
const dumpBytes=read('work/persistence-replay-analysis/exe/dumper/dump.cs'),dump=dumpBytes.toString('utf8');
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||sha(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369'||sha(dumpBytes)!=='1bf5680223c495b5d24aa5934831a17e9ecf03b95ba4ddb3ad4b9459c3c81912')throw Error('source identity');
const method_windows=['manifest','mapchip','scroll'].map(key=>JSON.parse(cp.execFileSync(process.execPath,[path.join(archivePaths.workDir,'inspect.cjs'),key],{encoding:'utf8',maxBuffer:100000}).split('\n')[0]));
const at=va=>{const rva=va-index.base,s=index.sections.find(s=>rva>=s.rva&&rva+4<=s.rva+s.rawSize);if(!s||s.raw+s.rawSize>dll.length)throw Error('source VA mapping');return s.raw+rva-s.rva;};
const literals=meta.readUInt32LE(8),literalBytes=meta.readUInt32LE(12),data=meta.readUInt32LE(16),dataBytes=meta.readUInt32LE(20);
if(literals+literalBytes>meta.length||data+dataBytes>meta.length)throw Error('metadata bounds');
const strings=[0x110f98e4,0x110f4384,0x110fa948,0x110fa3e0,0x110f70e8,0x11102660,0x110fb6b8].map(va=>{
 const offset=at(va),encoded=dll.readUInt32LE(offset),usage=encoded>>>29,i=(encoded>>>1)&0xfffffff;
 if(usage!==5||i*8+8>literalBytes)throw Error('literal usage');
 const p=literals+i*8,length=meta.readUInt32LE(p),start=meta.readUInt32LE(p+4);
 if(length>128||start+length>dataBytes)throw Error('literal range');
 return {va,offset,index:i,text:meta.toString('utf8',data+start,data+start+length),bytes:length};
});
const fields=[['SubForm','buildList_','0x168'],['SubForm','scrollValue_','0x88'],['SubForm','select_','0x90'],['SubForm','page_','0xD8'],['SubForm','frame_','0xA0'],['SubForm','frame2_','0xA4'],['TenantData','mapchipDataId_','0x3C'],['TenantData','mapchipPattern_','0x40'],['TenantData','haveNum_','0x88'],['TenantData','isBuildNow','0xA1'],['TenantData','isResident_','0x99'],['TenantData','SETPATTERN_MAPCHIP','0x48'],['TenantData','SETMAPCHIP_ADDRESS','0x4C'],['MapchipData','seb_','0x24'],['MapchipData','tenantDataId_','0x2C'],['BaseData','id_','0x8'],['UserData','isBuildNew','0x11C'],['AppData','resMapChip_','0x20'],['AppData','tenantData_','0x108'],['AppData','mapchipData_','0x10C']].map(([type,name,offset])=>{
 const re=new RegExp('public class '+type+'\\b[^\\n]*\\n\\{'),m=re.exec(dump);if(!m)throw Error('declared type');
 const end=dump.indexOf('// Methods',m.index),block=dump.slice(m.index,end),line=block.split('\n').find(l=>l.includes(' '+name+'; // '+offset)&&l.trim().endsWith(offset));
 if(!line)throw Error('declared field '+type+' '+name);
 return {type,name,offset,declaration:line.trim(),source_line:dump.slice(0,m.index+block.indexOf(line)).split('\n').length};
});
const coverageBytes=read('assets/IMAGE_COVERAGE.json'),records=JSON.parse(coverageBytes).records;
const resources=[];
for(const [inf,ids] of [['img.inf',[17,119,147,74,72,103,70,98]],['seb.inf',[3,12,21,81]]]){
 const source=records.find(r=>r.source==='EXE'&&r.group==='common'&&r.entry===inf);if(!source)throw Error('Steam common index');
 for(const id of ids){const target=source.index_targets.find(r=>r.index===id),record=records.find(r=>r.id===target?.record);if(!record)throw Error('Steam indexed resource');const local=read('assets/original/common/'+target.filename),equal=sha(local)===record.sha256;
  // 两个实际差异图片依赖只登记差异，不移除原7个直接源槽的逐字节同核断言。
  const expected_equal=inf==='seb.inf'||![103,98].includes(id);if(equal!==expected_equal)throw Error('published resource identity relation changed');
  const r={index_kind:inf,index:id,name:target.filename,modifiers:target.modifiers,source_id:record.id,sha256:record.sha256,bytes:record.bytes,published_apk_sha256:sha(local),published_bytes_equal:equal};
  if(equal)r.alias='assets/original/common/'+target.filename;else r.limit='Steam与APK图片字节不同；不得作为同字节alias消费，须另解析/发布Steam条目';
  if(target.filename.endsWith('.seb')){if(local.readInt16BE(0)!==1)throw Error('SEB single layer');const n=local.readInt16BE(4);if(n<0||8+n*20!==local.length)throw Error('SEB bounded records');r.frame_count=local.readInt16BE(2);r.image_ids=[...new Set(Array.from({length:n},(_,i)=>local.readInt16BE(10+i*20)))];r.first_part=Array.from({length:10},(_,i)=>local.readInt16BE(8+i*2));}
  resources.push(r);
 }
}
const out={scope:'Steam raw21完整局部分支；不等于完整SubForm或原窗口',sources:{dll:sha(dll),metadata:sha(meta),method_index:sha(indexBytes),dump:sha(dumpBytes),image_coverage:sha(coverageBytes)},method_windows,unique_prefix_bytes:method_windows.reduce((n,w)=>n+w.bytes,0),
 branch:{dispatch:0x10353248,entry:0x10353799,return:0x10354f8d,bytes:0x10354f8e-0x10353799,draw_mutation:'frame_先加一再min(3)；不是纯重绘'},strings,fields,resources,
 rows:{count:5,stride:37,preview:{offset:[10,25],size:[64,32],rgb:[190,242,230]},mapchip_anchor:[12,35],inversion:0,touch:{id:11,offset:[2,22],size:[165,37],value_type:0x20000,flag:16,margin:[10,0,0,0]}},
 tabs:{count:3,stride:57,touch:{id:3,offset:[0,2],size:[60,20],value_type:0x50000,margin:[0,0,30,0]}},
 scroll:{caller:{offset:[170,21],size:[4,189],maximum:'count-1',large_change:5},registered:[{id:12,value:0x40000,width:3,height:189,args:['count',5,0x20000],margin:[3,20,0,0]},{id:25,value:0,width:3,height:189,flag:4}]},
 limits:['未核Steam21 Init/Update业务与空行确认政策','未解Steam SETPATTERN_MAPCHIP/SETMAPCHIP_ADDRESS全部数值','默认公共素材字节同核不代替语言覆盖运行选择','Graphics原点/裁剪/事件最终OS映射另验'],scripts:['inspect.cjs','audit.cjs'].map(p=>({path:p,sha256:sha(fs.readFileSync(path.join(archivePaths.workDir,p)))}))};
fs.writeFileSync(path.join(archivePaths.workDir,'EVIDENCE.json'),JSON.stringify(out,null,2)+'\n');
console.log(JSON.stringify({windows:method_windows.length,unique_prefix_bytes:out.unique_prefix_bytes,raw21_bytes:out.branch.bytes,strings:strings.length,fields:fields.length,resources:resources.length}));
