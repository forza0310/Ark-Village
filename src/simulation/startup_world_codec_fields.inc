// 由 scripts/generate_owner_codec.mjs 生成；不要手改。
constexpr const char schema_identity[] = "0500cff0cd937c6967836c6bf7c594ff43dd64c23f408e7ea9188e4fc3423403";
template<> bool valid_enum< ::ark::simulation::rules::ActorKind>(::ark::simulation::rules::ActorKind value) { return value == ::ark::simulation::rules::ActorKind::human || value == ::ark::simulation::rules::ActorKind::monster; }
template<> bool valid_enum< ::ark::simulation::rules::CandidateOrigin>(::ark::simulation::rules::CandidateOrigin value) { return value == ::ark::simulation::rules::CandidateOrigin::map_scan || value == ::ark::simulation::rules::CandidateOrigin::event; }
template<> bool valid_enum< ::ark::simulation::rules::CashCategory>(::ark::simulation::rules::CashCategory value) { return value == ::ark::simulation::rules::CashCategory::facilities || value == ::ark::simulation::rules::CashCategory::monsters || value == ::ark::simulation::rules::CashCategory::adventurers || value == ::ark::simulation::rules::CashCategory::shop || value == ::ark::simulation::rules::CashCategory::other; }
template<> bool valid_enum< ::ark::simulation::rules::CashDirection>(::ark::simulation::rules::CashDirection value) { return value == ::ark::simulation::rules::CashDirection::income || value == ::ark::simulation::rules::CashDirection::expense; }
template<> bool valid_enum< ::ark::simulation::rules::DungeonCrewRequestKind>(::ark::simulation::rules::DungeonCrewRequestKind value) { return value == ::ark::simulation::rules::DungeonCrewRequestKind::retreat || value == ::ark::simulation::rules::DungeonCrewRequestKind::progress_label231 || value == ::ark::simulation::rules::DungeonCrewRequestKind::grant_catalog_reward || value == ::ark::simulation::rules::DungeonCrewRequestKind::spawn_catalog_reward || value == ::ark::simulation::rules::DungeonCrewRequestKind::exit_crew; }
template<> bool valid_enum< ::ark::simulation::rules::FacilityOrientation>(::ark::simulation::rules::FacilityOrientation value) { return value == ::ark::simulation::rules::FacilityOrientation::first || value == ::ark::simulation::rules::FacilityOrientation::second; }
template<> bool valid_enum< ::ark::simulation::rules::FacilityShape>(::ark::simulation::rules::FacilityShape value) { return value == ::ark::simulation::rules::FacilityShape::single || value == ::ark::simulation::rules::FacilityShape::pair || value == ::ark::simulation::rules::FacilityShape::square; }
template<> bool valid_enum< ::ark::simulation::rules::MapAccessError>(::ark::simulation::rules::MapAccessError value) { return value == ::ark::simulation::rules::MapAccessError::none || value == ::ark::simulation::rules::MapAccessError::invalid_map || value == ::ark::simulation::rules::MapAccessError::invalid_position || value == ::ark::simulation::rules::MapAccessError::invalid_limits || value == ::ark::simulation::rules::MapAccessError::invalid_layout || value == ::ark::simulation::rules::MapAccessError::unsupported_kind || value == ::ark::simulation::rules::MapAccessError::invalid_field || value == ::ark::simulation::rules::MapAccessError::binding_mismatch || value == ::ark::simulation::rules::MapAccessError::unreachable || value == ::ark::simulation::rules::MapAccessError::cost_limit || value == ::ark::simulation::rules::MapAccessError::expansion_limit; }
template<> bool valid_enum< ::ark::simulation::rules::ProjectileKind>(::ark::simulation::rules::ProjectileKind value) { return value == ::ark::simulation::rules::ProjectileKind::arrow || value == ::ark::simulation::rules::ProjectileKind::spell || value == ::ark::simulation::rules::ProjectileKind::delayed_damage; }
template<> bool valid_enum< ::ark::simulation::rules::RouteCategory>(::ark::simulation::rules::RouteCategory value) { return value == ::ark::simulation::rules::RouteCategory::terminal || value == ::ark::simulation::rules::RouteCategory::road || value == ::ark::simulation::rules::RouteCategory::ground || value == ::ark::simulation::rules::RouteCategory::blocked || value == ::ark::simulation::rules::RouteCategory::access; }
template<> bool valid_enum< ::ark::simulation::rules::WorldScriptPageKind>(::ark::simulation::rules::WorldScriptPageKind value) { return value == ::ark::simulation::rules::WorldScriptPageKind::scene || value == ::ark::simulation::rules::WorldScriptPageKind::dialogue || value == ::ark::simulation::rules::WorldScriptPageKind::simple_message || value == ::ark::simulation::rules::WorldScriptPageKind::newspaper || value == ::ark::simulation::rules::WorldScriptPageKind::raw_page; }
template<> struct CodecFields< ::ark::simulation::StartupWorldActorMetadata> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::StartupWorldActorMetadata.sex", v.sex);
  a.field("ark::simulation::StartupWorldActorMetadata.profession", v.profession);
  a.field("ark::simulation::StartupWorldActorMetadata.weapon", v.weapon);
  a.field("ark::simulation::StartupWorldActorMetadata.cached_view", v.cached_view);
  a.field("ark::simulation::StartupWorldActorMetadata.render_position", v.render_position);
  a.field("ark::simulation::StartupWorldActorMetadata.cached_screen_position", v.cached_screen_position);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::StartupWorldActorMetadata>().sex)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldActorMetadata>().profession)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldActorMetadata>().weapon)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldActorMetadata>().cached_view)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldActorMetadata>().render_position)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldActorMetadata>().cached_screen_position)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::StartupWorldFocusActor> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::StartupWorldFocusActor.actor", v.actor);
  a.field("ark::simulation::StartupWorldFocusActor.perception", v.perception);
  a.field("ark::simulation::StartupWorldFocusActor.routes", v.routes);
  a.field("ark::simulation::StartupWorldFocusActor.metadata", v.metadata);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::StartupWorldFocusActor>().actor)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldFocusActor>().perception)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldFocusActor>().routes)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldFocusActor>().metadata)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::StartupWorldHumanCalendar> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::StartupWorldHumanCalendar.absent_months", v.absent_months);
  a.field("ark::simulation::StartupWorldHumanCalendar.yearly_totals", v.yearly_totals);
  a.field("ark::simulation::StartupWorldHumanCalendar.legacy_F", v.legacy_F);
  a.field("ark::simulation::StartupWorldHumanCalendar.legacy_G", v.legacy_G);
  a.field("ark::simulation::StartupWorldHumanCalendar.continuation_cost", v.continuation_cost);
  a.field("ark::simulation::StartupWorldHumanCalendar.contribution", v.contribution);
  a.field("ark::simulation::StartupWorldHumanCalendar.celebrations", v.celebrations);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::StartupWorldHumanCalendar>().absent_months)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldHumanCalendar>().yearly_totals)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldHumanCalendar>().legacy_F)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldHumanCalendar>().legacy_G)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldHumanCalendar>().continuation_cost)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldHumanCalendar>().contribution)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldHumanCalendar>().celebrations)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::StartupWorldRuntimeState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::StartupWorldRuntimeState.scene", v.scene);
  // rules静态目录由decode参数重绑定，不编码地址。
  a.field("ark::simulation::StartupWorldRuntimeState.task", v.task);
  a.field("ark::simulation::StartupWorldRuntimeState.shop_humans", v.shop_humans);
  a.field("ark::simulation::StartupWorldRuntimeState.shop_actors", v.shop_actors);
  a.field("ark::simulation::StartupWorldRuntimeState.items", v.items);
  a.field("ark::simulation::StartupWorldRuntimeState.dungeon_facilities", v.dungeon_facilities);
  a.field("ark::simulation::StartupWorldRuntimeState.dungeon_actors", v.dungeon_actors);
  a.field("ark::simulation::StartupWorldRuntimeState.catalog", v.catalog);
  a.field("ark::simulation::StartupWorldRuntimeState.shops", v.shops);
  a.field("ark::simulation::StartupWorldRuntimeState.shop_order", v.shop_order);
  a.field("ark::simulation::StartupWorldRuntimeState.item_rewards", v.item_rewards);
  a.field("ark::simulation::StartupWorldRuntimeState.human_definition_state", v.human_definition_state);
  a.field("ark::simulation::StartupWorldRuntimeState.human_homes", v.human_homes);
  a.field("ark::simulation::StartupWorldRuntimeState.human_presence", v.human_presence);
  a.field("ark::simulation::StartupWorldRuntimeState.human_flags", v.human_flags);
  a.field("ark::simulation::StartupWorldRuntimeState.human_calendar", v.human_calendar);
  a.field("ark::simulation::StartupWorldRuntimeState.actor_metadata", v.actor_metadata);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_original_ids", v.facility_original_ids);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_ordinals", v.facility_ordinals);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_residents", v.facility_residents);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_difficulties", v.facility_difficulties);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_flags", v.facility_flags);
  a.field("ark::simulation::StartupWorldRuntimeState.neighbourhood", v.neighbourhood);
  a.field("ark::simulation::StartupWorldRuntimeState.neighbourhood_details", v.neighbourhood_details);
  a.field("ark::simulation::StartupWorldRuntimeState.exploration_summaries", v.exploration_summaries);
  a.field("ark::simulation::StartupWorldRuntimeState.exploration_displays", v.exploration_displays);
  a.field("ark::simulation::StartupWorldRuntimeState.dungeon_labels", v.dungeon_labels);
  a.field("ark::simulation::StartupWorldRuntimeState.surface", v.surface);
  a.field("ark::simulation::StartupWorldRuntimeState.road_patches", v.road_patches);
  a.field("ark::simulation::StartupWorldRuntimeState.scripts", v.scripts);
  a.field("ark::simulation::StartupWorldRuntimeState.cash_peak", v.cash_peak);
  a.field("ark::simulation::StartupWorldRuntimeState.cash_peak_village", v.cash_peak_village);
  a.field("ark::simulation::StartupWorldRuntimeState.monthly_cash", v.monthly_cash);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_monthly_cash", v.facility_monthly_cash);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_month_age", v.facility_month_age);
  a.field("ark::simulation::StartupWorldRuntimeState.shop_item_stock", v.shop_item_stock);
  a.field("ark::simulation::StartupWorldRuntimeState.report_state", v.report_state);
  a.field("ark::simulation::StartupWorldRuntimeState.report_counter", v.report_counter);
  a.field("ark::simulation::StartupWorldRuntimeState.report_snapshot", v.report_snapshot);
  a.field("ark::simulation::StartupWorldRuntimeState.report_records", v.report_records);
  a.field("ark::simulation::StartupWorldRuntimeState.report_new_records", v.report_new_records);
  a.field("ark::simulation::StartupWorldRuntimeState.report_portraits", v.report_portraits);
  a.field("ark::simulation::StartupWorldRuntimeState.report_kills", v.report_kills);
  a.field("ark::simulation::StartupWorldRuntimeState.maximum_income", v.maximum_income);
  a.field("ark::simulation::StartupWorldRuntimeState.village_points", v.village_points);
  a.field("ark::simulation::StartupWorldRuntimeState.popularity", v.popularity);
  a.field("ark::simulation::StartupWorldRuntimeState.maximum_popularity", v.maximum_popularity);
  a.field("ark::simulation::StartupWorldRuntimeState.popularity_display", v.popularity_display);
  a.field("ark::simulation::StartupWorldRuntimeState.popularity_pulses", v.popularity_pulses);
  a.field("ark::simulation::StartupWorldRuntimeState.popularity_rewards", v.popularity_rewards);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_definitions", v.facility_definitions);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_presence", v.facility_presence);
  a.field("ark::simulation::StartupWorldRuntimeState.residence_catalog_available", v.residence_catalog_available);
  a.field("ark::simulation::StartupWorldRuntimeState.page_human_bindings", v.page_human_bindings);
  a.field("ark::simulation::StartupWorldRuntimeState.task_abort_questions", v.task_abort_questions);
  a.field("ark::simulation::StartupWorldRuntimeState.task_abort_answers", v.task_abort_answers);
  a.field("ark::simulation::StartupWorldRuntimeState.human_pages_initialized", v.human_pages_initialized);
  a.field("ark::simulation::StartupWorldRuntimeState.human_profession_changes", v.human_profession_changes);
  a.field("ark::simulation::StartupWorldRuntimeState.human_page_catalogs", v.human_page_catalogs);
  a.field("ark::simulation::StartupWorldRuntimeState.equipment_page_catalogs", v.equipment_page_catalogs);
  a.field("ark::simulation::StartupWorldRuntimeState.human_page_selections", v.human_page_selections);
  a.field("ark::simulation::StartupWorldRuntimeState.page_job_bindings", v.page_job_bindings);
  a.field("ark::simulation::StartupWorldRuntimeState.human_page_parents", v.human_page_parents);
  a.field("ark::simulation::StartupWorldRuntimeState.human_page_answers", v.human_page_answers);
  a.field("ark::simulation::StartupWorldRuntimeState.human_equipment_choices", v.human_equipment_choices);
  a.field("ark::simulation::StartupWorldRuntimeState.human_attribute_display", v.human_attribute_display);
  a.field("ark::simulation::StartupWorldRuntimeState.equipment_attribute_display", v.equipment_attribute_display);
  a.field("ark::simulation::StartupWorldRuntimeState.human_gift_scores", v.human_gift_scores);
  a.field("ark::simulation::StartupWorldRuntimeState.human_gift_messages", v.human_gift_messages);
  a.field("ark::simulation::StartupWorldRuntimeState.tax_page_residents", v.tax_page_residents);
  a.field("ark::simulation::StartupWorldRuntimeState.tax_page_selection", v.tax_page_selection);
  a.field("ark::simulation::StartupWorldRuntimeState.tax_page_scroll", v.tax_page_scroll);
  a.field("ark::simulation::StartupWorldRuntimeState.residence_hint_counter", v.residence_hint_counter);
  a.field("ark::simulation::StartupWorldRuntimeState.activity_flags", v.activity_flags);
  a.field("ark::simulation::StartupWorldRuntimeState.activity_counts", v.activity_counts);
  a.field("ark::simulation::StartupWorldRuntimeState.human_activity_previous", v.human_activity_previous);
  a.field("ark::simulation::StartupWorldRuntimeState.activity_pages_initialized", v.activity_pages_initialized);
  a.field("ark::simulation::StartupWorldRuntimeState.activity_page_bindings", v.activity_page_bindings);
  a.field("ark::simulation::StartupWorldRuntimeState.activity_page_lists", v.activity_page_lists);
  a.field("ark::simulation::StartupWorldRuntimeState.activity_page_display_humans", v.activity_page_display_humans);
  a.field("ark::simulation::StartupWorldRuntimeState.activity_page_parents", v.activity_page_parents);
  a.field("ark::simulation::StartupWorldRuntimeState.activity_page_answers", v.activity_page_answers);
  a.field("ark::simulation::StartupWorldRuntimeState.activity_page_selections", v.activity_page_selections);
  a.field("ark::simulation::StartupWorldRuntimeState.activity_page_scroll", v.activity_page_scroll);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_details", v.facility_details);
  a.field("ark::simulation::StartupWorldRuntimeState.task_progress", v.task_progress);
  a.field("ark::simulation::StartupWorldRuntimeState.tasks", v.tasks);
  a.field("ark::simulation::StartupWorldRuntimeState.task_order", v.task_order);
  a.field("ark::simulation::StartupWorldRuntimeState.task_original_ids", v.task_original_ids);
  a.field("ark::simulation::StartupWorldRuntimeState.task_sequence", v.task_sequence);
  a.field("ark::simulation::StartupWorldRuntimeState.next_facility_identity", v.next_facility_identity);
  a.field("ark::simulation::StartupWorldRuntimeState.next_task_identity", v.next_task_identity);
  a.field("ark::simulation::StartupWorldRuntimeState.task_replay_order", v.task_replay_order);
  a.field("ark::simulation::StartupWorldRuntimeState.task_special_selection", v.task_special_selection);
  a.field("ark::simulation::StartupWorldRuntimeState.task_special_selection_index", v.task_special_selection_index);
  a.field("ark::simulation::StartupWorldRuntimeState.task_special_selection_list", v.task_special_selection_list);
  a.field("ark::simulation::StartupWorldRuntimeState.fence_level", v.fence_level);
  a.field("ark::simulation::StartupWorldRuntimeState.base_variants", v.base_variants);
  a.field("ark::simulation::StartupWorldRuntimeState.active_task", v.active_task);
  a.field("ark::simulation::StartupWorldRuntimeState.participants", v.participants);
  a.field("ark::simulation::StartupWorldRuntimeState.crew_summaries", v.crew_summaries);
  a.field("ark::simulation::StartupWorldRuntimeState.sites", v.sites);
  a.field("ark::simulation::StartupWorldRuntimeState.ground_definition", v.ground_definition);
  a.field("ark::simulation::StartupWorldRuntimeState.special_ground_definition", v.special_ground_definition);
  a.field("ark::simulation::StartupWorldRuntimeState.arrival_counter", v.arrival_counter);
  a.field("ark::simulation::StartupWorldRuntimeState.camera_delay", v.camera_delay);
  a.field("ark::simulation::StartupWorldRuntimeState.camera_follow", v.camera_follow);
  a.field("ark::simulation::StartupWorldRuntimeState.event89_count", v.event89_count);
  a.field("ark::simulation::StartupWorldRuntimeState.entry_updates", v.entry_updates);
  a.field("ark::simulation::StartupWorldRuntimeState.global_updates", v.global_updates);
  a.field("ark::simulation::StartupWorldRuntimeState.camera", v.camera);
  a.field("ark::simulation::StartupWorldRuntimeState.previous_camera", v.previous_camera);
  a.field("ark::simulation::StartupWorldRuntimeState.camera_velocity", v.camera_velocity);
  a.field("ark::simulation::StartupWorldRuntimeState.build_mode", v.build_mode);
  a.field("ark::simulation::StartupWorldRuntimeState.build_definition", v.build_definition);
  a.field("ark::simulation::StartupWorldRuntimeState.build_anchor", v.build_anchor);
  a.field("ark::simulation::StartupWorldRuntimeState.build_moving_facility", v.build_moving_facility);
  a.field("ark::simulation::StartupWorldRuntimeState.build_feedback_counter", v.build_feedback_counter);
  a.field("ark::simulation::StartupWorldRuntimeState.build_feedback_message", v.build_feedback_message);
  a.field("ark::simulation::StartupWorldRuntimeState.focus_actor", v.focus_actor);
  a.field("ark::simulation::StartupWorldRuntimeState.focus_held_input", v.focus_held_input);
  a.field("ark::simulation::StartupWorldRuntimeState.visual_effects", v.visual_effects);
  a.field("ark::simulation::StartupWorldRuntimeState.delayed_effects", v.delayed_effects);
  a.field("ark::simulation::StartupWorldRuntimeState.global_effects", v.global_effects);
  a.field("ark::simulation::StartupWorldRuntimeState.sound_requests", v.sound_requests);
  a.field("ark::simulation::StartupWorldRuntimeState.reference_viewport", v.reference_viewport);
  a.field("ark::simulation::StartupWorldRuntimeState.page_counters", v.page_counters);
  a.field("ark::simulation::StartupWorldRuntimeState.page_phases", v.page_phases);
  a.field("ark::simulation::StartupWorldRuntimeState.task_page_lists", v.task_page_lists);
  a.field("ark::simulation::StartupWorldRuntimeState.task_recruitment_pages", v.task_recruitment_pages);
  a.field("ark::simulation::StartupWorldRuntimeState.task_extra_pages", v.task_extra_pages);
  a.field("ark::simulation::StartupWorldRuntimeState.task_page_predictions", v.task_page_predictions);
  a.field("ark::simulation::StartupWorldRuntimeState.task_page_acceleration", v.task_page_acceleration);
  a.field("ark::simulation::StartupWorldRuntimeState.page_secondary_counters", v.page_secondary_counters);
  a.field("ark::simulation::StartupWorldRuntimeState.task_display_table", v.task_display_table);
  a.field("ark::simulation::StartupWorldRuntimeState.task_display_initialized", v.task_display_initialized);
  a.field("ark::simulation::StartupWorldRuntimeState.deadline_initialized", v.deadline_initialized);
  a.field("ark::simulation::StartupWorldRuntimeState.deadline_grades", v.deadline_grades);
  a.field("ark::simulation::StartupWorldRuntimeState.deadline_returns", v.deadline_returns);
  a.field("ark::simulation::StartupWorldRuntimeState.deadline_closed_page", v.deadline_closed_page);
  a.field("ark::simulation::StartupWorldRuntimeState.award_rankings", v.award_rankings);
  a.field("ark::simulation::StartupWorldRuntimeState.award_announced", v.award_announced);
  a.field("ark::simulation::StartupWorldRuntimeState.award_termination_pending", v.award_termination_pending);
  a.field("ark::simulation::StartupWorldRuntimeState.award_pending_humans", v.award_pending_humans);
  a.field("ark::simulation::StartupWorldRuntimeState.reward_display", v.reward_display);
  a.field("ark::simulation::StartupWorldRuntimeState.effort_display", v.effort_display);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_page_bindings", v.facility_page_bindings);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_definition_page_bindings", v.facility_definition_page_bindings);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_page_neighbours", v.facility_page_neighbours);
  a.field("ark::simulation::StartupWorldRuntimeState.build_page_catalogs", v.build_page_catalogs);
  a.field("ark::simulation::StartupWorldRuntimeState.residence_page_candidates", v.residence_page_candidates);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_upgrade_initialized", v.facility_upgrade_initialized);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_upgrade_display", v.facility_upgrade_display);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_item_confirmations", v.facility_item_confirmations);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_item_pages_initialized", v.facility_item_pages_initialized);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_item_page_items", v.facility_item_page_items);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_item_page_lists", v.facility_item_page_lists);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_item_page_selections", v.facility_item_page_selections);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_item_response", v.facility_item_response);
  a.field("ark::simulation::StartupWorldRuntimeState.item_commerce_read", v.item_commerce_read);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_commerce_read", v.facility_commerce_read);
  a.field("ark::simulation::StartupWorldRuntimeState.commerce_pages_initialized", v.commerce_pages_initialized);
  a.field("ark::simulation::StartupWorldRuntimeState.commerce_page_data", v.commerce_page_data);
  a.field("ark::simulation::StartupWorldRuntimeState.commerce_page_lists", v.commerce_page_lists);
  a.field("ark::simulation::StartupWorldRuntimeState.medal_count", v.medal_count);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_free_builds", v.facility_free_builds);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_unlock_counters", v.facility_unlock_counters);
  a.field("ark::simulation::StartupWorldRuntimeState.facility_unlock_notices", v.facility_unlock_notices);
  a.field("ark::simulation::StartupWorldRuntimeState.confirm_input", v.confirm_input);
  a.field("ark::simulation::StartupWorldRuntimeState.cancel_input", v.cancel_input);
  a.field("ark::simulation::StartupWorldRuntimeState.menu_input", v.menu_input);
  a.field("ark::simulation::StartupWorldRuntimeState.page_confirm_held", v.page_confirm_held);
  a.field("ark::simulation::StartupWorldRuntimeState.rank", v.rank);
  a.field("ark::simulation::StartupWorldRuntimeState.quarter_counter", v.quarter_counter);
  a.field("ark::simulation::StartupWorldRuntimeState.legacy_D", v.legacy_D);
  a.field("ark::simulation::StartupWorldRuntimeState.legacy_n", v.legacy_n);
  a.field("ark::simulation::StartupWorldRuntimeState.yearly_statistics", v.yearly_statistics);
  a.field("ark::simulation::StartupWorldRuntimeState.events_held", v.events_held);
  a.field("ark::simulation::StartupWorldRuntimeState.task_subperiods", v.task_subperiods);
  a.field("ark::simulation::StartupWorldRuntimeState.generation_retry", v.generation_retry);
  a.field("ark::simulation::StartupWorldRuntimeState.deadline_page", v.deadline_page);
  a.field("ark::simulation::StartupWorldRuntimeState.completion_mode", v.completion_mode);
  a.field("ark::simulation::StartupWorldRuntimeState.system_completion_mode", v.system_completion_mode);
  a.field("ark::simulation::StartupWorldRuntimeState.save_marker", v.save_marker);
  a.field("ark::simulation::StartupWorldRuntimeState.system_unlock_data", v.system_unlock_data);
  a.field("ark::simulation::StartupWorldRuntimeState.rank_met", v.rank_met);
  a.field("ark::simulation::StartupWorldRuntimeState.rank_values", v.rank_values);
  a.field("ark::simulation::StartupWorldRuntimeState.rank_history", v.rank_history);
  a.field("ark::simulation::StartupWorldRuntimeState.rank_celebration_participants", v.rank_celebration_participants);
  a.field("ark::simulation::StartupWorldRuntimeState.simulation_steps", v.simulation_steps);
  a.field("ark::simulation::StartupWorldRuntimeState.clock_parameter", v.clock_parameter);
  a.field("ark::simulation::StartupWorldRuntimeState.calendar_advance", v.calendar_advance);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().scene)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().task)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().shop_humans)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().shop_actors)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().items)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().dungeon_facilities)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().dungeon_actors)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().catalog)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().shops)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().shop_order)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().item_rewards)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().human_definition_state)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().human_homes)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().human_presence)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().human_flags)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().human_calendar)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().actor_metadata)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_original_ids)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_ordinals)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_residents)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_difficulties)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_flags)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().neighbourhood)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().neighbourhood_details)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().exploration_summaries)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().exploration_displays)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().dungeon_labels)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().surface)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().road_patches)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().scripts)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().cash_peak)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().cash_peak_village)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().monthly_cash)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_monthly_cash)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_month_age)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().shop_item_stock)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().report_state)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().report_counter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().report_snapshot)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().report_records)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().report_new_records)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().report_portraits)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().report_kills)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().maximum_income)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().village_points)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().popularity)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().maximum_popularity)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().popularity_display)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().popularity_pulses)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().popularity_rewards)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_definitions)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_presence)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().residence_catalog_available)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().page_human_bindings)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().task_abort_questions)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().task_abort_answers)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().human_pages_initialized)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().human_profession_changes)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().human_page_catalogs)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().equipment_page_catalogs)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().human_page_selections)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().page_job_bindings)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().human_page_parents)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().human_page_answers)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().human_equipment_choices)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().human_attribute_display)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().equipment_attribute_display)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().human_gift_scores)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().human_gift_messages)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().tax_page_residents)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().tax_page_selection)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().tax_page_scroll)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().residence_hint_counter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().activity_flags)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().activity_counts)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().human_activity_previous)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().activity_pages_initialized)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().activity_page_bindings)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().activity_page_lists)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().activity_page_display_humans)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().activity_page_parents)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().activity_page_answers)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().activity_page_selections)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().activity_page_scroll)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_details)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().task_progress)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().tasks)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().task_order)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().task_original_ids)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().task_sequence)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().next_facility_identity)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().next_task_identity)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().task_replay_order)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().task_special_selection)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().task_special_selection_index)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().task_special_selection_list)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().fence_level)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().base_variants)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().active_task)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().participants)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().crew_summaries)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().sites)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().ground_definition)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().special_ground_definition)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().arrival_counter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().camera_delay)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().camera_follow)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().event89_count)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().entry_updates)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().global_updates)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().camera)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().previous_camera)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().camera_velocity)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().build_mode)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().build_definition)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().build_anchor)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().build_moving_facility)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().build_feedback_counter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().build_feedback_message)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().focus_actor)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().focus_held_input)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().visual_effects)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().delayed_effects)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().global_effects)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().sound_requests)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().reference_viewport)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().page_counters)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().page_phases)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().task_page_lists)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().task_recruitment_pages)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().task_extra_pages)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().task_page_predictions)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().task_page_acceleration)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().page_secondary_counters)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().task_display_table)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().task_display_initialized)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().deadline_initialized)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().deadline_grades)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().deadline_returns)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().deadline_closed_page)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().award_rankings)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().award_announced)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().award_termination_pending)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().award_pending_humans)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().reward_display)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().effort_display)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_page_bindings)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_definition_page_bindings)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_page_neighbours)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().build_page_catalogs)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().residence_page_candidates)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_upgrade_initialized)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_upgrade_display)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_item_confirmations)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_item_pages_initialized)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_item_page_items)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_item_page_lists)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_item_page_selections)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_item_response)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().item_commerce_read)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_commerce_read)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().commerce_pages_initialized)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().commerce_page_data)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().commerce_page_lists)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().medal_count)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_free_builds)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_unlock_counters)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().facility_unlock_notices)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().confirm_input)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().cancel_input)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().menu_input)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().page_confirm_held)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().rank)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().quarter_counter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().legacy_D)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().legacy_n)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().yearly_statistics)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().events_held)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().task_subperiods)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().generation_retry)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().deadline_page)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().completion_mode)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().system_completion_mode)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().save_marker)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().system_unlock_data)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().rank_met)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().rank_values)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().rank_history)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().rank_celebration_participants)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().simulation_steps)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().clock_parameter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::StartupWorldRuntimeState>().calendar_advance)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::AccountingReport> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::AccountingReport.input", v.input);
  a.field("ark::simulation::rules::AccountingReport.categories", v.categories);
  a.field("ark::simulation::rules::AccountingReport.displayed", v.displayed);
  a.field("ark::simulation::rules::AccountingReport.displayed_net", v.displayed_net);
  a.field("ark::simulation::rules::AccountingReport.pending_points", v.pending_points);
  a.field("ark::simulation::rules::AccountingReport.awarded_points", v.awarded_points);
  a.field("ark::simulation::rules::AccountingReport.claimed", v.claimed);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::AccountingReport>().input)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AccountingReport>().categories)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AccountingReport>().displayed)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AccountingReport>().displayed_net)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AccountingReport>().pending_points)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AccountingReport>().awarded_points)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AccountingReport>().claimed)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::ActivityCandidateCell> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::ActivityCandidateCell.position", v.position);
  a.field("ark::simulation::rules::ActivityCandidateCell.definition", v.definition);
  a.field("ark::simulation::rules::ActivityCandidateCell.instance", v.instance);
  a.field("ark::simulation::rules::ActivityCandidateCell.cost", v.cost);
  a.field("ark::simulation::rules::ActivityCandidateCell.origin", v.origin);
  a.field("ark::simulation::rules::ActivityCandidateCell.source_index", v.source_index);
  a.field("ark::simulation::rules::ActivityCandidateCell.instance_definition", v.instance_definition);
  a.field("ark::simulation::rules::ActivityCandidateCell.legacy_state", v.legacy_state);
  a.field("ark::simulation::rules::ActivityCandidateCell.route_category", v.route_category);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::ActivityCandidateCell>().position)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ActivityCandidateCell>().definition)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ActivityCandidateCell>().instance)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ActivityCandidateCell>().cost)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ActivityCandidateCell>().origin)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ActivityCandidateCell>().source_index)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ActivityCandidateCell>().instance_definition)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ActivityCandidateCell>().legacy_state)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ActivityCandidateCell>().route_category)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::ActorControlState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::ActorControlState.flags", v.flags);
  a.field("ark::simulation::rules::ActorControlState.state", v.state);
  a.field("ark::simulation::rules::ActorControlState.action", v.action);
  a.field("ark::simulation::rules::ActorControlState.action_counter", v.action_counter);
  a.field("ark::simulation::rules::ActorControlState.alternate_counter", v.alternate_counter);
  a.field("ark::simulation::rules::ActorControlState.facing", v.facing);
  a.field("ark::simulation::rules::ActorControlState.queue", v.queue);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::ActorControlState>().flags)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ActorControlState>().state)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ActorControlState>().action)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ActorControlState>().action_counter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ActorControlState>().alternate_counter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ActorControlState>().facing)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ActorControlState>().queue)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::ActorEffectState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::ActorEffectState.display", v.display);
  a.field("ark::simulation::rules::ActorEffectState.delayed", v.delayed);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::ActorEffectState>().display)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ActorEffectState>().delayed)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::AiRewardState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::AiRewardState.battle", v.battle);
  a.field("ark::simulation::rules::AiRewardState.retired_actors", v.retired_actors);
  a.field("ark::simulation::rules::AiRewardState.human_order", v.human_order);
  a.field("ark::simulation::rules::AiRewardState.monster_order", v.monster_order);
  a.field("ark::simulation::rules::AiRewardState.contexts", v.contexts);
  a.field("ark::simulation::rules::AiRewardState.growth", v.growth);
  a.field("ark::simulation::rules::AiRewardState.professions", v.professions);
  a.field("ark::simulation::rules::AiRewardState.monster_growth", v.monster_growth);
  a.field("ark::simulation::rules::AiRewardState.monster_definition_order", v.monster_definition_order);
  a.field("ark::simulation::rules::AiRewardState.monster_progress", v.monster_progress);
  a.field("ark::simulation::rules::AiRewardState.monster_limit", v.monster_limit);
  a.field("ark::simulation::rules::AiRewardState.encounters", v.encounters);
  a.field("ark::simulation::rules::AiRewardState.encounter_order", v.encounter_order);
  a.field("ark::simulation::rules::AiRewardState.retired_encounters", v.retired_encounters);
  a.field("ark::simulation::rules::AiRewardState.external_actor_roots", v.external_actor_roots);
  a.field("ark::simulation::rules::AiRewardState.facility_actor_roots", v.facility_actor_roots);
  a.field("ark::simulation::rules::AiRewardState.external_encounter_roots", v.external_encounter_roots);
  a.field("ark::simulation::rules::AiRewardState.projectiles", v.projectiles);
  a.field("ark::simulation::rules::AiRewardState.projectile_order", v.projectile_order);
  a.field("ark::simulation::rules::AiRewardState.next_projectile_id", v.next_projectile_id);
  a.field("ark::simulation::rules::AiRewardState.accounting", v.accounting);
  a.field("ark::simulation::rules::AiRewardState.next_cash_id", v.next_cash_id);
  a.field("ark::simulation::rules::AiRewardState.next_actor_id", v.next_actor_id);
  a.field("ark::simulation::rules::AiRewardState.next_encounter_id", v.next_encounter_id);
  a.field("ark::simulation::rules::AiRewardState.legacy_encounter_counter", v.legacy_encounter_counter);
  a.field("ark::simulation::rules::AiRewardState.period", v.period);
  a.field("ark::simulation::rules::AiRewardState.pending_completion", v.pending_completion);
  a.field("ark::simulation::rules::AiRewardState.task_active", v.task_active);
  a.field("ark::simulation::rules::AiRewardState.task_completed", v.task_completed);
  a.field("ark::simulation::rules::AiRewardState.feature16", v.feature16);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().battle)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().retired_actors)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().human_order)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().monster_order)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().contexts)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().growth)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().professions)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().monster_growth)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().monster_definition_order)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().monster_progress)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().monster_limit)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().encounters)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().encounter_order)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().retired_encounters)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().external_actor_roots)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().facility_actor_roots)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().external_encounter_roots)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().projectiles)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().projectile_order)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().next_projectile_id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().accounting)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().next_cash_id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().next_actor_id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().next_encounter_id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().legacy_encounter_counter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().period)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().pending_completion)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().task_active)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().task_completed)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::AiRewardState>().feature16)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::ArrivalBinding> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::ArrivalBinding.goal", v.goal);
  a.field("ark::simulation::rules::ArrivalBinding.instance_id", v.instance_id);
  a.field("ark::simulation::rules::ArrivalBinding.definition_id", v.definition_id);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::ArrivalBinding>().goal)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ArrivalBinding>().instance_id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ArrivalBinding>().definition_id)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::BattleActorRecord> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::BattleActorRecord.id", v.id);
  a.field("ark::simulation::rules::BattleActorRecord.kind", v.kind);
  a.field("ark::simulation::rules::BattleActorRecord.definition", v.definition);
  a.field("ark::simulation::rules::BattleActorRecord.control", v.control);
  a.field("ark::simulation::rules::BattleActorRecord.hp", v.hp);
  a.field("ark::simulation::rules::BattleActorRecord.capacity", v.capacity);
  a.field("ark::simulation::rules::BattleActorRecord.baseline", v.baseline);
  a.field("ark::simulation::rules::BattleActorRecord.state_counter", v.state_counter);
  a.field("ark::simulation::rules::BattleActorRecord.state_parameter", v.state_parameter);
  a.field("ark::simulation::rules::BattleActorRecord.attack_count", v.attack_count);
  a.field("ark::simulation::rules::BattleActorRecord.down_timer", v.down_timer);
  a.field("ark::simulation::rules::BattleActorRecord.object_slot", v.object_slot);
  a.field("ark::simulation::rules::BattleActorRecord.rescue", v.rescue);
  a.field("ark::simulation::rules::BattleActorRecord.encounter", v.encounter);
  a.field("ark::simulation::rules::BattleActorRecord.group", v.group);
  a.field("ark::simulation::rules::BattleActorRecord.attack_slot", v.attack_slot);
  a.field("ark::simulation::rules::BattleActorRecord.monster_posture", v.monster_posture);
  a.field("ark::simulation::rules::BattleActorRecord.legacy_id", v.legacy_id);
  a.field("ark::simulation::rules::BattleActorRecord.body", v.body);
  a.field("ark::simulation::rules::BattleActorRecord.sprite", v.sprite);
  a.field("ark::simulation::rules::BattleActorRecord.position", v.position);
  a.field("ark::simulation::rules::BattleActorRecord.attack_position", v.attack_position);
  a.field("ark::simulation::rules::BattleActorRecord.miss", v.miss);
  a.field("ark::simulation::rules::BattleActorRecord.damage_total", v.damage_total);
  a.field("ark::simulation::rules::BattleActorRecord.hit_count", v.hit_count);
  a.field("ark::simulation::rules::BattleActorRecord.hit_flash", v.hit_flash);
  a.field("ark::simulation::rules::BattleActorRecord.label_timer", v.label_timer);
  a.field("ark::simulation::rules::BattleActorRecord.miss_label", v.miss_label);
  a.field("ark::simulation::rules::BattleActorRecord.follow", v.follow);
  a.field("ark::simulation::rules::BattleActorRecord.attack_armed", v.attack_armed);
  a.field("ark::simulation::rules::BattleActorRecord.combo_index", v.combo_index);
  a.field("ark::simulation::rules::BattleActorRecord.combo_count", v.combo_count);
  a.field("ark::simulation::rules::BattleActorRecord.attack_idle", v.attack_idle);
  a.field("ark::simulation::rules::BattleActorRecord.attack_destination", v.attack_destination);
  a.field("ark::simulation::rules::BattleActorRecord.perceived_enemy", v.perceived_enemy);
  a.field("ark::simulation::rules::BattleActorRecord.perceived_distance", v.perceived_distance);
  a.field("ark::simulation::rules::BattleActorRecord.blocked_battle_steps", v.blocked_battle_steps);
  a.field("ark::simulation::rules::BattleActorRecord.attack_cooldown", v.attack_cooldown);
  a.field("ark::simulation::rules::BattleActorRecord.decision_start", v.decision_start);
  a.field("ark::simulation::rules::BattleActorRecord.physics_pause", v.physics_pause);
  a.field("ark::simulation::rules::BattleActorRecord.vertical_velocity", v.vertical_velocity);
  a.field("ark::simulation::rules::BattleActorRecord.area_after", v.area_after);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().kind)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().definition)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().control)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().hp)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().capacity)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().baseline)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().state_counter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().state_parameter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().attack_count)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().down_timer)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().object_slot)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().rescue)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().encounter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().group)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().attack_slot)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().monster_posture)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().legacy_id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().body)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().sprite)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().position)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().attack_position)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().miss)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().damage_total)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().hit_count)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().hit_flash)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().label_timer)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().miss_label)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().follow)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().attack_armed)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().combo_index)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().combo_count)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().attack_idle)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().attack_destination)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().perceived_enemy)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().perceived_distance)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().blocked_battle_steps)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().attack_cooldown)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().decision_start)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().physics_pause)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().vertical_velocity)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleActorRecord>().area_after)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::BattleCommitState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::BattleCommitState.actors", v.actors);
  a.field("ark::simulation::rules::BattleCommitState.humans", v.humans);
  a.field("ark::simulation::rules::BattleCommitState.monsters", v.monsters);
  a.field("ark::simulation::rules::BattleCommitState.participants", v.participants);
  a.field("ark::simulation::rules::BattleCommitState.quest_encounters", v.quest_encounters);
  a.field("ark::simulation::rules::BattleCommitState.events", v.events);
  a.field("ark::simulation::rules::BattleCommitState.global_downs", v.global_downs);
  a.field("ark::simulation::rules::BattleCommitState.defeated_definitions", v.defeated_definitions);
  a.field("ark::simulation::rules::BattleCommitState.drop_progress", v.drop_progress);
  a.field("ark::simulation::rules::BattleCommitState.objects", v.objects);
  a.field("ark::simulation::rules::BattleCommitState.next_object_id", v.next_object_id);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::BattleCommitState>().actors)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleCommitState>().humans)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleCommitState>().monsters)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleCommitState>().participants)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleCommitState>().quest_encounters)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleCommitState>().events)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleCommitState>().global_downs)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleCommitState>().defeated_definitions)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleCommitState>().drop_progress)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleCommitState>().objects)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleCommitState>().next_object_id)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::BattleGroupMember> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::BattleGroupMember.id", v.id);
  a.field("ark::simulation::rules::BattleGroupMember.flags", v.flags);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::BattleGroupMember>().id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleGroupMember>().flags)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::BattleGroupState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::BattleGroupState.tick", v.tick);
  a.field("ark::simulation::rules::BattleGroupState.duration", v.duration);
  a.field("ark::simulation::rules::BattleGroupState.cycle", v.cycle);
  a.field("ark::simulation::rules::BattleGroupState.alternating_side", v.alternating_side);
  a.field("ark::simulation::rules::BattleGroupState.humans", v.humans);
  a.field("ark::simulation::rules::BattleGroupState.monsters", v.monsters);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::BattleGroupState>().tick)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleGroupState>().duration)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleGroupState>().cycle)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleGroupState>().alternating_side)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleGroupState>().humans)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::BattleGroupState>().monsters)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::BuildingId> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::BuildingId.value", v.value);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::BuildingId>().value)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::CalendarMaintenanceShopItem> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::CalendarMaintenanceShopItem.definition", v.definition);
  a.field("ark::simulation::rules::CalendarMaintenanceShopItem.minimum_rank", v.minimum_rank);
  a.field("ark::simulation::rules::CalendarMaintenanceShopItem.maximum_quantity", v.maximum_quantity);
  a.field("ark::simulation::rules::CalendarMaintenanceShopItem.quantity", v.quantity);
  a.field("ark::simulation::rules::CalendarMaintenanceShopItem.presence", v.presence);
  a.field("ark::simulation::rules::CalendarMaintenanceShopItem.legacy_q", v.legacy_q);
  a.field("ark::simulation::rules::CalendarMaintenanceShopItem.newly_available", v.newly_available);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::CalendarMaintenanceShopItem>().definition)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CalendarMaintenanceShopItem>().minimum_rank)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CalendarMaintenanceShopItem>().maximum_quantity)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CalendarMaintenanceShopItem>().quantity)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CalendarMaintenanceShopItem>().presence)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CalendarMaintenanceShopItem>().legacy_q)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CalendarMaintenanceShopItem>().newly_available)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::CandidateDefinition> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::CandidateDefinition.definition_id", v.definition_id);
  a.field("ark::simulation::rules::CandidateDefinition.legacy_category", v.legacy_category);
  a.field("ark::simulation::rules::CandidateDefinition.definition_charm", v.definition_charm);
  a.field("ark::simulation::rules::CandidateDefinition.legacy_kind", v.legacy_kind);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::CandidateDefinition>().definition_id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CandidateDefinition>().legacy_category)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CandidateDefinition>().definition_charm)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CandidateDefinition>().legacy_kind)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::CandidateInstance> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::CandidateInstance.instance_id", v.instance_id);
  a.field("ark::simulation::rules::CandidateInstance.definition_id", v.definition_id);
  a.field("ark::simulation::rules::CandidateInstance.legacy_phase", v.legacy_phase);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::CandidateInstance>().instance_id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CandidateInstance>().definition_id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CandidateInstance>().legacy_phase)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::CashEntry> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::CashEntry.event_id", v.event_id);
  a.field("ark::simulation::rules::CashEntry.period", v.period);
  a.field("ark::simulation::rules::CashEntry.category", v.category);
  a.field("ark::simulation::rules::CashEntry.direction", v.direction);
  a.field("ark::simulation::rules::CashEntry.amount", v.amount);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::CashEntry>().event_id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CashEntry>().period)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CashEntry>().category)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CashEntry>().direction)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CashEntry>().amount)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::CashTotals> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::CashTotals.income", v.income);
  a.field("ark::simulation::rules::CashTotals.expense", v.expense);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::CashTotals>().income)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CashTotals>().expense)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::CharacterHpState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::CharacterHpState.requested_delta", v.requested_delta);
  a.field("ark::simulation::rules::CharacterHpState.displayed", v.displayed);
  a.field("ark::simulation::rules::CharacterHpState.origin", v.origin);
  a.field("ark::simulation::rules::CharacterHpState.target", v.target);
  a.field("ark::simulation::rules::CharacterHpState.animating", v.animating);
  a.field("ark::simulation::rules::CharacterHpState.legacy_tick", v.legacy_tick);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::CharacterHpState>().requested_delta)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CharacterHpState>().displayed)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CharacterHpState>().origin)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CharacterHpState>().target)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CharacterHpState>().animating)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CharacterHpState>().legacy_tick)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::CharacterId> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::CharacterId.value", v.value);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::CharacterId>().value)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::CombatInfluenceCandidate> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::CombatInfluenceCandidate.width", v.width);
  a.field("ark::simulation::rules::CombatInfluenceCandidate.height", v.height);
  a.field("ark::simulation::rules::CombatInfluenceCandidate.human_field", v.human_field);
  a.field("ark::simulation::rules::CombatInfluenceCandidate.monster_field", v.monster_field);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::CombatInfluenceCandidate>().width)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CombatInfluenceCandidate>().height)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CombatInfluenceCandidate>().human_field)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CombatInfluenceCandidate>().monster_field)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::CombatPoint> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::CombatPoint.x", v.x);
  a.field("ark::simulation::rules::CombatPoint.height", v.height);
  a.field("ark::simulation::rules::CombatPoint.z", v.z);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::CombatPoint>().x)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CombatPoint>().height)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::CombatPoint>().z)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::DefeatTally> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::DefeatTally.definition_id", v.definition_id);
  a.field("ark::simulation::rules::DefeatTally.defeats", v.defeats);
  a.field("ark::simulation::rules::DefeatTally.points_per_defeat", v.points_per_defeat);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::DefeatTally>().definition_id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DefeatTally>().defeats)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DefeatTally>().points_per_defeat)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::DelayedRewardState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::DelayedRewardState.amount", v.amount);
  a.field("ark::simulation::rules::DelayedRewardState.counter", v.counter);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::DelayedRewardState>().amount)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DelayedRewardState>().counter)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::DungeonActorProgress> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::DungeonActorProgress.progress", v.progress);
  a.field("ark::simulation::rules::DungeonActorProgress.previous", v.previous);
  a.field("ark::simulation::rules::DungeonActorProgress.percent", v.percent);
  a.field("ark::simulation::rules::DungeonActorProgress.endurance", v.endurance);
  a.field("ark::simulation::rules::DungeonActorProgress.retreat_state", v.retreat_state);
  a.field("ark::simulation::rules::DungeonActorProgress.retreat_updates", v.retreat_updates);
  a.field("ark::simulation::rules::DungeonActorProgress.constrained", v.constrained);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonActorProgress>().progress)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonActorProgress>().previous)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonActorProgress>().percent)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonActorProgress>().endurance)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonActorProgress>().retreat_state)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonActorProgress>().retreat_updates)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonActorProgress>().constrained)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::DungeonCrewRequest> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::DungeonCrewRequest.kind", v.kind);
  a.field("ark::simulation::rules::DungeonCrewRequest.actor", v.actor);
  a.field("ark::simulation::rules::DungeonCrewRequest.first", v.first);
  a.field("ark::simulation::rules::DungeonCrewRequest.second", v.second);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonCrewRequest>().kind)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonCrewRequest>().actor)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonCrewRequest>().first)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonCrewRequest>().second)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::DungeonFacilityProgress> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::DungeonFacilityProgress.updates", v.updates);
  a.field("ark::simulation::rules::DungeonFacilityProgress.progress", v.progress);
  a.field("ark::simulation::rules::DungeonFacilityProgress.extent", v.extent);
  a.field("ark::simulation::rules::DungeonFacilityProgress.percent", v.percent);
  a.field("ark::simulation::rules::DungeonFacilityProgress.previous_percent", v.previous_percent);
  a.field("ark::simulation::rules::DungeonFacilityProgress.challenges", v.challenges);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonFacilityProgress>().updates)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonFacilityProgress>().progress)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonFacilityProgress>().extent)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonFacilityProgress>().percent)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonFacilityProgress>().previous_percent)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonFacilityProgress>().challenges)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::DungeonFinishSite> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::DungeonFinishSite.occupied_cells", v.occupied_cells);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonFinishSite>().occupied_cells)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::DungeonFinishSurface> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::DungeonFinishSurface.definition", v.definition);
  a.field("ark::simulation::rules::DungeonFinishSurface.updates", v.updates);
  a.field("ark::simulation::rules::DungeonFinishSurface.instance", v.instance);
  a.field("ark::simulation::rules::DungeonFinishSurface.fragment", v.fragment);
  a.field("ark::simulation::rules::DungeonFinishSurface.display_definition", v.display_definition);
  a.field("ark::simulation::rules::DungeonFinishSurface.variant", v.variant);
  a.field("ark::simulation::rules::DungeonFinishSurface.road_mask", v.road_mask);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonFinishSurface>().definition)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonFinishSurface>().updates)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonFinishSurface>().instance)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonFinishSurface>().fragment)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonFinishSurface>().display_definition)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonFinishSurface>().variant)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonFinishSurface>().road_mask)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::DungeonFinishTask> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::DungeonFinishTask.identity", v.identity);
  a.field("ark::simulation::rules::DungeonFinishTask.definition", v.definition);
  a.field("ark::simulation::rules::DungeonFinishTask.difficulty", v.difficulty);
  a.field("ark::simulation::rules::DungeonFinishTask.pending_completion_value", v.pending_completion_value);
  a.field("ark::simulation::rules::DungeonFinishTask.facility", v.facility);
  a.field("ark::simulation::rules::DungeonFinishTask.site", v.site);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonFinishTask>().identity)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonFinishTask>().definition)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonFinishTask>().difficulty)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonFinishTask>().pending_completion_value)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonFinishTask>().facility)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonFinishTask>().site)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::DungeonMonsterAvailability> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::DungeonMonsterAvailability.status", v.status);
  a.field("ark::simulation::rules::DungeonMonsterAvailability.pending_notice", v.pending_notice);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonMonsterAvailability>().status)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonMonsterAvailability>().pending_notice)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::DungeonTaskDefinitionProgress> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::DungeonTaskDefinitionProgress.kind", v.kind);
  a.field("ark::simulation::rules::DungeonTaskDefinitionProgress.flags", v.flags);
  a.field("ark::simulation::rules::DungeonTaskDefinitionProgress.completed", v.completed);
  a.field("ark::simulation::rules::DungeonTaskDefinitionProgress.monster_definition", v.monster_definition);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonTaskDefinitionProgress>().kind)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonTaskDefinitionProgress>().flags)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonTaskDefinitionProgress>().completed)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonTaskDefinitionProgress>().monster_definition)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::DungeonTaskSuccessState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::DungeonTaskSuccessState.successes", v.successes);
  a.field("ark::simulation::rules::DungeonTaskSuccessState.ordinary_explorations", v.ordinary_explorations);
  a.field("ark::simulation::rules::DungeonTaskSuccessState.exploration_stage", v.exploration_stage);
  a.field("ark::simulation::rules::DungeonTaskSuccessState.exploration_dates", v.exploration_dates);
  a.field("ark::simulation::rules::DungeonTaskSuccessState.task_pool_progress", v.task_pool_progress);
  a.field("ark::simulation::rules::DungeonTaskSuccessState.definitions", v.definitions);
  a.field("ark::simulation::rules::DungeonTaskSuccessState.monsters", v.monsters);
  a.field("ark::simulation::rules::DungeonTaskSuccessState.remaining_task_definitions", v.remaining_task_definitions);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonTaskSuccessState>().successes)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonTaskSuccessState>().ordinary_explorations)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonTaskSuccessState>().exploration_stage)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonTaskSuccessState>().exploration_dates)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonTaskSuccessState>().task_pool_progress)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonTaskSuccessState>().definitions)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonTaskSuccessState>().monsters)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::DungeonTaskSuccessState>().remaining_task_definitions)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::EncounterRuntimeState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::EncounterRuntimeState.id", v.id);
  a.field("ark::simulation::rules::EncounterRuntimeState.center", v.center);
  a.field("ark::simulation::rules::EncounterRuntimeState.state", v.state);
  a.field("ark::simulation::rules::EncounterRuntimeState.counter", v.counter);
  a.field("ark::simulation::rules::EncounterRuntimeState.idle", v.idle);
  a.field("ark::simulation::rules::EncounterRuntimeState.spawned", v.spawned);
  a.field("ark::simulation::rules::EncounterRuntimeState.quota", v.quota);
  a.field("ark::simulation::rules::EncounterRuntimeState.reward", v.reward);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::EncounterRuntimeState>().id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::EncounterRuntimeState>().center)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::EncounterRuntimeState>().state)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::EncounterRuntimeState>().counter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::EncounterRuntimeState>().idle)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::EncounterRuntimeState>().spawned)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::EncounterRuntimeState>().quota)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::EncounterRuntimeState>().reward)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::FacilityArrivalState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::FacilityArrivalState.legacy_visit_counts", v.legacy_visit_counts);
  a.field("ark::simulation::rules::FacilityArrivalState.legacy_category_one_count", v.legacy_category_one_count);
  a.field("ark::simulation::rules::FacilityArrivalState.legacy_category_six_counter", v.legacy_category_six_counter);
  a.field("ark::simulation::rules::FacilityArrivalState.last_visited_instance", v.last_visited_instance);
  a.field("ark::simulation::rules::FacilityArrivalState.legacy_actor_total", v.legacy_actor_total);
  a.field("ark::simulation::rules::FacilityArrivalState.current_month_facility_sales", v.current_month_facility_sales);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityArrivalState>().legacy_visit_counts)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityArrivalState>().legacy_category_one_count)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityArrivalState>().legacy_category_six_counter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityArrivalState>().last_visited_instance)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityArrivalState>().legacy_actor_total)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityArrivalState>().current_month_facility_sales)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::FacilityDeparture> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::FacilityDeparture.category", v.category);
  a.field("ark::simulation::rules::FacilityDeparture.selection", v.selection);
  a.field("ark::simulation::rules::FacilityDeparture.binding", v.binding);
  a.field("ark::simulation::rules::FacilityDeparture.route", v.route);
  a.field("ark::simulation::rules::FacilityDeparture.legacy_direction", v.legacy_direction);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityDeparture>().category)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityDeparture>().selection)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityDeparture>().binding)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityDeparture>().route)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityDeparture>().legacy_direction)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::FacilityEconomyDefinition> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::FacilityEconomyDefinition.attributes", v.attributes);
  a.field("ark::simulation::rules::FacilityEconomyDefinition.upgrade_uses", v.upgrade_uses);
  a.field("ark::simulation::rules::FacilityEconomyDefinition.construction_cost", v.construction_cost);
  a.field("ark::simulation::rules::FacilityEconomyDefinition.construction_ticks", v.construction_ticks);
  a.field("ark::simulation::rules::FacilityEconomyDefinition.legacy_flags", v.legacy_flags);
  a.field("ark::simulation::rules::FacilityEconomyDefinition.decoration", v.decoration);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityEconomyDefinition>().attributes)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityEconomyDefinition>().upgrade_uses)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityEconomyDefinition>().construction_cost)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityEconomyDefinition>().construction_ticks)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityEconomyDefinition>().legacy_flags)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityEconomyDefinition>().decoration)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::FacilityPlacement> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::FacilityPlacement.instance_id", v.instance_id);
  a.field("ark::simulation::rules::FacilityPlacement.definition_id", v.definition_id);
  a.field("ark::simulation::rules::FacilityPlacement.shape", v.shape);
  a.field("ark::simulation::rules::FacilityPlacement.orientation", v.orientation);
  a.field("ark::simulation::rules::FacilityPlacement.anchor", v.anchor);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityPlacement>().instance_id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityPlacement>().definition_id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityPlacement>().shape)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityPlacement>().orientation)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityPlacement>().anchor)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::FacilityTileBinding> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::FacilityTileBinding.instance_id", v.instance_id);
  a.field("ark::simulation::rules::FacilityTileBinding.definition_id", v.definition_id);
  a.field("ark::simulation::rules::FacilityTileBinding.fragment_index", v.fragment_index);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityTileBinding>().instance_id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityTileBinding>().definition_id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityTileBinding>().fragment_index)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::FacilityUseProgress> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::FacilityUseProgress.level", v.level);
  a.field("ark::simulation::rules::FacilityUseProgress.completed_uses", v.completed_uses);
  a.field("ark::simulation::rules::FacilityUseProgress.upgrade_pending", v.upgrade_pending);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityUseProgress>().level)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityUseProgress>().completed_uses)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::FacilityUseProgress>().upgrade_pending)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::GroundObjectState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::GroundObjectState.id", v.id);
  a.field("ark::simulation::rules::GroundObjectState.state", v.state);
  a.field("ark::simulation::rules::GroundObjectState.counter", v.counter);
  a.field("ark::simulation::rules::GroundObjectState.duration", v.duration);
  a.field("ark::simulation::rules::GroundObjectState.delay", v.delay);
  a.field("ark::simulation::rules::GroundObjectState.kind", v.kind);
  a.field("ark::simulation::rules::GroundObjectState.definition", v.definition);
  a.field("ark::simulation::rules::GroundObjectState.position", v.position);
  a.field("ark::simulation::rules::GroundObjectState.velocity", v.velocity);
  a.field("ark::simulation::rules::GroundObjectState.acceleration", v.acceleration);
  a.field("ark::simulation::rules::GroundObjectState.cached_cell", v.cached_cell);
  a.field("ark::simulation::rules::GroundObjectState.inside_town", v.inside_town);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::GroundObjectState>().id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::GroundObjectState>().state)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::GroundObjectState>().counter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::GroundObjectState>().duration)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::GroundObjectState>().delay)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::GroundObjectState>().kind)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::GroundObjectState>().definition)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::GroundObjectState>().position)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::GroundObjectState>().velocity)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::GroundObjectState>().acceleration)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::GroundObjectState>().cached_cell)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::GroundObjectState>().inside_town)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::HumanBattleRecord> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::HumanBattleRecord.kills", v.kills);
  a.field("ark::simulation::rules::HumanBattleRecord.killed_stat1", v.killed_stat1);
  a.field("ark::simulation::rules::HumanBattleRecord.battle_reward_stat", v.battle_reward_stat);
  a.field("ark::simulation::rules::HumanBattleRecord.task_kills", v.task_kills);
  a.field("ark::simulation::rules::HumanBattleRecord.participant_downs", v.participant_downs);
  a.field("ark::simulation::rules::HumanBattleRecord.recent_reward", v.recent_reward);
  a.field("ark::simulation::rules::HumanBattleRecord.recent_kills", v.recent_kills);
  a.field("ark::simulation::rules::HumanBattleRecord.luck", v.luck);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::HumanBattleRecord>().kills)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::HumanBattleRecord>().killed_stat1)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::HumanBattleRecord>().battle_reward_stat)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::HumanBattleRecord>().task_kills)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::HumanBattleRecord>().participant_downs)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::HumanBattleRecord>().recent_reward)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::HumanBattleRecord>().recent_kills)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::HumanBattleRecord>().luck)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::HumanDefinitionStatsInput> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::HumanDefinitionStatsInput.current_profession", v.current_profession);
  a.field("ark::simulation::rules::HumanDefinitionStatsInput.legacy_u", v.legacy_u);
  a.field("ark::simulation::rules::HumanDefinitionStatsInput.base", v.base);
  a.field("ark::simulation::rules::HumanDefinitionStatsInput.extra", v.extra);
  a.field("ark::simulation::rules::HumanDefinitionStatsInput.profession_levels", v.profession_levels);
  a.field("ark::simulation::rules::HumanDefinitionStatsInput.equipment", v.equipment);
  a.field("ark::simulation::rules::HumanDefinitionStatsInput.learned_spells", v.learned_spells);
  a.field("ark::simulation::rules::HumanDefinitionStatsInput.spell_professions", v.spell_professions);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::HumanDefinitionStatsInput>().current_profession)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::HumanDefinitionStatsInput>().legacy_u)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::HumanDefinitionStatsInput>().base)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::HumanDefinitionStatsInput>().extra)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::HumanDefinitionStatsInput>().profession_levels)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::HumanDefinitionStatsInput>().equipment)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::HumanDefinitionStatsInput>().learned_spells)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::HumanDefinitionStatsInput>().spell_professions)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::HumanDerivedStats> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::HumanDerivedStats.growth", v.growth);
  a.field("ark::simulation::rules::HumanDerivedStats.attributes", v.attributes);
  a.field("ark::simulation::rules::HumanDerivedStats.combat", v.combat);
  a.field("ark::simulation::rules::HumanDerivedStats.available_spells", v.available_spells);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::HumanDerivedStats>().growth)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::HumanDerivedStats>().attributes)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::HumanDerivedStats>().combat)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::HumanDerivedStats>().available_spells)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::HumanProfessionRule> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::HumanProfessionRule.maximum_growth", v.maximum_growth);
  a.field("ark::simulation::rules::HumanProfessionRule.attribute_percent", v.attribute_percent);
  a.field("ark::simulation::rules::HumanProfessionRule.difficulty", v.difficulty);
  a.field("ark::simulation::rules::HumanProfessionRule.unlocked", v.unlocked);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::HumanProfessionRule>().maximum_growth)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::HumanProfessionRule>().attribute_percent)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::HumanProfessionRule>().difficulty)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::HumanProfessionRule>().unlocked)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::LegacyMap> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::LegacyMap.width", v.width);
  a.field("ark::simulation::rules::LegacyMap.height", v.height);
  a.field("ark::simulation::rules::LegacyMap.cells", v.cells);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::LegacyMap>().width)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::LegacyMap>().height)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::LegacyMap>().cells)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::LegacyMapCell> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::LegacyMapCell.legacy_state", v.legacy_state);
  a.field("ark::simulation::rules::LegacyMapCell.category", v.category);
  a.field("ark::simulation::rules::LegacyMapCell.facility", v.facility);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::LegacyMapCell>().legacy_state)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::LegacyMapCell>().category)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::LegacyMapCell>().facility)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::LegacyPathResult> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::LegacyPathResult.error", v.error);
  a.field("ark::simulation::rules::LegacyPathResult.steps", v.steps);
  a.field("ark::simulation::rules::LegacyPathResult.cost", v.cost);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::LegacyPathResult>().error)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::LegacyPathResult>().steps)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::LegacyPathResult>().cost)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::LevelEndpoints> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::LevelEndpoints.first", v.first);
  a.field("ark::simulation::rules::LevelEndpoints.fifth", v.fifth);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::LevelEndpoints>().first)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::LevelEndpoints>().fifth)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::MonsterBattleRecord> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::MonsterBattleRecord.human_kills", v.human_kills);
  a.field("ark::simulation::rules::MonsterBattleRecord.stat1", v.stat1);
  a.field("ark::simulation::rules::MonsterBattleRecord.statF", v.statF);
  a.field("ark::simulation::rules::MonsterBattleRecord.death_reward", v.death_reward);
  a.field("ark::simulation::rules::MonsterBattleRecord.rank", v.rank);
  a.field("ark::simulation::rules::MonsterBattleRecord.flags", v.flags);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::MonsterBattleRecord>().human_kills)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::MonsterBattleRecord>().stat1)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::MonsterBattleRecord>().statF)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::MonsterBattleRecord>().death_reward)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::MonsterBattleRecord>().rank)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::MonsterBattleRecord>().flags)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::NeighbourSource> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::NeighbourSource.instance_id", v.instance_id);
  a.field("ark::simulation::rules::NeighbourSource.definition_id", v.definition_id);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::NeighbourSource>().instance_id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::NeighbourSource>().definition_id)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::ObjectCatalogRecord> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::ObjectCatalogRecord.flags", v.flags);
  a.field("ark::simulation::rules::ObjectCatalogRecord.status", v.status);
  a.field("ark::simulation::rules::ObjectCatalogRecord.unlock_counter", v.unlock_counter);
  a.field("ark::simulation::rules::ObjectCatalogRecord.newly_unlocked", v.newly_unlocked);
  a.field("ark::simulation::rules::ObjectCatalogRecord.inventory", v.inventory);
  a.field("ark::simulation::rules::ObjectCatalogRecord.free_purchases", v.free_purchases);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::ObjectCatalogRecord>().flags)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ObjectCatalogRecord>().status)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ObjectCatalogRecord>().unlock_counter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ObjectCatalogRecord>().newly_unlocked)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ObjectCatalogRecord>().inventory)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ObjectCatalogRecord>().free_purchases)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::ObjectId> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::ObjectId.value", v.value);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::ObjectId>().value)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::ObjectShopRecord> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::ObjectShopRecord.category", v.category);
  a.field("ark::simulation::rules::ObjectShopRecord.notices", v.notices);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::ObjectShopRecord>().category)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ObjectShopRecord>().notices)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::PeriodAccountingSnapshot> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::PeriodAccountingSnapshot.funds", v.funds);
  a.field("ark::simulation::rules::PeriodAccountingSnapshot.village_points", v.village_points);
  a.field("ark::simulation::rules::PeriodAccountingSnapshot.entries", v.entries);
  a.field("ark::simulation::rules::PeriodAccountingSnapshot.reports", v.reports);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::PeriodAccountingSnapshot>().funds)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::PeriodAccountingSnapshot>().village_points)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::PeriodAccountingSnapshot>().entries)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::PeriodAccountingSnapshot>().reports)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::Position> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::Position.x", v.x);
  a.field("ark::simulation::rules::Position.y", v.y);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::Position>().x)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::Position>().y)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::ProjectileState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::ProjectileState.kind", v.kind);
  a.field("ark::simulation::rules::ProjectileState.caster", v.caster);
  a.field("ark::simulation::rules::ProjectileState.original_target", v.original_target);
  a.field("ark::simulation::rules::ProjectileState.position", v.position);
  a.field("ark::simulation::rules::ProjectileState.velocity", v.velocity);
  a.field("ark::simulation::rules::ProjectileState.acceleration", v.acceleration);
  a.field("ark::simulation::rules::ProjectileState.facing", v.facing);
  a.field("ark::simulation::rules::ProjectileState.counter", v.counter);
  a.field("ark::simulation::rules::ProjectileState.delay", v.delay);
  a.field("ark::simulation::rules::ProjectileState.effect", v.effect);
  a.field("ark::simulation::rules::ProjectileState.damage", v.damage);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::ProjectileState>().kind)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ProjectileState>().caster)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ProjectileState>().original_target)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ProjectileState>().position)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ProjectileState>().velocity)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ProjectileState>().acceleration)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ProjectileState>().facing)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ProjectileState>().counter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ProjectileState>().delay)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ProjectileState>().effect)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ProjectileState>().damage)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::ReportInput> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::ReportInput.period", v.period);
  a.field("ark::simulation::rules::ReportInput.charges", v.charges);
  a.field("ark::simulation::rules::ReportInput.defeats", v.defeats);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::ReportInput>().period)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ReportInput>().charges)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ReportInput>().defeats)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::RescueActorContext> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::RescueActorContext.visits", v.visits);
  a.field("ark::simulation::rules::RescueActorContext.binding", v.binding);
  a.field("ark::simulation::rules::RescueActorContext.path_pending", v.path_pending);
  a.field("ark::simulation::rules::RescueActorContext.on_event_cell", v.on_event_cell);
  a.field("ark::simulation::rules::RescueActorContext.definition_task_flag", v.definition_task_flag);
  a.field("ark::simulation::rules::RescueActorContext.journey", v.journey);
  a.field("ark::simulation::rules::RescueActorContext.waypoint", v.waypoint);
  a.field("ark::simulation::rules::RescueActorContext.horizontal_velocity", v.horizontal_velocity);
  a.field("ark::simulation::rules::RescueActorContext.town_updates", v.town_updates);
  a.field("ark::simulation::rules::RescueActorContext.outside_updates", v.outside_updates);
  a.field("ark::simulation::rules::RescueActorContext.blocked_updates", v.blocked_updates);
  a.field("ark::simulation::rules::RescueActorContext.spawn_updates", v.spawn_updates);
  a.field("ark::simulation::rules::RescueActorContext.no_path_updates", v.no_path_updates);
  a.field("ark::simulation::rules::RescueActorContext.short_exit_updates", v.short_exit_updates);
  a.field("ark::simulation::rules::RescueActorContext.bad_area_updates", v.bad_area_updates);
  a.field("ark::simulation::rules::RescueActorContext.monster_mode", v.monster_mode);
  a.field("ark::simulation::rules::RescueActorContext.destination", v.destination);
  a.field("ark::simulation::rules::RescueActorContext.unbound_route", v.unbound_route);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::RescueActorContext>().visits)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueActorContext>().binding)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueActorContext>().path_pending)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueActorContext>().on_event_cell)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueActorContext>().definition_task_flag)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueActorContext>().journey)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueActorContext>().waypoint)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueActorContext>().horizontal_velocity)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueActorContext>().town_updates)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueActorContext>().outside_updates)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueActorContext>().blocked_updates)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueActorContext>().spawn_updates)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueActorContext>().no_path_updates)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueActorContext>().short_exit_updates)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueActorContext>().bad_area_updates)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueActorContext>().monster_mode)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueActorContext>().destination)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueActorContext>().unbound_route)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::RescueFacility> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::RescueFacility.placement", v.placement);
  a.field("ark::simulation::rules::RescueFacility.kind", v.kind);
  a.field("ark::simulation::rules::RescueFacility.category", v.category);
  a.field("ark::simulation::rules::RescueFacility.detail", v.detail);
  a.field("ark::simulation::rules::RescueFacility.price", v.price);
  a.field("ark::simulation::rules::RescueFacility.status", v.status);
  a.field("ark::simulation::rules::RescueFacility.upgrade_uses", v.upgrade_uses);
  a.field("ark::simulation::rules::RescueFacility.sales", v.sales);
  a.field("ark::simulation::rules::RescueFacility.occupants", v.occupants);
  a.field("ark::simulation::rules::RescueFacility.definition_wait", v.definition_wait);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::RescueFacility>().placement)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueFacility>().kind)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueFacility>().category)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueFacility>().detail)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueFacility>().price)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueFacility>().status)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueFacility>().upgrade_uses)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueFacility>().sales)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueFacility>().occupants)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueFacility>().definition_wait)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::RescueWorldState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::RescueWorldState.ai", v.ai);
  a.field("ark::simulation::rules::RescueWorldState.map", v.map);
  a.field("ark::simulation::rules::RescueWorldState.actors", v.actors);
  a.field("ark::simulation::rules::RescueWorldState.facilities", v.facilities);
  a.field("ark::simulation::rules::RescueWorldState.human_spending", v.human_spending);
  a.field("ark::simulation::rules::RescueWorldState.facility_uses", v.facility_uses);
  a.field("ark::simulation::rules::RescueWorldState.month_index", v.month_index);
  a.field("ark::simulation::rules::RescueWorldState.object_order", v.object_order);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::RescueWorldState>().ai)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueWorldState>().map)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueWorldState>().actors)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueWorldState>().facilities)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueWorldState>().human_spending)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueWorldState>().facility_uses)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueWorldState>().month_index)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RescueWorldState>().object_order)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::RewardActorContext> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::RewardActorContext.cell", v.cell);
  a.field("ark::simulation::rules::RewardActorContext.inside_town", v.inside_town);
  a.field("ark::simulation::rules::RewardActorContext.effects", v.effects);
  a.field("ark::simulation::rules::RewardActorContext.facility_category", v.facility_category);
  a.field("ark::simulation::rules::RewardActorContext.move_area", v.move_area);
  a.field("ark::simulation::rules::RewardActorContext.half_cell", v.half_cell);
  a.field("ark::simulation::rules::RewardActorContext.low_hp", v.low_hp);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::RewardActorContext>().cell)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardActorContext>().inside_town)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardActorContext>().effects)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardActorContext>().facility_category)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardActorContext>().move_area)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardActorContext>().half_cell)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardActorContext>().low_hp)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::RewardEncounter> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::RewardEncounter.runtime", v.runtime);
  a.field("ark::simulation::rules::RewardEncounter.members", v.members);
  a.field("ark::simulation::rules::RewardEncounter.group_exists", v.group_exists);
  a.field("ark::simulation::rules::RewardEncounter.group", v.group);
  a.field("ark::simulation::rules::RewardEncounter.legacy_id", v.legacy_id);
  a.field("ark::simulation::rules::RewardEncounter.influence", v.influence);
  a.field("ark::simulation::rules::RewardEncounter.human_scratch", v.human_scratch);
  a.field("ark::simulation::rules::RewardEncounter.monster_scratch", v.monster_scratch);
  a.field("ark::simulation::rules::RewardEncounter.linked_monsters", v.linked_monsters);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::RewardEncounter>().runtime)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardEncounter>().members)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardEncounter>().group_exists)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardEncounter>().group)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardEncounter>().legacy_id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardEncounter>().influence)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardEncounter>().human_scratch)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardEncounter>().monster_scratch)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardEncounter>().linked_monsters)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::RewardHumanDefinition> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::RewardHumanDefinition.definition", v.definition);
  a.field("ark::simulation::rules::RewardHumanDefinition.derived", v.derived);
  a.field("ark::simulation::rules::RewardHumanDefinition.experience", v.experience);
  a.field("ark::simulation::rules::RewardHumanDefinition.pending", v.pending);
  a.field("ark::simulation::rules::RewardHumanDefinition.notice_pending", v.notice_pending);
  a.field("ark::simulation::rules::RewardHumanDefinition.notice_attributes", v.notice_attributes);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::RewardHumanDefinition>().definition)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardHumanDefinition>().derived)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardHumanDefinition>().experience)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardHumanDefinition>().pending)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardHumanDefinition>().notice_pending)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardHumanDefinition>().notice_attributes)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::RewardMonsterDefinition> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::RewardMonsterDefinition.defeats", v.defeats);
  a.field("ark::simulation::rules::RewardMonsterDefinition.growth", v.growth);
  a.field("ark::simulation::rules::RewardMonsterDefinition.base_death_reward", v.base_death_reward);
  a.field("ark::simulation::rules::RewardMonsterDefinition.base_cash_reward", v.base_cash_reward);
  a.field("ark::simulation::rules::RewardMonsterDefinition.base_hp", v.base_hp);
  a.field("ark::simulation::rules::RewardMonsterDefinition.body", v.body);
  a.field("ark::simulation::rules::RewardMonsterDefinition.sprite_variant", v.sprite_variant);
  a.field("ark::simulation::rules::RewardMonsterDefinition.required_progress", v.required_progress);
  a.field("ark::simulation::rules::RewardMonsterDefinition.status", v.status);
  a.field("ark::simulation::rules::RewardMonsterDefinition.newly_unlocked", v.newly_unlocked);
  a.field("ark::simulation::rules::RewardMonsterDefinition.introduced", v.introduced);
  a.field("ark::simulation::rules::RewardMonsterDefinition.has_introduction_script", v.has_introduction_script);
  a.field("ark::simulation::rules::RewardMonsterDefinition.base_attack", v.base_attack);
  a.field("ark::simulation::rules::RewardMonsterDefinition.base_defense", v.base_defense);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::RewardMonsterDefinition>().defeats)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardMonsterDefinition>().growth)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardMonsterDefinition>().base_death_reward)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardMonsterDefinition>().base_cash_reward)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardMonsterDefinition>().base_hp)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardMonsterDefinition>().body)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardMonsterDefinition>().sprite_variant)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardMonsterDefinition>().required_progress)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardMonsterDefinition>().status)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardMonsterDefinition>().newly_unlocked)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardMonsterDefinition>().introduced)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardMonsterDefinition>().has_introduction_script)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardMonsterDefinition>().base_attack)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::RewardMonsterDefinition>().base_defense)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::ShopActorRecord> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::ShopActorRecord.weapon", v.weapon);
  a.field("ark::simulation::rules::ShopActorRecord.selected_weapon", v.selected_weapon);
  a.field("ark::simulation::rules::ShopActorRecord.selected_armor", v.selected_armor);
  a.field("ark::simulation::rules::ShopActorRecord.selected_accessory", v.selected_accessory);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::ShopActorRecord>().weapon)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ShopActorRecord>().selected_weapon)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ShopActorRecord>().selected_armor)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ShopActorRecord>().selected_accessory)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::ShopHumanRecord> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::ShopHumanRecord.equipment", v.equipment);
  a.field("ark::simulation::rules::ShopHumanRecord.reselect", v.reselect);
  a.field("ark::simulation::rules::ShopHumanRecord.satisfaction", v.satisfaction);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::ShopHumanRecord>().equipment)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ShopHumanRecord>().reselect)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::ShopHumanRecord>().satisfaction)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::SnapshotFacilityTarget> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::SnapshotFacilityTarget.drawn_active_index", v.drawn_active_index);
  a.field("ark::simulation::rules::SnapshotFacilityTarget.drawn_snapshot_index", v.drawn_snapshot_index);
  a.field("ark::simulation::rules::SnapshotFacilityTarget.goal_snapshot_index", v.goal_snapshot_index);
  a.field("ark::simulation::rules::SnapshotFacilityTarget.goal", v.goal);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::SnapshotFacilityTarget>().drawn_active_index)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::SnapshotFacilityTarget>().drawn_snapshot_index)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::SnapshotFacilityTarget>().goal_snapshot_index)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::SnapshotFacilityTarget>().goal)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::TaskRecruitmentAnimation> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::TaskRecruitmentAnimation.entries", v.entries);
  a.field("ark::simulation::rules::TaskRecruitmentAnimation.portraits", v.portraits);
  a.field("ark::simulation::rules::TaskRecruitmentAnimation.arrival_extent", v.arrival_extent);
  a.field("ark::simulation::rules::TaskRecruitmentAnimation.completion_tick", v.completion_tick);
  a.field("ark::simulation::rules::TaskRecruitmentAnimation.portrait_timer", v.portrait_timer);
  a.field("ark::simulation::rules::TaskRecruitmentAnimation.displayed_count", v.displayed_count);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::TaskRecruitmentAnimation>().entries)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::TaskRecruitmentAnimation>().portraits)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::TaskRecruitmentAnimation>().arrival_extent)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::TaskRecruitmentAnimation>().completion_tick)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::TaskRecruitmentAnimation>().portrait_timer)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::TaskRecruitmentAnimation>().displayed_count)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::TaskRecruitmentEntry> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::TaskRecruitmentEntry.human", v.human);
  a.field("ark::simulation::rules::TaskRecruitmentEntry.counter", v.counter);
  a.field("ark::simulation::rules::TaskRecruitmentEntry.phrase", v.phrase);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::TaskRecruitmentEntry>().human)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::TaskRecruitmentEntry>().counter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::TaskRecruitmentEntry>().phrase)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::TownBounds> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::TownBounds.left", v.left);
  a.field("ark::simulation::rules::TownBounds.right", v.right);
  a.field("ark::simulation::rules::TownBounds.top", v.top);
  a.field("ark::simulation::rules::TownBounds.bottom", v.bottom);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::TownBounds>().left)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::TownBounds>().right)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::TownBounds>().top)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::TownBounds>().bottom)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::WorldCalendarState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::WorldCalendarState.year", v.year);
  a.field("ark::simulation::rules::WorldCalendarState.month", v.month);
  a.field("ark::simulation::rules::WorldCalendarState.subperiod", v.subperiod);
  a.field("ark::simulation::rules::WorldCalendarState.units", v.units);
  a.field("ark::simulation::rules::WorldCalendarState.previous_units", v.previous_units);
  a.field("ark::simulation::rules::WorldCalendarState.month_ticks", v.month_ticks);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::WorldCalendarState>().year)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldCalendarState>().month)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldCalendarState>().subperiod)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldCalendarState>().units)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldCalendarState>().previous_units)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldCalendarState>().month_ticks)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::WorldEventTask> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::WorldEventTask.definition_task_flag", v.definition_task_flag);
  a.field("ark::simulation::rules::WorldEventTask.kind", v.kind);
  a.field("ark::simulation::rules::WorldEventTask.center", v.center);
  a.field("ark::simulation::rules::WorldEventTask.encounter", v.encounter);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::WorldEventTask>().definition_task_flag)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldEventTask>().kind)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldEventTask>().center)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldEventTask>().encounter)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::WorldExplorationGroundDisplay> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::WorldExplorationGroundDisplay.cell", v.cell);
  a.field("ark::simulation::rules::WorldExplorationGroundDisplay.kind", v.kind);
  a.field("ark::simulation::rules::WorldExplorationGroundDisplay.delay", v.delay);
  a.field("ark::simulation::rules::WorldExplorationGroundDisplay.x_offset", v.x_offset);
  a.field("ark::simulation::rules::WorldExplorationGroundDisplay.y_offset", v.y_offset);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::WorldExplorationGroundDisplay>().cell)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldExplorationGroundDisplay>().kind)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldExplorationGroundDisplay>().delay)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldExplorationGroundDisplay>().x_offset)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldExplorationGroundDisplay>().y_offset)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::WorldExplorationSummary> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::WorldExplorationSummary.legacy_page", v.legacy_page);
  a.field("ark::simulation::rules::WorldExplorationSummary.task", v.task);
  a.field("ark::simulation::rules::WorldExplorationSummary.definition", v.definition);
  a.field("ark::simulation::rules::WorldExplorationSummary.rewards", v.rewards);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::WorldExplorationSummary>().legacy_page)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldExplorationSummary>().task)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldExplorationSummary>().definition)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldExplorationSummary>().rewards)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::WorldFacilityUpdateDefinition> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::WorldFacilityUpdateDefinition.flags", v.flags);
  a.field("ark::simulation::rules::WorldFacilityUpdateDefinition.popularity_reward", v.popularity_reward);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::WorldFacilityUpdateDefinition>().flags)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldFacilityUpdateDefinition>().popularity_reward)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::WorldFacilityUpdateDetails> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::WorldFacilityUpdateDetails.construction_limit", v.construction_limit);
  a.field("ark::simulation::rules::WorldFacilityUpdateDetails.condition", v.condition);
  a.field("ark::simulation::rules::WorldFacilityUpdateDetails.completion_popularity", v.completion_popularity);
  a.field("ark::simulation::rules::WorldFacilityUpdateDetails.residence_mode", v.residence_mode);
  a.field("ark::simulation::rules::WorldFacilityUpdateDetails.resident_definition", v.resident_definition);
  a.field("ark::simulation::rules::WorldFacilityUpdateDetails.notices", v.notices);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::WorldFacilityUpdateDetails>().construction_limit)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldFacilityUpdateDetails>().condition)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldFacilityUpdateDetails>().completion_popularity)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldFacilityUpdateDetails>().residence_mode)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldFacilityUpdateDetails>().resident_definition)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldFacilityUpdateDetails>().notices)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::WorldMapNeighbourCache> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::WorldMapNeighbourCache.current", v.current);
  a.field("ark::simulation::rules::WorldMapNeighbourCache.previous", v.previous);
  a.field("ark::simulation::rules::WorldMapNeighbourCache.sources", v.sources);
  a.field("ark::simulation::rules::WorldMapNeighbourCache.visited", v.visited);
  a.field("ark::simulation::rules::WorldMapNeighbourCache.notices", v.notices);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::WorldMapNeighbourCache>().current)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldMapNeighbourCache>().previous)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldMapNeighbourCache>().sources)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldMapNeighbourCache>().visited)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldMapNeighbourCache>().notices)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::WorldPopularityReward> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::WorldPopularityReward.definition", v.definition);
  a.field("ark::simulation::rules::WorldPopularityReward.legacy_tag", v.legacy_tag);
  a.field("ark::simulation::rules::WorldPopularityReward.threshold", v.threshold);
  a.field("ark::simulation::rules::WorldPopularityReward.human", v.human);
  a.field("ark::simulation::rules::WorldPopularityReward.facility", v.facility);
  a.field("ark::simulation::rules::WorldPopularityReward.program", v.program);
  a.field("ark::simulation::rules::WorldPopularityReward.flags", v.flags);
  a.field("ark::simulation::rules::WorldPopularityReward.status", v.status);
  a.field("ark::simulation::rules::WorldPopularityReward.pending_notice", v.pending_notice);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::WorldPopularityReward>().definition)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldPopularityReward>().legacy_tag)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldPopularityReward>().threshold)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldPopularityReward>().human)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldPopularityReward>().facility)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldPopularityReward>().program)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldPopularityReward>().flags)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldPopularityReward>().status)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldPopularityReward>().pending_notice)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::WorldPosition> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::WorldPosition.x", v.x);
  a.field("ark::simulation::rules::WorldPosition.z", v.z);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::WorldPosition>().x)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldPosition>().z)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::WorldRandomSnapshot> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::WorldRandomSnapshot.engine_state", v.engine_state);
  a.field("ark::simulation::rules::WorldRandomSnapshot.tape", v.tape);
  a.field("ark::simulation::rules::WorldRandomSnapshot.cursor", v.cursor);
  a.field("ark::simulation::rules::WorldRandomSnapshot.tape_mode", v.tape_mode);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::WorldRandomSnapshot>().engine_state)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldRandomSnapshot>().tape)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldRandomSnapshot>().cursor)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldRandomSnapshot>().tape_mode)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::WorldSceneState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::WorldSceneState.world", v.world);
  a.field("ark::simulation::rules::WorldSceneState.random", v.random);
  a.field("ark::simulation::rules::WorldSceneState.calendar", v.calendar);
  a.field("ark::simulation::rules::WorldSceneState.scene_state", v.scene_state);
  a.field("ark::simulation::rules::WorldSceneState.speed_setting", v.speed_setting);
  a.field("ark::simulation::rules::WorldSceneState.frame_counter", v.frame_counter);
  a.field("ark::simulation::rules::WorldSceneState.scene_counter", v.scene_counter);
  a.field("ark::simulation::rules::WorldSceneState.processing_phase", v.processing_phase);
  a.field("ark::simulation::rules::WorldSceneState.processing_subphase", v.processing_subphase);
  a.field("ark::simulation::rules::WorldSceneState.first_normal_refresh", v.first_normal_refresh);
  a.field("ark::simulation::rules::WorldSceneState.top_is_main", v.top_is_main);
  a.field("ark::simulation::rules::WorldSceneState.framework_paused", v.framework_paused);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::WorldSceneState>().world)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldSceneState>().random)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldSceneState>().calendar)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldSceneState>().scene_state)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldSceneState>().speed_setting)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldSceneState>().frame_counter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldSceneState>().scene_counter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldSceneState>().processing_phase)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldSceneState>().processing_subphase)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldSceneState>().first_normal_refresh)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldSceneState>().top_is_main)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldSceneState>().framework_paused)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::WorldScheduleState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::WorldScheduleState.world", v.world);
  a.field("ark::simulation::rules::WorldScheduleState.surface", v.surface);
  a.field("ark::simulation::rules::WorldScheduleState.map_flags", v.map_flags);
  a.field("ark::simulation::rules::WorldScheduleState.town", v.town);
  a.field("ark::simulation::rules::WorldScheduleState.spawn_cells", v.spawn_cells);
  a.field("ark::simulation::rules::WorldScheduleState.facility_order", v.facility_order);
  a.field("ark::simulation::rules::WorldScheduleState.updates", v.updates);
  a.field("ark::simulation::rules::WorldScheduleState.hints", v.hints);
  a.field("ark::simulation::rules::WorldScheduleState.floating_notes", v.floating_notes);
  a.field("ark::simulation::rules::WorldScheduleState.popularity_queue", v.popularity_queue);
  a.field("ark::simulation::rules::WorldScheduleState.rescue_available", v.rescue_available);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScheduleState>().world)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScheduleState>().surface)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScheduleState>().map_flags)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScheduleState>().town)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScheduleState>().spawn_cells)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScheduleState>().facility_order)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScheduleState>().updates)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScheduleState>().hints)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScheduleState>().floating_notes)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScheduleState>().popularity_queue)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScheduleState>().rescue_available)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::WorldScriptContinuation> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::WorldScriptContinuation.event", v.event);
  a.field("ark::simulation::rules::WorldScriptContinuation.next_instruction", v.next_instruction);
  a.field("ark::simulation::rules::WorldScriptContinuation.remaining_updates", v.remaining_updates);
  a.field("ark::simulation::rules::WorldScriptContinuation.legacy_tag", v.legacy_tag);
  a.field("ark::simulation::rules::WorldScriptContinuation.saved_context", v.saved_context);
  a.field("ark::simulation::rules::WorldScriptContinuation.replacement", v.replacement);
  a.field("ark::simulation::rules::WorldScriptContinuation.parameters", v.parameters);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptContinuation>().event)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptContinuation>().next_instruction)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptContinuation>().remaining_updates)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptContinuation>().legacy_tag)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptContinuation>().saved_context)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptContinuation>().replacement)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptContinuation>().parameters)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::WorldScriptFacilityDefinition> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::WorldScriptFacilityDefinition.category", v.category);
  a.field("ark::simulation::rules::WorldScriptFacilityDefinition.level", v.level);
  a.field("ark::simulation::rules::WorldScriptFacilityDefinition.economy", v.economy);
  a.field("ark::simulation::rules::WorldScriptFacilityDefinition.improvements", v.improvements);
  a.field("ark::simulation::rules::WorldScriptFacilityDefinition.attributes", v.attributes);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptFacilityDefinition>().category)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptFacilityDefinition>().level)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptFacilityDefinition>().economy)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptFacilityDefinition>().improvements)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptFacilityDefinition>().attributes)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::WorldScriptFinance> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::WorldScriptFinance.cash", v.cash);
  a.field("ark::simulation::rules::WorldScriptFinance.cash_peak", v.cash_peak);
  a.field("ark::simulation::rules::WorldScriptFinance.cash_peak_village", v.cash_peak_village);
  a.field("ark::simulation::rules::WorldScriptFinance.legacy_flags14", v.legacy_flags14);
  a.field("ark::simulation::rules::WorldScriptFinance.month", v.month);
  a.field("ark::simulation::rules::WorldScriptFinance.localized_gold_template", v.localized_gold_template);
  a.field("ark::simulation::rules::WorldScriptFinance.monthly_totals", v.monthly_totals);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptFinance>().cash)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptFinance>().cash_peak)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptFinance>().cash_peak_village)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptFinance>().legacy_flags14)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptFinance>().month)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptFinance>().localized_gold_template)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptFinance>().monthly_totals)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::WorldScriptNotice> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::WorldScriptNotice.message", v.message);
  a.field("ark::simulation::rules::WorldScriptNotice.counter", v.counter);
  a.field("ark::simulation::rules::WorldScriptNotice.duration", v.duration);
  a.field("ark::simulation::rules::WorldScriptNotice.replacement", v.replacement);
  a.field("ark::simulation::rules::WorldScriptNotice.text", v.text);
  a.field("ark::simulation::rules::WorldScriptNotice.definitions", v.definitions);
  a.field("ark::simulation::rules::WorldScriptNotice.human_definition", v.human_definition);
  a.field("ark::simulation::rules::WorldScriptNotice.attribute_changes", v.attribute_changes);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptNotice>().message)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptNotice>().counter)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptNotice>().duration)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptNotice>().replacement)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptNotice>().text)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptNotice>().definitions)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptNotice>().human_definition)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptNotice>().attribute_changes)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::WorldScriptPage> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::WorldScriptPage.id", v.id);
  a.field("ark::simulation::rules::WorldScriptPage.lifecycle", v.lifecycle);
  a.field("ark::simulation::rules::WorldScriptPage.kind", v.kind);
  a.field("ark::simulation::rules::WorldScriptPage.legacy_page", v.legacy_page);
  a.field("ark::simulation::rules::WorldScriptPage.source_record", v.source_record);
  a.field("ark::simulation::rules::WorldScriptPage.replacement", v.replacement);
  a.field("ark::simulation::rules::WorldScriptPage.title", v.title);
  a.field("ark::simulation::rules::WorldScriptPage.paragraphs", v.paragraphs);
  a.field("ark::simulation::rules::WorldScriptPage.speaker_kind", v.speaker_kind);
  a.field("ark::simulation::rules::WorldScriptPage.speaker_definition", v.speaker_definition);
  a.field("ark::simulation::rules::WorldScriptPage.legacy_tag", v.legacy_tag);
  a.field("ark::simulation::rules::WorldScriptPage.legacy_r", v.legacy_r);
  a.field("ark::simulation::rules::WorldScriptPage.legacy_s", v.legacy_s);
  a.field("ark::simulation::rules::WorldScriptPage.legacy_t", v.legacy_t);
  a.field("ark::simulation::rules::WorldScriptPage.legacy_f", v.legacy_f);
  a.field("ark::simulation::rules::WorldScriptPage.legacy_g", v.legacy_g);
  a.field("ark::simulation::rules::WorldScriptPage.legacy_l", v.legacy_l);
  a.field("ark::simulation::rules::WorldScriptPage.message_commands", v.message_commands);
  a.field("ark::simulation::rules::WorldScriptPage.task_identity", v.task_identity);
  a.field("ark::simulation::rules::WorldScriptPage.task_definition", v.task_definition);
  a.field("ark::simulation::rules::WorldScriptPage.monster_definition", v.monster_definition);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().lifecycle)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().kind)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().legacy_page)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().source_record)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().replacement)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().title)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().paragraphs)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().speaker_kind)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().speaker_definition)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().legacy_tag)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().legacy_r)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().legacy_s)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().legacy_t)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().legacy_f)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().legacy_g)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().legacy_l)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().message_commands)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().task_identity)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().task_definition)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptPage>().monster_definition)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::WorldScriptState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::WorldScriptState.event_calls", v.event_calls);
  a.field("ark::simulation::rules::WorldScriptState.continuations", v.continuations);
  a.field("ark::simulation::rules::WorldScriptState.context", v.context);
  a.field("ark::simulation::rules::WorldScriptState.village_name", v.village_name);
  a.field("ark::simulation::rules::WorldScriptState.pending_completion", v.pending_completion);
  a.field("ark::simulation::rules::WorldScriptState.popularity_queue", v.popularity_queue);
  a.field("ark::simulation::rules::WorldScriptState.pages", v.pages);
  a.field("ark::simulation::rules::WorldScriptState.executing_page", v.executing_page);
  a.field("ark::simulation::rules::WorldScriptState.next_page_id", v.next_page_id);
  a.field("ark::simulation::rules::WorldScriptState.page_mutations_locked", v.page_mutations_locked);
  a.field("ark::simulation::rules::WorldScriptState.redraw_requested", v.redraw_requested);
  a.field("ark::simulation::rules::WorldScriptState.selected_actor", v.selected_actor);
  a.field("ark::simulation::rules::WorldScriptState.selected_monster", v.selected_monster);
  a.field("ark::simulation::rules::WorldScriptState.selected_facility", v.selected_facility);
  a.field("ark::simulation::rules::WorldScriptState.human_order", v.human_order);
  a.field("ark::simulation::rules::WorldScriptState.scene_mode", v.scene_mode);
  a.field("ark::simulation::rules::WorldScriptState.scene_updates", v.scene_updates);
  a.field("ark::simulation::rules::WorldScriptState.scene_labels", v.scene_labels);
  a.field("ark::simulation::rules::WorldScriptState.selection_mode", v.selection_mode);
  a.field("ark::simulation::rules::WorldScriptState.finance", v.finance);
  a.field("ark::simulation::rules::WorldScriptState.notices", v.notices);
  a.field("ark::simulation::rules::WorldScriptState.activities", v.activities);
  a.field("ark::simulation::rules::WorldScriptState.humans", v.humans);
  a.field("ark::simulation::rules::WorldScriptState.professions", v.professions);
  a.field("ark::simulation::rules::WorldScriptState.facilities", v.facilities);
  a.field("ark::simulation::rules::WorldScriptState.human_catalog_complete", v.human_catalog_complete);
  a.field("ark::simulation::rules::WorldScriptState.facility_catalog_complete", v.facility_catalog_complete);
  a.field("ark::simulation::rules::WorldScriptState.job_counts", v.job_counts);
  a.field("ark::simulation::rules::WorldScriptState.medal_count", v.medal_count);
  a.field("ark::simulation::rules::WorldScriptState.exploration_phase", v.exploration_phase);
  a.field("ark::simulation::rules::WorldScriptState.user_flags", v.user_flags);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().event_calls)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().continuations)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().context)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().village_name)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().pending_completion)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().popularity_queue)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().pages)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().executing_page)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().next_page_id)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().page_mutations_locked)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().redraw_requested)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().selected_actor)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().selected_monster)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().selected_facility)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().human_order)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().scene_mode)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().scene_updates)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().scene_labels)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().selection_mode)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().finance)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().notices)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().activities)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().humans)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().professions)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().facilities)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().human_catalog_complete)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().facility_catalog_complete)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().job_counts)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().medal_count)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().exploration_phase)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptState>().user_flags)>::minimum(); }
};
template<> struct CodecFields< ::ark::simulation::rules::WorldScriptUnlockDefinition> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("ark::simulation::rules::WorldScriptUnlockDefinition.status", v.status);
  a.field("ark::simulation::rules::WorldScriptUnlockDefinition.pending_notice", v.pending_notice);
  a.field("ark::simulation::rules::WorldScriptUnlockDefinition.name", v.name);
  a.field("ark::simulation::rules::WorldScriptUnlockDefinition.extra", v.extra);
  a.field("ark::simulation::rules::WorldScriptUnlockDefinition.satisfaction", v.satisfaction);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptUnlockDefinition>().status)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptUnlockDefinition>().pending_notice)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptUnlockDefinition>().name)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptUnlockDefinition>().extra)>::minimum() + Codec<decltype(std::declval< ::ark::simulation::rules::WorldScriptUnlockDefinition>().satisfaction)>::minimum(); }
};
