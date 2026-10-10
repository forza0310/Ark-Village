// 只读冻结表和既有DEX；候选摘要只写work/character-job-unlocks，不发布正式索引。
const fs = require('fs'), path = require('path'), crypto = require('crypto'), vm = require('vm');
const {createRequire} = require('module');
const root = path.resolve(__dirname, '../..');
const outputDirectory = path.join(root, 'work/character-job-unlocks');
if (process.argv.length !== 2) throw Error('本工具无参数；输出限定为研究work专题');
const hash = b => crypto.createHash('sha256').update(b).digest('hex');
const read = p => fs.readFileSync(path.join(root, p));
let checks = 0;
const need = (v, message) => { ++checks; if (!v) throw Error(message); };
const tables = JSON.parse(read('data/startup/TABLES.json'));
const index = JSON.parse(read('data/PROGRESSION_CONTENT_INDEX.json'));
need(tables.apk_sha256 === index.profiles.apk_sha256 &&
     tables.apk_sha256 === '1e52408aeaebec2d92a5e50b8c0d964566d0038456bc9b081e18b7d3446a4ef5', '固定APK身份');
const specs = [['human', 'character.txt', 25, 14], ['profession', 'job.txt', 23, 24],
               ['event', 'events.txt', 200, 5], ['popularity_bonus', 'popularBonus.txt', 100, 7]];
const rows = {}, identities = [];
for (const [kind, entry, count, width] of specs) {
    const t = tables.entries.find(t => t.entry === entry), i = index.tables.find(t => t.kind === kind);
    need(t && i && t.rows === count && i.definitions === count && t.sha256 === i.sha256 &&
         hash(Buffer.from(t.source_utf8)) === t.sha256, '表身份:' + entry);
    const lines = t.source_utf8.split('\n');
    need(lines.length === count, '表分母:' + entry);
    rows[kind] = lines.map((line, n) => {
        const r = line.split('\t'), record = i.records[n];
        need(r.length === width && Number(r[0]) === record.id &&
             hash(Buffer.from(line)) === record.source.row_sha256, '行身份:' + entry + ':' + n);
        return {fields: r, id: record.id, source: record.source, name: record.name_zh};
    });
    identities.push({kind, entry, definitions: count, bytes: t.bytes, sha256: t.sha256});
}
const commands = p => p ? p.split('&').map(c => c.split(',').map(Number)) : [];
const edges = [];
for (const kind of ['event', 'popularity_bonus'])
    for (const r of rows[kind]) {
        const program = commands(r.fields[kind === 'event' ? 4 : 5]);
        program.forEach((c, ordinal) => {
            if (![21, 31].includes(c[0])) return;
            need(rows[c[0] === 21 ? 'human' : 'profession'].some(r => r.id === c[1]), '指令引用存在');
            edges.push({target_kind: c[0] === 21 ? 'human' : 'profession', target_id: c[1],
                producer_kind: kind, producer_id: r.id, producer_name: r.name, source: r.source,
                instruction_index: ordinal, opcode: c[0], arguments: c.slice(1),
                preceding_delay_instructions: program.slice(0, ordinal).filter(c => c[0] === 6).map(c => c[1]),
                threshold: kind === 'popularity_bonus' ? Number(r.fields[2]) : null,
                event_flags: kind === 'event' ? Number(r.fields[2]) : null,
                event_conditions: kind === 'event' ? commands(r.fields[3]) : null,
                limit: '表内正向边；事件调用者/自然可达性不是本条目独立证明'});
        });
    }

// 复用已审计的逐指令DEX解码器，不复制另一个解析实现。固定其hash和抽取边界，
// 仅执行APK身份校验、表解析、decode及只读扫描段；原工具写输出段不会执行。
const decoderPath = 'tools/task-selection-producers/dex-audit.cjs';
const decoderFile = path.join(root, decoderPath), decoder = read(decoderPath).toString('utf8');
const legacyDecoderHash = '50a26126b54ed0e5790bd7afaa084a61a9958c009d08e5b8fdb21755f3bb0c04';
const currentDecoderHash = '336fde02be2efac3adc5205f15214ec44f65b69a0a02fc2adb196607b42346c1';
need(hash(Buffer.from(decoder)) === currentDecoderHash, '迁后DEX工具版本');
// 迁置只注入路径适配器，并替换require/__dirname/__filename。逆变换必须逐字节
// 还原原先已核50a261...源；这不是将旧证据hash更新为任意当前内容。
const adapterHeader = "const archivePaths = require('../work_archive_paths.cjs').forTool(__filename);\n";
need(decoder.startsWith(adapterHeader), '迁后DEX工具显式适配头');
const legacyDecoder = decoder.slice(adapterHeader.length)
    .replace(/archivePaths\.require\(/g, 'require(')
    .replace(/archivePaths\.workDir/g, '__dirname')
    .replace(/archivePaths\.sourceFile/g, '__filename');
need(hash(Buffer.from(legacyDecoder)) === legacyDecoderHash, '迁前DEX源逐字节还原');
const slice = (a, b) => {
    const start = decoder.indexOf(a), end = decoder.indexOf(b, start + a.length);
    need(start >= 0 && end > start, 'DEX复用边界');
    return decoder.slice(start, end);
};
const prefix = decoder.slice(0, decoder.indexOf('const wanted=new Set'));
const decode = slice('const widths=new Map', 'const wantedCalls=new Set');
const scan = slice('const hits=[],codeIndex=[]', 'const sourcePaths=');
const decoderReuse = {
    path: decoderPath, current_sha256: currentDecoderHash, original_sha256: legacyDecoderHash,
    original_reconstructed_sha256: hash(Buffer.from(legacyDecoder)),
    adaptation: '仅路径适配头与require/__dirname/__filename替换；逆变换命中原完整源hash',
    slices: [['decode', decode], ['scan', scan]].map(([name, text]) => {
        need(legacyDecoder.includes(text), '原解码/扫描语义切片原字节保持:' + name);
        return {name, bytes: Buffer.byteLength(text), sha256: hash(Buffer.from(text)), unchanged: true};
    })
};
const selector = `
const wanted = new Set(fields.filter(f => ['La/d;::p:I','La/e;::p:I','La/h;::p:I'].includes(f)));
const wantedCalls = new Set(['La/e;::i()V', 'La/h;::c()V', 'La/e;::h()V', 'La/h;::b()V']);
const windows = new Map([
 ['La/e;::h()V', [[0,22]]], ['La/e;::i()V', [[0,15]]],
 ['La/h;::b()V', [[0,22]]], ['La/h;::c()V', [[0,12]]],
 ['La/e;::a(Lc/b;)V', [[430,510]]], ['Lc/n;::c()V', [[480,515]]],
 ['Ld/a;::a(IZ)V', [[2120,2200]]]
]);
`;
// 使用被复用工具本身的真实CommonJS定位；不把scripts目录伪装成其父目录。
// 适配器只读迁后输入，以下VM执行串不含原工具的写输出段。
const context = {require: createRequire(decoderFile), __filename: decoderFile,
                 __dirname: path.dirname(decoderFile), result: null};
vm.runInNewContext(prefix + decode + selector + scan + `
result = {dex_sha256:sha(b),dex_bytes:b.length,apk_sha256:sha(apk),
 method_definitions_scanned:scanned,instruction_units_scanned:units,
 target_fields:[...wanted],call_targets:[...wantedCalls],
 writes:hits.filter(h=>h.access.endsWith('put')),callers,method_windows:methodWindows};`, context, {timeout: 30000});
const dex = context.result;
need(dex.method_definitions_scanned === 1906 && dex.instruction_units_scanned === 295672, 'DEX扫描分母');
need(dex.target_fields.includes('La/e;::p:I') && dex.target_fields.includes('La/h;::p:I'), '人物/职业具名字段');
const loadWindow = dex.method_windows.find(w => w.method === 'Ld/a;::a(IZ)V').windows[0].instructions;
for (const [pc, op, target] of [[2142,'39',2155],[2146,'3c',2152],[2150,'3d',2159],[2185,'33',2191]])
    need(loadWindow.some(i => i.pc === pc && i.op === op && i.target === target), '加载修复真实分支:' + pc);

const humans = rows.human.map(r => ({id: r.id, name_zh: r.name, source: r.source,
    flags: Number(r.fields[13]), initial_open: (Number(r.fields[13]) & 1) !== 0,
    first_arrival_flag8: (Number(r.fields[13]) & 8) !== 0, initial_job: Number(r.fields[3]),
    direct_table_edges: edges.filter(e => e.target_kind === 'human' && e.target_id === r.id),
    saved_state_reader: 'a/e.a(InputStream) restores p; DEX d/a.a(IZ) repairs p==0 to1 when C>0 or u>0 or a loaded bl actor has same definition; not a popularity threshold',
    generic_reward_consumer: 'raw93/94/95 r2 confirms at40 and calls i(); per-definition page producer not proved by this generic consumer',
    boundary: '开放不等于到访；人物已有初职不等于该职业已向所有人开放'}));
const jobs = rows.profession.map(r => ({id: r.id, name_zh: r.name, source: r.source,
    flags: Number(r.fields[23]), initial_open: (Number(r.fields[23]) & 1) !== 0,
    direct_table_edges: edges.filter(e => e.target_kind === 'profession' && e.target_id === r.id),
    original_humans: humans.filter(h => h.initial_job === r.id).map(h => h.id),
    mastery_edge: {condition: '当前职业到10且其共享p==0', effect: 'a/h.c()先开放，再请求94r4和事件113/通知33',
                  note: '原表人物初职提供候选产生者，不证明该人物自然到访/成长完成'},
    inheritance_edge: '系统J.e1按定义序恢复职业p，非个人M等级',
    generic_reward_consumer: 'raw93/94/95 r4 confirms at40 and calls c(); opcode31/mastery already opened before their announcement pages',
    saved_state_reader: 'a/h.a(InputStream) restores p/r', boundary: '开放不等于转职或大师加成已领取'}));
need(humans.filter(h => h.initial_open).map(h => h.id).join(',') === '1,2,3', '人物初始集合');
need(jobs.filter(j => j.initial_open).map(j => j.id).join(',') === '0,1,2,3', '职业初始集合');
need(humans.every(h => h.initial_open || h.direct_table_edges.some(e => e.producer_kind === 'popularity_bonus')), '每个人物有初始或人气正向边');
const sourcePaths = ['data/startup/TABLES.json','data/PROGRESSION_CONTENT_INDEX.json',decoderPath,
    'tools/work_archive_paths.cjs','tools/scripts/character_job_unlocks.cjs',
    'work/decompiled/sources/a/e.java','work/decompiled/sources/a/h.java','work/decompiled/sources/a/l.java',
    'work/decompiled/sources/d/a.java','work/decompiled/sources/c/n.java','work/decompiled/sources/b/g.java'];
const sources = sourcePaths.map(p => {const b = read(p); return {path:p,bytes:b.length,sha256:hash(b)};});
const result = {version:1,date:'2026-10-10',scope:'固定APK正向来源静态审计；不证明自然路线/Steam消费者/全程序可达性',
    identities,humans,jobs,dex,decoder_reuse:decoderReuse,sources,
    gaps:['事件66至71仅见表内边；调用者尚未据此认证','具名DEX直接写与调用不是别名/反射/JNI穷尽证明',
          '普通到访/入住及职业转职沿既有合同，未在本次执行自然路线','Steam表数字一致不证明所有消费者一致']};
const output = Buffer.from(JSON.stringify(result, null, 2) + '\n');
need(output.length <= 256 * 1024, '派生输出预算');
fs.mkdirSync(outputDirectory, {recursive:true});
fs.writeFileSync(path.join(outputDirectory, 'INDEX.json'), output);
need(hash(fs.readFileSync(path.join(outputDirectory, 'INDEX.json'))) === hash(output), '写后实际消费');
const validation = {checks,humans:humans.length,jobs:jobs.length,table_edges:edges.length,
    human_popularity_edges:edges.filter(e=>e.target_kind==='human'&&e.producer_kind==='popularity_bonus').length,
    dex_methods:dex.method_definitions_scanned,dex_writes:dex.writes.length,dex_calls:dex.callers.length,
    output_bytes:output.length,output_sha256:hash(output),no_game_or_build:true,no_save_access:true};
fs.writeFileSync(path.join(outputDirectory, 'VALIDATION.json'), JSON.stringify(validation,null,2)+'\n');
console.log(JSON.stringify(validation,null,2));
