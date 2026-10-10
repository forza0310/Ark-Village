// 主动经营应用的独立三进程认证；禁止接纳被动自然Driver或旧世界证书。
// 全部管理输入、规范Driver、完整应用/世界/目录身份和typed声音按原序比较。
import fs from 'node:fs/promises';
import path from 'node:path';
import {applicationReplayIdentity,applicationReplayProcesses,publishApplicationReplayPair} from './application_process_support.mjs';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';

const controller='application-active-progression-v1';
const qualification='active_application_management_tail';
const researchRoot=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const hash=b=>createHash('sha256').update(b).digest('hex');
const need=(value,message)=>{if(!value)throw Error(message);};
const digest=value=>typeof value==='string'&&/^[0-9a-f]{64}$/.test(value);
const within=(root,file)=>{const relative=path.relative(root,file);
    return !!relative&&!relative.startsWith('..')&&!path.isAbsolute(relative);};
const integer=(value,fallback,min,max)=>{
    const text=String(value??fallback);need(/^\d+$/.test(text),'整数参数非法');
    const n=Number(text);need(Number.isSafeInteger(n)&&n>=min&&n<=max,'整数参数越界');return n;
};
const natural=n=>Number.isSafeInteger(n)&&n>=0;
async function readBounded(file,maximum){
    const stat=await fs.stat(file);need(stat.isFile()&&stat.size>0&&stat.size<=maximum,'观察文件预算或类型非法');
    const bytes=await fs.readFile(file);need(bytes.length===stat.size,'观察文件读取期间尺寸变化');return bytes;
}

// 仅预检容器身份/摘要；完整世界4/应用7及Driver语义仍由C++权威恢复器验证。
const identity=bytes=>applicationReplayIdentity(bytes,controller);
function traceRows(bytes,capture,tail){
    const rows=bytes.toString('utf8').trimEnd().split('\n').map(line=>JSON.parse(line));
    need(rows.length===tail,'主动尾段行数');
    let nextCommand;
    for(const [index,row] of rows.entries()){
        need(row.frame===capture+index+1&&row.next_frame===row.frame+1&&natural(row.next_command)&&
             digest(row.digest)&&digest(row.system_digest)&&digest(row.driver_digest)&&
             typeof row.driver==='string'&&/^(?:[0-9a-f]{2})+$/.test(row.driver)&&row.driver.length<=2*1024*1024&&
             hash(Buffer.from(row.driver,'hex'))===row.driver_digest,'主动逐轮状态与规范Driver摘要');
        need(Array.isArray(row.commands)&&row.commands.length>0&&row.commands.every(command=>
             Array.isArray(command)&&command.length===5&&command.every(Number.isSafeInteger)&&
             command[0]>=0&&command[0]<=14&&command[1]>=0),'实际管理输入必须完整保留已登记枚举及五元组');
        const waiting=row.commands.some(command=>command[0]===0);
        need(!waiting||(row.commands.length===1&&row.commands[0].every(value=>value===0)),
             'wait必须是独立全零五元组，不能混入已提交输入');
        const committed=row.commands.filter(command=>command[0]!==0).length;
        if(nextCommand!==undefined)need(row.next_command===nextCommand+committed,'命令序号只递增实际已提交输入数量');
        nextCommand=row.next_command;
        need(Array.isArray(row.sounds)&&row.sounds.every(sound=>Array.isArray(sound)&&sound.length===2&&
             sound.every(Number.isSafeInteger)&&sound[0]>=0&&sound[0]<=2&&sound[1]>=0&&sound[1]<26),
             'typed声音操作/ID/原序');
    }
    return rows;
}

// 源预检不恢复或认证世界；三进程均继续使用同一C++完整应用/Driver/引用验证。
export async function readActiveApplicationSource(options,trusted,saveAt){
    need(!(options.loadPrefix!==undefined&&options.loadCandidate!==undefined),'load-prefix与load-candidate互斥');
    let source=null,sourceBytes,sourceCertificateBytes,uncertifiedHistory=null;
    if(options.loadPrefix!==undefined){
        const file=await fs.realpath(path.resolve(options.loadPrefix));need(within(trusted,file),'源前缀越界');
        sourceBytes=await readBounded(file,128*1024*1024);const meta=identity(sourceBytes);
        const certificateFile=await fs.realpath(file+'.json');need(within(trusted,certificateFile),'源证书实际路径越界');
        sourceCertificateBytes=await readBounded(certificateFile,1024*1024);const cert=JSON.parse(sourceCertificateBytes);
        need(cert.controller===controller&&cert.qualification===qualification&&cert.process_count===3&&
             cert.producer_revision===meta.producer&&
             cert.snapshot_sha256===hash(sourceBytes)&&cert.snapshot_bytes===sourceBytes.length&&
             natural(cert.capture_frame)&&cert.capture_frame+1===meta.next_frame&&
             natural(cert.capture_rank)&&cert.capture_rank<=5&&natural(cert.tail_frames)&&cert.tail_frames>0&&
             cert.tail_frames<=1000&&cert.stop_at===cert.capture_frame+cert.tail_frames&&
             digest(cert.trace_sha256)&&natural(cert.trace_bytes)&&cert.trace_bytes>0,'主动证书与实际前缀不符');
        for(const key of ['dataset','world_schema','application_schema'])need(cert[key]===meta[key],'源身份不符 '+key);
        need(saveAt>=meta.next_frame,'捕获必须晚于源前缀');
        source={file,certificate_file:certificateFile,snapshot_sha256:hash(sourceBytes),
            certificate_sha256:hash(sourceCertificateBytes),next_frame:meta.next_frame,
            capture_frame:cert.capture_frame,rank:cert.capture_rank,qualification,source_status:'certified'};
        uncertifiedHistory=cert.uncertified_history??null;
    }
    if(options.loadCandidate!==undefined){
        const file=await fs.realpath(path.resolve(options.loadCandidate));need(within(trusted,file),'候选实际路径必须位于research/work内');
        sourceBytes=await readBounded(file,128*1024*1024);const meta=identity(sourceBytes);
        need(saveAt>=meta.next_frame,'新捕获必须晚于候选');
        source={file,source_status:'candidate',qualification:'unverified_complete_round_candidate',
            snapshot_sha256:hash(sourceBytes),snapshot_bytes:sourceBytes.length,
            next_frame:meta.next_frame,capture_frame:meta.next_frame-1,producer_revision:meta.producer,
            dataset:meta.dataset,world_schema:meta.world_schema,application_schema:meta.application_schema};
        uncertifiedHistory={file,snapshot_sha256:source.snapshot_sha256,snapshot_bytes:source.snapshot_bytes,
            next_frame:meta.next_frame,producer_revision:meta.producer};
    }
    if(uncertifiedHistory!==null)need(typeof uncertifiedHistory==='object'&&!Array.isArray(uncertifiedHistory)&&
        typeof uncertifiedHistory.file==='string'&&digest(uncertifiedHistory.snapshot_sha256)&&
        natural(uncertifiedHistory.snapshot_bytes)&&uncertifiedHistory.snapshot_bytes>0&&
        uncertifiedHistory.snapshot_bytes<=128*1024*1024&&natural(uncertifiedHistory.next_frame)&&
        uncertifiedHistory.next_frame>1&&typeof uncertifiedHistory.producer_revision==='string',
        '候选未认证历史边界非法');
    return {source,sourceBytes,sourceCertificateBytes,uncertifiedHistory};
}

export async function verifyActiveApplication(options){
    const exe=path.resolve(options.exe),trusted=await fs.realpath(path.join(researchRoot,'build'));
    const work=await fs.realpath(path.resolve(options.workDir));need(within(trusted,work),'实际工作目录必须位于research/work内');
    const saveAt=integer(options.saveAt,420,1,999999),tail=integer(options.tailFrames,20,1,1000);
    const limit=integer(options.frameLimit,2000,1,1000000),timeout=integer(options.timeoutSeconds,120,1,3600)*1000;
    const producer=options.producerRevision??'unspecified';
    need(typeof producer==='string'&&Buffer.byteLength(producer)<=256&&!producer.includes('\0'),
         '来源版本必须满足实际metadata文本契约');
    need(saveAt+tail<=limit,'捕获加尾段必须处于显式轮数预算内');
    let output;
    if(options.snapshotFile){
        const selected=path.resolve(options.snapshotFile),parent=await fs.realpath(path.dirname(selected));
        need(within(trusted,parent),'证书输出父目录越界');output=path.join(parent,path.basename(selected));
        for(const file of [output,output+'.json']){
            try{await fs.lstat(file);throw Error('快照/证书已存在，禁止覆盖');}catch(e){if(e.code!=='ENOENT')throw e;}
        }
    }
    const {source,sourceBytes,sourceCertificateBytes,uncertifiedHistory}=
        await readActiveApplicationSource(options,trusted,saveAt);
    const sourceUnchanged=async()=>{
        if(source)need(sourceBytes.equals(await readBounded(source.file,128*1024*1024))&&
            (!sourceCertificateBytes||sourceCertificateBytes.equals(await readBounded(source.certificate_file,1024*1024))),
            '冻结主动源或证书发生变化');
    };
    const owned=await fs.mkdtemp(path.join(work,'application-active-'));
    let passed=false;
    const processes=applicationReplayProcesses(exe,owned,timeout),seconds=processes.seconds;
    const run=args=>processes.run([controller,...args]);
    const summary=stdout=>{
        const prefix='application-active-summary ',lines=stdout.split(/\r?\n/).filter(line=>line.startsWith(prefix));
        need(lines.length===1,'缺少唯一主动应用终点');const result=JSON.parse(lines[0].slice(prefix.length));
        need(result.controller===controller&&
             ['completed_frame','next_frame','next_command','rank','months','sound_count','random','phase','trace_rows',
                 'accepted_tasks','departed_tasks','task_successes','completed_activities','upgrades']
                 .every(key=>natural(result[key]))&&
             ['date','resources','peaks'].every(key=>Array.isArray(result[key])&&result[key].length>0&&
                 result[key].every(natural))&&digest(result.digest)&&digest(result.sound_hash)&&
             typeof result.terminal==='boolean'&&result.phase===(result.terminal?1:0)&&
             result.departed_tasks<=result.accepted_tasks,
             '终点Driver身份/完整字段');
        need(result.progress&&typeof result.progress==='object'&&!Array.isArray(result.progress)&&
             ['cash','popularity','maximum_income','village_points'].every(key=>Number.isSafeInteger(result.progress[key]))&&
             ['events_held','quarter_counter','task_successes','facilities_kind3_9','houses_kind12','current_month_facility_income']
                 .every(key=>natural(result.progress[key]))&&result.progress.task_successes===result.task_successes,
             '终点经营诊断字段/任务统计不一致');
        need(!result.terminal||result.progress.current_month_facility_income>0,
             '完整主动终点必须证明当前新月份实际设施收入，不能只看活动后累计增加');
        return result;
    };
    try{
        const dirs=[0,1,2].map(n=>path.join(owned,'process-'+n));for(const dir of dirs)await fs.mkdir(dir);
        const traces=dirs.map(dir=>path.join(dir,'tail.jsonl')),snapshot=path.join(dirs[0],'prefix.avra');
        const reference=summary(await run(['--work-dir',dirs[0],'--trace-file',traces[0],'--stop-at',String(limit),
            '--save-file',snapshot,'--save-at',String(saveAt),'--tail-after-save',String(tail),
            '--producer-revision',producer,
            ...(source?['--load-file',source.file]:[])]));
        const bytes=await readBounded(snapshot,128*1024*1024),meta=identity(bytes),capture=meta.next_frame-1;
        need(meta.producer===producer&&capture===saveAt&&reference.capture_frame===capture&&reference.completed_frame===capture+tail&&
             reference.next_frame===capture+tail+1&&reference.trace_rows===tail,'主动捕获/尾段轮数');
        const expected=await readBounded(traces[0],8*1024*1024),rows=traceRows(expected,capture,tail),last=rows.at(-1);
        need(rows[0].next_command===meta.next_command+rows[0].commands.filter(command=>command[0]!==0).length&&
             reference.next_command===last.next_command&&reference.digest===last.digest&&
             digest(reference.sound_hash)&&natural(reference.sound_count)&&natural(reference.rank)&&reference.rank<=5&&
             natural(reference.capture_rank)&&reference.capture_rank<=5,'终点与捕获Driver/尾段不一致');
        for(let n=1;n<3;++n){
            const restored=summary(await run(['--work-dir',dirs[n],'--trace-file',traces[n],
                '--stop-at',String(reference.completed_frame),'--load-file',snapshot,'--producer-revision',producer]));
            need(expected.equals(await readBounded(traces[n],8*1024*1024)),'三路实际命令/Driver/世界/目录/声音trace不一致');
            for(const key of ['controller','completed_frame','next_frame','next_command','rank','months','date','digest',
                'sound_count','sound_hash','random','resources','peaks','phase','trace_rows',
                'accepted_tasks','departed_tasks','task_successes','completed_activities','upgrades','terminal','progress'])
                need(JSON.stringify(restored[key])===JSON.stringify(reference[key]),'完整终点不同 '+key);
            need(bytes.equals(await readBounded(snapshot,128*1024*1024)),'恢复改写源快照');
        }
        const certificate={controller,qualification,producer_revision:meta.producer,seed:1,speed:0,
            dataset:meta.dataset,world_schema:meta.world_schema,application_schema:meta.application_schema,
            source_prefix:source,uncertified_history:uncertifiedHistory,
            capture_frame:capture,capture_rank:reference.capture_rank,stop_at:reference.completed_frame,
            tail_frames:tail,process_count:3,snapshot_bytes:bytes.length,snapshot_sha256:hash(bytes),section_bytes:meta.section_bytes,
            trace_bytes:expected.length,trace_sha256:hash(expected),terminal:reference,process_wall_seconds:seconds,
            comparison:'完整实际命令五元组、规范Driver字节、应用含世界/引用身份、系统实际字节、typed声音逐轮三路相同',
            certification_boundary:uncertifiedHistory?'候选来源的先前历史未认证；仅本轮新捕获后的尾段完成三进程认证':
                source?'已认证主动前缀恢复后继续；本轮认证新捕获尾段':'真实新局主动经营reference及指定捕获尾段',
            limitations:['短尾段证书不代表自然首星或通关已经完成','不接受被动自然Driver或旧语义证书','不认证原APK/Steam窗口及自动绘制频率']};
        await sourceUnchanged(); // 发布前核源身份，不能先发证书再发现reference输入已变化。
        if(output)await publishApplicationReplayPair(output,bytes,certificate);
        console.log(JSON.stringify(certificate,null,2));passed=true;return certificate;
    }finally{
        processes.finish();
        await sourceUnchanged();
        need(path.dirname(owned)===work,'临时目录越界');
        if(passed)await fs.rm(owned,{recursive:true});else console.error('主动失败现场保留：'+owned);
    }
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
    const args=process.argv.slice(2),options={};
    const keys={'--exe':'exe','--work-dir':'workDir','--save-at':'saveAt','--tail-frames':'tailFrames',
        '--frame-limit':'frameLimit','--timeout-seconds':'timeoutSeconds','--load-prefix':'loadPrefix','--load-candidate':'loadCandidate',
        '--snapshot-file':'snapshotFile','--producer-revision':'producerRevision'};
    for(let n=0;n<args.length;n+=2){need(keys[args[n]]&&n+1<args.length&&options[keys[args[n]]]===undefined,'参数非法');
        options[keys[args[n]]]=args[n+1];}
    need(options.exe&&options.workDir,'缺少exe/work-dir');await verifyActiveApplication(options);
}
