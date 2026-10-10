// 独立构建期原表投影；完整哈希、行/列和跨目录引用验证后才生成只读C++。
import { readFileSync, writeFileSync } from 'node:fs';
import { createHash } from 'node:crypto';
import { fileURLToPath } from 'node:url';
import { compileStartup } from './compile_startup.mjs';

const apk = '1e52408aeaebec2d92a5e50b8c0d964566d0038456bc9b081e18b7d3446a4ef5';
const hashes = {
  'character.txt':'8d9bb0464708d5296ab158012887f52cabc03b450e56800245b54334a25690ee',
  'job.txt':'6021bea7106b500afbf3b5fb59a2b943ee8b52aef952945174d9dfdb56cb6825',
  'weapon.txt':'691ea930f3b9d18114e34fe2a4f0c211cc8ad22c1b7a8c6be9e78c735c1b7670',
  'monster.txt':'c501df93a6823dbf029f32c37d5092c3a2ec252a3287887e2e16e7969e2b4da5',
  'questData.txt':'90fd2ec1f9ffa5110e03c1f41c8f338f50c98c88b279605cd4a8ee006883f5c6',
  'armour.txt':'6421b227874fca6c843452d3627890f0e61e9ed1d522b6571e4f00da1d2b6441',
  'accessory.txt':'533574fdd2c6704180d7cfec770c2bdd1bbe7fadb7e6293b92c2622234328af2',
  'item.txt':'95e29d253688e30ac623b925a9a5dacd1f6280f60ed0528b551707df1252d6b8',
  'tenantData.txt':'5ae35310fbd178f98b273fc2bbe98b1bbf5b72950fca56dbd93c089ca345834a',
  'asEventData.txt':'11d22c23de2760bbcc7b87450ba70534482afd2533ca7bab1bf6887c46829a1a',
  'magicPot.txt':'7a28381b01ae861cd99ce8ccc63bef5f8ce62f12473e3047052879f8f12a5b8c'
};
const scriptHashes = {
  'events.txt':'64a777f2dd590b5e8837a10168bc8dd12353426eb6e286befa4b88753eedb28d',
  'talk.txt':'0e078de2e6ea574244fe6f8a9a46b69b834195bc9d1508275aaa9accbaf4baf5',
  'news.txt':'5ea7c170b95e5110b8c44fedfd1056a323a7cfb57dc7959e52073b318cc155df',
  'evtmsgs.txt':'143faa1f2f9ba243e9e37cde06999f9cd080d7d329b6c3355f570d8fe6dd48f0',
  'popularBonus.txt':'19a010efa9bca14e2757a7ede3fd9080ad306cb50cc758157597575f1407bfcb'
};
const need = (condition, message) => { if (!condition) throw new Error(message); };
const n = value => {
  need(/^-?\d+$/.test(String(value)), '整数格式错误');
  const result = Number(value);
  need(Number.isSafeInteger(result) && result >= -2147483648 && result <= 2147483647, '整数越界');
  return result;
};
const array = values => `{${values.map(n).join(',')}}`;
const text = JSON.stringify;
const values = value => value === '' ? [] : value.split('&').map(n);
const truth = value => value ? 'true' : 'false';
// 固定配方字段/引用同一验证路径；独立导出供既有输入拒绝套件定向检查。
// compile入口始终先验证原文哈希，不能通过此函数绕过冻结输入。
export function validateMagicPotRows(recipes, rewardCounts) {
  need(Array.isArray(recipes) && recipes.length === 40, '魔法壶固定配方数量不符');
  need(Array.isArray(rewardCounts) && rewardCounts.length === 5 &&
    rewardCounts.every(v => Number.isSafeInteger(v) && v > 0), '魔法壶奖励目录数量非法');
  recipes.forEach((row,index) => {
    need(Array.isArray(row) && row.length === 10 && n(row[0]) === index &&
      typeof row[1] === 'string' && row[1].length > 0, '魔法壶配方行/列/身份不符');
    const type=n(row[2]), reward=n(row[3]), experience=n(row[4]), costs=row.slice(5,9).map(n), flags=n(row[9]);
    need(type >= 0 && type < 5, '魔法壶奖励类型非法');
    need(reward >= 0 && reward < rewardCounts[type], '魔法壶奖励跨目录引用缺失');
    need(experience >= 0 && costs.every(v=>v >= 0) && flags >= 0, '魔法壶经验/成本/标志非法');
  });
}
export function compileStartupWorld(tables, map, sources, state) {
  need(tables.apk_sha256 === apk && map.apk_sha256 === apk, 'APK身份不匹配');
  // 复用已有完整地图重编码/哈希与新局状态交叉校验，不仅相信region.logical。
  compileStartup(map, state, tables, sources['tenantData.txt']);
  for(const [name,hash] of Object.entries(scriptHashes))
    need(typeof sources[name]==='string' && createHash('sha256').update(sources[name]).digest('hex')===hash,
      `固定脚本哈希不匹配：${name}`);
  const raw = new Map(tables.entries.map(entry => [entry.entry, entry.source_utf8]));
  need(raw.size === tables.entries.length, '重复发布表');
  for (const [name, contents] of Object.entries(sources)) raw.set(name, contents);
  const rows = {};
  const widths = {'character.txt':14,'job.txt':24,'weapon.txt':19,'monster.txt':17,
    'questData.txt':17,'armour.txt':13,'accessory.txt':13,'item.txt':25,'tenantData.txt':36,'asEventData.txt':12,'magicPot.txt':10};
  for (const [name, hash] of Object.entries(hashes)) {
    const content = raw.get(name);
    need(typeof content === 'string' && createHash('sha256').update(content).digest('hex') === hash,
      `固定原表哈希不匹配：${name}`);
    rows[name] = content.replace(/\n$/, '').split('\n').map((line,index) => {
      const row = line.replace(/\r$/, '').split('\t');
      need(row.length === widths[name] && n(row[0]) === index, `${name}行/列/身份不符`);
      return row;
    });
  }
  const jobs = rows['job.txt'], weapons = rows['weapon.txt'], armor = rows['armour.txt'];
  const accessory = rows['accessory.txt'], monsters = rows['monster.txt'];
  const facilities = rows['tenantData.txt'], humans = rows['character.txt'];
  need(jobs.length === 23 && weapons.length === 33 && facilities.length === 85,
    '固定目录数量不符');
  validateMagicPotRows(rows['magicPot.txt'], [rows['item.txt'].length,weapons.length,armor.length,accessory.length,facilities.length]);
  const recipeOutput=rows['magicPot.txt'].map(row=>
    `{${n(row[0])},${text(row[1])},${n(row[2])},${n(row[3])},${n(row[4])},${array(row.slice(5,9))},${n(row[9])}}`);
  const spells = Array.from({length:4},(_,slot)=>jobs.find(row=>n(row[18]) >= 10 &&
    n(row[18]) % 10 === slot)).map(row=>{ need(row, '魔法职业缺失'); return n(row[0]); });
  const humanOutput = humans.map(row => {
    const job = n(row[3]), equipment = values(row[11]), base = values(row[12]);
    need(job >= 0 && job < jobs.length && equipment.length === 4 && base.length === 6,
      '人物职业/属性/装备数组不符');
    const levels = Array(jobs.length).fill(1), ids = values(row[4]), initial = values(row[5]);
    need(ids.length === initial.length, '人物职业初值长度不符');
    ids.forEach((id,i)=>{need(id>=0&&id<jobs.length&&initial[i]>=1&&initial[i]<=10,'职业初值越界');levels[id]=initial[i];});
    const combat = equipment.map((id,slot)=>{
      if (id === -1) return 'std::nullopt';
      const table = slot === 0 ? weapons : slot === 3 ? accessory : armor;
      need(id>=0&&id<table.length, '人物装备引用缺失');
      return `std::array<int,4>${array(slot===0?table[id].slice(12,16):table[id].slice(6,10))}`;
    });
    const flags = n(row[13]);
    return `{${n(row[0])},${text(row[1])},${n(row[2])},${flags},${flags&1?1:0},`+
      `{${job},${n(row[6])},${array(base)},{},${array(levels)},{{${combat.join(',')}}},{},${array(spells)}},${array(equipment)},`+
      `${n(row[7])},`+(row[10]===''?'{}':`{${row[10].split('&').map(op=>array(op.split(','))).join(',')}}`)+
      `,${n(row[8])},`+(row[9]===''?'{}':`{${row[9].split('&').map(op=>array(op.split(','))).join(',')}}`)+'}';
  });
  const equipmentOutput = [weapons,armor,accessory].flatMap((table,index)=>table.map(row=>{
    const weapon = index===0, flag = n(row[weapon?18:12]), opened = (flag&1)!==0;
    const rank = n(row[weapon?5:4]), type = n(row[weapon?2:2]);
    const price = n(row[weapon?11:5]), combat = row.slice(weapon?12:6,weapon?16:10);
    const renderImage=n(row[3]), renderStyle=weapon?n(row[6]):0;
    const weaponImage = [[0,5],[10,15],[20,25],[30,30],[40,46],[50,56]]
      .some(([lo,hi])=>renderImage>=lo&&renderImage<=hi); // 原weapon/img.inf显式ID域。
    need(renderImage >= 0 && (weapon ? weaponImage && renderStyle >= 0 && renderStyle < 4
      : renderImage < (index===1 ? 50 : 30)), '装备举物图片/图标/风格索引越界');
    return `{{${index+1},${n(row[0])},${rank},${type},${truth(opened)},${price},${array(combat)}},`+
      `{${flag},${opened?1:0},0,false,0,${opened?1:0}},`+
      (weapon?`{${n(row[6])},${n(row[7])},${n(row[8])},${n(row[9])},${n(row[10])}}`:'{}')+
      `,${n(row[weapon?5:4])},${text(row[1])},${n(row[weapon?16:10])},${n(row[weapon?17:11])},${renderImage},${renderStyle}}`;
  }));
  const monsterOutput = monsters.map(row=>{
    const flag=n(row[16]), opened=(flag&1)!==0;
    need(n(row[2])>=0&&n(row[2])<=3&&n(row[6])>=0,'怪物体型/难度越界');
    return `{${n(row[0])},${text(row[1])},${flag},${n(row[12])},${n(row[11])},`+
      `{0,0,${n(row[10])},${n(row[13])},${n(row[7])},${n(row[2])},${n(row[4])},`+
      `${n(row[6])},${opened?1:0},${truth(opened)},${truth(opened)},${truth(row[15]!=='')},${n(row[8])},${n(row[9])}},`+
      (row[15]===''?'{}':`{${row[15].split('&').map(op=>array(op.split(','))).join(',')}}`)+'}';
  });
  const taskOutput = rows['questData.txt'].map(row=>{
    const sites=values(row[7]), monster=n(row[11]),flag=n(row[16]), normal=values(row[10]);
    need(monster>=0&&monster<monsters.length&&sites.every(id=>id>=0&&id<facilities.length)&&
      normal.every(id=>id>=0&&id<monsters.length),
      '任务跨目录引用缺失');
    need(n(row[8])<=n(row[9]),'任务奖励范围反转');
    return `{{${n(row[0])},${n(row[3])},${n(row[5])},${flag},0,${monster},${array(sites)},`+
      `${n(row[8])},${n(row[9])},${n(row[14])}},${text(row[1])},${text(row[2])},${flag&1?1:0},${n(row[12])},${array(normal)},${n(row[6])},${n(row[15])}}`;
  });
  const itemOutput=rows['item.txt'].map(row=>{
    const flag=n(row[24]),opened=(flag&1)!==0;
    need(row.slice(12,16).map(n).every(value=>value>=0), '道具魔法壶四元素非法');
    need(n(row[5])>=0 && n(row[5])<89, '道具图标超出原分类表C');
    return `{${n(row[0])},{${flag},${opened?1:0},0,false,${n(row[17])},0},`+
      `{${n(row[0])},${n(row[7])},${n(row[20])},${n(row[18])},${opened?1:0},0,false},${n(row[8])},${text(row[1])},`+
      `${n(row[3])},${n(row[4])},${n(row[6])},${n(row[16])},${n(row[21])},${array(row.slice(9,12))},${n(row[19])},${array(row.slice(12,16))},${n(row[5])},${text(row[23])}}`;
  });
  const excess=[];
  const facilityOutput=facilities.map(row=>{
    const attrs=Array.from({length:4},(_,slot)=>array(row.slice(15+slot*2,17+slot*2)));
    const economy=`{{{${attrs.join(',')}}},${array(row.slice(23,25))},${n(row[13])},${n(row[14])},${n(row[35])},${truth(n(row[3])===2)}}`;
    const pairs=(indices,deltas,max,exit=false)=>{
      const a=values(indices),b=values(deltas);
      need((exit?b.length>=a.length:a.length===b.length)&&a.every(v=>v>=0&&v<max),'设施效果数组错误');
      if(exit&&b.length>a.length)excess.push(`{${n(row[0])},${array(b.slice(a.length))}}`);
      return `{${a.map((v,i)=>`{${v},${b[i]}}`).join(',')}}`;
    };
    need(n(row[10])>=0&&n(row[10])<=2,'设施形状越界');
    return `{${n(row[0])},${text(row[1])},${n(row[3])},${n(row[8])},${n(row[13])},${n(row[14])},`+
      `${n(row[9])},${n(row[4])},${n(row[11])},${n(row[35])},${n(row[10])},${n(row[19])},`+
      `${n(row[5])},${n(row[25])},${economy},${pairs(row[28],row[29],6,true)},${pairs(row[26],row[27],3)},${n(row[32])},${n(row[2])}}`;
  });
  const regions=name=>`{${map[name].map(region=>{
    const a=region.logical;
    need(a.length===4&&a.every(Number.isSafeInteger)&&a.join(',')===
      [region.raw[0],23-region.raw[1],region.raw[2],23-region.raw[3]].join(','),'区域坐标来源不符');
    return `{{{${a[0]},${a[1]}},{${a[2]},${a[3]}}}}`;
  }).join(',')}}`;
  return '// 自动生成；固定APK完整世界目录，不修改原表。\n'+
    '#include "dungeon_village_prototype/startup_world_projection.hpp"\n'+
    'namespace dungeon_village_prototype {\nconst StartupWorldRules &startup_world_rules() {\n'+
    'static const StartupWorldRules value{\n'+
    `{${jobs.map(row=>`{${n(row[6])},${n(row[8])},${array(row.slice(3,5))},${array(row.slice(13,15))},${array(row.slice(11,13))},${text(row[1])},${n(row[23])},${n(row[23])&1?1:0},${n(row[5])},${n(row[2])},${n(row[7])},${n(row[15])},${n(row[16])},${n(row[18])},${n(row[19])},${array(values(row[17]))}}`).join(',')}},\n`+
    `{${humanOutput.join(',')}},\n{${equipmentOutput.join(',')}},\n{${monsterOutput.join(',')}},\n`+
    `{${itemOutput.join(',')}},\n{${taskOutput.join(',')}},\n{${facilityOutput.join(',')}},\n`+
    `${regions('regions_l')},${regions('regions_m')},{${excess.join(',')}},\n`+
    `{${facilities.map(row=>{const f=n(row[35]);
      const program=row[33]===''?'{}':`{${row[33].split('&').map(op=>array(op.split(','))).join(',')}}`;
      // c/n.c在逐定义a()后调用o.g：实际新局N为种类2取30，其余20，不是Java分配时0。
      // o.a()的bit1分支调用o.b()，后者写p=2；Steam TenantData.NewGame亦如此。
      return `{${f},${f&1?2:0},${n(row[3])===2?30:20},${f&64?(n(row[0])===24?1:280):0},${n(row[12])},${truth(f&2)},${truth(f&1)},${program},${array(values(row[34]))}}`;
    }).join(',')}},\n`+
    `{${Object.keys(scriptHashes).map(name=>text(sources[name])).join(',')}},\n`+
    `{${rows['asEventData.txt'].map(row=>`{${n(row[0])},${text(row[1])},${array(row.slice(2,9))},${text(row[9])},${text(row[10])},${n(row[11])},${n(row[11])&1?1:0}}`).join(',')}},"G",`+
    `${array(map.cells.flat().map(cell=>cell[1]))},{${recipeOutput.join(',')}}};\nreturn value;\n}\n}\n`;
}
if (process.argv[1] === fileURLToPath(import.meta.url)) {
  const [startup,world,tenant,output]=process.argv.slice(2);
  need(startup&&world&&tenant&&output,'参数：startup目录 world目录 tenant原表 输出C++');
  const sources={};
  for (const name of ['monster.txt','questData.txt','armour.txt','accessory.txt','item.txt','asEventData.txt','magicPot.txt'])
    sources[name]=readFileSync(`${world}/${name}`,'utf8');
  sources['tenantData.txt']=readFileSync(tenant,'utf8');
  for(const name of Object.keys(scriptHashes))sources[name]=readFileSync(`${world}/../scripts/original/${name}`,'utf8');
  writeFileSync(output,compileStartupWorld(JSON.parse(readFileSync(`${startup}/TABLES.json`)),
    JSON.parse(readFileSync(`${startup}/MAP.json`)),sources,
    JSON.parse(readFileSync(`${startup}/STATE.json`))));
}
