#include "startup_world_restore_checks.hpp"

#include "../../src/simulation/persistence/startup_world_restore_validation.hpp"
#include "../../src/simulation/persistence/startup_world_codec.hpp"
#include "ark/simulation/facilities/startup_world_building.hpp"
#include "ark/simulation/facilities/startup_world_commerce.hpp"
#include "ark/simulation/facilities/startup_world_facility_catalog.hpp"
#include "ark/simulation/facilities/startup_world_magic_pot.hpp"
#include "ark/simulation/village/startup_world_information.hpp"
#include "ark/simulation/actors/startup_world_human.hpp"

#include <algorithm>

#include <limits>
#include <stdexcept>

int check_startup_world_restore_contracts(
    const ark::simulation::StartupWorldRuntimeState &baseline) {
    namespace p = ark::simulation;
    namespace r = ark::simulation::rules;
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
        auto shop = baseline;
        shop.scripts.pages.back().lifecycle = 3;
        r::WorldScriptPage page;
        page.id = shop.scripts.next_page_id++;
        page.kind = r::WorldScriptPageKind::raw_page;
        page.legacy_page = 85;
        shop.scripts.pages.push_back(page);
        if (!p::initialize_startup_world_commerce_pages(shop))
            throw std::runtime_error("restore fixture cannot initialize blueprint catalogue");
        expect(shop, true, "unopened blueprint catalogue restore");
        for (int presence : {1, 2}) {
            auto stale = shop;
            const int definition = stale.commerce_page_lists.at(page.id).front();
            stale.facility_presence.at(definition) = presence;
            expect(stale, false, "restored catalogue rejects already unlocked blueprint");
        }
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
        for(int fault=0;fault<4;++fault) {
            auto invalid=restored;
            if(fault==0)invalid.information_page_data.erase(directory);
            if(fault==1)invalid.page_phases.at(directory)=4;
            if(fault==2)invalid.information_page_data.at(directory).lists[0].push_back(definition);
            if(fault==3)invalid.information_page_data.at(directory).lists[0].clear();
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
        auto deposit = pot;
        const auto second = add(deposit, 42, -1, root);
        if (!p::initialize_startup_world_magic_pot_pages(deposit))
            throw std::runtime_error("restore fixture cannot initialize actual42 catalogue");
        deposit.scripts.pages.back().lifecycle = 2;
        expect(deposit, true, "magic42 inventory source and real parent41");
        reject(deposit, "magic42 missing parent", [&](auto &v) { v.magic_pot_page_parents.erase(second); });
        reject(deposit, "magic42 wrong parent", [&](auto &v) { v.magic_pot_page_parents.at(second) = v.scripts.pages.front().id; });
        const auto animation = add(deposit, 44, 0, second);
        if (!p::initialize_startup_world_magic_pot_pages(deposit))
            throw std::runtime_error("restore fixture cannot initialize44 binding");
        deposit.scripts.pages.back().lifecycle = 2;
        expect(deposit, true, "magic44 real item binding");
        reject(deposit, "magic44 invalid item binding", [&](auto &v) {
            v.scripts.pages.back().legacy_g = std::numeric_limits<int>::max();
            v.magic_pot_page_data.at(animation)[2] = std::numeric_limits<int>::max();
        });
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
        for (int raw : {45, 46}) {
            auto result = baseline;
            const auto id = add(result, raw, raw == 46 ? 1 : -1);
            if (!p::initialize_startup_world_magic_pot_pages(result))
                throw std::runtime_error("restore fixture cannot initialize45/46 result");
            result.scripts.pages.back().lifecycle = 2;
            expect(result, true, raw == 45 ? "magic45 queued result" : "magic46 undiscovered recipe");
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
    return checks;
}
