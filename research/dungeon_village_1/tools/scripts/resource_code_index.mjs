// 资源反查索引：只输出身份、索引关系和引用坐标，不保存生成Java的源码片段。
// 字面量/固定槽引用是定位候选，不是可达性、完整绘制合同或Steam等价证明。
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const hash = b => crypto.createHash('sha256').update(b).digest('hex');
const relative = p => path.relative(root,p).replaceAll('\\','/');
const sourceRoot = path.join(root,'work/decompiled/sources');
const output = path.join(root,'assets/RESOURCE_CODE_INDEX.json');
const apkHash='1e52408aeaebec2d92a5e50b8c0d964566d0038456bc9b081e18b7d3446a4ef5';
assert.equal(hash(fs.readFileSync(path.join(root,'maoxianmigongcun.apk'))),apkHash,'APK身份');
const check = process.argv.includes('--check');
assert(process.argv.slice(2).every(x => x === '--check'), '仅支持 --check');
const files = p => fs.readdirSync(p,{withFileTypes:true}).sort((a,b)=>a.name.localeCompare(b.name,'en'))
    .flatMap(e=>e.isDirectory()?files(path.join(p,e.name)):[path.join(p,e.name)]);
const records = new Map();
const manifest = fs.readFileSync(path.join(root,'assets/MANIFEST.tsv'),'utf8').trimEnd().split(/\r?\n/);
for (const line of manifest.slice(1)) {
    const [key,archive,entry,sha] = line.split('\t');
    // 七个logical_key与原图共用一行；仍按原归档/条目计数，不把normalized副本另计。
    const group = archive.replace(/\.dat$/,'');
    const id = group+'/'+entry;
    assert(!records.has(id),id);
    const p = path.join(root,'assets/original',id), bytes = fs.readFileSync(p);
    assert.equal(hash(bytes),sha,id);
    records.set(id,{id,archive,entry,sha256:sha,bytes:bytes.length,
        kind:path.extname(entry).slice(1),normalized_key:key||null,slots:[],code_candidates:[],default_seb_users:[]});
}
assert.equal(records.size,761);
assert.deepEqual(files(path.join(root,'assets/original')).map(f=>path.relative(path.join(root,'assets/original'),f).replaceAll('\\','/')).sort(),
    [...records.keys()].sort(),'原始发布目录与来源清单逐文件相等');
const groups = [...new Set([...records.values()].map(r=>r.archive.replace(/\.dat$/,'')))].sort();
const tables = new Map(), unresolved = [], structures = [];

// 原加载器先保留显式ID，再为无ID项依次分配未占用的最小ID；重复文件名是合法别名。
for (const group of groups) for (const type of ['img','seb']) {
    const p = path.join(root,'assets/original',group,type+'.inf');
    if (!fs.existsSync(p)) continue;
    const rows=fs.readFileSync(p,'utf8').trim().split(/\r?\n/).filter(Boolean).map((line,n)=>{
        const cells=line.trim().split('\t');
        const explicit=cells.length>=2;
        const raw=explicit?cells[1]:cells[0], parts=raw.split(',');
        const name=parts[0].replace(/\.gif$/i,'.png');
        return {index:explicit?Number(cells[0]):null,entry:name,modifiers:parts.slice(1),line:n+1};
    });
    const used=new Set(rows.filter(r=>r.index!==null).map(r=>r.index));
    assert.equal(used.size,rows.filter(r=>r.index!==null).length,p);
    for(const row of rows) {
        if(row.index===null){let i=0;while(used.has(i))++i;row.index=i;used.add(i);}
        assert(Number.isSafeInteger(row.index)&&row.index>=0,p);
        const id=group+'/'+row.entry;
        assert(records.has(id),'INF目标缺失 '+id);
        records.get(id).slots.push({table:type,index:row.index,modifiers:row.modifiers,line:row.line});
    }
    tables.set(group+'/'+type,new Map(rows.map(r=>[r.index,group+'/'+r.entry])));
}
let layerCount=0,partCount=0;
for(const r of records.values()) if(r.kind==='seb') {
    const b=fs.readFileSync(path.join(root,'assets/original',r.id));let p=0;
    const i16=()=>{assert(p+2<=b.length,r.id);const v=b.readInt16BE(p);p+=2;return v;};
    const layers=i16(),frames=i16();assert(layers>=0&&layers<128&&frames>=0,r.id);
    const detail={id:r.id,frames,layers:[],image_references:[]};
    const group=r.archive.replace(/\.dat$/,'');
    for(let layer=0;layer<layers;layer++) {
        const count=i16(),tag=i16()&65535;assert(count>=0&&count<=10000,r.id);
        detail.layers.push({records:count,legacy_tag:tag});layerCount++;partCount+=count;
        for(let part=0;part<count;part++) {
            const fields=Array.from({length:10},i16);
            const [frame,index,x,y,w,h,dx,dy,flipX,flipY]=fields;
            const target=index<0?null:tables.get(group+'/img')?.get(index);
            if(index>=0&&!target)unresolved.push({kind:'seb_image_slot',resource:r.id,layer,part,index});
            detail.image_references.push({layer,part,frame,index,target:target??null,
                source_rect:[x,y,w,h],offset:[dx,dy],raw_parameters:[flipX,flipY],
                qualification:index<0?'negative_drawing_command':'default_image_binding_override_possible'});
            if(target)records.get(target).default_seb_users.push({resource:r.id,layer,part,frame});
        }
    }
    assert.equal(p,b.length,r.id);structures.push(detail);
}
assert.equal(layerCount,346);assert.equal(partCount,1228);

// 词法扫描仅去除注释和字符串对语法搜索的干扰；不尝试从JADX猜完整控制流。
function lex(text) {
    const masked=text.split(''), literals=[];let i=0;
    const blank=(a,b)=>{for(let n=a;n<b;n++)if(text[n]!=='\n'&&text[n]!=='\r')masked[n]=' ';};
    while(i<text.length) {
        if(text.startsWith('//',i)){const a=i;while(i<text.length&&text[i]!=='\n')i++;blank(a,i);}
        else if(text.startsWith('/*',i)){const a=i,end=text.indexOf('*/',i+2);i=end<0?text.length:end+2;blank(a,i);}
        else if(text[i]==='"'||text[i]==="'") {
            const a=i,q=text[i++];let raw='';
            while(i<text.length&&text[i]!==q){if(text[i]==='\\'){raw+=text.slice(i,i+2);i+=2;}else raw+=text[i++];}
            i++;if(q==='"'&&!raw.includes('\\'))literals.push({offset:a,value:raw});blank(a,i);
        } else i++;
    }
    return {code:masked.join(''),literals};
}
const bindings={p:'common',q:'common2',r:'human',s:'image',t:'event',u:'weapon',v:'monster',w:'effect','k.f129a':'title'};
const loaderEvidence=[];
const archiveOrder=['test','common','image','human','monster','weapon','effect','common2','map','event','xls','title','sound'];
const globalText=fs.readFileSync(path.join(sourceRoot,'d/a.java'),'utf8');
assert(globalText.includes('new String[]{'+archiveOrder.map(x=>'"'+x+'"').join(', ')+'}'),'固定归档号表');
for(const [field,group]of Object.entries(bindings)) {
    const file=group==='image'?'b/c.java':'b/a.java';
    const text=fs.readFileSync(path.join(sourceRoot,file),'utf8');
    const statement='this.av.'+field+'.a(kairo.android.c.e.a(1, '+archiveOrder.indexOf(group)+'));';
    const offset=text.indexOf(statement);assert(offset>=0,'加载组绑定 '+field);
    loaderEvidence.push({field,group,archive_index:archiveOrder.indexOf(group),source:'work/decompiled/sources/'+file,
        line:text.slice(0,offset).split('\n').length,qualification:'ordinary_jadx_loader_binding'});
}
const sourceIdentities=[], dynamic=[], unmatched=[];
const alias=new Map();
for(const r of records.values())for(const name of new Set([r.entry,r.entry.replace(/\.png$/,'.gif')])) {
    if(!alias.has(name))alias.set(name,[]);alias.get(name).push(r.id);
}
function indexEnd(code,start){let depth=1,p=start;while(p<code.length&&depth){if(code[p]==='[')depth++;if(code[p]===']')depth--;p++;}assert.equal(depth,0);return p-1;}
for(const file of files(sourceRoot).filter(f=>f.endsWith('.java'))) {
    const bytes=fs.readFileSync(file), text=bytes.toString('utf8'), src=relative(file);
    sourceIdentities.push({path:src,sha256:hash(bytes),bytes:bytes.length});
    const {code,literals}=lex(text);
    const lineAt=offset=>text.slice(0,offset).split('\n').length;
    for(const literal of literals) {
        const candidates=alias.get(literal.value);if(!candidates)continue;
        for(const id of candidates)records.get(id).code_candidates.push({source:src,line:lineAt(literal.offset),
            kind:'exact_filename_literal',ambiguous_groups:candidates.length});
    }
    const patterns=[{regex:/(?:this\.)?av\.(k\.f129a|[pqrstuvw])\.(f591a|f592b)\s*\[/g,
        group:m=>bindings[m[1]]}];
    if(src.endsWith('/d/a.java'))patterns.push({regex:/this\.([pqrstuvw])\.(f591a|f592b)\s*\[/g,group:m=>bindings[m[1]]});
    if(src.endsWith('/b/h.java'))patterns.push({regex:/this\.(f129a)\.(f591a|f592b)\s*\[/g,group:()=> 'title'});
    if(src.endsWith('/b/a.java'))patterns.push({regex:/this\.(m)\.(f591a|f592b)\s*\[/g,group:()=> 'load'});
    for(const {regex,group}of patterns)for(const m of code.matchAll(regex)) {
        const start=m.index+m[0].length,end=indexEnd(code,start),expr=code.slice(start,end).trim();
        const g=group(m),type=m[2]==='f591a'?'img':'seb';
        const site={source:src,line:lineAt(m.index),group:g,table:type};
        if(!/^\d+$/.test(expr)){dynamic.push({...site,expression_sha256:hash(Buffer.from(expr))});continue;}
        const index=Number(expr),target=tables.get(g+'/'+type)?.get(index);
        if(target)records.get(target).code_candidates.push({...site,index,kind:'bound_constant_slot'});
        else unmatched.push({...site,index,kind:'constant_slot_missing'});
    }
}
// 通用绘制helper、间接别名和算式下标不升级为精确消费者；全部保留未闭合入口。
const values=[...records.values()];
const summary={resources:values.length,png:values.filter(r=>r.kind==='png').length,seb:structures.length,
    inf:values.filter(r=>r.kind==='inf').length,source_files:sourceIdentities.length,
    resources_with_direct_candidates:values.filter(r=>r.code_candidates.length).length,
    direct_candidate_sites:values.reduce((n,r)=>n+r.code_candidates.length,0),
    resources_without_direct_candidates:values.filter(r=>!r.code_candidates.length).length,
    dynamic_slot_sites:dynamic.length,unmatched_constant_slots:unmatched.length,
    seb_layers:layerCount,seb_records:partCount,unresolved_default_image_slots:unresolved.length,
    fully_certified_consumers_from_lexical_scan:0};
const result={schema:1,input:'APK1.0.8-localized-resigned',apk_sha256:apkHash,
    scope:'已发布11视觉归档；其余APK/EXE素材见IMAGE_COVERAGE，不把无直接引用当未使用',
    qualification:'字符串和固定槽仅为源码定位候选；默认SEB图片可被运行时覆盖；Steam消费者另证',
    summary,loader_bindings:loaderEvidence,sources:sourceIdentities,records:values,seb_structures:structures,
    dynamic_slot_sites:dynamic,unmatched_constant_slots:unmatched,unresolved_default_image_slots:unresolved};
const serialized=JSON.stringify(result,null,2)+'\n';
if(check)assert.equal(fs.readFileSync(output,'utf8'),serialized,'资源反查索引须重新生成');
else fs.writeFileSync(output,serialized);
console.log(JSON.stringify({mode:check?'check':'generate',...summary,bytes:Buffer.byteLength(serialized),sha256:hash(Buffer.from(serialized))}));
