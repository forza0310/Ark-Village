const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// Steam raw81：复用具名冻结方法的有限分支；新helper从入口解码，仅stdout。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto');
const root=path.resolve(archivePaths.workDir,'../..'),sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=fs.readFileSync(path.join(root,'DungeonVillageEXE/GameAssembly.dll'));
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a')throw Error('source');
const ix=archivePaths.require('../persistence-replay-analysis/exe/methods.json');
const cached={draw:['steam-input-closure',0x3472b0],input:['steam-interaction-analysis',0x317940],init:['facility-animation-closure',0x30e570],cctor:['facility-animation-closure',0x3285e0]};
const slices={draw:['draw',0x1034e3c2,0x1034e830],actors:['draw',0x1034e830,0x1034e9cb],touch:['draw',0x103477da,0x10347825],input:['input',0x1031a662,0x1031a887],init:['init',0x1030f0e8,0x1030f11d],tables:['cctor',0x1032c350,0x1032c4a0],labels:['cctor',0x1032a4c0,0x1032a530]};
const fresh={prepare:[0x223920,0x224050],strength:[0x2d7cd0,0x2d87e0],actor:[0x30d8c0,0x30db80],cursor:[0x30a180,0x30a210],rate:[0x2a4780,0x2a47e0],count:[0x2a4620,0x2a4780],parabola:[0x2a43f0,0x2a44b0],cursor2:[0x2cfe40,0x2cff00]};
const key=process.argv[2];if(process.argv.length!==3||(!Object.hasOwn(slices,key)&&!Object.hasOwn(fresh,key)&&key!=='manifest'))throw Error('fixed key');
function readCached(name){const [dir,rva]=cached[name],file='work/'+dir+'/disassembly.json',b=fs.readFileSync(path.join(root,file)),m=JSON.parse(b).find(x=>x.method.rva===rva);if(!m||sha(dll.subarray(m.method.offset,m.method.offset+m.size))!==m.sha256)throw Error('cached method identity');return {m,file,b};}
if(key==='manifest'){
 for(const k of Object.keys(cached)){const {m,file,b}=readCached(k);console.log(JSON.stringify({key:k,source:file,source_sha256:sha(b),rva:m.method.rva,offset:m.method.offset,bytes:m.size,sha256:m.sha256,reused:true}));}
 for(const [k,[rva,end]] of Object.entries(fresh)){const m=ix.methods.find(x=>x.rva===rva);console.log(JSON.stringify({key:k,rva,offset:m.offset,bytes:end-rva,sha256:sha(dll.subarray(m.offset,m.offset+end-rva)),reused:false}));}process.exit(0);
}
if(Object.hasOwn(slices,key)){const [name,lo,hi]=slices[key],{m}=readCached(name);for(const line of m.lines){const match=/^([0-9a-f]+) ([0-9a-f]+)\s+(.*)$/.exec(line);if(!match)throw Error('line');const va=parseInt(match[1],16),b=Buffer.from(match[2],'hex');if(va<lo||va>=hi)continue;const at=m.method.offset+va-m.method.va;if(!dll.subarray(at,at+b.length).equals(b))throw Error('instruction');console.log(match[1]+' '+match[3]);}process.exit(0);}
const [rva,end]=fresh[key],m=ix.methods.find(x=>x.rva===rva),n=end-rva;
if(!m||n>4096||!ix.methods.some(x=>x.rva===end)||ix.methods.some(x=>x.rva>rva&&x.rva<end))throw Error('method boundary');
const s=ix.sections.find(s=>rva>=s.rva&&end<=s.rva+s.rawSize);if(!s||s.raw+rva-s.rva!==m.offset)throw Error('PE mapping');
const iced=archivePaths.require('../local-tools/iced-x86-1.21.0/package'),d=new iced.Decoder(32,dll.subarray(m.offset,m.offset+n),iced.DecoderOptions.None);d.ip=BigInt(m.va);
const f=new iced.Formatter(iced.FormatterSyntax.Intel),names=new Map(ix.methods.map(x=>[x.va,x.type+'::'+x.signature]));
while(d.canDecode){const i=d.decode();if(i.isInvalid){i.free();break;}console.log(Number(i.ip).toString(16)+' '+f.format(i)+(i.isCallNear?' ; '+(names.get(Number(i.nearBranchTarget))||''):''));i.free();}f.free();d.free();
