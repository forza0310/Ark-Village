// 固定数据身份用于文件恢复资格；Git提交只作追溯，不作为机械失效条件。
import { readFileSync, writeFileSync } from 'node:fs';
import { createHash } from 'node:crypto';
import path from 'node:path';
const [root, output] = process.argv.slice(2);
if (!root || !output) throw Error('expected product root and generated C++ output');
const files = ['data/startup/MAP.json','data/startup/STATE.json','data/startup/TABLES.json',
 'data/original/tenantData.txt','data/world/monster.txt','data/world/questData.txt',
 'data/world/armour.txt','data/world/accessory.txt','data/world/item.txt','data/world/asEventData.txt',
 'data/scripts/original/events.txt','data/scripts/original/talk.txt','data/scripts/original/news.txt',
 'data/scripts/original/evtmsgs.txt','data/scripts/original/popularBonus.txt'];
const hash = b => createHash('sha256').update(b).digest('hex');
// Preserve published logical paths in the hash; only the on-disk product root differs.
const productFile = file => file === 'data/original/tenantData.txt'
  ? 'assets/simulation/tenantData.txt' : file.replace(/^data\//, 'assets/simulation/');
const manifest = files.map(file => [file,hash(readFileSync(path.join(root,productFile(file))))]);
const identity = hash(JSON.stringify(manifest));
writeFileSync(output, '// 固定来源清单的SHA-256；只读生成文件。\nnamespace ark::simulation {\nconst char *startup_world_persistence_dataset() { return "'+identity+'"; }\n}\n');
writeFileSync(output+'.json', JSON.stringify({identity,files:manifest},null,2)+'\n');
