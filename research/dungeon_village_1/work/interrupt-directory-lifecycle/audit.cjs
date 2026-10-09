// 仅静态读取固定 APK 反编译文件与 Steam 具名方法；不访问玩家档、不执行游戏。
// --write 首次生成专题证据；默认重新计算并与已有 EVIDENCE.json 严格比较。
const fs = require('fs'), path = require('path'), crypto = require('crypto');
const root = path.resolve(__dirname, '../..');
const hash = b => crypto.createHash('sha256').update(b).digest('hex');
const read = p => fs.readFileSync(path.join(root, p));
const dllPath = 'DungeonVillageEXE/GameAssembly.dll';
const metaPath = 'DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat';
const dll = read(dllPath), metadata = read(metaPath);
if (hash(dll) !== '9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a' ||
    hash(metadata) !== '80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369') throw Error('固定输入身份不匹配');
const methodsPath = 'work/persistence-replay-analysis/exe/methods.json';
const methods = JSON.parse(read(methodsPath)).methods;
const iced = require(path.join(root, 'work/local-tools/iced-x86-1.21.0/package'));
const specs = [
  { rva:0x2602d0, size:15728, type:'main.AppData', name:'SaveGame', windows:[
    [0x1026368d,0x102636e1], [0x10263755,0x10263787], [0x1026384a,0x10263863], [0x102638ef,0x102638fc]] },
  { rva:0x259f70, size:20544, type:'main.AppData', name:'LoadGame', windows:[
    [0x1025a17e,0x1025a1e6], [0x1025a218,0x1025a223]] },
  { rva:0x25f160, size:1008, type:'main.AppData', name:'NewGame', windows:[[0x1025f1d8,0x1025f20b]] },
  { rva:0x2fa820, size:1200, type:'form.GameForm', name:'Init', windows:[
    [0x102faa25,0x102faa2a], [0x102fabf1,0x102fabf6], [0x102fac4c,0x102fac94]] },
  { rva:0x20a3a0, size:672, type:'form.TitleForm', name:'ContinueGame', windows:[
    [0x1020a40e,0x1020a43f], [0x1020a4ee,0x1020a519]] },
  { rva:0x20c230, size:5024, type:'form.TitleForm', name:'Update', windows:[
    [0x1020cb58,0x1020cbde], [0x1020cc52,0x1020cc57],
    [0x1020cc9d,0x1020cce7], [0x1020cdb8,0x1020cdbd], [0x1020d470,0x1020d4cd]] },
];
// 从 PE 节表核 RVA→文件偏移；没有跨方法或扫描整个映像。
const pe = dll.readUInt32LE(0x3c), opt = dll.readUInt16LE(pe+20);
if (dll.readUInt32LE(pe)!==0x4550 || dll.readUInt16LE(pe+24)!==0x10b || dll.readUInt32LE(pe+52)!==0x10000000) throw Error('PE身份');
const sections = Array.from({length:dll.readUInt16LE(pe+6)}, (_,n)=>{
  const p=pe+24+opt+n*40;
  return {rva:dll.readUInt32LE(p+12), size:dll.readUInt32LE(p+16), offset:dll.readUInt32LE(p+20), flags:dll.readUInt32LE(p+36)};
});
const steam = specs.map(s=>{
  const m = methods.find(m=>m.rva===s.rva && m.type===s.type && m.signature.includes(' '+s.name+'('));
  if (!m || m.va!==0x10000000+s.rva || !methods.some(m=>m.rva===s.rva+s.size) ||
      methods.some(m=>m.rva>s.rva && m.rva<s.rva+s.size) ||
      !sections.some(x=>s.rva>=x.rva && s.rva+s.size<=x.rva+x.size && m.offset===x.offset+s.rva-x.rva && (x.flags&0x20000000)!==0)) throw Error('具名方法边界');
  const bytes=dll.subarray(m.offset,m.offset+s.size), decoder=new iced.Decoder(32,bytes,iced.DecoderOptions.None);
  decoder.ip=BigInt(m.va);
  const formatter=new iced.Formatter(iced.FormatterSyntax.Intel), windows=s.windows.map(([begin,end])=>({begin:'0x'+begin.toString(16),end_exclusive:'0x'+end.toString(16),instructions:[]}));
  while(decoder.canDecode){
    const i=decoder.decode(), va=Number(i.ip);
    s.windows.forEach(([begin,end],n)=>{if(va>=begin&&va<end) windows[n].instructions.push({va:'0x'+va.toString(16),bytes:bytes.subarray(va-m.va,va-m.va+i.length).toString('hex'),instruction:formatter.format(i)});});
    i.free();
  }
  decoder.free(); formatter.free();
  if(windows.some(w=>!w.instructions.length))throw Error('空证据窗口');
  return {method:m.type+'::'+m.signature,rva:'0x'+s.rva.toString(16),offset:m.offset,bytes:s.size,sha256:hash(bytes),windows};
});
const apkSpecs=[
  ['work/decompiled/sources/d/a.java',1016,1041,'新局选择8'],
  ['work/decompiled/sources/d/a.java',1279,1289,'载入入口选择8；普通JADX有大方法警告'],
  ['work/decompiled/sources/d/a.java',3300,3310,'保存当前目录'],
  ['work/decompiled/sources/b/h.java',64,79,'继续只查目录再调用加载'],
  ['work/decompiled/sources/b/h.java',510,535,'raw20删除当前目录'],
  ['work/decompiled/sources/b/c.java',1160,1186,'主场景初始化'],
  ['work/persistence-replay-analysis/exe/dumper/dump.cs',219661,219680,'Steam系统索引常量；生成metadata派生文件'],
];
const apk=apkSpecs.map(([file,first,last,purpose])=>{
  const bytes=read(file),lines=bytes.toString('utf8').split(/\r?\n/);
  if(lines.length<last)throw Error('来源范围越界');
  const excerpt=lines.slice(first-1,last).join('\n');
  return {file,source_bytes:bytes.length,source_sha256:hash(bytes),first_line:first,last_line:last,excerpt_utf8_lf_sha256:hash(excerpt),purpose};
});
const result={scope:'具名入口直接写者；不构成全程序别名与间接调用的否定证明',
  sources:[dllPath,metaPath,methodsPath].map(file=>({file,bytes:read(file).length,sha256:hash(read(file))})),steam,source_windows:apk};
const text=JSON.stringify(result,null,2)+'\n',out=path.join(__dirname,'EVIDENCE.json');
if(process.argv.length===3&&process.argv[2]==='--write')fs.writeFileSync(out,text);
else if(process.argv.length!==2)throw Error('只允许无参数核验或 --write');
else if(fs.readFileSync(out,'utf8')!==text)throw Error('证据发生变化');
console.log(JSON.stringify({verified_methods:steam.length,source_windows:apk.length,evidence_bytes:Buffer.byteLength(text),mode:process.argv[2]||'verify'}));
