// 固定AppData静态表前缀；具名入口顺序解码，有限stdout，不写反汇编全文。
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=fs.readFileSync(path.join(root,'DungeonVillageEXE/GameAssembly.dll'));
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a')throw Error('source');
const ix=require('../persistence-replay-analysis/exe/methods.json'),m=ix.methods.find(x=>x.rva===0x266a60&&x.type==='main.AppData');
const n=0x102689e4-m.va;if(!m||m.offset!==0x265860||n>8192||!ix.methods.some(x=>x.rva===0x26b300)||ix.methods.some(x=>x.rva>m.rva&&x.rva<0x26b300))throw Error('method');
const section=ix.sections.find(s=>m.rva>=s.rva&&m.rva+n<=s.rva+s.rawSize);if(!section||section.raw+m.rva-section.rva!==m.offset)throw Error('PE mapping');
const key=process.argv[2];if(process.argv.length!==3||!['manifest','table'].includes(key))throw Error('fixed key');
if(key==='manifest'){console.log(JSON.stringify({method:m,bytes:n,sha256:sha(dll.subarray(m.offset,m.offset+n))}));process.exit(0);}
const ranges=[[0x102685ff,0x102686d3],[0x102689c7,0x102689e4]];
const iced=require('../local-tools/iced-x86-1.21.0/package'),d=new iced.Decoder(32,dll.subarray(m.offset,m.offset+n),iced.DecoderOptions.None);d.ip=BigInt(m.va);
const f=new iced.Formatter(iced.FormatterSyntax.Intel),names=new Map(ix.methods.map(x=>[x.va,x.type+'::'+x.signature]));
while(d.canDecode){const i=d.decode();if(i.isInvalid){i.free();break;}const va=Number(i.ip);if(ranges.some(([lo,hi])=>va>=lo&&va<hi))console.log(va.toString(16)+' '+f.format(i)+(i.isCallNear?' ; '+(names.get(Number(i.nearBranchTarget))||''):''));i.free();}f.free();d.free();
