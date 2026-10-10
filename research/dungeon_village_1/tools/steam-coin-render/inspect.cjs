const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// Steam X4具名入口有限顺序解码；不全DLL扫描、不保存方法实现文本。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto');
const root=path.resolve(archivePaths.workDir,'../..'),hash=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=fs.readFileSync(path.join(root,'DungeonVillageEXE/GameAssembly.dll'));
const source=fs.readFileSync(path.join(root,'work/persistence-replay-analysis/exe/methods.json'));
if(hash(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||
   hash(source)!=='399156eff1f2623623d489b3e36ab9a83b9695d311c7c91e21b013dafc665c42')throw Error('source identity');
const methods=JSON.parse(source).methods;
// kind4入口由原分派cmp4/je确认；DrawEffect共13,488字节，顺序解码至尾部以核正序循环。
const ranges={draw:{rva:0x24bb30,bytes:13488,type:'main.AppData',signature:'public void DrawEffect(Graphics g) { }'},
    coin:{rva:0x246040,bytes:464,type:'main.AppData',signature:'public void AddEffectCoin(int sx, int sy, int wait) { }'},
    update:{rva:0x2660d0,bytes:2000,type:'main.AppData',signature:'public void UpdateEffect() { }'},
    camera:{rva:0x29f310,bytes:176,type:'game.Camera',signature:'public static void ConvertScreenToCamera(int sx, int sy, int[] camera_pos) { }'},
    velocity:{rva:0x2a4a70,bytes:48,type:'game.GameUtil',signature:'public static float SetVStop(float dis, int t) { }'},
    acceleration:{rva:0x2a4990,bytes:64,type:'game.GameUtil',signature:'public static float SetAStop(float dis, int t) { }'}};
const manifest=Object.entries(ranges).map(([key,range])=>{
    const method=methods.find(m=>m.rva===range.rva&&m.type===range.type&&m.signature===range.signature);
    const next=Math.min(...methods.filter(m=>m.rva>range.rva).map(m=>m.rva));
    if(!method||range.rva+range.bytes>next||method.offset+range.bytes>dll.length)throw Error('named entry bounds');
    const pe=dll.readUInt32LE(0x3c),count=dll.readUInt16LE(pe+6),table=pe+24+dll.readUInt16LE(pe+20);
    let mapped=false;
    for(let i=0;i<count;i++){const at=table+i*40,rva=dll.readUInt32LE(at+12),size=dll.readUInt32LE(at+16),offset=dll.readUInt32LE(at+20);
        if(range.rva>=rva&&range.rva+range.bytes<=rva+size&&method.offset===offset+range.rva-rva&&
           (dll.readUInt32LE(at+36)&0x20000000))mapped=true;}
    if(!mapped)throw Error('PE executable mapping');
    return {key,...range,offset:method.offset,va:method.va,next,sha256:hash(dll.subarray(method.offset,method.offset+range.bytes))};
});
const key=process.argv[2];
if(key==='manifest'){console.log(JSON.stringify({ranges:manifest,unique_bytes:manifest.reduce((n,m)=>n+m.bytes,0)},null,2));process.exit(0);}
const selected=manifest.find(m=>m.key===(['drawCoin','drawTail'].includes(key)?'draw':key)),start=Number(process.argv[3]??0),count=Number(process.argv[4]??120);
if(!selected||!Number.isSafeInteger(start)||start<0||!Number.isSafeInteger(count)||count<1||count>160)throw Error('bounded named output');
const iced=archivePaths.require(path.join(root,'work/local-tools/iced-x86-1.21.0/package'));
const decoder=new iced.Decoder(32,dll.subarray(selected.offset,selected.offset+selected.bytes),iced.DecoderOptions.None);
decoder.ip=BigInt(selected.va);const formatter=new iced.Formatter(iced.FormatterSyntax.Intel);
const names=new Map(methods.map(m=>[m.va,m.type+'::'+m.signature]));let n=0;
while(decoder.canDecode){const i=decoder.decode();if(Number(i.ip)+i.length>selected.va+selected.bytes){i.free();break;}
    const sliced=key==='drawCoin'||key==='drawTail';
    const visible=key==='drawCoin'?Number(i.ip)>=0x1024e2d0&&Number(i.ip)<0x1024e4e2:
        key==='drawTail'?Number(i.ip)>=0x1024efa7&&Number(i.ip)<0x1024efd0:n>=start&&n<start+count;
    if(visible)console.log(n,'0x'+Number(i.ip).toString(16),formatter.format(i),i.isCallNear?(names.get(Number(i.nearBranchTarget))??''):'');
    i.free();if(++n>=start+count&&!sliced)break;}
formatter.free();decoder.free();
