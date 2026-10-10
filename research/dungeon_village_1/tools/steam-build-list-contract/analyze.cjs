const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 固定入口只读探针：用户授权建设21研究。无任意地址、跳表推导或文件写入。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto');
const exe=path.resolve(archivePaths.workDir,'../persistence-replay-analysis/exe');
const dll=fs.readFileSync(path.resolve(exe,'../../../DungeonVillageEXE/GameAssembly.dll'));
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
if(hash(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a')throw Error('DLL identity');
const methods=archivePaths.require(path.join(exe,'methods.json')).methods;
const windows=Object.freeze({
 init:{rva:0x30e570,offset:0x30d370,end:0x3123c0,bytes:1024},
 draw:{rva:0x352e20,offset:0x351c20,end:0x383210,bytes:1024},
 update:{rva:0x321990,offset:0x320790,end:0x3284b0,bytes:1024},
 touch:{rva:0x3160b0,offset:0x314eb0,end:0x316360,bytes:688},
 set:{rva:0x317600,offset:0x316400,end:0x317680,bytes:128},
 scroll:{rva:0x3284b0,offset:0x3272b0,end:0x3285e0,bytes:304}
});
const key=process.argv[2];if(process.argv.length!==3||!Object.hasOwn(windows,key))throw Error('fixed window name required');
const w=windows[key],m=methods.find(x=>x.rva===w.rva&&x.offset===w.offset);
if(!m||m.va!==0x10000000+w.rva||methods.some(x=>x.va>m.va&&x.va<0x10000000+w.end)||w.rva+w.bytes>w.end||w.offset+w.bytes>dll.length)throw Error('registered boundary');
const pe=dll.readUInt32LE(0x3c);if(pe+24>dll.length||dll.readUInt32LE(pe)!==0x4550)throw Error('PE header');
const count=dll.readUInt16LE(pe+6),optional=dll.readUInt16LE(pe+20),table=pe+24+optional;
if(optional<96||table+count*40>dll.length||dll.readUInt16LE(pe+24)!==0x10b||dll.readUInt32LE(pe+52)!==0x10000000)throw Error('PE range');
let mapped=false;for(let n=0;n<count;n++){const p=table+n*40,rva=dll.readUInt32LE(p+12),size=dll.readUInt32LE(p+16),off=dll.readUInt32LE(p+20),flags=dll.readUInt32LE(p+36);
if(w.rva>=rva&&w.rva+w.bytes<=rva+size&&w.offset===off+w.rva-rva&&off+size<=dll.length&&(flags&0x20000000)!==0)mapped=true;}
if(!mapped)throw Error('executable PE mapping');
const iced=archivePaths.require(path.resolve(exe,'../../local-tools/iced-x86-1.21.0/package'));
const names=new Map();for(const method of methods){if(!names.has(method.va))names.set(method.va,[]);names.get(method.va).push(method.type+'::'+method.signature);}
console.log(JSON.stringify({key,method:m.signature,va:m.va,bytes:w.bytes,sha256:hash(dll.subarray(w.offset,w.offset+w.bytes))}));
const decoder=new iced.Decoder(32,dll.subarray(w.offset,w.offset+w.bytes),iced.DecoderOptions.None);decoder.ip=BigInt(m.va);
const formatter=new iced.Formatter(iced.FormatterSyntax.Intel);
while(decoder.canDecode){const i=decoder.decode();if(Number(i.ip)+i.length>m.va+w.bytes){i.free();break;}const refs=i.isCallNear?names.get(Number(i.nearBranchTarget)):undefined;console.log('0x'+Number(i.ip).toString(16).toUpperCase(),formatter.format(i),refs?refs.slice(0,2).join(' | '):'');i.free();}
formatter.free();decoder.free();
