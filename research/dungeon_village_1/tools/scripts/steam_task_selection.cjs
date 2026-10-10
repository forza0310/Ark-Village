// 固定 Steam 样本的任务选择审计：只读方法字节，输出局部证据到忽略的 work。
// 不执行游戏、不重建选择算法；指令锚由人工逐窗口复核后固定，保留未核数组限制。
'use strict';
const fs = require('fs'), path = require('path'), crypto = require('crypto');
const root = path.resolve(__dirname, '../..');
const sha = b => crypto.createHash('sha256').update(b).digest('hex');
const read = p => fs.readFileSync(path.join(root, p));
const dllPath = 'DungeonVillageEXE/GameAssembly.dll';
const metadataPath = 'DungeonVillageEXE/KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat';
const indexPath = 'work/persistence-replay-analysis/exe/methods.json';
const dumpPath = 'work/persistence-replay-analysis/exe/dumper/dump.cs';
const dll = read(dllPath), metadata = read(metadataPath);
if (sha(dll) !== '9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a' ||
    sha(metadata) !== '80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369')
  throw Error('固定 DLL / metadata 身份不符');
if (process.argv.length !== 2) throw Error('此工具无参数，只审计固定样本');
const ix = JSON.parse(read(indexPath)), dump = read(dumpPath).toString('utf8');
const iced = require(path.join(root, 'work/local-tools/iced-x86-1.21.0/package'));
// 独立读取 PE 文件段，不仅信任派生方法索引中的 offset。
const pe = dll.readUInt32LE(0x3c), optional = pe + 24;
if (dll.toString('ascii', pe, pe + 4) !== 'PE\0\0' || dll.readUInt16LE(optional) !== 0x10b)
  throw Error('需要固定 PE32');
const imageBase = dll.readUInt32LE(optional + 28), sections = [];
for (let n = 0, p = optional + dll.readUInt16LE(pe + 20); n < dll.readUInt16LE(pe + 6); ++n, p += 40)
  sections.push({rva: dll.readUInt32LE(p + 12), rawSize: dll.readUInt32LE(p + 16), raw: dll.readUInt32LE(p + 20)});
const specs = {
  wrapper: {rva: 0x2db540, bytes: 48, signature: 'public Quest GenerateQuest(int quest_type) { }',
    sha256: 'b6a894e1f6a5732fc8558838248bcf03ed917cca2370817fa1d35a2821142cd5'},
  select: {rva: 0x2ddd80, bytes: 3616, signature: 'public QuestData GetStartQuest(int quest_type) { }',
    sha256: '0909dbc920a01d83363fe341d4b2a90fc8af762c63e7ecb8d5aab66334be6e67'},
};
const expected = {
  wrapper: {
    '102db54b': 'call 102DDD80h', '102db552': 'push eax', '102db559': 'call 102DB570h',
  },
  select: {
    '102dde9d': 'cmp dword ptr [eax+0Ch],1', '102ddea1': 'jne short 102DDF1Eh',
    '102ddea3': 'cmp dword ptr [ebp+0Ch],1', '102ddea7': 'jne short 102DDF1Eh',
    '102ddec1': 'mov ecx,[eax+8]', '102dded2': 'push dword ptr [eax+10h]',
    '102dded6': 'call 10650690h', '102ddf17': 'mov eax,edx', '102ddf1d': 'ret',
    '102ddfb3': 'mov eax,[esi+2Ch]', '102ddfb6': 'cmp eax,[edx+60h]',
    '102ddfb9': 'jne short 102DDFEFh', '102ddfd7': 'push 2', '102ddfdc': 'call 10837C30h',
    '102ddfec': 'cmovne ebx,esi', '102ddfef': 'inc edi',
    '102ddfff': 'cmp dword ptr [edi+84h],1F4h', '102de009': 'jl 102DE3E8h',
    '102de00f': 'cmp esi,1', '102de012': 'jne 102DE3E8h',
    '102de050': 'mov ecx,[ebx+40h]', '102de068': 'cmp dword ptr [eax+5Ch],0',
    '102de06c': 'jle 102DE3DFh', '102de13f': 'push 2', '102de144': 'call 10837C30h',
    '102de14e': 'je short 102DE162h', '102de15a': 'call 10650450h',
    '102de175': 'call 10653220h', '102de209': 'mov eax,[eax+5Ch]',
    '102de20e': 'mov [ebp-8],eax', '102de2c9': 'cmp [eax+5Ch],ecx',
    '102de2cc': 'jle short 102DE2D1h', '102de2ce': 'mov [eax+5Ch],ecx',
    '102de2db': 'mov dword ptr [ebp-4],7FFFFFFFh',
    '102de3a3': 'mov eax,[eax+5Ch]', '102de3a6': 'cmp eax,[ebp-4]',
    '102de3a9': 'jge short 102DE3B1h', '102de3ab': 'mov [ebp-4],eax',
    '102de3ae': 'mov [ebp-8],edi', '102de3be': 'call 10650690h',
    '102de3e1': 'mov eax,ebx', '102de3e7': 'ret',
    '102de3e8': 'test esi,esi', '102de3ea': 'je 102DE869h',
    '102de406': 'push 64h', '102de408': 'call 102A44E0h',
    '102de410': 'cmp eax,28h', '102de413': 'jge 102DE6AEh',
    '102de441': 'push 3Ch', '102de444': 'call 10249BC0h', '102de44e': 'je 102DE6AEh',
    '102de4e6': 'mov eax,[eax+14h]', '102de502': 'sub edi,1', '102de505': 'js 102DE64Ah',
    '102de583': 'mov eax,[esi+2Ch]', '102de586': 'cmp eax,[ecx+60h]',
    '102de589': 'jg 102DE641h', '102de5dc': 'cmp byte ptr [eax+68h],0',
    '102de5e0': 'je short 102DE641h', '102de60f': 'call 10650450h',
    '102de63c': 'cmp eax,6', '102de63f': 'jge short 102DE64Ah',
    '102de641': 'sub edi,1', '102de644': 'jns 102DE510h',
    '102de683': 'test eax,eax', '102de685': 'jle short 102DE6AEh', '102de6a9': 'jmp 102DEB13h',
    '102de7b4': 'mov eax,[esi+2Ch]', '102de7b7': 'cmp eax,[edx+60h]',
    '102de7ba': 'jne 102DE85Ch', '102de7c3': 'cmp [esi+24h],eax', '102de7c6': 'jne 102DE85Ch',
    '102de7e5': 'push 2', '102de7e8': 'call 10837C30h', '102de7f2': 'jne short 102DE856h',
    '102de80d': 'push 4', '102de810': 'call 10837C30h', '102de81a': 'jne short 102DE856h',
    '102de84e': 'call 10650450h', '102de85c': 'inc edi',
    '102de8e2': 'mov edi,[edi+5Ch]', '102de8fc': 'mov esi,[eax+0D0h]',
    '102de91e': 'mov eax,[esi+0Ch]', '102de921': 'dec eax', '102de926': 'call 1024A000h',
    '102de94e': 'mov esi,[ecx+eax*4+10h]', '102de968': 'push 64h',
    '102de96a': 'call 102A44E0h', '102de975': 'cmp eax,esi', '102de97c': 'cmovl ecx,edx',
    '102dea15': 'cmp eax,[edx+60h]', '102dea18': 'jne 102DEAD5h',
    '102dea1e': 'cmp dword ptr [esi+24h],0', '102dea22': 'jne 102DEAD5h',
    '102dea41': 'push 2', '102dea44': 'call 10837C30h', '102dea4e': 'jne 102DEACFh',
    '102dea6d': 'push 8', '102dea70': 'call 10837C30h', '102dea7a': 'jne short 102DEAC9h',
    '102deaae': 'call 10650450h', '102deac9': 'cmp byte ptr [ebp-8],0',
    '102deacd': 'jne short 102DEAE2h', '102deae6': 'mov dword ptr [eax+5Ch],0',
    '102deaed': 'mov eax,esi', '102deaf4': 'ret',
    '102deb1e': 'call 106573A0h', '102deb23': 'mov esi,eax',
    '102deb3e': 'push esi', '102deb3f': 'call 102A44E0h', '102deb4c': 'call 10650690h',
  },
};
const methods = {}, anchors = [], branches = [];
for (const [key, spec] of Object.entries(specs)) {
  const m = ix.methods.find(v => v.rva === spec.rva && v.type === 'game.UserData' && v.signature === spec.signature);
  const end = Math.min(...ix.methods.filter(v => v.rva > spec.rva).map(v => v.rva));
  const section = sections.find(s => spec.rva >= s.rva && end <= s.rva + s.rawSize);
  if (!m || !section || end - spec.rva !== spec.bytes) throw Error('具名方法/范围 ' + key);
  const offset = section.raw + spec.rva - section.rva, va = imageBase + spec.rva;
  if (offset !== m.offset || va !== m.va || sha(dll.subarray(offset, offset + spec.bytes)) !== spec.sha256)
    throw Error('方法字节身份 ' + key);
  const d = new iced.Decoder(32, dll.subarray(offset, offset + spec.bytes), iced.DecoderOptions.None);
  const f = new iced.Formatter(iced.FormatterSyntax.Intel), lines = new Map(), jumps = [];
  d.ip = BigInt(va);
  try {
    while (d.canDecode) {
      const i = d.decode();
      try {
        if (i.isInvalid) throw Error('无效指令');
        const address = Number(i.ip), instruction = f.format(i);
        lines.set(address, instruction);
        if (i.isJccShortOrNear || i.isJmpShortOrNear)
          jumps.push({va: address, target: Number(i.nearBranchTarget), instruction});
      } finally { i.free(); }
    }
  } finally { d.free(); f.free(); }
  for (const b of jumps) {
    if (!lines.has(b.target)) throw Error('方法内跳转未落在指令边界 ' + b.va.toString(16));
    branches.push({method: key, va: '0x' + b.va.toString(16), target: '0x' + b.target.toString(16)});
  }
  for (const [address, instruction] of Object.entries(expected[key])) {
    if (lines.get(parseInt(address, 16)) !== instruction) throw Error('指令锚 ' + address);
    anchors.push({method: key, va: '0x' + address, instruction});
  }
  methods[key] = {...spec, offset, va, instructions: lines.size, direct_branches: jumps.length};
}
if (branches.length < 100) throw Error('跳转枚举异常');
const fields = {
  QuestData: ['public int type_; // 0x24', 'public int appearRank_; // 0x2C',
    'public int monster_bossData_; // 0x40', 'public static Vector BOSS_QUESTS; // 0x8',
    'public static int CH_MODE; // 0xC', 'public static int CH_INDEX; // 0x10',
    'public static Vector MANYMONSTER_QUESTS; // 0x14'],
  UserData: ['public int questDungeonCnt_; // 0x5C', 'public int questRank_; // 0x60',
    'public int questPoint_; // 0x84', 'private static Vector questList; // 0xC8',
    'private static Vector manyMonsterQuestList; // 0xCC',
    'private static readonly int[] QUESTDUNGEON_RARE_RATE; // 0xD0'],
  MonsterData: ['public int dieCnt_total_; // 0x5C', 'public bool isAppearWindow_; // 0x68'],
  AppData: ['public MonsterData[] monsterData_; // 0x11C', 'public QuestData[] questData_; // 0x138'],
};
for (const [type, entries] of Object.entries(fields)) {
  const begin = dump.indexOf('public class ' + type + ' '), end = dump.indexOf('\n// Namespace:', begin + 1);
  if (begin < 0 || end < 0) throw Error('类型边界 ' + type);
  const body = dump.slice(begin, end);
  for (const entry of entries) if (!body.includes(entry)) throw Error('字段 ' + type + ' ' + entry);
}
// 只具名核定被调用入口，未据此认证这些方法的内部算法。
const calls = [
  [0x2db570, 'game.UserData', 'public Quest GenerateQuest(int quest_type, QuestData questData) { }'],
  [0x2a44e0, 'game.GameUtil', 'public static int Random(int n) { }'],
  [0x24a000, 'game.GameUtil', 'public static int ClampMax(int val, int max) { }'],
  [0x249bc0, 'main.AppData', 'public bool CheckAlreadyEvent(int id) { }'],
  [0x837c30, 'kairo.unity.util.BitUtil', 'public static bool Check(int v, int bit) { }'],
].map(([rva, type, signature]) => {
  if (!ix.methods.some(m => m.rva === rva && m.type === type && m.signature === signature))
    throw Error('具名调用 ' + signature);
  return {rva, type, signature};
});
const result = {
  scope: '固定 Steam GetStartQuest 选择器及 GenerateQuest 单参 wrapper；APK 对照不充当 Steam 数据数组证明',
  sources: [dllPath, metadataPath, indexPath, dumpPath].map(p => { const b = read(p); return {path: p, bytes: b.length, sha256: sha(b)}; }),
  methods, calls, fields, anchors, branches,
  contracts: {
    selector_then_creator: true,
    explicit_mode: 'CH_MODE==1 && kind==1 -> BOSS_QUESTS[CH_INDEX]',
    default_boss: 'questPoint>=500 && kind==1；当前chapter原序最后flags2；未胜直接返回',
    repeat_boss: '全定义flags2原序；首项dieCnt_total作cap并真实写回其他超限怪物；严格最小，平局保原序',
    exploration: 'QUESTDUNGEON_RARE_RATE[ClampMax(questDungeonCnt,length-1)]；Random(100)；同chapter/type0排除flags2，flags8命中清questDungeonCnt后直接返回',
    recurrence: '非kind0回退分支先Random(100)<40，再event60；逆序MANYMONSTER，chapter<=当前且isAppearWindow，最多6；空池回普通',
    ordinary: '同chapter、同type、排除flags2/4；按池size随机选取',
    side_effects: ['临时选择池清空/追加', '重复BOSS分支降低超cap怪物dieCnt_total', '稀有探索命中清questDungeonCnt'],
  },
  limitations: [
    '未认证 Steam 稀有概率数组实际元素、特殊/复发目录初始化全值，不能把 APK 数组直接标成 Steam 事实',
    '非kind0分支本身不验证kind==1；公开合法kind范围及非法输入保护不由这段原函数证明',
    '仅核选择器及 wrapper；双参 GenerateQuest 难度/地点/创建、Random 算法及 ClampMax 内部本批未审计',
    '具名索引与字段来自本地 IL2CPP 派生输出，记录其hash；原 DLL/metadata 和方法字节身份单独固定',
    '静态分支对照不代表原窗口、自然通关或所有外部调用路径已执行；原方法副作用不具维护Owner事务承诺',
  ],
};
const output = path.join(root, 'work/steam-task-selection/EVIDENCE.json');
const encoded = JSON.stringify(result, null, 2) + '\n';
if (Buffer.byteLength(encoded) > 96 * 1024) throw Error('输出预算');
fs.mkdirSync(path.dirname(output), {recursive: true});
fs.writeFileSync(output, encoded, 'utf8');
console.log(JSON.stringify({methods: Object.keys(methods).length, anchors: anchors.length, branches: branches.length,
  fields: Object.values(fields).flat().length, output_bytes: Buffer.byteLength(encoded), output_sha256: sha(Buffer.from(encoded))}));
