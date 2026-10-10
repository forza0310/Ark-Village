// 由 scripts/generate_owner_codec.mjs 生成；不要手改。
constexpr const char schema_identity[] = "8af417e3aa669e375feacbd46c4b96c7c5651bcabdd46e2804c98c5ce78e247f";
template<> bool valid_enum< ::dungeon_village_prototype::StartupAudioOperation>(::dungeon_village_prototype::StartupAudioOperation value) { return value == ::dungeon_village_prototype::StartupAudioOperation::ordinary_play || value == ::dungeon_village_prototype::StartupAudioOperation::replace_bgm || value == ::dungeon_village_prototype::StartupAudioOperation::jingle; }
template<> bool valid_enum< ::dungeon_village_reference::ActorKind>(::dungeon_village_reference::ActorKind value) { return value == ::dungeon_village_reference::ActorKind::human || value == ::dungeon_village_reference::ActorKind::monster; }
template<> bool valid_enum< ::dungeon_village_reference::CandidateOrigin>(::dungeon_village_reference::CandidateOrigin value) { return value == ::dungeon_village_reference::CandidateOrigin::map_scan || value == ::dungeon_village_reference::CandidateOrigin::event; }
template<> bool valid_enum< ::dungeon_village_reference::CashCategory>(::dungeon_village_reference::CashCategory value) { return value == ::dungeon_village_reference::CashCategory::facilities || value == ::dungeon_village_reference::CashCategory::monsters || value == ::dungeon_village_reference::CashCategory::adventurers || value == ::dungeon_village_reference::CashCategory::shop || value == ::dungeon_village_reference::CashCategory::other; }
template<> bool valid_enum< ::dungeon_village_reference::CashDirection>(::dungeon_village_reference::CashDirection value) { return value == ::dungeon_village_reference::CashDirection::income || value == ::dungeon_village_reference::CashDirection::expense; }
template<> bool valid_enum< ::dungeon_village_reference::DungeonCrewRequestKind>(::dungeon_village_reference::DungeonCrewRequestKind value) { return value == ::dungeon_village_reference::DungeonCrewRequestKind::retreat || value == ::dungeon_village_reference::DungeonCrewRequestKind::progress_label231 || value == ::dungeon_village_reference::DungeonCrewRequestKind::grant_catalog_reward || value == ::dungeon_village_reference::DungeonCrewRequestKind::spawn_catalog_reward || value == ::dungeon_village_reference::DungeonCrewRequestKind::exit_crew; }
template<> bool valid_enum< ::dungeon_village_reference::FacilityOrientation>(::dungeon_village_reference::FacilityOrientation value) { return value == ::dungeon_village_reference::FacilityOrientation::first || value == ::dungeon_village_reference::FacilityOrientation::second; }
template<> bool valid_enum< ::dungeon_village_reference::FacilityShape>(::dungeon_village_reference::FacilityShape value) { return value == ::dungeon_village_reference::FacilityShape::single || value == ::dungeon_village_reference::FacilityShape::pair || value == ::dungeon_village_reference::FacilityShape::square; }
template<> bool valid_enum< ::dungeon_village_reference::MapAccessError>(::dungeon_village_reference::MapAccessError value) { return value == ::dungeon_village_reference::MapAccessError::none || value == ::dungeon_village_reference::MapAccessError::invalid_map || value == ::dungeon_village_reference::MapAccessError::invalid_position || value == ::dungeon_village_reference::MapAccessError::invalid_limits || value == ::dungeon_village_reference::MapAccessError::invalid_layout || value == ::dungeon_village_reference::MapAccessError::unsupported_kind || value == ::dungeon_village_reference::MapAccessError::invalid_field || value == ::dungeon_village_reference::MapAccessError::binding_mismatch || value == ::dungeon_village_reference::MapAccessError::unreachable || value == ::dungeon_village_reference::MapAccessError::cost_limit || value == ::dungeon_village_reference::MapAccessError::expansion_limit; }
template<> bool valid_enum< ::dungeon_village_reference::ProjectileKind>(::dungeon_village_reference::ProjectileKind value) { return value == ::dungeon_village_reference::ProjectileKind::arrow || value == ::dungeon_village_reference::ProjectileKind::spell || value == ::dungeon_village_reference::ProjectileKind::delayed_damage; }
template<> bool valid_enum< ::dungeon_village_reference::RouteCategory>(::dungeon_village_reference::RouteCategory value) { return value == ::dungeon_village_reference::RouteCategory::terminal || value == ::dungeon_village_reference::RouteCategory::road || value == ::dungeon_village_reference::RouteCategory::ground || value == ::dungeon_village_reference::RouteCategory::blocked || value == ::dungeon_village_reference::RouteCategory::access; }
template<> bool valid_enum< ::dungeon_village_reference::WorldScriptPageKind>(::dungeon_village_reference::WorldScriptPageKind value) { return value == ::dungeon_village_reference::WorldScriptPageKind::scene || value == ::dungeon_village_reference::WorldScriptPageKind::dialogue || value == ::dungeon_village_reference::WorldScriptPageKind::simple_message || value == ::dungeon_village_reference::WorldScriptPageKind::newspaper || value == ::dungeon_village_reference::WorldScriptPageKind::raw_page; }
template<> struct CodecFields< ::dungeon_village_prototype::StartupAudioRequest> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_prototype::StartupAudioRequest.operation", v.operation);
  a.field("dungeon_village_prototype::StartupAudioRequest.id", v.id);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupAudioRequest>().operation)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupAudioRequest>().id)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_prototype::StartupHumanDetailContext> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_prototype::StartupHumanDetailContext.chase_mode", v.chase_mode);
  a.field("dungeon_village_prototype::StartupHumanDetailContext.actor", v.actor);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupHumanDetailContext>().chase_mode)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupHumanDetailContext>().actor)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_prototype::StartupInformationPageData> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_prototype::StartupInformationPageData.lists", v.lists);
  a.field("dungeon_village_prototype::StartupInformationPageData.selection", v.selection);
  a.field("dungeon_village_prototype::StartupInformationPageData.first_visible", v.first_visible);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupInformationPageData>().lists)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupInformationPageData>().selection)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupInformationPageData>().first_visible)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_prototype::StartupWorldActorMetadata> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_prototype::StartupWorldActorMetadata.sex", v.sex);
  a.field("dungeon_village_prototype::StartupWorldActorMetadata.profession", v.profession);
  a.field("dungeon_village_prototype::StartupWorldActorMetadata.weapon", v.weapon);
  a.field("dungeon_village_prototype::StartupWorldActorMetadata.cached_view", v.cached_view);
  a.field("dungeon_village_prototype::StartupWorldActorMetadata.render_position", v.render_position);
  a.field("dungeon_village_prototype::StartupWorldActorMetadata.cached_screen_position", v.cached_screen_position);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldActorMetadata>().sex)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldActorMetadata>().profession)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldActorMetadata>().weapon)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldActorMetadata>().cached_view)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldActorMetadata>().render_position)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldActorMetadata>().cached_screen_position)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_prototype::StartupWorldFocusActor> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_prototype::StartupWorldFocusActor.actor", v.actor);
  a.field("dungeon_village_prototype::StartupWorldFocusActor.perception", v.perception);
  a.field("dungeon_village_prototype::StartupWorldFocusActor.routes", v.routes);
  a.field("dungeon_village_prototype::StartupWorldFocusActor.metadata", v.metadata);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldFocusActor>().actor)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldFocusActor>().perception)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldFocusActor>().routes)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldFocusActor>().metadata)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_prototype::StartupWorldHumanCalendar> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_prototype::StartupWorldHumanCalendar.absent_months", v.absent_months);
  a.field("dungeon_village_prototype::StartupWorldHumanCalendar.yearly_totals", v.yearly_totals);
  a.field("dungeon_village_prototype::StartupWorldHumanCalendar.legacy_F", v.legacy_F);
  a.field("dungeon_village_prototype::StartupWorldHumanCalendar.legacy_G", v.legacy_G);
  a.field("dungeon_village_prototype::StartupWorldHumanCalendar.continuation_cost", v.continuation_cost);
  a.field("dungeon_village_prototype::StartupWorldHumanCalendar.contribution", v.contribution);
  a.field("dungeon_village_prototype::StartupWorldHumanCalendar.celebrations", v.celebrations);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldHumanCalendar>().absent_months)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldHumanCalendar>().yearly_totals)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldHumanCalendar>().legacy_F)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldHumanCalendar>().legacy_G)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldHumanCalendar>().continuation_cost)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldHumanCalendar>().contribution)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldHumanCalendar>().celebrations)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_prototype::StartupWorldHumanProfile> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_prototype::StartupWorldHumanProfile.name", v.name);
  a.field("dungeon_village_prototype::StartupWorldHumanProfile.sex", v.sex);
  a.field("dungeon_village_prototype::StartupWorldHumanProfile.custom_name", v.custom_name);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldHumanProfile>().name)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldHumanProfile>().sex)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldHumanProfile>().custom_name)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_prototype::StartupWorldRuntimeState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.scene", v.scene);
  // rules静态目录由decode参数重绑定，不编码地址。
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.task", v.task);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.shop_humans", v.shop_humans);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.shop_actors", v.shop_actors);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.items", v.items);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.dungeon_facilities", v.dungeon_facilities);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.dungeon_actors", v.dungeon_actors);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.catalog", v.catalog);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.shops", v.shops);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.shop_order", v.shop_order);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.item_rewards", v.item_rewards);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.human_definition_state", v.human_definition_state);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.human_profiles", v.human_profiles);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.human_homes", v.human_homes);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.human_presence", v.human_presence);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.human_flags", v.human_flags);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.human_calendar", v.human_calendar);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.actor_metadata", v.actor_metadata);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_original_ids", v.facility_original_ids);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_ordinals", v.facility_ordinals);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_residents", v.facility_residents);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_difficulties", v.facility_difficulties);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_flags", v.facility_flags);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.neighbourhood", v.neighbourhood);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.neighbourhood_details", v.neighbourhood_details);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.exploration_summaries", v.exploration_summaries);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.exploration_displays", v.exploration_displays);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.dungeon_labels", v.dungeon_labels);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.surface", v.surface);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.road_patches", v.road_patches);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.scripts", v.scripts);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.cash_peak", v.cash_peak);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.cash_peak_village", v.cash_peak_village);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.monthly_cash", v.monthly_cash);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_monthly_cash", v.facility_monthly_cash);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_month_age", v.facility_month_age);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.shop_item_stock", v.shop_item_stock);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.report_state", v.report_state);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.report_counter", v.report_counter);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.report_snapshot", v.report_snapshot);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.report_records", v.report_records);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.report_new_records", v.report_new_records);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.report_portraits", v.report_portraits);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.report_kills", v.report_kills);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.maximum_income", v.maximum_income);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.village_points", v.village_points);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.popularity", v.popularity);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.maximum_popularity", v.maximum_popularity);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.popularity_display", v.popularity_display);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.popularity_pulses", v.popularity_pulses);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.popularity_rewards", v.popularity_rewards);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_definitions", v.facility_definitions);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_presence", v.facility_presence);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.residence_catalog_available", v.residence_catalog_available);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.page_human_bindings", v.page_human_bindings);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.task_abort_questions", v.task_abort_questions);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.task_abort_answers", v.task_abort_answers);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.human_pages_initialized", v.human_pages_initialized);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.human_detail_contexts", v.human_detail_contexts);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.human_profession_changes", v.human_profession_changes);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.human_page_catalogs", v.human_page_catalogs);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.equipment_page_catalogs", v.equipment_page_catalogs);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.human_page_selections", v.human_page_selections);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.page_job_bindings", v.page_job_bindings);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.human_page_parents", v.human_page_parents);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.human_page_answers", v.human_page_answers);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.human_equipment_choices", v.human_equipment_choices);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.human_attribute_display", v.human_attribute_display);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.equipment_attribute_display", v.equipment_attribute_display);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.human_gift_scores", v.human_gift_scores);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.human_gift_messages", v.human_gift_messages);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.tax_page_residents", v.tax_page_residents);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.tax_page_selection", v.tax_page_selection);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.tax_page_scroll", v.tax_page_scroll);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.residence_hint_counter", v.residence_hint_counter);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.activity_flags", v.activity_flags);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.activity_counts", v.activity_counts);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.human_activity_previous", v.human_activity_previous);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.activity_pages_initialized", v.activity_pages_initialized);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.activity_page_bindings", v.activity_page_bindings);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.activity_page_lists", v.activity_page_lists);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.activity_page_display_humans", v.activity_page_display_humans);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.activity_page_parents", v.activity_page_parents);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.activity_page_answers", v.activity_page_answers);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.activity_page_selections", v.activity_page_selections);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.activity_page_scroll", v.activity_page_scroll);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_details", v.facility_details);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.task_progress", v.task_progress);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.tasks", v.tasks);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.task_order", v.task_order);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.task_original_ids", v.task_original_ids);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.task_sequence", v.task_sequence);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.next_facility_identity", v.next_facility_identity);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.next_task_identity", v.next_task_identity);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.task_replay_order", v.task_replay_order);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.task_special_selection", v.task_special_selection);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.task_special_selection_index", v.task_special_selection_index);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.task_special_selection_list", v.task_special_selection_list);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.fence_level", v.fence_level);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.base_variants", v.base_variants);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.active_task", v.active_task);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.participants", v.participants);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.crew_summaries", v.crew_summaries);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.sites", v.sites);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.ground_definition", v.ground_definition);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.special_ground_definition", v.special_ground_definition);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.arrival_counter", v.arrival_counter);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.camera_delay", v.camera_delay);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.camera_follow", v.camera_follow);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.event89_count", v.event89_count);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.entry_updates", v.entry_updates);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.global_updates", v.global_updates);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.camera", v.camera);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.previous_camera", v.previous_camera);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.camera_velocity", v.camera_velocity);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.build_mode", v.build_mode);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.build_definition", v.build_definition);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.build_anchor", v.build_anchor);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.build_moving_facility", v.build_moving_facility);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.build_feedback_counter", v.build_feedback_counter);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.build_feedback_message", v.build_feedback_message);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.focus_actor", v.focus_actor);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.focus_held_input", v.focus_held_input);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.visual_effects", v.visual_effects);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.delayed_effects", v.delayed_effects);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.global_effects", v.global_effects);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.sound_requests", v.sound_requests);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.reference_viewport", v.reference_viewport);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.page_counters", v.page_counters);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.page_phases", v.page_phases);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.information_page_data", v.information_page_data);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.task_page_lists", v.task_page_lists);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.task_recruitment_pages", v.task_recruitment_pages);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.task_extra_pages", v.task_extra_pages);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.task_page_predictions", v.task_page_predictions);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.task_page_acceleration", v.task_page_acceleration);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.page_secondary_counters", v.page_secondary_counters);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.task_display_table", v.task_display_table);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.task_display_initialized", v.task_display_initialized);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.deadline_initialized", v.deadline_initialized);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.deadline_grades", v.deadline_grades);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.deadline_returns", v.deadline_returns);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.deadline_closed_page", v.deadline_closed_page);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.award_rankings", v.award_rankings);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.award_announced", v.award_announced);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.award_termination_pending", v.award_termination_pending);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.award_pending_humans", v.award_pending_humans);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.reward_display", v.reward_display);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.effort_display", v.effort_display);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_page_bindings", v.facility_page_bindings);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_definition_page_bindings", v.facility_definition_page_bindings);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_page_neighbours", v.facility_page_neighbours);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.build_page_catalogs", v.build_page_catalogs);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.residence_page_candidates", v.residence_page_candidates);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_upgrade_initialized", v.facility_upgrade_initialized);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_upgrade_display", v.facility_upgrade_display);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_item_confirmations", v.facility_item_confirmations);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_item_pages_initialized", v.facility_item_pages_initialized);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_item_page_items", v.facility_item_page_items);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_item_page_lists", v.facility_item_page_lists);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_item_page_selections", v.facility_item_page_selections);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_item_response", v.facility_item_response);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.item_commerce_read", v.item_commerce_read);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_commerce_read", v.facility_commerce_read);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.commerce_pages_initialized", v.commerce_pages_initialized);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.commerce_page_data", v.commerce_page_data);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.commerce_page_lists", v.commerce_page_lists);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_catalog_pages_initialized", v.facility_catalog_pages_initialized);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_catalog_page_data", v.facility_catalog_page_data);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_catalog_page_lists", v.facility_catalog_page_lists);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_catalog_page_parents", v.facility_catalog_page_parents);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.magic_pot_recipes", v.magic_pot_recipes);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.magic_pot_display", v.magic_pot_display);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.magic_pot_comment", v.magic_pot_comment);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.magic_pot_output", v.magic_pot_output);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.magic_pot_pages_initialized", v.magic_pot_pages_initialized);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.magic_pot_page_data", v.magic_pot_page_data);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.magic_pot_page_lists", v.magic_pot_page_lists);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.magic_pot_page_parents", v.magic_pot_page_parents);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.medal_count", v.medal_count);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_free_builds", v.facility_free_builds);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_unlock_counters", v.facility_unlock_counters);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.facility_unlock_notices", v.facility_unlock_notices);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.confirm_input", v.confirm_input);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.cancel_input", v.cancel_input);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.menu_input", v.menu_input);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.page_confirm_held", v.page_confirm_held);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.rank", v.rank);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.quarter_counter", v.quarter_counter);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.legacy_D", v.legacy_D);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.legacy_n", v.legacy_n);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.yearly_statistics", v.yearly_statistics);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.events_held", v.events_held);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.task_subperiods", v.task_subperiods);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.generation_retry", v.generation_retry);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.deadline_page", v.deadline_page);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.completion_mode", v.completion_mode);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.system_completion_mode", v.system_completion_mode);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.save_marker", v.save_marker);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.system_unlock_data", v.system_unlock_data);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.rank_met", v.rank_met);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.rank_values", v.rank_values);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.rank_history", v.rank_history);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.rank_celebration_participants", v.rank_celebration_participants);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.simulation_steps", v.simulation_steps);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.clock_parameter", v.clock_parameter);
  a.field("dungeon_village_prototype::StartupWorldRuntimeState.calendar_advance", v.calendar_advance);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().scene)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().task)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().shop_humans)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().shop_actors)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().items)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().dungeon_facilities)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().dungeon_actors)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().catalog)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().shops)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().shop_order)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().item_rewards)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().human_definition_state)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().human_profiles)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().human_homes)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().human_presence)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().human_flags)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().human_calendar)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().actor_metadata)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_original_ids)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_ordinals)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_residents)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_difficulties)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_flags)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().neighbourhood)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().neighbourhood_details)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().exploration_summaries)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().exploration_displays)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().dungeon_labels)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().surface)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().road_patches)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().scripts)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().cash_peak)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().cash_peak_village)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().monthly_cash)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_monthly_cash)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_month_age)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().shop_item_stock)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().report_state)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().report_counter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().report_snapshot)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().report_records)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().report_new_records)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().report_portraits)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().report_kills)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().maximum_income)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().village_points)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().popularity)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().maximum_popularity)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().popularity_display)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().popularity_pulses)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().popularity_rewards)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_definitions)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_presence)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().residence_catalog_available)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().page_human_bindings)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().task_abort_questions)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().task_abort_answers)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().human_pages_initialized)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().human_detail_contexts)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().human_profession_changes)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().human_page_catalogs)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().equipment_page_catalogs)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().human_page_selections)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().page_job_bindings)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().human_page_parents)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().human_page_answers)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().human_equipment_choices)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().human_attribute_display)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().equipment_attribute_display)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().human_gift_scores)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().human_gift_messages)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().tax_page_residents)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().tax_page_selection)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().tax_page_scroll)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().residence_hint_counter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().activity_flags)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().activity_counts)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().human_activity_previous)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().activity_pages_initialized)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().activity_page_bindings)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().activity_page_lists)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().activity_page_display_humans)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().activity_page_parents)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().activity_page_answers)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().activity_page_selections)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().activity_page_scroll)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_details)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().task_progress)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().tasks)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().task_order)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().task_original_ids)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().task_sequence)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().next_facility_identity)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().next_task_identity)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().task_replay_order)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().task_special_selection)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().task_special_selection_index)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().task_special_selection_list)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().fence_level)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().base_variants)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().active_task)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().participants)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().crew_summaries)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().sites)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().ground_definition)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().special_ground_definition)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().arrival_counter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().camera_delay)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().camera_follow)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().event89_count)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().entry_updates)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().global_updates)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().camera)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().previous_camera)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().camera_velocity)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().build_mode)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().build_definition)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().build_anchor)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().build_moving_facility)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().build_feedback_counter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().build_feedback_message)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().focus_actor)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().focus_held_input)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().visual_effects)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().delayed_effects)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().global_effects)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().sound_requests)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().reference_viewport)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().page_counters)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().page_phases)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().information_page_data)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().task_page_lists)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().task_recruitment_pages)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().task_extra_pages)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().task_page_predictions)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().task_page_acceleration)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().page_secondary_counters)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().task_display_table)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().task_display_initialized)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().deadline_initialized)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().deadline_grades)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().deadline_returns)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().deadline_closed_page)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().award_rankings)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().award_announced)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().award_termination_pending)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().award_pending_humans)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().reward_display)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().effort_display)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_page_bindings)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_definition_page_bindings)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_page_neighbours)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().build_page_catalogs)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().residence_page_candidates)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_upgrade_initialized)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_upgrade_display)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_item_confirmations)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_item_pages_initialized)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_item_page_items)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_item_page_lists)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_item_page_selections)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_item_response)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().item_commerce_read)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_commerce_read)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().commerce_pages_initialized)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().commerce_page_data)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().commerce_page_lists)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_catalog_pages_initialized)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_catalog_page_data)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_catalog_page_lists)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_catalog_page_parents)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().magic_pot_recipes)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().magic_pot_display)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().magic_pot_comment)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().magic_pot_output)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().magic_pot_pages_initialized)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().magic_pot_page_data)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().magic_pot_page_lists)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().magic_pot_page_parents)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().medal_count)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_free_builds)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_unlock_counters)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().facility_unlock_notices)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().confirm_input)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().cancel_input)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().menu_input)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().page_confirm_held)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().rank)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().quarter_counter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().legacy_D)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().legacy_n)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().yearly_statistics)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().events_held)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().task_subperiods)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().generation_retry)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().deadline_page)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().completion_mode)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().system_completion_mode)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().save_marker)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().system_unlock_data)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().rank_met)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().rank_values)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().rank_history)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().rank_celebration_participants)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().simulation_steps)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().clock_parameter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_prototype::StartupWorldRuntimeState>().calendar_advance)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::AccountingReport> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::AccountingReport.input", v.input);
  a.field("dungeon_village_reference::AccountingReport.categories", v.categories);
  a.field("dungeon_village_reference::AccountingReport.displayed", v.displayed);
  a.field("dungeon_village_reference::AccountingReport.displayed_net", v.displayed_net);
  a.field("dungeon_village_reference::AccountingReport.pending_points", v.pending_points);
  a.field("dungeon_village_reference::AccountingReport.awarded_points", v.awarded_points);
  a.field("dungeon_village_reference::AccountingReport.claimed", v.claimed);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::AccountingReport>().input)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AccountingReport>().categories)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AccountingReport>().displayed)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AccountingReport>().displayed_net)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AccountingReport>().pending_points)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AccountingReport>().awarded_points)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AccountingReport>().claimed)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::ActivityCandidateCell> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::ActivityCandidateCell.position", v.position);
  a.field("dungeon_village_reference::ActivityCandidateCell.definition", v.definition);
  a.field("dungeon_village_reference::ActivityCandidateCell.instance", v.instance);
  a.field("dungeon_village_reference::ActivityCandidateCell.cost", v.cost);
  a.field("dungeon_village_reference::ActivityCandidateCell.origin", v.origin);
  a.field("dungeon_village_reference::ActivityCandidateCell.source_index", v.source_index);
  a.field("dungeon_village_reference::ActivityCandidateCell.instance_definition", v.instance_definition);
  a.field("dungeon_village_reference::ActivityCandidateCell.legacy_state", v.legacy_state);
  a.field("dungeon_village_reference::ActivityCandidateCell.route_category", v.route_category);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::ActivityCandidateCell>().position)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ActivityCandidateCell>().definition)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ActivityCandidateCell>().instance)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ActivityCandidateCell>().cost)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ActivityCandidateCell>().origin)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ActivityCandidateCell>().source_index)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ActivityCandidateCell>().instance_definition)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ActivityCandidateCell>().legacy_state)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ActivityCandidateCell>().route_category)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::ActorControlState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::ActorControlState.flags", v.flags);
  a.field("dungeon_village_reference::ActorControlState.state", v.state);
  a.field("dungeon_village_reference::ActorControlState.action", v.action);
  a.field("dungeon_village_reference::ActorControlState.action_counter", v.action_counter);
  a.field("dungeon_village_reference::ActorControlState.alternate_counter", v.alternate_counter);
  a.field("dungeon_village_reference::ActorControlState.facing", v.facing);
  a.field("dungeon_village_reference::ActorControlState.queue", v.queue);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::ActorControlState>().flags)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ActorControlState>().state)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ActorControlState>().action)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ActorControlState>().action_counter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ActorControlState>().alternate_counter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ActorControlState>().facing)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ActorControlState>().queue)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::ActorEffectState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::ActorEffectState.display", v.display);
  a.field("dungeon_village_reference::ActorEffectState.delayed", v.delayed);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::ActorEffectState>().display)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ActorEffectState>().delayed)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::AiRewardState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::AiRewardState.battle", v.battle);
  a.field("dungeon_village_reference::AiRewardState.retired_actors", v.retired_actors);
  a.field("dungeon_village_reference::AiRewardState.human_order", v.human_order);
  a.field("dungeon_village_reference::AiRewardState.monster_order", v.monster_order);
  a.field("dungeon_village_reference::AiRewardState.contexts", v.contexts);
  a.field("dungeon_village_reference::AiRewardState.growth", v.growth);
  a.field("dungeon_village_reference::AiRewardState.professions", v.professions);
  a.field("dungeon_village_reference::AiRewardState.monster_growth", v.monster_growth);
  a.field("dungeon_village_reference::AiRewardState.monster_definition_order", v.monster_definition_order);
  a.field("dungeon_village_reference::AiRewardState.monster_progress", v.monster_progress);
  a.field("dungeon_village_reference::AiRewardState.monster_limit", v.monster_limit);
  a.field("dungeon_village_reference::AiRewardState.encounters", v.encounters);
  a.field("dungeon_village_reference::AiRewardState.encounter_order", v.encounter_order);
  a.field("dungeon_village_reference::AiRewardState.retired_encounters", v.retired_encounters);
  a.field("dungeon_village_reference::AiRewardState.external_actor_roots", v.external_actor_roots);
  a.field("dungeon_village_reference::AiRewardState.facility_actor_roots", v.facility_actor_roots);
  a.field("dungeon_village_reference::AiRewardState.external_encounter_roots", v.external_encounter_roots);
  a.field("dungeon_village_reference::AiRewardState.projectiles", v.projectiles);
  a.field("dungeon_village_reference::AiRewardState.projectile_order", v.projectile_order);
  a.field("dungeon_village_reference::AiRewardState.next_projectile_id", v.next_projectile_id);
  a.field("dungeon_village_reference::AiRewardState.accounting", v.accounting);
  a.field("dungeon_village_reference::AiRewardState.next_cash_id", v.next_cash_id);
  a.field("dungeon_village_reference::AiRewardState.next_actor_id", v.next_actor_id);
  a.field("dungeon_village_reference::AiRewardState.next_encounter_id", v.next_encounter_id);
  a.field("dungeon_village_reference::AiRewardState.legacy_encounter_counter", v.legacy_encounter_counter);
  a.field("dungeon_village_reference::AiRewardState.period", v.period);
  a.field("dungeon_village_reference::AiRewardState.pending_completion", v.pending_completion);
  a.field("dungeon_village_reference::AiRewardState.task_active", v.task_active);
  a.field("dungeon_village_reference::AiRewardState.task_completed", v.task_completed);
  a.field("dungeon_village_reference::AiRewardState.feature16", v.feature16);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().battle)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().retired_actors)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().human_order)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().monster_order)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().contexts)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().growth)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().professions)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().monster_growth)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().monster_definition_order)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().monster_progress)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().monster_limit)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().encounters)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().encounter_order)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().retired_encounters)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().external_actor_roots)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().facility_actor_roots)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().external_encounter_roots)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().projectiles)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().projectile_order)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().next_projectile_id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().accounting)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().next_cash_id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().next_actor_id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().next_encounter_id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().legacy_encounter_counter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().period)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().pending_completion)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().task_active)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().task_completed)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::AiRewardState>().feature16)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::ArrivalBinding> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::ArrivalBinding.goal", v.goal);
  a.field("dungeon_village_reference::ArrivalBinding.instance_id", v.instance_id);
  a.field("dungeon_village_reference::ArrivalBinding.definition_id", v.definition_id);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::ArrivalBinding>().goal)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ArrivalBinding>().instance_id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ArrivalBinding>().definition_id)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::BattleActorRecord> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::BattleActorRecord.id", v.id);
  a.field("dungeon_village_reference::BattleActorRecord.kind", v.kind);
  a.field("dungeon_village_reference::BattleActorRecord.definition", v.definition);
  a.field("dungeon_village_reference::BattleActorRecord.control", v.control);
  a.field("dungeon_village_reference::BattleActorRecord.hp", v.hp);
  a.field("dungeon_village_reference::BattleActorRecord.capacity", v.capacity);
  a.field("dungeon_village_reference::BattleActorRecord.baseline", v.baseline);
  a.field("dungeon_village_reference::BattleActorRecord.state_counter", v.state_counter);
  a.field("dungeon_village_reference::BattleActorRecord.state_parameter", v.state_parameter);
  a.field("dungeon_village_reference::BattleActorRecord.attack_count", v.attack_count);
  a.field("dungeon_village_reference::BattleActorRecord.down_timer", v.down_timer);
  a.field("dungeon_village_reference::BattleActorRecord.object_slot", v.object_slot);
  a.field("dungeon_village_reference::BattleActorRecord.rescue", v.rescue);
  a.field("dungeon_village_reference::BattleActorRecord.encounter", v.encounter);
  a.field("dungeon_village_reference::BattleActorRecord.group", v.group);
  a.field("dungeon_village_reference::BattleActorRecord.attack_slot", v.attack_slot);
  a.field("dungeon_village_reference::BattleActorRecord.monster_posture", v.monster_posture);
  a.field("dungeon_village_reference::BattleActorRecord.legacy_id", v.legacy_id);
  a.field("dungeon_village_reference::BattleActorRecord.body", v.body);
  a.field("dungeon_village_reference::BattleActorRecord.sprite", v.sprite);
  a.field("dungeon_village_reference::BattleActorRecord.position", v.position);
  a.field("dungeon_village_reference::BattleActorRecord.attack_position", v.attack_position);
  a.field("dungeon_village_reference::BattleActorRecord.miss", v.miss);
  a.field("dungeon_village_reference::BattleActorRecord.damage_total", v.damage_total);
  a.field("dungeon_village_reference::BattleActorRecord.hit_count", v.hit_count);
  a.field("dungeon_village_reference::BattleActorRecord.hit_flash", v.hit_flash);
  a.field("dungeon_village_reference::BattleActorRecord.label_timer", v.label_timer);
  a.field("dungeon_village_reference::BattleActorRecord.miss_label", v.miss_label);
  a.field("dungeon_village_reference::BattleActorRecord.follow", v.follow);
  a.field("dungeon_village_reference::BattleActorRecord.attack_armed", v.attack_armed);
  a.field("dungeon_village_reference::BattleActorRecord.combo_index", v.combo_index);
  a.field("dungeon_village_reference::BattleActorRecord.combo_count", v.combo_count);
  a.field("dungeon_village_reference::BattleActorRecord.attack_idle", v.attack_idle);
  a.field("dungeon_village_reference::BattleActorRecord.attack_destination", v.attack_destination);
  a.field("dungeon_village_reference::BattleActorRecord.perceived_enemy", v.perceived_enemy);
  a.field("dungeon_village_reference::BattleActorRecord.perceived_distance", v.perceived_distance);
  a.field("dungeon_village_reference::BattleActorRecord.blocked_battle_steps", v.blocked_battle_steps);
  a.field("dungeon_village_reference::BattleActorRecord.attack_cooldown", v.attack_cooldown);
  a.field("dungeon_village_reference::BattleActorRecord.decision_start", v.decision_start);
  a.field("dungeon_village_reference::BattleActorRecord.physics_pause", v.physics_pause);
  a.field("dungeon_village_reference::BattleActorRecord.vertical_velocity", v.vertical_velocity);
  a.field("dungeon_village_reference::BattleActorRecord.area_after", v.area_after);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().kind)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().definition)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().control)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().hp)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().capacity)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().baseline)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().state_counter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().state_parameter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().attack_count)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().down_timer)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().object_slot)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().rescue)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().encounter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().group)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().attack_slot)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().monster_posture)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().legacy_id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().body)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().sprite)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().position)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().attack_position)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().miss)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().damage_total)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().hit_count)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().hit_flash)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().label_timer)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().miss_label)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().follow)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().attack_armed)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().combo_index)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().combo_count)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().attack_idle)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().attack_destination)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().perceived_enemy)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().perceived_distance)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().blocked_battle_steps)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().attack_cooldown)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().decision_start)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().physics_pause)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().vertical_velocity)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleActorRecord>().area_after)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::BattleCommitState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::BattleCommitState.actors", v.actors);
  a.field("dungeon_village_reference::BattleCommitState.humans", v.humans);
  a.field("dungeon_village_reference::BattleCommitState.monsters", v.monsters);
  a.field("dungeon_village_reference::BattleCommitState.participants", v.participants);
  a.field("dungeon_village_reference::BattleCommitState.quest_encounters", v.quest_encounters);
  a.field("dungeon_village_reference::BattleCommitState.events", v.events);
  a.field("dungeon_village_reference::BattleCommitState.global_downs", v.global_downs);
  a.field("dungeon_village_reference::BattleCommitState.defeated_definitions", v.defeated_definitions);
  a.field("dungeon_village_reference::BattleCommitState.drop_progress", v.drop_progress);
  a.field("dungeon_village_reference::BattleCommitState.objects", v.objects);
  a.field("dungeon_village_reference::BattleCommitState.next_object_id", v.next_object_id);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::BattleCommitState>().actors)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleCommitState>().humans)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleCommitState>().monsters)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleCommitState>().participants)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleCommitState>().quest_encounters)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleCommitState>().events)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleCommitState>().global_downs)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleCommitState>().defeated_definitions)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleCommitState>().drop_progress)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleCommitState>().objects)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleCommitState>().next_object_id)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::BattleGroupMember> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::BattleGroupMember.id", v.id);
  a.field("dungeon_village_reference::BattleGroupMember.flags", v.flags);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::BattleGroupMember>().id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleGroupMember>().flags)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::BattleGroupState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::BattleGroupState.tick", v.tick);
  a.field("dungeon_village_reference::BattleGroupState.duration", v.duration);
  a.field("dungeon_village_reference::BattleGroupState.cycle", v.cycle);
  a.field("dungeon_village_reference::BattleGroupState.alternating_side", v.alternating_side);
  a.field("dungeon_village_reference::BattleGroupState.humans", v.humans);
  a.field("dungeon_village_reference::BattleGroupState.monsters", v.monsters);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::BattleGroupState>().tick)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleGroupState>().duration)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleGroupState>().cycle)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleGroupState>().alternating_side)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleGroupState>().humans)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::BattleGroupState>().monsters)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::BuildingId> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::BuildingId.value", v.value);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::BuildingId>().value)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::CalendarMaintenanceShopItem> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::CalendarMaintenanceShopItem.definition", v.definition);
  a.field("dungeon_village_reference::CalendarMaintenanceShopItem.minimum_rank", v.minimum_rank);
  a.field("dungeon_village_reference::CalendarMaintenanceShopItem.maximum_quantity", v.maximum_quantity);
  a.field("dungeon_village_reference::CalendarMaintenanceShopItem.quantity", v.quantity);
  a.field("dungeon_village_reference::CalendarMaintenanceShopItem.presence", v.presence);
  a.field("dungeon_village_reference::CalendarMaintenanceShopItem.legacy_q", v.legacy_q);
  a.field("dungeon_village_reference::CalendarMaintenanceShopItem.newly_available", v.newly_available);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::CalendarMaintenanceShopItem>().definition)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CalendarMaintenanceShopItem>().minimum_rank)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CalendarMaintenanceShopItem>().maximum_quantity)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CalendarMaintenanceShopItem>().quantity)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CalendarMaintenanceShopItem>().presence)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CalendarMaintenanceShopItem>().legacy_q)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CalendarMaintenanceShopItem>().newly_available)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::CandidateDefinition> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::CandidateDefinition.definition_id", v.definition_id);
  a.field("dungeon_village_reference::CandidateDefinition.legacy_category", v.legacy_category);
  a.field("dungeon_village_reference::CandidateDefinition.definition_charm", v.definition_charm);
  a.field("dungeon_village_reference::CandidateDefinition.legacy_kind", v.legacy_kind);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::CandidateDefinition>().definition_id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CandidateDefinition>().legacy_category)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CandidateDefinition>().definition_charm)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CandidateDefinition>().legacy_kind)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::CandidateInstance> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::CandidateInstance.instance_id", v.instance_id);
  a.field("dungeon_village_reference::CandidateInstance.definition_id", v.definition_id);
  a.field("dungeon_village_reference::CandidateInstance.legacy_phase", v.legacy_phase);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::CandidateInstance>().instance_id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CandidateInstance>().definition_id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CandidateInstance>().legacy_phase)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::CashEntry> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::CashEntry.event_id", v.event_id);
  a.field("dungeon_village_reference::CashEntry.period", v.period);
  a.field("dungeon_village_reference::CashEntry.category", v.category);
  a.field("dungeon_village_reference::CashEntry.direction", v.direction);
  a.field("dungeon_village_reference::CashEntry.amount", v.amount);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::CashEntry>().event_id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CashEntry>().period)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CashEntry>().category)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CashEntry>().direction)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CashEntry>().amount)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::CashTotals> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::CashTotals.income", v.income);
  a.field("dungeon_village_reference::CashTotals.expense", v.expense);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::CashTotals>().income)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CashTotals>().expense)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::CharacterHpState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::CharacterHpState.requested_delta", v.requested_delta);
  a.field("dungeon_village_reference::CharacterHpState.displayed", v.displayed);
  a.field("dungeon_village_reference::CharacterHpState.origin", v.origin);
  a.field("dungeon_village_reference::CharacterHpState.target", v.target);
  a.field("dungeon_village_reference::CharacterHpState.animating", v.animating);
  a.field("dungeon_village_reference::CharacterHpState.legacy_tick", v.legacy_tick);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::CharacterHpState>().requested_delta)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CharacterHpState>().displayed)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CharacterHpState>().origin)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CharacterHpState>().target)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CharacterHpState>().animating)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CharacterHpState>().legacy_tick)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::CharacterId> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::CharacterId.value", v.value);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::CharacterId>().value)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::CombatInfluenceCandidate> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::CombatInfluenceCandidate.width", v.width);
  a.field("dungeon_village_reference::CombatInfluenceCandidate.height", v.height);
  a.field("dungeon_village_reference::CombatInfluenceCandidate.human_field", v.human_field);
  a.field("dungeon_village_reference::CombatInfluenceCandidate.monster_field", v.monster_field);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::CombatInfluenceCandidate>().width)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CombatInfluenceCandidate>().height)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CombatInfluenceCandidate>().human_field)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CombatInfluenceCandidate>().monster_field)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::CombatPoint> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::CombatPoint.x", v.x);
  a.field("dungeon_village_reference::CombatPoint.height", v.height);
  a.field("dungeon_village_reference::CombatPoint.z", v.z);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::CombatPoint>().x)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CombatPoint>().height)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::CombatPoint>().z)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::DefeatTally> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::DefeatTally.definition_id", v.definition_id);
  a.field("dungeon_village_reference::DefeatTally.defeats", v.defeats);
  a.field("dungeon_village_reference::DefeatTally.points_per_defeat", v.points_per_defeat);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::DefeatTally>().definition_id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DefeatTally>().defeats)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DefeatTally>().points_per_defeat)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::DelayedRewardState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::DelayedRewardState.amount", v.amount);
  a.field("dungeon_village_reference::DelayedRewardState.counter", v.counter);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::DelayedRewardState>().amount)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DelayedRewardState>().counter)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::DungeonActorProgress> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::DungeonActorProgress.progress", v.progress);
  a.field("dungeon_village_reference::DungeonActorProgress.previous", v.previous);
  a.field("dungeon_village_reference::DungeonActorProgress.percent", v.percent);
  a.field("dungeon_village_reference::DungeonActorProgress.endurance", v.endurance);
  a.field("dungeon_village_reference::DungeonActorProgress.retreat_state", v.retreat_state);
  a.field("dungeon_village_reference::DungeonActorProgress.retreat_updates", v.retreat_updates);
  a.field("dungeon_village_reference::DungeonActorProgress.constrained", v.constrained);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonActorProgress>().progress)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonActorProgress>().previous)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonActorProgress>().percent)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonActorProgress>().endurance)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonActorProgress>().retreat_state)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonActorProgress>().retreat_updates)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonActorProgress>().constrained)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::DungeonCrewRequest> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::DungeonCrewRequest.kind", v.kind);
  a.field("dungeon_village_reference::DungeonCrewRequest.actor", v.actor);
  a.field("dungeon_village_reference::DungeonCrewRequest.first", v.first);
  a.field("dungeon_village_reference::DungeonCrewRequest.second", v.second);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonCrewRequest>().kind)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonCrewRequest>().actor)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonCrewRequest>().first)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonCrewRequest>().second)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::DungeonFacilityProgress> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::DungeonFacilityProgress.updates", v.updates);
  a.field("dungeon_village_reference::DungeonFacilityProgress.progress", v.progress);
  a.field("dungeon_village_reference::DungeonFacilityProgress.extent", v.extent);
  a.field("dungeon_village_reference::DungeonFacilityProgress.percent", v.percent);
  a.field("dungeon_village_reference::DungeonFacilityProgress.previous_percent", v.previous_percent);
  a.field("dungeon_village_reference::DungeonFacilityProgress.challenges", v.challenges);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonFacilityProgress>().updates)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonFacilityProgress>().progress)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonFacilityProgress>().extent)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonFacilityProgress>().percent)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonFacilityProgress>().previous_percent)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonFacilityProgress>().challenges)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::DungeonFinishSite> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::DungeonFinishSite.occupied_cells", v.occupied_cells);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonFinishSite>().occupied_cells)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::DungeonFinishSurface> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::DungeonFinishSurface.definition", v.definition);
  a.field("dungeon_village_reference::DungeonFinishSurface.updates", v.updates);
  a.field("dungeon_village_reference::DungeonFinishSurface.instance", v.instance);
  a.field("dungeon_village_reference::DungeonFinishSurface.fragment", v.fragment);
  a.field("dungeon_village_reference::DungeonFinishSurface.display_definition", v.display_definition);
  a.field("dungeon_village_reference::DungeonFinishSurface.variant", v.variant);
  a.field("dungeon_village_reference::DungeonFinishSurface.road_mask", v.road_mask);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonFinishSurface>().definition)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonFinishSurface>().updates)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonFinishSurface>().instance)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonFinishSurface>().fragment)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonFinishSurface>().display_definition)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonFinishSurface>().variant)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonFinishSurface>().road_mask)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::DungeonFinishTask> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::DungeonFinishTask.identity", v.identity);
  a.field("dungeon_village_reference::DungeonFinishTask.definition", v.definition);
  a.field("dungeon_village_reference::DungeonFinishTask.difficulty", v.difficulty);
  a.field("dungeon_village_reference::DungeonFinishTask.pending_completion_value", v.pending_completion_value);
  a.field("dungeon_village_reference::DungeonFinishTask.facility", v.facility);
  a.field("dungeon_village_reference::DungeonFinishTask.site", v.site);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonFinishTask>().identity)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonFinishTask>().definition)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonFinishTask>().difficulty)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonFinishTask>().pending_completion_value)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonFinishTask>().facility)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonFinishTask>().site)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::DungeonMonsterAvailability> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::DungeonMonsterAvailability.status", v.status);
  a.field("dungeon_village_reference::DungeonMonsterAvailability.pending_notice", v.pending_notice);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonMonsterAvailability>().status)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonMonsterAvailability>().pending_notice)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::DungeonTaskDefinitionProgress> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::DungeonTaskDefinitionProgress.kind", v.kind);
  a.field("dungeon_village_reference::DungeonTaskDefinitionProgress.flags", v.flags);
  a.field("dungeon_village_reference::DungeonTaskDefinitionProgress.completed", v.completed);
  a.field("dungeon_village_reference::DungeonTaskDefinitionProgress.monster_definition", v.monster_definition);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonTaskDefinitionProgress>().kind)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonTaskDefinitionProgress>().flags)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonTaskDefinitionProgress>().completed)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonTaskDefinitionProgress>().monster_definition)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::DungeonTaskSuccessState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::DungeonTaskSuccessState.successes", v.successes);
  a.field("dungeon_village_reference::DungeonTaskSuccessState.ordinary_explorations", v.ordinary_explorations);
  a.field("dungeon_village_reference::DungeonTaskSuccessState.exploration_stage", v.exploration_stage);
  a.field("dungeon_village_reference::DungeonTaskSuccessState.exploration_dates", v.exploration_dates);
  a.field("dungeon_village_reference::DungeonTaskSuccessState.task_pool_progress", v.task_pool_progress);
  a.field("dungeon_village_reference::DungeonTaskSuccessState.definitions", v.definitions);
  a.field("dungeon_village_reference::DungeonTaskSuccessState.monsters", v.monsters);
  a.field("dungeon_village_reference::DungeonTaskSuccessState.remaining_task_definitions", v.remaining_task_definitions);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonTaskSuccessState>().successes)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonTaskSuccessState>().ordinary_explorations)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonTaskSuccessState>().exploration_stage)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonTaskSuccessState>().exploration_dates)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonTaskSuccessState>().task_pool_progress)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonTaskSuccessState>().definitions)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonTaskSuccessState>().monsters)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::DungeonTaskSuccessState>().remaining_task_definitions)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::EncounterRuntimeState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::EncounterRuntimeState.id", v.id);
  a.field("dungeon_village_reference::EncounterRuntimeState.center", v.center);
  a.field("dungeon_village_reference::EncounterRuntimeState.state", v.state);
  a.field("dungeon_village_reference::EncounterRuntimeState.counter", v.counter);
  a.field("dungeon_village_reference::EncounterRuntimeState.idle", v.idle);
  a.field("dungeon_village_reference::EncounterRuntimeState.spawned", v.spawned);
  a.field("dungeon_village_reference::EncounterRuntimeState.quota", v.quota);
  a.field("dungeon_village_reference::EncounterRuntimeState.reward", v.reward);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::EncounterRuntimeState>().id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::EncounterRuntimeState>().center)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::EncounterRuntimeState>().state)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::EncounterRuntimeState>().counter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::EncounterRuntimeState>().idle)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::EncounterRuntimeState>().spawned)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::EncounterRuntimeState>().quota)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::EncounterRuntimeState>().reward)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::FacilityArrivalState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::FacilityArrivalState.legacy_visit_counts", v.legacy_visit_counts);
  a.field("dungeon_village_reference::FacilityArrivalState.legacy_category_one_count", v.legacy_category_one_count);
  a.field("dungeon_village_reference::FacilityArrivalState.legacy_category_six_counter", v.legacy_category_six_counter);
  a.field("dungeon_village_reference::FacilityArrivalState.last_visited_instance", v.last_visited_instance);
  a.field("dungeon_village_reference::FacilityArrivalState.legacy_actor_total", v.legacy_actor_total);
  a.field("dungeon_village_reference::FacilityArrivalState.current_month_facility_sales", v.current_month_facility_sales);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityArrivalState>().legacy_visit_counts)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityArrivalState>().legacy_category_one_count)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityArrivalState>().legacy_category_six_counter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityArrivalState>().last_visited_instance)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityArrivalState>().legacy_actor_total)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityArrivalState>().current_month_facility_sales)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::FacilityDeparture> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::FacilityDeparture.category", v.category);
  a.field("dungeon_village_reference::FacilityDeparture.selection", v.selection);
  a.field("dungeon_village_reference::FacilityDeparture.binding", v.binding);
  a.field("dungeon_village_reference::FacilityDeparture.route", v.route);
  a.field("dungeon_village_reference::FacilityDeparture.legacy_direction", v.legacy_direction);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityDeparture>().category)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityDeparture>().selection)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityDeparture>().binding)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityDeparture>().route)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityDeparture>().legacy_direction)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::FacilityEconomyDefinition> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::FacilityEconomyDefinition.attributes", v.attributes);
  a.field("dungeon_village_reference::FacilityEconomyDefinition.upgrade_uses", v.upgrade_uses);
  a.field("dungeon_village_reference::FacilityEconomyDefinition.construction_cost", v.construction_cost);
  a.field("dungeon_village_reference::FacilityEconomyDefinition.construction_ticks", v.construction_ticks);
  a.field("dungeon_village_reference::FacilityEconomyDefinition.legacy_flags", v.legacy_flags);
  a.field("dungeon_village_reference::FacilityEconomyDefinition.decoration", v.decoration);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityEconomyDefinition>().attributes)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityEconomyDefinition>().upgrade_uses)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityEconomyDefinition>().construction_cost)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityEconomyDefinition>().construction_ticks)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityEconomyDefinition>().legacy_flags)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityEconomyDefinition>().decoration)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::FacilityPlacement> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::FacilityPlacement.instance_id", v.instance_id);
  a.field("dungeon_village_reference::FacilityPlacement.definition_id", v.definition_id);
  a.field("dungeon_village_reference::FacilityPlacement.shape", v.shape);
  a.field("dungeon_village_reference::FacilityPlacement.orientation", v.orientation);
  a.field("dungeon_village_reference::FacilityPlacement.anchor", v.anchor);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityPlacement>().instance_id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityPlacement>().definition_id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityPlacement>().shape)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityPlacement>().orientation)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityPlacement>().anchor)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::FacilityTileBinding> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::FacilityTileBinding.instance_id", v.instance_id);
  a.field("dungeon_village_reference::FacilityTileBinding.definition_id", v.definition_id);
  a.field("dungeon_village_reference::FacilityTileBinding.fragment_index", v.fragment_index);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityTileBinding>().instance_id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityTileBinding>().definition_id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityTileBinding>().fragment_index)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::FacilityUseProgress> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::FacilityUseProgress.level", v.level);
  a.field("dungeon_village_reference::FacilityUseProgress.completed_uses", v.completed_uses);
  a.field("dungeon_village_reference::FacilityUseProgress.upgrade_pending", v.upgrade_pending);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityUseProgress>().level)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityUseProgress>().completed_uses)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::FacilityUseProgress>().upgrade_pending)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::GroundObjectState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::GroundObjectState.id", v.id);
  a.field("dungeon_village_reference::GroundObjectState.state", v.state);
  a.field("dungeon_village_reference::GroundObjectState.counter", v.counter);
  a.field("dungeon_village_reference::GroundObjectState.duration", v.duration);
  a.field("dungeon_village_reference::GroundObjectState.delay", v.delay);
  a.field("dungeon_village_reference::GroundObjectState.kind", v.kind);
  a.field("dungeon_village_reference::GroundObjectState.definition", v.definition);
  a.field("dungeon_village_reference::GroundObjectState.position", v.position);
  a.field("dungeon_village_reference::GroundObjectState.velocity", v.velocity);
  a.field("dungeon_village_reference::GroundObjectState.acceleration", v.acceleration);
  a.field("dungeon_village_reference::GroundObjectState.cached_cell", v.cached_cell);
  a.field("dungeon_village_reference::GroundObjectState.inside_town", v.inside_town);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::GroundObjectState>().id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::GroundObjectState>().state)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::GroundObjectState>().counter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::GroundObjectState>().duration)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::GroundObjectState>().delay)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::GroundObjectState>().kind)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::GroundObjectState>().definition)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::GroundObjectState>().position)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::GroundObjectState>().velocity)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::GroundObjectState>().acceleration)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::GroundObjectState>().cached_cell)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::GroundObjectState>().inside_town)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::HumanBattleRecord> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::HumanBattleRecord.kills", v.kills);
  a.field("dungeon_village_reference::HumanBattleRecord.killed_stat1", v.killed_stat1);
  a.field("dungeon_village_reference::HumanBattleRecord.battle_reward_stat", v.battle_reward_stat);
  a.field("dungeon_village_reference::HumanBattleRecord.task_kills", v.task_kills);
  a.field("dungeon_village_reference::HumanBattleRecord.participant_downs", v.participant_downs);
  a.field("dungeon_village_reference::HumanBattleRecord.recent_reward", v.recent_reward);
  a.field("dungeon_village_reference::HumanBattleRecord.recent_kills", v.recent_kills);
  a.field("dungeon_village_reference::HumanBattleRecord.luck", v.luck);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::HumanBattleRecord>().kills)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::HumanBattleRecord>().killed_stat1)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::HumanBattleRecord>().battle_reward_stat)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::HumanBattleRecord>().task_kills)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::HumanBattleRecord>().participant_downs)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::HumanBattleRecord>().recent_reward)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::HumanBattleRecord>().recent_kills)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::HumanBattleRecord>().luck)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::HumanDefinitionStatsInput> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::HumanDefinitionStatsInput.current_profession", v.current_profession);
  a.field("dungeon_village_reference::HumanDefinitionStatsInput.legacy_u", v.legacy_u);
  a.field("dungeon_village_reference::HumanDefinitionStatsInput.base", v.base);
  a.field("dungeon_village_reference::HumanDefinitionStatsInput.extra", v.extra);
  a.field("dungeon_village_reference::HumanDefinitionStatsInput.profession_levels", v.profession_levels);
  a.field("dungeon_village_reference::HumanDefinitionStatsInput.equipment", v.equipment);
  a.field("dungeon_village_reference::HumanDefinitionStatsInput.learned_spells", v.learned_spells);
  a.field("dungeon_village_reference::HumanDefinitionStatsInput.spell_professions", v.spell_professions);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::HumanDefinitionStatsInput>().current_profession)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::HumanDefinitionStatsInput>().legacy_u)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::HumanDefinitionStatsInput>().base)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::HumanDefinitionStatsInput>().extra)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::HumanDefinitionStatsInput>().profession_levels)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::HumanDefinitionStatsInput>().equipment)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::HumanDefinitionStatsInput>().learned_spells)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::HumanDefinitionStatsInput>().spell_professions)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::HumanDerivedStats> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::HumanDerivedStats.growth", v.growth);
  a.field("dungeon_village_reference::HumanDerivedStats.attributes", v.attributes);
  a.field("dungeon_village_reference::HumanDerivedStats.combat", v.combat);
  a.field("dungeon_village_reference::HumanDerivedStats.available_spells", v.available_spells);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::HumanDerivedStats>().growth)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::HumanDerivedStats>().attributes)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::HumanDerivedStats>().combat)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::HumanDerivedStats>().available_spells)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::HumanProfessionRule> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::HumanProfessionRule.maximum_growth", v.maximum_growth);
  a.field("dungeon_village_reference::HumanProfessionRule.attribute_percent", v.attribute_percent);
  a.field("dungeon_village_reference::HumanProfessionRule.difficulty", v.difficulty);
  a.field("dungeon_village_reference::HumanProfessionRule.unlocked", v.unlocked);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::HumanProfessionRule>().maximum_growth)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::HumanProfessionRule>().attribute_percent)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::HumanProfessionRule>().difficulty)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::HumanProfessionRule>().unlocked)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::LegacyMap> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::LegacyMap.width", v.width);
  a.field("dungeon_village_reference::LegacyMap.height", v.height);
  a.field("dungeon_village_reference::LegacyMap.cells", v.cells);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::LegacyMap>().width)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::LegacyMap>().height)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::LegacyMap>().cells)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::LegacyMapCell> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::LegacyMapCell.legacy_state", v.legacy_state);
  a.field("dungeon_village_reference::LegacyMapCell.category", v.category);
  a.field("dungeon_village_reference::LegacyMapCell.facility", v.facility);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::LegacyMapCell>().legacy_state)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::LegacyMapCell>().category)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::LegacyMapCell>().facility)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::LegacyPathResult> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::LegacyPathResult.error", v.error);
  a.field("dungeon_village_reference::LegacyPathResult.steps", v.steps);
  a.field("dungeon_village_reference::LegacyPathResult.cost", v.cost);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::LegacyPathResult>().error)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::LegacyPathResult>().steps)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::LegacyPathResult>().cost)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::LevelEndpoints> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::LevelEndpoints.first", v.first);
  a.field("dungeon_village_reference::LevelEndpoints.fifth", v.fifth);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::LevelEndpoints>().first)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::LevelEndpoints>().fifth)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::MonsterBattleRecord> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::MonsterBattleRecord.human_kills", v.human_kills);
  a.field("dungeon_village_reference::MonsterBattleRecord.stat1", v.stat1);
  a.field("dungeon_village_reference::MonsterBattleRecord.statF", v.statF);
  a.field("dungeon_village_reference::MonsterBattleRecord.death_reward", v.death_reward);
  a.field("dungeon_village_reference::MonsterBattleRecord.rank", v.rank);
  a.field("dungeon_village_reference::MonsterBattleRecord.flags", v.flags);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::MonsterBattleRecord>().human_kills)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::MonsterBattleRecord>().stat1)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::MonsterBattleRecord>().statF)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::MonsterBattleRecord>().death_reward)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::MonsterBattleRecord>().rank)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::MonsterBattleRecord>().flags)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::NeighbourSource> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::NeighbourSource.instance_id", v.instance_id);
  a.field("dungeon_village_reference::NeighbourSource.definition_id", v.definition_id);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::NeighbourSource>().instance_id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::NeighbourSource>().definition_id)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::ObjectCatalogRecord> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::ObjectCatalogRecord.flags", v.flags);
  a.field("dungeon_village_reference::ObjectCatalogRecord.status", v.status);
  a.field("dungeon_village_reference::ObjectCatalogRecord.unlock_counter", v.unlock_counter);
  a.field("dungeon_village_reference::ObjectCatalogRecord.newly_unlocked", v.newly_unlocked);
  a.field("dungeon_village_reference::ObjectCatalogRecord.inventory", v.inventory);
  a.field("dungeon_village_reference::ObjectCatalogRecord.free_purchases", v.free_purchases);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::ObjectCatalogRecord>().flags)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ObjectCatalogRecord>().status)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ObjectCatalogRecord>().unlock_counter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ObjectCatalogRecord>().newly_unlocked)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ObjectCatalogRecord>().inventory)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ObjectCatalogRecord>().free_purchases)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::ObjectId> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::ObjectId.value", v.value);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::ObjectId>().value)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::ObjectShopRecord> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::ObjectShopRecord.category", v.category);
  a.field("dungeon_village_reference::ObjectShopRecord.notices", v.notices);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::ObjectShopRecord>().category)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ObjectShopRecord>().notices)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::PeriodAccountingSnapshot> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::PeriodAccountingSnapshot.funds", v.funds);
  a.field("dungeon_village_reference::PeriodAccountingSnapshot.village_points", v.village_points);
  a.field("dungeon_village_reference::PeriodAccountingSnapshot.entries", v.entries);
  a.field("dungeon_village_reference::PeriodAccountingSnapshot.reports", v.reports);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::PeriodAccountingSnapshot>().funds)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::PeriodAccountingSnapshot>().village_points)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::PeriodAccountingSnapshot>().entries)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::PeriodAccountingSnapshot>().reports)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::Position> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::Position.x", v.x);
  a.field("dungeon_village_reference::Position.y", v.y);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::Position>().x)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::Position>().y)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::ProjectileState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::ProjectileState.kind", v.kind);
  a.field("dungeon_village_reference::ProjectileState.caster", v.caster);
  a.field("dungeon_village_reference::ProjectileState.original_target", v.original_target);
  a.field("dungeon_village_reference::ProjectileState.position", v.position);
  a.field("dungeon_village_reference::ProjectileState.velocity", v.velocity);
  a.field("dungeon_village_reference::ProjectileState.acceleration", v.acceleration);
  a.field("dungeon_village_reference::ProjectileState.facing", v.facing);
  a.field("dungeon_village_reference::ProjectileState.counter", v.counter);
  a.field("dungeon_village_reference::ProjectileState.delay", v.delay);
  a.field("dungeon_village_reference::ProjectileState.effect", v.effect);
  a.field("dungeon_village_reference::ProjectileState.damage", v.damage);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::ProjectileState>().kind)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ProjectileState>().caster)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ProjectileState>().original_target)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ProjectileState>().position)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ProjectileState>().velocity)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ProjectileState>().acceleration)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ProjectileState>().facing)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ProjectileState>().counter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ProjectileState>().delay)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ProjectileState>().effect)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ProjectileState>().damage)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::ReportInput> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::ReportInput.period", v.period);
  a.field("dungeon_village_reference::ReportInput.charges", v.charges);
  a.field("dungeon_village_reference::ReportInput.defeats", v.defeats);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::ReportInput>().period)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ReportInput>().charges)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ReportInput>().defeats)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::RescueActorContext> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::RescueActorContext.visits", v.visits);
  a.field("dungeon_village_reference::RescueActorContext.binding", v.binding);
  a.field("dungeon_village_reference::RescueActorContext.path_pending", v.path_pending);
  a.field("dungeon_village_reference::RescueActorContext.on_event_cell", v.on_event_cell);
  a.field("dungeon_village_reference::RescueActorContext.definition_task_flag", v.definition_task_flag);
  a.field("dungeon_village_reference::RescueActorContext.journey", v.journey);
  a.field("dungeon_village_reference::RescueActorContext.waypoint", v.waypoint);
  a.field("dungeon_village_reference::RescueActorContext.horizontal_velocity", v.horizontal_velocity);
  a.field("dungeon_village_reference::RescueActorContext.town_updates", v.town_updates);
  a.field("dungeon_village_reference::RescueActorContext.outside_updates", v.outside_updates);
  a.field("dungeon_village_reference::RescueActorContext.blocked_updates", v.blocked_updates);
  a.field("dungeon_village_reference::RescueActorContext.spawn_updates", v.spawn_updates);
  a.field("dungeon_village_reference::RescueActorContext.no_path_updates", v.no_path_updates);
  a.field("dungeon_village_reference::RescueActorContext.short_exit_updates", v.short_exit_updates);
  a.field("dungeon_village_reference::RescueActorContext.bad_area_updates", v.bad_area_updates);
  a.field("dungeon_village_reference::RescueActorContext.monster_mode", v.monster_mode);
  a.field("dungeon_village_reference::RescueActorContext.destination", v.destination);
  a.field("dungeon_village_reference::RescueActorContext.unbound_route", v.unbound_route);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::RescueActorContext>().visits)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueActorContext>().binding)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueActorContext>().path_pending)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueActorContext>().on_event_cell)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueActorContext>().definition_task_flag)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueActorContext>().journey)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueActorContext>().waypoint)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueActorContext>().horizontal_velocity)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueActorContext>().town_updates)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueActorContext>().outside_updates)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueActorContext>().blocked_updates)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueActorContext>().spawn_updates)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueActorContext>().no_path_updates)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueActorContext>().short_exit_updates)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueActorContext>().bad_area_updates)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueActorContext>().monster_mode)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueActorContext>().destination)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueActorContext>().unbound_route)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::RescueFacility> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::RescueFacility.placement", v.placement);
  a.field("dungeon_village_reference::RescueFacility.kind", v.kind);
  a.field("dungeon_village_reference::RescueFacility.category", v.category);
  a.field("dungeon_village_reference::RescueFacility.detail", v.detail);
  a.field("dungeon_village_reference::RescueFacility.price", v.price);
  a.field("dungeon_village_reference::RescueFacility.status", v.status);
  a.field("dungeon_village_reference::RescueFacility.upgrade_uses", v.upgrade_uses);
  a.field("dungeon_village_reference::RescueFacility.sales", v.sales);
  a.field("dungeon_village_reference::RescueFacility.occupants", v.occupants);
  a.field("dungeon_village_reference::RescueFacility.definition_wait", v.definition_wait);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::RescueFacility>().placement)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueFacility>().kind)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueFacility>().category)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueFacility>().detail)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueFacility>().price)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueFacility>().status)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueFacility>().upgrade_uses)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueFacility>().sales)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueFacility>().occupants)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueFacility>().definition_wait)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::RescueWorldState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::RescueWorldState.ai", v.ai);
  a.field("dungeon_village_reference::RescueWorldState.map", v.map);
  a.field("dungeon_village_reference::RescueWorldState.actors", v.actors);
  a.field("dungeon_village_reference::RescueWorldState.facilities", v.facilities);
  a.field("dungeon_village_reference::RescueWorldState.human_spending", v.human_spending);
  a.field("dungeon_village_reference::RescueWorldState.facility_uses", v.facility_uses);
  a.field("dungeon_village_reference::RescueWorldState.month_index", v.month_index);
  a.field("dungeon_village_reference::RescueWorldState.object_order", v.object_order);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::RescueWorldState>().ai)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueWorldState>().map)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueWorldState>().actors)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueWorldState>().facilities)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueWorldState>().human_spending)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueWorldState>().facility_uses)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueWorldState>().month_index)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RescueWorldState>().object_order)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::RewardActorContext> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::RewardActorContext.cell", v.cell);
  a.field("dungeon_village_reference::RewardActorContext.inside_town", v.inside_town);
  a.field("dungeon_village_reference::RewardActorContext.effects", v.effects);
  a.field("dungeon_village_reference::RewardActorContext.facility_category", v.facility_category);
  a.field("dungeon_village_reference::RewardActorContext.move_area", v.move_area);
  a.field("dungeon_village_reference::RewardActorContext.half_cell", v.half_cell);
  a.field("dungeon_village_reference::RewardActorContext.low_hp", v.low_hp);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::RewardActorContext>().cell)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardActorContext>().inside_town)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardActorContext>().effects)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardActorContext>().facility_category)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardActorContext>().move_area)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardActorContext>().half_cell)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardActorContext>().low_hp)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::RewardEncounter> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::RewardEncounter.runtime", v.runtime);
  a.field("dungeon_village_reference::RewardEncounter.members", v.members);
  a.field("dungeon_village_reference::RewardEncounter.group_exists", v.group_exists);
  a.field("dungeon_village_reference::RewardEncounter.group", v.group);
  a.field("dungeon_village_reference::RewardEncounter.legacy_id", v.legacy_id);
  a.field("dungeon_village_reference::RewardEncounter.influence", v.influence);
  a.field("dungeon_village_reference::RewardEncounter.human_scratch", v.human_scratch);
  a.field("dungeon_village_reference::RewardEncounter.monster_scratch", v.monster_scratch);
  a.field("dungeon_village_reference::RewardEncounter.linked_monsters", v.linked_monsters);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::RewardEncounter>().runtime)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardEncounter>().members)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardEncounter>().group_exists)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardEncounter>().group)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardEncounter>().legacy_id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardEncounter>().influence)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardEncounter>().human_scratch)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardEncounter>().monster_scratch)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardEncounter>().linked_monsters)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::RewardHumanDefinition> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::RewardHumanDefinition.definition", v.definition);
  a.field("dungeon_village_reference::RewardHumanDefinition.derived", v.derived);
  a.field("dungeon_village_reference::RewardHumanDefinition.experience", v.experience);
  a.field("dungeon_village_reference::RewardHumanDefinition.pending", v.pending);
  a.field("dungeon_village_reference::RewardHumanDefinition.notice_pending", v.notice_pending);
  a.field("dungeon_village_reference::RewardHumanDefinition.notice_attributes", v.notice_attributes);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::RewardHumanDefinition>().definition)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardHumanDefinition>().derived)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardHumanDefinition>().experience)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardHumanDefinition>().pending)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardHumanDefinition>().notice_pending)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardHumanDefinition>().notice_attributes)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::RewardMonsterDefinition> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::RewardMonsterDefinition.defeats", v.defeats);
  a.field("dungeon_village_reference::RewardMonsterDefinition.growth", v.growth);
  a.field("dungeon_village_reference::RewardMonsterDefinition.base_death_reward", v.base_death_reward);
  a.field("dungeon_village_reference::RewardMonsterDefinition.base_cash_reward", v.base_cash_reward);
  a.field("dungeon_village_reference::RewardMonsterDefinition.base_hp", v.base_hp);
  a.field("dungeon_village_reference::RewardMonsterDefinition.body", v.body);
  a.field("dungeon_village_reference::RewardMonsterDefinition.sprite_variant", v.sprite_variant);
  a.field("dungeon_village_reference::RewardMonsterDefinition.required_progress", v.required_progress);
  a.field("dungeon_village_reference::RewardMonsterDefinition.status", v.status);
  a.field("dungeon_village_reference::RewardMonsterDefinition.newly_unlocked", v.newly_unlocked);
  a.field("dungeon_village_reference::RewardMonsterDefinition.introduced", v.introduced);
  a.field("dungeon_village_reference::RewardMonsterDefinition.has_introduction_script", v.has_introduction_script);
  a.field("dungeon_village_reference::RewardMonsterDefinition.base_attack", v.base_attack);
  a.field("dungeon_village_reference::RewardMonsterDefinition.base_defense", v.base_defense);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::RewardMonsterDefinition>().defeats)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardMonsterDefinition>().growth)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardMonsterDefinition>().base_death_reward)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardMonsterDefinition>().base_cash_reward)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardMonsterDefinition>().base_hp)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardMonsterDefinition>().body)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardMonsterDefinition>().sprite_variant)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardMonsterDefinition>().required_progress)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardMonsterDefinition>().status)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardMonsterDefinition>().newly_unlocked)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardMonsterDefinition>().introduced)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardMonsterDefinition>().has_introduction_script)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardMonsterDefinition>().base_attack)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::RewardMonsterDefinition>().base_defense)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::ShopActorRecord> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::ShopActorRecord.weapon", v.weapon);
  a.field("dungeon_village_reference::ShopActorRecord.selected_weapon", v.selected_weapon);
  a.field("dungeon_village_reference::ShopActorRecord.selected_armor", v.selected_armor);
  a.field("dungeon_village_reference::ShopActorRecord.selected_accessory", v.selected_accessory);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::ShopActorRecord>().weapon)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ShopActorRecord>().selected_weapon)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ShopActorRecord>().selected_armor)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ShopActorRecord>().selected_accessory)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::ShopHumanRecord> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::ShopHumanRecord.equipment", v.equipment);
  a.field("dungeon_village_reference::ShopHumanRecord.reselect", v.reselect);
  a.field("dungeon_village_reference::ShopHumanRecord.satisfaction", v.satisfaction);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::ShopHumanRecord>().equipment)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ShopHumanRecord>().reselect)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::ShopHumanRecord>().satisfaction)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::SnapshotFacilityTarget> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::SnapshotFacilityTarget.drawn_active_index", v.drawn_active_index);
  a.field("dungeon_village_reference::SnapshotFacilityTarget.drawn_snapshot_index", v.drawn_snapshot_index);
  a.field("dungeon_village_reference::SnapshotFacilityTarget.goal_snapshot_index", v.goal_snapshot_index);
  a.field("dungeon_village_reference::SnapshotFacilityTarget.goal", v.goal);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::SnapshotFacilityTarget>().drawn_active_index)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::SnapshotFacilityTarget>().drawn_snapshot_index)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::SnapshotFacilityTarget>().goal_snapshot_index)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::SnapshotFacilityTarget>().goal)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::TaskRecruitmentAnimation> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::TaskRecruitmentAnimation.entries", v.entries);
  a.field("dungeon_village_reference::TaskRecruitmentAnimation.portraits", v.portraits);
  a.field("dungeon_village_reference::TaskRecruitmentAnimation.arrival_extent", v.arrival_extent);
  a.field("dungeon_village_reference::TaskRecruitmentAnimation.completion_tick", v.completion_tick);
  a.field("dungeon_village_reference::TaskRecruitmentAnimation.portrait_timer", v.portrait_timer);
  a.field("dungeon_village_reference::TaskRecruitmentAnimation.displayed_count", v.displayed_count);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::TaskRecruitmentAnimation>().entries)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::TaskRecruitmentAnimation>().portraits)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::TaskRecruitmentAnimation>().arrival_extent)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::TaskRecruitmentAnimation>().completion_tick)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::TaskRecruitmentAnimation>().portrait_timer)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::TaskRecruitmentAnimation>().displayed_count)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::TaskRecruitmentEntry> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::TaskRecruitmentEntry.human", v.human);
  a.field("dungeon_village_reference::TaskRecruitmentEntry.counter", v.counter);
  a.field("dungeon_village_reference::TaskRecruitmentEntry.phrase", v.phrase);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::TaskRecruitmentEntry>().human)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::TaskRecruitmentEntry>().counter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::TaskRecruitmentEntry>().phrase)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::TownBounds> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::TownBounds.left", v.left);
  a.field("dungeon_village_reference::TownBounds.right", v.right);
  a.field("dungeon_village_reference::TownBounds.top", v.top);
  a.field("dungeon_village_reference::TownBounds.bottom", v.bottom);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::TownBounds>().left)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::TownBounds>().right)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::TownBounds>().top)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::TownBounds>().bottom)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::WorldCalendarState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::WorldCalendarState.year", v.year);
  a.field("dungeon_village_reference::WorldCalendarState.month", v.month);
  a.field("dungeon_village_reference::WorldCalendarState.subperiod", v.subperiod);
  a.field("dungeon_village_reference::WorldCalendarState.units", v.units);
  a.field("dungeon_village_reference::WorldCalendarState.previous_units", v.previous_units);
  a.field("dungeon_village_reference::WorldCalendarState.month_ticks", v.month_ticks);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::WorldCalendarState>().year)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldCalendarState>().month)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldCalendarState>().subperiod)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldCalendarState>().units)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldCalendarState>().previous_units)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldCalendarState>().month_ticks)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::WorldEventTask> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::WorldEventTask.definition_task_flag", v.definition_task_flag);
  a.field("dungeon_village_reference::WorldEventTask.kind", v.kind);
  a.field("dungeon_village_reference::WorldEventTask.center", v.center);
  a.field("dungeon_village_reference::WorldEventTask.encounter", v.encounter);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::WorldEventTask>().definition_task_flag)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldEventTask>().kind)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldEventTask>().center)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldEventTask>().encounter)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::WorldExplorationGroundDisplay> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::WorldExplorationGroundDisplay.cell", v.cell);
  a.field("dungeon_village_reference::WorldExplorationGroundDisplay.kind", v.kind);
  a.field("dungeon_village_reference::WorldExplorationGroundDisplay.delay", v.delay);
  a.field("dungeon_village_reference::WorldExplorationGroundDisplay.x_offset", v.x_offset);
  a.field("dungeon_village_reference::WorldExplorationGroundDisplay.y_offset", v.y_offset);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::WorldExplorationGroundDisplay>().cell)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldExplorationGroundDisplay>().kind)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldExplorationGroundDisplay>().delay)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldExplorationGroundDisplay>().x_offset)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldExplorationGroundDisplay>().y_offset)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::WorldExplorationSummary> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::WorldExplorationSummary.legacy_page", v.legacy_page);
  a.field("dungeon_village_reference::WorldExplorationSummary.task", v.task);
  a.field("dungeon_village_reference::WorldExplorationSummary.definition", v.definition);
  a.field("dungeon_village_reference::WorldExplorationSummary.rewards", v.rewards);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::WorldExplorationSummary>().legacy_page)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldExplorationSummary>().task)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldExplorationSummary>().definition)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldExplorationSummary>().rewards)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::WorldFacilityUpdateDefinition> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::WorldFacilityUpdateDefinition.flags", v.flags);
  a.field("dungeon_village_reference::WorldFacilityUpdateDefinition.popularity_reward", v.popularity_reward);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::WorldFacilityUpdateDefinition>().flags)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldFacilityUpdateDefinition>().popularity_reward)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::WorldFacilityUpdateDetails> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::WorldFacilityUpdateDetails.construction_limit", v.construction_limit);
  a.field("dungeon_village_reference::WorldFacilityUpdateDetails.condition", v.condition);
  a.field("dungeon_village_reference::WorldFacilityUpdateDetails.completion_popularity", v.completion_popularity);
  a.field("dungeon_village_reference::WorldFacilityUpdateDetails.residence_mode", v.residence_mode);
  a.field("dungeon_village_reference::WorldFacilityUpdateDetails.resident_definition", v.resident_definition);
  a.field("dungeon_village_reference::WorldFacilityUpdateDetails.notices", v.notices);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::WorldFacilityUpdateDetails>().construction_limit)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldFacilityUpdateDetails>().condition)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldFacilityUpdateDetails>().completion_popularity)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldFacilityUpdateDetails>().residence_mode)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldFacilityUpdateDetails>().resident_definition)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldFacilityUpdateDetails>().notices)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::WorldMagicPotRecipeProgress> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::WorldMagicPotRecipeProgress.identity", v.identity);
  a.field("dungeon_village_reference::WorldMagicPotRecipeProgress.status", v.status);
  a.field("dungeon_village_reference::WorldMagicPotRecipeProgress.pending_notice", v.pending_notice);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::WorldMagicPotRecipeProgress>().identity)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldMagicPotRecipeProgress>().status)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldMagicPotRecipeProgress>().pending_notice)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::WorldMapNeighbourCache> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::WorldMapNeighbourCache.current", v.current);
  a.field("dungeon_village_reference::WorldMapNeighbourCache.previous", v.previous);
  a.field("dungeon_village_reference::WorldMapNeighbourCache.sources", v.sources);
  a.field("dungeon_village_reference::WorldMapNeighbourCache.visited", v.visited);
  a.field("dungeon_village_reference::WorldMapNeighbourCache.notices", v.notices);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::WorldMapNeighbourCache>().current)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldMapNeighbourCache>().previous)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldMapNeighbourCache>().sources)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldMapNeighbourCache>().visited)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldMapNeighbourCache>().notices)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::WorldPopularityReward> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::WorldPopularityReward.definition", v.definition);
  a.field("dungeon_village_reference::WorldPopularityReward.legacy_tag", v.legacy_tag);
  a.field("dungeon_village_reference::WorldPopularityReward.threshold", v.threshold);
  a.field("dungeon_village_reference::WorldPopularityReward.human", v.human);
  a.field("dungeon_village_reference::WorldPopularityReward.facility", v.facility);
  a.field("dungeon_village_reference::WorldPopularityReward.program", v.program);
  a.field("dungeon_village_reference::WorldPopularityReward.flags", v.flags);
  a.field("dungeon_village_reference::WorldPopularityReward.status", v.status);
  a.field("dungeon_village_reference::WorldPopularityReward.pending_notice", v.pending_notice);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::WorldPopularityReward>().definition)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldPopularityReward>().legacy_tag)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldPopularityReward>().threshold)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldPopularityReward>().human)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldPopularityReward>().facility)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldPopularityReward>().program)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldPopularityReward>().flags)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldPopularityReward>().status)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldPopularityReward>().pending_notice)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::WorldPosition> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::WorldPosition.x", v.x);
  a.field("dungeon_village_reference::WorldPosition.z", v.z);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::WorldPosition>().x)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldPosition>().z)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::WorldRandomSnapshot> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::WorldRandomSnapshot.engine_state", v.engine_state);
  a.field("dungeon_village_reference::WorldRandomSnapshot.tape", v.tape);
  a.field("dungeon_village_reference::WorldRandomSnapshot.cursor", v.cursor);
  a.field("dungeon_village_reference::WorldRandomSnapshot.tape_mode", v.tape_mode);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::WorldRandomSnapshot>().engine_state)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldRandomSnapshot>().tape)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldRandomSnapshot>().cursor)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldRandomSnapshot>().tape_mode)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::WorldSceneState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::WorldSceneState.world", v.world);
  a.field("dungeon_village_reference::WorldSceneState.random", v.random);
  a.field("dungeon_village_reference::WorldSceneState.calendar", v.calendar);
  a.field("dungeon_village_reference::WorldSceneState.scene_state", v.scene_state);
  a.field("dungeon_village_reference::WorldSceneState.speed_setting", v.speed_setting);
  a.field("dungeon_village_reference::WorldSceneState.frame_counter", v.frame_counter);
  a.field("dungeon_village_reference::WorldSceneState.scene_counter", v.scene_counter);
  a.field("dungeon_village_reference::WorldSceneState.processing_phase", v.processing_phase);
  a.field("dungeon_village_reference::WorldSceneState.processing_subphase", v.processing_subphase);
  a.field("dungeon_village_reference::WorldSceneState.first_normal_refresh", v.first_normal_refresh);
  a.field("dungeon_village_reference::WorldSceneState.top_is_main", v.top_is_main);
  a.field("dungeon_village_reference::WorldSceneState.framework_paused", v.framework_paused);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::WorldSceneState>().world)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldSceneState>().random)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldSceneState>().calendar)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldSceneState>().scene_state)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldSceneState>().speed_setting)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldSceneState>().frame_counter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldSceneState>().scene_counter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldSceneState>().processing_phase)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldSceneState>().processing_subphase)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldSceneState>().first_normal_refresh)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldSceneState>().top_is_main)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldSceneState>().framework_paused)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::WorldScheduleState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::WorldScheduleState.world", v.world);
  a.field("dungeon_village_reference::WorldScheduleState.surface", v.surface);
  a.field("dungeon_village_reference::WorldScheduleState.map_flags", v.map_flags);
  a.field("dungeon_village_reference::WorldScheduleState.town", v.town);
  a.field("dungeon_village_reference::WorldScheduleState.spawn_cells", v.spawn_cells);
  a.field("dungeon_village_reference::WorldScheduleState.facility_order", v.facility_order);
  a.field("dungeon_village_reference::WorldScheduleState.updates", v.updates);
  a.field("dungeon_village_reference::WorldScheduleState.hints", v.hints);
  a.field("dungeon_village_reference::WorldScheduleState.floating_notes", v.floating_notes);
  a.field("dungeon_village_reference::WorldScheduleState.popularity_queue", v.popularity_queue);
  a.field("dungeon_village_reference::WorldScheduleState.rescue_available", v.rescue_available);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScheduleState>().world)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScheduleState>().surface)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScheduleState>().map_flags)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScheduleState>().town)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScheduleState>().spawn_cells)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScheduleState>().facility_order)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScheduleState>().updates)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScheduleState>().hints)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScheduleState>().floating_notes)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScheduleState>().popularity_queue)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScheduleState>().rescue_available)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::WorldScriptContinuation> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::WorldScriptContinuation.event", v.event);
  a.field("dungeon_village_reference::WorldScriptContinuation.next_instruction", v.next_instruction);
  a.field("dungeon_village_reference::WorldScriptContinuation.remaining_updates", v.remaining_updates);
  a.field("dungeon_village_reference::WorldScriptContinuation.legacy_tag", v.legacy_tag);
  a.field("dungeon_village_reference::WorldScriptContinuation.saved_context", v.saved_context);
  a.field("dungeon_village_reference::WorldScriptContinuation.replacement", v.replacement);
  a.field("dungeon_village_reference::WorldScriptContinuation.parameters", v.parameters);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptContinuation>().event)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptContinuation>().next_instruction)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptContinuation>().remaining_updates)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptContinuation>().legacy_tag)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptContinuation>().saved_context)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptContinuation>().replacement)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptContinuation>().parameters)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::WorldScriptFacilityDefinition> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::WorldScriptFacilityDefinition.category", v.category);
  a.field("dungeon_village_reference::WorldScriptFacilityDefinition.icon", v.icon);
  a.field("dungeon_village_reference::WorldScriptFacilityDefinition.level", v.level);
  a.field("dungeon_village_reference::WorldScriptFacilityDefinition.economy", v.economy);
  a.field("dungeon_village_reference::WorldScriptFacilityDefinition.improvements", v.improvements);
  a.field("dungeon_village_reference::WorldScriptFacilityDefinition.attributes", v.attributes);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptFacilityDefinition>().category)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptFacilityDefinition>().icon)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptFacilityDefinition>().level)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptFacilityDefinition>().economy)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptFacilityDefinition>().improvements)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptFacilityDefinition>().attributes)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::WorldScriptFinance> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::WorldScriptFinance.cash", v.cash);
  a.field("dungeon_village_reference::WorldScriptFinance.cash_peak", v.cash_peak);
  a.field("dungeon_village_reference::WorldScriptFinance.cash_peak_village", v.cash_peak_village);
  a.field("dungeon_village_reference::WorldScriptFinance.legacy_flags14", v.legacy_flags14);
  a.field("dungeon_village_reference::WorldScriptFinance.month", v.month);
  a.field("dungeon_village_reference::WorldScriptFinance.localized_gold_template", v.localized_gold_template);
  a.field("dungeon_village_reference::WorldScriptFinance.monthly_totals", v.monthly_totals);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptFinance>().cash)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptFinance>().cash_peak)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptFinance>().cash_peak_village)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptFinance>().legacy_flags14)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptFinance>().month)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptFinance>().localized_gold_template)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptFinance>().monthly_totals)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::WorldScriptNotice> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::WorldScriptNotice.message", v.message);
  a.field("dungeon_village_reference::WorldScriptNotice.counter", v.counter);
  a.field("dungeon_village_reference::WorldScriptNotice.duration", v.duration);
  a.field("dungeon_village_reference::WorldScriptNotice.replacement", v.replacement);
  a.field("dungeon_village_reference::WorldScriptNotice.text", v.text);
  a.field("dungeon_village_reference::WorldScriptNotice.definitions", v.definitions);
  a.field("dungeon_village_reference::WorldScriptNotice.human_definition", v.human_definition);
  a.field("dungeon_village_reference::WorldScriptNotice.attribute_changes", v.attribute_changes);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptNotice>().message)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptNotice>().counter)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptNotice>().duration)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptNotice>().replacement)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptNotice>().text)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptNotice>().definitions)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptNotice>().human_definition)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptNotice>().attribute_changes)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::WorldScriptPage> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::WorldScriptPage.id", v.id);
  a.field("dungeon_village_reference::WorldScriptPage.lifecycle", v.lifecycle);
  a.field("dungeon_village_reference::WorldScriptPage.kind", v.kind);
  a.field("dungeon_village_reference::WorldScriptPage.legacy_page", v.legacy_page);
  a.field("dungeon_village_reference::WorldScriptPage.source_record", v.source_record);
  a.field("dungeon_village_reference::WorldScriptPage.replacement", v.replacement);
  a.field("dungeon_village_reference::WorldScriptPage.title", v.title);
  a.field("dungeon_village_reference::WorldScriptPage.paragraphs", v.paragraphs);
  a.field("dungeon_village_reference::WorldScriptPage.speaker_kind", v.speaker_kind);
  a.field("dungeon_village_reference::WorldScriptPage.speaker_definition", v.speaker_definition);
  a.field("dungeon_village_reference::WorldScriptPage.legacy_tag", v.legacy_tag);
  a.field("dungeon_village_reference::WorldScriptPage.legacy_r", v.legacy_r);
  a.field("dungeon_village_reference::WorldScriptPage.legacy_s", v.legacy_s);
  a.field("dungeon_village_reference::WorldScriptPage.legacy_t", v.legacy_t);
  a.field("dungeon_village_reference::WorldScriptPage.legacy_f", v.legacy_f);
  a.field("dungeon_village_reference::WorldScriptPage.legacy_g", v.legacy_g);
  a.field("dungeon_village_reference::WorldScriptPage.legacy_l", v.legacy_l);
  a.field("dungeon_village_reference::WorldScriptPage.message_commands", v.message_commands);
  a.field("dungeon_village_reference::WorldScriptPage.task_identity", v.task_identity);
  a.field("dungeon_village_reference::WorldScriptPage.task_definition", v.task_definition);
  a.field("dungeon_village_reference::WorldScriptPage.monster_definition", v.monster_definition);
  a.field("dungeon_village_reference::WorldScriptPage.facility_definition", v.facility_definition);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().lifecycle)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().kind)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().legacy_page)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().source_record)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().replacement)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().title)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().paragraphs)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().speaker_kind)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().speaker_definition)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().legacy_tag)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().legacy_r)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().legacy_s)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().legacy_t)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().legacy_f)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().legacy_g)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().legacy_l)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().message_commands)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().task_identity)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().task_definition)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().monster_definition)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptPage>().facility_definition)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::WorldScriptState> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::WorldScriptState.event_calls", v.event_calls);
  a.field("dungeon_village_reference::WorldScriptState.continuations", v.continuations);
  a.field("dungeon_village_reference::WorldScriptState.context", v.context);
  a.field("dungeon_village_reference::WorldScriptState.village_name", v.village_name);
  a.field("dungeon_village_reference::WorldScriptState.pending_completion", v.pending_completion);
  a.field("dungeon_village_reference::WorldScriptState.popularity_queue", v.popularity_queue);
  a.field("dungeon_village_reference::WorldScriptState.pages", v.pages);
  a.field("dungeon_village_reference::WorldScriptState.executing_page", v.executing_page);
  a.field("dungeon_village_reference::WorldScriptState.next_page_id", v.next_page_id);
  a.field("dungeon_village_reference::WorldScriptState.page_mutations_locked", v.page_mutations_locked);
  a.field("dungeon_village_reference::WorldScriptState.redraw_requested", v.redraw_requested);
  a.field("dungeon_village_reference::WorldScriptState.selected_actor", v.selected_actor);
  a.field("dungeon_village_reference::WorldScriptState.selected_monster", v.selected_monster);
  a.field("dungeon_village_reference::WorldScriptState.selected_facility", v.selected_facility);
  a.field("dungeon_village_reference::WorldScriptState.human_order", v.human_order);
  a.field("dungeon_village_reference::WorldScriptState.scene_mode", v.scene_mode);
  a.field("dungeon_village_reference::WorldScriptState.scene_updates", v.scene_updates);
  a.field("dungeon_village_reference::WorldScriptState.scene_labels", v.scene_labels);
  a.field("dungeon_village_reference::WorldScriptState.selection_mode", v.selection_mode);
  a.field("dungeon_village_reference::WorldScriptState.finance", v.finance);
  a.field("dungeon_village_reference::WorldScriptState.notices", v.notices);
  a.field("dungeon_village_reference::WorldScriptState.activities", v.activities);
  a.field("dungeon_village_reference::WorldScriptState.humans", v.humans);
  a.field("dungeon_village_reference::WorldScriptState.professions", v.professions);
  a.field("dungeon_village_reference::WorldScriptState.facilities", v.facilities);
  a.field("dungeon_village_reference::WorldScriptState.human_catalog_complete", v.human_catalog_complete);
  a.field("dungeon_village_reference::WorldScriptState.facility_catalog_complete", v.facility_catalog_complete);
  a.field("dungeon_village_reference::WorldScriptState.job_counts", v.job_counts);
  a.field("dungeon_village_reference::WorldScriptState.medal_count", v.medal_count);
  a.field("dungeon_village_reference::WorldScriptState.exploration_phase", v.exploration_phase);
  a.field("dungeon_village_reference::WorldScriptState.user_flags", v.user_flags);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().event_calls)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().continuations)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().context)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().village_name)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().pending_completion)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().popularity_queue)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().pages)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().executing_page)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().next_page_id)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().page_mutations_locked)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().redraw_requested)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().selected_actor)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().selected_monster)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().selected_facility)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().human_order)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().scene_mode)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().scene_updates)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().scene_labels)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().selection_mode)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().finance)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().notices)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().activities)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().humans)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().professions)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().facilities)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().human_catalog_complete)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().facility_catalog_complete)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().job_counts)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().medal_count)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().exploration_phase)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptState>().user_flags)>::minimum(); }
};
template<> struct CodecFields< ::dungeon_village_reference::WorldScriptUnlockDefinition> {
 template<class Archive, class Value> static void visit(Archive& a, Value& v) {
  a.field("dungeon_village_reference::WorldScriptUnlockDefinition.status", v.status);
  a.field("dungeon_village_reference::WorldScriptUnlockDefinition.pending_notice", v.pending_notice);
  a.field("dungeon_village_reference::WorldScriptUnlockDefinition.name", v.name);
  a.field("dungeon_village_reference::WorldScriptUnlockDefinition.extra", v.extra);
  a.field("dungeon_village_reference::WorldScriptUnlockDefinition.satisfaction", v.satisfaction);
 }
 static std::size_t minimum() { return 0 + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptUnlockDefinition>().status)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptUnlockDefinition>().pending_notice)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptUnlockDefinition>().name)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptUnlockDefinition>().extra)>::minimum() + Codec<decltype(std::declval< ::dungeon_village_reference::WorldScriptUnlockDefinition>().satisfaction)>::minimum(); }
};
