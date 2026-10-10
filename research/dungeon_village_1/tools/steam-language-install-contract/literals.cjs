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
const strings=[0x110f0cb4,0x110f5ae0,0x110f6d14,0x110e9098,0x110e7030,0x110e9844,0x110f7750,0x110fb958,0x110fb91c,0x110ed6b4,0x110f6d3c,0x110f6d90,0x110e932c,0x110e9358,0x110e9434,0x110e9384,0x110e93b0,0x110e9408,0x110ec850,0x110ec900,0x110ec9dc,0x110ec984,0x110ec824,0x110eac70,0x110fee10,0x11102838,0x11102450,0x110e8b58,0x110e8a7c,0x110e8aa8,0x110e8b84,0x110e8b2c].map(va=>{
 const rva=va-index.base,s=index.sections.find(s=>rva>=s.rva&&rva+4<=s.rva+s.rawSize);
 if(!s||s.raw+s.rawSize>dll.length)throw Error('fixed slot range');
 const offset=s.raw+rva-s.rva,encoded=dll.readUInt32LE(offset),usage=encoded>>>29,i=(encoded>>>1)&0x0fffffff;
 if(usage!==5||i*8+8>literalBytes)throw Error('literal usage');
 const at=literals+i*8,length=meta.readUInt32LE(at),start=meta.readUInt32LE(at+4);
 if(length>128||start+length>dataBytes)throw Error('literal bounds');
 return {va,offset,encoded,index:i,metadata_offset:data+start,length,text:meta.toString('utf8',data+start,data+start+length)};
});
console.log(JSON.stringify({dll_sha256:hash(dll),metadata_sha256:hash(meta),strings},null,2));
