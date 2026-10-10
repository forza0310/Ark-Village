// 固定Steam怪物开放/介绍消费者，仅输出指定具名方法；不读取进程或改写原程序。
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=fs.readFileSync(path.join(root,'DungeonVillageEXE/GameAssembly.dll'));
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a')throw Error('固定DLL身份');
const ix=require('../persistence-replay-analysis/exe/methods.json');
const entries={reset:0x21c8d0,choose:0x27e850,change:0x2116c0};
const selected=process.argv[2];
if(process.argv.length!==3||!['manifest',...Object.keys(entries)].includes(selected))throw Error('固定具名入口');
const specs=Object.fromEntries(Object.entries(entries).map(([key,rva])=>{
 const m=ix.methods.find(v=>v.rva===rva),next=Math.min(...ix.methods.filter(v=>v.rva>rva).map(v=>v.rva));
 const n=next-rva,section=ix.sections.find(s=>rva>=s.rva&&next<=s.rva+s.rawSize);
 if(!m||!section||n<=0||n>8192||section.raw+rva-section.rva!==m.offset)throw Error('方法边界/预算');
 return [key,{...m,bytes:n,sha256:sha(dll.subarray(m.offset,m.offset+n))}];
}));
if(selected==='manifest'){console.log(JSON.stringify(specs,null,2));process.exit(0);}
const m=specs[selected],iced=require('../local-tools/iced-x86-1.21.0/package');
const decoder=new iced.Decoder(32,dll.subarray(m.offset,m.offset+m.bytes),iced.DecoderOptions.None);
decoder.ip=BigInt(m.va);const formatter=new iced.Formatter(iced.FormatterSyntax.Intel);
const names=new Map(ix.methods.map(v=>[v.va,v.type+'::'+v.signature]));
while(decoder.canDecode){const i=decoder.decode();if(i.isInvalid){i.free();break;}
 console.log(Number(i.ip).toString(16)+' '+formatter.format(i)+(i.isCallNear?' ; '+(names.get(Number(i.nearBranchTarget))||''):''));i.free();}
formatter.free();decoder.free();
