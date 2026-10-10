// 住宅策略的独立三进程认证。只接受固定v2交接源或本策略已认证前缀，不改签旧Driver。
import fs from 'node:fs/promises';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
import {applicationReplayProcesses,publishApplicationReplayPair} from './application_process_support.mjs';
import {progressionApplicationIdentity} from './application_progression_evidence.mjs';
import {readSecondStarSource} from './application_second_star_process.mjs';

const controller='application-active-residence-v1',oldController='application-active-progression-v2';
const qualification='active_residence_management_tail';
const researchRoot=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..'),budget=128*1024*1024;
const known={snapshot:'dc30d31a83c021d6af6ac66fbb31fbb40ea01e49a2679c6823f9567724a3f8f6',
    certificate:'4dcb2135d5ee4045b7a176df31b124af3fd04ab3121031a25469372ee27889ad',
    terminal:'62aaa3970f616e4af63c8cf4c12685b0e510aee9c70f7b84ed9553068063ba25',
    driver:'f293d9463eb014cb6a61f62a3f5fae2d285b50197dc7f77f138d3ea98358afc7',
    files:'ece8dc44f2f2b910d2a9e8498e5f5ead021c676ac61418a7c59b90ebeff6028d',
    history:'e6452b6d5b7ab9d9124ffde656144f1f4780c0a19538eb5f1c07a74b32978218'};
const receiptKeys=['gifts_committed','recruitments_created','homes_admitted','completed_houses','completed_business_facilities'];
const hash=b=>createHash('sha256').update(b).digest('hex');
const need=(ok,why)=>{if(!ok)throw Error(why);};
const natural=n=>Number.isSafeInteger(n)&&n>=0;
const digest=n=>typeof n==='string'&&/^[0-9a-f]{64}$/.test(n);
const canonical=value=>JSON.stringify(value,(_key,v)=>v&&typeof v==='object'&&!Array.isArray(v)?
    Object.fromEntries(Object.keys(v).sort().map(k=>[k,v[k]])):v);
const same=(a,b)=>canonical(a)===canonical(b);
const within=(root,file)=>{const p=path.relative(root,file);return !!p&&!p.startsWith('..')&&!path.isAbsolute(p);};
function integer(value,fallback,min,max){
    const s=String(value??fallback),n=Number(s);
    need(/^\d+$/.test(s)&&Number.isSafeInteger(n)&&n>=min&&n<=max,'整数参数非法或超限');return n;
}
async function readBounded(file,max){
    const st=await fs.stat(file);need(st.isFile()&&st.size>0&&st.size<=max,'来源类型或预算非法');
    const b=await fs.readFile(file);need(b.length===st.size,'读取期间来源尺寸变化');return b;
}
const identity=b=>progressionApplicationIdentity(b,controller,'AVRESDR1');
const receipts=value=>{
    need(value&&receiptKeys.every(k=>natural(value[k])),'住宅里程碑回执缺失或非法');
    return Object.fromEntries(receiptKeys.map(k=>[k,value[k]]));
};
function handoff(value,prior){
    need(value&&value.origin_metadata,'缺少完整v2交接来源');const m=value.origin_metadata;
    need(m.controller===oldController&&m.producer_revision==='2c612c9-handoff-audit'&&m.next_frame===34490&&m.next_command===4&&
        typeof m.driver==='string'&&m.driver.length===2*1572&&/^(?:[0-9a-f]{2})+$/.test(m.driver)&&
        m.driver_digest===known.driver&&hash(Buffer.from(m.driver,'hex'))===known.driver&&
        Buffer.from(m.driver,'hex').subarray(0,8).toString()==='AVACTDR2','v2终点完整规范Driver身份不符');
    need(value.before_digest===known.terminal&&value.after_digest===known.terminal&&
        value.origin_snapshot_sha256===known.snapshot&&value.origin_certificate_sha256===known.certificate&&
        value.uncertified_history_sha256===known.history&&value.files_before_digest===known.files&&value.files_after_digest===known.files&&
        same(value.prior_handoff,prior),'住宅交接不变证明或完整v1来源失配');
}
function milestone(value){
    receipts(value);const m=value.milestones;
    need(Array.isArray(value.homes)&&value.homes.length<=4&&value.homes.length===value.completed_houses&&
        value.homes.length<=value.homes_admitted&&value.homes.every(h=>h&&natural(h.human)&&natural(h.instance)&&h.instance>0&&
            natural(h.completed_frame)&&h.completed_frame>34489&&h.completed_frame<=value.completed_frame)&&
        new Set(value.homes.map(h=>h.human)).size===value.homes.length&&new Set(value.homes.map(h=>h.instance)).size===value.homes.length&&
        value.homes.every((h,i)=>i===0||h.completed_frame>value.homes[i-1].completed_frame)&&
        value.homes_admitted<=value.recruitments_created&&value.recruitments_created<=4,
        '已完工住宅回执必须完整、按实际完成轮有序且为独立人物/实例，不能放未完成施工项');
    need(m&&['first_home_frame','first_home_human','first_home_instance','completed_houses','completed_business_facilities']
        .every(k=>natural(m[k]))&&m.completed_houses===value.completed_houses&&
        m.completed_business_facilities===value.completed_business_facilities&&typeof value.stage_complete==='boolean'&&
        value.stage_complete===(m.first_home_frame>0)&&typeof value.terminal==='boolean','完成里程碑与Owner计数失配');
    if(m.first_home_frame===0)need(m.first_home_human===0&&m.first_home_instance===0&&value.homes.length===0,'未完成住宅不能伪留首次完成身份');
    else need(m.first_home_frame>34489&&m.first_home_frame<=value.completed_frame&&m.first_home_instance>0&&value.completed_houses>=1,
        '首次住宅须为真实交接后完工轮与非零实例，不能以created代替');
    if(m.first_home_frame>0)need(value.homes.length>0&&value.homes[0].human===m.first_home_human&&
        value.homes[0].instance===m.first_home_instance&&value.homes[0].completed_frame===m.first_home_frame,'首次住宅里程碑与实际完成回执失配');
    need(!value.terminal||(value.completed_houses>=4&&value.completed_business_facilities>=10&&value.stage_complete),
        '路线终点不能由首个入住或未完工住宅代替');
}

// 本策略链只保存hash/长度，不长期持有各份大快照Buffer；根v2链复用原严格审计器。
export async function readResidenceSource(options,trusted){
    need((options.handoffPrefix!==undefined)!==(options.loadPrefix!==undefined),'必须且只能指定handoff-prefix或load-prefix');
    const frozen=new Map(),visited=new Map();let old;
    const local=async p=>{const file=await fs.realpath(path.resolve(p));need(within(trusted,file),'来源实际路径越界');return file;};
    const read=async(file,max)=>{
        const b=await readBounded(file,max),h=hash(b),previous=frozen.get(file);
        need(!previous||previous.sha256===h,'来源遍历期间文件改变');
        frozen.set(file,{file,sha256:h,bytes:b.length,maximum:max});return b;
    };
    async function fixedV2(file,depth){
        need(!old,'住宅来源链重复到达v2根');
        old=await readSecondStarSource({loadPrefix:file},trusted);const s=old.source,c=s.certificate;
        need(depth+(old.evidence.length-1)/2<=32,'含旧来源链超过32层');
        need(s.certificate_hash===known.certificate&&c.snapshot_sha256===known.snapshot&&c.snapshot_bytes===32214863&&
            c.producer_revision==='2c612c9-handoff-audit'&&c.capture_frame===34469&&c.stop_at===34489&&c.tail_frames===20&&
            c.terminal.digest===known.terminal&&c.terminal.next_frame===34490&&c.terminal.next_command===4&&
            c.terminal.accepted_task_identity===7&&c.terminal.accepted_tasks===1&&c.terminal.departed_tasks===0&&
            c.terminal.terminal===false&&c.terminal.stage_complete===true,'住宅只接受已核任务7募集v2源及原20轮尾段');
        return s;
    }
    async function visit(file,chain=[]){
        need(!chain.includes(file)&&chain.length<32,'住宅来源循环或超限');
        if(visited.has(file))return visited.get(file);
        const certFile=await local(file+'.json'),raw=await read(certFile,1024*1024),c=JSON.parse(raw.toString('utf8'));
        if(c.controller===oldController)return fixedV2(file,chain.length);
        need(c.controller===controller&&c.qualification===qualification&&c.process_count===3&&c.seed===1&&c.speed===0,'住宅证书资格非法');
        const bytes=await read(file,budget),meta=identity(bytes);
        need(c.snapshot_sha256===hash(bytes)&&c.snapshot_bytes===bytes.length&&c.producer_revision===meta.producer&&
            natural(c.capture_frame)&&c.capture_frame+1===meta.next_frame&&natural(c.capture_rank)&&c.capture_rank<=5&&
            natural(c.tail_frames)&&c.tail_frames>=20&&c.tail_frames<=1000&&c.stop_at===c.capture_frame+c.tail_frames&&
            digest(c.trace_sha256)&&natural(c.trace_bytes)&&c.trace_bytes>0&&same(c.section_bytes,meta.section_bytes),
            '住宅来源快照/捕获/摘要失配');
        for(const key of ['dataset','world_schema','application_schema'])need(c[key]===meta[key],'住宅schema失配 '+key);
        const source=c.source_prefix;
        need(source&&source.source_status==='certified'&&typeof source.file==='string'&&digest(source.certificate_sha256),
            '住宅后继必须明确引用已认证前驱');
        const parent=await local(source.file),parentCert=await local(source.certificate_file);
        need(parentCert===await local(parent+'.json')&&hash(await read(parentCert,1024*1024))===source.certificate_sha256,'前驱证书hash/路径失配');
        const prior=await visit(parent,[...chain,file]);
        need(source.snapshot_sha256===prior.certificate.snapshot_sha256&&source.capture_frame===prior.certificate.capture_frame&&
            source.next_frame===prior.meta.next_frame&&source.qualification===prior.certificate.qualification&&
            source.capture_frame<c.capture_frame&&same(c.uncertified_history,prior.certificate.uncertified_history),'住宅来源链/历史限制失配');
        handoff(c.handoff,old.source.certificate.handoff);
        if(prior.certificate.controller===controller)need(same(c.handoff,prior.certificate.handoff),'后继改变住宅旧origin');
        else need(c.capture_frame===34489&&meta.next_command===1,'首份住宅快照必须零新输入交接');
        const end=c.terminal;
        need(end&&end.controller===controller&&end.completed_frame===c.stop_at&&end.next_frame===c.stop_at+1&&
            end.trace_rows===c.tail_frames&&digest(end.digest)&&same(end.handoff,c.handoff),'住宅终点/来源身份失配');
        milestone(end);need(c.stage_complete===end.stage_complete&&same(c.milestones,end.milestones)&&natural(c.tail_active_command_count),
            '住宅证书里程碑缺失或改写');
        const capture=receipts(c.capture_receipts),delta=receipts(c.tail_receipts);
        for(const k of receiptKeys)need(capture[k]+delta[k]===end[k],'住宅回执捕获/尾段累计失配 '+k);
        need(same(capture,receipts(end.capture_receipts))&&end.capture_frame===c.capture_frame&&
            natural(end.next_command)&&end.next_command>=meta.next_command&&
            c.tail_active_command_count===end.next_command-meta.next_command&&
            natural(end.active_command_count)&&end.active_command_count===end.next_command-1&&
            capture.completed_houses===end.homes.filter(h=>h.completed_frame<=c.capture_frame).length,
            '住宅捕获回执、实际完成轮或尾段/累计命令域失配');
        const result={file,certificate_file:certFile,certificate_hash:hash(raw),certificate:c,meta};visited.set(file,result);return result;
    }
    const file=await local(options.handoffPrefix??options.loadPrefix);
    const source=options.handoffPrefix!==undefined?await fixedV2(file,0):await visit(file);
    need(options.handoffPrefix!==undefined||source.certificate.controller===controller,'load-prefix仅接受住宅策略认证文件');
    const unchanged=async()=>{
        await old.unchanged();
        for(const evidence of frozen.values()){
            const b=await readBounded(evidence.file,evidence.maximum);
            need(b.length===evidence.bytes&&hash(b)===evidence.sha256,'住宅来源或证书已改变：'+evidence.file);
        }
    };
    const evidence=new Map(old.evidence.map(e=>[e.file,e]));
    for(const {maximum,...e} of frozen.values())evidence.set(e.file,e);
    return {source,unchanged,evidence:[...evidence.values()],priorHandoff:old.source.certificate.handoff};
}

function summary(stdout,prior){
    const prefix='application-residence-summary ',lines=stdout.split(/\r?\n/).filter(s=>s.startsWith(prefix));
    need(lines.length===1,'缺少唯一住宅应用终点');const r=JSON.parse(lines[0].slice(prefix.length));
    need(r.controller===controller&&['completed_frame','next_frame','next_command','rank','months','sound_count','random','phase','trace_rows']
        .every(k=>natural(r[k]))&&['date','resources','peaks'].every(k=>Array.isArray(r[k])&&r[k].length>0&&r[k].every(natural))&&
        digest(r.digest)&&digest(r.sound_hash),'住宅summary字段非法');
    need(typeof r.entry_gift_pending==='boolean'&&r.driver_checks===13+(r.entry_gift_pending?12:0),
        '住宅入口检查数必须与待消费父64答案一致');
    milestone(r);handoff(r.handoff,prior);return r;
}
function traceRows(bytes,capture,tail,nextCommand){
    const rows=bytes.toString('utf8').trimEnd().split('\n').map(line=>JSON.parse(line));need(rows.length===tail,'住宅尾段行数');
    let active=0;
    for(const [i,row] of rows.entries()){
        need(row.frame===capture+i+1&&row.next_frame===row.frame+1&&natural(row.next_command)&&digest(row.digest)&&
            digest(row.system_digest)&&digest(row.driver_digest)&&typeof row.driver==='string'&&
            /^(?:[0-9a-f]{2})+$/.test(row.driver)&&row.driver.length<=2*1024*1024&&
            Buffer.from(row.driver,'hex').subarray(0,8).toString()==='AVRESDR1'&&hash(Buffer.from(row.driver,'hex'))===row.driver_digest,
            '住宅完整逐轮Driver/应用摘要不符');
        need(Array.isArray(row.commands)&&row.commands.length>0&&row.commands.every(c=>Array.isArray(c)&&c.length===5&&
            c.every(Number.isSafeInteger)&&c[0]>=0&&c[0]<=13&&c[1]>=0),'住宅独立命令五元组/枚举非法');
        need(!row.commands.some(c=>c[0]===0)||(row.commands.length===1&&row.commands[0].every(n=>n===0)),'wait必须独立全零');
        const committed=row.commands.filter(c=>c[0]!==0).length;active+=committed;nextCommand+=committed;
        need(row.next_command===nextCommand,'wait或自动消费不能递增命令序号');
        need(Array.isArray(row.sounds)&&row.sounds.every(s=>Array.isArray(s)&&s.length===2&&s.every(Number.isSafeInteger)&&
            s[0]>=0&&s[0]<=2&&s[1]>=0&&s[1]<26),'typed声音ID/次序非法');
    }
    return {rows,active};
}

export async function verifyResidenceApplication(options){
    const trusted=await fs.realpath(path.join(researchRoot,'build')),work=await fs.realpath(path.resolve(options.workDir));
    need(within(trusted,work),'工作目录必须在产品build内');
    const loaded=await readResidenceSource(options,trusted),source=loaded.source,initial=options.handoffPrefix!==undefined;
    const saveAt=integer(options.saveAt,initial?34489:source.meta.next_frame,34489,179999);
    const tail=integer(options.tailFrames,20,20,1000),limit=integer(options.frameLimit,saveAt+tail,34509,180000);
    const timeout=integer(options.timeoutSeconds,120,1,3600)*1000;
    need(saveAt+tail<=limit&&(!initial||saveAt===34489)&&(initial||saveAt>=source.meta.next_frame),'交接捕获/后继轮预算非法');
    const producer=options.producerRevision??'unspecified';
    need(typeof producer==='string'&&Buffer.byteLength(producer)<=256&&!producer.includes('\0'),'producer文本契约');
    let output;
    if(options.snapshotFile){
        const selected=path.resolve(options.snapshotFile),parent=await fs.realpath(path.dirname(selected));
        need(within(trusted,parent),'发布父目录越界');output=path.join(parent,path.basename(selected));
        for(const f of [output,output+'.json'])try{await fs.lstat(f);throw Error('禁止覆盖已有快照/证书');}
        catch(e){if(e.code!=='ENOENT')throw e;}
    }
    const owned=await fs.mkdtemp(path.join(work,'application-residence-'));
    const processes=applicationReplayProcesses(path.resolve(options.exe),owned,timeout);let passed=false;
    const run=args=>processes.run([controller,...args]);
    try{
        const dirs=[0,1,2].map(n=>path.join(owned,'process-'+n));for(const d of dirs)await fs.mkdir(d);
        const traces=dirs.map(d=>path.join(d,'tail.jsonl')),snapshot=path.join(dirs[0],'prefix.avra');
        const from=initial?['--handoff-file',source.file,'--handoff-certificate-sha256',source.certificate_hash,
            '--handoff-snapshot-sha256',source.certificate.snapshot_sha256]:['--load-file',source.file];
        const reference=summary(await run(['--work-dir',dirs[0],'--trace-file',traces[0],'--stop-at',String(limit),
            '--save-file',snapshot,'--save-at',String(saveAt),'--tail-after-save',String(tail),'--producer-revision',producer,...from]),loaded.priorHandoff);
        const bytes=await readBounded(snapshot,budget),meta=identity(bytes),capture=meta.next_frame-1;
        need(meta.producer===producer&&capture===saveAt&&reference.capture_frame===capture&&reference.completed_frame===capture+tail&&
            reference.next_frame===capture+tail+1&&reference.trace_rows===tail&&natural(reference.capture_rank)&&reference.capture_rank<=5,
            '住宅捕获及尾段边界不符');
        const captured=receipts(reference.capture_receipts),ending=receipts(reference),deltas={};
        for(const k of receiptKeys){need(ending[k]>=captured[k],'住宅回执倒退 '+k);deltas[k]=ending[k]-captured[k];}
        need(captured.completed_houses===reference.homes.filter(h=>h.completed_frame<=capture).length,
            '捕获前已完成住宅与真实完成帧失配');
        if(initial)need(meta.next_command===1&&captured.gifts_committed===0&&captured.recruitments_created===0&&captured.homes_admitted===0,
            '初次交接必须零住宅输入/付款回执，不能继承为已完成新操作');
        else need(same(reference.handoff,source.certificate.handoff),'后继改变住宅origin');
        const expected=await readBounded(traces[0],8*1024*1024),parsed=traceRows(expected,capture,tail,meta.next_command),last=parsed.rows.at(-1);
        need(reference.next_command===last.next_command&&reference.digest===last.digest&&
            natural(reference.active_command_count)&&reference.active_command_count===reference.next_command-1,
            '住宅终点与逐轮trace或累计命令域不符');
        // 完工及父64消费属于update。尾段可从已付费/已回答案快照开始，不能要求其必有新输入。
        const comparable=r=>Object.fromEntries(Object.entries(r).filter(([k])=>
            !['capture_frame','capture_rank','capture_receipts','capture_gift_pending','capture_seconds','restore_seconds',
                'entry_gift_pending','driver_checks'].includes(k)));
        for(let n=1;n<3;++n){
            const restored=summary(await run(['--work-dir',dirs[n],'--trace-file',traces[n],'--stop-at',String(reference.completed_frame),
                '--load-file',snapshot,'--producer-revision',producer]),loaded.priorHandoff);
            need(typeof reference.capture_gift_pending==='boolean'&&
                restored.entry_gift_pending===reference.capture_gift_pending,
                '住宅恢复入口父64答案状态与真实捕获不符');
            need(expected.equals(await readBounded(traces[n],8*1024*1024)),'住宅三路命令/Driver/应用/目录/系统/声音trace不同');
            need(same(comparable(restored),comparable(reference)),'住宅三路完整summary不同');
            need(bytes.equals(await readBounded(snapshot,budget)),'住宅恢复改写源快照');
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
            stage_complete:reference.stage_complete,milestones:reference.milestones,tail_active_command_count:parsed.active,
            capture_receipts:captured,tail_receipts:deltas,
            comparison:'完整住宅命令、规范Driver及两层旧origin、应用/世界/目录、系统实际字节和typed声音逐轮三路相同',
            certification_boundary:'只认证本捕获短尾段，沿完整v2/v1链保留20000轮候选之前历史未认证限制',
            limitations:['赠礼仅父64实际消费计提交；募集创建、入住替换与住宅完工分别记回执',
                'stage_complete仅表示第一住宅完整生命周期；terminal才表示四完工住宅及十可经营设施',
                '未由本策略认证二星晋级、任务成功12或活动30领取；不认证原APK/Steam窗口']};
        await loaded.unchanged();if(output)await publishApplicationReplayPair(output,bytes,certificate);
        console.log(JSON.stringify(certificate,null,2));passed=true;return certificate;
    }finally{
        processes.finish();await loaded.unchanged();need(path.dirname(owned)===work,'回收路径越界');
        if(passed)await fs.rm(owned,{recursive:true});else console.error('住宅失败现场保留：'+owned);
    }
}

if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
    const args=process.argv.slice(2),options={};
    const keys={'--exe':'exe','--work-dir':'workDir','--handoff-prefix':'handoffPrefix','--load-prefix':'loadPrefix','--save-at':'saveAt',
        '--tail-frames':'tailFrames','--frame-limit':'frameLimit','--timeout-seconds':'timeoutSeconds','--snapshot-file':'snapshotFile',
        '--producer-revision':'producerRevision'};
    for(let i=0;i<args.length;i+=2){need(keys[args[i]]&&i+1<args.length&&options[keys[args[i]]]===undefined,'参数非法');options[keys[args[i]]]=args[i+1];}
    need(options.exe&&options.workDir,'缺少exe/work-dir');await verifyResidenceApplication(options);
}
