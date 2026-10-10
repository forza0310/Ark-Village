const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// Steam标题菜单固定方法入口：只读有界stdout，不执行原游戏、不落盘指令全文。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto');
const exe=path.resolve(archivePaths.workDir,'../persistence-replay-analysis/exe'),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=fs.readFileSync(path.resolve(exe,'../../../DungeonVillageEXE/GameAssembly.dll'));
const meta=fs.readFileSync(path.resolve(exe,'../../../DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat'));
if(hash(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||hash(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('source identity');
const methods=archivePaths.require(path.join(exe,'methods.json')).methods;
const windows=Object.freeze({
 drawMenu:{rva:0x308540,offset:0x307340,end:0x30a110,bytes:4096},
 drawMenuTail:{rva:0x308540,offset:0x307340,end:0x30a110,bytes:7120,print_after:4090},
 drawEntry:{rva:0x352e20,offset:0x351c20,end:0x383210,bytes:4096},
 drawWrapper:{rva:0x30ce30,offset:0x30bc30,end:0x30d160,bytes:816},
 drawDispatch:{rva:0x352e20,offset:0x351c20,end:0x383210,bytes:16384,print_after:15872},
 drawDialog:{rva:0x352e20,offset:0x351c20,end:0x383210,bytes:17312,print_after:16320,code_end:0x103571c0},
 ctor:{rva:0x32e970,offset:0x32d770,end:0x32ead0,bytes:352},
 defaultCtor:{rva:0x32e830,offset:0x32d630,end:0x32e970,bytes:320},
 titleCtor:{rva:0x20ddd0,offset:0x20cbd0,end:0x20df30,bytes:352,type:'form.TitleForm'},
 addMenu:{rva:0x305d20,offset:0x304b20,end:0x305e40,bytes:288},
 frameDefault:{rva:0x30e250,offset:0x30d050,end:0x30e270,bytes:32},
 isMenu:{rva:0x3153e0,offset:0x3141e0,end:0x315420,bytes:64},
 makeDialog:{rva:0x315490,offset:0x314290,end:0x3154f0,bytes:96},
 makeDialogBase:{rva:0x315420,offset:0x314220,end:0x315490,bytes:112},
 pop:{rva:0x7e3a20,offset:0x7e2820,end:0x7e3a40,bytes:32,type:'kairo.unity.form.FormBase'},
 managerPop:{rva:0x7e7d60,offset:0x7e6b60,end:0x7e7eb0,bytes:336,type:'kairo.unity.form.FormManagerBase'},
 popBody:{rva:0x7f29e0,offset:0x7f17e0,end:0x7f2e60,bytes:1152,type:'.FormManagerBase.<>c__DisplayClass75_0'},
 drawString:{rva:0x766290,offset:0x765090,end:0x7662d0,bytes:64,type:'kairo.unity.ui.Graphics'},
 drawStringCore:{rva:0x7663d0,offset:0x7651d0,end:0x7678c0,bytes:1024,type:'kairo.unity.ui.Graphics'},
 languageLT:{rva:0x812900,offset:0x811700,end:0x812990,bytes:144,type:'kairo.unity.util.Language'},
 translate:{rva:0x8211a0,offset:0x81ffa0,end:0x821340,bytes:416,type:'kairo.unity.util.Language'},
 translateCall:{rva:0x80bf70,offset:0x80ad70,end:0x80c570,bytes:1536,type:'kairo.unity.util.Language'},
 loadTranslations:{rva:0x816d70,offset:0x815b70,end:0x8172e0,bytes:1392,type:'kairo.unity.util.Language'},
 translateLeaf:{rva:0x836930,offset:0x835730,end:0x836aa0,bytes:368,type:'kairo.unity.util.Language'},
 translateLookup:{rva:0x835ed0,offset:0x834cd0,end:0x836930,bytes:2656,type:'kairo.unity.util.Language'},
 kairoLT:{rva:0x1dc330,offset:0x1db130,end:0x1dc6b0,bytes:896,type:'system.KairoText'},
 initDialog:{rva:0x3123c0,offset:0x3111c0,end:0x312460,bytes:160}
});
const request=process.argv[2];if(process.argv.length!==3||(request!=='manifest'&&!Object.hasOwn(windows,request)))throw Error('fixed method name required');
for(const key of request==='manifest'?Object.keys(windows):[request]){
const w=windows[key],m=methods.find(x=>x.rva===w.rva&&x.offset===w.offset&&x.type===(w.type||'form.SubForm'));
if(!m||m.va!==0x10000000+w.rva||!methods.some(x=>x.rva===w.end)||methods.some(x=>x.rva>w.rva&&x.rva<w.end)||w.bytes<=0||w.bytes>17408||(w.bytes-(w.print_after||0))>4112||w.rva+w.bytes>w.end||w.offset+w.bytes>dll.length)throw Error('registered boundary');
const pe=dll.readUInt32LE(0x3c),count=dll.readUInt16LE(pe+6),optional=dll.readUInt16LE(pe+20),table=pe+24+optional;
if(dll.readUInt32LE(pe)!==0x4550||optional<96||table+count*40>dll.length||dll.readUInt16LE(pe+24)!==0x10b||dll.readUInt32LE(pe+52)!==0x10000000)throw Error('PE header');
let mapped=false;for(let n=0;n<count;n++){const p=table+n*40,rva=dll.readUInt32LE(p+12),size=dll.readUInt32LE(p+16),off=dll.readUInt32LE(p+20),flags=dll.readUInt32LE(p+36);if(w.rva>=rva&&w.rva+w.bytes<=rva+size&&w.offset===off+w.rva-rva&&off+size<=dll.length&&(flags&0x20000000)!==0)mapped=true;}
if(!mapped)throw Error('executable PE mapping');
const iced=archivePaths.require(path.resolve(exe,'../../local-tools/iced-x86-1.21.0/package')),names=new Map();
for(const method of methods){if(!names.has(method.va))names.set(method.va,[]);names.get(method.va).push(method.type+'::'+method.signature);}
console.log(JSON.stringify({key,method:m.type+'::'+m.signature,rva:w.rva,offset:w.offset,next_rva:w.end,bytes:w.bytes,sha256:hash(dll.subarray(w.offset,w.offset+w.bytes))}));
if(request==='manifest')continue;
const decoder=new iced.Decoder(32,dll.subarray(w.offset,w.offset+w.bytes),iced.DecoderOptions.None);decoder.ip=BigInt(m.va);
const f=new iced.Formatter(iced.FormatterSyntax.Intel);
while(decoder.canDecode){const i=decoder.decode();if(Number(i.ip)+i.length>Math.min(m.va+w.bytes,w.code_end||Infinity)){i.free();break;}const refs=i.isCallNear?names.get(Number(i.nearBranchTarget)):undefined;if(Number(i.ip)>=m.va+(w.print_after||0))console.log('0x'+Number(i.ip).toString(16).toUpperCase(),f.format(i),refs?refs.slice(0,2).join(' | '):'');i.free();}
f.free();decoder.free();
}
