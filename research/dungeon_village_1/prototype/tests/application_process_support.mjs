// 两类应用回放共用的容器完整性与子进程生命周期；不授予任何Driver/轨迹认证资格。
import fs from 'node:fs/promises';
import path from 'node:path';
import {spawn} from 'node:child_process';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
const hash=bytes=>createHash('sha256').update(bytes).digest('hex');
const need=(value,message)=>{if(!value)throw Error(message);};

// 双文件发布不是原子事务：证书最后出现才具有认证资格。
// 第二次创建/写入失败时，仅回收本轮独占创建且身份/内容仍匹配的文件，不删除他人证书。
export async function publishApplicationReplayPair(destination,snapshot,certificate){
    const trusted=await fs.realpath(path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../../work'));
    const parent=await fs.realpath(path.dirname(path.resolve(destination)));
    const relative=path.relative(trusted,parent);
    need(relative&&!relative.startsWith('..')&&!path.isAbsolute(relative),'应用认证发布父目录越界');
    const target=path.join(parent,path.basename(destination)),owned=[];
    const write=async(file,bytes)=>{
        const handle=await fs.open(file,'wx');
        const entry={file,identity:undefined,bytes,written:false};owned.push(entry);
        try{entry.identity=await handle.stat();await handle.writeFile(bytes);entry.written=true;}
        finally{await handle.close();}
    };
    try{
        await write(target,snapshot);
        await write(target+'.json',Buffer.from(JSON.stringify(certificate,null,2)+'\n'));
    }catch(error){
        const failures=[];
        for(const entry of owned.reverse()){
            try{
                const stat=await fs.lstat(entry.file);
                need(entry.identity&&stat.isFile()&&!stat.isSymbolicLink()&&stat.dev===entry.identity.dev&&
                     stat.ino===entry.identity.ino&&stat.nlink===1,'发布失败文件身份已改变');
                // 完成写入的文件还须原字节相同；未完成写入也只允许本次字节前缀。
                need(stat.size<=entry.bytes.length,'发布失败文件出现外部增长');
                const actual=await fs.readFile(entry.file);
                need(actual.equals(entry.bytes.subarray(0,actual.length))&&
                     (!entry.written||actual.length===entry.bytes.length),'发布失败文件内容已改变');
                await fs.unlink(entry.file);
            }catch(cleanup){
                if(cleanup.code!=='ENOENT')failures.push(entry.file+': '+cleanup.message);
            }
        }
        if(failures.length)throw Error(error.message+'；本轮发布回收未完成：'+failures.join('；'));
        throw error;
    }
}

// 完整世界4/应用7与Driver语义由C++核验；本层仅验证相同的容器、分区及指定controller身份。
export function applicationReplayIdentity(bytes,expectedController){
    need(bytes.length>84&&bytes.length<=128*1024*1024,'应用文件预算');
    need(bytes.subarray(-64).toString('ascii')===hash(bytes.subarray(0,-64)),'应用整体摘要');
    const reader=b=>({at:0,b,raw(n){need(Number.isSafeInteger(n)&&n>=0&&n<=this.b.length-this.at,'分区边界');
        const value=this.b.subarray(this.at,this.at+n);this.at+=n;return value;},
        u32(){return this.raw(4).readUInt32LE();},u64(){const value=this.raw(8).readBigUInt64LE();
            need(value<=BigInt(Number.MAX_SAFE_INTEGER),'不可表示的整数');return Number(value);},
        text(){const size=this.u32();need(size<=4096,'身份文本预算');return this.raw(size).toString('utf8');}});
    const r=reader(bytes.subarray(0,-64));
    need(r.raw(8).toString('ascii')==='AVRAPP01'&&r.u32()===1&&r.u32()===7&&r.u32()===1,'应用版本');
    const dataset=r.text(),world_schema=r.text(),application_schema=r.text(),count=r.u32();
    need(count>=6&&count<=66,'应用分区数量');
    const sections=new Map(),sizes={};
    for(let i=0;i<count;i++){
        const id=r.u32(),version=r.u32(),required=r.u32(),length=r.u64(),digest=r.raw(64).toString('ascii');
        const body=r.raw(length);need(!sections.has(id)&&version>0&&required<=1&&hash(body)===digest,'分区身份/摘要');
        sections.set(id,{version,required,body});sizes[id]=length;
    }
    need(r.at===r.b.length,'容器尾部');
    for(const id of [1,2,3,4,5,6])need(sections.get(id)?.version===1&&sections.get(id)?.required===1,'应用缺段');
    const meta=reader(sections.get(1).body),controller=meta.text(),producer=meta.text();
    const next_frame=meta.u64(),next_command=meta.u64();need(meta.at===meta.b.length,'metadata尾部');
    need(controller===expectedController&&next_frame>1,'指定应用Driver身份');
    return {controller,producer,next_frame,next_command,dataset,world_schema,application_schema,section_bytes:sizes};
}

// 调用者先创建独占mkdtemp目录。辅助只写该目录日志，不创建/删除应用根或认证证书。
// 任一时刻最多一个直接子进程；失败、取消、超时和输出超预算均等close后才回到调用者。
export function applicationReplayProcesses(executable,owned,timeout){
    let child,cancelled=false,invocation=0;const seconds=[];
    const cancel=()=>{cancelled=true;child?.kill();};
    process.on('SIGINT',cancel);process.on('SIGTERM',cancel);
    return {
        seconds,
        async run(args){
            need(!cancelled&&!child,'应用回放取消或旧进程未收齐');const index=invocation++,started=performance.now();
            return await new Promise((resolve,reject)=>{
                const p=spawn(executable,args,{cwd:path.dirname(executable),windowsHide:true,stdio:['ignore','pipe','pipe']});
                child=p;const out=[],err=[];let size=0,timedOut=false,overflow=false,launchError;
                const timer=setTimeout(()=>{timedOut=true;p.kill();},timeout);
                const collect=(bytes,isError)=>{
                    size+=bytes.length;if(size>1024*1024){overflow=true;p.kill();return;}
                    (isError?err:out).push(bytes);if(!isError)process.stdout.write(bytes);
                };
                p.stdout.on('data',bytes=>collect(bytes,false));p.stderr.on('data',bytes=>collect(bytes,true));
                p.on('error',error=>{launchError=error;});
                p.on('close',async(code,signal)=>{
                    clearTimeout(timer);child=undefined;
                    const stdout=Buffer.concat(out).toString(),stderr=Buffer.concat(err).toString();
                    try{
                        await fs.writeFile(path.join(owned,`process-${index}.log`),stdout+'\n'+stderr,{flag:'wx'});
                        need(!launchError&&!timedOut&&!overflow&&!cancelled&&code===0&&!signal&&!stderr.trim(),
                            `应用回放进程失败 code=${code} timeout=${timedOut} overflow=${overflow}: ${launchError??''}\n${stderr}`);
                        seconds.push((performance.now()-started)/1000);resolve(stdout);
                    }catch(error){reject(error);}
                });
            });
        },
        finish(){
            process.removeListener('SIGINT',cancel);process.removeListener('SIGTERM',cancel);
            need(!child,'子进程未收齐');
        },
    };
}
