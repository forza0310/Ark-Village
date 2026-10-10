// 固定Steam方法及共享状态helper的指令锚/字段审计，复用APK已核产生者。
const fs=require('fs'),path=require('path'),crypto=require('crypto'),cp=require('child_process');
const root=path.resolve(__dirname,'../..'),read=p=>fs.readFileSync(path.join(root,p));
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=read('DungeonVillageEXE/GameAssembly.dll'),meta=read('DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat');
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||
 sha(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('固定Steam输入');
const methods=JSON.parse(cp.execFileSync(process.execPath,[path.join(__dirname,'inspect.cjs'),'manifest'],{encoding:'utf8'}));
if(methods.reset.bytes!==160||methods.choose.bytes!==1392||methods.change.bytes!==32||methods.choose.signature!=='public static int GetAppearMonsterDataId() { }')throw Error('范围/具名身份');
const methodIndex=JSON.parse(read('work/persistence-replay-analysis/exe/methods.json'));
const changeAliases=methodIndex.methods.filter(m=>m.rva===0x2116c0).map(m=>({type:m.type,signature:m.signature}));
if(!changeAliases.some(m=>m.type==='data.MonsterData'&&m.signature==='public void ChangeState(int state) { }'))throw Error('共享状态入口缺少MonsterData别名');
const expected={
 reset:{
  '1021c8f8':'mov dword ptr [esi+10h],0','1021c8ff':'mov byte ptr [esi+18h],0',
  '1021c903':'mov dword ptr [esi+58h],0','1021c90a':'mov dword ptr [esi+5Ch],0',
  '1021c911':'mov dword ptr [esi+60h],0','1021c918':'mov dword ptr [esi+64h],0',
  '1021c91f':'mov byte ptr [esi+68h],0','1021c939':'push 1',
  '1021c93c':'call 10837C30h','1021c946':'je short 1021C95Dh',
  '1021c952':'mov dword ptr [esi+10h],1','1021c959':'mov byte ptr [esi+68h],1'},
 choose:{
  '1027e975':'mov eax,[eax+11Ch]','1027e986':'dec eax','1027e989':'jge 1027EA85h',
  '1027e9cb':'mov ebx,[edi+eax]','1027e9dd':'mov esi,[edi+esi+4]',
  '1027ea05':'push 4','1027ea0a':'call 10837C30h','1027ea1a':'jne short 1027EA61h',
  '1027ea20':'mov edx,[esi+30h]','1027ea48':'cmp edx,[eax+60h]','1027ea4b':'jg short 1027EA61h',
  '1027ea55':'cmp dword ptr [ebx+10h],1','1027ea5b':'cmp dword ptr [esi+10h],0',
  '1027ea6c':'cmp dword ptr [ebx+5Ch],8','1027ea70':'jl short 1027EA85h',
  '1027ea74':'push 1','1027ea77':'call 102116C0h',
  '1027eab8':'sub esi,1','1027eb28':'push 4','1027eb3d':'cmp dword ptr [edi+10h],1',
  '1027ebb6':'cmp eax,5','1027ebb9':'jge 1027ED9Ah','1027ebcb':'jns 1027EAC8h',
  '1027ec1e':'push esi','1027ec1f':'call 102A44E0h',
  '1027ec6c':'cmp byte ptr [esi+68h],0','1027ec70':'jne 1027ED90h',
  '1027ec8f':'push 1','1027ec9c':'jne 1027ED90h',
  '1027eccc':'call 102E03A0h','1027ecd6':'jne 1027ED90h',
  '1027ecdc':'mov eax,[esi+54h]','1027ece7':'cmp dword ptr [eax+0Ch],0',
  '1027eceb':'je short 1027ED2Eh','1027ed18':'push 2','1027ed20':'push 1',
  '1027ed26':'call 102558B0h','1027ed44':'push 59h','1027ed47':'call 1032EAD0h',
  '1027ed4c':'lea ecx,[edi+198h]','1027ed52':'mov [ecx],esi','1027ed84':'call 1025FD80h',
  '1027ed8c':'mov byte ptr [esi+68h],1','1027ed90':'mov eax,[esi+8]'},
 change:{'102116c9':'cmp dword ptr [ecx+10h],0','102116cd':'mov [ecx+10h],eax',
  '102116d0':'jne short 102116DAh','102116d2':'cmp eax,1','102116d5':'jne short 102116DAh','102116d7':'mov [ecx+18h],al'}
};
const anchors=[];
for(const [key,spec] of Object.entries(expected)){
 const text=cp.execFileSync(process.execPath,[path.join(__dirname,'inspect.cjs'),key],{encoding:'utf8'});
 const lines=new Map(text.trim().split('\n').map(line=>{const n=line.indexOf(' ');return [line.slice(0,n),line.slice(n+1).split(' ; ')[0]];}));
 for(const [va,wanted] of Object.entries(spec)){if(lines.get(va)!==wanted)throw Error('指令锚 '+va);anchors.push({method:key,va:'0x'+va,instruction:wanted});}
}
const dumpPath='work/persistence-replay-analysis/exe/dumper/dump.cs',dump=read(dumpPath).toString('utf8');
const fields={MonsterData:['public int monsterRank_; // 0x30','public int dieCnt_total_; // 0x5C',
 'public bool isAppearWindow_; // 0x68','public int[][] execsScript_; // 0x54'],
 BaseData:['public int id_; // 0x8','public int flag_; // 0xC','public int state_; // 0x10'],
 UserData:['public int questRank_; // 0x60'],SubForm:['public MonsterData monsterData_; // 0x198']};
for(const [type,expected] of Object.entries(fields)){
 const begin=dump.indexOf('public class '+type+' '),end=dump.indexOf('\n// Namespace:',begin+1);
 if(begin<0)throw Error('字段类型 '+type);const body=dump.slice(begin,end<0?undefined:end);
 for(const field of expected)if(!body.includes(field))throw Error('具名字段 '+type+' '+field);
}
const apkPath='work/task-selection-producers/DEX_FIELD_REFS.json',apk=JSON.parse(read(apkPath));
if(apk.dex_sha256!=='b4386a0de612fe18196a390633607ef36c69c2a63881244832314e3bf4902d1a')throw Error('APK引用身份');
const sourcePaths=['DungeonVillageEXE/GameAssembly.dll','DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat',
 'work/persistence-replay-analysis/exe/methods.json',dumpPath,apkPath,'example/src/encounter_ai.cpp',
 'example/src/encounter_creation.cpp','example/tests/encounter_creation_test.cpp',
 'prototype/src/startup_world_runtime_tasks.cpp','example/src/world_task_creation.cpp'];
const result={scope:'Steam普通怪物开放与介绍资格；非全部任务选择或原窗口动态',
 sources:sourcePaths.map(p=>{const b=read(p);return {path:p,bytes:b.length,sha256:sha(b)};}),
 methods,changeAliases,anchors,fields,contracts:{initial_flags:1,next_definition_minimum_previous_defeats:8,
 next_definition_chapter_gate:'monsterRank<=questRank',candidate_order:'reverse original order',
 maximum_candidates:5,excluded_monster_flag:4,selection_draw:'one GameUtil.Random(candidate_count)',
 introduction:'not isAppearWindow, not flags1, not IsSelectQuest',
 introduction_order:['optional definition script','raw89 binding and Push','isAppearWindow=true'],
 confirmation_required_to_set_introduction:false},
 limitations:['只核两个Steam怪物方法及32字节共享helper，不扫描所有反射/外部写点',
 '普通怪物开放和任务复发的用途已APK侧核；本批未核Steam完整任务选择器',
 '原Push返回值在本局部不检查；维护Owner失败整体回滚是保护策略',
 'Random内部算法和原UI输入不由本批认证'],
 scripts:['inspect.cjs','audit.cjs'].map(p=>{const b=fs.readFileSync(path.join(__dirname,p));return {path:p,bytes:b.length,sha256:sha(b)};})};
const text=JSON.stringify(result,null,2)+'\n';if(Buffer.byteLength(text)>64*1024)throw Error('证据输出预算');
fs.writeFileSync(path.join(__dirname,'EVIDENCE.json'),text);
console.log(JSON.stringify({methods:3,bytes:1584,anchors:anchors.length,fields:Object.values(fields).flat().length}));
