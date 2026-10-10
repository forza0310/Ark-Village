const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 仅核已见方法引用的固定字符串／float槽及已盘点 ResourceManager 对象。
// 不导出字体或机器码；输出有限JSON，不写源文件。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto');
if(process.argv.length!==2)throw Error('no arguments');
const root=path.resolve(archivePaths.workDir,'../..'),game=path.join(root,'DungeonVillageEXE');
const dll=fs.readFileSync(path.join(game,'GameAssembly.dll'));
const meta=fs.readFileSync(path.join(game,'KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat'));
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
if(hash(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||hash(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('source identity');
const pe=dll.readUInt32LE(0x3c),count=dll.readUInt16LE(pe+6),opt=dll.readUInt16LE(pe+20),sections=[];
if(dll.readUInt32LE(pe)!==0x4550||dll.readUInt16LE(pe+24)!==0x10b||dll.readUInt32LE(pe+52)!==0x10000000||pe+24+opt+count*40>dll.length)throw Error('PE header');
for(let i=0;i<count;i++){const p=pe+24+opt+i*40;sections.push({rva:dll.readUInt32LE(p+12),size:dll.readUInt32LE(p+16),raw:dll.readUInt32LE(p+20)});}
function offset(va){const rva=va-0x10000000,s=sections.find(x=>rva>=x.rva&&rva+4<=x.rva+x.size);if(!s||s.raw+s.size>dll.length)throw Error('fixed data mapping');return s.raw+rva-s.rva;}
const literals=meta.readUInt32LE(8),literalBytes=meta.readUInt32LE(12),data=meta.readUInt32LE(16),dataBytes=meta.readUInt32LE(20);
if(literals+literalBytes>meta.length||data+dataBytes>meta.length)throw Error('metadata ranges');
const strings=[0x110fe874,0x11102450,0x11103058,0x111017f0,0x110f67a8,0x110e89f8].map(va=>{
 const at=offset(va),encoded=dll.readUInt32LE(at),usage=encoded>>>29,index=(encoded>>>1)&0x0fffffff;
 if(usage!==5||index*8+8>literalBytes)throw Error('string usage identity');
 const row=literals+index*8,length=meta.readUInt32LE(row),start=meta.readUInt32LE(row+4);
 if(length>128||start+length>dataBytes)throw Error('literal bounds');
 return {va,offset:at,encoded,index,metadata_offset:data+start,length,text:meta.toString('utf8',data+start,data+start+length)};
});
const constants=[0x10e27c1c,0x10e27c20,0x10dde550,0x10dde54c,0x10dde538,0x10dde5b4,0x10dde584].map(va=>({va,offset:offset(va),float32:dll.readFloatLE(offset(va)),bytes:dll.subarray(offset(va),offset(va)+4).toString('hex')}));
const coverage=JSON.parse(fs.readFileSync(path.join(root,'assets/IMAGE_COVERAGE.json'),'utf8'));
const rec=coverage.records.find(x=>x.class_id===147&&x.container==='globalgamemanagers');
const container=fs.readFileSync(path.join(game,'KairoGames_Data/globalgamemanagers'));
const containerRecord=coverage.containers.find(x=>x.kind==='UnitySerializedFile'&&x.path==='globalgamemanagers');
if(!containerRecord||container.length!==containerRecord.bytes||hash(container)!==containerRecord.sha256)throw Error('ResourceManager container identity');
const obj=container.subarray(rec.offset,rec.offset+rec.bytes);
if(obj.length!==rec.bytes||hash(obj)!==rec.sha256)throw Error('ResourceManager identity');
let at=4;const n=obj.readInt32LE(0),entries=[];
if(n<0||n>4096)throw Error('resource count');
for(let i=0;i<n;i++){
 if(at+4>obj.length)throw Error('resource name header');
 const length=obj.readInt32LE(at);at+=4;
 if(length<0||length>4096||at+length>obj.length)throw Error('resource name bounds');
 const name=obj.toString('utf8',at,at+length);at=(at+length+3)&~3;
 if(at+12>obj.length)throw Error('resource pointer bounds');
 const file_id=obj.readInt32LE(at),path_id=obj.readBigInt64LE(at+4).toString();at+=12;
 entries.push({name,file_id,path_id});
}
const duplicated=entries.filter((e,i)=>entries.findIndex(x=>x.name===e.name)!==i);
// 实际目录同一字体路径同时登记 Font/Material/Texture 对象；路径本身不是唯一键。
// 保留全部行，仅禁止完全相同的路径+对象引用，不能按名字覆盖掉字体候选。
if(new Set(entries.map(e=>JSON.stringify(e))).size!==entries.length)throw Error('duplicate resource path and pointer');
// Unity v22外部文件表：沿对象表顺序读取，不按路径名猜 file_id=4 的目标。
if(container.readUInt32BE(8)!==22)throw Error('Unity version');
const dataStart=Number(container.readBigUInt64BE(32));let cursor=48;
function take(n){if(!Number.isSafeInteger(n)||n<0||cursor+n>dataStart)throw Error('Unity metadata bounds');const p=cursor;cursor+=n;return p;}
function i32(){return container.readInt32LE(take(4));}
function size(){const n=i32();if(n<0||n>100000)throw Error('Unity metadata count');return n;}
function zeroString(){const end=container.indexOf(0,cursor);if(end<cursor||end>=dataStart||end-cursor>4096)throw Error('Unity external string');const s=container.toString('utf8',cursor,end);cursor=end+1;return s;}
zeroString();i32();const tree=container[take(1)];
for(let i=0,n=size();i<n;i++){
 const cid=i32();take(1);take(2);if(cid===114)take(16);take(16);
 if(tree){const nodes=size(),strings=size();take(nodes*32+strings);take(size()*4);}
}
for(let i=0,n=size();i<n;i++){cursor=(cursor+3)&~3;take(24);}
for(let i=0,n=size();i<n;i++){take(4);cursor=(cursor+3)&~3;take(8);}
const external=[];
for(let i=0,n=size();i<n;i++){zeroString();take(16);const type=i32(),name=zeroString();external.push({file_id:i+1,type,path:name});}
const resourceBytes=fs.readFileSync(path.join(game,'KairoGames_Data/resources.assets'));
const resourceRecord=coverage.containers.find(x=>x.kind==='UnitySerializedFile'&&x.path==='resources.assets');
if(!resourceRecord||resourceBytes.length!==resourceRecord.bytes||hash(resourceBytes)!==resourceRecord.sha256)throw Error('font container identity');
const fonts=entries.filter(e=>e.name.includes('font')||e.name.includes('default')).map(e=>{
 const file=external[e.file_id-1];if(!file||file.path!=='resources.assets')throw Error('font external identity');
 const obj=coverage.records.find(r=>r.container===file.path&&r.entry==='pathID='+e.path_id);
 if(!obj||obj.offset<0||obj.offset+obj.bytes>resourceBytes.length||hash(resourceBytes.subarray(obj.offset,obj.offset+obj.bytes))!==obj.sha256)throw Error('font object reference');
 return {...e,container:file.path,class_id:obj.class_id,object_sha256:obj.sha256};
});
console.log(JSON.stringify({dll_sha256:hash(dll),metadata_sha256:hash(meta),strings,constants,
 resources:{object:rec.id,offset:rec.offset,bytes:rec.bytes,sha256:rec.sha256,entries:n,table_end:at,unparsed_tail_bytes:obj.length-at,
 container_sha256:containerRecord.sha256,font_container_sha256:resourceRecord.sha256,
 duplicated_path_rows:duplicated.length,external_files:external,fonts}},null,2));
