// 只沿已核 TitleForm.cctor 的一个 RuntimeFieldHandle 解析11元素初始化值；不借APK。
const fs=require('fs'),path=require('path'),crypto=require('crypto');
if(process.argv.length!==2)throw Error('no arguments');
const root=path.resolve(__dirname,'../..');
const dll=fs.readFileSync(path.join(root,'DungeonVillageEXE/GameAssembly.dll'));
const metadata=fs.readFileSync(path.join(root,'DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat'));
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
if(hash(dll)!=='9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a'||hash(metadata)!=='80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369'||metadata.readInt32LE(4)!==29)throw Error('source identity/version');
const index=JSON.parse(fs.readFileSync(path.join(root,'work/persistence-replay-analysis/exe/methods.json')));
const slot=0x110f2100,rva=slot-index.base;
const section=index.sections.find(s=>rva>=s.rva&&rva+4<=s.rva+s.rawSize);
if(!section||section.raw+section.rawSize>dll.length)throw Error('slot PE mapping');
const slotOffset=section.raw+rva-section.rva,encoded=dll.readUInt32LE(slotOffset);
if(encoded>>>29!==4||!(encoded&1))throw Error('field usage');
function table(header,stride){const offset=metadata.readUInt32LE(header),bytes=metadata.readUInt32LE(header+4);if(bytes%stride||offset+bytes>metadata.length)throw Error('metadata table');return {offset,count:bytes/stride,bytes};}
const refs=table(184,8),types=table(160,88),fields=table(96,12),defaults=table(64,12);
const strings=table(24,1),data=table(72,1),referenceIndex=(encoded&0x1fffffff)>>>1;
if(referenceIndex>=refs.count)throw Error('reference bound');
const refOffset=refs.offset+referenceIndex*8,typeIndex=metadata.readInt32LE(refOffset),localField=metadata.readInt32LE(refOffset+4);
const typeDefinition=4129,typeOffset=types.offset+typeDefinition*88;
if(typeDefinition>=types.count||metadata.readInt32LE(typeOffset+8)!==typeIndex||localField<0)throw Error('private data type identity');
const fieldIndex=metadata.readInt32LE(typeOffset+32)+localField;
if(fieldIndex<0||fieldIndex>=fields.count)throw Error('field index');
const fieldOffset=fields.offset+fieldIndex*12,nameIndex=metadata.readUInt32LE(fieldOffset),nameOffset=strings.offset+nameIndex;
if(nameIndex>=strings.bytes)throw Error('field name offset');
const nameEnd=metadata.indexOf(0,nameOffset);
if(nameEnd<nameOffset||nameEnd>=strings.offset+strings.bytes||nameEnd-nameOffset!==64)throw Error('private data digest name');
const fieldName=metadata.toString('utf8',nameOffset,nameEnd);
const matches=[];
for(let i=0;i<defaults.count;i++){const p=defaults.offset+i*12;if(metadata.readInt32LE(p)===fieldIndex)matches.push(p);}
if(matches.length!==1)throw Error('unique default record');
const dataIndex=metadata.readInt32LE(matches[0]+8),length=11;
if(dataIndex<0||dataIndex+length*4>data.bytes)throw Error('array default bounds');
const dataOffset=data.offset+dataIndex,bytes=metadata.subarray(dataOffset,dataOffset+length*4);
if(hash(bytes).toUpperCase()!==fieldName)throw Error('private data digest');
console.log(JSON.stringify({qualification:'Steam固定cctor实参链；非APK替代',dll_sha256:hash(dll),metadata_sha256:hash(metadata),
 name:'TitleForm.TITLE_GUEST',call:'TitleForm.cctor fixed field handle',slot,slotOffset,encoded,referenceIndex,refOffset,typeIndex,typeDefinition,
 localField,fieldIndex,fieldOffset,fieldName,fieldDefaultOffset:matches[0],dataIndex,dataOffset,length,
 values:Array.from({length},(_,i)=>bytes.readInt32LE(i*4)),bytes_sha256:hash(bytes)},null,2));
