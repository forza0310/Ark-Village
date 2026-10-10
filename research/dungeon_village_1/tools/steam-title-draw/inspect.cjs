const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 标题固定方法入口探针：只读stdout，不保存指令全文，不跳到未经核验的中途地址。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto');
const exe=path.resolve(archivePaths.workDir,'../persistence-replay-analysis/exe');
const dll=fs.readFileSync(path.resolve(exe,'../../../DungeonVillageEXE/GameAssembly.dll'));
const metadata=fs.readFileSync(path.resolve(exe,'../../../DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat'));
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
if(hash(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||hash(metadata)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('source identity');
const methods=archivePaths.require(path.join(exe,'methods.json')).methods;
const windows=Object.freeze({
 wrapper:{rva:0x20a640,offset:0x209440,end:0x20a710,bytes:208},
 draw:{rva:0x20df30,offset:0x20cd30,end:0x210a30,bytes:2048},
 // 仍从方法入口顺序解码，扩大已知入口前缀；只抑制已审首段stdout，不从中途地址启动。
 logo:{rva:0x20df30,offset:0x20cd30,end:0x210a30,bytes:4096,print_after:2040},
 begin:{rva:0x20be00,offset:0x20ac00,end:0x20bf30,bytes:304},
 staticInit:{rva:0x20d9f0,offset:0x20c7f0,end:0x20ddd0,bytes:992},
 init:{rva:0x20adf0,offset:0x209bf0,end:0x20b580,bytes:1936},
 load:{rva:0x7b0b20,offset:0x7af920,end:0x7b0c30,bytes:272,type:'kairo.unity.ui.ResourceManager'},
 filename:{rva:0x7ae9e0,offset:0x7ad7e0,end:0x7aeaa0,bytes:192,type:'kairo.unity.ui.ResourceManager'},
 effective:{rva:0x7addd0,offset:0x7acbd0,end:0x7ade60,bytes:144,type:'kairo.unity.ui.ResourceManager'},
 ready:{rva:0x7aefd0,offset:0x7addd0,end:0x7af790,bytes:1984,type:'kairo.unity.ui.ResourceManager'},
 start:{rva:0x7af930,offset:0x7ae730,end:0x7b08b0,bytes:3968,type:'kairo.unity.ui.ResourceManager'},
 imageTask:{rva:0x7b9990,offset:0x7b8790,end:0x7bad90,bytes:2048,type:'.ResourceManager.<>c__DisplayClass60_0'},
 imageLoad:{rva:0x7b9990,offset:0x7b8790,end:0x7bad90,bytes:4096,print_after:2040,type:'.ResourceManager.<>c__DisplayClass60_0'},
 jarData:{rva:0x84a560,offset:0x849360,end:0x84a8e0,bytes:896,type:'kairo.unity.util.JarInflater'},
 jarSearch:{rva:0x84bbe0,offset:0x84a9e0,end:0x84bcf0,bytes:272,type:'kairo.unity.util.JarInflater'},
 extension:{rva:0x84a320,offset:0x849120,end:0x84a510,bytes:496,type:'kairo.unity.util.JarInflater'},
 languageFiles:{rva:0x717050,offset:0x715e50,end:0x7179e0,bytes:2448,type:'kairo.unity.io.AssetReader'},
 folder:{rva:0x80dee0,offset:0x80cce0,end:0x80df50,bytes:112,type:'kairo.unity.util.Language'},
 folderById:{rva:0x80df50,offset:0x80cd50,end:0x80dfb0,bytes:96,type:'kairo.unity.util.Language'},
 languageGet:{rva:0x80f0a0,offset:0x80dea0,end:0x80f3a0,bytes:768,type:'kairo.unity.util.Language'},
 resourceCtor:{rva:0x7b2610,offset:0x7b1410,end:0x7b29b0,bytes:928,type:'kairo.unity.ui.ResourceManager'},
 resourceInit:{rva:0x7b29b0,offset:0x7b17b0,end:0x7b2b20,bytes:368,type:'kairo.unity.ui.ResourceManager'}
});
const key=process.argv[2];if(process.argv.length!==3||!Object.hasOwn(windows,key))throw Error('fixed window name required');
const w=windows[key],m=methods.find(x=>x.rva===w.rva&&x.offset===w.offset&&x.type===(w.type||'form.TitleForm'));
if(!m||m.va!==0x10000000+w.rva||!methods.some(x=>x.rva===w.end)||methods.some(x=>x.rva>w.rva&&x.rva<w.end)||w.bytes<=0||w.bytes>4096||w.rva+w.bytes>w.end||w.offset+w.bytes>dll.length)throw Error('registered boundary');
const pe=dll.readUInt32LE(0x3c);if(pe+24>dll.length||dll.readUInt32LE(pe)!==0x4550)throw Error('PE header');
const count=dll.readUInt16LE(pe+6),optional=dll.readUInt16LE(pe+20),table=pe+24+optional;
if(optional<96||table+count*40>dll.length||dll.readUInt16LE(pe+24)!==0x10b||dll.readUInt32LE(pe+52)!==0x10000000)throw Error('PE range');
let mapped=false;for(let n=0;n<count;n++){const p=table+n*40,rva=dll.readUInt32LE(p+12),size=dll.readUInt32LE(p+16),off=dll.readUInt32LE(p+20),flags=dll.readUInt32LE(p+36);
if(w.rva>=rva&&w.rva+w.bytes<=rva+size&&w.offset===off+w.rva-rva&&off+size<=dll.length&&(flags&0x20000000)!==0)mapped=true;}
if(!mapped)throw Error('executable PE mapping');
const iced=archivePaths.require(path.resolve(exe,'../../local-tools/iced-x86-1.21.0/package'));
const names=new Map();for(const method of methods){if(!names.has(method.va))names.set(method.va,[]);names.get(method.va).push(method.type+'::'+method.signature);}
console.log(JSON.stringify({key,method:m.signature,rva:w.rva,offset:w.offset,next_rva:w.end,bytes:w.bytes,sha256:hash(dll.subarray(w.offset,w.offset+w.bytes))}));
const decoder=new iced.Decoder(32,dll.subarray(w.offset,w.offset+w.bytes),iced.DecoderOptions.None);decoder.ip=BigInt(m.va);
const formatter=new iced.Formatter(iced.FormatterSyntax.Intel);
while(decoder.canDecode){const i=decoder.decode();if(Number(i.ip)+i.length>m.va+w.bytes){i.free();break;}const refs=i.isCallNear?names.get(Number(i.nearBranchTarget)):undefined;if(Number(i.ip)>=m.va+(w.print_after||0))console.log('0x'+Number(i.ip).toString(16).toUpperCase(),formatter.format(i),refs?refs.slice(0,2).join(' | '):'');i.free();}
formatter.free();decoder.free();
