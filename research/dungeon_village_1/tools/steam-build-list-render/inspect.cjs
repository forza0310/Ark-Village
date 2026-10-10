const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 固定具名入口顺序解码；段名仅过滤stdout，不从中途地址初始化解码器。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto');
const root=path.resolve(archivePaths.workDir,'../..'),sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=fs.readFileSync(path.join(root,'DungeonVillageEXE/GameAssembly.dll'));
const methodsBytes=fs.readFileSync(path.join(root,'work/persistence-replay-analysis/exe/methods.json'));
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||sha(methodsBytes)!=='399156eff1f2623623d489b3e36ab9a83b9695d311c7c91e21b013dafc665c42')throw Error('fixed source identity');
const methods=JSON.parse(methodsBytes).methods;
const key=process.argv[2];
const helpers=Object.freeze({mapchip:{rva:0x21bca0,offset:0x21aaa0,next:0x21bfb0,bytes:784,type:'data.MapchipData',signature:'public static void DrawMapchip(Graphics g, int dx, int dy, int mapchipId, int inversion) { }'},scroll:{rva:0x2517b0,offset:0x2505b0,next:0x2518e0,bytes:304,type:'main.AppData',signature:'public void DrawVerticalScroll2(Graphics g, int x, int y, int w, int h, int value, int maximum, int largeChange) { }'}});
const w=Object.hasOwn(helpers,key)?helpers[key]:{rva:0x352e20,offset:0x351c20,next:0x383210,bytes:8704,type:'form.SubForm',signature:'private void _draw(Graphics g) { }'};
const m=methods.find(m=>m.rva===w.rva&&m.offset===w.offset&&m.type===w.type&&m.signature===w.signature);
if(!m||m.va!==0x10000000+w.rva||!methods.some(m=>m.rva===w.next)||methods.some(m=>m.rva>w.rva&&m.rva<w.next)||w.rva+w.bytes>w.next||w.bytes>16384||w.offset+w.bytes>dll.length)throw Error('named entry bounds');
const pe=dll.readUInt32LE(0x3c),count=dll.readUInt16LE(pe+6),optional=dll.readUInt16LE(pe+20),table=pe+24+optional;
if(dll.readUInt32LE(pe)!==0x4550||optional<96||dll.readUInt16LE(pe+24)!==0x10b||dll.readUInt32LE(pe+52)!==0x10000000||table+count*40>dll.length)throw Error('PE bounds');
let mapped=false;for(let n=0;n<count;n++){const p=table+n*40,rva=dll.readUInt32LE(p+12),size=dll.readUInt32LE(p+16),off=dll.readUInt32LE(p+20),flags=dll.readUInt32LE(p+36);if(w.rva>=rva&&w.rva+w.bytes<=rva+size&&w.offset===off+w.rva-rva&&off+size<=dll.length&&(flags&0x20000000))mapped=true;}
if(!mapped)throw Error('executable mapping');
const slices=Object.freeze({dispatch:[0x10353198,0x10353269],layout:[0x10353799,0x10353ed0],rows:[0x10353ed0,0x10354620],registration:[0x10354620,0x10354fa0]});
if(process.argv.length!==3||(key!=='manifest'&&!Object.hasOwn(slices,key)&&!Object.hasOwn(helpers,key)))throw Error('fixed display slice only');
console.log(JSON.stringify({method:m.type+'::'+m.signature,...w,va:m.va,sha256:sha(dll.subarray(w.offset,w.offset+w.bytes)),display_slices:slices,policy:'decode from named entry; filter output only; no files written'}));
if(key==='manifest')process.exit(0);
const iced=archivePaths.require(path.join(root,'work/local-tools/iced-x86-1.21.0/package'));
const decoder=new iced.Decoder(32,dll.subarray(w.offset,w.offset+w.bytes),iced.DecoderOptions.None);decoder.ip=BigInt(m.va);
const formatter=new iced.Formatter(iced.FormatterSyntax.Intel),names=new Map();
for(const method of methods){if(!names.has(method.va))names.set(method.va,[]);names.get(method.va).push(method.type+'::'+method.signature);}
while(decoder.canDecode){const i=decoder.decode(),ip=Number(i.ip);if(ip+i.length>m.va+w.bytes){i.free();break;}if(Object.hasOwn(helpers,key)||(ip>=slices[key][0]&&ip<slices[key][1])){const name=i.isCallNear?names.get(Number(i.nearBranchTarget)):null;console.log('0x'+ip.toString(16).toUpperCase(),formatter.format(i),name?name.slice(0,2).join(' | '):'');}i.free();}
formatter.free();decoder.free();
