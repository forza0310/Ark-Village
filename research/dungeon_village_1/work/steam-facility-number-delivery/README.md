# Steam设施数字helper请求展开

2026-10-10，接续`1b9db07`。依据[原helper合同](../../ui/STEAM_FACILITY_DRAW_HELPERS.md)，在现有`steam_facility_skin`模块增加`steam_facility_number_draws`，不新增target／Owner／格式或素材。

普通数字步宽由调用方提供实际SEB frame0/line0的SP_W，padding是间隔，anchor先bit2后bit4；0至少一位，负值保持一位负商请求。金额／加号按signed串及固定8绘制，原逗号在后一个数字之后绘制；金额先dx−9、同锚货币frame20，加号按GetFig绝对位数定位且自身不因0省略。81的差0隐藏继续由外层计划控制。

接口值域为维护属性实际使用的int32；不推广LONG_MIN原取负溢出。负frame请求保留原事实，不画一个猜测的减号，不将末端SEB行为认证为已完成。输入资源仅允许三套具名数字，缺步宽、坏padding或坐标溢出明确返回空，失败不返回半串图元。Mapchip2／文字后端／原窗口不在本批完成范围。

单Release目标构建通过，既有visuals通过0.58秒。扩原职责文件，用独立请求序列核anchor6居中优先、1234567逗号覆盖顺序、dx−9、零加号、负普通商／负signed串差别、缺宽／坏资源／溢出拒绝；没有重跑世界长轨迹。原素材复用，没有文件或后台状态；返回向量按十进制位数有界，调用方消费后释放，不推及世界历史永久有界。
