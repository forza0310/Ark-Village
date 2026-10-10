// 固定17命令三进程认证，复用应用测试CLI；不生成业务值、不写原游戏档。
import fs from 'node:fs/promises';
import path from 'node:path';
import {spawn} from 'node:child_process';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
const hash=b=>createHash('sha256').update(b).digest('hex');
const need=(v,m)=>{if(!v)throw Error(m);};
const within=(p,root)=>{const r=path.relative(root,p);return !!r&&!r.startsWith('..')&&!path.isAbsolute(r);};
export async function verifyTitleMenuReplay({exe,workDir,certificateFile}) {
    const trusted=await fs.realpath(path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../../build'));
    const work=await fs.realpath(workDir);
    need(within(work,trusted),'标题文件回放必须位于research/work独立目录');
    const owned=await fs.mkdtemp(path.join(work,'title-files-'));
    let child,passed=false,interrupted=false;
    const seconds=[];
    const cancel=()=>{interrupted=true;if(child)child.kill('SIGTERM');};
    process.on('SIGINT',cancel);process.on('SIGTERM',cancel);
    async function run(args) {
        need(!interrupted&&!child,'标题回放取消或子进程未收齐');
        const start=performance.now();
        return await new Promise((resolve,reject)=>{
            child=spawn(path.resolve(exe),['title-menu-files-v1',...args],{stdio:['ignore','pipe','pipe'],windowsHide:true});
            let stdout='',stderr='',overflow=false,timedOut=false;
            const timer=setTimeout(()=>{timedOut=true;child?.kill('SIGTERM');},60000);
            const collect=(data,isError)=>{
                if(isError)stderr+=data.toString();else stdout+=data.toString();
                if(Buffer.byteLength(stdout)+Buffer.byteLength(stderr)>1024*1024){overflow=true;child?.kill('SIGTERM');}
            };
            child.stdout.on('data',b=>collect(b,false));child.stderr.on('data',b=>collect(b,true));
            child.once('error',e=>{clearTimeout(timer);child=undefined;reject(e);});
            child.once('close',(code,signal)=>{
                clearTimeout(timer);child=undefined;seconds.push((performance.now()-start)/1000);
                if(code!==0||signal||timedOut||overflow||interrupted)
                    reject(Error(`标题回放失败 code=${code} signal=${signal} timeout=${timedOut}\n${stdout}\n${stderr}`));
                else resolve(stdout);
            });
        });
    }
    try {
        const roots=[0,1,2].map(n=>path.join(owned,`process-${n}`));
        for(const root of roots)await fs.mkdir(root);
        const traces=roots.map(root=>path.join(root,'tail.jsonl')),snapshot=path.join(roots[0],'prefix.avra');
        const parse=out=>{
            const rows=out.split(/\r?\n/).filter(s=>s.startsWith('title menu replay summary '));
            need(rows.length===1,'标题回放缺唯一终点');return JSON.parse(rows[0].slice('title menu replay summary '.length));
        };
        const terminal=parse(await run(['--work-dir',roots[0],'--save-file',snapshot,'--trace-file',traces[0]]));
        const source=await fs.readFile(snapshot),trace=await fs.readFile(traces[0]);
        const rows=trace.toString('utf8').trimEnd().split('\n').map(JSON.parse);
        need(rows.length===18&&rows.every((r,i)=>r.step===i&&r.random===0&&r.blob_count===1&&
            Number.isSafeInteger(r.blob_bytes)&&r.blob_bytes>0&&
            [r.app_driver_digest,r.system_sha256,r.file_view_digest].every(h=>/^[0-9a-f]{64}$/.test(h))&&
            Array.isArray(r.typed_audio)&&r.typed_audio.every(a=>Array.isArray(a)&&a.length===2&&a[0]===1&&(a[1]===0||a[1]===1))),
            '标题17命令须保完整18行状态/磁盘/有序声音/随机');
        need(terminal.controller==='title-menu-files-v1'&&terminal.steps===17&&terminal.audio_count===6&&
            terminal.random===0&&terminal.blob_count===1&&terminal.blob_bytes===rows[17].blob_bytes&&
            terminal.digest===rows[17].app_driver_digest&&terminal.system_sha256===rows[17].system_sha256&&
            terminal.file_view_digest===rows[17].file_view_digest,'标题菜单终点不符合固定真实路线');
        need(JSON.stringify(rows.map(r=>r.typed_audio).flat())===JSON.stringify([[1,1],[1,0],[1,1]]),
            '尾段新局G1/返标题B0/实际加载G1必须原序且仅一次');
        for(let i=1;i<3;i++) {
            const result=parse(await run(['--work-dir',roots[i],'--load-file',snapshot,'--trace-file',traces[i]]));
            need(JSON.stringify(result)===JSON.stringify(terminal),'标题完整终点不同');
            need(trace.equals(await fs.readFile(traces[i])),'标题文件动作三路逐字节trace不同');
            need(source.equals(await fs.readFile(snapshot)),'标题恢复改写源容器');
        }
        const certificate={controller:'title-menu-files-v1',qualification:'real_startup_menu_file_tail',
            command_count:17,trace_rows:18,process_count:3,process_wall_seconds:seconds,
            snapshot_bytes:source.length,snapshot_sha256:hash(source),trace_bytes:trace.length,trace_sha256:hash(trace),terminal,
            limits:['原中断自动产生者未覆盖','不是Steam窗口或输入投递认证','无世界日期推进、不认证自然通关']};
        if(certificateFile) {
            const parent=await fs.realpath(path.dirname(path.resolve(certificateFile)));
            need(within(parent,trusted),'证书父目录须在research/work内');
            await fs.writeFile(path.join(parent,path.basename(certificateFile)),JSON.stringify(certificate,null,2)+'\n',{flag:'wx'});
        }
        passed=true;return certificate;
    } finally {
        process.removeListener('SIGINT',cancel);process.removeListener('SIGTERM',cancel);
        need(!child,'标题子进程未收齐');need(path.dirname(owned)===work,'临时路径越界');
        if(passed)await fs.rm(owned,{recursive:true});else console.error('标题文件失败现场保留：'+owned);
    }
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)) {
    const args=process.argv.slice(2),options={};
    const keys={'--exe':'exe','--work-dir':'workDir','--certificate-file':'certificateFile'};
    for(let i=0;i<args.length;i+=2){need(keys[args[i]]&&i+1<args.length&&!options[keys[args[i]]],'参数非法');options[keys[args[i]]]=args[i+1];}
    need(options.exe&&options.workDir,'缺exe/work-dir');
    console.log(JSON.stringify(await verifyTitleMenuReplay(options),null,2));
}
