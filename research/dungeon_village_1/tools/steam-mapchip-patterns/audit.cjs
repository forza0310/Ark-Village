const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);
// 独立核两完整数组、真实调用锚与资源异常；不改变原表或生成替代裁片。
const fs=archivePaths.require('fs'),path=archivePaths.require('path'),crypto=archivePaths.require('crypto'),cp=archivePaths.require('child_process');
if(process.argv.length!==2)throw Error('no arguments');
const root=path.resolve(archivePaths.workDir,'../..'),sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const read=p=>fs.readFileSync(path.join(archivePaths.workDir,p));
const arraysBytes=cp.execFileSync(process.execPath,[path.join(archivePaths.workDir,'arrays.cjs'),'tenant'],{maxBuffer:1024*1024});
if(!arraysBytes.equals(read('ARRAYS.json')))throw Error('array regeneration');
const arrays=JSON.parse(arraysBytes),res=JSON.parse(read('RESOURCES.json'));
const expected=[[[[0],[1]],[[0,2],[1,3]],[[0,2,4,6],[1,3,5,7]]],[[[[0,0]],[[0,0]]],[[[0,1],[0,0]],[[-1,0],[0,0]]],[[[-1,1],[-1,0],[0,1],[0,0]],[[-1,1],[0,1],[-1,0],[0,0]]]]];
if(JSON.stringify(arrays.targets.map(t=>t.values))!==JSON.stringify(expected))throw Error('reviewed complete arrays');
if(arrays.inspected_bytes!==4381||arrays.targets[0].publication.at!==0x10225d9d||arrays.targets[1].publication.at!==0x102264ea)throw Error('array publication boundary');
const dll=fs.readFileSync(path.join(root,'DungeonVillageEXE/GameAssembly.dll')),index=archivePaths.require('../persistence-replay-analysis/exe/methods.json');
if(sha(dll)!==arrays.source.dll)throw Error('fixed DLL');
function bytes(va,n){const r=va-index.base,s=index.sections.find(s=>r>=s.rva&&r+n<=s.rva+s.rawSize);if(!s)throw Error('PE range');return dll.subarray(s.raw+r-s.rva,s.raw+r-s.rva+n);}
const anchors=[];
function fixed(va,hex,label){const b=Buffer.from(hex,'hex');if(!bytes(va,b.length).equals(b))throw Error(label);anchors.push({va,bytes:b.length,sha256:sha(b),label});}
fixed(0x10225d9d,'897048','publish SETPATTERN_MAPCHIP');
fixed(0x102264ea,'89784c','publish SETMAPCHIP_ADDRESS');
fixed(0x1022191c,'ff7634','tenant column9 source');
fixed(0x10221927,'89473c','tenant mapchipDataId store');
fixed(0x10221936,'ff7638','tenant column10 source');
fixed(0x10221941,'894740','tenant mapchipPattern store');
fixed(0x1021c046,'ff761c','mapchip column3 source');
fixed(0x1021c051,'894724','mapchip seb store');
fixed(0x1021c072,'ff7624','mapchip column5 source');
fixed(0x1021c07d,'89472c','mapchip reverse tenant store');
for(const [va,target,label] of [[0x107bc73e,0x107c4c20,'DrawFrame dispatch'],[0x107c4c79,0x107c28e0,'lineNo -1 takes all layers'],[0x107c2a17,0x107c1500,'per-layer frame resolution'],[0x1021bf79,0x107bc710,'mapchip DrawFrame consumer']]){
 const b=bytes(va,5);if(b[0]!==0xe8||va+5+b.readInt32LE(1)!==target)throw Error(label);anchors.push({va,bytes:5,sha256:sha(b),target,label});
}
fixed(0x107c1a6d,'0f8c83040000','before first keyframe returns null');
fixed(0x107c1a83,'0f8f6d040000','after last keyframe returns null');
fixed(0x107c1ef8,'33c0','null sprite result');
fixed(0x107bf9be,'0f84d3040000','null sprite skips drawing');
fixed(0x107bfe97,'5e8be55dc3','null sprite return');
const methods=cp.execFileSync(process.execPath,[path.join(archivePaths.workDir,'inspect.cjs'),'manifest'],{encoding:'utf8'}).trim().split('\n').map(JSON.parse);
const missing=res.sprites.flatMap(s=>s.parts.filter(p=>p.resource===null).map(p=>[s.index,p.frame,p.image]));
const badCrops=res.sprites.flatMap(s=>s.parts.filter(p=>p.resource!==null&&!p.crop_inside_image).map(p=>[s.index,p.frame,p.crop]));
const noFrames=res.joins.flatMap(j=>j.directions.flatMap(d=>d.pieces.filter(p=>p.resolution!=='exact').map(p=>[j.tenant_id,d.orientation,p.frame])));
const requestedBad=res.joins.flatMap(j=>j.directions.flatMap(d=>d.pieces.filter(p=>!p.crops_inside_images).map(p=>[j.tenant_id,d.orientation,p.frame])));
function same(a,b,label){if(JSON.stringify(a)!==JSON.stringify(b))throw Error(label);}
same(missing,[[67,2,43],[67,3,44]],'missing source image diagnostic');
same(badCrops,[[27,3,[0,87,60,29]],[38,1,[60,0,60,29]],[38,2,[120,0,60,29]]],'source crop diagnostics');
same(requestedBad,[[19,1,1]],'only direction1 sea request crosses image');
same(noFrames.map(x=>x[0]),[1,2,3,8,9,10,11,12,13,15,16,20,21,22,23,24,27,75,76,77,78,79,80,81,82,83,84],'outside layer keyframes');
if(noFrames.some(x=>x[1]!==1||x[2]!==1)||res.joins.length!==85||res.sprites.length!==87||res.identities.length!==194||res.joins.some(j=>j.tenant_id!==j.reverse_tenant_id||j.caller_pattern!==j.used_pattern))throw Error('join denominator/reverse identity');
const common=res.identities.filter(r=>r.group==='common'&&r.name.endsWith('.png'));
if(common.length!==2||common.some(r=>r.apk_bytes_equal||r.apk_pixels_equal))throw Error('two actual image differences');
const patternCounts=[0,1,2].map(p=>res.joins.filter(j=>j.used_pattern===p).length);same(patternCounts,[75,7,3],'pattern count');
const files=['inspect.cjs','arrays.cjs','resources.py','ARRAYS.json','RESOURCES.json'].map(p=>{const b=read(p);return {path:p,bytes:b.length,sha256:sha(b)};});
const out={scope:'Steam fixed mapchip arrays, table-to-SEB joins and explicit original resource anomalies',sources:arrays.source,methods,method_bytes:methods.reduce((n,m)=>n+m.bytes,0),anchors,arrays:arrays.targets.map(t=>({name:t.name,offset:t.offset,publication:t.publication,values:t.values})),pattern_counts:patternCounts,joins:85,sprites:87,resources:194,common_differences:common,diagnostics:{missing_image_records:missing,outside_image_records:badCrops,requested_outside_layer_ranges:noFrames,requested_outside_images:requestedBad,raw21_orientation0_all_exact_and_in_image:true},files,limits:['Static metadata/array/resource correspondence does not prove every definition can rotate or is unlocked','Original SEB header nominal frame counts do not constrain all stored keyframes','No full language resource replacement or frame interpolation certification','No C++/product mutation, asset export, original game/save access or build']};
fs.writeFileSync(path.join(archivePaths.workDir,'EVIDENCE.json'),JSON.stringify(out,null,2)+'\n');
console.log(JSON.stringify({methods:methods.length,method_bytes:out.method_bytes,anchors:anchors.length,joins:85,resources:194,outside_layer_requests:noFrames.length,requested_bad_crops:requestedBad.length}));
