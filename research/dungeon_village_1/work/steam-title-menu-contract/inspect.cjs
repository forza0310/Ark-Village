// Steam标题菜单固定方法入口：只读有界stdout，不执行原游戏、不落盘指令全文。
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const exe=path.resolve(__dirname,'../persistence-replay-analysis/exe'),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=fs.readFileSync(path.resolve(exe,'../../../DungeonVillageEXE/GameAssembly.dll'));
const meta=fs.readFileSync(path.resolve(exe,'../../../DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat'));
if(hash(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||hash(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('source identity');
const methods=require(path.join(exe,'methods.json')).methods;
const windows=Object.freeze({
 drawMiddle:{rva:0x20df30,offset:0x20cd30,end:0x210a30,bytes:8192,print_after:4080},
 menu:{rva:0x20df30,offset:0x20cd30,end:0x210a30,bytes:6400,print_after:5000},
 drawTail:{rva:0x20df30,offset:0x20cd30,end:0x210a30,bytes:11008,print_after:8170},
 drawEnd:{rva:0x20df30,offset:0x20cd30,end:0x210a30,bytes:11008,print_after:10000},
 updateMenu:{rva:0x20c230,offset:0x20b030,end:0x20d5d0,bytes:2800,print_after:1680},
 updateTail:{rva:0x20c230,offset:0x20b030,end:0x20d5d0,bytes:5024,print_after:2000},
 updateNav:{rva:0x20c230,offset:0x20b030,end:0x20d5d0,bytes:1800,print_after:600},
 change:{rva:0x20a2c0,offset:0x2090c0,end:0x20a3a0,bytes:224},
 continueGame:{rva:0x20a3a0,offset:0x2091a0,end:0x20a640,bytes:672},
 newGame:{rva:0x20b8e0,offset:0x20a6e0,end:0x20b950,bytes:112},
 touchValue:{rva:0x20bdb0,offset:0x20abb0,end:0x20be00,bytes:80},
 touchEvent:{rva:0x20b950,offset:0x20a750,end:0x20bcb0,bytes:864},
 back:{rva:0x20bcb0,offset:0x20aab0,end:0x20bd10,bytes:96},
 menuTouch:{rva:2348496,offset:2343888,end:2348656,bytes:160,type:'surface.GameView'},
 addRect:{rva:2334240,offset:2329632,end:2334352,bytes:112,type:'surface.GameView'},
 viewEvent:{rva:2344992,offset:2340384,end:2346768,bytes:1776,code_end:0x1023cea0,type:'surface.GameView'},
 touchSelect:{rva:2344752,offset:2340144,end:2344992,bytes:240,type:'surface.GameView'},
 surfaceAdd:{rva:0x88b270,offset:0x88a070,end:0x88b790,bytes:1312,type:'kairo.unity.surface.SurfaceBase'},
 checkFlag:{rva:0x895cf0,offset:0x894af0,end:0x895d10,bytes:32,type:'kairo.unity.surface.TouchComponent'},
 setValue:{rva:0x23dd40,offset:0x23cb40,end:0x23dde0,bytes:160,type:'surface.GameView'},
 setAccessor:{rva:0x23de60,offset:0x23cc60,end:0x23dfd0,bytes:368,type:'surface.GameView'},
 drawGate:{rva:0x20df30,offset:0x20cd30,end:0x210a30,bytes:4100,print_after:3400},
 init:{rva:0x20adf0,offset:0x209bf0,end:0x20b580,bytes:1936},
 staticInit:{rva:0x20d9f0,offset:0x20c7f0,end:0x20ddd0,bytes:992}
});
const request=process.argv[2];if(process.argv.length!==3||(request!=='manifest'&&!Object.hasOwn(windows,request)))throw Error('fixed method name required');
for(const key of request==='manifest'?Object.keys(windows):[request]){
const w=windows[key],m=methods.find(x=>x.rva===w.rva&&x.offset===w.offset&&x.type===(w.type||'form.TitleForm'));
if(!m||m.va!==0x10000000+w.rva||!methods.some(x=>x.rva===w.end)||methods.some(x=>x.rva>w.rva&&x.rva<w.end)||w.bytes<=0||w.bytes>11008||(w.bytes-(w.print_after||0))>4112||w.rva+w.bytes>w.end||w.offset+w.bytes>dll.length)throw Error('registered boundary');
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
while(decoder.canDecode){const i=decoder.decode();if(Number(i.ip)+i.length>Math.min(m.va+w.bytes,w.code_end||Infinity)){i.free();break;}const refs=i.isCallNear?names.get(Number(i.nearBranchTarget)):undefined;if(Number(i.ip)>=m.va+(w.print_after||0))console.log('0x'+Number(i.ip).toString(16).toUpperCase(),f.format(i),refs?refs.slice(0,2).join(' | '):'');i.free();}
f.free();decoder.free();
}
