// Steam窗框／矩形与裁剪的固定19方法；逐入口有界stdout，不执行原游戏、不落盘全文。
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const exe=path.resolve(__dirname,'../persistence-replay-analysis/exe'),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=fs.readFileSync(path.resolve(exe,'../../../DungeonVillageEXE/GameAssembly.dll'));
const meta=fs.readFileSync(path.resolve(exe,'../../../DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat'));
if(hash(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||hash(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('source identity');
const methods=require(path.join(exe,'methods.json')).methods;
const windows=Object.freeze({
  "windowDefault": {
    "rva": 2433104,
    "offset": 2428496,
    "end": 2433152,
    "bytes": 48,
    "type": "main.AppData"
  },
  "window": {
    "rva": 2433152,
    "offset": 2428544,
    "end": 2434112,
    "bytes": 960,
    "type": "main.AppData"
  },
  "boxDefault": {
    "rva": 2404368,
    "offset": 2399760,
    "end": 2404416,
    "bytes": 48,
    "type": "main.AppData"
  },
  "box": {
    "rva": 2404416,
    "offset": 2399808,
    "end": 2405616,
    "bytes": 1200,
    "type": "main.AppData"
  },
  "window2": {
    "rva": 2432272,
    "offset": 2427664,
    "end": 2432624,
    "bytes": 352,
    "type": "main.AppData"
  },
  "window3": {
    "rva": 2432624,
    "offset": 2428016,
    "end": 2433104,
    "bytes": 480,
    "type": "main.AppData"
  },
  "whiteCorner": {
    "rva": 2405616,
    "offset": 2401008,
    "end": 2405664,
    "bytes": 48,
    "type": "main.AppData"
  },
  "drawRect": {
    "rva": 7746960,
    "offset": 7742352,
    "end": 7749968,
    "bytes": 3008,
    "type": "kairo.unity.ui.Graphics"
  },
  "fillRect": {
    "rva": 7776656,
    "offset": 7772048,
    "end": 7778080,
    "bytes": 1424,
    "type": "kairo.unity.ui.Graphics"
  },
  "pushClip": {
    "rva": 7803584,
    "offset": 7798976,
    "end": 7803952,
    "bytes": 368,
    "type": "kairo.unity.ui.Graphics"
  },
  "popClip": {
    "rva": 7800208,
    "offset": 7795600,
    "end": 7800384,
    "bytes": 176,
    "type": "kairo.unity.ui.Graphics"
  },
  "setClip": {
    "rva": 7813872,
    "offset": 7809264,
    "end": 7814288,
    "bytes": 416,
    "type": "kairo.unity.ui.Graphics"
  },
  "clipRect": {
    "rva": 7724912,
    "offset": 7720304,
    "end": 7725520,
    "bytes": 608,
    "type": "kairo.unity.ui.Graphics"
  },
  "guiGroup": {
    "rva": 7820608,
    "offset": 7816000,
    "end": 7821296,
    "bytes": 688,
    "type": "kairo.unity.ui.Graphics"
  },
  "setClipInternal": {
    "rva": 7846976,
    "offset": 7842368,
    "end": 7847264,
    "bytes": 288,
    "type": "kairo.unity.ui.Graphics"
  },
  "updateGLClip": {
    "rva": 7819280,
    "offset": 7814672,
    "end": 7820608,
    "bytes": 1328,
    "type": "kairo.unity.ui.Graphics"
  },
  "transRect": {
    "rva": 7796240,
    "offset": 7791632,
    "end": 7796576,
    "bytes": 336,
    "type": "kairo.unity.ui.Graphics"
  },
  "beginMatrix": {
    "rva": 7722352,
    "offset": 7717744,
    "end": 7722944,
    "bytes": 592,
    "type": "kairo.unity.ui.Graphics"
  },
  "transRectGL": {
    "rva": 7794816,
    "offset": 7790208,
    "end": 7795712,
    "bytes": 896,
    "type": "kairo.unity.ui.Graphics"
  }
});
const request=process.argv[2];if(process.argv.length!==3||(request!=='manifest'&&!Object.hasOwn(windows,request)))throw Error('fixed method name required');
for(const key of request==='manifest'?Object.keys(windows):[request]){
const w=windows[key],m=methods.find(x=>x.rva===w.rva&&x.offset===w.offset&&x.type===(w.type||'form.SubForm'));
if(!m||m.va!==0x10000000+w.rva||!methods.some(x=>x.rva===w.end)||methods.some(x=>x.rva>w.rva&&x.rva<w.end)||w.bytes<=0||w.bytes>17408||(w.bytes-(w.print_after||0))>4112||w.rva+w.bytes>w.end||w.offset+w.bytes>dll.length)throw Error('registered boundary');
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
while(decoder.canDecode){const i=decoder.decode();if(i.isInvalid||Number(i.ip)+i.length>Math.min(m.va+w.bytes,w.code_end||Infinity)){i.free();break;}const refs=i.isCallNear?names.get(Number(i.nearBranchTarget)):undefined;if(Number(i.ip)>=m.va+(w.print_after||0))console.log('0x'+Number(i.ip).toString(16).toUpperCase(),f.format(i),refs?refs.slice(0,2).join(' | '):'');i.free();}
f.free();decoder.free();
}
