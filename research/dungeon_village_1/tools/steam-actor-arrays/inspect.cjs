const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// Steam标题人物固定方法入口：只读有界stdout，不执行原游戏、不落盘指令全文。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto');
const exe=path.resolve(archivePaths.workDir,'../persistence-replay-analysis/exe'),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=fs.readFileSync(path.resolve(exe,'../../../DungeonVillageEXE/GameAssembly.dll'));
const meta=fs.readFileSync(path.resolve(exe,'../../../DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat'));
if(hash(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||hash(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('source identity');
const methods=archivePaths.require(path.join(exe,'methods.json')).methods;
const windows=Object.freeze({
 characterInit:{rva:0x293560,offset:0x292360,end:0x29d2e0,bytes:4096,type:'game.Character2'},
 weaponInit:{rva:0x229e40,offset:0x228c40,end:0x2317e0,bytes:4096,type:'data.WeaponData'},
 offset:{rva:0x283d40,offset:0x282b40,end:0x283ee0,bytes:416,type:'game.Character2'}
});
const request=process.argv[2];if(process.argv.length!==3||(request!=='manifest'&&!Object.hasOwn(windows,request)))throw Error('fixed method name required');
for(const key of request==='manifest'?Object.keys(windows):[request]){
const w=windows[key],m=methods.find(x=>x.rva===w.rva&&x.offset===w.offset&&x.type===(w.type||'form.TitleForm'));
if(!m||m.va!==0x10000000+w.rva||!methods.some(x=>x.rva===w.end)||methods.some(x=>x.rva>w.rva&&x.rva<w.end)||w.bytes<=0||w.bytes>4096||w.rva+w.bytes>w.end||w.offset+w.bytes>dll.length)throw Error('registered boundary');
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
while(decoder.canDecode){const i=decoder.decode();if(Number(i.ip)+i.length>m.va+w.bytes){i.free();break;}const refs=i.isCallNear?names.get(Number(i.nearBranchTarget)):undefined;if(Number(i.ip)>=m.va+(w.print_after||0))console.log('0x'+Number(i.ip).toString(16).toUpperCase(),f.format(i),refs?refs.slice(0,2).join(' | '):'');i.free();}
f.free();decoder.free();
}
