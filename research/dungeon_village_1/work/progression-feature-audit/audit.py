"""归档本批三个短功能套件的独立日志和研究源身份，不运行或改写世界。"""
from pathlib import Path
import hashlib
import json
import re

BASE = Path(__file__).resolve().parent
ROOT = BASE.parent.parent


def identity(path):
    data = path.read_bytes()
    return dict(path=path.relative_to(ROOT).as_posix(), bytes=len(data), sha256=hashlib.sha256(data).hexdigest())


def log_text(path):
    data = path.read_bytes()
    return data.decode('utf-16' if data[:2] in (b'\xff\xfe', b'\xfe\xff') else 'utf-8-sig')


checks = []
for name, log in [('startup_world_pages', 'pages-verified.log'),
                  ('startup_world_building', 'high-stars.log'),
                  ('startup_world_runtime', 'runtime-final.log')]:
    file = BASE / log
    text = log_text(file)
    found = re.search(r'dungeon_village_prototype\.' + name + r'\s+\.+\s+Passed\s+([\d.]+) sec', text)
    if not found or '100% tests passed' not in text:
        raise ValueError('本批独立结果不完整 ' + name)
    checks.append(dict(case='dungeon_village_prototype.' + name,
                       seconds=float(found[1]), evidence=identity(file)))
files = [
    'prototype/tests/startup_world_pages_test.cpp',
    'prototype/tests/startup_world_building_test.cpp',
    'prototype/tests/startup_world_runtime_test.cpp',
    'rules/PROGRESSION_ROUTES.md', 'rules/STEAM_MONSTER_PROGRESSION.md',
    'work/progression-feature-audit/README.md', 'work/progression-feature-audit/MAGIC_POT.md',
    'work/progression-feature-audit/HIGH_STARS.md', 'work/progression-feature-audit/audit.py',
    'work/steam-monster-progression/EVIDENCE.json'
]
static = json.loads((ROOT / files[-1]).read_text(encoding='utf-8'))
if len(static['anchors']) != 62:
    raise ValueError('Steam方法锚点缺失')
for record in static['sources']:
    if identity(ROOT / record['path']) != record:
        raise ValueError('Steam静态输入改变')
result = dict(
    qualification='明确条件夹具的功能组合，不是自然高星或原Steam动态',
    checks=checks, files=[identity(ROOT / file) for file in files],
    source_contracts=['APK1.0.8规则基线', 'Steam固定DLL普通怪物开放及介绍局部交叉'],
    tested_chains=['二星48→43→95活动30→51/52/53类型6→主菜单41',
                   '44延迟3→学校63领取；真实63建造完工→活动7',
                   '45延迟30→活动26领取；第二扩张沿既有精确功能用例',
                   '四星85博物馆64目录/199与200点→93领取；真实64支付3000G建造完工→活动21',
                   '46延迟30→职业21/22实际Owner与AI共享状态开放'],
    failures_preserved=['pages.log保留首次具名raw11遗漏',
                        'pages-final.log受共享CTest LastTest覆盖，不用作pages验收证据；最终使用独立stdout pages-verified.log'],
    resources=dict(new_targets=0, new_assets=0, new_build_trees=0, new_world_snapshots=0,
                   ownership='各用例自持Owner，建筑实例数/首次事件/页载荷退休及声音消费按现职责检查',
                   history='未删除合法历史，不宣称永久有界'),
    processes='本批构建和短测试均已退出；无研究长跑或后台游戏',
    limitations=['新起点是明确条件夹具，不证明自然达到星级条件',
                 '产品连续经营/新解锁实际消费/存读档后继续仍独立验收',
                 '没有宣布81任务、31活动或全部图像消费者已穷尽'])
(BASE / 'VALIDATION.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
print(json.dumps(dict(checks=len(checks), seconds=sum(c['seconds'] for c in checks), files=len(files))))
