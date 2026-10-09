// 设施74/81实际共用helper；固定具名入口、有限字节，stdout不保存反汇编全文。
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),sha=b=>crypto.createHash('sha256').update(b).digest('hex'),dll=fs.readFileSync(path.join(root,'DungeonVillageEXE/GameAssembly.dll'));
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a')throw Error('source');
const ix=require('../persistence-replay-analysis/exe/methods.json'),windows={bottom:[0x30a2c0,0x30a5f0],scroll:[0x2517b0,0x2518e0],money:[0x255210,0x2552a0],number:[0x24fcb0,0x24fd00],numberText:[0x24fd00,0x250150],plus:[0x255410,0x2554f0],comma:[0x24f760,0x24f980],separate:[0x24a210,0x24a330],fig:[0x2578c0,0x2579b0]};
const key=process.argv[2];if(process.argv.length!==3||(key!=='manifest'&&!Object.hasOwn(windows,key)))throw Error('fixed key');
const iced=require('../local-tools/iced-x86-1.21.0/package'),names=new Map(ix.methods.map(x=>[x.va,x.type+'::'+x.signature]));
for(const k of key==='manifest'?Object.keys(windows):[key]){const [rva,end]=windows[k],m=ix.methods.find(x=>x.rva===rva),n=end-rva;
if(!m||n>2048||!ix.methods.some(x=>x.rva===end)||ix.methods.some(x=>x.rva>rva&&x.rva<end))throw Error('method boundary');const s=ix.sections.find(s=>rva>=s.rva&&end<=s.rva+s.rawSize);if(!s||s.raw+rva-s.rva!==m.offset)throw Error('PE mapping');
if(key==='manifest'){console.log(JSON.stringify({key:k,method:m,bytes:n,sha256:sha(dll.subarray(m.offset,m.offset+n))}));continue;}
const d=new iced.Decoder(32,dll.subarray(m.offset,m.offset+n),iced.DecoderOptions.None);d.ip=BigInt(m.va);const f=new iced.Formatter(iced.FormatterSyntax.Intel);
while(d.canDecode){const i=d.decode();if(i.isInvalid){i.free();break;}console.log(Number(i.ip).toString(16)+' '+f.format(i)+(i.isCallNear?' ; '+(names.get(Number(i.nearBranchTarget))||''):''));i.free();}f.free();d.free();}
