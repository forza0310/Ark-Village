// 固定Character2静态初始化与raw60危险图标消费者；不输出整个静态构造器。
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'../..'),sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=fs.readFileSync(path.join(root,'DungeonVillageEXE/GameAssembly.dll'));
const ix=require('../persistence-replay-analysis/exe/methods.json');
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a')throw Error('source');
const methods={cctor:[0x293560,0x29d2e0],portrait:[0x2cff00,0x2d0420]};
const slices={consumer:['portrait',0x102d02d0,0x102d0401],offset:['cctor',0x102968da,0x102969f0]};
const key=process.argv[2];
if(process.argv.length!==3||(!Object.hasOwn(slices,key)&&!['manifest','locate'].includes(key)))throw Error('fixed key');
function method(key){const [rva,end]=methods[key],m=ix.methods.find(x=>x.rva===rva);if(!m||end-rva>65536||!ix.methods.some(x=>x.rva===end)||ix.methods.some(x=>x.rva>rva&&x.rva<end))throw Error('named method boundary');const sec=ix.sections.find(s=>rva>=s.rva&&end<=s.rva+s.rawSize);if(!sec||sec.raw+rva-sec.rva!==m.offset)throw Error('PE');return {m,n:end-rva};}
if(key==='manifest'){for(const k of Object.keys(methods)){const {m,n}=method(k);console.log(JSON.stringify({key:k,type:m.type,signature:m.signature,rva:m.rva,bytes:n,sha256:sha(dll.subarray(m.offset,m.offset+n))}));}process.exit(0);}
const [k,lo,hi]=key==='locate'?['cctor',0,0]:slices[key],{m,n}=method(k),iced=require('../local-tools/iced-x86-1.21.0/package');
const d=new iced.Decoder(32,dll.subarray(m.offset,m.offset+n),iced.DecoderOptions.None);d.ip=BigInt(m.va);
const f=new iced.Formatter(iced.FormatterSyntax.Intel),names=new Map(ix.methods.map(x=>[x.va,x.type+'::'+x.signature]));
while(d.canDecode){const i=d.decode();if(i.isInvalid){i.free();break;}const va=Number(i.ip),s=f.format(i);if(key==='locate'?/\+0A0h\]/.test(s):va>=lo&&va<hi)console.log(va.toString(16)+' '+s+(i.isCallNear?' ; '+(names.get(Number(i.nearBranchTarget))||''):''));i.free();}f.free();d.free();
