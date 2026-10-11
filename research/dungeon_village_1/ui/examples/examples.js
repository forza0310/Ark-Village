// 离线说明型UI：按钮只切换示例，不调用Owner、不读存档、不执行任何业务结算。
'use strict';
const base='../../references/screenshots/2026-10-07-steam-restore/';
const samples={
 building:{title:'建筑详情',badge:'Steam S043 · 视觉适配示例',image:'facilities/S043-details-before.jpg',note:'原版设施详情 1/2。图中文字、490G／21／25／等级1／18人来自该帧。',contract:'<p>字段：设施定义名称／大图、实例价格／品质／魅力、共享等级、到下级剩余人数。第一页原名称不拼实例编号，第二页来源名才拼ordinal+1。右下返回与父场景底栏分离。</p><p>演示链：详情 → 使用道具 → 候选 → 结果 → 候选 → 父详情。本例不模拟消耗或数值增长；左右切页不新建子层。</p><p>木框与图块使用既有APK原资源。Steam已证局部绘制另见设施详情合同；本HTML的CSS字体、数字、手形、响应尺寸和物理热区属于适配，不代替正式C++资源计划。</p>'},
 human:{title:'人物详情',badge:'APK 字段示意 · Steam 布局待证',note:'无同身份 Steam 人物60截图。六属性、四战斗值与四装备槽来自已证APK合同；示例数值留空。',contract:'<p>六属性：体力、力量、灵活、结实、魔力、运气。四战斗值：最大HP、攻击、防御、魔法。当前HP应从实例单独读取。</p><p>入口：礼物、职业、武器、防具1、防具2、饰品、取消。点击详情不控制人物移动。</p><p>当前是产品字段／层级示例，布局、头像和原EXE操作尚未认证，不可直接标记“已还原”。</p>'},
 menu:{title:'主菜单与子菜单',badge:'Steam S042／S049 · 层级示例',image:'menu/S049-system-menu.jpg',note:'五行主菜单保留父级选中，系统子菜单展开在右侧。NEW条件未接，不画静态假标记。',contract:'<p>主菜单：建造／冒险／办公处／信息／系统；系统子菜单：保存／更改设定／游戏说明／纪录／结束游戏。</p><p>点击系统展开；保存只显示示例信息窗。Escape按层返回，其他入口展示路由提示。图标暂用占位符，不能作为原图映射。</p><p>本例点击即演示选中/展开；EXE列表标记确认与键盘／控制器完整语义见交互合同，不能从本例推广。</p>'},
 items:{title:'设施道具候选',badge:'Steam S044 · 列表示例',image:'items/S044-facility-item-list-before.jpg',note:'松软的坐垫、剩余2及价格/魅力“+”来自S044。第一次点击标记，第二次点击同项进入结果参考。',contract:'<p>列：名称／剩余。原75采用标记后确认语义，不能泛化为限时双击；本例无时间阈值。</p><p>“+”为定性预览，不解释为固定效果值。返回只退出本层；结果按钮不扣库存、不改父详情。</p>'},
 info:{title:'信息小窗',badge:'Steam S041 · 提示示例',image:'menu/S041-save-complete.jpg',note:'信息／保存完毕／好的。示例未写入任何存档。',contract:'<p>信息提示居中，白内容板承载短文本，橙色文字操作，返回后恢复父场景。</p><p>游戏内木色询问与窗口关闭蓝色确认属于不同路径，不能复用截图冒充同一原页面。</p>'}
};
// 第二页条件夹具复用固定APK原表第30／68／70／71／73／74行的身份与raw y。
// 序号仅用于演示同定义实例及五行窗口，不代表S043的邻接或自然经营结果。
const bonusDefinitions={
 30:{name:'武器店',icon:1,values:[['魅力',10]]},
 68:{name:'枞树',icon:6,values:[['价格',30],['品质',10]]},
 70:{name:'枫树',icon:6,values:[['价格',40],['品质',10]]},
 71:{name:'槙树',icon:6,values:[['价格',20],['品质',25]]},
 73:{name:'可可树',icon:6,values:[['价格',40],['品质',15]]},
 74:{name:'木灵大树',icon:6,values:[['价格',70],['品质',20]]}
};
const bonusFixtures={empty:[],neighbours:[[71,9],[71,10],[30,0]],long:[[71,9],[71,10],[30,0],[68,0],[70,0],[73,0],[74,0]]};
let selected='building', stack=[], marked=false, humanPage=0, menuSelection='建造';
let buildingPage=0, bonusFixture='neighbours', bonusSelection=0, bonusFirst=0;
const $=id=>document.getElementById(id);
const currentPage=()=>stack.at(-1)||selected;
function resetBuilding(){buildingPage=0;bonusFixture='neighbours';bonusSelection=0;bonusFirst=0;}
function sprite(){return '<img class="cafe" src="../../assets/original/image/t_cafe.png" alt="固定APK咖啡店原图">';}
function frame(title,body,action=''){return `<article class="window"><h3 class="window-title"><span>${title}</span></h3>${body}${action?`<div class="action-area">${action}</div>`:''}</article><button class="return-soft" data-action="back">返回</button>`;}
function building(){
 const title=`设施信息 ${buildingPage+1}/2`;
 let body,action;
 if(buildingPage===0){
  body=`<div class="name-row"><span class="sprite facility-icon" aria-hidden="true"></span>咖啡店<span class="price">价格 <b>490G</b></span></div><div class="white-panel"><div class="building-grid"><div class="building-art">${sprite()}<span class="level">等级 1</span></div><div><div class="stats"><div class="stat">品质<b>21</b></div><div class="stat">魅力<b>25</b></div></div><div class="heart" role="img" aria-label="爱心效果，一枚蓝色加号"><span class="effect-heart" aria-hidden="true"></span><span class="effect-plus" aria-hidden="true"></span></div></div></div><p class="remaining">距离下个等级还有<strong>18 人</strong></p></div>`;
  action='<button class="game-action" data-action="items">使用道具</button>';
 }else{
  const rows=bonusFixtures[bonusFixture];
  body=`<div class="bonus-heading"><span>设施奖励</span><span>维护费 <b>—</b></span></div><div class="white-panel bonus-panel"><div class="bonus-list" role="listbox" aria-label="邻接来源（条件夹具）">${rows.length?rows.slice(bonusFirst,bonusFirst+5).map(([id,ordinal],visible)=>{
   const index=bonusFirst+visible,definition=bonusDefinitions[id];
   return `<button class="bonus-row ${index===bonusSelection?'active':''}" role="option" aria-selected="${index===bonusSelection}" data-action="bonus-select" data-index="${index}"><span class="bonus-finger" aria-hidden="true"></span><span class="bonus-icon" style="background-position:${-(definition.icon%10)*32}px 0" aria-hidden="true"></span><span class="bonus-name">${definition.name}${ordinal+1}</span><span class="bonus-values">${definition.values.map(([label,value])=>`<span><span class="bonus-label">${label}</span>+${value}</span>`).join('')}</span></button>`;
  }).join(''):'<p class="bonus-empty">没有奖励</p>'}</div><div class="bonus-scroll"><button data-action="bonus-scroll" data-step="-1" aria-label="上一行" ${bonusFirst===0?'disabled':''}>▲</button><span>${rows.length?`${bonusFirst+1}–${Math.min(bonusFirst+5,rows.length)}`:'0'} / ${rows.length}</span><button data-action="bonus-scroll" data-step="1" aria-label="下一行" ${bonusFirst>=Math.max(rows.length-5,0)?'disabled':''}>▼</button></div></div>`;
  action='<span class="bonus-caption">周围设施的奖励</span>';
 }
 return frame(title,body,action).replace(`<span>${title}</span>`,`<button class="arrow" data-action="page" aria-label="上一页">◀</button><span>${title}</span><button class="arrow" data-action="page" aria-label="下一页">▶</button>`);
}
function selectBonus(index){
 const count=bonusFixtures[bonusFixture].length;
 if(!count)return;
 bonusSelection=((index%count)+count)%count;
 if(bonusSelection<bonusFirst)bonusFirst=bonusSelection;
 if(bonusSelection>=bonusFirst+5)bonusFirst=bonusSelection-4;
 render();
}
function scrollBonus(step){
 const count=bonusFixtures[bonusFixture].length;
 bonusFirst=Math.max(0,Math.min(bonusFirst+step,Math.max(count-5,0)));
 // 本示例滚动按钮只移动可见窗；选中项仍需显式选择，不执行列表确认。
 render();
}
function human(){const fields=[['最大HP','攻击','防御','魔法'],['体力','力量','灵活','结实','魔力','运气'],['武器','防具1','防具2','饰品'],['学会魔法','当前可用魔法']][humanPage];const rows=humanPage===2?`<div class="equipment">${fields.map(x=>`<button data-action="human-route" data-label="${x}">${x}　—</button>`).join('')}</div>`:`<div class="human-values">${fields.map(x=>`<div class="stat">${x}<b>—</b></div>`).join('')}</div>`;return frame(`冒险者情报 ${humanPage+1}/4`,`<div class="human-head"><div class="portrait" aria-label="头像待绑定">?</div><div>姓名 —<br><span class="muted">当前职业 — ／ 等级 —</span></div></div><div class="white-panel">${rows}<p class="muted">${humanPage===0?'当前 HP —　满足度 —　努力 —　勋章 —':'数值留空，等待真实Owner字段。'}</p></div>`,humanPage<2?`<button class="game-action" data-action="human-route" data-label="${humanPage===0?'礼物':'职业'}">${humanPage===0?'礼物':'转职'}</button>`:'').replace(`<span>冒险者情报 ${humanPage+1}/4</span>`,`<button class="arrow" data-action="human-page" data-step="-1" aria-label="人物上一页">◀</button><span>冒险者情报 ${humanPage+1}/4</span><button class="arrow" data-action="human-page" data-step="1" aria-label="人物下一页">▶</button>`);}
function menu(system=false){const labels=['建造','冒险','办公处','信息','系统'];return `<div class="menu-stack"><div class="menu-column">${labels.map((x,i)=>`<button class="menu-row ${(system?i===4:x===menuSelection)?'active':''}" data-action="menu" data-label="${x}"><span class="menu-symbol">${['▦','⚔','♜','▤','⚙'][i]}</span>${x}</button>`).join('')}</div>${system?`<div class="menu-column secondary">${['保存','更改设定','游戏说明','纪录','结束游戏'].map((x,i)=>`<button class="menu-row ${i===0?'active':''}" data-action="system" data-label="${x}">${x}</button>`).join('')}</div>`:''}</div><button class="return-soft" data-action="back">返回</button>`;}
function items(){return frame('设施强化',`<div class="white-panel"><div class="item-header"><span>名称</span><span>剩余</span></div><div class="item-row"><button data-action="mark">${marked?'☞ ':''}松软的坐垫</button><b>2</b></div><div class="preview"><span>价格 ＋</span><span>品质</span><span>魅力 ＋</span></div></div>`);}
function render(){const current=currentPage();const s=samples[current==='system'?'menu':current==='result'?'items':current];$('sample-title').textContent=s.title;$('qualification').textContent=s.badge;$('source-note').textContent=s.note;$('contract').innerHTML=s.contract;$('reference').hidden=!s.image;$('no-reference').hidden=!!s.image;$('no-reference').textContent='尚无同身份 Steam 人物详情截图。当前仅提供 APK 字段与层级示意，布局不能当作 EXE 精确还原。';$('reference-link').removeAttribute('href');if(s.image){$('reference').src=base+s.image;$('reference-link').href=base+s.image;}
 $('building-controls').hidden=current!=='building';$('bonus-fixture').value=bonusFixture;
 if(current==='building'&&buildingPage===1){
  $('qualification').textContent='Steam静态合同 · 条件夹具／CSS适配';
  $('source-note').textContent='第二页布局与原序五行来自Steam已核raw74合同；夹具身份、图标及raw y来自固定APK原表，未证明是S043咖啡店的实际邻接。维护费未提供，留空。';
  $('contract').innerHTML='<p>原序：槙树10、槙树11、武器店1；长列表再加入枞树1、枫树1、可可树1、木灵大树1。显示编号为原ordinal+1，与等级无关。植物显示原y[0]价格／y[1]品质，商店显示原y[0]魅力；没有汇总、乘等级或补道路来源。</p><p>左右切页循环，保留邻接选择与滚动；上下选择循环，并保持五行可见。第二页确认无操作，不使用道具、不打开来源建筑。返回属于父层恢复示意，未认证FormManager或OS输入。</p><p>图标按原类别9的16×16裁片绘制，CSS等比放大两倍。原Steam滚动helper只注册组件；本例右侧上下按钮／区间文字是浏览器适配，不冒充原滚动条。字体、缩放及物理热区另验。</p>';
  $('reference').hidden=false;$('no-reference').hidden=true;
  $('reference').src='../../references/screenshots/2026-10-11-steam-facility/S070-bonus-page2.jpg';
  $('reference-link').href=$('reference').src;
  $('source-note').textContent+=' 右栏S070是Steam2.56咖啡店的两条真实来源（马戏团广场1、罗汉松9），与左侧夹具分开；原图右蓝竖条不等于本例辅助按钮。';
 }
 $('stage').innerHTML=current==='building'?building():current==='human'?human():current==='menu'?menu():current==='system'?menu(true):current==='items'?items():current==='result'?frame('设施强化 · 结果参考','<div class="white-panel"><p class="message">成功!!</p><p class="muted">S045：价格510G（+20G）、品质21、魅力27（+2）。<br>这是原截图示例，本窗口未执行改良。</p></div>','<button class="game-action" data-action="back">好的</button>'):frame('信息','<div class="white-panel message">保存完毕</div>','<button class="game-action" data-action="back">好的</button>');
 if(current==='items'){$('reference').src=base+'items/S044-facility-item-list-before.jpg';$('reference-link').href=$('reference').src;}
 if(current==='result'){$('reference').src=base+'facilities/S045-reinforcement-result.jpg';$('reference-link').href=$('reference').src;$('sample-title').textContent='设施强化结果';$('source-note').textContent='S045原版结果参考；本例不模拟76演出和77计数49/55门槛。';}
 for(const b of $('samples').children)b.setAttribute('aria-pressed',String(b.dataset.sample===selected));}
function note(text){$('feedback').textContent=text;}
function back(){if(stack.length){stack.pop();render();note('已返回父层；示例未变更任何业务数据。');}else note('当前是示例根层。游戏中这里应返回来源页面或场景。');}
for(const [id,s]of Object.entries(samples)){const b=document.createElement('button');b.textContent=s.title;b.dataset.sample=id;b.addEventListener('click',()=>{selected=id;stack=[];marked=false;humanPage=0;menuSelection='建造';resetBuilding();render();note('已切换示例。字段与证据等级见右侧。');});$('samples').append(b);}
$('stage').addEventListener('click',event=>{const button=event.target.closest('[data-action]');if(!button)return;const {action,label}=button.dataset;
 if(action==='back'){back();return;}if(action==='items'){stack.push('items');marked=false;}
 else if(action==='mark'){if(marked)stack.push('result');else{marked=true;note('已标记。再次点击同项进入结果参考，无时间阈值。');}}
 else if(action==='menu'){menuSelection=label;if(label==='系统'){if(stack.at(-1)!=='system')stack.push('system');}else{if(stack.at(-1)==='system')stack.pop();render();note(`已选择${label}；该分支的后续窗口不在本例中。`);return;}}
 else if(action==='system'){if(label==='保存'){stack.push('info');note('仅显示保存提示样式，没有写档。');}else{note(`已选择${label}；对应原页面待后续示例补齐。`);return;}}
 else if(action==='page'){buildingPage=(buildingPage+1)%2;note('已切换设施页；邻接选择与可见窗保持。第二页是明确条件夹具。');}
 else if(action==='bonus-select'){selectBonus(Number(button.dataset.index));note('仅选择邻接来源；确认不会使用道具或打开该建筑。');return;}
 else if(action==='bonus-scroll'){scrollBonus(Number(button.dataset.step));return;}
 else if(action==='human-page'){humanPage=(humanPage+Number(button.dataset.step)+4)%4;}
 else if(action==='human-route'){note(`${label}：产品应提交已有Owner页面命令，并保留人物绑定；本例不创建候选数据。`);return;}render();});
$('bonus-fixture').addEventListener('change',()=>{bonusFixture=$('bonus-fixture').value;bonusSelection=0;bonusFirst=0;buildingPage=1;render();note('已切换第二页条件夹具；未更改Owner或S043来源数值。');});
$('back').addEventListener('click',back);$('reset').addEventListener('click',()=>{stack=[];marked=false;humanPage=0;menuSelection='建造';resetBuilding();render();note('示例已重置。');});$('width').addEventListener('change',()=>{$('viewport').classList.toggle('compact',$('width').value==='compact');});
document.addEventListener('keydown',e=>{
 if(e.key==='Escape'){e.preventDefault();back();return;}
 if(currentPage()!=='building'||e.target.closest('select,input,textarea')||e.altKey||e.ctrlKey||e.metaKey)return;
 if(e.key==='ArrowLeft'||e.key==='ArrowRight'){e.preventDefault();buildingPage=(buildingPage+1)%2;render();note('已切页，邻接选择与滚动保持。');}
 else if(buildingPage===0&&e.key==='Enter'){e.preventDefault();stack.push('items');marked=false;render();note('已进入道具候选层；示例未执行道具消费。');}
 else if(buildingPage===1&&['ArrowUp','ArrowDown','Home','End','Enter'].includes(e.key)){
  e.preventDefault();
  if(e.key==='Enter'){note('设施第2页确认无操作：不消费道具，不进入来源建筑。');return;}
  const index=e.key==='Home'?0:e.key==='End'?bonusFixtures[bonusFixture].length-1:bonusSelection+(e.key==='ArrowUp'?-1:1);
  selectBonus(index);
 }
});
$('stage').addEventListener('wheel',e=>{if(currentPage()==='building'&&buildingPage===1){e.preventDefault();if(e.deltaY)scrollBonus(Math.sign(e.deltaY));}},{passive:false});
render();note('点击“使用道具”查看父子窗口关系；左右键切设施两页，第二页条件可在下方选择。');
