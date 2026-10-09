// 二星策略的有限交接及三进程回放。来源仍包含未认证历史，不改签旧v1证书。
import fs from 'node:fs/promises';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
import {applicationReplayIdentity,applicationReplayProcesses,publishApplicationReplayPair} from './application_process_support.mjs';

const controller='application-active-progression-v2',oldController='application-active-progression-v1';
const qualification='active_second_star_management_tail',oldQualification='active_application_management_tail';
const researchRoot=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const budget=128*1024*1024;
const known={snapshot:'16a1b8c48993a96c536e4fc5868c7408355776c3b4631145047a2b4706c8062b',
    certificate:'8484c8cac5578c431dfd307903d7659b24296741eeace84cd875ab176c3332e3',
    terminal:'2ee7ad91c02e2d701fa3ac3b9578930b3c9b9b348c4099621e10958e74f0524b',
    origin_driver:'a76854cfd6a0ad2349648ac89da42b783a3972e7fb1980b1a0be564e0794f0d6',
    origin_files:'ece8dc44f2f2b910d2a9e8498e5f5ead021c676ac61418a7c59b90ebeff6028d',
    candidate:'e6452b6d5b7ab9d9124ffde656144f1f4780c0a19538eb5f1c07a74b32978218',
    dataset:'c3f9419d56e2db0c4a4fcf582f3b34fe5100a12b6acaa2b3bbf3ca0273faa690',
    world_schema:'7f33851d8b0430afbb6234595ab01d2190f29959515d216e6da64b1919dbf440',
    application_schema:'bd1940f2adef221c3da305403518082e34fd5eb2b1756e478240f564725edf0c'};
const hash=b=>createHash('sha256').update(b).digest('hex');
const need=(ok,why)=>{if(!ok)throw Error(why);};
const digest=n=>typeof n==='string'&&/^[0-9a-f]{64}$/.test(n);
const natural=n=>Number.isSafeInteger(n)&&n>=0;
const canonical=value=>JSON.stringify(value,(_key,item)=>item&&typeof item==='object'&&!Array.isArray(item)?
    Object.fromEntries(Object.keys(item).sort().map(key=>[key,item[key]])):item);
const same=(a,b)=>canonical(a)===canonical(b);
const within=(root,file)=>{const r=path.relative(root,file);return !!r&&!r.startsWith('..')&&!path.isAbsolute(r);};
function integer(value,fallback,min,max){
    const s=String(value??fallback),n=Number(s);
    need(/^\d+$/.test(s)&&Number.isSafeInteger(n)&&n>=min&&n<=max,'整数参数非法或超限');return n;
}
async function readBounded(file,maximum){
    const st=await fs.stat(file);need(st.isFile()&&st.size>0&&st.size<=maximum,'证据类型或预算非法');
    const b=await fs.readFile(file);need(b.length===st.size,'证据读取期间尺寸变化');return b;
}

// 共用容器预检之外补世界4/系统2、保留分区和规范Driver的身份；业务语义仍由C++严格核验。
function identity(bytes,expectedController){
    const result=applicationReplayIdentity(bytes,expectedController);
    for(const key of ['dataset','world_schema','application_schema'])need(result[key]===known[key],'固定来源字段身份不符 '+key);
    let at=20;
    for(let i=0;i<3;++i){const size=bytes.readUInt32LE(at);at+=4+size;}
    const count=bytes.readUInt32LE(at);at+=4;
    const sections=new Map();
    for(let i=0;i<count;++i){
        const id=bytes.readUInt32LE(at),version=bytes.readUInt32LE(at+4),required=bytes.readUInt32LE(at+8);
        const length=Number(bytes.readBigUInt64LE(at+12));at+=84;
        need(id>=1&&((id<=6&&version===1&&required===1)||(id>=1024&&required===0)),'未知必需段或保留分区');
        sections.set(id,bytes.subarray(at,at+length));at+=length;
    }
    const world=sections.get(4),system=sections.get(2),driver=sections.get(5);
    need(world.length>=84&&world.subarray(0,8).toString()==='AVRSAVE1'&&world.readUInt32LE(8)===1&&
        world.readUInt32LE(12)===4&&world.readUInt32LE(16)===2&&
        hash(world.subarray(0,-64))===world.subarray(-64).toString(),'内嵌世界4完整回放身份');
    need(system.length>=76&&system.subarray(0,8).toString()==='AVRSYS01'&&system.readUInt32LE(8)===2&&
        hash(system.subarray(0,-64))===system.subarray(-64).toString(),'内嵌系统2身份');
    need(driver.length>=8&&driver.length<=1024*1024&&driver.subarray(0,8).toString()===
        (expectedController===controller?'AVACTDR2':'AVACTDR1'),'策略规范Driver身份');
    return {...result,driver_digest:hash(driver)};
}

function history(value){
    need(value&&typeof value==='object'&&!Array.isArray(value)&&typeof value.file==='string'&&
        value.snapshot_sha256===known.candidate&&value.snapshot_bytes===17380140&&value.next_frame===20001&&
        value.producer_revision==='87023c3','未认证历史必须保留实际20000轮候选完整身份');
}
function handoff(value){
    need(value&&value.origin_metadata,'缺少完整旧Driver来源');const m=value.origin_metadata;
    need(m.controller===oldController&&m.producer_revision==='11818a9-income-gate'&&m.next_frame===34430&&m.next_command===881&&
        typeof m.driver==='string'&&/^(?:[0-9a-f]{2})+$/.test(m.driver)&&m.driver.length===2*635&&
        Buffer.from(m.driver,'hex').subarray(0,8).toString()==='AVACTDR1'&&m.driver_digest===known.origin_driver&&
        hash(Buffer.from(m.driver,'hex'))===m.driver_digest,
        '旧metadata/完整Driver摘要失配');
    need(value.before_digest===known.terminal&&value.after_digest===known.terminal&&
        value.origin_snapshot_sha256===known.snapshot&&value.origin_certificate_sha256===known.certificate&&
        value.uncertified_history_sha256===known.candidate&&value.files_before_digest===known.origin_files&&
        value.files_before_digest===value.files_after_digest,'交接不变证明或不可变来源失配');
}

// 只沿显式来源链，最多32层；逐个核hash，不把目录最新文件或可解析候选当已认证前缀。
export async function readSecondStarSource(options,trusted){
    need((options.handoffPrefix!==undefined)!==(options.loadPrefix!==undefined),'必须且只能指定handoff-prefix或load-prefix');
    const frozen=new Map(),visited=new Map();
    const local=async p=>{const file=await fs.realpath(path.resolve(p));need(within(trusted,file),'来源实际路径越界');return file;};
    const read=async(file,max)=>{
        const b=await readBounded(file,max),h=hash(b),previous=frozen.get(file);
        need(!previous||previous.sha256===h,'链遍历期间来源发生变化');
        frozen.set(file,{file,sha256:h,bytes:b.length,maximum:max});return b;
    };
    const candidate=async value=>{
        history(value);const file=await local(value.file),bytes=await read(file,budget),meta=identity(bytes,oldController);
        need(hash(bytes)===value.snapshot_sha256&&bytes.length===value.snapshot_bytes&&meta.next_frame===value.next_frame&&
            meta.producer===value.producer_revision,'候选真实文件与历史限制失配');
    };
    async function visit(file,chain=[]){
        need(!chain.includes(file)&&chain.length<32,'来源循环或超过32层');
        if(visited.has(file))return visited.get(file);
        const certFile=await local(file+'.json'),raw=await read(certFile,1024*1024),c=JSON.parse(raw.toString('utf8'));
        need(c.controller===controller||c.controller===oldController,'未知来源controller');
        const bytes=await read(file,budget),meta=identity(bytes,c.controller);
        need(c.qualification===(c.controller===controller?qualification:oldQualification)&&c.process_count===3&&c.seed===1&&c.speed===0&&
            c.snapshot_sha256===hash(bytes)&&c.snapshot_bytes===bytes.length&&c.producer_revision===meta.producer&&
            natural(c.capture_frame)&&c.capture_frame+1===meta.next_frame&&natural(c.capture_rank)&&c.capture_rank<=5&&
            natural(c.tail_frames)&&c.tail_frames>=1&&c.tail_frames<=1000&&c.stop_at===c.capture_frame+c.tail_frames&&
            digest(c.trace_sha256)&&natural(c.trace_bytes)&&c.trace_bytes>0&&same(c.section_bytes,meta.section_bytes),
            '来源证书、快照或捕获轮身份失配');
        for(const key of ['dataset','world_schema','application_schema'])need(c[key]===meta[key],'证书schema失配 '+key);
        const end=c.terminal;
        need(end&&end.controller===c.controller&&end.completed_frame===c.stop_at&&end.next_frame===c.stop_at+1&&
            end.trace_rows===c.tail_frames&&digest(end.digest),'来源终点/尾段边界失配');
        history(c.uncertified_history);await candidate(c.uncertified_history);
        const source=c.source_prefix;
        need(source&&typeof source.file==='string','有限来源链不能缺少前驱');
        if(c.controller===oldController&&source.source_status==='candidate'){
            need(source.qualification==='unverified_complete_round_candidate','候选资格失配');
            await candidate(source);
        }else{
            need(source.source_status==='certified'&&digest(source.certificate_sha256),'前驱必须为明确已认证来源');
            const parent=await local(source.file),parentCert=await local(source.certificate_file);
            need(parentCert===await local(parent+'.json')&&hash(await read(parentCert,1024*1024))===source.certificate_sha256,
                '前驱证书真实路径/hash失配');
            const prior=await visit(parent,[...chain,file]);
            need(source.snapshot_sha256===prior.certificate.snapshot_sha256&&source.capture_frame===prior.certificate.capture_frame&&
                source.next_frame===prior.meta.next_frame&&source.qualification===prior.certificate.qualification&&
                source.capture_frame<c.capture_frame&&same(c.uncertified_history,prior.certificate.uncertified_history),
                '前驱身份/轮数或未认证历史继承失配');
            if(c.controller===controller){
                need(c.tail_frames>=20&&typeof c.stage_complete==='boolean'&&c.stage_complete===end.stage_complete&&
                    c.stage_complete===(end.accepted_tasks>=1)&&natural(c.active_command_count)&&
                    natural(c.capture_accepted_tasks)&&natural(c.accepted_tasks_in_tail)&&
                    c.capture_accepted_tasks+c.accepted_tasks_in_tail===end.accepted_tasks&&
                    (c.accepted_tasks_in_tail===0||c.active_command_count>0),'二星尾段资格缺失');
                handoff(c.handoff);need(same(end.handoff,c.handoff),'二星终点丢失交接来源');
                if(prior.certificate.controller===controller)need(same(c.handoff,prior.certificate.handoff),'后继改变原交接证明');
                else need(prior.certificate_hash===known.certificate&&c.capture_frame===34429,'首份v2必须来自已核v1并零输入捕获');
            }
        }
        const result={file,certificate_file:certFile,certificate_hash:hash(raw),certificate:c,meta};visited.set(file,result);return result;
    }
    const file=await local(options.handoffPrefix??options.loadPrefix),source=await visit(file);
    if(options.handoffPrefix!==undefined){
        const c=source.certificate;
        need(c.controller===oldController&&source.certificate_hash===known.certificate&&c.snapshot_sha256===known.snapshot&&
            c.snapshot_bytes===31737023&&c.producer_revision==='11818a9-income-gate'&&c.capture_frame===34401&&
            c.stop_at===34429&&c.tail_frames===28&&c.terminal.terminal===true&&c.terminal.digest===known.terminal&&
            c.terminal.next_command===881&&c.terminal.progress.current_month_facility_income===400,
            '仅接受修正收入gate后的首星新月收入terminal证书');
    }else need(source.certificate.controller===controller,'load-prefix仅接受v2已认证来源');
    const unchanged=async()=>{for(const evidence of frozen.values()){
        const b=await readBounded(evidence.file,evidence.maximum);
        need(b.length===evidence.bytes&&hash(b)===evidence.sha256,'冻结来源或证书已改变：'+evidence.file);
    }};
    return {source,unchanged,evidence:[...frozen.values()].map(({maximum,...v})=>v)};
}

function summary(stdout){
    const prefix='application-second-star-summary ',lines=stdout.split(/\r?\n/).filter(s=>s.startsWith(prefix));
    need(lines.length===1,'缺少唯一二星应用终点');const result=JSON.parse(lines[0].slice(prefix.length));
    need(result.controller===controller&&['completed_frame','next_frame','next_command','rank','months','sound_count','random',
        'phase','trace_rows','accepted_tasks','departed_tasks','task_successes'].every(k=>natural(result[k]))&&
        ['date','resources','peaks'].every(k=>Array.isArray(result[k])&&result[k].length>0&&result[k].every(natural))&&
        digest(result.digest)&&digest(result.sound_hash)&&result.terminal===false&&
        typeof result.stage_complete==='boolean'&&result.stage_complete===(result.accepted_tasks>=1)&&
        result.departed_tasks<=result.accepted_tasks,'二星阶段终点/策略资格字段');
    handoff(result.handoff);return result;
}
function traceRows(bytes,capture,tail,nextCommand){
    const rows=bytes.toString('utf8').trimEnd().split('\n').map(s=>JSON.parse(s));need(rows.length===tail,'尾段行数');
    let active=0;
    for(const [index,row] of rows.entries()){
        need(row.frame===capture+index+1&&row.next_frame===row.frame+1&&natural(row.next_command)&&
            digest(row.digest)&&digest(row.system_digest)&&digest(row.driver_digest)&&
            typeof row.driver==='string'&&/^(?:[0-9a-f]{2})+$/.test(row.driver)&&row.driver.length<=2*1024*1024&&
            Buffer.from(row.driver,'hex').subarray(0,8).toString()==='AVACTDR2'&&
            hash(Buffer.from(row.driver,'hex'))===row.driver_digest,'完整v2逐轮Driver/摘要');
        need(Array.isArray(row.commands)&&row.commands.length>0&&row.commands.every(c=>Array.isArray(c)&&c.length===5&&
            c.every(Number.isSafeInteger)&&c[0]>=0&&c[0]<=5&&c[1]>=0),'v2独立命令枚举/五元组');
        need(!row.commands.some(c=>c[0]===0)||(row.commands.length===1&&row.commands[0].every(n=>n===0)),'wait必须独立全零');
        const committed=row.commands.filter(c=>c[0]!==0).length;nextCommand+=committed;active+=committed;
        need(row.next_command===nextCommand,'只有实际管理输入递增命令序号');
        need(Array.isArray(row.sounds)&&row.sounds.every(s=>Array.isArray(s)&&s.length===2&&s.every(Number.isSafeInteger)&&
            s[0]>=0&&s[0]<=2&&s[1]>=0&&s[1]<26),'typed音频次序/身份');
    }
    return {rows,active};
}

export async function verifySecondStarApplication(options){
    const trusted=await fs.realpath(path.join(researchRoot,'work')),work=await fs.realpath(path.resolve(options.workDir));
    need(within(trusted,work),'实际工作目录必须在research/work内');
    const loaded=await readSecondStarSource(options,trusted),source=loaded.source,isHandoff=options.handoffPrefix!==undefined;
    const saveAt=integer(options.saveAt,isHandoff?34429:source.meta.next_frame,34429,179999);
    const tail=integer(options.tailFrames,20,20,1000),limit=integer(options.frameLimit,saveAt+tail,34449,180000);
    const timeout=integer(options.timeoutSeconds,120,1,3600)*1000;
    need(saveAt+tail<=limit&&(!isHandoff||saveAt===34429)&&(isHandoff||saveAt>=source.meta.next_frame),
        '首次必须在34429零v2输入捕获；后继捕获须向前且处于显式轮预算');
    const producer=options.producerRevision??'unspecified';
    need(typeof producer==='string'&&Buffer.byteLength(producer)<=256&&!producer.includes('\0'),'producer文本契约');
    let output;
    if(options.snapshotFile){
        const selected=path.resolve(options.snapshotFile),parent=await fs.realpath(path.dirname(selected));
        need(within(trusted,parent),'发布父目录越界');output=path.join(parent,path.basename(selected));
        for(const file of [output,output+'.json'])try{await fs.lstat(file);throw Error('禁止覆盖已有快照/证书');}
        catch(e){if(e.code!=='ENOENT')throw e;}
    }
    const owned=await fs.mkdtemp(path.join(work,'application-second-star-'));
    const processes=applicationReplayProcesses(path.resolve(options.exe),owned,timeout);let passed=false;
    const run=args=>processes.run([controller,...args]);
    try{
        const dirs=[0,1,2].map(n=>path.join(owned,'process-'+n));for(const dir of dirs)await fs.mkdir(dir);
        const traces=dirs.map(dir=>path.join(dir,'tail.jsonl')),snapshot=path.join(dirs[0],'prefix.avra');
        const from=isHandoff?['--handoff-file',source.file,'--handoff-certificate-sha256',source.certificate_hash,
            '--handoff-snapshot-sha256',source.certificate.snapshot_sha256]:['--load-file',source.file];
        const reference=summary(await run(['--work-dir',dirs[0],'--trace-file',traces[0],'--stop-at',String(limit),
            '--save-file',snapshot,'--save-at',String(saveAt),'--tail-after-save',String(tail),'--producer-revision',producer,...from]));
        const bytes=await readBounded(snapshot,budget),meta=identity(bytes,controller),capture=meta.next_frame-1;
        need(meta.producer===producer&&capture===saveAt&&reference.capture_frame===capture&&reference.completed_frame===capture+tail&&
            reference.next_frame===capture+tail+1&&reference.trace_rows===tail,'v2捕获与尾段轮数');
        if(isHandoff)need(meta.next_command===1,'零v2输入交接捕获命令序号须为1');
        else need(same(reference.handoff,source.certificate.handoff),'后继恢复改变旧来源');
        const expected=await readBounded(traces[0],8*1024*1024),parsed=traceRows(expected,capture,tail,meta.next_command);
        const last=parsed.rows.at(-1);
        need(reference.next_command===last.next_command&&reference.digest===last.digest&&
            natural(reference.capture_rank)&&reference.capture_rank<=5&&natural(reference.capture_accepted_tasks)&&
            reference.capture_accepted_tasks<=reference.accepted_tasks,'v2终点与尾段不一致');
        const acceptedInTail=reference.accepted_tasks-reference.capture_accepted_tasks;
        need(acceptedInTail===0||(parsed.active>0&&parsed.rows.some(row=>row.commands.some(command=>command[0]===3))),
            '新增任务接受必须在完整尾段中出现实际任务动作');
        if(isHandoff)need(reference.capture_accepted_tasks===0,'零输入交接不能继承为已执行v2任务');
        // 仅排除捕获位置与耗时；其它全部summary字段均比较，新增诊断也不能悄悄漏验。
        const comparable=value=>Object.fromEntries(Object.entries(value).filter(([key])=>
            !['capture_frame','capture_rank','capture_accepted_tasks','capture_seconds','restore_seconds'].includes(key)));
        for(let n=1;n<3;++n){
            const restored=summary(await run(['--work-dir',dirs[n],'--trace-file',traces[n],'--stop-at',String(reference.completed_frame),
                '--load-file',snapshot,'--producer-revision',producer]));
            need(expected.equals(await readBounded(traces[n],8*1024*1024)),'v2三路完整命令/Driver/应用/系统/typed声音trace不同');
            need(same(comparable(restored),comparable(reference)),'v2三路完整终点不同');
            need(bytes.equals(await readBounded(snapshot,budget)),'恢复改写v2捕获文件');
        }
        const certificate={controller,qualification,producer_revision:meta.producer,seed:1,speed:0,
            dataset:meta.dataset,world_schema:meta.world_schema,application_schema:meta.application_schema,
            source_prefix:{file:source.file,certificate_file:source.certificate_file,source_status:'certified',
                snapshot_sha256:source.certificate.snapshot_sha256,certificate_sha256:source.certificate_hash,
                next_frame:source.meta.next_frame,capture_frame:source.certificate.capture_frame,qualification:source.certificate.qualification},
            uncertified_history:source.certificate.uncertified_history,source_evidence:loaded.evidence,handoff:reference.handoff,
            capture_frame:capture,capture_rank:reference.capture_rank,stop_at:reference.completed_frame,tail_frames:tail,process_count:3,
            snapshot_bytes:bytes.length,snapshot_sha256:hash(bytes),section_bytes:meta.section_bytes,
            trace_bytes:expected.length,trace_sha256:hash(expected),terminal:reference,process_wall_seconds:processes.seconds,
            stage_complete:reference.stage_complete,active_command_count:parsed.active,
            capture_accepted_tasks:reference.capture_accepted_tasks,accepted_tasks_in_tail:acceptedInTail,
            comparison:'三路逐轮完整命令、规范v2 Driver及其完整旧origin、应用/世界/目录、系统实际字节、typed声音一致',
            certification_boundary:'只认证所列v2短尾段；完整继承20000轮候选之前历史未认证限制',
            limitations:['stage_complete只表示本策略已收到至少一次实际任务接受回执，不表示二星已完成',
                'active_command_count仅计本尾段非wait输入；20轮等待不能单独证明主动任务操作',
                '未取得四住宅、二星晋级或活动30真正领取；不认证原APK/Steam窗口']};
        await loaded.unchanged();
        if(output)await publishApplicationReplayPair(output,bytes,certificate);
        console.log(JSON.stringify(certificate,null,2));passed=true;return certificate;
    }finally{
        processes.finish();await loaded.unchanged();need(path.dirname(owned)===work,'回收路径越界');
        if(passed)await fs.rm(owned,{recursive:true});else console.error('二星失败现场保留：'+owned);
    }
}

if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
    const args=process.argv.slice(2),options={};
    const keys={'--exe':'exe','--work-dir':'workDir','--handoff-prefix':'handoffPrefix','--load-prefix':'loadPrefix',
        '--save-at':'saveAt','--tail-frames':'tailFrames','--frame-limit':'frameLimit','--timeout-seconds':'timeoutSeconds',
        '--snapshot-file':'snapshotFile','--producer-revision':'producerRevision'};
    for(let i=0;i<args.length;i+=2){need(keys[args[i]]&&i+1<args.length&&options[keys[args[i]]]===undefined,'参数非法');options[keys[args[i]]]=args[i+1];}
    need(options.exe&&options.workDir,'缺少exe/work-dir');await verifySecondStarApplication(options);
}
