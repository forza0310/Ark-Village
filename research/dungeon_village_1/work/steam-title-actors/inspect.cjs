// Steam标题人物固定方法入口：只读有界stdout，不执行原游戏、不落盘指令全文。
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const exe=path.resolve(__dirname,'../persistence-replay-analysis/exe'),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=fs.readFileSync(path.resolve(exe,'../../../DungeonVillageEXE/GameAssembly.dll'));
const meta=fs.readFileSync(path.resolve(exe,'../../../DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat'));
if(hash(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||hash(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('source identity');
const methods=require(path.join(exe,'methods.json')).methods;
const windows=Object.freeze({
 draw:{rva:0x20a710,offset:0x209510,end:0x20ad50,bytes:1600},
 update:{rva:0x20d5d0,offset:0x20c3d0,end:0x20d9f0,bytes:1056},
 interval:{rva:0x20bd50,offset:0x20ab50,end:0x20bdb0,bytes:96},
 reset:{rva:0x20bd10,offset:0x20ab10,end:0x20bd50,bytes:64},
 admission:{rva:0x20c230,offset:0x20b030,end:0x20d5d0,bytes:2048},
 actor:{rva:0x2ccea0,offset:0x2cbca0,end:0x2cd000,bytes:352,type:'game.UserData'},
 actorState:{rva:0x2e6b90,offset:0x2e5990,end:0x2e6c40,bytes:176,type:'game.UserData'},
 job:{rva:0x2134f0,offset:0x2122f0,end:0x213560,bytes:112,type:'data.CharacterData'},
 image:{rva:0x21ac90,offset:0x219a90,end:0x21acc0,bytes:48,type:'data.JobData'},
 random:{rva:0x2a44e0,offset:0x2a32e0,end:0x2a4550,bytes:112,type:'game.GameUtil'},
 alpha:{rva:0x774a80,offset:0x773880,end:0x774ab0,bytes:48,type:'kairo.unity.ui.Graphics'},
 alphaReset:{rva:0x774980,offset:0x773780,end:0x7749d0,bytes:80,type:'kairo.unity.ui.Graphics'},
 alphaFull:{rva:0x7749d0,offset:0x7737d0,end:0x774a80,bytes:176,type:'kairo.unity.ui.Graphics'},
 rate:{rva:0x2a4780,offset:0x2a3580,end:0x2a47e0,bytes:96,type:'game.GameUtil'},
 pulse:{rva:0x747e60,offset:0x746c60,end:0x747f90,bytes:304,type:'kairo.unity.ui.Canvas'},
 weaponStep:{rva:0x283c80,offset:0x282a80,end:0x283cd0,bytes:80,type:'game.Character2'},
 body:{rva:0x27bcf0,offset:0x27aaf0,end:0x27c560,bytes:2160,type:'game.Character2'},
 weapon:{rva:0x27df30,offset:0x27cd30,end:0x27e520,bytes:1520,type:'game.Character2'},
 drawParam:{rva:0x28ae00,offset:0x289c00,end:0x28bbb0,bytes:3504,type:'game.Character2'},
 keypadPulse:{rva:0x7ab780,offset:0x7aa580,end:0x7ab7c0,bytes:64,type:'kairo.unity.ui.Keypad'},
 joystickIndex:{rva:0x74bd00,offset:0x74ab00,end:0x74bda0,bytes:160,type:'kairo.unity.ui.Canvas'}
});
const request=process.argv[2];if(process.argv.length!==3||(request!=='manifest'&&!Object.hasOwn(windows,request)))throw Error('fixed method name required');
for(const key of request==='manifest'?Object.keys(windows):[request]){
const w=windows[key],m=methods.find(x=>x.rva===w.rva&&x.offset===w.offset&&x.type===(w.type||'form.TitleForm'));
if(!m||m.va!==0x10000000+w.rva||!methods.some(x=>x.rva===w.end)||methods.some(x=>x.rva>w.rva&&x.rva<w.end)||w.bytes<=0||w.bytes>4096||w.rva+w.bytes>w.end||w.offset+w.bytes>dll.length)throw Error('registered boundary');
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
while(decoder.canDecode){const i=decoder.decode();if(Number(i.ip)+i.length>m.va+w.bytes){i.free();break;}const refs=i.isCallNear?names.get(Number(i.nearBranchTarget)):undefined;if(Number(i.ip)>=m.va+(w.print_after||0))console.log('0x'+Number(i.ip).toString(16).toUpperCase(),f.format(i),refs?refs.slice(0,2).join(' | '):'');i.free();}
f.free();decoder.free();
}
