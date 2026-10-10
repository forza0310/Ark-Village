#include "ark/simulation/application/startup_title_menu.hpp"
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

using namespace ark::simulation;
namespace {
using Kind=StartupTitleMenuRequestKind;
using Intent=StartupTitleIntentKind;
struct Checks {
    int count{};
    void operator()(bool ok,const char *message) {
        ++count;
        if(!ok) throw std::runtime_error(std::string("标题菜单控制器：")+message);
    }
};
StartupTitleMenuRequest input(bool left=false,bool right=false,bool up=false,bool down=false,
                              bool confirm=false,bool back=false) {
    StartupTitleMenuRequest r;
    r.keys={left,right,up,down,confirm,back};
    return r;
}
StartupTitleMenuRequest event(Kind kind,int component=0,int value=0) {
    StartupTitleMenuRequest r; r.kind=kind; r.component=component; r.value=value; return r;
}
struct Fixture {
    Checks &check;
    StartupTitleMenuState state;
    StartupTitleMenuContext context;
    explicit Fixture(Checks &c):check(c) {
        context.catalog=StartupTitleCatalogStamp{};
        context.catalog->revision=7;
        context.catalog->digest.fill(0x5a);
        context.present={{{false,true},{true,true}}};
    }
    std::optional<StartupTitleMenuIntent> apply(StartupTitleMenuRequest r) {
        auto result=prepare_startup_title_menu(state,context,startup_title_menu_top_id(state),r);
        if(!result.error.empty()) throw std::runtime_error("标题有效请求被拒："+result.error);
        check(result.candidate.has_value(),"成功请求有完整候选");
        auto c=std::move(*result.candidate);
        state=std::move(c.state); context.slot=c.slot;
        return c.intent;
    }
    void slots() { apply(input(false,false,false,false,true)); }
    void manual_menu() { slots(); apply(input(false,false,false,false,true)); }
    void reject(StartupTitleMenuRequest r,std::uint64_t id) {
        const auto result=prepare_startup_title_menu(state,context,id,r);
        check(!result.error.empty() && !result.candidate,"拒绝不返回部分候选或一次性意图");
    }
};
}

// 现有application套件主责纯控制器；四目录皆条件输入，不把这些菜单路径说成自然存档产生者。
int run_startup_title_menu_checks() {
    Checks check;
    {
        Fixture f(check);
        f.apply(input(false,false,true,true));
        check(f.state.selection==0,"标题上下独立if同轮翻两次");
        f.slots();
        check(f.state.mode==StartupTitleRootMode::slots && !f.state.external && !f.state.save_menu,
              "标题开始只进入选档，不创建配置/世界");
        f.apply(input(true,true,true,true));
        check(f.context.slot==0 && f.state.row==1,"选档四方向独立，双向均真不改变最后选择");
        f.apply(input(false,false,true));
        check(f.state.row==1,"空中断在导航后强制回手动");
        f.reject(event(Kind::touch_enter,3,0),f.state.root_id);
        f.apply(input(false,true,true));
        check(f.context.slot==1 && f.state.row==0,"切槽后按新槽中断资格判断");
        const auto load=f.apply(input(false,false,false,false,true,true));
        check(load && load->kind==Intent::load_record && load->slot==1 && load->row==0 &&
              f.state.mode==StartupTitleRootMode::slots,"中断直接请求读取，确认优先于同轮返回");
        f.apply(input(false,false,false,false,false,true));
        check(f.state.mode==StartupTitleRootMode::menu && f.context.slot==1 && f.state.row==0,
              "返回标题保留槽号/行号，不偷偷重初始化");
    }
    {
        Fixture f(check);
        // id0 UP本身不写选择；这里没有模拟ENTER，故仍确认原第0项。
        f.apply(event(Kind::touch_up,0,1));
        check(f.state.mode==StartupTitleRootMode::slots && f.state.selection==0,
              "标题UP仅发确认，不借事件value伪造先前ENTER");
        f.apply(input(false,false,false,false,false,true));
        f.apply(event(Kind::touch_enter,0,1));
        const auto records=f.apply(event(Kind::touch_up,0,1));
        check(records && records->kind==Intent::open_records && f.state.external &&
              f.state.external->kind==StartupTitleExternalPage::records,"ENTER选中再UP请求纪录子页");
        const auto retired=f.state.external->id;
        f.reject(input(),f.state.root_id);
        f.apply(event(Kind::return_external));
        f.reject(input(false,false,false,false,true),f.state.root_id);
        check(!f.apply(event(Kind::consume_return)) && !f.state.external && f.state.selection==1,
              "纪录返回结果独立消费，父选择保留且不重开纪录");
        f.reject(event(Kind::return_external),retired);
    }
    {
        Fixture f(check); f.manual_menu();
        const auto menu=f.state.save_menu->id;
        check(menu>f.state.root_id && f.state.save_menu->parent==f.state.root_id &&
              f.state.save_menu->selection==0 && f.state.save_menu->frame==0,
              "手动非空创建稳定raw20父子关系与初值");
        f.apply(input(false,false,true,true,true,true));
        check(f.state.save_menu->selection==2 && !f.state.confirmation &&
              f.state.save_menu->frame==0,"FrameMenu上优先下/确认/返回，导航不暗推计数");
        f.apply(input(true,true));
        check(f.state.save_menu->selection==2 && !f.state.save_menu->returned,
              "raw20左右不变成确认或返回快捷键");
        for(int i=0;i<4;++i) f.apply(event(Kind::advance_frame_menu));
        check(f.state.save_menu->frame==3,"具名FrameMenu内部计数请求加1并夹3");
        check(!f.apply(event(Kind::touch_up,9,1)) && f.state.confirmation &&
              f.state.confirmation->selection==1 && f.state.save_menu->result==1 &&
              !f.state.save_menu->returned,"重新开始只挂默认否询问，预存1不能当返回意图");
        const auto dialog=f.state.confirmation->id;
        f.reject(event(Kind::consume_return),menu);
        f.reject(event(Kind::advance_frame_menu),dialog);
        f.apply(input(false,false,false,false,true));
        check(f.state.confirmation->returned && f.state.confirmation->result==1 &&
              startup_title_menu_top_id(f.state)==menu,"否只标子返回，恢复实际父ID");
        f.reject(event(Kind::touch_up,3,0),dialog);
        check(!f.apply(event(Kind::consume_return)) && !f.state.confirmation &&
              f.state.save_menu->selection==1 && f.state.save_menu->result==1 &&
              !f.state.save_menu->returned,"父消费否决保留选择与预存值，不进入配置");
        f.apply(input(false,false,false,false,false,true));
        check(f.state.save_menu->returned && f.state.save_menu->result==-1,
              "raw20取消覆盖旧待返回结果，禁止否决后的1泄漏");
        check(!f.apply(event(Kind::consume_return)) && !f.state.save_menu,
              "标题消费取消不发新局/隐藏意图");
        f.apply(input(false,false,false,false,true));
        check(f.state.save_menu->id>dialog,"再次打开分配新ID，退休菜单ID永不重用");
        f.reject(input(false,false,false,false,true),menu);
    }
    for(int action: {1,2}) {
        Fixture f(check); f.manual_menu();
        f.apply(event(Kind::touch_up,9,action));
        const auto dialog=f.state.confirmation->id;
        f.apply(input(true,true,false,false,true,true));
        check(f.state.confirmation->selection==0 && !f.state.confirmation->returned,
              "raw1右优先左/确认/返回，不把多个键顺序全执行");
        f.apply(event(Kind::touch_up,3,0));
        check(f.state.confirmation->result==0 && !f.state.save_menu->returned,
              "是也不能在子回调直接消费父文件事务");
        check(!f.apply(event(Kind::consume_return)) && f.state.save_menu->returned,
              "父raw20消费是后才返回，未发最终标题意图");
        const auto result=f.apply(event(Kind::consume_return));
        check(result && result->kind==(action==1?Intent::open_configuration:Intent::hide_record) &&
              result->slot==0 && result->row==1 && result->catalog &&
              result->catalog->revision==7,"标题最终消费精确动作和目录身份");
        f.reject(event(Kind::touch_up,3,0),dialog);
        if(action==1) {
            const auto config=f.state.external->id;
            f.apply(event(Kind::return_external));
            check(!f.apply(event(Kind::consume_return)) && !f.state.external &&
                  f.state.mode==StartupTitleRootMode::slots,"配置取消回实际选档父页，不回标题初始化");
            f.reject(event(Kind::return_external),config);
        } else check(!f.state.save_menu && !f.state.external,"隐藏请求退休菜单但不假装修改Context目录");
    }
    {
        Fixture f(check); f.manual_menu();
        f.apply(event(Kind::touch_up,9,2));
        f.apply(input(false,false,false,false,false,true));
        check(f.state.confirmation->result==-1,"询问取消保留-1，不等同选择是");
        f.apply(event(Kind::consume_return));
        check(!f.state.confirmation && !f.state.save_menu->returned && f.state.save_menu->selection==2,
              "取消询问恢复raw20原选择");
        f.apply(event(Kind::touch_up,9,0));
        check(f.state.save_menu->returned && f.state.save_menu->result==0,"继续直接返回0，无额外询问");
        check(f.apply(event(Kind::consume_return))->kind==Intent::load_record,"标题消费继续才发加载意图");
    }
    {
        Fixture f(check); f.context.present[0][1]=false; f.slots();
        const auto config=f.apply(event(Kind::touch_up,3,1));
        check(config && config->kind==Intent::open_configuration && !f.state.save_menu,
              "手动空行直接配置，无覆盖询问");
        auto r=event(Kind::return_external); r.completed=true;
        check(!f.apply(r),"实际开始结果也交给唯一Owner联合提交");
        const auto start=f.apply(event(Kind::consume_return));
        check(start && start->kind==Intent::start_game && !f.state.external,"仅完成配置产生start意图");
    }
    {
        Fixture f(check); f.slots(); f.context.catalog.reset();
        f.reject(input(false,false,false,false,true),f.state.root_id);
    }
    {
        Fixture f(check); f.manual_menu();
        const auto id=f.state.save_menu->id;
        ++f.context.catalog->revision;
        f.reject(event(Kind::touch_up,9,0),id);
        --f.context.catalog->revision; f.context.catalog->digest[0]^=1;
        f.reject(event(Kind::touch_up,9,0),id);
        f.context.catalog.reset(); f.reject(input(),id);
        auto broken=f.state;
        broken.save_menu->catalog.reset();
        check(!validate_startup_title_menu(broken).empty() && startup_title_menu_top_id(broken)==0,
              "已初始化raw20缺目录载荷显式拒绝");
        broken=f.state; broken.save_menu->parent=999;
        check(!validate_startup_title_menu(broken).empty(),"错误父页拒绝");
        broken=f.state; broken.confirmation=StartupTitleConfirmation{};
        check(!validate_startup_title_menu(broken).empty(),"空初始化询问不能用默认值伪装有效载荷");
    }
    {
        Fixture f(check); f.slots(); f.state.next_id=std::numeric_limits<std::uint64_t>::max();
        f.reject(input(false,false,false,false,true),f.state.root_id);
        f.reject(event(Kind::touch_enter,3,2),f.state.root_id);
        auto mixed=input(false,false,false,false,true); mixed.kind=Kind::consume_return;
        f.reject(mixed,f.state.root_id);
        check(f.state.next_id==std::numeric_limits<std::uint64_t>::max() && !f.state.save_menu,
              "序号耗尽与坏输入不发布部分菜单");
    }
    {
        Fixture f(check); f.manual_menu();
        f.state.next_id=std::numeric_limits<std::uint64_t>::max();
        f.reject(event(Kind::touch_up,9,2),f.state.save_menu->id);
        check(f.state.save_menu->selection==0 && f.state.save_menu->result==-1 && !f.state.confirmation,
              "先选择/预存结果后子页分配失败也不能留下部分修改");
    }
    return check.count;
}
