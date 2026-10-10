#include "ark/simulation/application/startup_title_menu.hpp"
#include <limits>
#include <utility>

namespace ark::simulation {
namespace {
using State=StartupTitleMenuState;
using Context=StartupTitleMenuContext;
using Candidate=StartupTitleMenuCandidate;
using Request=StartupTitleMenuRequest;
using Kind=StartupTitleMenuRequestKind;
using Intent=StartupTitleIntentKind;
bool valid_mode(StartupTitleRootMode mode) {
    return mode==StartupTitleRootMode::menu || mode==StartupTitleRootMode::slots;
}
bool equal(const StartupTitleCatalogStamp &a,const StartupTitleCatalogStamp &b) {
    return a.revision==b.revision && a.digest==b.digest;
}
bool no_keys(const StartupTitleMenuKeys &k) {
    return !k.left && !k.right && !k.up && !k.down && !k.confirm && !k.back;
}
bool valid_id(std::uint64_t id,const State &s) { return id>s.root_id && id<s.next_id; }
bool allocate(State &s,std::uint64_t &id) {
    if(s.next_id==std::numeric_limits<std::uint64_t>::max()) return false;
    id=s.next_id++;
    return true;
}
std::uint64_t top_id(const State &s) {
    if(s.confirmation && !s.confirmation->returned) return s.confirmation->id;
    if(s.save_menu && !s.save_menu->returned) return s.save_menu->id;
    if(s.external && !s.external->returned) return s.external->id;
    return s.root_id;
}
bool pending_return(const State &s) {
    return (s.confirmation && s.confirmation->returned) || (s.save_menu && s.save_menu->returned) ||
        (s.external && s.external->returned);
}
std::string open_external(Candidate &c,const Context &context,StartupTitleExternalPage kind) {
    if(kind==StartupTitleExternalPage::configure && !context.catalog) return "配置入口缺目录身份";
    std::uint64_t id{};
    if(!allocate(c.state,id)) return "标题页面序号耗尽";
    c.state.external=StartupTitleExternal{id,c.state.root_id,kind,false,false};
    c.intent=StartupTitleMenuIntent{kind==StartupTitleExternalPage::records?Intent::open_records:Intent::open_configuration,
        c.slot,c.state.row,id,context.catalog};
    return {};
}
std::string consume_return(Candidate &c,const Context &context) {
    auto &s=c.state;
    if(s.confirmation && s.confirmation->returned) {
        const int answer=s.confirmation->result;
        s.confirmation.reset();
        // 原raw20保留否决后的result1/2；但只有返回标记才允许标题读取它。
        if(answer==0) s.save_menu->returned=true;
        return {};
    }
    if(s.save_menu && s.save_menu->returned) {
        const auto menu=*s.save_menu;
        s.save_menu.reset();
        if(menu.result==1) return open_external(c,context,StartupTitleExternalPage::configure);
        if(menu.result==0 || menu.result==2)
            c.intent=StartupTitleMenuIntent{menu.result==0?Intent::load_record:Intent::hide_record,
                menu.slot,1,menu.id,menu.catalog};
        return {};
    }
    if(s.external && s.external->returned) {
        const auto page=*s.external;
        s.external.reset();
        if(page.completed) {
            if(!context.catalog) return "开始游戏缺当前目录身份";
            c.intent=StartupTitleMenuIntent{Intent::start_game,c.slot,1,page.id,context.catalog};
        }
        return {};
    }
    return "没有可消费的标题子页结果";
}
std::string confirm_menu(Candidate &c) {
    auto &s=c.state;
    auto &menu=*s.save_menu;
    menu.result=menu.selection;
    if(menu.selection==0) {
        menu.returned=true;
        return {};
    }
    std::uint64_t id{};
    if(!allocate(s,id)) return "标题页面序号耗尽";
    s.confirmation=StartupTitleConfirmation{id,menu.id,menu.selection==1?
        StartupTitleConfirmationReason::restart:StartupTitleConfirmationReason::hide,1,-1,false};
    return {};
}
std::string keys(Candidate &c,const Context &context,const StartupTitleMenuKeys &k) {
    auto &s=c.state;
    if(s.confirmation) {
        auto &dialog=*s.confirmation;
        // Steam SubForm.Update 0x10328261：right else-if left else-if confirm else-if back。
        if(k.right) dialog.selection=(dialog.selection+1)%2;
        else if(k.left) dialog.selection=(dialog.selection+1)%2;
        else if(k.confirm) { dialog.result=dialog.selection; dialog.returned=true; }
        else if(k.back) { dialog.result=-1; dialog.returned=true; }
        return {};
    }
    if(s.save_menu) {
        auto &menu=*s.save_menu;
        // FrameMenu实际早返回：上下不是标题的两个独立if；左右不充当raw20确认/返回。
        if(k.up) menu.selection=(menu.selection+2)%3;
        else if(k.down) menu.selection=(menu.selection+1)%3;
        else if(k.confirm) return confirm_menu(c);
        else if(k.back) { menu.result=-1; menu.returned=true; }
        return {};
    }
    if(s.external) return "纪录或配置需独立页面消费者";
    if(s.mode==StartupTitleRootMode::menu) {
        if(k.up) s.selection=(s.selection+1)%2;
        if(k.down) s.selection=(s.selection+1)%2;
        if(k.confirm) {
            if(s.selection==0) s.mode=StartupTitleRootMode::slots;
            else return open_external(c,context,StartupTitleExternalPage::records);
        }
        return {};
    }
    if(k.left) c.slot=(c.slot+1)%2;
    if(k.right) c.slot=(c.slot+1)%2;
    if(k.up) s.row=(s.row+1)%2;
    if(k.down) s.row=(s.row+1)%2;
    if(s.row==0 && !context.present[c.slot][0]) s.row=1;
    if(k.confirm) {
        if(!context.catalog) return "选档确认缺目录身份";
        if(s.row==0) {
            c.intent=StartupTitleMenuIntent{Intent::load_record,c.slot,0,s.root_id,context.catalog};
        } else if(context.present[c.slot][1]) {
            std::uint64_t id{};
            if(!allocate(s,id)) return "标题页面序号耗尽";
            s.save_menu=StartupTitleSaveMenu{id,s.root_id,c.slot,0,0,-1,false,context.catalog};
        } else return open_external(c,context,StartupTitleExternalPage::configure);
    } else if(k.back) s.mode=StartupTitleRootMode::menu;
    return {};
}
std::string touch(Candidate &c,const Context &context,const Request &r) {
    auto &s=c.state;
    int *selection{};
    int component{}, count{};
    bool title_menu{};
    if(s.confirmation) { selection=&s.confirmation->selection; component=3; count=2; }
    else if(s.save_menu) { selection=&s.save_menu->selection; component=9; count=3; }
    else if(s.external) return "外部子页触摸不是标题消费者";
    else if(s.mode==StartupTitleRootMode::menu) {
        selection=&s.selection; component=0; count=2; title_menu=true;
    } else { selection=&s.row; component=3; count=2; }
    if(r.component!=component || r.value<0 || r.value>=count) return "标题触摸组件或选择越界";
    if(!s.save_menu && !s.confirmation && s.mode==StartupTitleRootMode::slots &&
        r.value==0 && !context.present[c.slot][0]) return "空中断行没有触摸组件";
    // 标题id0的UP只发确认，ENTER才写选择；id3/id9的UP先写选择再确认。
    if(!title_menu || r.kind==Kind::touch_enter) *selection=r.value;
    if(r.kind==Kind::touch_up) {
        StartupTitleMenuKeys input;
        input.confirm=true;
        return keys(c,context,input);
    }
    return {};
}
}
std::string validate_startup_title_menu(const State &s) {
    if(!valid_mode(s.mode) || s.selection<0 || s.selection>1 || s.row<0 || s.row>1 ||
       s.root_id==0 || s.next_id<=s.root_id) return "标题根模式、选择或序号非法";
    if(s.save_menu && s.external) return "标题同时持有两个互斥子页";
    if(s.confirmation && !s.save_menu) return "询问缺raw20父页";
    if(s.save_menu) {
        const auto &m=*s.save_menu;
        if(s.mode!=StartupTitleRootMode::slots || s.row!=1 || !valid_id(m.id,s) ||
           m.parent!=s.root_id || m.slot<0 || m.slot>1 || m.selection<0 || m.selection>2 ||
           m.frame<0 || m.frame>3 || m.result < -1 || m.result>2 || !m.catalog ||
           (!m.returned && m.result==0) || (m.returned && m.result>=0 && m.result!=m.selection))
            return "raw20载荷或父关系非法";
        if(s.confirmation) {
            const auto &d=*s.confirmation;
            const int expected=d.reason==StartupTitleConfirmationReason::restart?1:
                d.reason==StartupTitleConfirmationReason::hide?2:0;
            if(!expected || !valid_id(d.id,s) || d.id<=m.id || d.parent!=m.id || m.returned ||
               m.result!=expected || m.selection!=expected || d.selection<0 || d.selection>1 ||
               d.result < -1 || d.result>1 || (!d.returned && d.result!=-1) ||
               (d.returned && d.result>=0 && d.result!=d.selection)) return "raw1原因、答案或父关系非法";
        }
    }
    if(s.external) {
        const auto &e=*s.external;
        if(!valid_id(e.id,s) || e.parent!=s.root_id ||
           (e.kind!=StartupTitleExternalPage::records && e.kind!=StartupTitleExternalPage::configure) ||
           (!e.returned && e.completed) || (e.kind==StartupTitleExternalPage::records && e.completed) ||
           (e.kind==StartupTitleExternalPage::records && (s.mode!=StartupTitleRootMode::menu || s.selection!=1)) ||
           (e.kind==StartupTitleExternalPage::configure && (s.mode!=StartupTitleRootMode::slots || s.row!=1)))
            return "外部标题子页载荷非法";
    }
    return {};
}
std::uint64_t startup_title_menu_top_id(const State &s) {
    return validate_startup_title_menu(s).empty()?top_id(s):0;
}
StartupTitleMenuResult prepare_startup_title_menu(const State &s,const Context &context,
    std::uint64_t expected_top_id,const Request &r) {
    const auto invalid=validate_startup_title_menu(s);
    if(!invalid.empty()) return {invalid,{}};
    if(context.slot<0 || context.slot>1) return {"应用唯一档位非法",{}};
    if(expected_top_id!=top_id(s)) return {"标题输入不是当前有效栈顶",{}};
    if(r.kind<Kind::keys || r.kind>Kind::advance_frame_menu) return {"标题请求种类未知",{}};
    const bool is_touch=r.kind==Kind::touch_enter || r.kind==Kind::touch_up;
    if((r.kind!=Kind::keys && !no_keys(r.keys)) || (!is_touch && (r.component!=0 || r.value!=0)) ||
       (r.kind!=Kind::return_external && r.completed)) return {"标题请求混入其它操作载荷",{}};
    if(s.save_menu && (context.slot!=s.save_menu->slot || !context.catalog ||
        !equal(*context.catalog,*s.save_menu->catalog) || !context.present[context.slot][1]))
        return {"raw20打开后的目录身份或档位已变化",{}};
    Candidate c{s,context.slot,{}};
    std::string error;
    if(r.kind==Kind::consume_return) error=consume_return(c,context);
    else if(pending_return(s)) error="标题子页结果必须先由父页具名消费";
    else if(r.kind==Kind::return_external) {
        if(!s.external) error="当前不是外部标题子页";
        else if(r.completed && s.external->kind!=StartupTitleExternalPage::configure)
            error="纪录返回不能请求开始游戏";
        else { c.state.external->returned=true; c.state.external->completed=r.completed; }
    } else if(r.kind==Kind::advance_frame_menu) {
        if(!s.save_menu || s.confirmation) error="FrameMenu准入不是活动raw20";
        else if(c.state.save_menu->frame<3) ++c.state.save_menu->frame;
    } else if(r.kind==Kind::keys) error=keys(c,context,r.keys);
    else error=touch(c,context,r);
    if(error.empty()) error=validate_startup_title_menu(c.state);
    if(!error.empty()) return {error,{}};
    return {{},std::move(c)};
}
} // namespace ark::simulation
