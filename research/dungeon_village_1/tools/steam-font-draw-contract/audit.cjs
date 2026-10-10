const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 文字落点／阴影／局部裁剪转换审计；止于Unity GUI或已有矩阵／字体接口。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto'),cp=archivePaths.require('child_process');
if(process.argv.length!==2)throw Error('no args');
const root=path.resolve(archivePaths.workDir,'../..'),read=p=>fs.readFileSync(path.join(root,p)),sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const dll=read('DungeonVillageEXE/GameAssembly.dll'),meta=read('DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat'),ix=JSON.parse(read('work/persistence-replay-analysis/exe/methods.json'));
if(sha(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||sha(meta)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')throw Error('fixed source');
function bytes(va,n){const r=va-ix.base,s=ix.sections.find(s=>r>=s.rva&&r+n<=s.rva+s.rawSize);if(!s)throw Error('PE');return dll.subarray(s.raw+r-s.rva,s.raw+r-s.rva+n);}
const methods=cp.execFileSync(process.execPath,[path.join(archivePaths.workDir,'inspect.cjs'),'manifest'],{encoding:'utf8'}).trim().split('\n').map(JSON.parse);
if(methods.reduce((n,m)=>n+m.bytes,0)>8192)throw Error('fixed method batch 8KiB');
const iced=archivePaths.require('../local-tools/iced-x86-1.21.0/package'),ins=new Map();
for(const m of methods){const d=new iced.Decoder(32,bytes(ix.base+m.rva,m.bytes),iced.DecoderOptions.None);d.ip=BigInt(ix.base+m.rva);const f=new iced.Formatter(iced.FormatterSyntax.Intel);while(d.canDecode){const i=d.decode();if(i.isInvalid){i.free();break;}ins.set(Number(i.ip),{va:Number(i.ip),bytes:i.length,text:f.format(i),target:i.isCallNear?Number(i.nearBranchTarget):null});i.free();}f.free();d.free();}
const anchors=[];
function exact(va,text){const i=ins.get(va);if(!i||i.text!==text)throw Error('instruction '+va.toString(16)+' '+i?.text);anchors.push({...i,sha256:sha(bytes(va,i.bytes))});}
function call(va,target,label){const i=ins.get(va);if(!i||i.target!==target)throw Error(label);anchors.push({...i,label,sha256:sha(bytes(va,i.bytes))});}
for(const [va,text] of [
 [0x1076629a,'push 11h'],[0x10766767,'test byte ptr [ebp+1Ch],60h'],[0x1076677e,'sub edi,[eax+14h]'],[0x10766788,'mulss xmm0,[10DDE528h]'],
 [0x10766804,'test al,2'],[0x10766808,'test al,4'],[0x1076687e,'mulss xmm0,xmm1'],[0x10766894,'test al,20h'],[0x10766898,'test al,40h'],
 [0x1076689c,'cmp byte ptr [edi+1C4h],0'],[0x107668b9,'imul eax,esi,64h'],[0x107668bd,'idiv ecx'],[0x107668bf,'sub eax,esi'],[0x107668c4,'sar eax,1'],
 [0x1076690e,'mulss xmm0,xmm1'],[0x10766b44,'addss xmm0,[10DDE5B4h]'],[0x10766b4f,'cvttsd2si eax,xmm0'],
 [0x10766d40,'subss xmm1,xmm0'],[0x10766d60,'subss xmm1,xmm0'],[0x10766f89,'mulss xmm0,[10DDE560h]'],[0x10766fa1,'mov dword ptr [esp+8],3F800000h'],
 [0x1076611e,'mov eax,[eax+30h]'],[0x10766121,'inc eax'],[0x10766142,'inc edi'],[0x10766153,'mulss xmm1,[eax+2Ch]'],[0x10766163,'divss xmm1,xmm0'],
 [0x10766175,'addss xmm1,[ebp-8]'],[0x1076619d,'addss xmm0,[ebp-4]'],
 [0x1077d98e,'mulss xmm0,[10DDE528h]'],[0x1077bceb,'movss xmm1,[10DDE54Ch]'],[0x1077bd00,'cvttsd2si eax,xmm0'],
 [0x1075d5d1,'cmp dword ptr [esi+164h],0'],[0x1075d5da,'cmp dword ptr [esi+180h],0'],[0x1075d75d,'mov dword ptr [edi],0'],[0x1075d763,'mov dword ptr [ebx],0']])exact(va,text);
for(const [va,target,label] of [
 [0x107662b7,0x107663d0,'plain default anchor wrapper'],[0x107678e8,0x107663d0,'explicit anchor wrapper'],
 [0x107664c7,0x108211a0,'translate'],[0x107665bd,0x107ca250,'tags/newline delegate to TextLayout'],
 [0x10766692,0x107c8ec0,'TextFormat x'],[0x107666b5,0x107c8f60,'TextFormat y'],[0x107666d0,0x107c8d70,'TextFormat anchor'],
 [0x10766749,0x1077d810,'localize size'],[0x107667c5,0x1075d570,'begin render matrix before anchor'],
 [0x10766820,0x1077e770,'right width shared F/F2 entry'],[0x10766864,0x1077e770,'center width shared F/F2 entry'],
 [0x10766a20,0x1076f4a0,'font-size rectangle transformation'],[0x10766b55,0x10c30680,'GUIStyle font size'],
 [0x10766ccd,0x107abf50,'screen matrix text position'],[0x10766f05,0x10821340,'UnicodeToChanakya'],[0x10766f3a,0x10775540,'GUI group'],[0x10766fec,0x10c37e00,'GUI Label boundary'],
 [0x10765f5c,0x10765820,'tagged shadow separate consumer'],[0x107660f6,0x10773c90,'shadow color'],
 [0x10766190,0x107663d0,'shadow positive x'],[0x107661ca,0x107663d0,'shadow positive y'],[0x107661f3,0x107663d0,'shadow positive x and y'],
 [0x10766206,0x10773c90,'restore foreground color'],[0x1076622f,0x107663d0,'foreground unshifted'],[0x10766253,0x1077eb30,'unlock localize'],[0x10766277,0x1077df50,'restore temporary size'],
 [0x1077bc4a,0x1075da80,'clip check'],[0x1077bd2d,0x1073c9a0,'integer clip rectangle'],
 [0x1076f504,0x107abf50,'rectangle left/top map'],[0x1076f528,0x107abf50,'rectangle right/bottom map'],
 [0x1075d755,0x1075d1d0,'local coordinate delegate'],[0x1075d795,0x107acfa0,'world pretranslate'],[0x1075d79d,0x107757f0,'update screen matrix']])call(va,target,label);
const constants=[[0x10dde528,0x3f000000],[0x10dde54c,0x3c23d70a],[0x10dde560,0x40000000],[0x10dde5b4,0x3a83126f]].map(([va,bits])=>{const b=bytes(va,4);if(b.readUInt32LE()!==bits)throw Error('float bits');return {va,bits:bits.toString(16).padStart(8,'0'),value:b.readFloatLE(),sha256:sha(b)};});
const dumpPath='work/persistence-replay-analysis/exe/dumper/dump.cs',dump=read(dumpPath).toString('utf8');
const fields=['public const int ANCHOR_LEFT = 1;','public const int ANCHOR_CENTER = 2;','public const int ANCHOR_RIGHT = 4;','public const int ANCHOR_TOP = 16;','public const int ANCHOR_MIDDLE = 32;','public const int ANCHOR_BOTTOM = 64;','protected int color_; // 0x34','private bool isGUI_; // 0x4C','protected Rectangle clipRegion_; // 0x12C','protected Matrix screenMatrix_; // 0x168','internal bool fontScaleOffsetEnable_; // 0x1C4','internal static float outlineWidth_; // 0x2C','internal static int outlineQuality_; // 0x30','private static int lineSpacing_; // 0x34','internal int usedSize_; // 0x14','internal int fontScale_; // 0x24'];for(const f of fields)if(!dump.includes(f))throw Error('field mapping '+f);
const widthAliases=ix.methods.filter(m=>m.rva===0x77e770).map(m=>m.signature);if(widthAliases.length!==2)throw Error('shared width aliases');
const reused=['work/steam-font-consumer/EVIDENCE.json','ui/STEAM_FONT_CONSUMER.md','work/skin-observation-analysis/ANALYSIS.md'].map(p=>({path:p,bytes:read(p).length,sha256:sha(read(p))}));
const scripts=['inspect.cjs','audit.cjs'].map(p=>{const b=fs.readFileSync(path.join(archivePaths.workDir,p));if(b[0]===239||b.includes(13))throw Error('UTF8 LF');return {path:p,bytes:b.length,sha256:sha(b)};});
const out={scope:'Graphics logical text positioning, ordinary shadow and clip rounding; not native glyph baseline or complete renderer',sources:{dll:sha(dll),metadata:sha(meta),methods:sha(read('work/persistence-replay-analysis/exe/methods.json')),dump:sha(read(dumpPath))},methods,anchors,constants,fields,width_aliases:widthAliases,contracts:{plain_default_anchor:17,shadow_calls:'3*(outlineQuality+1)+1 under ordinary text branch and nonnegative quality',shadow_offsets:['delta,0','0,delta','delta,delta'],clip_rounding:'trunc(float32(edge + float32(0.01)))',font_width:'float32(usedSize*0.5)',gui_anchor_stage:'after BeginRenderMatrix, before screen MapPoints',font_size_rounding:'trunc(float32(usedSize*min(scaleX,scaleY)+float32(0.001)))',gui_label_width:1,gui_label_height:'2*transformed font rectangle height'},reused,scripts,limitations:['No actual font family chosen by current running Chinese UI','No glyph ascent/descent/native baseline or kerning proof','TextLayout tagged shadow, GLText and font-texture branches remain separate','No full Matrix internals/Unity GUI clipping or external plugin internals','No default outlineWidth/outlineQuality value assertion','Observed JPEG dimensions are not DPI or scale calibration']};
fs.writeFileSync(path.join(archivePaths.workDir,'EVIDENCE.json'),JSON.stringify(out,null,2)+'\n');console.log(JSON.stringify({methods:methods.length,bytes:methods.reduce((n,m)=>n+m.bytes,0),anchors:anchors.length,float_constants:constants.length,fields:fields.length}));
