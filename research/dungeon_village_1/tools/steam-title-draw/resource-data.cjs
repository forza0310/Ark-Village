const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 仅解已见资源加载消费者的固定字符串槽，引用冻结图像清单；不导出素材。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto');
if(process.argv.length!==2)throw Error('no arguments');
const root=path.resolve(archivePaths.workDir,'../..'),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=fs.readFileSync(path.join(root,'DungeonVillageEXE/GameAssembly.dll'));
const meta=fs.readFileSync(path.join(root,'DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat'));
if(hash(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||hash(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('source identity');
const index=JSON.parse(fs.readFileSync(path.join(root,'work/persistence-replay-analysis/exe/methods.json')));
const literals=meta.readUInt32LE(8),literalBytes=meta.readUInt32LE(12),data=meta.readUInt32LE(16),dataBytes=meta.readUInt32LE(20);
if(literals+literalBytes>meta.length||data+dataBytes>meta.length)throw Error('metadata range');
const strings=[0x110e7164,0x110fb91c,0x110e6ea0,0x110e708c,0x110e6fc4,0x110e6fec,0x110e6ef8,0x110e6f20,0x110e7138,0x11103478,0x11103420,0x11101b08].map(va=>{
 const rva=va-index.base,s=index.sections.find(s=>rva>=s.rva&&rva+4<=s.rva+s.rawSize);
 if(!s||s.raw+s.rawSize>dll.length)throw Error('fixed slot range');
 const offset=s.raw+rva-s.rva,encoded=dll.readUInt32LE(offset),usage=encoded>>>29,i=(encoded>>>1)&0x0fffffff;
 if(usage!==5||i*8+8>literalBytes)throw Error('literal usage');
 const at=literals+i*8,length=meta.readUInt32LE(at),start=meta.readUInt32LE(at+4);
 if(length>128||start+length>dataBytes)throw Error('literal bounds');
 return {va,offset,encoded,index:i,metadata_offset:data+start,length,text:meta.toString('utf8',data+start,data+start+length)};
});
const coveragePath=path.join(root,'assets/IMAGE_COVERAGE.json'),coverageBytes=fs.readFileSync(coveragePath),coverage=JSON.parse(coverageBytes);
const variants=coverage.records.filter(x=>x.container==='resources.assets:1138:title'&&x.entry.endsWith('title_logo.png'));
if(variants.length!==2)throw Error('review logo variant inventory');
console.log(JSON.stringify({qualification:'固定消费者字符串；Logo条目引用已核资产目录，不代表运行窗口当前选择',
 dll_sha256:hash(dll),metadata_sha256:hash(meta),strings,coverage_sha256:hash(coverageBytes),
 variants:variants.map(({id,entry,bytes,sha256,width,height,rgba_sha256})=>({id,entry,bytes,sha256,width,height,rgba_sha256}))},null,2));
