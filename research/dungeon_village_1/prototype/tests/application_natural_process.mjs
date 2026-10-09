// 自然应用三进程认证：reference从新局或已认证前缀继续，只比较新捕获后的短尾段。
// 采用完整应用格式和实际Driver；不调用旧晋级策略，不修改原表或预算。
import fs from 'node:fs/promises';
import path from 'node:path';
import {spawn} from 'node:child_process';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';

const researchRoot=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const hash=b=>createHash('sha256').update(b).digest('hex');
const need=(v,m)=>{if(!v)throw Error(m);};
const within=(root,p)=>{const r=path.relative(root,p);return !!r&&!r.startsWith('..')&&!path.isAbsolute(r);};
const integer=(value,fallback,min,max)=>{
    const text=String(value??fallback);need(/^\d+$/.test(text),'整数参数非法');
    const n=Number(text);need(Number.isSafeInteger(n)&&n>=min&&n<=max,'整数参数越界');return n;
};

// 仅校验容器与metadata身份，完整应用/Driver/引用恢复仍由C++权威加载器核验。
function identity(bytes){
    need(bytes.length>84&&bytes.length<=128*1024*1024,'应用文件预算');
    need(bytes.subarray(-64).toString('ascii')===hash(bytes.subarray(0,-64)),'应用整体摘要');
    const reader=b=>({at:0,b,raw(n){need(Number.isSafeInteger(n)&&n>=0&&n<=this.b.length-this.at,'分区边界');
        const v=this.b.subarray(this.at,this.at+n);this.at+=n;return v;},
        u32(){return this.raw(4).readUInt32LE();},u64(){const n=this.raw(8).readBigUInt64LE();
            need(n<=BigInt(Number.MAX_SAFE_INTEGER),'不可表示的整数');return Number(n);},
        text(){const n=this.u32();need(n<=4096,'身份文本预算');return this.raw(n).toString('utf8');}});
    const r=reader(bytes.subarray(0,-64));
    need(r.raw(8).toString()==='AVRAPP01'&&r.u32()===1&&r.u32()===6&&r.u32()===1,'应用版本');
    const dataset=r.text(),world_schema=r.text(),application_schema=r.text(),count=r.u32();
    need(count>=6&&count<=66,'自然应用分区数量');
    const sections=new Map();const sizes={};
    for(let i=0;i<count;i++){
        const id=r.u32(),version=r.u32(),required=r.u32(),length=r.u64(),digest=r.raw(64).toString();
        const body=r.raw(length);need(!sections.has(id)&&version>0&&required<=1&&hash(body)===digest,'分区身份/摘要');
        sections.set(id,{version,required,body});sizes[id]=length;
    }
    need(r.at===r.b.length,'容器尾部');
    for(const id of [1,2,3,4,5,6])need(sections.get(id)?.version===1&&sections.get(id)?.required===1,'自然应用缺段');
    const meta=reader(sections.get(1).body),controller=meta.text(),producer=meta.text();
    const next_frame=meta.u64(),next_command=meta.u64();need(meta.at===meta.b.length,'metadata尾部');
    need(controller==='application-natural-clear-v3'&&next_frame>1,'自然Driver身份');
    return {controller,producer,next_frame,next_command,dataset,world_schema,application_schema,section_bytes:sizes};
}

export async function verifyNaturalApplication(options){
    const exe=path.resolve(options.exe),realWork=await fs.realpath(path.join(researchRoot,'work'));
    const work=await fs.realpath(path.resolve(options.workDir));
    need(within(realWork,work),'实际工作目录必须位于research/work内');
    const month=options.saveMonth===undefined?null:integer(options.saveMonth,1,1,180);
    need(month===null||options.saveAt===undefined,'保存月数和轮数互斥');
    const saveAt=month===null?integer(options.saveAt,420,1,999999):null;
    const tail=integer(options.tailFrames,20,1,1000),limit=integer(options.frameLimit,1000000,1,1000000);
    const timeout=integer(options.timeoutSeconds,600,1,3600)*1000;
    let output;
    if(options.snapshotFile){
        const selected=path.resolve(options.snapshotFile),parent=await fs.realpath(path.dirname(selected));
        need(within(realWork,parent),'证书输出父目录必须位于research/work内');
        output=path.join(parent,path.basename(selected));
        for(const p of [output,output+'.json']){
            try{await fs.lstat(p);throw Error('快照/证书已存在，禁止覆盖');}catch(e){if(e.code!=='ENOENT')throw e;}
        }
    }
    let source=null,sourceBytes,sourceCertificateBytes;
    if(options.loadPrefix){
        const file=await fs.realpath(path.resolve(options.loadPrefix));need(within(realWork,file),'源前缀越界');
        const stat=await fs.stat(file);need(stat.size>0&&stat.size<=128*1024*1024,'源文件预算');
        sourceBytes=await fs.readFile(file);const meta=identity(sourceBytes);
        const certificatePath=await fs.realpath(file+'.json');
        need(within(realWork,certificatePath),'源证书实际路径越界');
        const certificateStat=await fs.stat(certificatePath);
        need(certificateStat.isFile()&&certificateStat.size>0&&certificateStat.size<=1024*1024,'源证书预算');
        sourceCertificateBytes=await fs.readFile(certificatePath);const cert=JSON.parse(sourceCertificateBytes);
        need(sourceCertificateBytes.length===certificateStat.size,'源证书读取期间尺寸变化');
        need(cert.controller===meta.controller&&cert.qualification==='natural_application_tail'&&
             cert.snapshot_sha256===hash(sourceBytes)&&cert.snapshot_bytes===sourceBytes.length&&
             Number.isSafeInteger(cert.capture_frame)&&cert.capture_frame>=1&&
             cert.capture_frame+1===meta.next_frame&&cert.process_count===3&&
             Number.isSafeInteger(cert.tail_frames)&&cert.tail_frames>0&&cert.tail_frames<=1000&&
             cert.stop_at===cert.capture_frame+cert.tail_frames&&
             Number.isSafeInteger(cert.capture_months)&&cert.capture_months>=0&&cert.capture_months<=181&&
             /^[0-9a-f]{64}$/.test(cert.trace_sha256)&&Number.isSafeInteger(cert.trace_bytes)&&cert.trace_bytes>0,
             '源证书与实际前缀不符');
        for(const k of ['dataset','world_schema','application_schema'])need(cert[k]===meta[k],'源身份不符 '+k);
        need(saveAt===null||saveAt>=meta.next_frame,'捕获必须晚于源前缀');
        source={file,certificate_file:certificatePath,snapshot_sha256:hash(sourceBytes),certificate_sha256:hash(sourceCertificateBytes),
            next_frame:meta.next_frame,capture_frame:cert.capture_frame,months:cert.capture_months,
            qualification:cert.qualification};
        need(month===null||Number.isSafeInteger(cert.capture_months)&&month>cert.capture_months,'保存月必须晚于源');
    }
    const owned=await fs.mkdtemp(path.join(work,'application-natural-'));
    let child,cancelled=false,passed=false,invocation=0;const seconds=[];
    const cancel=()=>{cancelled=true;child?.kill();};
    process.on('SIGINT',cancel);process.on('SIGTERM',cancel);
    async function run(args){
        need(!cancelled,'测试被取消');const index=invocation++,started=performance.now();
        return await new Promise((resolve,reject)=>{
            const p=spawn(exe,args,{cwd:path.dirname(exe),windowsHide:true,stdio:['ignore','pipe','pipe']});child=p;
            const out=[],err=[];let timedOut=false,launchError;
            const timer=setTimeout(()=>{timedOut=true;p.kill();},timeout);
            p.stdout.on('data',b=>{out.push(b);process.stdout.write(b);});p.stderr.on('data',b=>err.push(b));
            p.on('error',e=>{launchError=e;});
            p.on('close',async code=>{
                clearTimeout(timer);child=undefined;const stdout=Buffer.concat(out).toString(),stderr=Buffer.concat(err).toString();
                try{await fs.writeFile(path.join(owned,`process-${index}.log`),stdout+'\n'+stderr,{flag:'wx'});
                    need(!launchError&&!timedOut&&!cancelled&&code===0&&!stderr.trim(),
                         `自然应用进程失败 code=${code} timeout=${timedOut}: ${launchError??''}\n${stderr}`);
                    seconds.push((performance.now()-started)/1000);resolve(stdout);
                }catch(e){reject(e);}
            });
        });
    }
    const summary=text=>{
        const rows=text.split(/\r?\n/).filter(s=>s.startsWith('application-natural-summary '));
        need(rows.length===1,'缺少唯一自然应用终点');return JSON.parse(rows[0].slice('application-natural-summary '.length));
    };
    try{
        const dirs=[0,1,2].map(i=>path.join(owned,'process-'+i));for(const p of dirs)await fs.mkdir(p);
        const traces=dirs.map(p=>path.join(p,'tail.jsonl')),snapshot=path.join(dirs[0],'prefix.avra');
        const reference=summary(await run(['application-natural-clear-v3','--work-dir',dirs[0],
            '--trace-file',traces[0],'--stop-at',String(limit),'--save-file',snapshot,
            ...(month===null?['--save-at',String(saveAt)]:['--save-month',String(month)]),
            '--tail-after-save',String(tail),...(source?['--load-file',source.file]:[])]));
        const bytes=await fs.readFile(snapshot),meta=identity(bytes),capture=meta.next_frame-1;
        need(reference.capture_frame===capture&&reference.completed_frame===capture+tail,'捕获/尾段轮数');
        const expected=await fs.readFile(traces[0]),rows=expected.toString().trimEnd().split('\n').map(s=>JSON.parse(s));
        need(rows.length===tail&&rows.every((r,i)=>r.frame===capture+i+1&&r.next_frame===r.frame+1&&
            /^[0-9a-f]{64}$/.test(r.digest)&&/^[0-9a-f]{64}$/.test(r.system_digest)&&Array.isArray(r.sounds)&&
            r.sounds.every(s=>Array.isArray(s)&&s.length===2&&s.every(Number.isSafeInteger)&&
                s[0]>=0&&s[0]<=2&&s[1]>=0&&s[1]<26)),
            '尾段完整逐轮字段/顺序');
        for(let i=1;i<3;i++){
            const actual=summary(await run(['application-natural-clear-v3','--work-dir',dirs[i],
                '--trace-file',traces[i],'--stop-at',String(reference.completed_frame),'--load-file',snapshot]));
            need(expected.equals(await fs.readFile(traces[i])),'三路完整trace不同');
            for(const key of ['completed_frame','next_frame','months','digest','sound_count'])
                need(actual[key]===reference[key],'终点字段不同 '+key);
            need(JSON.stringify(actual.resources)===JSON.stringify(reference.resources),'资源累计不同');
            need(bytes.equals(await fs.readFile(snapshot)),'恢复改写源快照');
        }
        const certificate={controller:meta.controller,qualification:'natural_application_tail',
            producer_revision:options.producerRevision??'unspecified',seed:1,speed:0,
            dataset:meta.dataset,world_schema:meta.world_schema,application_schema:meta.application_schema,
            source_prefix:source,capture_frame:capture,capture_months:reference.capture_months,
            stop_at:reference.completed_frame,tail_frames:tail,process_count:3,
            snapshot_bytes:bytes.length,snapshot_sha256:hash(bytes),section_bytes:meta.section_bytes,
            trace_bytes:expected.length,trace_sha256:hash(expected),terminal:reference,process_wall_seconds:seconds,
            certification_boundary:source?'已认证源恢复后继续；本轮仅认证新捕获尾段':'本轮真实新局reference及指定捕获尾段',
            limitations:['被动自然日期策略，不认证五星/全部任务/BOSS','不认证原APK/Steam动态或自动绘制频率']};
        need(Number.isSafeInteger(certificate.capture_months)&&certificate.capture_months>=0,'捕获月份缺失');
        if(output){
            await fs.writeFile(output,bytes,{flag:'wx'});
            // 证书最后发布；若中断仅留下未认证候选，下次source预检拒绝缺少证书。
            await fs.writeFile(output+'.json',JSON.stringify(certificate,null,2)+'\n',{flag:'wx'});
        }
        console.log(JSON.stringify(certificate,null,2));passed=true;return certificate;
    }finally{
        process.removeListener('SIGINT',cancel);process.removeListener('SIGTERM',cancel);
        need(!child,'子进程未收齐');
        if(source)need(sourceBytes.equals(await fs.readFile(source.file))&&
            sourceCertificateBytes.equals(await fs.readFile(source.certificate_file)),'冻结源/证书改变');
        need(path.dirname(owned)===work,'临时目录越界');
        if(passed)await fs.rm(owned,{recursive:true});else console.error('失败现场保留：'+owned);
    }
}

if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
    const args=process.argv.slice(2),o={};
    const keys={'--exe':'exe','--work-dir':'workDir','--save-at':'saveAt','--save-month':'saveMonth',
        '--tail-frames':'tailFrames','--frame-limit':'frameLimit','--timeout-seconds':'timeoutSeconds',
        '--load-prefix':'loadPrefix','--snapshot-file':'snapshotFile','--producer-revision':'producerRevision'};
    for(let i=0;i<args.length;i+=2){need(keys[args[i]]&&i+1<args.length&&o[keys[args[i]]]===undefined,'参数非法');o[keys[args[i]]]=args[i+1];}
    need(o.exe&&o.workDir,'缺少exe/work-dir');await verifyNaturalApplication(o);
}
