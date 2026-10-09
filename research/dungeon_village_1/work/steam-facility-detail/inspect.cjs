// Steam设施74局部证据：复用已解码具名方法，新增Init2顺序解码；只读源、仅stdout。
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=fs.readFileSync(path.join(root,'DungeonVillageEXE/GameAssembly.dll'));
const meta=fs.readFileSync(path.join(root,'DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat'));
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||sha(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('source identity');
const index=JSON.parse(fs.readFileSync(path.join(root,'work/persistence-replay-analysis/exe/methods.json'))),methods=index.methods;
const cached={draw:['steam-input-closure',0x342c30],update:['steam-interaction-analysis',0x317940]};
const slices={layout:['draw',0x103453b8,0x10345922],neighbors:['draw',0x10345922,0x1034608e],main:['draw',0x1034608e,0x10346800],main2:['draw',0x10346800,0x1034728b],input:['update',0x1031b3b0,0x1031b880],returns:['update',0x1031f640,0x1031f681],previewReturn:['update',0x1031fc7a,0x1031fc96]};
const fresh={init2:[0x30e570,0x3123c0,[[0x1030e66c,0x1030e711],[0x1030f8c3,0x1030fc58]]],mapchip2:[0x21b9c0,0x21bca0],arrow:[0x30db80,0x30dcc0],icon:[0x30c350,0x30ce30,[[0x1030c350,0x1030c432],[0x1030c671,0x1030c6fd],[0x1030c762,0x1030c7fb]]],scene:[0x303430,0x305d20,[[0x103053d0,0x10305985]]]};
const key=process.argv[2];if(process.argv.length!==3||(!Object.hasOwn(slices,key)&&!Object.hasOwn(fresh,key)&&key!=='manifest'))throw Error('fixed key required');
function readCached(name){const [dir,rva]=cached[name],file='work/'+dir+'/disassembly.json',b=fs.readFileSync(path.join(root,file)),m=JSON.parse(b).find(x=>x.method.rva===rva);if(!m||sha(dll.subarray(m.method.offset,m.method.offset+m.size))!==m.sha256)throw Error('cached method bytes');return {m,file,b};}
if(key==='manifest'){
  for(const name of Object.keys(cached)){const {m,file,b}=readCached(name);console.log(JSON.stringify({key:name,source:file,source_sha256:sha(b),rva:m.method.rva,offset:m.method.offset,bytes:m.size,sha256:m.sha256,reused:true}));}
  for(const [k,[rva,end]] of Object.entries(fresh)){const m=methods.find(m=>m.rva===rva),n=end-rva;console.log(JSON.stringify({key:k,rva,offset:m.offset,bytes:n,sha256:sha(dll.subarray(m.offset,m.offset+n)),reused:k==='scene'}));}process.exit(0);
}
if(Object.hasOwn(slices,key)){
  const [name,lo,hi]=slices[key],{m}=readCached(name);
  for(const line of m.lines){const match=/^([0-9a-f]+) ([0-9a-f]+)\s+(.*)$/.exec(line);if(!match)throw Error('cached line');const va=parseInt(match[1],16),b=Buffer.from(match[2],'hex');if(va<lo||va>=hi)continue;const at=m.method.offset+va-m.method.va;if(!dll.subarray(at,at+b.length).equals(b))throw Error('cached instruction');console.log(match[1]+' '+match[3]);}process.exit(0);
}
const [rva,end,ranges]=fresh[key],m=methods.find(m=>m.rva===rva),n=end-rva;
if(!m||m.va!==0x10000000+rva||n>16384||!methods.some(x=>x.rva===end)||methods.some(x=>x.rva>rva&&x.rva<end))throw Error('method bounds');
const sec=index.sections.find(s=>m.rva>=s.rva&&m.rva+n<=s.rva+s.rawSize);if(!sec||sec.raw+m.rva-sec.rva!==m.offset)throw Error('PE mapping');
const iced=require(path.join(root,'work/local-tools/iced-x86-1.21.0/package')),d=new iced.Decoder(32,dll.subarray(m.offset,m.offset+n),iced.DecoderOptions.None);d.ip=BigInt(m.va);
const f=new iced.Formatter(iced.FormatterSyntax.Intel),names=new Map(methods.map(x=>[x.va,x.type+'::'+x.signature]));
while(d.canDecode){const i=d.decode();if(i.isInvalid){i.free();break;}const va=Number(i.ip);if(!ranges||ranges.some(([lo,hi])=>va>=lo&&va<hi))console.log(va.toString(16)+' '+f.format(i)+(i.isCallNear?' ; '+(names.get(Number(i.nearBranchTarget))||''):''));i.free();}f.free();d.free();
