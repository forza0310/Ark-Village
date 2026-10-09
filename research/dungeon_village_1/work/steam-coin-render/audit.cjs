// 固定Steam X4来源和资源复核；仅保存有限身份/中文合同，不复制反编译实现。
const fs=require('fs'),path=require('path'),crypto=require('crypto'),cp=require('child_process');
if(process.argv.length!==2)throw Error('no arguments');
const root=path.resolve(__dirname,'../..'),read=p=>fs.readFileSync(path.join(root,p));
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=read('DungeonVillageEXE/GameAssembly.dll'),metadata=read('DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat');
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||
   sha(metadata)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('fixed source');
const index=JSON.parse(read('work/persistence-replay-analysis/exe/methods.json'));
const bytes=(va,n)=>{const rva=va-index.base,s=index.sections.find(s=>rva>=s.rva&&rva+n<=s.rva+s.rawSize);
    if(!s)throw Error('mapped range');return dll.subarray(s.raw+rva-s.rva,s.raw+rva-s.rva+n);};
const manifest=JSON.parse(cp.execFileSync(process.execPath,[path.join(__dirname,'inspect.cjs'),'manifest'],{encoding:'utf8'}));
if(manifest.unique_bytes!==16240||manifest.ranges.length!==6||manifest.unique_bytes>16384)throw Error('method budget');
const assertions=[];
function call(va,target,label){const b=bytes(va,5);if(b[0]!==0xe8||va+5+b.readInt32LE(1)!==target)throw Error(label);
    assertions.push({va,bytes:5,sha256:sha(b),target,label});}
for(const [va,target,label] of [[0x1024e324,0x1029f310,'X4固定屏幕点转当前相机'],
    [0x1024e4d5,0x102516f0,'显式SEB94和图片144绘制'],[0x10246104,0x102a4a70,'SetVStop'],
    [0x10246165,0x102a4990,'SetAStop'],[0x102461f2,0x10650450,'完整六字段追加到X'],
    [0x10266858,0x106564c0,'达到寿命后按当前索引删除']])call(va,target,label);
const priorPath='work/headless-save-audit-analysis/steam-static/disassembly.json';
const prior=read(priorPath),cctor=JSON.parse(prior).find(m=>m.method.rva===0x266a60);
if(!cctor||sha(bytes(cctor.method.va,cctor.size))!==cctor.sha256)throw Error('reused named static initializer identity');
for(const [va,hex] of [[0x102692cf,'c74010f0ffffff'],[0x102692e0,'8981a4000000'],
    [0x1026931b,'c7401017000000'],[0x1026932c,'8981a8000000']])
    if(bytes(va,hex.length/2).toString('hex')!==hex)throw Error('coin static value/field');
const constant=bytes(0x10dde62c,4);if(constant.readFloatLE()!==-2)throw Error('SetAStop coefficient');
const windows=[[0x1024bc49,0x1024bc52,'kind4分派'],[0x1024e2d0,0x1024e4e2,'X4完整局部分支'],
    [0x1024efa7,0x1024efc3,'原索引前向绘制循环'],[0x10266767,0x102667a7,'X4更新增加计数并读TIME'],
    [0x10266847,0x1026686f,'达到上限退休且循环索引继续增加'],
    [0x102692ad,0x10269344,'复用静态初始化[-16]与[23]']].map(([va,end,label])=>
        ({va,end,bytes:end-va,sha256:sha(bytes(va,end-va)),label}));
const coverageBytes=read('assets/IMAGE_COVERAGE.json'),records=JSON.parse(coverageBytes).records.filter(r=>r.source==='EXE'&&r.group==='common');
const resources=[];
for(const [kind,id,name] of [['image',144,'eff_coin.png'],['sprite',94,'eff_coin.seb']]){
    const record=records.find(r=>r.entry===name),inf=records.find(r=>r.entry===(kind==='image'?'img.inf':'seb.inf'));
    if(!record||!inf.index_targets.some(t=>t.index===id&&t.filename===name&&t.record===record.id))throw Error('Steam index binding');
    const raw=read('assets/original/common/'+name);if(raw.length!==record.bytes||sha(raw)!==record.sha256)throw Error('same-byte source resource');
    const item={kind,index:id,id:record.id,path:'../../assets/original/common/'+name,bytes:raw.length,sha256:sha(raw)};
    if(kind==='image'){if(raw.readUInt32BE(16)!==70||raw.readUInt32BE(20)!==10)throw Error('coin atlas size');item.size=[70,10];}
    else{if(raw.readInt16BE(0)!==1||raw.readInt16BE(2)!==7||raw.readInt16BE(4)!==7||raw.length!==148)throw Error('coin SEB');
        item.parts=Array.from({length:7},(_,n)=>Array.from({length:10},(_,j)=>raw.readInt16BE(8+n*20+j*2)));
        for(const [n,part] of item.parts.entries())if(JSON.stringify(part)!==JSON.stringify([n,144,10*n,0,10,10,-5,-10,0,0]))throw Error('coin source part');}
    resources.push(item);
}
const dump=read('work/persistence-replay-analysis/exe/dumper/dump.cs'),lines=dump.toString('utf8').split(/\r?\n/);
const fields=lines.map((text,i)=>({line:i+1,text:text.trim()})).filter(x=>
    x.line>=219790&&x.line<=219840&&/EFF_COIN/.test(x.text)||
    x.line>=221064&&x.line<=221140&&/static (Vector3D pos|int DRAW_)/.test(x.text));
const output={scope:'Steam2.56 X4静态局部，非原窗口/所有层序',dll_sha256:sha(dll),metadata_sha256:sha(metadata),
    methods:manifest,assertions,windows,static_values:{dis_y:[-16],time:[23],acceleration_float:{va:0x10dde62c,hex:constant.toString('hex'),value:-2}},
    reused_initializer:{path:'../headless-save-audit-analysis/steam-static/disassembly.json',sha256:sha(prior),method:cctor.method,method_sha256:cctor.sha256},
    resource_index:{path:'../../assets/IMAGE_COVERAGE.json',sha256:sha(coverageBytes)},resources,
    field_source:{path:'../persistence-replay-analysis/exe/dumper/dump.cs',sha256:sha(dump),fields},
    limitations:['未核死亡调用AddEffectCoin传入wait的Steam具体链','未核DrawEffect相对场景/HUD完整相位及实际帧频','未运行原游戏/未改维护代码或档案'],
    scripts:['inspect.cjs','audit.cjs'].map(name=>{const b=fs.readFileSync(path.join(__dirname,name));return {path:name,bytes:b.length,sha256:sha(b)};})};
fs.writeFileSync(path.join(__dirname,'EVIDENCE.json'),JSON.stringify(output,null,2)+'\n');
console.log(JSON.stringify({methods:manifest.ranges.length,method_bytes:manifest.unique_bytes,calls:assertions.length,resources:resources.length,resource_bytes:626}));
