// 固定字体方法入口的有界只读探针；不运行游戏，不读取原档，不输出指令文件。
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const exe=path.resolve(__dirname,'../persistence-replay-analysis/exe');
const dll=fs.readFileSync(path.resolve(exe,'../../../DungeonVillageEXE/GameAssembly.dll'));
const meta=fs.readFileSync(path.resolve(exe,'../../../DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat'));
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
if(hash(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||hash(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('source identity');
const methods=require(path.join(exe,'methods.json')).methods;
const windows=Object.freeze({
 select:{rva:0x77f4a0,offset:0x77e2a0,end:0x77fc90},
 update:{rva:0x780710,offset:0x77f510,end:0x780910},
 widthF:{rva:0x77ffb0,offset:0x77edb0,end:0x780620},
 width:{rva:0x780620,offset:0x77f420,end:0x780710},
 scale:{rva:0x77d810,offset:0x77c610,end:0x77d920},
 needScale:{rva:0x77daa0,offset:0x77c8a0,end:0x77dbc0},
 offset:{rva:0x77d920,offset:0x77c720,end:0x77d980},
 updateScale:{rva:0x77eb70,offset:0x77d970,end:0x77ebf0},
 init:{rva:0x77fc90,offset:0x77ea90,end:0x77fd50},
 construct:{rva:0x77f2f0,offset:0x77e0f0,end:0x77f450},
 staticInit:{rva:0x77edd0,offset:0x77dbd0,end:0x77f010},
 draw:{rva:0x77f450,offset:0x77e250,end:0x77f4a0}
});
const key=process.argv[2];
if(process.argv.length!==3||!Object.hasOwn(windows,key))throw Error('fixed method name required');
const w=windows[key],bytes=w.end-w.rva,m=methods.find(x=>x.rva===w.rva&&x.offset===w.offset&&x.type==='kairo.unity.ui.Font');
if(!m||bytes<=0||bytes>2048||m.va!==0x10000000+w.rva||!methods.some(x=>x.rva===w.end)||methods.some(x=>x.rva>w.rva&&x.rva<w.end)||w.offset+bytes>dll.length)throw Error('registered method boundary');
const pe=dll.readUInt32LE(0x3c);
if(pe+24>dll.length||dll.readUInt32LE(pe)!==0x4550)throw Error('PE header');
const count=dll.readUInt16LE(pe+6),optional=dll.readUInt16LE(pe+20),table=pe+24+optional;
if(optional<96||table+count*40>dll.length||dll.readUInt16LE(pe+24)!==0x10b||dll.readUInt32LE(pe+52)!==0x10000000)throw Error('PE range');
let mapped=false;
for(let n=0;n<count;n++){
 const p=table+n*40,rva=dll.readUInt32LE(p+12),size=dll.readUInt32LE(p+16),off=dll.readUInt32LE(p+20),flags=dll.readUInt32LE(p+36);
 if(w.rva>=rva&&w.end<=rva+size&&w.offset===off+w.rva-rva&&off+size<=dll.length&&(flags&0x20000000)!==0)mapped=true;
}
if(!mapped)throw Error('executable PE mapping');
const iced=require(path.resolve(exe,'../../local-tools/iced-x86-1.21.0/package'));
const names=new Map();for(const method of methods){if(!names.has(method.va))names.set(method.va,[]);names.get(method.va).push(method.type+'::'+method.signature);}
console.log(JSON.stringify({key,method:m.signature,rva:w.rva,offset:w.offset,next_rva:w.end,bytes,sha256:hash(dll.subarray(w.offset,w.offset+bytes))}));
const decoder=new iced.Decoder(32,dll.subarray(w.offset,w.offset+bytes),iced.DecoderOptions.None);decoder.ip=BigInt(m.va);
const formatter=new iced.Formatter(iced.FormatterSyntax.Intel);
while(decoder.canDecode){
 const i=decoder.decode();if(Number(i.ip)+i.length>m.va+bytes){i.free();break;}
 const refs=i.isCallNear?names.get(Number(i.nearBranchTarget)):undefined;
 console.log('0x'+Number(i.ip).toString(16).toUpperCase(),formatter.format(i),refs?refs.slice(0,2).join(' | '):'');i.free();
}
formatter.free();decoder.free();
