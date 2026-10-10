// 主动策略共同的只读身份预检；不授予来源资格，业务与Driver语义仍由各C++validator核验。
import {createHash} from 'node:crypto';
import {applicationReplayIdentity} from './application_process_support.mjs';
const hash=b=>createHash('sha256').update(b).digest('hex');
const need=(ok,why)=>{if(!ok)throw Error(why);};
const schema={dataset:'c3f9419d56e2db0c4a4fcf582f3b34fe5100a12b6acaa2b3bbf3ca0273faa690',
    world_schema:'7f33851d8b0430afbb6234595ab01d2190f29959515d216e6da64b1919dbf440',
    application_schema:'bd1940f2adef221c3da305403518082e34fd5eb2b1756e478240f564725edf0c'};

export function progressionApplicationIdentity(bytes,controller,driverMagic){
    need(typeof driverMagic==='string'&&/^[A-Z0-9]{8}$/.test(driverMagic),'显式Driver magic非法');
    const result=applicationReplayIdentity(bytes,controller);
    for(const key of Object.keys(schema))need(result[key]===schema[key],'固定来源字段身份不符 '+key);
    // 共有容器预检已经证明每个分区的边界、长度和hash；这里只检更窄的主动来源语义。
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
    need(driver.length>=8&&driver.length<=1024*1024&&driver.subarray(0,8).toString()===driverMagic,'策略规范Driver身份');
    return {...result,driver_digest:hash(driver)};
}
