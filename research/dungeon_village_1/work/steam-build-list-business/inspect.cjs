// raw21具名入口顺序解码；只显示固定分支，禁止任意地址或落盘指令全文。
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const exe=path.resolve(__dirname,'../persistence-replay-analysis/exe'),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=fs.readFileSync(path.resolve(exe,'../../../DungeonVillageEXE/GameAssembly.dll'));
const meta=fs.readFileSync(path.resolve(exe,'../../../DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat'));
if(hash(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||hash(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('source identity');
const methods=require(path.join(exe,'methods.json')).methods;
const windows=Object.freeze({"init": {"rva": 3220576, "offset": 3215968, "end": 3232672, "bytes": 9899, "type": "form.SubForm", "print_after": 0, "code_end": 271665931, "display_ranges": [[271656032, 271656816], [271665043, 271665931]]}, "update": {"rva": 3283344, "offset": 3278736, "end": 3310768, "bytes": 18761, "type": "form.SubForm", "print_after": 5811, "display_ranges": [[271724611, 271724639], [271731023, 271731072], [271735194, 271737561]]}, "softBuild": {"rva": 3280224, "offset": 3275616, "end": 3280576, "bytes": 352, "type": "form.SubForm"}, "touch": {"rva": 3236016, "offset": 3231408, "end": 3236704, "bytes": 688, "type": "form.SubForm"}, "mode": {"rva": 2228896, "offset": 2224288, "end": 2228976, "bytes": 80, "type": "data.TenantData"}, "ctorType": {"rva": 3336912, "offset": 3332304, "end": 3337248, "bytes": 336, "type": "form.SubForm"}, "pop": {"rva": 8272416, "offset": 8267808, "end": 8272448, "bytes": 32, "type": "kairo.unity.form.FormBase"}, "helpMode": {"rva": 2239376, "offset": 2234768, "end": 2239504, "bytes": 128, "type": "data.TenantData"}});
const request=process.argv[2];if(process.argv.length!==3||(request!=='manifest'&&!Object.hasOwn(windows,request)))throw Error('fixed method name required');
for(const key of request==='manifest'?Object.keys(windows):[request]){
const w=windows[key],m=methods.find(x=>x.rva===w.rva&&x.offset===w.offset&&x.type===(w.type||'form.SubForm'));
if(!m||m.va!==0x10000000+w.rva||!methods.some(x=>x.rva===w.end)||methods.some(x=>x.rva>w.rva&&x.rva<w.end)||w.bytes<=0||w.bytes>20480||(w.bytes-(w.print_after||0))>16384||w.rva+w.bytes>w.end||w.offset+w.bytes>dll.length)throw Error('registered boundary');
const pe=dll.readUInt32LE(0x3c),count=dll.readUInt16LE(pe+6),optional=dll.readUInt16LE(pe+20),table=pe+24+optional;
if(dll.readUInt32LE(pe)!==0x4550||optional<96||table+count*40>dll.length||dll.readUInt16LE(pe+24)!==0x10b||dll.readUInt32LE(pe+52)!==0x10000000)throw Error('PE header');
let mapped=false;for(let n=0;n<count;n++){const p=table+n*40,rva=dll.readUInt32LE(p+12),size=dll.readUInt32LE(p+16),off=dll.readUInt32LE(p+20),flags=dll.readUInt32LE(p+36);if(w.rva>=rva&&w.rva+w.bytes<=rva+size&&w.offset===off+w.rva-rva&&off+size<=dll.length&&(flags&0x20000000)!==0)mapped=true;}
if(!mapped)throw Error('executable PE mapping');
const iced=require(path.resolve(exe,'../../local-tools/iced-x86-1.21.0/package')),names=new Map();
for(const method of methods){if(!names.has(method.va))names.set(method.va,[]);names.get(method.va).push(method.type+'::'+method.signature);}
console.log(JSON.stringify({key,method:m.type+'::'+m.signature,rva:w.rva,offset:w.offset,next_rva:w.end,bytes:w.bytes,sha256:hash(dll.subarray(w.offset,w.offset+w.bytes))}));
if(request==='manifest')continue;
const decoder=new iced.Decoder(32,dll.subarray(w.offset,w.offset+w.bytes),iced.DecoderOptions.None);decoder.ip=BigInt(m.va);
const f=new iced.Formatter(iced.FormatterSyntax.Intel);
while(decoder.canDecode){const i=decoder.decode();if(i.isInvalid||Number(i.ip)+i.length>Math.min(m.va+w.bytes,w.code_end||Infinity)){i.free();break;}const refs=i.isCallNear?names.get(Number(i.nearBranchTarget)):undefined;if(Number(i.ip)>=m.va+(w.print_after||0)&&(!w.display_ranges||w.display_ranges.some(([a,b])=>Number(i.ip)>=a&&Number(i.ip)<b)))console.log('0x'+Number(i.ip).toString(16).toUpperCase(),f.format(i),refs?refs.slice(0,2).join(' | '):'');i.free();}
f.free();decoder.free();
}
