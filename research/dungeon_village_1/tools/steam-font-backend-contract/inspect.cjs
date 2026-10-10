const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// Steam标题菜单固定方法入口：只读有界stdout，不执行原游戏、不落盘指令全文。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto');
const exe=path.resolve(archivePaths.workDir,'../persistence-replay-analysis/exe'),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=fs.readFileSync(path.resolve(exe,'../../../DungeonVillageEXE/GameAssembly.dll'));
const meta=fs.readFileSync(path.resolve(exe,'../../../DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat'));
if(hash(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||hash(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('source identity');
const methods=archivePaths.require(path.join(exe,'methods.json')).methods;
const windows=Object.freeze({
  "setSize": {
    "rva": 7863632,
    "offset": 7859024,
    "end": 7863856,
    "bytes": 224,
    "type": "kairo.unity.ui.Font"
  },
  "adjust": {
    "rva": 7859456,
    "offset": 7854848,
    "end": 7859664,
    "bytes": 208,
    "type": "kairo.unity.ui.Font"
  },
  "copy": {
    "rva": 7860240,
    "offset": 7855632,
    "end": 7860528,
    "bytes": 288,
    "type": "kairo.unity.ui.Font"
  },
  "set": {
    "rva": 7863984,
    "offset": 7859376,
    "end": 7864080,
    "bytes": 96,
    "type": "kairo.unity.ui.Font"
  },
  "defaultGet": {
    "rva": 7847264,
    "offset": 7842656,
    "end": 7847328,
    "bytes": 64,
    "type": "kairo.unity.ui.Graphics"
  },
  "defaultSet": {
    "rva": 7847328,
    "offset": 7842720,
    "end": 7847824,
    "bytes": 80,
    "type": "kairo.unity.ui.Graphics"
  },
  "push": {
    "rva": 7805168,
    "offset": 7800560,
    "end": 7805344,
    "bytes": 176,
    "type": "kairo.unity.ui.Graphics"
  },
  "pop": {
    "rva": 7800672,
    "offset": 7796064,
    "end": 7800784,
    "bytes": 112,
    "type": "kairo.unity.ui.Graphics"
  },
  "graphicsCctor": {
    "rva": 7821712,
    "offset": 7817104,
    "end": 7825216,
    "bytes": 3504,
    "type": "kairo.unity.ui.Graphics"
  },
  "drawFirst": {
    "rva": 7758800,
    "offset": 7754192,
    "end": 7764160,
    "bytes": 4096,
    "type": "kairo.unity.ui.Graphics"
  },
  "drawTail": {
    "rva": 7758800,
    "offset": 7754192,
    "end": 7764160,
    "bytes": 5360,
    "type": "kairo.unity.ui.Graphics",
    "print_after": 4090
  },
  "setFont": {
    "rva": 7816544,
    "offset": 7811936,
    "end": 7816656,
    "bytes": 112,
    "type": "kairo.unity.ui.Graphics"
  },
  "pluginWidth": {
    "rva": 8866208,
    "offset": 8861600,
    "end": 8866496,
    "bytes": 288,
    "type": "kairo.unity.native.KairoPlugin"
  },
  "pluginWidthTask": {
    "rva": 8941728,
    "offset": 8937120,
    "end": 8942432,
    "bytes": 704,
    "type": ".KairoPlugin.<>c__DisplayClass156_0"
  },
  "initGUI": {
    "rva": 7891392,
    "offset": 7886784,
    "end": 7892736,
    "bytes": 1344,
    "print_after": 0,
    "type": "kairo.unity.ui.IApplication"
  },
  "onGUIFirst": {
    "rva": 7899600,
    "offset": 7894992,
    "end": 7906912,
    "bytes": 4096,
    "print_after": 0,
    "type": "kairo.unity.ui.IApplication"
  },
  "onGUITail": {
    "rva": 7899600,
    "offset": 7894992,
    "end": 7906912,
    "bytes": 7312,
    "print_after": 4090,
    "type": "kairo.unity.ui.IApplication"
  },
  "graphicsInit": {
    "rva": 7796960,
    "offset": 7792352,
    "end": 7798480,
    "bytes": 1520,
    "type": "kairo.unity.ui.Graphics"
  },
  "onGUICleanup": {
    "rva": 7847408,
    "offset": 7842800,
    "end": 7847824,
    "bytes": 116,
    "type": "IApplication.OnGUI generated cleanup",
    "caller_rva": 7899600,
    "call_va": 276336710
  }
});
const request=process.argv[2];if(process.argv.length!==3||(request!=='manifest'&&!Object.hasOwn(windows,request)))throw Error('fixed method name required');
for(const key of request==='manifest'?Object.keys(windows):[request]){
const w=windows[key];let m=methods.find(x=>x.rva===w.rva&&x.offset===w.offset&&x.type===w.type);
if(w.caller_rva){const caller=methods.find(x=>x.rva===w.caller_rva&&x.type==='kairo.unity.ui.IApplication'),p=caller.offset+w.call_va-caller.va;if(!caller||dll[p]!==0xe8||w.call_va+5+dll.readInt32LE(p+1)!==0x10000000+w.rva)throw Error('cleanup direct target');m={rva:w.rva,offset:w.offset,va:0x10000000+w.rva,type:w.type,signature:'direct call target; bounded cleanup only'};}
if(!m||m.va!==0x10000000+w.rva||(!w.caller_rva&&!methods.some(x=>x.rva===w.end))||methods.some(x=>x.rva>w.rva&&x.rva<w.end)||w.bytes<=0||w.bytes>17408||(w.bytes-(w.print_after||0))>4112||w.rva+w.bytes>w.end||w.offset+w.bytes>dll.length)throw Error('registered boundary');
const pe=dll.readUInt32LE(0x3c),count=dll.readUInt16LE(pe+6),optional=dll.readUInt16LE(pe+20),table=pe+24+optional;
if(dll.readUInt32LE(pe)!==0x4550||optional<96||table+count*40>dll.length||dll.readUInt16LE(pe+24)!==0x10b||dll.readUInt32LE(pe+52)!==0x10000000)throw Error('PE header');
let mapped=false;for(let n=0;n<count;n++){const p=table+n*40,rva=dll.readUInt32LE(p+12),size=dll.readUInt32LE(p+16),off=dll.readUInt32LE(p+20),flags=dll.readUInt32LE(p+36);if(w.rva>=rva&&w.rva+w.bytes<=rva+size&&w.offset===off+w.rva-rva&&off+size<=dll.length&&(flags&0x20000000)!==0)mapped=true;}
if(!mapped)throw Error('executable PE mapping');
const iced=archivePaths.require(path.resolve(exe,'../../local-tools/iced-x86-1.21.0/package')),names=new Map();
for(const method of methods){if(!names.has(method.va))names.set(method.va,[]);names.get(method.va).push(method.type+'::'+method.signature);}
if(w.caller_rva){const c=methods.find(x=>x.rva===w.caller_rva&&x.type==='kairo.unity.ui.IApplication');const d=new iced.Decoder(32,dll.subarray(c.offset,c.offset+w.call_va-c.va+5),iced.DecoderOptions.None);d.ip=BigInt(c.va);let valid=false;while(d.canDecode){const i=d.decode();if(i.isInvalid){i.free();break;}if(Number(i.ip)===w.call_va&&i.isCallNear&&Number(i.nearBranchTarget)===m.va)valid=true;i.free();}d.free();if(!valid)throw Error('cleanup caller instruction boundary');}
console.log(JSON.stringify({key,method:m.type+'::'+m.signature,rva:w.rva,offset:w.offset,next_rva:w.end,bytes:w.bytes,sha256:hash(dll.subarray(w.offset,w.offset+w.bytes))}));
if(request==='manifest')continue;
const decoder=new iced.Decoder(32,dll.subarray(w.offset,w.offset+w.bytes),iced.DecoderOptions.None);decoder.ip=BigInt(m.va);
const f=new iced.Formatter(iced.FormatterSyntax.Intel);
while(decoder.canDecode){const i=decoder.decode();if(i.isInvalid||Number(i.ip)+i.length>Math.min(m.va+w.bytes,w.code_end||Infinity)){i.free();break;}const refs=i.isCallNear?names.get(Number(i.nearBranchTarget)):undefined;if(Number(i.ip)>=m.va+(w.print_after||0))console.log('0x'+Number(i.ip).toString(16).toUpperCase(),f.format(i),refs?refs.slice(0,2).join(' | '):'');i.free();}
f.free();decoder.free();
}
