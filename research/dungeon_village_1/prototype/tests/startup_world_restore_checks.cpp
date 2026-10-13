#include "startup_world_restore_checks.hpp"

#include "startup_world_restore_validation.hpp"
#include "startup_world_codec.hpp"
#include "dungeon_village_prototype/startup_world_persistence.hpp"
#include "dungeon_village_prototype/startup_world_building.hpp"
#include "dungeon_village_prototype/startup_world_facility_catalog.hpp"
#include "dungeon_village_prototype/startup_world_magic_pot.hpp"
#include "dungeon_village_prototype/startup_world_information.hpp"
#include "dungeon_village_prototype/startup_world_menu.hpp"
#include "dungeon_village_prototype/startup_world_save.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"
#include "dungeon_village_prototype/startup_world_commerce.hpp"
#include "dungeon_village_prototype/startup_world_facility_items.hpp"
#include "dungeon_village_prototype/startup_world_village_activity.hpp"
#include "dungeon_village_prototype/startup_world_runtime_tasks.hpp"
#include "support/world_fixture.hpp"

#include <algorithm>

#include <limits>
#include <stdexcept>

int check_startup_world_restore_contracts(
    const dungeon_village_prototype::StartupWorldRuntimeState &baseline) {
    namespace p = dungeon_village_prototype;
    namespace r = dungeon_village_reference;
    int checks{};
    const auto expect = [&](const p::StartupWorldRuntimeState &state, bool valid,
                            const char *scenario) {
        std::string reason;
        ++checks;
        if (p::persistence_detail::validate_restored_state(state, reason) != valid)
            throw std::runtime_error(std::string("restore fixture ") + scenario + ": " + reason);
    };
    expect(baseline, true, "natural baseline");
    {
        // 原工厂与页面producer负责实体/载荷，仅人数资格和演出边界使用条件夹具。
        using E=p::StartupWorldRuntimeError;using A=p::StartupWorldTaskAction;
        const auto require=[&](bool ok,const char *why) {++checks;if(!ok)throw std::runtime_error(std::string("restore task input: ")+why);};
        auto tasks=baseline;tasks.human_presence.at(1)=1;
        const auto made=r::prepare_world_task_creation(p::startup_world_runtime_factory(tasks),0);
        require(made.candidate&&made.candidate->created_task&&p::write_startup_world_runtime_factory(tasks,made.candidate->state),"real factory");
        const auto task=*made.candidate->created_task;
        tasks.task_order.assign(7,task);
        require(p::open_startup_world_runtime_task_menu(tasks)==E::none,"open22");
        const auto id=tasks.scripts.pages.back().id;
        tasks.scripts.pages.back().lifecycle=2; // DTO作用于已获准轮，不以待Init producer代替。
        tasks.page_phases.at(id)=6;tasks.page_secondary_counters.at(id)=6;
        expect(tasks,true,"22 legal short-tail producer state");
        const auto reject=[&](const auto &source,const char *why,const auto &damage) {auto bad=source;damage(bad);expect(bad,false,why);};
        reject(tasks,"22 missing selection",[&](auto &s){s.page_phases.erase(id);});
        reject(tasks,"22 missing first",[&](auto &s){s.page_secondary_counters.erase(id);});
        reject(tasks,"22 selected offscreen",[&](auto &s){s.page_phases.at(id)=0;});
        const auto roundtrip=[&](const auto &source,const auto &consume,const char *why) {
            expect(source,true,why);const auto bytes=p::persistence_detail::encode_state(source);
            auto restored=p::persistence_detail::decode_state(bytes,*source.rules),live=source;
            require(p::persistence_detail::encode_state(restored)==bytes,"exact task state roundtrip");
            consume(live);consume(restored);
            expect(live,true,why);expect(restored,true,why);
            require(p::persistence_detail::encode_state(live)==p::persistence_detail::encode_state(restored),
                    "same task input preserves common random, counters and outputs");
        };
        p::StartupTaskPageInput input;input.up=input.down=input.confirm=true;
        const auto choose=[&](auto &s){require(p::input_startup_world_runtime_task_page(s,id,input).error==E::none,"22 combined confirm");};
        roundtrip(tasks,choose,"22 same-row confirmation to23 restore");choose(tasks);
        const auto offer=tasks.scripts.pages.back().id;
        tasks.scripts.pages.back().lifecycle=2;
        input={};input.right=true;
        require(p::input_startup_world_runtime_task_page(tasks,offer,input).error==E::none,"23 stored option1");
        reject(tasks,"23 spurious secondary",[&](auto &s){s.page_secondary_counters[offer]=0;});
        auto cancelled=tasks;require(p::act_startup_world_runtime_task_page(cancelled,offer,A::cancel).error==E::none,"23 actual cancel");
        expect(cancelled,true,"closed23 without invented secondary remains restorable");
        reject(cancelled,"closed23 missing selection",[&](auto &s){s.page_phases.erase(offer);});
        input={};input.confirm=true;
        const auto recruit=[&](auto &s){const auto result=p::input_startup_world_runtime_task_page(s,offer,input);require(result.error==E::none&&result.accepted,"23 paid recruitment");};
        roundtrip(tasks,recruit,"23 option1 confirmation still creates24");recruit(tasks);
        const auto animation=tasks.scripts.pages.back().id;
        auto &display=tasks.task_recruitment_pages.at(animation);
        require(!display.entries.empty(),"real recruited source exists");
        display.portraits={display.entries.front().human};display.portrait_timer=1;display.completion_tick=1000;
        for(auto &entry:display.entries)entry.counter=-100;
        const auto held=[&](auto &s){const auto result=p::update_startup_world_runtime_task_page(s,animation,true);require(result.has_value(),"24 held update");s=*result;};
        roundtrip(tasks,held,"24 independent clocks and ar1 to minus1");held(tasks);
        require(tasks.task_recruitment_pages.at(animation).portrait_timer==-1,"ar minus1 retained");
        reject(tasks,"24 timer below original minus1",[&](auto &s){s.task_recruitment_pages.at(animation).portrait_timer=-2;});
        reject(tasks,"24 missing independent clock",[&](auto &s){s.page_secondary_counters.erase(animation);});
        tasks.task_recruitment_pages.at(animation).completion_tick=tasks.page_counters.at(animation)+1;
        const auto completed=p::update_startup_world_runtime_task_page(tasks,animation,false);
        require(completed.has_value(),"24 real completion to25");tasks=*completed;
        expect(tasks,true,"24 completion and closed animation do not put secondary on25");
        for(int turn=0;turn<40;++turn) {
            const auto top=std::find_if(tasks.scripts.pages.rbegin(),tasks.scripts.pages.rend(),[](const auto &v){return v.lifecycle!=4;});
            if(top!=tasks.scripts.pages.rend()&&top->legacy_page==25)break;
            require(top!=tasks.scripts.pages.rend()&&p::acknowledge_startup_world_runtime_page(tasks,top->id)==E::none,"consume real recruitment message");
            const auto next=p::prepare_startup_world_runtime(tasks);require(next.candidate.has_value(),"recruitment message frame");tasks=*next.candidate;
        }
        const auto team=std::find_if(tasks.scripts.pages.rbegin(),tasks.scripts.pages.rend(),[](const auto &v){return v.lifecycle!=4;});
        require(team!=tasks.scripts.pages.rend()&&team->legacy_page==25,"25 becomes real top");
        require(!tasks.page_secondary_counters.count(team->id)&&p::act_startup_world_runtime_task_page(tasks,team->id,A::depart).error==E::none,"25 real opens28 without extra clock");
        const auto departure=tasks.scripts.pages.back().id;expect(tasks,true,"28 actual producer has independent clock");
        reject(tasks,"28 missing independent clock",[&](auto &s){s.page_secondary_counters.erase(departure);});
        roundtrip(tasks,[&](auto &s){require(p::act_startup_world_runtime_task_page(s,departure,A::confirm).error==E::none,"28 begin departure");},
                  "28 main reset retains real secondary after replay");

        auto tracking=baseline;
        const auto candidate=r::prepare_world_task_creation(p::startup_world_runtime_factory(tracking),0);
        require(candidate.candidate&&candidate.candidate->created_task&&p::write_startup_world_runtime_factory(tracking,candidate.candidate->state),"tracking real task");
        require(p::open_startup_world_runtime_task_menu(tracking)==E::none,"tracking open22");
        tracking.scripts.pages.back().lifecycle=2;
        const auto directory=tracking.scripts.pages.back().id;input={};input.track=true;
        roundtrip(tracking,[&](auto &s){require(p::input_startup_world_runtime_task_page(s,directory,input).error==E::none,"soft6 activation");},
                  "22 soft6 source tracking state5 restore");
        require(p::input_startup_world_runtime_task_page(tracking,directory,input).error==E::none,"tracking scene installed");
        const auto scene=tracking.scripts.pages.back().id;
        reject(tracking,"scene5 missing copied catalogue",[&](auto &s){s.task_page_lists.erase(scene);});
        reject(tracking,"scene5 missing selection",[&](auto &s){s.page_phases.erase(scene);});
        roundtrip(tracking,[&](auto &s){const auto next=p::update_startup_world_runtime_task_tracking_scene(s);require(next.has_value(),"source camera update");s=*next;},
                  "scene5 both cameras replay from exact source snapshot");
    }
    {
        // 30沿真实工厂/胜利producer安装；胜利时点是短条件夹具，不重复完整经营前缀。
        using E=p::StartupWorldRuntimeError;
        const auto require=[&](bool ok,const char *why){++checks;if(!ok)throw std::runtime_error(std::string("restore victory30: ")+why);};
        auto victory=baseline;
        const auto made=r::prepare_world_task_creation(p::startup_world_runtime_factory(victory),0);
        require(made.candidate&&made.candidate->created_task&&p::write_startup_world_runtime_factory(victory,made.candidate->state),"real task");
        const auto previous_active=victory.active_task;victory.active_task=*made.candidate->created_task;
        require(p::consume_startup_world_runtime_task_encounter_request(victory,{r::EncounterRequestKind::page30,{},30,70}),"real page30 producer");
        victory.active_task=previous_active;
        const auto id=victory.scripts.pages.back().id;
        expect(victory,true,"30 pending with full constructor clocks");
        const auto reject=[&](const auto &source,const char *why,const auto &damage){auto bad=source;damage(bad);expect(bad,false,why);};
        reject(victory,"pending30 missing secondary",[&](auto &s){s.page_secondary_counters.erase(id);});
        reject(victory,"pending30 already advanced secondary",[&](auto &s){s.page_secondary_counters.at(id)=1;});
        const auto advance=[&](auto &s){const auto next=p::prepare_startup_world_runtime(s);require(next.candidate.has_value(),"admitted30 update");s=*next.candidate;};
        advance(victory);
        require(victory.page_counters.at(id)==1&&victory.page_secondary_counters.at(id)==1,"both real clocks advanced");
        const auto check_replay=[&](const auto &source,const auto &consume,const char *why) {
            expect(source,true,why);const auto bytes=p::persistence_detail::encode_state(source);
            auto restored=p::persistence_detail::decode_state(bytes,*source.rules),live=source;
            consume(live);consume(restored);expect(live,true,why);expect(restored,true,why);
            require(p::persistence_detail::encode_state(live)==p::persistence_detail::encode_state(restored),"exact replay retains both clocks, random and outputs");
        };
        const auto next_phase=[&](auto &s){
            require(p::acknowledge_startup_world_runtime_page(s,id)==E::none&&s.page_counters.at(id)==40,"phase0 early skip40");
            require(p::acknowledge_startup_world_runtime_page(s,id)==E::none&&s.page_phases.at(id)==1&&
                    s.page_counters.at(id)==0&&s.page_secondary_counters.at(id)==1,"phase1 clears only main clock");
            advance(s);
        };
        check_replay(victory,next_phase,"30 phase0 to phase1 short restore");next_phase(victory);
        require(victory.page_secondary_counters.at(id)==2&&victory.sound_requests.size()==1&&
                victory.sound_requests.front()==p::StartupAudioRequest{p::StartupAudioOperation::jingle,4},"phase1 does not replay first phase sound");
        reject(victory,"initialized30 missing main",[&](auto &s){s.page_counters.erase(id);});
        reject(victory,"initialized30 missing phase",[&](auto &s){s.page_phases.erase(id);});
        reject(victory,"initialized30 invalid phase",[&](auto &s){s.page_phases.at(id)=2;});
        reject(victory,"initialized30 missing secondary",[&](auto &s){s.page_secondary_counters.erase(id);});
        reject(victory,"initialized30 negative secondary",[&](auto &s){s.page_secondary_counters.at(id)=-1;});
        reject(victory,"initialized30 overflow secondary",[&](auto &s){s.page_secondary_counters.at(id)=std::numeric_limits<int>::max();});
        reject(victory,"initialized30 wrong task binding",[&](auto &s){s.exploration_summaries.at(id).task=s.next_task_identity;});
        reject(victory,"secondary cannot be attached to32",[&](auto &s){s.scripts.pages.back().legacy_page=32;s.exploration_summaries.at(id).legacy_page=32;});
        reject(victory,"orphan secondary",[&](auto &s){s.page_secondary_counters.emplace(s.scripts.next_page_id+1,0);});
        auto paused=victory;paused.scene.framework_paused=true;
        check_replay(paused,advance,"30 paused phase1 restore retains clocks");
        const auto close=[&](auto &s){require(p::acknowledge_startup_world_runtime_page(s,id)==E::none&&
            p::acknowledge_startup_world_runtime_page(s,id)==E::none,"phase1 skip and close");};
        check_replay(victory,close,"30 phase1 close keeps complete payload until Finish");close(victory);
        require(victory.scripts.pages.back().lifecycle==4&&victory.page_secondary_counters.at(id)==2,"closed30 retains actual frame2");
        reject(victory,"closed30 missing secondary",[&](auto &s){s.page_secondary_counters.erase(id);});
        reject(victory,"closed30 missing summary",[&](auto &s){s.exploration_summaries.erase(id);});
        advance(victory);
        require(!victory.page_secondary_counters.count(id)&&!victory.page_counters.count(id)&&
                !victory.page_phases.count(id)&&!victory.exploration_summaries.count(id),"Finish retires all victory references");
        expect(victory,true,"retired30 no residual map entries");
    }
    {
        // 年度授勋真实producer产生88/67；资格和135边界为定向夹具，不重跑一年。
        using E=p::StartupWorldRuntimeError;
        const auto require=[&](bool ok,const char *why){++checks;if(!ok)throw std::runtime_error(std::string("restore award/effort: ")+why);};
        auto effort=p::test_support::page_fixture(87);effort.human_presence.at(1)=1;
        const auto parent=effort.scripts.pages.back().id;
        const auto advance=[&](auto &s){const auto next=p::prepare_startup_world_runtime(s);require(next.candidate.has_value(),"admitted result update");s=*next.candidate;};
        advance(effort);
        require(p::act_startup_world_runtime_award_page(effort,parent,r::WorldAwardAction::request_award)==E::none&&
                p::act_startup_world_runtime_award_page(effort,parent,r::WorldAwardAction::confirm_award)==E::none,"actual annual reward producer");
        const auto page=std::find_if(effort.scripts.pages.begin(),effort.scripts.pages.end(),[](const auto &v){return v.legacy_page==67;});
        require(page!=effort.scripts.pages.end(),"real67 queued");const auto id=page->id;
        expect(effort,true,"actual award67 pending below88");
        const auto reject=[&](const auto &source,const char *why,const auto &damage){auto bad=source;damage(bad);expect(bad,false,why);};
        const auto check_replay=[&](const auto &source,const auto &consume,const char *why) {
            expect(source,true,why);const auto bytes=p::persistence_detail::encode_state(source);
            auto restored=p::persistence_detail::decode_state(bytes,*source.rules),live=source;
            require(p::persistence_detail::encode_state(restored)==bytes,"exact ceremony clock and shared display restore");
            consume(live);consume(restored);expect(live,true,why);expect(restored,true,why);
            require(p::persistence_detail::encode_state(live)==p::persistence_detail::encode_state(restored),"same input preserves clocks, reward, bound dialogue and random");
        };
        reject(effort,"pending67 cannot omit main",[&](auto &s){s.page_counters.erase(id);});
        reject(effort,"pending67 cannot invent advanced clock",[&](auto &s){s.page_counters.at(id)=1;});
        reject(effort,"pending67 cannot omit phase",[&](auto &s){s.page_phases.erase(id);});
        const auto display=effort.scripts.pages.back().id;
        require(effort.scripts.pages.back().legacy_page==88,"source88 covers67");
        reject(effort,"pending88 cannot omit clock",[&](auto &s){s.page_counters.erase(display);});
        reject(effort,"pending88 cannot invent phase1",[&](auto &s){s.page_phases.at(display)=1;});
        auto pending_paused=effort;pending_paused.scene.framework_paused=true;
        check_replay(pending_paused,advance,"88 pause before Init retains complete zero payload");
        check_replay(effort,advance,"88 first admitted update with restored shared reward");advance(effort);
        reject(effort,"initialized88 missing phase",[&](auto &s){s.page_phases.erase(display);});
        reject(effort,"initialized88 wrong human",[&](auto &s){s.page_human_bindings.at(display)=-1;});
        reject(effort,"initialized88 extra secondary",[&](auto &s){s.page_secondary_counters.emplace(display,0);});
        reject(effort,"initialized88 invalid shared new reward",[&](auto &s){s.reward_display[1][0]=101;});
        effort.page_counters.at(display)=134;
        check_replay(effort,advance,"88 boundary134 to automatic135 phase1 without random");advance(effort);
        require(effort.page_phases.at(display)==1&&effort.page_counters.at(display)==135,"source ordinary135 transition");
        auto paused_award=effort;paused_award.scene.framework_paused=true;
        check_replay(paused_award,advance,"88 phase1 pause retains independent application state");
        const auto fastforward=[&](auto &s){
            require(p::acknowledge_startup_world_runtime_page(s,display)==E::none&&s.page_counters.at(display)==202&&
                    p::acknowledge_startup_world_runtime_page(s,display)==E::none&&s.page_counters.at(display)==212,
                    "separate local0 then67 confirmations fast-forward without closing");
        };
        check_replay(effort,fastforward,"88 restore keeps202 and212 separate from final random event");fastforward(effort);
        const auto final_award=[&](auto &s){require(p::acknowledge_startup_world_runtime_page(s,display)==E::none&&
            s.scripts.pages.back().speaker_kind==1&&s.scripts.pages.back().speaker_definition==1,"final88 random dialogue binds same human");};
        check_replay(effort,final_award,"88 final confirmation restores same random event and closes before Finish");
        require(p::acknowledge_startup_world_runtime_page(effort,display)==E::none,"qualified88 close through actual event");
        reject(effort,"closed88 cannot omit main",[&](auto &s){s.page_counters.erase(display);});
        reject(effort,"closed88 cannot omit bound human",[&](auto &s){s.page_human_bindings.erase(display);});
        check_replay(effort,advance,"closed88 Finish clears retained references exactly once");
        for(int n=0;n<32&&!(effort.scripts.pages.back().id==id&&effort.scripts.pages.back().lifecycle==2);++n) {
            advance(effort);
            if(effort.scripts.pages.back().kind==r::WorldScriptPageKind::dialogue)
                require(p::acknowledge_startup_world_runtime_page(effort,effort.scripts.pages.back().id)==E::none,"real award dialogue return");
        }
        require(effort.scripts.pages.back().id==id&&effort.scripts.pages.back().lifecycle==2&&
                !effort.page_counters.count(display)&&!effort.page_phases.count(display)&&!effort.page_human_bindings.count(display),
                "source67 becomes admitted top after all88 references retire");
        reject(effort,"initialized67 missing binding",[&](auto &s){s.page_human_bindings.erase(id);});
        reject(effort,"initialized67 wrong phase",[&](auto &s){s.page_phases.at(id)=1;});
        reject(effort,"67 does not accept invented secondary clock",[&](auto &s){s.page_secondary_counters.emplace(id,0);});
        reject(effort,"67 shared delta inconsistent",[&](auto &s){s.effort_display[2][0]+=1;});
        auto paused=effort;paused.scene.framework_paused=true;
        check_replay(paused,advance,"initialized67 pause keeps main and payload");
        const auto skip=[&](auto &s){require(p::acknowledge_startup_world_runtime_page(s,id)==E::none&&s.page_counters.at(id)==67,"source67 fast-forward");};
        check_replay(effort,skip,"actual67 fast-forward restore");skip(effort);
        const auto finish=[&](auto &s){for(int n=0;n<6;++n)advance(s);require(p::acknowledge_startup_world_runtime_page(s,id)==E::none,"73 confirmed close");};
        check_replay(effort,finish,"actual67 six updates then confirmed close");finish(effort);
        reject(effort,"closed67 missing main",[&](auto &s){s.page_counters.erase(id);});
        reject(effort,"closed67 missing human",[&](auto &s){s.page_human_bindings.erase(id);});
        check_replay(effort,advance,"actual67 Finish retires shared references once");
    }
    {
        // 96页面调用点夹具：定义引用来自真实原表；不以codec测试冒认自然入住。
        using E=p::StartupWorldRuntimeError;
        const auto require=[&](bool ok,const char *why){++checks;if(!ok)throw std::runtime_error(std::string("restore residence96: ")+why);};
        auto home=baseline;
        r::WorldScriptPage page;page.kind=r::WorldScriptPageKind::raw_page;page.legacy_page=96;page.legacy_f=25;
        const auto made=r::prepare_world_script_page(p::startup_world_runtime_scripts(home),page);
        require(made.candidate&&made.candidate->last_page&&p::write_startup_world_runtime_scripts(home,made.candidate->state),"page insertion");
        const auto id=*made.candidate->last_page;
        home.page_human_bindings.emplace(id,1);home.page_counters.emplace(id,0);
        home.page_phases.emplace(id,0);home.page_secondary_counters.emplace(id,0);
        home.reward_display={{{98,90},{100,100},{5,15}}};
        expect(home,true,"96 complete pending constructor payload");
        const auto reject=[&](const auto &source,const char *why,const auto &damage){auto bad=source;damage(bad);expect(bad,false,why);};
        reject(home,"pending96 cannot start with advanced independent clock",[&](auto &s){s.page_secondary_counters.at(id)=1;});
        reject(home,"pending96 missing main",[&](auto &s){s.page_counters.erase(id);});
        reject(home,"96 wrong nonresidential definition",[&](auto &s){s.scripts.pages.back().legacy_f=33;});
        reject(home,"96 absent human definition",[&](auto &s){s.page_human_bindings.at(id)=-1;});
        const auto advance=[&](auto &s){const auto next=p::prepare_startup_world_runtime(s);require(next.candidate.has_value(),"admitted update");s=*next.candidate;};
        const auto check_replay=[&](const auto &source,const auto &consume,const char *why) {
            expect(source,true,why);const auto bytes=p::persistence_detail::encode_state(source);
            auto restored=p::persistence_detail::decode_state(bytes,*source.rules),live=source;
            require(p::persistence_detail::encode_state(restored)==bytes,"exact clocks/definition references decode");
            consume(live);consume(restored);expect(live,true,why);expect(restored,true,why);
            require(p::persistence_detail::encode_state(live)==p::persistence_detail::encode_state(restored),
                    "same input retains independent clocks, random and single output");
        };
        auto paused=home;paused.scene.framework_paused=true;
        check_replay(paused,advance,"96 paused pending remains uninitialized across restore");
        check_replay(home,advance,"96 pending first jingle replay");advance(home);
        const auto next_phase=[&](auto &s){
            require(p::acknowledge_startup_world_runtime_page(s,id)==E::none&&
                    p::acknowledge_startup_world_runtime_page(s,id)==E::none,"phase0 skip then enter phase1");advance(s);
        };
        check_replay(home,next_phase,"96 phase transition retains independent frame2");next_phase(home);
        require(home.page_counters.at(id)==1&&home.page_secondary_counters.at(id)==2&&home.page_phases.at(id)==1,
                "phase1 main differs from preserved frame2");
        reject(home,"initialized96 missing secondary",[&](auto &s){s.page_secondary_counters.erase(id);});
        reject(home,"initialized96 invalid phase",[&](auto &s){s.page_phases.at(id)=2;});
        reject(home,"initialized96 overflow clock",[&](auto &s){s.page_secondary_counters.at(id)=std::numeric_limits<int>::max();});
        reject(home,"96 orphan secondary",[&](auto &s){s.page_secondary_counters.emplace(s.scripts.next_page_id+1,0);});
        const auto close=[&](auto &s){require(p::acknowledge_startup_world_runtime_page(s,id)==E::none&&
            p::acknowledge_startup_world_runtime_page(s,id)==E::none,"phase1 skip77 then close");};
        check_replay(home,close,"96 close restores complete retained payload");close(home);
        reject(home,"closed96 missing secondary",[&](auto &s){s.page_secondary_counters.erase(id);});
        reject(home,"closed96 missing human binding",[&](auto &s){s.page_human_bindings.erase(id);});
        check_replay(home,advance,"96 Finish releases restored references once");
    }
    {
        const auto require = [&](bool valid, const char *scenario) {
            ++checks;
            if (!valid) throw std::runtime_error(std::string("restore manual save: ") + scenario);
        };
        const auto tick = [&](auto &state) {
            auto result = p::prepare_startup_world_runtime(state);
            require(result.candidate.has_value(), "actual framework admission");
            state = std::move(*result.candidate);
        };
        const auto choose = [&](auto &state, int tag) {
            const auto id = state.scripts.pages.back().id;
            const auto view = p::inspect_startup_world_menu_page(state, id);
            require(view.has_value(), "initialized source menu");
            const auto row = std::find(view->tags.begin(), view->tags.end(), tag);
            require(row != view->tags.end(), "source tag exists");
            p::StartupWorldMenuInput select;
            select.select_row = static_cast<int>(row - view->tags.begin());
            p::StartupWorldMenuInput confirm; confirm.confirm = true;
            require(p::input_startup_world_menu_page(state, id, select) == p::StartupWorldRuntimeError::none &&
                        p::input_startup_world_menu_page(state, id, confirm) == p::StartupWorldRuntimeError::none,
                    "real navigation consumes menu row");
        };
        auto save = baseline;
        require(p::open_startup_world_main_menu(save) == p::StartupWorldRuntimeError::none, "open3");
        tick(save); choose(save, 6); tick(save); choose(save, 20);
        const auto id = save.scripts.pages.back().id;
        expect(save, true, "raw14 pending Init with source menu retirement");
        require(p::initialize_startup_world_save_pages(save), "raw14 actual Init");
        const auto check_roundtrip = [&](const auto &state, const char *scenario) {
            expect(state, true, scenario);
            const auto bytes = p::persistence_detail::encode_state(state);
            const auto restored = p::persistence_detail::decode_state(bytes, *state.rules);
            expect(restored, true, scenario);
            require(p::persistence_detail::encode_state(restored) == bytes,
                    "save stage/counter/result/provenance restore without Init or file writes");
            for (int fault = 0; fault < 6; ++fault) {
                auto bad = restored;
                auto page = std::find_if(bad.scripts.pages.begin(), bad.scripts.pages.end(),
                    [&](const auto &entry) { return entry.id == id; });
                require(page != bad.scripts.pages.end(), "restored save page exists");
                if (fault == 0) bad.page_phases.erase(id);
                if (fault == 1) bad.page_counters.erase(id);
                if (fault == 2) bad.page_phases.at(id) = 3;
                if (fault == 3) page->source_record = 9;
                if (fault == 4) page->legacy_tag = 21;
                if (fault == 5) page->legacy_f = 7;
                expect(bad, false, "raw14 malformed initialized provenance/stage/result rejects");
            }
        };
        check_roundtrip(save, "raw14 initialized stage0 exact restore");
        tick(save);
        require(save.page_phases.at(id) == 1, "first update reaches awaiting application writer");
        check_roundtrip(save, "raw14 stage1 exact restore does not repeat earlier world prefix");
        auto source3 = save;
        // 已核raw3来源的专用消费者条件夹具；不宣称save_shortcut已接或重放双Pop。
        source3.scripts.pages.back().source_record = 3;
        check_roundtrip(source3, "raw14 declared source3 consumer payload exact restore");
        for (const bool success : {false, true}) {
            const auto result = p::complete_startup_world_save_page(save, id, success,
                success ? "" : "disk write failed");
            require(result.has_value(), "prepare explicit result");
            check_roundtrip(*result, "raw14 stage2 success/failure exact restore does not write again");
            auto bad = *result;
            bad.page_counters.at(id) = 0;
            expect(bad, false, "raw14 result cannot masquerade as unexecuted first frame");
            if(success) {
                bad=*result;bad.save_marker=0;
                expect(bad,false,"raw14 successful result requires actual committed save marker");
            }
        }
    }
    {
        auto tracking = baseline;
        // 当前任务已消失但镜头页尚未获准更新的合法短边界；下一轮自行退休。
        r::WorldScriptPage page;
        page.id = tracking.scripts.next_page_id++;
        page.kind = r::WorldScriptPageKind::raw_page;
        page.legacy_page = 58;
        tracking.scripts.pages.front().lifecycle = 3;
        tracking.scripts.pages.push_back(page);
        tracking.page_counters.emplace(page.id, 0);
        expect(tracking, true, "58 pending real update with no current task retains counter");
        tracking.page_counters.erase(page.id);
        expect(tracking, false, "58 missing mandatory counter rejects before installation");
    }
    {
        const auto require = [&](bool valid, const char *scenario) {
            ++checks;
            if (!valid) throw std::runtime_error(std::string("restore navigation menu: ") + scenario);
        };
        auto menu = baseline;
        require(p::open_startup_world_main_menu(menu) == p::StartupWorldRuntimeError::none,
                "actual raw3 entry");
        const auto id = menu.scripts.pages.back().id;
        expect(menu, true, "raw3 pending initialization");
        auto initialized = p::prepare_startup_world_runtime(menu);
        require(initialized.candidate.has_value(), "framework initializes real menu");
        menu = std::move(*initialized.candidate);
        expect(menu, true, "raw3 initialized with source position and cached row");
        const auto wire = p::persistence_detail::encode_state(menu);
        const auto restored = p::persistence_detail::decode_state(wire, *menu.rules);
        expect(restored, true, "raw3 exact restore");
        require(p::persistence_detail::encode_state(restored) == wire &&
                    restored.menu_page_data.at(id).tags == menu.menu_page_data.at(id).tags &&
                    restored.menu_page_positions == menu.menu_page_positions,
                "menu payload and original positions are serialized rather than reinitialized");
        for (int fault = 0; fault < 8; ++fault) {
            auto bad = restored;
            if (fault == 0) bad.menu_page_data.erase(id);
            if (fault == 1) bad.menu_page_positions.erase(id);
            if (fault == 2) bad.menu_page_data.at(id).selection = -1;
            if (fault == 3) bad.menu_page_data.at(id).tags.clear();
            if (fault == 4) bad.menu_page_data.at(id).parent = bad.scripts.next_page_id;
            if (fault == 5) bad.main_menu_selection = (bad.menu_page_data.at(id).selection + 1) % 5;
            if (fault == 6) bad.menu_page_data.emplace(bad.scripts.next_page_id, bad.menu_page_data.at(id));
            if (fault == 7) bad.menu_page_positions.emplace(bad.scripts.pages.front().id, std::array<int,2>{});
            expect(bad, false, "menu rejects missing data, wrong row/parent/cache or orphaned position");
            if (fault < 4) {
                bad.scene.framework_paused = true;
                const auto before = p::persistence_detail::encode_state(bad);
                require(!p::prepare_startup_world_runtime(bad).candidate &&
                            !p::update_startup_world_runtime_page(bad) &&
                            p::persistence_detail::encode_state(bad) == before,
                        "paused initialized menu rejects missing source without repair");
            }
        }
        require(p::retire_startup_world_menu_pages(menu), "retire actual navigation menu");
        menu.main_menu_selection = (menu.menu_page_data.at(id).selection + 1) % 5;
        expect(menu, true, "retiring raw3 retains old choice independently of later global cache");
        auto cleaned = p::prepare_startup_world_runtime(menu);
        require(cleaned.candidate && cleaned.candidate->menu_page_data.count(id) == 0 &&
                    cleaned.candidate->menu_page_positions.count(id) == 0,
                "framework Finish releases menu payload and original position");
    }
    {
        const auto require = [&](bool valid, const char *scenario) {
            ++checks;
            if (!valid) throw std::runtime_error(std::string("restore navigation child: ") + scenario);
        };
        const auto tick = [&](auto &state) {
            auto result = p::prepare_startup_world_runtime(state);
            require(result.candidate.has_value(), "actual framework admission");
            state = std::move(*result.candidate);
        };
        const auto select_menu = [&](auto &state, int tag, bool english) {
            const auto id = state.scripts.pages.back().id;
            const auto &tags = state.menu_page_data.at(id).tags;
            const auto row = std::find(tags.begin(), tags.end(), tag);
            require(row != tags.end(), "actual frozen menu contains target tag");
            p::StartupWorldMenuInput select;
            select.select_row = static_cast<int>(row - tags.begin());
            require(p::input_startup_world_menu_page(state, id, select) == p::StartupWorldRuntimeError::none,
                    "select real parent row");
            p::StartupWorldMenuInput confirm;
            confirm.confirm = true; confirm.english = english;
            require(p::input_startup_world_menu_page(state, id, confirm) == p::StartupWorldRuntimeError::none,
                    "confirm real parent row");
        };
        auto information = baseline;
        require(p::open_startup_world_main_menu(information) == p::StartupWorldRuntimeError::none, "open3");
        tick(information);
        select_menu(information, 5, true);
        const auto menu = information.scripts.pages.back().id;
        require(information.scripts.pages.back().legacy_page == 9, "tag5 opens9");
        for (int initialized = 0; initialized < 2; ++initialized) {
            const auto wire = p::persistence_detail::encode_state(information);
            const auto restored = p::persistence_detail::decode_state(wire, *information.rules);
            expect(restored, true, "actual3-to9 pending/initialized position roundtrip");
            require(p::persistence_detail::encode_state(restored) == wire,
                    "restored raw9 retains parent position without reconstructing it");
            for (int fault = 0; fault < 3; ++fault) {
                auto bad = restored;
                if (fault == 0) bad.menu_page_positions.erase(menu);
                if (fault == 1) ++bad.menu_page_positions.at(menu)[0];
                if (fault == 2) ++bad.menu_page_positions.at(menu)[1];
                expect(bad, false, "real3-to9 missing or mismatched stored child position");
            }
            if (initialized == 0) tick(information);
        }
        auto direct = baseline;
        require(p::open_startup_world_information_menu(direct) == p::StartupWorldRuntimeError::none,
                "explicit scene shortcut remains available");
        direct.menu_page_positions.emplace(direct.scripts.pages.back().id, std::array<int,2>{96,109});
        expect(direct, false, "scene shortcut cannot forge a stored raw3 child position");

        auto gifts = baseline;
        require(p::open_startup_world_navigation_submenu(gifts, 4) == p::StartupWorldRuntimeError::none, "open4");
        tick(gifts);
        select_menu(gifts, 9, false);
        const auto directory = gifts.scripts.pages.back().id;
        require(gifts.scripts.pages.back().legacy_page == 40, "tag9 opens actual gift directory40");
        tick(gifts);
        for (int n = 0; n < 16; ++n) {
            const auto active = std::find_if(gifts.scripts.pages.rbegin(), gifts.scripts.pages.rend(),
                                            [](const auto &page) { return page.lifecycle != 4; });
            require(active != gifts.scripts.pages.rend(), "gift tutorial has active page");
            if (active->id == directory && active->lifecycle == 2) break;
            if (active->id != directory)
                require(active->kind == r::WorldScriptPageKind::dialogue &&
                            p::acknowledge_startup_world_runtime_page(gifts, active->id) == p::StartupWorldRuntimeError::none,
                        "98 returns through real dialogue consumer");
            tick(gifts);
        }
        p::StartupInformationInput confirm; confirm.confirm = true;
        require(p::input_startup_world_information_page(gifts, directory, confirm) == p::StartupWorldRuntimeError::none,
                "40 selects person and opens64");
        const auto child = gifts.scripts.pages.back().id;
        const int human = gifts.page_human_bindings.at(child);
        const auto other = std::find_if(gifts.rules->humans.begin(), gifts.rules->humans.end(),
                                        [&](const auto &h) { return h.identity != human; });
        require(other != gifts.rules->humans.end(), "source provides a distinct valid definition");
        for (int initialized = 0; initialized < 2; ++initialized) {
            const auto wire = p::persistence_detail::encode_state(gifts);
            const auto restored = p::persistence_detail::decode_state(wire, *gifts.rules);
            expect(restored, true, "actual40-to64 pending/initialized parent binding roundtrip");
            require(p::persistence_detail::encode_state(restored) == wire, "40-to64 exact payload roundtrip");
            auto bad = restored;
            bad.page_human_bindings.at(child) = other->identity;
            expect(bad, false, "64 cannot change to another existing person behind frozen40 selection");
            if (initialized == 0) tick(gifts);
        }
    }
    {
        const auto require=[&](bool valid,const char *scenario) {
            ++checks;if(!valid)throw std::runtime_error(std::string("restore human catalogue: ")+scenario);
        };
        const auto tick=[&](auto &state) {
            auto result=p::prepare_startup_world_runtime(state);
            require(result.candidate.has_value(),"actual framework admission");state=std::move(*result.candidate);
        };
        using A=p::StartupHumanPageAction;using E=p::StartupWorldRuntimeError;
        for(const int raw:{61,64}) {
            // 全解锁是目录/恢复条件夹具，不是自然经营进度；保留固定规则定义和自然人物。
            auto source=baseline;source.village_points=999;
            for(auto &entry:source.scripts.professions)entry.second.status=1;
            for(auto &entry:source.catalog)if(entry.first.first!=0) {entry.second.status=1;entry.second.free_purchases=1;}
            for(const int event:{99,111,112})source.scripts.event_calls[event]=1;
            const int human=source.scene.world.world.ai.battle.actors.at(source.scene.world.world.ai.human_order.front()).definition;
            source.human_calendar.at(human).celebrations=99;
            require(p::open_startup_world_human_page(source,human)==E::none,"open actual human60");tick(source);
            const auto detail=source.scripts.pages.back().id;
            require(p::act_startup_world_human_page(source,detail,raw==61?A::professions:A::gifts)==E::none,
                    "actual detail opens61/64");
            const auto id=source.scripts.pages.back().id;
            auto premature=source;premature.human_page_scroll.emplace(id,0);
            expect(premature,false,"pending Init catalogue cannot contain installed scroll payload");
            tick(source);
            const auto cursor=p::inspect_startup_world_human_catalogue_cursor(source,id);
            require(cursor&&cursor->count>cursor->visible,"real directory extends beyond visible rows");
            p::StartupHumanCatalogueInput input;input.selection=cursor->visible;
            require(p::input_startup_world_human_catalogue_page(source,id,input)==E::none&&source.human_page_scroll.at(id)==1,
                    "actual selection produces nonzero stored scroll");
            const auto wire=p::persistence_detail::encode_state(source);
            auto restored=p::persistence_detail::decode_state(wire,*source.rules);
            expect(restored,true,"61/64 nonzero-scroll full Owner codec roundtrip");
            require(p::persistence_detail::encode_state(restored)==wire,"cursor is persisted exactly rather than re-derived");
            for(int fault=0;fault<7;++fault) {
                auto bad=restored;
                if(fault==0)bad.human_page_scroll.erase(id);
                if(fault==1)bad.human_page_scroll.at(id)=-1;
                if(fault==2)bad.human_page_scroll.at(id)=cursor->count;
                if(fault==3)bad.human_page_selections.at(id)=0; // 不在first=1显示窗内。
                if(fault==4)bad.scripts.pages.back().legacy_page=60;
                if(fault==5)bad.human_page_scroll.emplace(bad.scripts.next_page_id,0);
                if(fault==6) {
                    const int selected=bad.human_page_selections.at(id);
                    if(raw==61)bad.human_page_catalogs.at(id).at(selected)=std::numeric_limits<int>::max();
                    else bad.equipment_page_catalogs.at(id)[bad.page_phases.at(id)].at(selected)=std::numeric_limits<int>::max();
                }
                expect(bad,false,"missing/range/off-window/wrong-page/orphan/bad-definition catalogue scroll rejects");
            }
            input={};input.up=input.down=true;
            require(p::input_startup_world_human_catalogue_page(source,id,input)==E::none&&
                    p::input_startup_world_human_catalogue_page(restored,id,input)==E::none&&
                    p::startup_world_state_digest(source)==p::startup_world_state_digest(restored),
                    "restored same-round opposing input preserves identical nonzero cursor");
            input={};input.down=true;
            require(p::input_startup_world_human_catalogue_page(source,id,input)==E::none&&
                    p::input_startup_world_human_catalogue_page(restored,id,input)==E::none&&
                    p::startup_world_state_digest(source)==p::startup_world_state_digest(restored),
                    "restored successor input follows identical directory/window/output state");
            if(raw==61) {
                const auto &jobs=source.human_page_catalogs.at(id);
                const auto other=std::find_if(jobs.begin()+cursor->visible,jobs.end(),[&](int job){
                    return job!=source.scene.world.world.ai.growth.at(human).definition.current_profession;});
                require(other!=jobs.end(),"alternative profession beyond first window");
                input={};input.selection=static_cast<int>(other-jobs.begin());
                require(p::input_startup_world_human_catalogue_page(source,id,input)==E::none,"select alternative profession");
                input={};input.confirm=true;
            } else {input={};input.confirm=true;}
            require(p::input_startup_world_human_catalogue_page(source,id,input)==E::none,"open actual62/65 confirmation child");
            const auto child=source.scripts.pages.back().id;
            auto child_restored=p::persistence_detail::decode_state(p::persistence_detail::encode_state(source),*source.rules);
            expect(child_restored,true,"nonzero-scroll parent with pending62/65 child roundtrip");
            for(int fault=0;fault<3;++fault) {
                auto bad=child_restored;
                if(fault==0)bad.human_page_parents.erase(child);
                if(fault==1)bad.human_page_parents.at(child)=detail;
                if(fault==2)bad.human_page_parents.at(child)=bad.scripts.next_page_id;
                expect(bad,false,"catalogue child missing/wrong-kind/orphan parent rejects");
            }
            if(raw==64) {
                // 在真实60→64→65链分支验证结果恢复；原目录取消检查仍用未消费的Owner。
                auto gift=source;
                const auto choice=gift.human_equipment_choices.at(child);
                require(choice[0]==0&&gift.shop_humans.at(human).equipment[0]!=std::optional<int>{choice[1]},
                        "actual nonzero-scroll weapon selection changes equipment identity");
                const auto cash=gift.scene.world.world.ai.accounting.funds();
                const auto draws=gift.scene.random.draws();
                const auto stock=gift.catalog.at({1,choice[1]}).free_purchases;
                const auto inventory=gift.catalog.at({1,choice[1]}).inventory;
                require(stock==1&&p::act_startup_world_human_page(gift,child,A::confirm)==E::none&&
                        gift.human_page_answers.at(id)==0&&
                        gift.scene.world.world.ai.accounting.funds()==cash&&gift.scene.random.draws()==draws&&
                        gift.catalog.at({1,choice[1]}).free_purchases==stock,
                        "actual65 only returns answer before parent64 commits gift");
                const auto answer_wire=p::persistence_detail::encode_state(gift);
                auto replay=p::persistence_detail::decode_state(answer_wire,*gift.rules);
                expect(replay,true,"actual65 answer awaiting parent64 recovery");
                require(p::persistence_detail::encode_state(replay)==answer_wire,
                        "restore retains actual parent answer and frozen selected equipment");
                tick(gift);tick(replay);
                const auto display=std::find_if(gift.scripts.pages.begin(),gift.scripts.pages.end(),
                    [](const auto &page){return page.kind==r::WorldScriptPageKind::raw_page&&page.legacy_page==68;});
                require(display!=gift.scripts.pages.end(),"resumed64 creates actual equipment display68");
                const auto result=display->id;
                require(gift.page_human_bindings.at(result)==human&&gift.human_equipment_choices.at(result)==choice&&
                        gift.shop_humans.at(human).equipment[choice[0]]==choice[1]&&
                        gift.scene.world.world.ai.accounting.funds()==cash&&gift.scene.random.draws()==draws+1&&
                        gift.catalog.at({1,choice[1]}).free_purchases==stock-1&&
                        gift.catalog.at({1,choice[1]}).inventory==inventory&&
                        p::persistence_detail::encode_state(gift)==p::persistence_detail::encode_state(replay),
                        "restored parent commits same head identity, one text draw and one free share without cash/inventory charge");
                const auto reject_head=[&](const auto &owner) {
                    for(int fault=0;fault<3;++fault) {
                        auto bad=owner;
                        if(fault==0)bad.human_equipment_choices.erase(result);
                        if(fault==1)bad.human_equipment_choices.at(result)[0]=4;
                        if(fault==2)bad.human_equipment_choices.at(result)[1]=std::numeric_limits<int>::max();
                        expect(bad,false,"68 missing head, item slot4 or invalid equipment ID rejects without default");
                    }
                };
                expect(gift,true,"actual68 pending Init retains source equipment head");
                reject_head(gift);
                // 同一64回调锚点后插页位于先插66下方：先真实消费66，再消费可能出现的67。
                for(int step=0;step<192;++step) {
                    const auto active=std::find_if(gift.scripts.pages.rbegin(),gift.scripts.pages.rend(),
                        [](const auto &page){return page.lifecycle!=4;});
                    require(active!=gift.scripts.pages.rend(),"equipment result retains active page");
                    if(active->id==result&&p::startup_world_human_page_ready(gift,result))break;
                    if(active->id!=result) {
                        const bool effort=active->kind==r::WorldScriptPageKind::raw_page&&active->legacy_page==67;
                        const bool present=active->kind==r::WorldScriptPageKind::raw_page&&active->legacy_page==66&&
                            gift.page_human_bindings.at(active->id)==human&&
                            gift.human_equipment_choices.at(active->id)==choice&&
                            gift.human_gift_scores.count(active->id)&&gift.human_gift_messages.count(active->id);
                        if(!effort&&!present) {
                            std::string stack;
                            for(const auto &page:gift.scripts.pages)
                                stack+=" id="+std::to_string(page.id)+" kind="+std::to_string(static_cast<int>(page.kind))+
                                    " raw="+std::to_string(page.legacy_page)+" source="+std::to_string(page.source_record)+
                                    " life="+std::to_string(page.lifecycle);
                            require(false,("unexpected page covering actual68:"+stack).c_str());
                        }
                        const auto covering=active->id;
                        for(auto *owner:{&gift,&replay})
                            require(p::acknowledge_startup_world_runtime_page(*owner,covering)==E::none,
                                    "actual covering66/67 result receives same confirmation");
                    }
                    tick(gift);tick(replay);
                    require(p::persistence_detail::encode_state(gift)==p::persistence_detail::encode_state(replay),
                            "restored result stack reaches initialized68 with identical inputs");
                }
                require(p::startup_world_human_page_ready(gift,result)&&gift.page_counters.at(result)>0&&
                        gift.page_counters.at(result)<67&&
                        std::any_of(gift.equipment_attribute_display[2].begin(),gift.equipment_attribute_display[2].end(),
                                    [](int delta){return delta!=0;}),
                        "actual gift supplies nonzero equipment comparison and initialized animation frame");
                const auto result_wire=p::persistence_detail::encode_state(gift);
                replay=p::persistence_detail::decode_state(result_wire,*gift.rules);
                expect(replay,true,"initialized68 nonzero display exact restore");
                require(p::persistence_detail::encode_state(replay)==result_wire&&
                        replay.human_equipment_choices.at(result)==choice&&
                        replay.equipment_attribute_display==gift.equipment_attribute_display,
                        "68 frozen source head and before/after/difference restore without reconstruction");
                reject_head(replay);
                for(auto *owner:{&gift,&replay})
                    require(p::act_startup_world_human_page(*owner,result,A::confirm)==E::none&&
                            owner->page_counters.at(result)==67,"same restored68 confirmation reaches source67");
                for(int step=0;step<6;++step) {tick(gift);tick(replay);}
                require(gift.page_counters.at(result)==73&&
                        p::persistence_detail::encode_state(gift)==p::persistence_detail::encode_state(replay),
                        "restored68 continues actual six updates to source73");
                for(auto *owner:{&gift,&replay})
                    require(p::act_startup_world_human_page(*owner,result,A::confirm)==E::none,
                            "actual68 final confirmation closes restored result");
                expect(replay,true,"closed68 retains complete head before Finish");
                tick(gift);tick(replay);
                require(!replay.human_equipment_choices.count(result)&&!replay.page_human_bindings.count(result)&&
                        !replay.human_pages_initialized.count(result)&&!replay.page_counters.count(result)&&
                        replay.scene.world.world.ai.accounting.funds()==cash&&replay.scene.random.draws()==draws+1&&
                        replay.catalog.at({1,choice[1]}).free_purchases==stock-1&&
                        replay.catalog.at({1,choice[1]}).inventory==inventory&&
                        p::persistence_detail::encode_state(gift)==p::persistence_detail::encode_state(replay),
                        "68 Finish releases frozen choices and restored continuation never repeats gift charge/stock/random");
            }
            require(p::act_startup_world_human_page(source,child,A::cancel)==E::none&&
                    p::act_startup_world_human_page(child_restored,child,A::cancel)==E::none,"child cancel returns answer identically");
            tick(source);tick(child_restored);
            require(p::startup_world_state_digest(source)==p::startup_world_state_digest(child_restored)&&
                    !source.page_human_bindings.count(child)&&source.human_page_scroll.at(id)>0,
                    "restored child retirement preserves actual parent scroll and all successor state");
            require(p::act_startup_world_human_page(source,id,A::cancel)==E::none,"close source catalogue");
            expect(source,true,"life4 catalogue still has complete retained cursor before retirement");
            auto invalid=source;invalid.human_page_scroll.at(id)=cursor->count;
            expect(invalid,false,"retained life4 catalogue cannot bypass scroll range validation");
            tick(source);
            require(!source.human_page_scroll.count(id)&&!source.human_pages_initialized.count(id),"framework releases retired scroll payload");
        }
        require(std::string(p::persistence_detail::codec_schema_identity())!=
                "45d69d967b55ef50a4dceb077b780f2058e63fbb43edcb6e1b59de4ee2f58d74",
                "new stored scroll changes codec layout identity; old-header rejection belongs to file-reader suite");
    }
    {
        auto stale=baseline;
        const auto inn=std::find_if(stale.scene.world.world.facilities.begin(),stale.scene.world.world.facilities.end(),
            [](const auto &entry){return entry.second.placement.definition_id==28;});
        if(inn==stale.scene.world.world.facilities.end() || !p::refresh_startup_world_build_notices(stale))
            throw std::runtime_error("restore fixture cannot prepare actual construction notice cache");
        const auto inn_id=inn->first;
        if(!p::retire_startup_world_facility(stale,inn_id) || !p::refresh_startup_world_map(stale,false))
            throw std::runtime_error("restore fixture cannot retire original inn and refresh actual map");
        ++checks;
        if(!stale.facility_build_present.at(28))
            throw std::runtime_error("restore fixture retirement unexpectedly cleared cached P");
        // 原P/aY可能落后于实际地图，精确恢复不得以当前实体重算或要求相等。
        const auto wire=p::persistence_detail::encode_state(stale);
        const auto copy=p::persistence_detail::decode_state(wire,*stale.rules);
        expect(copy,true,"stale construction notice cache is legitimate");
        ++checks;
        if(copy.facility_build_present!=stale.facility_build_present || copy.build_category_new!=stale.build_category_new)
            throw std::runtime_error("restore fixture rewrote construction notice cache");
        stale.facility_build_present.erase(stale.facility_build_present.begin());
        expect(stale,false,"construction cache missing definition key");
        stale=baseline;stale.facility_build_present.emplace(std::numeric_limits<int>::max(),false);
        expect(stale,false,"construction cache unknown definition key");
    }
    {
        auto information = baseline;
        if (p::open_startup_world_information_menu(information) != p::StartupWorldRuntimeError::none)
            throw std::runtime_error("restore fixture cannot open information menu9");
        expect(information, true, "information9 uninitialized entry");
        const auto tick = [&](auto &state) {
            auto result = p::prepare_startup_world_runtime(state);
            if (!result.candidate)
                throw std::runtime_error("restore fixture information framework rejected");
            state = std::move(*result.candidate);
        };
        tick(information);
        p::StartupInformationInput select;
        select.select_row = 2;
        if (p::input_startup_world_information_page(information, information.scripts.pages.back().id,
                                                   select) != p::StartupWorldRuntimeError::none ||
            p::acknowledge_startup_world_runtime_page(information, information.scripts.pages.back().id) !=
                p::StartupWorldRuntimeError::none)
            throw std::runtime_error("restore fixture cannot select income36");
        // 新页0与已关闭菜单4同时存在，恢复不提前初始化或执行Finish。
        expect(information, true, "information9 closed with income36 pending");
        auto broken_closed = information;
        const auto closed_id = broken_closed.scripts.pages[broken_closed.scripts.pages.size() - 2].id;
        broken_closed.page_counters.erase(closed_id);
        expect(broken_closed, false, "retiring information9 cannot retain half a payload");
        tick(information);
        const auto id = information.scripts.pages.back().id;
        p::StartupInformationInput right;
        right.right = true;
        if (p::input_startup_world_information_page(information, id, right) !=
            p::StartupWorldRuntimeError::none)
            throw std::runtime_error("restore fixture income year selection rejected");
        const auto wire = p::persistence_detail::encode_state(information);
        auto restored = p::persistence_detail::decode_state(wire, *information.rules);
        expect(restored, true, "income36 initialized year exact restore");
        ++checks;
        if (restored.page_phases.at(id) != 1 || p::persistence_detail::encode_state(restored) != wire)
            throw std::runtime_error("restore fixture income36 exact codec roundtrip");
        tick(information); tick(restored);
        ++checks;
        if (p::persistence_detail::encode_state(restored) != p::persistence_detail::encode_state(information))
            throw std::runtime_error("restore fixture income36 same-input continuation");
        for (int fault = 0; fault < 5; ++fault) {
            auto damaged = restored;
            if (fault == 0 || fault == 2) damaged.page_counters.erase(id);
            if (fault == 1 || fault == 2) damaged.page_phases.erase(id);
            if (fault == 3) damaged.page_phases.at(id) = 2;
            if (fault == 4) damaged.page_counters.at(id) = -1;
            expect(damaged, false, "income36 refuses absent or malformed initialized payload");
        }
    }
    {
        const auto require = [&](bool valid, const char *scenario) {
            ++checks;
            if (!valid) throw std::runtime_error(std::string("restore information directory: ") + scenario);
        };
        const auto tick = [&](auto &state) {
            auto result = p::prepare_startup_world_runtime(state);
            require(result.candidate.has_value(), "framework admission");
            state = std::move(*result.candidate);
        };
        const auto input = [&](auto &state, std::uint64_t id, const p::StartupInformationInput &action) {
            require(p::input_startup_world_information_page(state, id, action) ==
                        p::StartupWorldRuntimeError::none, "real page input");
        };
        for (int raw : {37, 38}) {
            auto state = baseline;
            // 最小库存条件用于取得可滚动37；目录/页签由真实9入口及框架Init生成。
            // 38仅置一条装备NEW，检验恢复与关闭不会误用37的整类清理。
            if (raw == 37) {
                require(state.rules->items.size() >= 6, "source supplies six ordinary item definitions");
                for (std::size_t n = 0; n < 6; ++n) {
                    const int item = state.rules->items[n].identity;
                    state.items.at(item).inventory = 1;
                    state.items.at(item).newly_unlocked = true;
                    state.catalog.at({0, item}) = state.items.at(item);
                }
            } else {
                state.catalog.at({1, 0}).newly_unlocked = true;
            }
            require(p::open_startup_world_information_menu(state) == p::StartupWorldRuntimeError::none,
                    "open actual menu9");
            tick(state);
            const auto menu = state.scripts.pages.back().id;
            p::StartupInformationInput selection;
            selection.select_row = raw == 37 ? 3 : 4;
            input(state, menu, selection);
            p::StartupInformationInput confirm;
            confirm.confirm = true;
            input(state, menu, confirm);
            const auto id = state.scripts.pages.back().id;
            require(state.scripts.pages.back().legacy_page == raw, "menu opens requested directory");
            expect(state, true, "directory pending Init with retired parent menu");
            tick(state);
            if (raw == 38) {
                p::StartupInformationInput right;
                right.right = true;
                input(state, id, right);
            }
            selection.select_row = 5;
            input(state, id, selection);
            const auto &data = state.information_page_data.at(id);
            require(data.selection == 5 && data.first_visible == (raw == 37 ? 1 : 2) &&
                        state.page_phases.at(id) == (raw == 37 ? 0 : 1),
                    "nonfirst row, scroll and equipment tab reached through actual input");
            const auto wire = p::persistence_detail::encode_state(state);
            auto restored = p::persistence_detail::decode_state(wire, *state.rules);
            expect(restored, true, "initialized37/38 frozen directory restore");
            require(p::persistence_detail::encode_state(restored) == wire,
                    "exact directory payload and NEW survive codec without initialization");

            const auto reject = [&](const char *scenario, const auto &damage) {
                auto broken = restored;
                damage(broken);
                expect(broken, false, scenario);
            };
            reject("directory missing entire map", [&](auto &v) { v.information_page_data.clear(); });
            reject("directory missing phase", [&](auto &v) { v.page_phases.erase(id); });
            reject("directory wrong group count", [&](auto &v) { v.information_page_data.at(id).lists.pop_back(); });
            reject("directory duplicate definition", [&](auto &v) {
                auto &list = v.information_page_data.at(id).lists.front(); list[1] = list[0];
            });
            reject("directory unknown definition", [&](auto &v) {
                v.information_page_data.at(id).lists.front().front() = std::numeric_limits<int>::max();
            });
            reject("directory omitted source row", [&](auto &v) { v.information_page_data.at(id).lists.front().pop_back(); });
            reject("directory negative selection", [&](auto &v) { v.information_page_data.at(id).selection = -1; });
            reject("directory selection outside current group", [&](auto &v) {
                auto &d = v.information_page_data.at(id);
                d.selection = static_cast<int>(d.lists.at(v.page_phases.at(id)).size());
            });
            reject("directory negative scroll", [&](auto &v) { v.information_page_data.at(id).first_visible = -1; });
            reject("directory scroll past selection", [&](auto &v) { v.information_page_data.at(id).first_visible = 6; });
            reject("directory selection outside visible rows", [&](auto &v) { v.information_page_data.at(id).first_visible = 0; });
            reject("directory payload on wrong raw type", [&](auto &v) { v.scripts.pages.back().legacy_page = 36; });
            reject("directory orphaned payload identity", [&](auto &v) {
                v.information_page_data.emplace(v.scripts.next_page_id++, v.information_page_data.at(id));
            });
            if (raw == 38) {
                reject("equipment hidden tab invalid definition", [&](auto &v) {
                    v.information_page_data.at(id).lists.at(3).front() = std::numeric_limits<int>::max();
                });
                reject("equipment excluded Steam flag-zero row injected", [&](auto &v) {
                    v.information_page_data.at(id).lists.at(3).push_back(26);
                });
            }

            p::StartupInformationInput down;
            down.down = true;
            input(state, id, down); input(restored, id, down);
            tick(state); tick(restored);
            require(p::persistence_detail::encode_state(restored) == p::persistence_detail::encode_state(state),
                    "same input and update continue exact restored directory state");
            input(state, id, confirm);
            require(state.scripts.pages.back().lifecycle == 4 && state.information_page_data.count(id),
                    "real close retains complete payload until framework retirement");
            const auto closed_wire = p::persistence_detail::encode_state(state);
            auto closed = p::persistence_detail::decode_state(closed_wire, *state.rules);
            expect(closed, true, "complete closed37/38 survives pre-Finish snapshot");
            require(p::persistence_detail::encode_state(closed) == closed_wire,
                    "closed snapshot does not run Init, clear NEW or consume inputs");
            if (raw == 38)
                require(closed.catalog.at({1, 0}).newly_unlocked,
                        "equipment close and restore preserve existing NEW");
            else
                require(std::none_of(closed.items.begin(), closed.items.end(),
                            [](const auto &entry) { return entry.second.newly_unlocked; }),
                        "item NEW clearing is already committed by actual close");
            tick(closed);
            require(!closed.information_page_data.count(id) && !closed.page_phases.count(id) &&
                        !closed.page_counters.count(id), "real framework retires directory references");
        }
    }
    {
        const auto require=[&](bool valid,const char *scenario) {
            ++checks;
            if(!valid)throw std::runtime_error(std::string("restore adventurer35/detail60: ")+scenario);
        };
        const auto tick=[&](auto &state) {
            auto result=p::prepare_startup_world_runtime(state);
            require(result.candidate.has_value(),"actual framework admission");
            state=std::move(*result.candidate);
        };
        const auto input=[&](auto &state,std::uint64_t id,const p::StartupInformationInput &action) {
            require(p::input_startup_world_information_page(state,id,action)==p::StartupWorldRuntimeError::none,
                    "actual information input");
        };
        auto state=baseline;
        // 复用现套件已自然取得的首访W；不重复新局前缀，也不手造actor/context。
        require(!state.scene.world.world.ai.human_order.empty(),"natural baseline provides live human");
        const auto actor=state.scene.world.world.ai.human_order.front();
        const int definition=state.scene.world.world.ai.battle.actors.at(actor).definition;
        state.scripts.humans.at(definition).pending_notice=true; // 单独验证NEW消费的条件。
        require(p::open_startup_world_information_menu(state)==p::StartupWorldRuntimeError::none,"open9");
        const auto menu=state.scripts.pages.back().id;tick(state);
        p::StartupInformationInput selected;selected.select_row=0;
        p::StartupInformationInput confirm;confirm.confirm=true;
        input(state,menu,selected);input(state,menu,confirm);
        const auto directory=state.scripts.pages.back().id;
        require(state.scripts.pages.back().legacy_page==35,"row0 opens35");
        expect(state,true,"new35 waiting for real Init");tick(state);
        const auto &list=state.information_page_data.at(directory).lists.at(0);
        const auto target=std::find(list.begin(),list.end(),definition);
        require(target!=list.end(),"original definition directory includes natural live actor definition");
        selected.select_row=static_cast<int>(target-list.begin());input(state,directory,selected);
        p::StartupInformationInput right;right.right=true;
        input(state,directory,right);input(state,directory,right);
        const auto directory_wire=p::persistence_detail::encode_state(state);
        auto restored=p::persistence_detail::decode_state(directory_wire,*state.rules);
        expect(restored,true,"35 contribution and tab2 exact restore");
        require(p::persistence_detail::encode_state(restored)==directory_wire,"35 decode does not recalculate contribution or clearNEW");
        for(int fault=0;fault<5;++fault) {
            auto invalid=restored;
            if(fault==0)invalid.information_page_data.erase(directory);
            if(fault==1)invalid.page_phases.at(directory)=4;
            if(fault==2)invalid.information_page_data.at(directory).lists[0].push_back(definition);
            if(fault==3)invalid.information_page_data.at(directory).lists[0].clear();
            if(fault==4)invalid.information_page_data.at(directory).facilities.push_back(1);
            expect(invalid,false,"35 absent/malformed frozen definition catalogue");
        }
        input(state,directory,confirm);input(restored,directory,confirm);
        const auto detail=state.scripts.pages.back().id;
        require(state.scripts.pages.back().legacy_page==60 &&
                state.human_detail_contexts.at(detail).chase_mode==1 &&
                !state.human_detail_contexts.at(detail).actor &&
                !state.scripts.humans.at(definition).pending_notice,
                "35 confirmation binds source1 definition-only60 and commits directory NEW");
        expect(state,true,"source160 pending Init with actual35 parent");
        tick(state);tick(restored);
        require(p::persistence_detail::encode_state(state)==p::persistence_detail::encode_state(restored),
                "same input continues restored35 through real60 Init");
        const auto detail_wire=p::persistence_detail::encode_state(state);
        restored=p::persistence_detail::decode_state(detail_wire,*state.rules);
        expect(restored,true,"initialized source160 exact restore");
        require(p::persistence_detail::encode_state(restored)==detail_wire,"60 context codec survives unchanged");
        for(int fault=0;fault<8;++fault) {
            auto invalid=restored;
            if(fault==0)invalid.human_detail_contexts.erase(detail);
            if(fault==1)invalid.human_detail_contexts.at(detail).chase_mode=2;
            if(fault==2)invalid.human_detail_contexts.at(detail)={0,actor};
            if(fault==3)invalid.human_detail_contexts.emplace(directory,p::StartupHumanDetailContext{1,{}});
            if(fault==4)invalid.human_detail_contexts.emplace(invalid.scripts.next_page_id++,p::StartupHumanDetailContext{1,{}});
            if(fault==5)invalid.page_human_bindings.at(detail)=(definition+1)%static_cast<int>(invalid.rules->humans.size());
            if(fault==6)invalid.human_pages_initialized.erase(detail);
            if(fault==7)std::find_if(invalid.scripts.pages.begin(),invalid.scripts.pages.end(),[&](const auto &page){return page.id==detail;})->lifecycle=0;
            expect(invalid,false,"60 missing/bad/wrong-parent/orphan context or inconsistent Init stage");
        }
        // 首次60可能被真实111教程遮住；快照两侧消费同一提示并恢复父页。
        for(int n=0;n<16;++n) {
            const auto top=std::find_if(state.scripts.pages.rbegin(),state.scripts.pages.rend(),
                                       [](const auto &page){return page.lifecycle!=4;});
            require(top!=state.scripts.pages.rend(),"first60 tutorial retains active page");
            if(top->id==detail && top->lifecycle==2)break;
            if(top->id!=detail) {
                require(top->kind==r::WorldScriptPageKind::dialogue && top->source_record==88,
                        "only actual111 tutorial is consumed before chase");
                const auto prompt=top->id;
                for(auto *owner:{&state,&restored})
                    require(p::acknowledge_startup_world_runtime_page(*owner,prompt)==
                                p::StartupWorldRuntimeError::none,"actual tutorial confirmation on both owners");
            }
            tick(state);tick(restored);
            require(p::persistence_detail::encode_state(state)==p::persistence_detail::encode_state(restored),
                    "restored60 tutorial follows identical real inputs");
        }
        require(state.scripts.pages.back().id==detail && state.scripts.pages.back().lifecycle==2,
                "first60 tutorial restores active detail within bounded updates");
        {
            auto slots=state;
            require(p::act_startup_world_human_page(slots,detail,p::StartupHumanPageAction::view_tab,2)==p::StartupWorldRuntimeError::none&&
                    p::act_startup_world_human_page(slots,detail,p::StartupHumanPageAction::select,2)==p::StartupWorldRuntimeError::none,
                    "actual equipment page selects slot2 before snapshot");
            const auto wire=p::persistence_detail::encode_state(slots);
            auto recovered=p::persistence_detail::decode_state(wire,*slots.rules);
            expect(recovered,true,"nonzero raw60 equipment selection restores using existing field");
            require(p::persistence_detail::encode_state(recovered)==wire&&
                    p::inspect_startup_world_human_presentation(recovered,detail)->equipment_selection==2,
                    "slot2 exact bytes and view survive without Init or reselect");
            auto invalid=recovered;invalid.human_page_selections.at(detail)=4;
            expect(invalid,false,"equipment snapshot rejects slot4");
            require(p::act_startup_world_human_page(slots,detail,p::StartupHumanPageAction::next)==p::StartupWorldRuntimeError::none&&
                    p::act_startup_world_human_page(recovered,detail,p::StartupHumanPageAction::next)==p::StartupWorldRuntimeError::none&&
                    p::persistence_detail::encode_state(slots)==p::persistence_detail::encode_state(recovered),
                    "same input from slot2 restores accessory3 with identical Owner");
        }
        const auto actor_before=state.scene.world.world.ai.battle.actors.at(actor).position;
        for(auto *owner:{&state,&restored})
            require(p::act_startup_world_human_page(*owner,detail,p::StartupHumanPageAction::track)==
                        p::StartupWorldRuntimeError::none,"source1 real chase command");
        require(state.scene.scene_state==6 && state.scripts.selection_mode==1 &&
                state.scripts.selected_actor==actor.value && !state.scripts.selected_facility &&
                state.scripts.pages.back().kind==r::WorldScriptPageKind::scene &&
                state.scripts.pages.back().lifecycle==2 &&
                state.scene.world.world.ai.battle.actors.at(actor).position.x==actor_before.x &&
                state.scene.world.world.ai.battle.actors.at(actor).position.height==actor_before.height &&
                state.scene.world.world.ai.battle.actors.at(actor).position.z==actor_before.z,
                "tracking restores actual scene6 and selector1 without moving autonomous human");
        require(p::persistence_detail::encode_state(state)==p::persistence_detail::encode_state(restored),
                "live tracking produces identical restored owner");
        expect(state,true,"retired35/60 payloads before framework Finish");
        const auto tracking_wire=p::persistence_detail::encode_state(state);
        auto tracking=p::persistence_detail::decode_state(tracking_wire,*state.rules);
        expect(tracking,true,"tracking scene and complete retired contexts restore");
        const int updates=tracking.global_updates;
        state.confirm_input=true;tracking.confirm_input=true;
        tick(state);tick(tracking);
        require(!tracking.information_page_data.count(directory) && !tracking.human_detail_contexts.count(detail) &&
                !tracking.page_human_bindings.count(detail),"framework retires list/context/binding references once");
        const auto reopened=tracking.scripts.pages.back().id;
        require(reopened!=detail && tracking.scripts.pages.back().legacy_page==60 &&
                tracking.human_detail_contexts.at(reopened).chase_mode==1 &&
                tracking.human_detail_contexts.at(reopened).actor==actor &&
                tracking.scene.scene_state==0 && !tracking.confirm_input && tracking.global_updates==updates,
                "scene6 confirm opens fresh source160 bound to actual W and consumes input once");
        require(p::persistence_detail::encode_state(state)==p::persistence_detail::encode_state(tracking),
                "same scene-confirm input continues exact tracking snapshot");
        expect(tracking,true,"actor-bound source160 pending Init restore");
        const auto reopened_wire=p::persistence_detail::encode_state(tracking);
        auto recovered=p::persistence_detail::decode_state(reopened_wire,*tracking.rules);
        require(p::persistence_detail::encode_state(recovered)==reopened_wire,"optional actual actor context roundtrips");
        auto stale=recovered;
        stale.human_detail_contexts.at(reopened).actor=r::CharacterId{std::numeric_limits<std::uint64_t>::max()};
        expect(stale,false,"60 context cannot restore missing W");
        tick(recovered);
        require(recovered.scripts.pages.back().id==reopened && !recovered.confirm_input,
                "following detail Init cannot replay the consumed scene confirmation");
        require(p::act_startup_world_human_page(recovered,reopened,p::StartupHumanPageAction::cancel)==
                    p::StartupWorldRuntimeError::none,"real reopened60 return");
        expect(recovered,true,"closed actor-bound60 retains context before Finish");
        tick(recovered);
        require(!recovered.human_detail_contexts.count(reopened) && !recovered.page_human_bindings.count(reopened),
                "reopened60 Finish retires actor-context references");
    }
    {
        const auto require=[&](bool valid,const char *scenario) {
            ++checks;
            if(!valid)throw std::runtime_error(std::string("restore town34/facility39: ")+scenario);
        };
        const auto tick=[&](auto &state) {
            auto result=p::prepare_startup_world_runtime(state);
            require(result.candidate.has_value(),"actual framework admission");
            state=std::move(*result.candidate);
        };
        const auto input=[&](auto &state,std::uint64_t page,const p::StartupInformationInput &action) {
            require(p::input_startup_world_information_page(state,page,action)==p::StartupWorldRuntimeError::none,
                    "actual information input");
        };
        auto state=baseline;
        require(p::open_startup_world_information_menu(state)==p::StartupWorldRuntimeError::none,"open9");
        const auto menu=state.scripts.pages.back().id;tick(state);
        p::StartupInformationInput selected;selected.select_row=1;
        p::StartupInformationInput confirm;confirm.confirm=true;
        input(state,menu,selected);input(state,menu,confirm);
        const auto town=state.scripts.pages.back().id;
        expect(state,true,"34 pending Init with retired9");tick(state);
        const auto town_wire=p::persistence_detail::encode_state(state);
        auto restored=p::persistence_detail::decode_state(town_wire,*state.rules);
        require(p::persistence_detail::encode_state(restored)==town_wire,"34 exact payload roundtrip");
        expect(restored,true,"initialized34");
        auto malformed=restored;malformed.information_page_data.emplace(town,p::StartupInformationPageData{});
        expect(malformed,false,"34 cannot own a directory payload");
        input(state,town,confirm);input(restored,town,confirm);
        const auto directory=state.scripts.pages.back().id;
        expect(state,true,"39 pending Init with actual34 parent");
        auto pending=p::persistence_detail::decode_state(p::persistence_detail::encode_state(state),*state.rules);
        require(pending.scripts.pages.back().legacy_page==39 && !pending.information_page_data.count(directory),
                "pending39 decode cannot initialize its list");
        tick(state);tick(restored);
        require(p::persistence_detail::encode_state(state)==p::persistence_detail::encode_state(restored),
                "34 identical confirmation and39 Init across restore");
        const auto fixture_actor=state.scene.world.world.ai.human_order.front();
        state.scripts.selected_actor=fixture_actor.value;
        const auto facility=state.information_page_data.at(directory).facilities.front();
        const auto camera=p::startup_world_runtime_facility_target(state,facility);
        require(camera.has_value(),"original selected facility has actual target");
        // 镜头已在目标是显式短边界条件；不移动人物/建筑，不跑重复世界前缀。
        state.camera=*camera;state.previous_camera=*camera;
        const auto wire=p::persistence_detail::encode_state(state);
        restored=p::persistence_detail::decode_state(wire,*state.rules);
        require(p::persistence_detail::encode_state(restored)==wire,"39 exact stable-ID catalogue and selection restore");
        expect(restored,true,"initialized39 with current original facility");
        for(int fault=0;fault<9;++fault) {
            auto invalid=restored;
            if(fault==0)invalid.information_page_data.erase(directory);
            if(fault==1)invalid.information_page_data.at(directory).lists={{1}};
            if(fault==2)invalid.information_page_data.at(directory).facilities.clear();
            if(fault==3)invalid.information_page_data.at(directory).facilities.front()=std::uint64_t{1}<<40;
            if(fault==4)invalid.information_page_data.at(directory).selection=
                static_cast<int>(invalid.information_page_data.at(directory).facilities.size());
            if(fault==5)invalid.information_page_data.emplace(invalid.scripts.next_page_id++,invalid.information_page_data.at(directory));
            if(fault==6)for(auto &page:invalid.scripts.pages)if(page.id==town)page.legacy_page=36;
            if(fault==7)invalid.facility_monthly_cash.erase(facility);
            if(fault==8)for(auto &page:invalid.scripts.pages)if(page.id==town)page.lifecycle=2;
            expect(invalid,false,"39 missing/wrong/retired reference or parent/source rejects restore");
            if(fault==3) {
                const auto high=p::persistence_detail::decode_state(p::persistence_detail::encode_state(invalid),*invalid.rules);
                require(high.information_page_data.at(directory).facilities.front()==(std::uint64_t{1}<<40),
                        "typed39 codec preserves uint64 before validation; cannot truncate into a live low ID");
            }
        }
        auto closed=restored;p::StartupInformationInput cancel;cancel.cancel=true;
        input(closed,directory,cancel);expect(closed,true,"closed39 before actual Finish retains complete catalogue");
        closed=p::persistence_detail::decode_state(p::persistence_detail::encode_state(closed),*closed.rules);
        tick(closed);
        require(closed.scripts.pages.back().id==town && !closed.information_page_data.count(directory),
                "closed39 restored next frame retires catalogue and recovers34");
        input(state,directory,confirm);input(restored,directory,confirm);
        require(state.scene.scene_state==7 && state.scripts.selected_facility==facility &&
                state.scripts.selected_actor==fixture_actor.value && state.scripts.selection_mode==0,
                "39 confirm chooses scene7 facility while preserving actual actor selector");
        require(p::persistence_detail::encode_state(state)==p::persistence_detail::encode_state(restored),
                "39 restored confirmation keeps exact owner and random order");
        expect(state,true,"scene7 retains closing34/39 payloads before Finish");
        const auto focus=p::persistence_detail::encode_state(state);
        restored=p::persistence_detail::decode_state(focus,*state.rules);
        tick(state);tick(restored);
        require(p::persistence_detail::encode_state(state)==p::persistence_detail::encode_state(restored) &&
                state.scene.scene_state==0 && !state.scripts.selected_facility &&
                state.scripts.selected_actor==fixture_actor.value &&
                !state.information_page_data.count(directory) && !state.page_phases.count(town) &&
                !state.page_counters.count(directory),
                "actual scene7 completion clears facility only, retires pages and replays identically");
    }
    {
        using A=p::StartupCommerceAction;
        const auto require=[&](bool valid,const char *scenario) {
            ++checks;if(!valid)throw std::runtime_error(std::string("restore commerce85/93: ")+scenario);
        };
        const auto tick=[&](auto &state) {
            auto result=p::prepare_startup_world_runtime(state);
            require(result.candidate.has_value(),"real framework admission");state=std::move(*result.candidate);
        };
        auto state=baseline;
        // 只准备85调用点和30村点；原p0定义/资金来自共同baseline，不重复商会解锁长链。
        r::WorldScriptPage entry;entry.kind=r::WorldScriptPageKind::raw_page;entry.legacy_page=85;
        const auto inserted=r::prepare_world_script_page(p::startup_world_runtime_scripts(state),entry);
        require(inserted.candidate && p::write_startup_world_runtime_scripts(state,inserted.candidate->state),"actual85 page factory");
        const auto shop=state.scripts.pages.back().id;
        state.village_points=30;expect(state,true,"pending85 with actual p0 definitions");tick(state);
        const auto &list=state.commerce_page_lists.at(shop);
        const auto drink=std::find(list.begin(),list.end(),34);
        require(drink!=list.end() && state.facility_presence.at(34)==0,"original p0 drink34 in initialized85");
        require(p::act_startup_world_commerce_page(state,shop,A::select,static_cast<int>(drink-list.begin()))==
                p::StartupWorldRuntimeError::none,"select actual drink34");
        const auto wire=p::persistence_detail::encode_state(state);
        auto restored=p::persistence_detail::decode_state(wire,*state.rules);
        expect(restored,true,"p0 commerce85 exact restore");
        require(p::persistence_detail::encode_state(restored)==wire,"85 decode does not re-list or clear read flags");
        for(const int presence:{1,2}) {
            auto stale=restored;stale.facility_presence.at(34)=presence;
            expect(stale,false,"85 frozen list cannot restore a now-p1/p2 definition");
        }
        auto preview=restored;
        require(p::act_startup_world_commerce_page(preview,shop,A::inspect)==p::StartupWorldRuntimeError::none,
                "85 actual definition preview entry");
        expect(preview,true,"p0 definition74 with actual85 parent");
        for(const int presence:{1,2}) {
            auto stale=preview;stale.facility_presence.at(34)=presence;
            expect(stale,false,"74 definition preview cannot restore already-open p1/p2 definition");
        }
        for(auto *owner:{&state,&restored})
            require(p::act_startup_world_commerce_page(*owner,shop,A::confirm)==p::StartupWorldRuntimeError::none,
                    "actual85 payment on both restored owners");
        const auto reward=state.scripts.pages.back().id;
        require(state.scripts.pages.back().legacy_page==93 && state.village_points==0 &&
                state.facility_presence.at(34)==0 && state.facility_free_builds.at(34)==0 &&
                p::persistence_detail::encode_state(state)==p::persistence_detail::encode_state(restored),
                "85 deducts points identically but leaves p0/H0 pending93");
        expect(state,true,"uninitialized93 after points payment");
        for(const int presence:{1,2}) {
            auto stale=state;stale.facility_presence.at(34)=presence;
            expect(stale,false,"pending Init93 also requires unclaimed p0 definition");
        }
        tick(state);tick(restored);
        const auto waiting=p::persistence_detail::encode_state(state);
        restored=p::persistence_detail::decode_state(waiting,*state.rules);
        expect(restored,true,"initialized live93 p0 pending entitlement");
        for(const int presence:{1,2}) {
            auto stale=restored;stale.facility_presence.at(34)=presence;
            expect(stale,false,"live93 cannot restore after external p1/p2 unlock");
        }
        for(int confirmations=0;confirmations<2;++confirmations)
            for(auto *owner:{&state,&restored})
                require(p::act_startup_world_commerce_page(*owner,reward,A::confirm)==p::StartupWorldRuntimeError::none,
                        "real93 early fast-forward then entitlement confirmation");
        require(state.facility_presence.at(34)==2 && state.facility_free_builds.at(34)==1 &&
                state.village_points==0 && p::persistence_detail::encode_state(state)==p::persistence_detail::encode_state(restored),
                "same93 inputs grant p2/H1 once without second point deduction");
        expect(state,true,"retired claimed93 p2 remains valid before Finish");
        for(int fault=0;fault<3;++fault) {
            auto invalid=state;
            if(fault==0)invalid.facility_presence.at(34)=1;
            if(fault==1)invalid.page_counters.erase(reward);
            if(fault==2)invalid.commerce_page_data.at(reward)[4]=-1;
            expect(invalid,false,"retired93 p1,half payload or missing binding cannot bypass restore validation");
        }
        const auto claimed=p::persistence_detail::encode_state(state);
        restored=p::persistence_detail::decode_state(claimed,*state.rules);
        require(p::act_startup_world_commerce_page(restored,reward,A::confirm)!=p::StartupWorldRuntimeError::none &&
                p::persistence_detail::encode_state(restored)==claimed,"restored closed93 cannot grant twice");
        tick(state);tick(restored);
        require(!restored.commerce_pages_initialized.count(reward) && !restored.commerce_page_data.count(reward) &&
                !restored.page_counters.count(reward) && restored.facility_free_builds.at(34)==1 &&
                p::persistence_detail::encode_state(state)==p::persistence_detail::encode_state(restored),
                "real Finish releases93 payload; replay does not refill or repeat payment");
    }
    {
        // 只准备旅店升级资格；页面及独立计时由真实Owner初始化，不手填已初始化载荷。
        auto upgrade = baseline;
        const auto facility = std::find_if(upgrade.scene.world.world.facilities.begin(),
            upgrade.scene.world.world.facilities.end(),
            [](const auto &entry) { return entry.second.placement.definition_id == 28; });
        if (facility == upgrade.scene.world.world.facilities.end())
            throw std::runtime_error("restore fixture missing original inn28 instance");
        auto &progress = upgrade.scene.world.world.facility_uses.at(28);
        progress.upgrade_pending = true;
        progress.completed_uses = 57; // 原旅店一级门槛50；升级计算矩阵由building套件负责。
        if (progress.level != 1 ||
            p::open_startup_world_facility_page(upgrade, facility->first) != p::StartupWorldRuntimeError::none ||
            upgrade.scripts.pages.back().legacy_page != 81)
            throw std::runtime_error("restore fixture cannot open real inn upgrade81");
        const auto id = upgrade.scripts.pages.back().id;
        const auto tick = [&](p::StartupWorldRuntimeState &state) {
            auto result = p::prepare_startup_world_runtime(state);
            ++checks;
            if (!result.candidate)
                throw std::runtime_error("restore fixture upgrade81 update rejected");
            state = std::move(*result.candidate);
        };
        tick(upgrade);
        if (!upgrade.facility_upgrade_initialized.count(id))
            throw std::runtime_error("restore fixture upgrade81 did not initialize");
        upgrade.sound_requests.clear(); // 首次升级声音已领取；恢复不得重新触发它。
        tick(upgrade);
        const auto secondary = upgrade.page_secondary_counters.at(id);
        // 原确认先快进40，再进入phase1并将frame重置0，frame2保持独立。
        for (int input = 0; input < 2; ++input) {
            ++checks;
            if (p::acknowledge_startup_world_runtime_page(upgrade, id) != p::StartupWorldRuntimeError::none)
                throw std::runtime_error("restore fixture upgrade81 phase transition rejected");
        }
        ++checks;
        if (secondary <= 0 || upgrade.page_secondary_counters.at(id) != secondary ||
            upgrade.page_phases.at(id) != 1 || upgrade.page_counters.at(id) != 0)
            throw std::runtime_error("restore fixture upgrade81 independent phase counters");
        expect(upgrade, true, "initialized81 independent secondary counter");
        const auto wire = p::persistence_detail::encode_state(upgrade);
        auto restored = p::persistence_detail::decode_state(wire, *upgrade.rules);
        expect(restored, true, "decoded81 initialized payload");
        ++checks;
        if (p::persistence_detail::encode_state(restored) != wire ||
            restored.page_secondary_counters.at(id) != secondary || restored.page_phases.at(id) != 1 ||
            restored.page_counters.at(id) != 0)
            throw std::runtime_error("restore fixture upgrade81 exact codec roundtrip");
        const auto level = upgrade.scene.world.world.facility_uses.at(28).level;
        const auto uses = upgrade.scene.world.world.facility_uses.at(28).completed_uses;
        tick(upgrade);
        tick(restored);
        ++checks;
        if (p::persistence_detail::encode_state(restored) != p::persistence_detail::encode_state(upgrade) ||
            restored.page_secondary_counters.at(id) <= secondary ||
            restored.scene.world.world.facility_uses.at(28).level != level ||
            restored.scene.world.world.facility_uses.at(28).completed_uses != uses ||
            !restored.sound_requests.empty())
            throw std::runtime_error("restore fixture upgrade81 continues without repeated upgrade or sound");
        for (int fault = 0; fault < 3; ++fault) {
            auto damaged = restored;
            if (fault == 0) damaged.page_secondary_counters.erase(id);
            if (fault == 1) damaged.page_secondary_counters.at(id) = -1;
            if (fault == 2) damaged.page_phases.erase(id);
            expect(damaged, false, fault == 0 ? "initialized81 missing secondary" :
                fault == 1 ? "initialized81 negative secondary" : "initialized81 missing phase");
        }
    }
    auto profile = baseline; // 合法定义覆盖夹具；不创建定义0实例、不改原表。
    profile.human_profiles.emplace(0, p::StartupWorldHumanProfile{"恢复姓名", 1, true});
    profile.scripts.humans.at(0).name = "恢复姓名";
    expect(profile, true, "profile without main character instance");
    auto profile_bad = profile;
    profile_bad.human_profiles.emplace(1, p::StartupWorldHumanProfile{"越权", 0, false});
    expect(profile_bad, false, "unsupported profile stable definition");
    profile_bad = profile;
    profile_bad.human_profiles.at(0).sex = 2;
    expect(profile_bad, false, "profile invalid sex");
    profile_bad = profile;
    profile_bad.human_profiles.at(0).name = std::string("a\0b", 3);
    expect(profile_bad, false, "profile invalid name");
    profile_bad = profile;
    profile_bad.scripts.humans.at(0).name = "旧缓存";
    expect(profile_bad, false, "profile stale script-name reference");
    // 存活主场景有合法身份，也不能接受其它页型的附属载荷。
    for (int domain = 0; domain < 3; ++domain) {
        auto damaged = baseline;
        const auto id = damaged.scripts.pages.front().id;
        if (domain == 0) damaged.facility_catalog_page_data[id] = {1, 0, 0, -1};
        if (domain == 1) damaged.facility_catalog_page_lists[id] = {0};
        if (domain == 2) damaged.facility_catalog_page_parents[id] = id;
        expect(damaged, false, "catalogue payload attached to valid wrong page kind");
    }
    {
        auto damaged = baseline;
        damaged.scripts.pending_completion = 1;
        expect(damaged, false, "duplicate authoritative completion");
    }
    {
        auto damaged = baseline;
        damaged.focus_actor.actor.position.x = std::numeric_limits<float>::infinity();
        expect(damaged, false, "nonfinite independent focus actor");
    }
    {
        auto damaged = baseline;
        damaged.surface.pop_back();
        expect(damaged, false, "partial surface");
    }
    // 明确夹具：期限页允许无任务，以及初始化后尚未作答的-1；不推进业务修补字段。
    auto deadline = baseline;
    r::WorldScriptPage page;
    page.id = deadline.scripts.next_page_id++;
    page.kind = r::WorldScriptPageKind::raw_page;
    page.legacy_page = 33;
    page.lifecycle = 2;
    deadline.scripts.pages.push_back(page);
    deadline.deadline_page = page.id;
    deadline.deadline_initialized.insert(page.id);
    deadline.deadline_grades[page.id] = 4;
    deadline.deadline_returns[page.id] = -1;
    deadline.page_counters[page.id] = 0;
    deadline.page_phases[page.id] = 0;
    expect(deadline, true, "deadline null task and unanswered");
    {
        auto damaged = deadline;
        damaged.deadline_grades[page.id] = 5;
        expect(damaged, false, "deadline grade out of range");
    }
    {
        auto damaged = deadline;
        damaged.deadline_returns.erase(page.id);
        expect(damaged, false, "initialized deadline missing answer");
    }
    {
        auto damaged = deadline;
        damaged.page_phases[page.id] = 2;
        expect(damaged, false, "deadline phase out of range");
    }
    // 关页后可由主场景延迟消费：初始化/答案仍保留，通用页面载荷已退休。
    deadline.deadline_returns[page.id] = 1;
    page.lifecycle = 4;
    deadline.deadline_closed_page = page;
    deadline.scripts.pages.pop_back();
    deadline.page_counters.erase(page.id);
    deadline.page_phases.erase(page.id);
    expect(deadline, true, "retired deadline awaiting main scene");
    {
        auto build = baseline;
        r::WorldScriptPage menu;
        menu.id = build.scripts.next_page_id++;
        menu.kind = r::WorldScriptPageKind::raw_page;
        menu.legacy_page = 21;
        menu.lifecycle = 2;
        build.scripts.pages.push_back(menu);
        build.build_page_catalogs.try_emplace(menu.id);
        build.page_phases[menu.id] = 0;
        build.page_counters[menu.id] = 0;
        expect(build, true, "build menu structural fixture");
        auto damaged = build;
        damaged.build_page_catalogs.erase(menu.id);
        expect(damaged, false, "build menu missing catalogue");
        damaged = build;
        damaged.page_phases[menu.id] =
            static_cast<int>(build.build_page_catalogs.at(menu.id).size());
        expect(damaged, false, "build menu tab overflow");
    }
    {
        auto residence = baseline;
        r::WorldScriptPage menu;
        menu.id = residence.scripts.next_page_id++;
        menu.kind = r::WorldScriptPageKind::raw_page;
        menu.legacy_page = 80;
        menu.lifecycle = 2;
        residence.scripts.pages.push_back(menu);
        residence.residence_page_candidates[menu.id] = {baseline.rules->humans.front().identity};
        residence.facility_page_bindings[menu.id] = baseline.scene.world.facility_order.front();
        expect(residence, true, "residence menu structural fixture");
        auto damaged = residence;
        damaged.residence_page_candidates.erase(menu.id);
        expect(damaged, false, "residence menu missing candidates");
        damaged = residence;
        damaged.facility_page_bindings.erase(menu.id);
        expect(damaged, false, "residence menu missing facility binding");
    }
    {
        // 明确源码合同夹具：world_dungeon_finish::restore_site与runtime_deadline::restore_site
        // 会移除活跃实体/探索进度/邻接，但不清以下九表；期限路径还保留sites。
        // 复制已证真实条目作为旧实例载荷，保留与活跃实例重复的可复用raw/ordinal。
        auto history = baseline;
        const auto live = history.scene.world.facility_order.front();
        const auto retired = history.next_facility_identity++;
#define RETAIN(field) history.field.emplace(retired, history.field.at(live))
        RETAIN(facility_original_ids);
        RETAIN(facility_ordinals);
        RETAIN(facility_residents);
        RETAIN(facility_difficulties);
        RETAIN(facility_flags);
        RETAIN(facility_monthly_cash);
        RETAIN(facility_month_age);
        RETAIN(facility_item_confirmations);
        RETAIN(facility_details);
#undef RETAIN
        history.sites[retired].occupied_cells = {
            history.scene.world.world.facilities.at(live).placement.anchor};
        expect(history, true, "task retirement preserves nine auxiliary histories and old site");
        const auto reject = [&](const char *scenario, const auto &damage) {
            auto broken = history;
            damage(broken);
            expect(broken, false, scenario);
        };
        reject("live facility missing original identity",
               [&](auto &s) { s.facility_original_ids.erase(live); });
        reject("live facility missing details", [&](auto &s) { s.facility_details.erase(live); });
        reject("live facility missing monthly ledger",
               [&](auto &s) { s.facility_monthly_cash.erase(live); });
        reject("retained facility identity outside allocator",
               [&](auto &s) { s.facility_original_ids.emplace(s.next_facility_identity, 0); });
        reject("retained original identity negative",
               [&](auto &s) { s.facility_original_ids.at(retired) = -1; });
        reject("retained residence definition missing", [&](auto &s) {
            s.facility_residents.at(retired) = std::numeric_limits<int>::max();
        });
        reject("retained site outside map", [&](auto &s) {
            s.sites.at(retired).occupied_cells.front().x = s.scene.world.world.map.width;
        });
        reject("retired facility cannot stay in active dungeon progress", [&](auto &s) {
            s.dungeon_facilities.emplace(retired, s.dungeon_facilities.at(live));
        });
        reject("retired facility cannot stay in active neighbourhood", [&](auto &s) {
            s.neighbourhood.emplace(retired, s.neighbourhood.at(live));
            s.neighbourhood_details.emplace(retired, s.neighbourhood_details.at(live));
        });
        reject("active neighbourhood source cannot reference retired facility", [&](auto &s) {
            s.neighbourhood_details.at(live).sources.push_back(
                {{retired}, s.scene.world.world.facilities.at(live).placement.definition_id});
        });
        if (!history.shops.empty()) {
            const auto shop = history.shops.begin()->first;
            reject("active shop missing record", [&](auto &s) { s.shops.erase(shop); });
            reject("shop order references retired facility",
                   [&](auto &s) { s.shop_order.push_back(retired); });
            reject("retired facility cannot stay in shop records", [&](auto &s) {
                s.shops.emplace(retired, s.shops.at(shop));
                s.shop_order.push_back(retired);
            });
        }
    }
    {
        // 页面结构夹具；目录、人物、库存和随机均来自已跑到稳定主场景的真实baseline。
        auto catalogue = baseline;
        r::WorldScriptPage goods;
        goods.id = catalogue.scripts.next_page_id++;
        goods.kind = r::WorldScriptPageKind::raw_page;
        goods.legacy_page = 79;
        goods.lifecycle = 0; // 与prepare_world_script_page新插页一致，未初始化不能伪装更新态。
        goods.legacy_f = 1;
        catalogue.scripts.pages.push_back(goods);
        expect(catalogue, true, "uninitialized79 valid source");
        if (!p::initialize_startup_world_facility_catalog_pages(catalogue))
            throw std::runtime_error("restore fixture cannot initialize79");
        catalogue.scripts.pages.back().lifecycle = 2;
        expect(catalogue, true, "initialized79 complete source");
        const auto reject = [&](const p::StartupWorldRuntimeState &valid,
                                const char *scenario, const auto &damage) {
            auto broken = valid;
            damage(broken);
            expect(broken, false, scenario);
        };
        reject(catalogue, "initialized79 missing data", [&](auto &v) {
            v.facility_catalog_page_data.erase(goods.id);
        });
        reject(catalogue, "initialized79 missing list", [&](auto &v) {
            v.facility_catalog_page_lists.erase(goods.id);
        });
        reject(catalogue, "initialized79 missing counter", [&](auto &v) {
            v.page_counters.erase(goods.id);
        });
        reject(catalogue, "initialized79 missing phase", [&](auto &v) {
            v.page_phases.erase(goods.id);
        });
        if (p::act_startup_world_facility_catalog_page(
                catalogue, goods.id, p::StartupFacilityCatalogAction::inspect) !=
            p::StartupWorldRuntimeError::none ||
            !p::initialize_startup_world_facility_catalog_pages(catalogue))
            throw std::runtime_error("restore fixture cannot open real79 information72");
        const auto info = catalogue.scripts.pages.back().id;
        catalogue.scripts.pages.back().lifecycle = 2;
        expect(catalogue, true, "real79 information72 source");
        reject(catalogue, "information72 missing parent", [&](auto &v) {
            v.facility_catalog_page_parents.erase(info);
        });
        reject(catalogue, "information72 wrong parent", [&](auto &v) {
            v.facility_catalog_page_parents.at(info) = v.scripts.pages.front().id;
        });
        reject(catalogue, "information72 missing equipment binding", [&](auto &v) {
            v.facility_catalog_page_data.at(info)[3] = std::numeric_limits<int>::max();
        });
        reject(catalogue, "information72 wrong parent category", [&](auto &v) {
            v.scripts.pages[v.scripts.pages.size() - 2].legacy_f = 5;
        });
        auto retired = baseline;
        const auto stale = retired.scripts.next_page_id++;
        reject(retired, "retired catalogue initialized identity", [&](auto &v) {
            v.facility_catalog_pages_initialized.insert(stale);
        });
        reject(retired, "retired catalogue data map", [&](auto &v) {
            v.facility_catalog_page_data[stale] = {1, 0, 0, -1};
        });
        reject(retired, "retired catalogue list map", [&](auto &v) {
            v.facility_catalog_page_lists[stale] = {0};
        });
        reject(retired, "retired catalogue parent map", [&](auto &v) {
            v.facility_catalog_page_parents[stale] = goods.id;
        });
        auto praise = baseline;
        const auto d = std::find_if(praise.rules->facilities.begin(), praise.rules->facilities.end(),
                                   [](const auto &v) { return v.legacy_icon == 2; });
        if (d == praise.rules->facilities.end())
            throw std::runtime_error("restore fixture fixed catalogue has no icon2 source");
        r::WorldScriptPage animation;
        animation.id = praise.scripts.next_page_id++;
        animation.kind = r::WorldScriptPageKind::raw_page;
        animation.legacy_page = 82;
        animation.lifecycle = 0;
        animation.legacy_f = 0;
        animation.facility_definition = d->id;
        praise.scripts.pages.push_back(animation);
        expect(praise, true, "uninitialized82 defined source");
        if (!p::initialize_startup_world_facility_catalog_pages(praise))
            throw std::runtime_error("restore fixture cannot initialize82");
        praise.scripts.pages.back().lifecycle = 2;
        expect(praise, true, "initialized82 complete source");
        reject(praise,"initialized82 missing independent counter",[&](auto &v){v.page_secondary_counters.erase(animation.id);});
        reject(praise,"initialized82 independent counter overflow",[&](auto &v){v.page_secondary_counters.at(animation.id)=std::numeric_limits<int>::max();});
        {
            auto pending=praise;pending.facility_catalog_pages_initialized.erase(animation.id);
            expect(pending,false,"uninitialized82 preinstalled independent counter");
        }
        // 两段的短往返只认证来源条件夹具；不重复自然世界前缀。
        const auto praise_source=praise;
        praise.page_counters.at(animation.id)=40;praise.page_secondary_counters.at(animation.id)=117;
        for(int phase=0;phase<2;++phase) {
            expect(praise,true,"82 phase boundary exact source");
            const auto bytes=p::persistence_detail::encode_state(praise);
            auto restored=p::persistence_detail::decode_state(bytes,*praise.rules);
            expect(restored,true,"82 phase boundary exact restore");
            if(p::persistence_detail::encode_state(restored)!=bytes||
               p::act_startup_world_facility_catalog_page(praise,animation.id,p::StartupFacilityCatalogAction::confirm)!=p::StartupWorldRuntimeError::none||
               p::act_startup_world_facility_catalog_page(restored,animation.id,p::StartupFacilityCatalogAction::confirm)!=p::StartupWorldRuntimeError::none||
               p::persistence_detail::encode_state(restored)!=p::persistence_detail::encode_state(praise))
                throw std::runtime_error("restore82 phase input diverges");
            const auto a=p::prepare_startup_world_runtime(praise),b=p::prepare_startup_world_runtime(restored);
            if(!a.candidate||!b.candidate||p::persistence_detail::encode_state(*a.candidate)!=p::persistence_detail::encode_state(*b.candidate))
                throw std::runtime_error("restore82 phase update/output diverges");
            if(phase==0) {praise=*a.candidate;praise.page_counters.at(animation.id)=40;}
        }
        praise=praise_source; // 后面的破坏测试使用原有效页，不能伪造复活已退休的演出。
        reject(praise, "animation82 missing facility definition", [&](auto &v) {
            v.scripts.pages.back().facility_definition.reset();
        });
        reject(praise, "animation82 invalid source definition", [&](auto &v) {
            v.scripts.pages.back().facility_definition = std::numeric_limits<int>::max();
            v.facility_catalog_page_data.at(animation.id)[3] = std::numeric_limits<int>::max();
        });
        reject(praise, "animation82 wrong icon for source mode", [&](auto &v) {
            v.scripts.pages.back().legacy_f = 1;
            v.facility_catalog_page_data.at(animation.id)[0] = 1;
        });
        reject(praise, "animation82 invalid f", [&](auto &v) {
            v.scripts.pages.back().legacy_f = 2;
            v.facility_catalog_page_data.at(animation.id)[0] = 2;
        });
        reject(praise, "animation82 missing human reference", [&](auto &v) {
            v.facility_catalog_page_lists.at(animation.id).front() =
                std::numeric_limits<int>::max();
        });
    }
    {
        auto facility=baseline;
        for(auto &[id,owned]:facility.items) {
            owned.inventory=id>=0&&id<8?2:0;facility.catalog.at({0,id})=owned;
        }
        const auto source=std::find_if(facility.scene.world.world.facilities.begin(),facility.scene.world.world.facilities.end(),
            [](const auto &entry){return entry.second.placement.definition_id==33&&entry.second.status!=0;});
        if(source==facility.scene.world.world.facilities.end()||
           p::open_startup_world_facility_page(facility,source->first)!=p::StartupWorldRuntimeError::none||
           p::open_startup_world_facility_items(facility,facility.scripts.pages.back().id)!=p::StartupWorldRuntimeError::none)
            throw std::runtime_error("restore75 actual source74 unavailable");
        const auto directory=facility.scripts.pages.back().id;
        const auto tick=[&](auto &state) {
            auto next=p::prepare_startup_world_runtime(state);
            if(!next.candidate)throw std::runtime_error("restore75/76/77 actual framework rejected");
            state=std::move(*next.candidate);
        };
        expect(facility,true,"pending75 no preinstalled payload");
        auto broken=facility;broken.facility_item_page_scroll[directory]=0;
        expect(broken,false,"pending75 preinstalled scroll rejects");
        tick(facility);
        p::StartupFacilityItemInput input;input.up=true;
        if(p::input_startup_world_facility_item_page(facility,directory,input)!=p::StartupWorldRuntimeError::none||
           facility.facility_item_page_scroll.at(directory)!=3)
            throw std::runtime_error("restore75 actual tail navigation rejected");
        const auto roundtrip=[&](const auto &state,p::StartupFacilityItemInput action) {
            expect(state,true,"75/76/77 complete replay source");
            const auto bytes=p::persistence_detail::encode_state(state);
            auto restored=p::persistence_detail::decode_state(bytes,*state.rules),live=state;
            expect(restored,true,"75/76/77 replay recovered");
            if(p::persistence_detail::encode_state(restored)!=bytes)
                throw std::runtime_error("restore75/76/77 bytes diverged");
            const auto id=state.scripts.pages.back().id;
            const auto a=p::input_startup_world_facility_item_page(live,id,action),
                       b=p::input_startup_world_facility_item_page(restored,id,action);
            if(a!=p::StartupWorldRuntimeError::none||a!=b)
                throw std::runtime_error("restore75/76/77 input rejected");
            tick(live);tick(restored);
            if(p::persistence_detail::encode_state(live)!=p::persistence_detail::encode_state(restored))
                throw std::runtime_error("restore75/76/77 input/update/output differs");
        };
        input={};input.confirm=true;roundtrip(facility,input);
        auto retiring=facility;
        if(p::act_startup_world_facility_item_page(retiring,directory,p::StartupFacilityItemAction::cancel)!=p::StartupWorldRuntimeError::none)
            throw std::runtime_error("restore75 actual retirement rejected");
        expect(retiring,true,"retiring75 retains cursor until Finish");
        retiring.facility_item_page_scroll.erase(directory);
        expect(retiring,false,"retiring initialized75 still requires scroll payload");
        for(int fault=0;fault<6;++fault) {
            broken=facility;
            if(fault==0)broken.facility_item_page_scroll.erase(directory);
            if(fault==1)broken.facility_item_page_scroll.at(directory)=-1;
            if(fault==2)broken.facility_item_page_scroll.at(directory)=8;
            if(fault==3)broken.facility_item_page_selections.at(directory)=0;
            if(fault==4)broken.facility_item_page_scroll[facility.scripts.pages.front().id]=0;
            if(fault==5)broken.facility_page_bindings.erase(facility.scripts.pages[facility.scripts.pages.size()-2].id);
            expect(broken,false,"75 missing/offscreen/orphan/wrong-parent rejects");
        }
        if(p::input_startup_world_facility_item_page(facility,directory,input)!=p::StartupWorldRuntimeError::none)
            throw std::runtime_error("restore75 confirm rejected");
        for(int frame=0;frame<37;++frame)tick(facility);
        roundtrip(facility,{}); // 下一轮count38必须恰一次排声音8。
        for(int frame=37;frame<50;++frame)tick(facility);
        roundtrip(facility,input); // 77快进/输出/冻结改良与父75首行全部保持。
        if(p::input_startup_world_facility_item_page(facility,facility.scripts.pages.back().id,input)!=p::StartupWorldRuntimeError::none)
            throw std::runtime_error("restore77 fast forward rejected");
        for(int frame=0;frame<6;++frame)tick(facility);
        roundtrip(facility,input); // 退休77并返回75不重付、不重Init、不洗scroll。
    }
    {
        // 壶资格/库存是明确条件组合；页面载荷只准备最小结构，不宣称自然新局解锁。
        auto pot = baseline;
        pot.scripts.user_flags |= 1U;
        pot.items.at(0).inventory = 2;
        pot.catalog.at({0, 0}) = pot.items.at(0);
        const auto add = [&](auto &state, int raw, int binding = -1,
                             std::optional<std::uint64_t> parent = {}) {
            r::WorldScriptPage p;
            p.id = state.scripts.next_page_id++;
            p.kind = r::WorldScriptPageKind::raw_page;
            p.legacy_page = raw;
            p.legacy_g = binding;
            p.lifecycle = 0; // 对齐真实prepare_world_script_page producer，默认Page并非待Init态。
            state.scripts.pages.push_back(p);
            if (parent) state.magic_pot_page_parents.emplace(p.id, *parent);
            return p.id;
        };
        const auto root = add(pot, 41);
        // 41已看过首壶说明后的稳定结构：不注入脚本seen计数或重执初始化。
        pot.magic_pot_pages_initialized.insert(root);
        pot.magic_pot_page_data[root] = {0, 0, -1};
        pot.magic_pot_page_lists[root] = {};
        pot.page_phases[root] = 0;
        pot.page_counters[root] = 0;
        pot.scripts.pages.back().lifecycle = 2;
        expect(pot, true, "magic41 complete structural fixture");
        const auto reject = [&](const p::StartupWorldRuntimeState &source, const char *scenario,
                                const auto &damage) {
            auto broken = source;
            damage(broken);
            expect(broken, false, scenario);
        };
        reject(pot, "initialized magic41 missing data", [&](auto &v) { v.magic_pot_page_data.erase(root); });
        reject(pot, "initialized magic41 missing list", [&](auto &v) { v.magic_pot_page_lists.erase(root); });
        reject(pot, "initialized magic41 missing counter", [&](auto &v) { v.page_counters.erase(root); });
        reject(pot, "initialized magic41 missing phase", [&](auto &v) { v.page_phases.erase(root); });
        reject(pot, "magic payload on wrong raw page", [&](auto &v) { v.scripts.pages.back().legacy_page = 11; });
        reject(pot, "magic recipe missing definition progress", [&](auto &v) { v.magic_pot_recipes.erase(1); });
        reject(pot, "magic recipe identity mismatch", [&](auto &v) { v.magic_pot_recipes.at(1).identity = 2; });
        reject(pot, "magic recipe status2 unsupported", [&](auto &v) { v.magic_pot_recipes.at(1).status = 2; });
        reject(pot, "magic recipe reward catalog missing", [&](auto &v) { v.catalog.erase({0, 5}); });
        reject(pot, "magic level outside original range", [&](auto &v) { v.legacy_n[11] = 4; });
        reject(pot, "magic pending exceeds current capacity", [&](auto &v) { v.legacy_n[1] = 11; });
        reject(pot, "magic current element negative", [&](auto &v) { v.legacy_n[3] = -1; });
        reject(pot, "magic future processing timestamp", [&](auto &v) { v.legacy_n[12] = std::numeric_limits<int>::max(); });
        reject(pot, "magic display negative", [&](auto &v) { v.magic_pot_display[2][0] = -1; });
        reject(pot, "magic output negative", [&](auto &v) { v.magic_pot_output[0] = -1; });
        auto history = pot;
        history.magic_pot_display[0][0] = 1000;
        history.magic_pot_display[1][0] = 1;
        expect(history, true, "without44/45 consumer historical scratch has no active triple invariant");
        auto deposit = pot;
        const auto second = add(deposit, 42, -1, root);
        if (!p::initialize_startup_world_magic_pot_pages(deposit))
            throw std::runtime_error("restore fixture cannot initialize actual42 catalogue");
        deposit.scripts.pages.back().lifecycle = 2;
        expect(deposit, true, "magic42 inventory source and real parent41");
        reject(deposit, "magic42 missing parent", [&](auto &v) { v.magic_pot_page_parents.erase(second); });
        reject(deposit, "magic42 wrong parent", [&](auto &v) { v.magic_pot_page_parents.at(second) = v.scripts.pages.front().id; });
        if (p::act_startup_world_magic_pot_page(deposit, second, p::StartupMagicPotAction::confirm) != p::StartupWorldRuntimeError::none)
            throw std::runtime_error("restore fixture cannot produce44 through actual42 deposit");
        const auto animation = deposit.scripts.pages.back().id;
        if (!p::initialize_startup_world_magic_pot_pages(deposit))
            throw std::runtime_error("restore fixture cannot initialize44 binding");
        deposit.scripts.pages.back().lifecycle = 2;
        expect(deposit, true, "magic44 real item binding");
        reject(deposit, "magic44 invalid item binding", [&](auto &v) {
            v.scripts.pages.back().legacy_g = std::numeric_limits<int>::max();
            v.magic_pot_page_data.at(animation)[2] = std::numeric_limits<int>::max();
        });
        reject(deposit, "magic44 missing independent clock", [&](auto &v) { v.page_secondary_counters.erase(animation); });
        reject(deposit, "magic44 negative independent clock", [&](auto &v) { v.page_secondary_counters.at(animation) = -1; });
        reject(deposit, "magic44 overflowing independent clock", [&](auto &v) {
            v.page_secondary_counters.at(animation) = std::numeric_limits<int>::max();
        });
        reject(deposit, "magic44 missing frozen comment", [&](auto &v) { v.magic_pot_comment.clear(); });
        reject(deposit, "magic44 old value outside cap", [&](auto &v) { v.magic_pot_display[0][0] = 1000; });
        reject(deposit, "magic44 new value outside cap", [&](auto &v) { v.magic_pot_display[1][0] = 1000; });
        reject(deposit, "magic44 frozen result does not match request", [&](auto &v) { ++v.magic_pot_display[1][0]; });
        auto saturated44 = deposit;
        saturated44.magic_pot_display[0][0] = 998;
        saturated44.magic_pot_display[1][0] = 999;
        saturated44.magic_pot_display[2][0] = 4;
        expect(saturated44, true, "44 saturation retains original requested4 rather than actual difference1");
        saturated44.magic_pot_display[2][0] = std::numeric_limits<int>::max();
        expect(saturated44, true, "44 frozen saturation addition uses int64 without overflow");
        saturated44.magic_pot_display[0][4] = 1000;
        expect(saturated44, true, "44 does not consume the historical fifth knowledge scratch column");
        const auto replay = [&](const auto &source, std::uint64_t id, const p::StartupMagicPotInput &input) {
            expect(source, true, "magic complete replay source");
            const auto bytes = p::persistence_detail::encode_state(source);
            auto restored = p::persistence_detail::decode_state(bytes, *source.rules), live = source;
            expect(restored, true, "magic exact restored state");
            ++checks;
            if (p::persistence_detail::encode_state(restored) != bytes)
                throw std::runtime_error("magic restore changed existing page/global payload");
            const auto a = p::input_startup_world_magic_pot_page(live, id, input);
            const auto b = p::input_startup_world_magic_pot_page(restored, id, input);
            ++checks;
            if (a != p::StartupWorldRuntimeError::none || a != b ||
                p::persistence_detail::encode_state(live) != p::persistence_detail::encode_state(restored))
                throw std::runtime_error("magic replay input differs including common random/output");
            const auto next_live = p::prepare_startup_world_runtime(live);
            const auto next_restored = p::prepare_startup_world_runtime(restored);
            ++checks;
            if (!next_live.candidate || !next_restored.candidate ||
                p::persistence_detail::encode_state(*next_live.candidate) !=
                    p::persistence_detail::encode_state(*next_restored.candidate))
                throw std::runtime_error("magic replay update/retirement/output differs");
        };
        auto animated = deposit;
        animated.page_counters.at(animation) = 34;
        animated.page_secondary_counters.at(animation) = 17;
        animated.magic_pot_display[0] = {1, 2, 3, 4, 5};
        animated.magic_pot_display[1] = {3, 4, 5, 6, 5};
        animated.magic_pot_display[2] = {2, 2, 2, 2, 0};
        animated.magic_pot_comment = "就那样吧";
        replay(animated, animation, {}); // 下一获准轮35排一次SE8，副钟及冻结展示不重置。
        animated.page_counters.at(animation) = 36;
        p::StartupMagicPotInput magic_input; magic_input.confirm = true;
        replay(animated, animation, magic_input);
        auto closing44 = animated;
        magic_input = {}; magic_input.cancel = true;
        if (p::input_startup_world_magic_pot_page(closing44, animation, magic_input) != p::StartupWorldRuntimeError::none)
            throw std::runtime_error("magic44 actual close rejected");
        expect(closing44, true, "closed44 retains valid bound parent and complete clocks until Finish");
        reject(closing44, "closed44 missing secondary payload", [&](auto &v) { v.page_secondary_counters.erase(animation); });
        reject(closing44, "closed44 missing frozen binding payload", [&](auto &v) { v.magic_pot_page_data.erase(animation); });
        reject(closing44, "closed44 forged parent", [&](auto &v) { v.magic_pot_page_parents.at(animation) = root; });
        reject(closing44, "closed44 still requires frozen comment", [&](auto &v) { v.magic_pot_comment.clear(); });
        reject(closing44, "closed44 still requires valid result", [&](auto &v) { v.magic_pot_display[1][0] = 900; });
        const auto finished44 = p::prepare_startup_world_runtime(closing44);
        if (!finished44.candidate) throw std::runtime_error("closed44 actual Finish failed");
        expect(*finished44.candidate, true, "Finish releases44 page maps and its second clock");
        ++checks;
        if (finished44.candidate->magic_pot_pages_initialized.count(animation) ||
            finished44.candidate->magic_pot_page_data.count(animation) ||
            finished44.candidate->magic_pot_page_lists.count(animation) ||
            finished44.candidate->magic_pot_page_parents.count(animation) ||
            finished44.candidate->page_secondary_counters.count(animation))
            throw std::runtime_error("magic44 Finish retained transient payload");
        auto inputs42 = pot;
        for (std::size_t row = 0; row < inputs42.rules->items.size(); ++row) {
            const auto definition = inputs42.rules->items[row].identity;
            inputs42.items.at(definition).inventory = row < 7 ? 2 : 0;
            inputs42.catalog.at({0, definition}) = inputs42.items.at(definition);
        }
        const auto inputs42_id = add(inputs42, 42, -1, root);
        if (!p::initialize_startup_world_magic_pot_pages(inputs42))
            throw std::runtime_error("magic42 replay Init failed");
        inputs42.scripts.pages.back().lifecycle = 2;
        inputs42.magic_pot_page_data.at(inputs42_id) = {6, 6, -1};
        magic_input = {}; magic_input.up = magic_input.down = magic_input.confirm = true;
        replay(inputs42, inputs42_id, magic_input); // 同一随机评语、投入结果和实际尾首行。
        auto recipes = pot;
        recipes.magic_pot_recipes.at(1).status = 1; // 47只绑定已发现的配方条件夹具。
        const auto third = add(recipes, 43, -1, root);
        if (!p::initialize_startup_world_magic_pot_pages(recipes))
            throw std::runtime_error("restore fixture cannot initialize43 catalogue");
        recipes.scripts.pages.back().lifecycle = 2;
        const auto confirmation = add(recipes, 47, 1, third);
        if (!p::initialize_startup_world_magic_pot_pages(recipes))
            throw std::runtime_error("restore fixture cannot initialize47 binding");
        recipes.scripts.pages.back().lifecycle = 2;
        expect(recipes, true, "magic47 recipe binding and real parent43");
        reject(recipes, "magic47 wrong parent type", [&](auto &v) { v.magic_pot_page_parents.at(confirmation) = root; });
        reject(recipes, "magic47 invalid recipe binding", [&](auto &v) {
            v.scripts.pages.back().legacy_g = std::numeric_limits<int>::max();
            v.magic_pot_page_data.at(confirmation)[2] = std::numeric_limits<int>::max();
        });
        auto closing47 = recipes;
        magic_input = {}; magic_input.cancel = true;
        if (p::input_startup_world_magic_pot_page(closing47, confirmation, magic_input) != p::StartupWorldRuntimeError::none)
            throw std::runtime_error("magic47 actual cancel failed");
        expect(closing47, true, "closed47 retains real selected43 parent until Finish");
        reject(closing47, "closed47 missing list", [&](auto &v) { v.magic_pot_page_lists.erase(confirmation); });
        reject(closing47, "closed47 missing counter", [&](auto &v) { v.page_counters.erase(confirmation); });
        reject(closing47, "closed47 wrong parent", [&](auto &v) { v.magic_pot_page_parents.at(confirmation) = root; });
        for (const int definition : {7, 35}) {
            auto rewarding = recipes;
            const auto row = std::find(rewarding.magic_pot_page_lists.at(third).begin(),
                rewarding.magic_pot_page_lists.at(third).end(), definition);
            if (row == rewarding.magic_pot_page_lists.at(third).end())
                throw std::runtime_error("magic47 reward fixture needs real catalogue recipe");
            rewarding.magic_pot_recipes.at(definition).status = 1;
            rewarding.scripts.pages.back().legacy_g = definition;
            rewarding.magic_pot_page_data.at(confirmation)[2] = definition;
            const auto selected = static_cast<int>(row - rewarding.magic_pot_page_lists.at(third).begin());
            rewarding.magic_pot_page_data.at(third) = {selected, selected, -1};
            for (int col = 3; col < 7; ++col) rewarding.legacy_n[col] = 999;
            rewarding.page_counters.at(confirmation) = 7;
            magic_input = {}; magic_input.confirm = true;
            replay(rewarding, confirmation, magic_input);
            if (p::input_startup_world_magic_pot_page(rewarding, confirmation, magic_input) != p::StartupWorldRuntimeError::none)
                throw std::runtime_error("magic47 actual reward confirmation failed");
            expect(rewarding, true, "closed47 with real107/item-or-facility reward preserves parent anchor");
            reject(rewarding, "rewarded47 still requires closed phase", [&](auto &v) { v.page_phases.erase(confirmation); });
        }
        auto rows = pot;
        const auto rows_id = add(rows, 43, -1, root);
        if (!p::initialize_startup_world_magic_pot_pages(rows))
            throw std::runtime_error("magic replay43 actual Init failed");
        rows.scripts.pages.back().lifecycle = 2;
        const auto final_row = static_cast<int>(rows.magic_pot_page_lists.at(rows_id).size()) - 1;
        rows.magic_pot_page_data.at(rows_id) = {final_row, final_row, -1};
        rows.page_phases.at(rows_id) = 1;
        magic_input = {}; magic_input.up = magic_input.down = magic_input.left = magic_input.right = true;
        replay(rows, rows_id, magic_input); // 合法短尾窗与页签往返后同输入不变。
        reject(rows, "magic43 offscreen selected row", [&](auto &v) { v.magic_pot_page_data.at(rows_id)[0] = 0; });
        reject(rows, "magic43 unowned second clock", [&](auto &v) { v.page_secondary_counters[rows_id] = 0; });
        for (int raw : {45, 46}) {
            auto result = baseline;
            const auto id = add(result, raw, raw == 46 ? 1 : -1);
            if (!p::initialize_startup_world_magic_pot_pages(result))
                throw std::runtime_error("restore fixture cannot initialize45/46 result");
            result.scripts.pages.back().lifecycle = 2;
            expect(result, true, raw == 45 ? "magic45 queued result" : "magic46 undiscovered recipe");
            if (raw == 45) {
                reject(result, "45 knowledge old value outside cap", [&](auto &v) { v.magic_pot_display[0][4] = 1000; });
                reject(result, "45 knowledge frozen result mismatch", [&](auto &v) { v.magic_pot_display[1][4] = 1; });
                reject(result, "45 recent processed above original capacity", [&](auto &v) { v.legacy_n[2] = 11; });
                result.page_counters.at(id) = 76;
                magic_input = {}; magic_input.confirm = true;
                replay(result, id, magic_input);
                auto closing45 = result;closing45.page_counters.at(id) = 83;
                if (p::input_startup_world_magic_pot_page(closing45, id, magic_input) != p::StartupWorldRuntimeError::none)
                    throw std::runtime_error("45 actual close failed");
                expect(closing45, true, "closed45 retains complete result until Finish");
                reject(closing45, "closed45 knowledge mismatch rejects", [&](auto &v) { v.magic_pot_display[1][4] = 1; });
            }
            reject(result, "magic result page cannot acquire fake parent", [&](auto &v) {
                v.magic_pot_page_parents[id] = v.scripts.pages.front().id;
            });
        }
        auto retired = baseline;
        const auto stale = retired.scripts.next_page_id++;
        reject(retired, "retired magic initialized identity", [&](auto &v) { v.magic_pot_pages_initialized.insert(stale); });
        reject(retired, "retired magic data map", [&](auto &v) { v.magic_pot_page_data[stale] = {0, 0, -1}; });
        reject(retired, "retired magic list map", [&](auto &v) { v.magic_pot_page_lists[stale] = {}; });
        reject(retired, "retired magic parent map", [&](auto &v) { v.magic_pot_page_parents[stale] = root; });
    }
    {
        using E=p::StartupWorldRuntimeError;
        using A=p::StartupVillageActivityAction;
        const auto require=[&](bool value,const char *message) {
            ++checks;if(!value)throw std::runtime_error(std::string("restore town progression: ")+message);
        };
        const auto tick=[&](auto &state) {
            auto next=p::prepare_startup_world_runtime(state);
            require(next.candidate.has_value(),"framework update rejected");state=std::move(*next.candidate);
        };
        const auto top=[](const auto &state) -> const r::WorldScriptPage & {
            const auto found=std::find_if(state.scripts.pages.rbegin(),state.scripts.pages.rend(),
                [](const auto &page){return page.lifecycle!=4;});
            if(found==state.scripts.pages.rend())throw std::runtime_error("town replay missing active page");
            return *found;
        };
        // 同输入／下一获准更新比较完整编码，连同随机及待消费输出，不重新生成世界前缀。
        const auto replay=[&](auto &state,const auto &action) {
            expect(state,true,"town source before short replay");
            const auto wire=p::persistence_detail::encode_state(state);
            auto restored=p::persistence_detail::decode_state(wire,*state.rules);
            expect(restored,true,"town decoded before short replay");
            require(p::persistence_detail::encode_state(restored)==wire,"decode changed cached page state");
            action(state);action(restored);tick(state);tick(restored);
            require(p::persistence_detail::encode_state(state)==p::persistence_detail::encode_state(restored),
                    "same input/update diverged in state, random or output");
        };
        const auto choose=[&](auto &state,int tag) {
            const auto id=top(state).id;const auto view=p::inspect_startup_world_menu_page(state,id);
            require(view.has_value(),"menu source missing");
            const auto row=std::find(view->tags.begin(),view->tags.end(),tag);
            require(row!=view->tags.end(),"menu tag missing");
            p::StartupWorldMenuInput input;input.select_row=static_cast<int>(row-view->tags.begin());
            require(p::input_startup_world_menu_page(state,id,input)==E::none,"menu selection rejected");
            input={};input.confirm=true;
            require(p::input_startup_world_menu_page(state,id,input)==E::none,"menu confirmation rejected");
        };
        const auto enter=[&](auto &state,int tag) {
            require(p::open_startup_world_main_menu(state)==E::none,"open actual main menu");
            tick(state);choose(state,2);tick(state);choose(state,tag);
        };
        auto village=baseline;
        // 解锁／预算仅为多行条件；菜单、确认、效果及后继均走实际Owner。
        village.village_points=999;village.quarter_counter=3;village.scripts.event_calls[100]=1;
        for(int n=0;n<8;++n)village.scripts.activities.at(n).status=1;
        enter(village,12);const auto directory=top(village).id;
        expect(village,true,"actual menu-to51 pending state");
        for(int fault=0;fault<4;++fault) {
            auto bad=village;
            if(fault==0)bad.activity_page_scroll[directory]=0;
            if(fault==1)bad.activity_page_selections[directory]=0;
            if(fault==2)bad.activity_page_lists[directory]={0};
            if(fault==3)bad.page_counters[directory]=0;
            expect(bad,false,"pending51 refuses half-installed directory");
        }
        replay(village,[](auto &){});
        p::StartupVillageActivityInput navigation;navigation.up=true;
        replay(village,[&](auto &v){require(p::input_startup_world_village_activity_page(v,directory,navigation)==E::none,"51 tail selection");});
        const auto first=village.activity_page_scroll.at(directory);
        const auto draws=village.scene.random.draws();
        replay(village,[&](auto &v){require(p::act_startup_world_village_activity_page(v,directory,A::confirm)==E::none,"51 opens52");});
        const auto question=top(village).id;
        for(int fault=0;fault<3;++fault) {
            auto bad=village;
            if(fault==0)bad.activity_page_parents.erase(question);
            if(fault==1)bad.activity_page_parents.at(question)=bad.scripts.pages.front().id;
            if(fault==2)bad.activity_page_selections.at(question)=2;
            expect(bad,false,"52 rejects missing/wrong parent or invalid answer selection");
        }
        replay(village,[&](auto &v){require(p::act_startup_world_village_activity_page(v,question,A::cancel)==E::none,"52 actual cancellation");});
        require(top(village).id==directory&&village.activity_page_scroll.at(directory)==first&&
                    village.scene.random.draws()==draws&&!village.activity_page_parents.count(question),
                "cancel52 must restore historical51 viewport without reshuffling");
        const auto row=std::find(village.activity_page_lists.at(directory).begin(),village.activity_page_lists.at(directory).end(),0);
        require(row!=village.activity_page_lists.at(directory).end(),"type0 activity exists");
        navigation={};navigation.selection=static_cast<int>(row-village.activity_page_lists.at(directory).begin());navigation.confirm=true;
        replay(village,[&](auto &v){require(p::input_startup_world_village_activity_page(v,directory,navigation)==E::none,"select and open type0");});
        const auto offer=top(village).id;
        replay(village,[&](auto &v){require(p::act_startup_world_village_activity_page(v,offer,A::confirm)==E::none,"52 pays and creates53");});
        const auto animation=top(village).id;
        require(top(village).legacy_page==53,"actual53 follows payment");
        auto bad=village;bad.page_counters.erase(animation);expect(bad,false,"initialized53 counter missing");
        village.page_counters.at(animation)=119; // 相位门槛条件，不等待世界日期。
        replay(village,[](auto &){});
        replay(village,[&](auto &v){require(p::act_startup_world_village_activity_page(v,animation,A::confirm)==E::none,"53 submits at120");});
        const auto result=top(village).id;
        require(top(village).legacy_page==54,"type0 completion creates54 with producer-owned display pair");
        bad=village;bad.activity_page_display_humans.erase(result);expect(bad,false,"54 frozen display pair missing");
        replay(village,[&](auto &v){require(p::act_startup_world_village_activity_page(v,result,A::cancel)==E::none,"54 closes without reapplying effects");});
        require(top(village).id==directory&&!village.activity_page_lists.count(result)&&
                    !village.activity_page_display_humans.count(result),"54 retirement frees frozen references");
        replay(village,[&](auto &v){require(p::act_startup_world_village_activity_page(v,directory,A::cancel)==E::none,"close51");});
        require(village.activity_pages_initialized.empty()&&village.activity_page_scroll.empty(),"village chain retires all page payloads");

        auto rank=baseline;rank.rank=1;rank.scripts.user_flags|=8U; // 晋级目录开放条件；不伪称自然取得。
        enter(rank,10);tick(rank);const auto rank_id=top(rank).id;
        p::StartupTownRankInput select;select.selection=2;
        replay(rank,[&](auto &v){require(p::input_startup_world_runtime_rank_page(v,rank_id,select)==E::none,"48 stores condition selection");});
        require(rank.page_phases.at(rank_id)==2,"48 selection survives same-input restore");
        select={};select.confirm=true;
        replay(rank,[&](auto &v){require(p::input_startup_world_runtime_rank_page(v,rank_id,select)==E::none,"48 opens selected condition explanation");});
        for(int n=0;n<16&&top(rank).id!=rank_id;++n) {
            const auto explanation=top(rank);
            require(explanation.kind==r::WorldScriptPageKind::dialogue||
                        explanation.kind==r::WorldScriptPageKind::simple_message,
                    "48 condition explanation uses a real script message");
            require(p::acknowledge_startup_world_runtime_page(rank,explanation.id)==E::none,
                    "48 explanation returns through actual confirmation");tick(rank);
        }
        require(top(rank).id==rank_id&&rank.page_phases.at(rank_id)==2,
                "48 condition explanation preserves the parent selection");
        for(int fault=0;fault<3;++fault) {
            bad=rank;if(fault==0)bad.page_phases.erase(rank_id);
            if(fault==1)bad.page_counters.erase(rank_id);
            if(fault==2)bad.page_phases.at(rank_id)=5;
            expect(bad,false,"48 missing or invalid selection rejects");
        }
        // 复用真实晋级→95活动30后继；只设置已满足的晋级显示缓存。
        rank.rank_met.fill(true);select={};select.selection=0;select.confirm=true;
        replay(rank,[&](auto &v){require(p::input_startup_world_runtime_rank_page(v,rank_id,select)==E::none,"48 promotes from condition fixture");});
        const auto advance_to=[&](int raw) {
            for(int n=0;n<600;++n) {
                const auto page=top(rank);
                if(page.kind==r::WorldScriptPageKind::raw_page&&page.legacy_page==raw&&page.lifecycle!=0)return page.id;
                if(page.kind==r::WorldScriptPageKind::dialogue||page.kind==r::WorldScriptPageKind::simple_message||
                   page.kind==r::WorldScriptPageKind::newspaper||
                   (page.kind==r::WorldScriptPageKind::raw_page&&(page.legacy_page==50||page.legacy_page==11)))
                    require(p::acknowledge_startup_world_runtime_page(rank,page.id)==E::none,"actual promotion message confirmation");
                tick(rank);
            }
            throw std::runtime_error("town replay promotion exceeded bounded callbacks");
        };
        const auto ceremony=advance_to(50);
        for(int phase=0;phase<3;++phase) {
            require(rank.page_phases.at(ceremony)==phase,"50 phase expected");
            const int secondary=rank.page_secondary_counters.at(ceremony);
            rank.page_counters.at(ceremony)=phase==0?134:phase==1?19:139;
            replay(rank,[](auto &){});
            require(rank.page_secondary_counters.at(ceremony)==secondary+1,"50 boundary preserves independent clock");
        }
        bad=rank;bad.rank_celebration_participants.erase(ceremony);expect(bad,false,"50 missing cast cannot silently reshuffle");
        bad=rank;bad.page_secondary_counters.erase(ceremony);expect(bad,false,"50 missing independent clock");
        replay(rank,[&](auto &v){require(p::acknowledge_startup_world_runtime_page(v,ceremony)==E::none,"50 final confirmation");});
        const auto reward=advance_to(95);
        require(top(rank).legacy_r==11&&top(rank).legacy_s==30&&rank.scripts.activities.at(30).status==0,
                "real delayed rank reward keeps activity30 locked until claim");
        bad=rank;bad.page_counters.erase(reward);expect(bad,false,"95 waiting claim missing counter rejects");
        replay(rank,[&](auto &v){require(p::acknowledge_startup_world_runtime_page(v,reward)==E::none,"95 early confirmation");});
        require(rank.scripts.activities.at(30).status==0,"95 early confirmation cannot grant");
        replay(rank,[&](auto &v){require(p::acknowledge_startup_world_runtime_page(v,reward)==E::none,"95 final claim");});
        require(rank.scripts.activities.at(30).status==1&&rank.activity_counts.at(30)==0&&
                    (rank.scripts.user_flags&3U)==0&&!rank.page_counters.count(reward)&&
                    p::acknowledge_startup_world_runtime_page(rank,reward)!=E::none,
                "95 grants activity once, retires and leaves actual holding/pot introduction to existing owner chain");
    }
    return checks;
}
