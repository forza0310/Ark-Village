#include "dungeon_village_prototype/startup_view.hpp"
#include "dungeon_village_prototype/road_render.hpp"
#include "dungeon_village_prototype/startup.hpp"
#include "dungeon_village_prototype/startup_world_building.hpp"
#include "dungeon_village_prototype/startup_world_commerce.hpp"
#include "dungeon_village_prototype/startup_world_editing.hpp"
#include "dungeon_village_prototype/startup_world_facility_items.hpp"
#include "dungeon_village_prototype/startup_world_facility_catalog.hpp"
#include "dungeon_village_prototype/startup_world_magic_pot.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"
#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include "dungeon_village_prototype/startup_world_persistence.hpp"
#include "dungeon_village_prototype/startup_world_runtime_tasks.hpp"
#include "dungeon_village_prototype/startup_world_menu.hpp"
#include "dungeon_village_prototype/startup_world_information.hpp"
#include "dungeon_village_prototype/steam_main_menu_skin.hpp"
#include "dungeon_village_prototype/steam_information_skin.hpp"
#include "dungeon_village_prototype/startup_world_tax.hpp"
#include "dungeon_village_prototype/startup_world_village_activity.hpp"
#include "dungeon_village_prototype/startup_world_visuals.hpp"
#include "dungeon_village_reference/world_notices.hpp"
#include "dungeon_village_tools/sprite.hpp"
#include "dungeon_village_tools/table.hpp"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <set>
#include <stdexcept>

namespace dungeon_village_prototype {
namespace {
constexpr int width = 240, height = 320, scale = 2;
// 原21窗口中心偏移；绘制与本研究窗口拾取共用逻辑坐标，不写入领域状态。
constexpr int build_menu_x = width / 2 - 87, build_menu_y = height / 2 - 104;
const Color ink{48, 44, 46, 255}, paper{248, 248, 244, 255};

std::vector<std::uint8_t> read_bytes(const std::filesystem::path &path) {
    if (std::filesystem::file_size(path) > 1024 * 1024)
        throw std::runtime_error("素材元数据过大");
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("素材元数据无法读取");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

// Decode only the published SEB schema; PNG rectangles, image bindings and raw flips are checked
// separately. legacy_tag is retained by the tool parser and is never used as a record count.
class SourceSprites {
  public:
    enum class Binding { map, farmer, secretary, human, monster, common, weapon };
    explicit SourceSprites(std::filesystem::path root) : root_(std::move(root)) {
        for (const auto &row :
             dungeon_village_tools::parse_tsv(read_bytes(root_ / "image/img.inf"))) {
            if (row.size() != 2)
                throw std::runtime_error("图片索引列数错误");
            auto name = std::filesystem::path(row[1]);
            if (name.has_parent_path())
                throw std::runtime_error("图片索引路径不安全");
            name.replace_extension(".png");
            const auto id = dungeon_village_tools::parse_table_integer(row[0]);
            if (!images_.emplace(id, name).second)
                throw std::runtime_error("图片索引重复");
        }
        for (const auto *group : {"human", "monster", "common", "weapon"}) {
            for (const auto &row :
                 dungeon_village_tools::parse_tsv(read_bytes(root_ / group / "img.inf"))) {
                if (row.size() != 2)
                    throw std::runtime_error("人物图片索引列数错误");
                std::filesystem::path path(row[1]);
                if (path.has_parent_path())
                    throw std::runtime_error("人物图片索引路径不安全");
                path.replace_extension(".png");
                if (!actor_images_[group]
                         .emplace(dungeon_village_tools::parse_table_integer(row[0]), path)
                         .second)
                    throw std::runtime_error("人物图片索引重复");
            }
            for (const auto &row :
                 dungeon_village_tools::parse_tsv(read_bytes(root_ / group / "seb.inf"))) {
                if (row.size() != 1 || std::filesystem::path(row[0]).has_parent_path())
                    throw std::runtime_error("人物精灵索引错误");
                actor_sprites_[group].push_back(row[0]);
            }
        }
    }
    ~SourceSprites() {
        for (const auto &v : textures_)
            UnloadTexture(v.second);
    }
    SourceSprites(const SourceSprites &) = delete;
    SourceSprites &operator=(const SourceSprites &) = delete;
    void draw(const std::string &sprite, int frame, Vector2 anchor, Color tint = WHITE,
              Binding binding = Binding::map, float factor = 1, int image_override = -1, int layer_only = -1) {
        const char *group = binding == Binding::farmer || binding == Binding::human ? "human"
                            : binding == Binding::monster                           ? "monster"
                            : binding == Binding::secretary || binding == Binding::common ? "common"
                            : binding == Binding::weapon                            ? "weapon"
                                                                                          : "image";
        const auto relative = std::filesystem::path(group) / sprite;
        auto it = sprites_.find(relative.string());
        if (it == sprites_.end())
            it = sprites_
                     .emplace(relative.string(),
                              dungeon_village_tools::parse_legacy_seb(read_bytes(root_ / relative)))
                     .first;
        // 原t.b(frame,layer)在layer首末key之外返回null，draw(null)不绘。
        // 地图逻辑朝向可以请求单帧SEB没有的frame1，不能抛异常或伪造复用frame0。
        if (binding == Binding::map && frame >= it->second.frame_count)
            return;
        if (frame < 0 || frame >= it->second.frame_count)
            throw std::runtime_error("源变体超出SEB帧边界");
        for (std::size_t layer_index = 0; layer_index < it->second.layers.size(); ++layer_index) {
            if (layer_only >= 0 && layer_index != static_cast<std::size_t>(layer_only)) continue;
            const auto &layer = it->second.layers[layer_index];
            for (const auto &part : layer.parts) {
                if (part.frame != frame)
                    continue;
                if (binding == Binding::secretary && part.image_index != 126)
                    throw std::runtime_error("秘书SEB图片绑定发生变化");
                const auto path =
                    image_override >= 0 ? root_ / group / actor_images_.at(group).at(image_override)
                    : binding == Binding::farmer    ? root_ / "human/chara_flower00.png"
                    : binding == Binding::secretary ? root_ / "common/chara_hishoko01.png"
                    : binding == Binding::common
                        ? root_ / "common" / actor_images_.at("common").at(part.image_index)
                        : root_ / "image" / images_.at(part.image_index);
                auto image = textures_.find(path.string());
                if (image == textures_.end()) {
                    const auto texture = LoadTexture(path.string().c_str());
                    if (texture.id == 0)
                        throw std::runtime_error("原始纹理无法读取");
                    SetTextureFilter(texture, TEXTURE_FILTER_POINT);
                    image = textures_.emplace(path.string(), texture).first;
                }
                const auto &tex = image->second;
                if (part.source_x < 0 || part.source_y < 0 || part.width <= 0 || part.height <= 0 ||
                    part.source_x + part.width > tex.width ||
                    part.source_y + part.height > tex.height || part.flip_x < 0 ||
                    part.flip_x > 1 || part.flip_y < 0 || part.flip_y > 1)
                    throw std::runtime_error("SEB图片矩形或翻转字段无效");
                DrawTexturePro(
                    tex,
                    {static_cast<float>(part.source_x), static_cast<float>(part.source_y),
                     static_cast<float>(part.flip_x ? -part.width : part.width),
                     static_cast<float>(part.flip_y ? -part.height : part.height)},
                    {anchor.x + part.offset_x * factor, anchor.y + part.offset_y * factor,
                     part.width * factor, part.height * factor},
                    {0, 0}, 0, tint);
            }
        }
    }
    void actor(bool monster, int sprite_index, int image_index, int frame, Vector2 anchor) {
        const char *group = monster ? "monster" : "human";
        draw(actor_sprites_.at(group).at(static_cast<std::size_t>(sprite_index)), frame, anchor,
             WHITE, monster ? Binding::monster : Binding::human, 1, image_index);
    }
    void visual(const StartupVisualDraw &plan, Vector2 anchor) {
        const char *group = plan.resource == StartupVisualResource::weapon ? "weapon" : "common";
        const Vector2 p{anchor.x + plan.offset[0], anchor.y + plan.offset[1]};
        if (plan.sprite >= 0) {
            draw(actor_sprites_.at(group).at(static_cast<std::size_t>(plan.sprite)), plan.frame, p,
                 WHITE, plan.resource == StartupVisualResource::weapon ? Binding::weapon : Binding::common,
                 1, plan.image, plan.layer);
        } else {
            crop(std::filesystem::path(group) / actor_images_.at(group).at(plan.image),
                 {static_cast<float>(plan.crop[0]), static_cast<float>(plan.crop[1]),
                  static_cast<float>(plan.crop[2]), static_cast<float>(plan.crop[3])}, p);
        }
    }
    void building(const std::vector<StartupBuildingDraw> &plans, Vector2 anchor) {
        for (const auto &part : plans)
            draw(part.sprite, part.frame,
                 {anchor.x + part.offset[0], anchor.y + part.offset[1]});
    }
    void portrait(const StartupPortrait &plan, Vector2 position) {
        BeginScissorMode(static_cast<int>(position.x), static_cast<int>(position.y),
                         plan.clip_width, plan.clip_height);
        actor(false, plan.sprite, plan.image, plan.frame,
              {position.x + plan.anchor_x, position.y + plan.anchor_y});
        EndScissorMode();
    }
    void crop(const std::filesystem::path &relative, Rectangle source, Vector2 position,
              bool published = false) {
        if (relative.is_absolute() || relative.string().find("..") != std::string::npos)
            throw std::runtime_error("裁剪资源路径无效");
        const auto path = (published ? root_.parent_path() : root_) / relative;
        auto found = textures_.find(path.string());
        if (found == textures_.end()) {
            const auto texture = LoadTexture(path.string().c_str());
            if (!texture.id)
                throw std::runtime_error("裁剪资源无法读取");
            SetTextureFilter(texture, TEXTURE_FILTER_POINT);
            found = textures_.emplace(path.string(), texture).first;
        }
        if (source.x < 0 || source.y < 0 || source.width <= 0 || source.height <= 0 ||
            source.x + source.width > found->second.width ||
            source.y + source.height > found->second.height)
            throw std::runtime_error("裁剪资源矩形越界");
        DrawTextureRec(found->second, source, position, WHITE);
    }
    void image(const std::filesystem::path &relative, Vector2 position) {
        if (relative.is_absolute() || relative.string().find("..") != std::string::npos)
            throw std::runtime_error("整图资源路径无效");
        const auto path = root_ / relative;
        auto found = textures_.find(path.string());
        if (found == textures_.end()) {
            const auto texture = LoadTexture(path.string().c_str());
            if (!texture.id)
                throw std::runtime_error("整图资源无法读取");
            SetTextureFilter(texture, TEXTURE_FILTER_POINT);
            found = textures_.emplace(path.string(), texture).first;
        }
        DrawTextureV(found->second, position, WHITE);
    }
    void menu_number(const SteamFacilityNumber &number, Vector2 origin) {
        const auto resource = steam_facility_resource(number.asset);
        if (!resource || !resource->published_sprite)
            throw std::runtime_error("菜单数字缺资源映射");
        const auto key = std::string(resource->published_sprite);
        auto found = sprites_.find(key);
        if (found == sprites_.end())
            found = sprites_.emplace(key, dungeon_village_tools::parse_legacy_seb(
                read_bytes(root_.parent_path() / key))).first;
        const auto &definition = found->second;
        if (definition.layers.empty() || definition.layers[0].parts.empty())
            throw std::runtime_error("菜单数字缺字宽来源");
        const auto digits = steam_facility_number_draws(number, definition.layers[0].parts[0].width);
        if (!digits) throw std::runtime_error("菜单数字计划无效");
        for (const auto &digit : *digits)
            for (const auto &layer : definition.layers)
                for (const auto &part : layer.parts) {
                    if (part.frame != digit.frame) continue;
                    if (part.flip_x || part.flip_y) throw std::runtime_error("菜单数字翻转尚未接入");
                    crop(resource->published_image,
                         {float(part.source_x), float(part.source_y), float(part.width), float(part.height)},
                         {origin.x + digit.position[0] + part.offset_x,
                          origin.y + digit.position[1] + part.offset_y}, true);
                }
    }

  private:
    std::filesystem::path root_;
    std::map<int, std::filesystem::path> images_;
    std::map<std::string, std::map<int, std::filesystem::path>> actor_images_;
    std::map<std::string, std::vector<std::string>> actor_sprites_;
    std::map<std::string, dungeon_village_tools::SpriteDefinition> sprites_;
    std::map<std::string, Texture2D> textures_;
};

// Original tile-plane increments and y-flip, with this adapter's 240x320 viewport midpoint.
Vector2 project(ref::Position cell, Vector2 camera) {
    return {120 + 30.0F * (cell.x + cell.y) - camera.x,
            160 - 15.0F * (cell.y - cell.x) - 15 + camera.y};
}
std::optional<ref::Position> pick(Vector2 point, Vector2 camera) {
    const auto &data = startup_evidence();
    for (int y = 0; y < data.height; ++y)
        for (int x = 0; x < data.width; ++x) {
            const auto p = project({x, y}, camera);
            if (std::abs(point.x - p.x) / 30 + std::abs(point.y - p.y) / 15 <= 1)
                return ref::Position{x, y};
        }
    return std::nullopt;
}

class ChineseFont {
  public:
    explicit ChineseFont(const std::filesystem::path &path, const std::string &extra = {}) {
        if (!std::filesystem::is_regular_file(path))
            throw std::runtime_error("请用 --font 指定可用中文TTF字体");
        std::string glyphs = "本月结算打倒怪物获得村子收入支出收支成果姓名数下降完成"
                             "建设返回确定撤除道路植物商店饮食金币点数人气年月份倍暂停继续月末"
                             "请选择街道内地域有建筑物金钱不足没有设施不可撤除状态异常施工"
                             "设施情报品质魅力维护费收入建设完毕加成返回图范围";
        for (int i = 32; i < 127; ++i)
            glyphs += static_cast<char>(i);
        for (const auto &v : startup_evidence().definitions)
            glyphs += v.name;
        for (const auto &v : startup_evidence().first_talk)
            glyphs += v;
        glyphs += startup_evidence().first_character.name;
        glyphs += extra;
        int count = 0;
        int *raw = LoadCodepoints(glyphs.c_str(), &count);
        if (!raw)
            throw std::runtime_error("中文码点读取失败");
        const std::set<int> unique(raw, raw + count);
        UnloadCodepoints(raw);
        std::vector<int> codes(unique.begin(), unique.end());
        font_ = LoadFontEx(path.string().c_str(), 24, codes.data(), static_cast<int>(codes.size()));
        if (font_.texture.id == 0 || font_.texture.id == GetFontDefault().texture.id)
            throw std::runtime_error("中文字体加载失败");
        for (const auto code : codes) {
            if (code > 127 && font_.glyphs[GetGlyphIndex(font_, code)].value != code) {
                UnloadFont(font_);
                throw std::runtime_error("中文字体缺少必要字形");
            }
        }
        SetTextureFilter(font_.texture, TEXTURE_FILTER_BILINEAR);
    }
    ~ChineseFont() { UnloadFont(font_); }
    ChineseFont(const ChineseFont &) = delete;
    ChineseFont &operator=(const ChineseFont &) = delete;
    void text(const std::string &value, float x, float y, Color color = ink,
              float size = 12) const {
        DrawTextEx(font_, value.c_str(), {x, y}, size, 0, color);
    }
    float measure(const std::string &value, float size = 12) const {
        return MeasureTextEx(font_, value.c_str(), size, 0).x;
    }
    // Wrap at measured codepoint boundaries, preserving Chinese without inserting broken UTF-8.
    float paragraph(const std::string &value, float x, float y, float max_width) const {
        const float start_y = y;
        std::string line;
        for (std::size_t i = 0; i < value.size();) {
            const auto first = static_cast<unsigned char>(value[i]);
            const std::size_t length = first < 128 ? 1 : first < 224 ? 2 : first < 240 ? 3 : 4;
            const auto next = value.substr(i, length);
            if (!line.empty() && MeasureTextEx(font_, (line + next).c_str(), 12, 0).x > max_width) {
                text(line, x, y);
                line.clear();
                y += 17;
            }
            line += next;
            i += length;
        }
        text(line, x, y);
        return y - start_y + 17;
    }

  private:
    Font font_{};
};

bool navigation_menu(int raw) { return raw == 3 || raw == 4 || raw == 7 || raw == 10; }
// 研究窗口中文适配，不认证Steam翻译/字体；标签编号来自真实Owner冻结目录。
const char *menu_label(int tag) {
    switch (tag) {
    case 0:return "建设";case 1:return "冒险";case 2:return "村办";case 3:return "开发";
    case 5:return "情报";case 6:return "系统";case 7:return "任务进度";case 8:return "中止任务";
    case 9:return "赠送礼物";case 10:return "晋级";case 11:return "商会";case 12:return "活动";
    case 14:return "村情报";case 15:return "冒险者";case 16:return "收支情报";
    case 17:return "持有物品";case 18:return "装备一览";
    case 20:return "保存";case 21:return "纪录";case 22:return "配置";case 23:return "排行榜";
    case 24:return "结束游戏";default:throw std::runtime_error("菜单标签缺文字适配");
    }
}
struct WindowNavigationPlan {
    std::array<int,2> origin{};
    std::optional<SteamMainMenuSkinPlan> main;
    std::optional<SteamInformationSkinPlan> system;
    std::optional<StartupWorldMenuView> fallback;
    std::vector<SteamStartupTouch> touches;
};
std::optional<WindowNavigationPlan> window_navigation_plan(const StartupWorldRuntimeState &state,
    std::uint64_t id,const ChineseFont &font,bool on_top) {
    const auto information=inspect_startup_world_information_page(state,id);
    if (information && information->raw==9) {
        WindowNavigationPlan result;
        const auto position=state.menu_page_positions.find(id);
        if(position!=state.menu_page_positions.end()) result.origin=position->second;
        SteamInformationMenuSkinOptions options;options.canvas={width,height};options.origin=result.origin;
        options.on_top=on_top;
        if(information->frame==3) {
            std::array<int,5> widths{};
            for(std::size_t row=0;row<widths.size();++row)
                widths[row]=static_cast<int>(font.measure(menu_label(information->entries[row].tag)));
            options.measured_text_widths=widths;
        }
        result.system=steam_information_menu_skin(state,id,options);
        if(!result.system)return {};
        for(const auto &touch:result.system->touches)result.touches.push_back(touch);
        return result;
    }
    const auto view = inspect_startup_world_menu_page(state,id);
    if (!view) return {};
    WindowNavigationPlan result;result.origin=view->stored_position;
    if (view->raw == 3) {
        SteamMainMenuSkinInput input;
        input.frame=view->frame;input.selection=view->selection;
        input.magic_unlocked=std::find(view->tags.begin(),view->tags.end(),3)!=view->tags.end();
        if (input.frame==3) {
            input.notices=steam_main_menu_notices(state);
            if (input.magic_unlocked) input.magic_period=startup_magic_pot_menu_information(state);
        }
        SteamMainMenuSkinOptions options;options.canvas={width,height};options.origin=result.origin;
        options.on_top=on_top;options.top_is_main_menu=on_top;
        result.main=steam_main_menu_skin(input,options);
        if (!result.main) return {};
        result.touches=result.main->touches;
    } else if (view->raw == 10) {
        SteamInformationMenuSkinOptions options;options.canvas={width,height};options.origin=result.origin;
        options.on_top=on_top;
        if (view->frame==3) {
            std::array<int,5> widths{};
            for (std::size_t row=0;row<widths.size();++row)
                widths[row]=static_cast<int>(font.measure(menu_label(view->tags[row])));
            options.measured_text_widths=widths;
        }
        result.system=steam_system_menu_skin({view->frame,view->selection},options);
        if (!result.system) return {};
        for (const auto &touch:result.system->touches) result.touches.push_back(touch);
    } else {
        // raw4/7本批只接真实目录与输入。此短列表是维护适配，不能冒认为已证完整皮肤。
        result.fallback=view;
        for (std::size_t row=0;row<view->tags.size();++row)
            result.touches.push_back({9,0x20000|static_cast<int>(row),
                                      std::array<int,4>{0,28*static_cast<int>(row),90,28},{},0});
    }
    return result;
}
void draw_navigation_plan(const WindowNavigationPlan &plan,SourceSprites &sprites,const ChineseFont &font) {
    const Vector2 origin{float(plan.origin[0]),float(plan.origin[1])};
    const auto draw_image=[&](const std::string &path,int sprite,int frame,
                              const std::array<int,4> &crop,const std::array<int,2> &position) {
        if (sprite>=0) {
            // 独立展示帧0不推进世界；Steam绘制动画/内部光标时序仍由后续平台消费者认证。
            sprites.visual({StartupVisualResource::common,sprite,70,std::max(0,frame),0,{},position},origin);
        } else if(crop[2]>0 && crop[3]>0)
            sprites.crop(path,{float(crop[0]),float(crop[1]),float(crop[2]),float(crop[3])},
                         {origin.x+position[0],origin.y+position[1]},true);
    };
    if (plan.main) for (const auto &draw:plan.main->draws) {
        if (const auto *part=std::get_if<SteamMainMenuImage>(&draw)) {
            const auto path=steam_main_menu_image(part->package,part->image);
            if(!path) throw std::runtime_error("主菜单缺出版资源");
            draw_image(std::string(*path),part->sprite,part->frame,part->crop,part->position);
        } else if(const auto *label=std::get_if<SteamMainMenuText>(&draw))
            font.text(menu_label(label->tag),origin.x+label->position[0],origin.y+label->position[1],
                      {static_cast<unsigned char>(label->rgb[0]),static_cast<unsigned char>(label->rgb[1]),
                       static_cast<unsigned char>(label->rgb[2]),255});
        else if(const auto *number=std::get_if<SteamFacilityNumber>(&draw))sprites.menu_number(*number,origin);
    }
    if (plan.system) for (const auto &draw:plan.system->draws) {
        if (const auto *part=std::get_if<StartupSkinDraw>(&draw)) {
            const auto path=steam_information_image(part->image);
            if(!path) throw std::runtime_error("系统菜单缺出版资源");
            draw_image(std::string(*path),part->sprite,part->frame,part->crop,part->offset);
        } else if(const auto *label=std::get_if<SteamInformationText>(&draw))
            font.text(menu_label(label->argument),origin.x+label->position[0],origin.y+label->position[1],
                      {static_cast<unsigned char>(label->rgb[0]),static_cast<unsigned char>(label->rgb[1]),
                       static_cast<unsigned char>(label->rgb[2]),255},label->font_size?float(label->font_size):12.F);
    }
    if (plan.fallback) for(std::size_t row=0;row<plan.fallback->tags.size();++row) {
        const int y=plan.origin[1]+28*static_cast<int>(row);
        DrawRectangle(plan.origin[0],y,90,27,paper);
        if(static_cast<int>(row)==plan.fallback->selection)
            DrawRectangleLines(plan.origin[0],y,90,27,ORANGE);
        font.text(menu_label(plan.fallback->tags[row]),origin.x+5,float(y+8),ink,11);
    }
}
struct Window {
    Window() {
        SetTraceLogLevel(LOG_WARNING);
        InitWindow(width * scale, height * scale, "Dungeon Village - startup research");
        if (!IsWindowReady())
            throw std::runtime_error("窗口初始化失败");
        SetExitKey(KEY_NULL);
        SetTargetFPS(60);
    }
    ~Window() { CloseWindow(); }
};
struct Canvas {
    RenderTexture2D value{LoadRenderTexture(width, height)};
    Canvas() {
        if (!value.id)
            throw std::runtime_error("画布创建失败");
        SetTextureFilter(value.texture, TEXTURE_FILTER_POINT);
    }
    ~Canvas() { UnloadRenderTexture(value); }
};
const char *error_text(StartupError error) {
    switch (error) {
    case StartupError::none:
        return "";
    case StartupError::insufficient_funds:
        return "金钱不足";
    case StartupError::outside_town:
        return "请选择街道内地域";
    case StartupError::occupied:
        return "有建筑物";
    case StartupError::not_found:
        return "没有设施";
    case StartupError::protected_seed:
        return "不可撤除";
    default:
        return "状态异常";
    }
}
} // namespace

void check_startup() {
    StartupSession session;
    for (int i = 0; i < 420; ++i)
        session.update();
    if (!session.state().character || session.state().character->uid != 0 ||
        session.state().character->definition_id != 1 || session.state().event89_count != 1 ||
        session.state().accounting.funds() != 5000 || session.state().mode != StartupMode::tutorial)
        throw std::runtime_error("新局首访自检失败");
    std::cout << "startup check passed: source_cells=576 seed_instances=8 uid=0 definition=1 "
                 "money=5000\n";
}

// Input is an adapter, not a reproduction of original physical hotspots. Logic uses exactly one
// outer update per rendered frame (or two at 2x); target FPS is presentation pacing, not APK time.
int run_startup_window(const std::filesystem::path &assets, const std::filesystem::path &font_path,
                       bool paused, int frames,
                       const std::optional<std::filesystem::path> &screenshot,
                       const std::string &inspect_page) {
    Window window;
    Canvas canvas;
    SourceSprites sprites(assets / "original");
    ChineseFont font(font_path);
    StartupSession session;
    session.set_paused(paused);
    const auto &data = startup_evidence();
    Vector2 camera{static_cast<float>(data.camera.x), static_cast<float>(data.camera.y)};
    int tab = 0, speed = 1, frame_count = 0;
    StartupError last = StartupError::none;
    std::optional<ref::Position> selected_cell;
    // Explicit snapshot inspection, never called by default reset or a real input command.
    if (inspect_page == "roads" || inspect_page == "shops" || inspect_page == "food") {
        tab = inspect_page == "roads" ? 0 : inspect_page == "shops" ? 1 : 2;
        session.open_catalog();
    } else if (inspect_page == "arrival" || inspect_page == "visitor") {
        session.set_paused(false);
        for (int i = 0; i < 420; ++i)
            session.update();
        if (inspect_page == "visitor") {
            session.acknowledge_talk();
            session.acknowledge_talk();
            const auto cell = session.state().character->cell;
            camera = {30.0F * (cell.x + cell.y), 15.0F * (cell.y - cell.x) + 15};
            session.finish_camera();
            session.set_paused(true);
        }
    }
    while (!WindowShouldClose()) {
        const Vector2 mouse{GetMousePosition().x / scale, GetMousePosition().y / scale};
        const bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        const auto mode = session.state().mode;
        if (IsKeyPressed(KEY_ESCAPE)) {
            session.cancel();
            selected_cell.reset();
            last = StartupError::none;
        }
        if ((mode == StartupMode::normal || mode == StartupMode::placement) &&
            IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            camera.x -= GetMouseDelta().x / scale;
            camera.y += GetMouseDelta().y / scale;
        }
        std::optional<ref::Position> hovered;
        if (mouse.y > 38 && mouse.y < 294 &&
            (mode == StartupMode::normal || mode == StartupMode::placement))
            hovered = pick(mouse, camera);
        const auto hit = [&](Rectangle r) { return click && CheckCollisionPointRec(mouse, r); };
        if (mode == StartupMode::normal) {
            if (hit({2, 296, 74, 22}))
                last = session.open_catalog();
            if (hit({80, 296, 74, 22}))
                session.set_paused(!session.state().paused);
            if (hit({158, 296, 80, 22}))
                speed = speed == 1 ? 2 : 1;
        } else if (mode == StartupMode::catalog) {
            if (hit({190, 296, 48, 22})) {
                session.cancel();
                last = StartupError::none;
            }
            for (int i = 0; i < 3; ++i)
                if (hit({static_cast<float>(4 + i * 77), 58, 73, 25}))
                    tab = i;
            int row = 0;
            for (const auto &item : data.definitions) {
                if (item.tab != tab)
                    continue;
                if (hit({8, static_cast<float>(90 + row * 28), 224, 26}))
                    last = session.select(item.id);
                ++row;
                if (tab == 0 && row == 1) {
                    if (hit({8, static_cast<float>(90 + row * 28), 224, 26}))
                        last = session.select(-1);
                    ++row;
                }
            }
        } else if (mode == StartupMode::placement) {
            if (hit({160, 296, 78, 22})) {
                session.cancel();
                selected_cell.reset();
                last = StartupError::none;
            } else if (hit({80, 296, 76, 22}) && selected_cell) {
                last = session.confirm(*selected_cell);
                if (last == StartupError::none)
                    selected_cell.reset();
            } else if (click && hovered) {
                selected_cell = hovered;
                last = session.preview(*hovered);
            }
        } else if (mode == StartupMode::tutorial && hit({178, 267, 52, 23})) {
            last = session.acknowledge_talk();
        }
        // Camera duration is this view's convergence policy, not a recovered original frame count.
        if (session.state().mode == StartupMode::camera) {
            const auto cell = session.state().character->cell;
            const Vector2 target{30.0F * (cell.x + cell.y), 15.0F * (cell.y - cell.x) + 15};
            camera.x += (target.x - camera.x) * 0.15F;
            camera.y += (target.y - camera.y) * 0.15F;
            if (std::abs(target.x - camera.x) + std::abs(target.y - camera.y) < 0.5F) {
                camera = target;
                session.finish_camera();
            }
        } else
            session.update(speed);

        BeginTextureMode(canvas.value);
        ClearBackground(Color{83, 141, 77, 255});
        struct Tile {
            int record;
            int variant;
            Vector2 p;
            Color tint;
        };
        std::vector<Tile> tiles;
        for (int y = 0; y < data.height; ++y)
            for (int x = 0; x < data.width; ++x) {
                const auto index = static_cast<std::size_t>(y * data.width + x);
                const auto edit = session.state().terrain_edits.find(index);
                const auto source = data.cells[index];
                int record =
                    edit == session.state().terrain_edits.end() ? source.display_id : edit->second;
                int variant = edit == session.state().terrain_edits.end() ? source.variant : 0;
                const auto p = project({x, y}, camera);
                const auto facility = session.facility_at({x, y});
                if (facility) {
                    record = 27;
                    variant = 0;
                }
                tiles.push_back({record, variant, p, WHITE});
            }
        std::stable_sort(tiles.begin(), tiles.end(),
                         [](const auto &a, const auto &b) { return a.p.y < b.p.y; });
        for (const auto &tile : tiles)
            sprites.draw(session.display(tile.record).sprite, tile.variant, tile.p, tile.tint);
        tiles.clear();
        for (const auto &entry : session.state().facilities) {
            const auto &v = entry.second;
            tiles.push_back({session.definition(v.definition_id).display_id, 0,
                             project(v.cell, camera),
                             v.remaining_ticks ? Color{190, 190, 190, 255} : WHITE});
        }
        std::stable_sort(tiles.begin(), tiles.end(),
                         [](const auto &a, const auto &b) { return a.p.y < b.p.y; });
        for (const auto &tile : tiles)
            sprites.draw(session.display(tile.record).sprite, 0, tile.p, tile.tint);
        if (session.state().character)
            sprites.draw("walk00.seb", 0, project(session.state().character->cell, camera), WHITE,
                         SourceSprites::Binding::farmer);
        const auto preview_cell = selected_cell ? selected_cell : hovered;
        if (session.state().mode == StartupMode::placement && preview_cell) {
            const auto p = project(*preview_cell, camera);
            const Color color = session.preview(*preview_cell) == StartupError::none
                                    ? Color{255, 244, 71, 255}
                                    : RED;
            if (*session.state().selection > 0)
                sprites.draw(
                    session.display(session.definition(*session.state().selection).display_id)
                        .sprite,
                    0, p, Color{color.r, color.g, color.b, 180});
            DrawLineEx({p.x - 30, p.y}, {p.x, p.y - 15}, 1, color);
            DrawLineEx({p.x, p.y - 15}, {p.x + 30, p.y}, 1, color);
            DrawLineEx({p.x + 30, p.y}, {p.x, p.y + 15}, 1, color);
            DrawLineEx({p.x, p.y + 15}, {p.x - 30, p.y}, 1, color);
        }
        DrawRectangle(0, 0, 240, 38, paper);
        const auto &state = session.state();
        font.text(TextFormat("%d年 %d月", state.calendar[0] + 1, state.calendar[1] + 1), 4, 3);
        font.text(TextFormat("金币 %lld", static_cast<long long>(state.accounting.funds())), 116,
                  3);
        font.text(
            TextFormat("点数 %u  人气 %d", state.accounting.village_points(), state.popularity), 4,
            21);
        DrawRectangle(0, 294, 240, 26, paper);
        if (state.mode == StartupMode::normal) {
            font.text("建设", 18, 300);
            font.text(state.paused ? "继续" : "暂停", 100, 300);
            font.text(speed == 1 ? "1倍" : "2倍", 184, 300);
        } else if (state.mode == StartupMode::placement) {
            font.text(*state.selection == -1 ? "撤除" : session.definition(*state.selection).name,
                      4, 300);
            font.text("确定", 96, 300);
            font.text("返回", 184, 300);
        } else if (state.mode == StartupMode::month_end)
            font.text("月末", 100, 300);
        if (state.mode == StartupMode::catalog) {
            DrawRectangle(4, 48, 232, 232, paper);
            font.text("道路植物", 12, 64);
            font.text("商店", 98, 64);
            font.text("饮食", 177, 64);
            DrawRectangle(4 + tab * 77, 82, 73, 2, Color{69, 124, 107, 255});
            int row = 0;
            for (const auto &item : data.definitions) {
                if (item.tab != tab)
                    continue;
                const int y = 94 + row * 28;
                sprites.draw(session.display(item.display_id).sprite, 0,
                             {24, static_cast<float>(y + 15)}, WHITE, SourceSprites::Binding::map,
                             0.4F);
                font.text(item.name, 54, static_cast<float>(y));
                font.text(std::to_string(item.cost), 184, static_cast<float>(y));
                ++row;
                if (tab == 0 && row == 1) {
                    font.text("撤除", 54, static_cast<float>(94 + row * 28));
                    font.text("0", 184, static_cast<float>(94 + row * 28));
                    ++row;
                }
                DrawLine(8, 90 + row * 28, 232, 90 + row * 28, Color{216, 220, 218, 255});
            }
            font.text("返回", 194, 300);
        }
        if (state.mode == StartupMode::tutorial) {
            DrawRectangle(6, 202, 228, 90, paper);
            sprites.draw("chara_hishoko01.seb", 0, {22, 202}, WHITE,
                         SourceSprites::Binding::secretary);
            font.paragraph(data.first_talk[state.talk_line], 14, 211, 208);
            font.text("确定", 186, 273);
        }
        if (last != StartupError::none) {
            DrawRectangle(4, 274, 232, 18, paper);
            font.text(error_text(last), 10, 277, RED);
        }
        EndTextureMode();
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(canvas.value.texture, {0, 0, width, -height},
                       {0, 0, width * scale, height * scale}, {0, 0}, 0, WHITE);
        EndDrawing();
        ++frame_count;
        if (frames > 0 && frame_count >= frames) {
            if (screenshot) {
                if (!screenshot->parent_path().empty())
                    std::filesystem::create_directories(screenshot->parent_path());
                auto image = LoadImageFromScreen();
                if (!image.data)
                    throw std::runtime_error("窗口像素读取失败");
                const bool saved = ExportImage(image, screenshot->string().c_str());
                UnloadImage(image);
                if (!saved)
                    throw std::runtime_error("截图导出失败");
            }
            break;
        }
    }
    std::cout << "startup window closed: frames=" << frame_count
              << " mode=" << static_cast<int>(session.state().mode)
              << " steps=" << session.state().simulation_steps
              << " money=" << session.state().accounting.funds() << '\n';
    return 0;
}

void check_startup_world() {
    StartupSession initial;
    // 明确研究seed，不认证固定APK默认seed或捕获轨迹。
    StartupWorldRuntimeSession session(initial.state(), ref::WorldRandomStream::from_java_seed(1));
    const auto result = session.update();
    if (result.error != StartupWorldRuntimeError::none)
        throw std::runtime_error("共同世界新局首个更新失败");
    std::cout << "world check passed: state=" << session.state().scene.scene_state
              << " pages=" << session.state().scripts.pages.size()
              << " actors=" << session.state().scene.world.world.ai.human_order.size() << '\n';
}

int run_startup_world_window(const std::filesystem::path &assets,
                             const std::filesystem::path &font_path, bool paused, int frames,
                             const std::optional<std::filesystem::path> &screenshot,
                             const std::string &inspect_page,
                             const std::optional<std::filesystem::path> &load_file,
                             const std::optional<std::filesystem::path> &save_file) {
    Window window;
    Canvas canvas;
    SourceSprites sprites(assets / "original");
    const auto &rules = startup_world_rules();
    std::string glyphs = rules.script_sources.talks + rules.script_sources.news +
                         rules.script_sources.event_messages;
    for (const auto &human : rules.humans)
        glyphs += human.name;
    for (const auto &task : rules.tasks)
        glyphs += task.name + task.title;
    for (const auto &item : rules.items)
        glyphs += item.name;
    for (const auto &equipment : rules.equipment)
        glyphs += equipment.name;
    for (const auto &job : rules.jobs)
        glyphs += job.name;
    for (const auto &activity : rules.activities)
        glyphs += activity.name + activity.detail + activity.description;
    for (const auto &recipe : rules.magic_pot_recipes)
        glyphs += recipe.name;
    glyphs += "魔法壶投入配方开发暗相性似乎不错成功感觉就那样吧嗯";
    glyphs += "菜单冒险情报系统任务进度中止任务赠送礼物保存纪录配置排行榜结束游戏输入未接入"
              "研究适配页签设施列表持有物品装备一览村情报";
    glyphs += "村办季度剩余次数村子点开展活动完成等待尚未接入获得奖励金币配置更替确认领取设备一般";
    glyphs += "体力力量灵活结实魔力运气";
    glyphs += "周围设施的奖励没有";
    glyphs += "南瓜商会购买出售持有剩余价格道具使用强化反应赠送多谢惠顾免费建设返回";
    glyphs += "道路移动撤除旋转请选择起点终点未开放不可操作街道内地域商品种类装饰信息口碑关闭继续";
    glyphs += "任务列表征集队伍征集费出发追加取消候选队伍评价休息成果商店追加"
              "年度贡献勋章授予终止是非满足努力能力上升自宅完成设施升级城镇等级晋级申请"
              "月收入设施数居住数指定建设任务完成数街道人气举办活动达成未暂不可用"
              "概况属性装备魔法体力力量灵活结实魔力运气攻击防御经验职业大师转职"
              "武器防具饰品装备礼物居民税收合计库存可用学会火冰雷恢复营业施工使用支出加成";
    // 只在接管前存在旧启动快照；runtime构造后释放，禁止两个可写世界并存。
    std::unique_ptr<StartupSession> initial = std::make_unique<StartupSession>();
    if (inspect_page == "visitor") {
        for (int n = 0; n < 420; ++n)
            initial->update();
    }
    StartupWorldRuntimeSession session(initial->state(), ref::WorldRandomStream::from_java_seed(1));
    initial.reset();
    StartupWorldSaveMetadata file_metadata;
    if (load_file) {
        auto loaded = load_startup_world_file(*load_file, rules, StartupWorldSavePurpose::normal);
        if (!loaded.snapshot) throw std::runtime_error("读取失败：" + loaded.error);
        file_metadata = std::move(loaded.snapshot->metadata);
        session = std::move(loaded.snapshot->session);
    }
    const auto human_name = [&session](int id) {
        const auto profile = startup_world_human_profile(session.state(), id);
        if (!profile)
            throw std::runtime_error("窗口人物资料非法");
        return profile->name;
    };
    for (const auto &human : rules.humans)
        glyphs += human_name(human.identity); // 原字体目录外的自定义中文在载入后一次装入。
    ChineseFont font(font_path, glyphs);
    const auto available_road = [](const StartupWorldRuntimeState &s) -> std::optional<int> {
        for (const auto &d : s.rules->facilities) {
            const auto presence = s.facility_presence.find(d.id);
            if (d.kind == 6 && (d.flags & 4) && presence != s.facility_presence.end() &&
                presence->second != 0)
                return d.id;
        }
        return {};
    };
    if (inspect_page == "world-menu" || inspect_page == "world-system" || inspect_page == "world-information") {
        // 有界验收只使用真实入口与输入，不注入选择、页计数、资金或随机。
        if (session.open_main_menu()!=StartupWorldRuntimeError::none)
            throw std::runtime_error("导航检查主菜单入口失败");
        for(int step=0;step<3;++step)
            if(!session.update().candidate)throw std::runtime_error("导航检查主菜单初始化失败");
        if(inspect_page!="world-menu") {
            const auto id=session.state().scripts.pages.back().id;
            const auto view=inspect_startup_world_menu_page(session.state(),id);
            if(!view)throw std::runtime_error("导航检查主菜单投影失败");
            const int tag=inspect_page=="world-system"?6:5;
            const auto entry=std::find(view->tags.begin(),view->tags.end(),tag);
            if(entry==view->tags.end())throw std::runtime_error("导航检查所需标签不存在");
            StartupWorldMenuInput selection;selection.select_row=static_cast<int>(entry-view->tags.begin());
            StartupWorldMenuInput confirm;confirm.confirm=true;
            if(session.input_menu_page(id,selection)!=StartupWorldRuntimeError::none ||
               session.input_menu_page(id,confirm)!=StartupWorldRuntimeError::none)
                throw std::runtime_error("导航检查实际选行确认失败");
            for(int step=0;step<3;++step)
                if(!session.update().candidate)throw std::runtime_error("导航检查子菜单初始化失败");
        }
        session.take_audio_requests();
    } else if (inspect_page == "world-editing") {
        const auto road = available_road(session.state());
        if (!road)
            throw std::runtime_error("真实新局没有已开放道路定义");
        const auto begun = session.begin_road(*road);
        if (begun.error != StartupWorldRuntimeError::none ||
            begun.denial != StartupBuildDenial::none)
            throw std::runtime_error("真实新局道路入口被拒绝");
        // 只选择原地图已有道路上的合法起点，不写地表、不付费、不注入坐标或世界字段。
        const auto map = session.state().scene.world.world.map;
        for (std::size_t n = 0; n < map.cells.size(); ++n) {
            if (map.cells[n].legacy_state != 3)
                continue;
            const auto result = session.confirm_edit(
                {static_cast<int>(n) % map.width, static_cast<int>(n) / map.width},
                ref::FacilityOrientation::first);
            if (result.error != StartupWorldRuntimeError::none)
                throw std::runtime_error("道路起点检查事务失败");
            if (result.denial == StartupBuildDenial::none && session.state().build_mode == 2)
                break;
        }
        if (session.state().build_mode != 2)
            throw std::runtime_error("原地图没有可检查的道路起点");
    } else if (inspect_page == "world-commerce" || inspect_page == "world-item-gift") {
        bool reached{}, purchased{};
        std::string last_command = "尚未派发输入";
        std::optional<std::pair<std::uint64_t, int>> previous_page;
        const auto dump_pages =
            [&](const StartupWorldRuntimeState &s) {
                std::cerr << "商会诊断 last_command=" << last_command << " purchased=" << purchased
                          << " scene=" << s.scene.scene_state
                          << " funds=" << s.scene.world.world.ai.accounting.funds()
                          << " draws=" << s.scene.random.draws()
                          << " flags=" << s.scripts.user_flags << std::endl;
                for (const auto &p : s.scripts.pages) {
                    const auto counter = s.page_counters.find(p.id);
                    std::cerr << " page id=" << p.id << " raw=" << p.legacy_page
                              << " life=" << p.lifecycle << " kind=" << static_cast<int>(p.kind)
                              << " source=" << p.source_record << " f/r/s/g=" << p.legacy_f << '/'
                              << p.legacy_r << '/' << p.legacy_s << '/' << p.legacy_g << " counter="
                              << (counter == s.page_counters.end() ? -1 : counter->second)
                              << " initialized(commerce/human/facility)="
                              << s.commerce_pages_initialized.count(p.id) << '/'
                              << s.human_pages_initialized.count(p.id) << '/'
                              << s.facility_item_pages_initialized.count(p.id);
                    if (const auto data = s.commerce_page_data.find(p.id);
                        data != s.commerce_page_data.end()) {
                        std::cerr << " commerce=";
                        for (int v : data->second)
                            std::cerr << v << ',';
                    }
                    if (const auto list = s.commerce_page_lists.find(p.id);
                        list != s.commerce_page_lists.end()) {
                        std::cerr << " list=";
                        for (int id : list->second) {
                            const auto stock = s.shop_item_stock.find(id);
                            const auto owned = s.items.find(id);
                            std::cerr
                                << id << "(A="
                                << (stock == s.shop_item_stock.end() ? -1 : stock->second.quantity)
                                << ",z=" << (owned == s.items.end() ? -1 : owned->second.inventory)
                                << "),";
                        }
                    }
                    std::cerr << std::endl;
                }
                for (const auto &[id, item] : s.items) {
                    const auto c = s.catalog.find({0, id});
                    if (c == s.catalog.end() || c->second.inventory != item.inventory ||
                        c->second.status != item.status ||
                        c->second.unlock_counter != item.unlock_counter ||
                        c->second.newly_unlocked != item.newly_unlocked)
                        std::cerr << " item镜像不一致 id=" << id << " z/p/q/r=" << item.inventory
                                  << '/' << item.status << '/' << item.unlock_counter << '/'
                                  << item.newly_unlocked << std::endl;
                }
            };
        const auto command = [&](const char *label, const auto &action) {
            last_command = label;
            return action();
        };
        const auto affordable_stock = [](const StartupWorldRuntimeState &s) {
            return std::any_of(s.rules->items.begin(), s.rules->items.end(), [&](const auto &item) {
                const auto stock = s.shop_item_stock.find(item.identity);
                return stock != s.shop_item_stock.end() && stock->second.quantity > 0 &&
                       item.commerce_price <= s.scene.world.world.ai.accounting.funds();
            });
        };
        // 有界玩家策略：真实任务成功/开放/补货后主动进商会，不重复接任务来等待延迟83。
        for (int step = 0; step < 20000 && !reached; ++step) {
            const auto update = session.update();
            if (!update.candidate) {
                std::cerr << "商会更新失败 step=" << step
                          << " error=" << static_cast<int>(update.error)
                          << " scene_error=" << static_cast<int>(update.scene_error)
                          << " world_error=" << static_cast<int>(update.world_error) << std::endl;
                dump_pages(session.state());
                throw std::runtime_error("自然商会检查推进失败，step=" + std::to_string(step));
            }
            const auto &s = session.state();
            const auto it = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                         [](const auto &p) { return p.lifecycle != 4; });
            if (it == s.scripts.pages.rend())
                throw std::runtime_error("自然商会检查页栈为空");
            const auto p = *it;
            const int raw = p.legacy_page;
            if (!previous_page || *previous_page != std::make_pair(p.id, raw)) {
                std::cout << "商会切页 step=" << step << " id=" << p.id << " raw=" << raw
                          << " life=" << p.lifecycle << " source=" << p.source_record
                          << " last_command=" << last_command << std::endl;
                previous_page = std::make_pair(p.id, raw);
            }
            auto error = StartupWorldRuntimeError::none;
            if (p.kind == ref::WorldScriptPageKind::scene) {
                if (purchased && s.scene.scene_state == 0 &&
                    !s.scene.world.world.ai.human_order.empty()) {
                    const auto actor = s.scene.world.world.ai.human_order.front();
                    error = command("打开实际人物详情", [&] {
                        return session.open_human_page(
                            s.scene.world.world.ai.battle.actors.at(actor).definition);
                    });
                } else if (!purchased && s.scene.scene_state == 0 && !s.active_task) {
                    if (s.task_progress.successes > 0 && (s.scripts.user_flags & 16U) != 0) {
                        // 若实际首次H()没有补出可负担货物，只继续经营等原月度补货。
                        if (affordable_stock(s))
                            error = command("主动打开已开放商会",
                                            [&] { return session.open_commerce(); });
                    } else if (!s.task_order.empty())
                        error = command("打开任务目录", [&] { return session.open_task_menu(); });
                }
            } else if (raw == 83) {
                if (inspect_startup_world_commerce_page(s, p.id)) {
                    if (inspect_page == "world-commerce")
                        reached = true;
                    else
                        error = command(purchased ? "取消商会83" : "商会83打开购买目录", [&] {
                            return session.act_commerce_page(
                                p.id, purchased ? StartupCommerceAction::cancel
                                                : StartupCommerceAction::confirm);
                        });
                }
            } else if (raw == 84) {
                const auto view = inspect_startup_world_commerce_page(s, p.id);
                if (!view)
                    throw std::runtime_error("自然商会目录缺载荷");
                if (purchased)
                    error = command("取消已购买目录84", [&] {
                        return session.act_commerce_page(p.id, StartupCommerceAction::cancel);
                    });
                else {
                    const auto affordable =
                        std::find_if(view->entries.begin(), view->entries.end(), [&](int id) {
                            return s.rules->items.at(id).commerce_price <=
                                   s.scene.world.world.ai.accounting.funds();
                        });
                    if (affordable == view->entries.end())
                        throw std::runtime_error("自然商会没有可负担道具");
                    error = command("选择可负担道具84", [&] {
                        return session.act_commerce_page(
                            p.id, StartupCommerceAction::select,
                            static_cast<int>(affordable - view->entries.begin()));
                    });
                    if (error == StartupWorldRuntimeError::none)
                        error = command("确认购买道具84", [&] {
                            return session.act_commerce_page(p.id, StartupCommerceAction::confirm);
                        });
                    purchased = error == StartupWorldRuntimeError::none;
                }
            } else if (raw == 60 && purchased && startup_world_human_page_ready(s, p.id))
                error = command("人物60打开礼物目录64", [&] {
                    return session.act_human_page(p.id, StartupHumanPageAction::gifts);
                });
            else if (raw == 64 && purchased && startup_world_human_page_ready(s, p.id)) {
                error = command("人物64选择第五道具页", [&] {
                    return session.act_human_page(p.id, StartupHumanPageAction::equipment_slot, 4);
                });
                reached = error == StartupWorldRuntimeError::none;
            } else if (raw == 22 || raw == 23)
                error = command("确认任务22/23", [&] {
                    return session.act_task_page(p.id, StartupWorldTaskAction::confirm).error;
                });
            else if (raw == 25)
                error = command("任务25出发", [&] {
                    return session.act_task_page(p.id, StartupWorldTaskAction::depart).error;
                });
            else if (raw == 28) {
                if (s.page_phases.at(p.id) == 0)
                    error = command("确认任务28演出", [&] {
                        return session.act_task_page(p.id, StartupWorldTaskAction::confirm).error;
                    });
            } else if (raw == 33)
                error = command("任务33续期或中止", [&] {
                    return session
                        .act_task_page(p.id, StartupWorldTaskAction::confirm,
                                       s.scene.world.world.ai.accounting.funds() >= p.legacy_f ? 0
                                                                                               : 1)
                        .error;
                });
            else if (raw == 87) {
                last_command = "年度87请求结束并确认实际问答";
                // 明确的诊断玩家选择结束授勋；保留尚未使用的勋章，不伪装成通用确认。
                const auto pending = s.award_termination_pending.find(p.id);
                if (pending != s.award_termination_pending.end() && pending->second)
                    error =
                        session.act_award_page(p.id, ref::WorldAwardAction::confirm_termination);
                else {
                    error =
                        session.act_award_page(p.id, ref::WorldAwardAction::request_termination);
                    if (error == StartupWorldRuntimeError::none) {
                        const auto question = session.state().award_termination_pending.find(p.id);
                        // 首轮说明可能先返回；只在实际生成了终止问答后提交确认。
                        if (question != session.state().award_termination_pending.end() &&
                            question->second)
                            error = session.act_award_page(
                                p.id, ref::WorldAwardAction::confirm_termination);
                    }
                }
            } else if (raw == 48)
                error =
                    command("返回晋级条件48", [&] { return session.act_rank_page(p.id, 0, true); });
            else if (raw != 16 && raw != 24 && raw != 56 && raw != 57 && raw != 86 && raw != 97 &&
                     raw != 98)
                error = command("确认当前已有页面消费者",
                                [&] { return session.acknowledge_page(p.id); });
            if (error != StartupWorldRuntimeError::none) {
                std::cerr << "商会命令失败 step=" << step << " error=" << static_cast<int>(error)
                          << std::endl;
                dump_pages(session.state());
                throw std::runtime_error("自然商会检查消费者失败，raw=" + std::to_string(raw));
            }
            (void)session
                .take_sound_requests(); // 有界预运行也逐轮消费输出，避免把未播放声音误当状态历史。
            if (step % 1000 == 0) {
                const auto &current = session.state();
                const auto stocks =
                    std::count_if(current.shop_item_stock.begin(), current.shop_item_stock.end(),
                                  [](const auto &entry) { return entry.second.quantity > 0; });
                std::cout << "商会预运行 step=" << step << " raw=" << raw
                          << " success=" << current.task_progress.successes
                          << " flags16=" << ((current.scripts.user_flags & 16U) != 0)
                          << " stocks=" << stocks << " affordable=" << affordable_stock(current)
                          << " funds=" << current.scene.world.world.ai.accounting.funds()
                          << std::endl;
            }
        }
        if (!reached)
            throw std::runtime_error("自然商会检查超过有界预算");
    } else if (inspect_page == "world-activities") {
        if (session.open_village_activities() != StartupWorldRuntimeError::none)
            throw std::runtime_error("真实村办目录检查入口失败");
        bool ready{};
        for (int n = 0; n < 100 && !ready; ++n) {
            if (!session.update().candidate)
                throw std::runtime_error("村办首次说明推进失败");
            const auto p = session.state().scripts.pages.back();
            ready = p.legacy_page == 51 &&
                    inspect_startup_world_village_activity_page(session.state(), p.id).has_value();
            if (!ready && p.kind == ref::WorldScriptPageKind::dialogue &&
                session.acknowledge_page(p.id) != StartupWorldRuntimeError::none)
                throw std::runtime_error("村办首次说明确认失败");
        }
        if (!ready)
            throw std::runtime_error("村办检查未返回真实目录");
    } else if (inspect_page == "world-facility-bonuses") {
        // 仅使用真实新局已装入实例及实际邻接来源，不注入来源名单或示例加成。
        std::optional<std::uint64_t> target;
        for (const auto id : session.state().scene.world.facility_order) {
            const auto &neighbours = session.state().neighbourhood_details.at(id);
            if (!neighbours.sources.empty() &&
                session.state().scene.world.world.facilities.at(id).status != 0) {
                target = id;
                break;
            }
        }
        if (!target || session.open_facility_page(*target) != StartupWorldRuntimeError::none)
            throw std::runtime_error("真实新局无可核设施奖励详情");
        const auto page = session.state().scripts.pages.back().id;
        if (session.act_facility_page(page, StartupFacilityPageAction::next) !=
                StartupWorldRuntimeError::none ||
            !startup_world_facility_bonus_rows(session.state(), page))
            throw std::runtime_error("真实设施第二页载荷拒绝");
    } else if (inspect_page == "world-building") {
        if (session.open_build_menu() != StartupWorldRuntimeError::none)
            throw std::runtime_error("真实新局建设目录检查入口失败");
    } else if (inspect_page == "world-goods" || inspect_page == "world-equipment-info") {
        // 真实新局已有武器设施优先；没有时按当前建设/施工消费者形成，不注入实例或资金。
        std::optional<std::uint64_t> shop;
        const auto find_shop = [&]() {
            for (auto id : session.state().scene.world.facility_order)
                if (session.state().scene.world.world.facilities.at(id).placement.definition_id == 30 &&
                    session.state().scene.world.world.facilities.at(id).status != 0)
                    shop = id;
        };
        find_shop();
        if (!shop) {
            const auto started = session.begin_build(30);
            if (started.error != StartupWorldRuntimeError::none || started.denial != StartupBuildDenial::none)
                throw std::runtime_error("自然商品检查武器店建设资格不足");
            for (int y = 0; y < 24 && !shop; ++y)
                for (int x = 0; x < 24 && !shop; ++x) {
                    const auto built = session.confirm_build({x, y}, ref::FacilityOrientation::first);
                    if (built.created) shop = built.created;
                }
            if (!shop || session.cancel_build() != StartupWorldRuntimeError::none)
                throw std::runtime_error("自然商品检查未形成合法建设");
            for (int n = 0; n < 2000 && session.state().scene.world.world.facilities.at(*shop).status == 0; ++n) {
                if (!session.update().candidate) throw std::runtime_error("自然商品检查施工推进失败");
                const auto p = session.state().scripts.pages.back();
                if (p.kind != ref::WorldScriptPageKind::scene &&
                    session.acknowledge_page(p.id) != StartupWorldRuntimeError::none)
                    throw std::runtime_error("自然商品检查前置页面确认失败");
                (void)session.take_sound_requests();
            }
        }
        if (session.open_facility_page(*shop) != StartupWorldRuntimeError::none)
            throw std::runtime_error("真实武器店74打开失败");
        const auto parent = session.state().scripts.pages.back().id;
        if (session.act_facility_page(parent, StartupFacilityPageAction::confirm) != StartupWorldRuntimeError::none)
            throw std::runtime_error("真实武器店74商品选择失败");
        const auto goods = session.state().scripts.pages.back().id;
        if (!inspect_startup_world_facility_catalog_page(session.state(), goods))
            throw std::runtime_error("自然79检查未形成合法目录");
        if (inspect_page == "world-equipment-info") {
            if (session.act_facility_catalog_page(goods, StartupFacilityCatalogAction::inspect) != StartupWorldRuntimeError::none ||
                !session.update().candidate)
                throw std::runtime_error("自然79信息72检查失败");
        }
    } else if (inspect_page == "world-details") {
        if (session.open_facility_page(4) != StartupWorldRuntimeError::none)
            throw std::runtime_error("真实初局旅店详情检查入口失败");
    } else if (inspect_page == "world-month" || inspect_page == "world-active" ||
               inspect_page == "task-team" || inspect_page == "world-award" ||
               inspect_page == "world-human") {
        bool reached{};
        // 显式窗口检查策略：只给真实页栈逐轮确认，不改人物/日期/随机/资金。
        for (int step = 0; step < (inspect_page == "world-award" ? 50000 : 20000); ++step) {
            const auto result = session.update();
            if (!result.candidate)
                throw std::runtime_error("共同世界检查预运行失败，step=" + std::to_string(step));
            const auto &s = session.state();
            const auto top = s.scripts.pages.back();
            if (inspect_page == "world-award" && top.lifecycle != 4 &&
                top.kind == ref::WorldScriptPageKind::raw_page && top.legacy_page == 87) {
                reached = true;
                break;
            }
            if (top.kind != ref::WorldScriptPageKind::scene && top.lifecycle != 4 &&
                !(top.kind == ref::WorldScriptPageKind::raw_page &&
                  (top.legacy_page == 56 || top.legacy_page == 57 || top.legacy_page == 16 ||
                   top.legacy_page == 97 || top.legacy_page == 98))) {
                auto error = StartupWorldRuntimeError::none;
                if (inspect_page == "task-team" && (top.legacy_page == 22 || top.legacy_page == 23))
                    error = session.act_task_page(top.id, StartupWorldTaskAction::confirm).error;
                else if (top.legacy_page != 24 && top.legacy_page != 25 && top.legacy_page != 28)
                    error = session.acknowledge_page(top.id);
                if (error != StartupWorldRuntimeError::none)
                    throw std::runtime_error("共同世界检查预运行页面消费者失败");
            }
            if (inspect_page == "task-team" && top.kind == ref::WorldScriptPageKind::scene &&
                !session.state().task_order.empty() && !session.state().active_task &&
                session.open_task_menu() != StartupWorldRuntimeError::none)
                throw std::runtime_error("任务检查预运行菜单入口失败");
            const auto &current = session.state();
            if (inspect_page == "world-human" && top.kind == ref::WorldScriptPageKind::scene &&
                top.lifecycle != 4 && current.scene.scene_state == 0 &&
                !current.scene.world.world.ai.human_order.empty()) {
                const int human = current.scene.world.world.ai.battle.actors
                                      .at(current.scene.world.world.ai.human_order.front())
                                      .definition;
                if (session.open_human_page(human) != StartupWorldRuntimeError::none)
                    throw std::runtime_error("自然人物详情检查入口失败");
                for (int n = 0; n < 100; ++n) {
                    if (!session.update().candidate)
                        throw std::runtime_error("人物详情首次说明推进失败");
                    const auto current_page = session.state().scripts.pages.back();
                    if (current_page.legacy_page == 60 && current_page.lifecycle != 4 &&
                        session.state().human_pages_initialized.count(current_page.id))
                        break;
                    if (current_page.kind == ref::WorldScriptPageKind::dialogue &&
                        session.acknowledge_page(current_page.id) != StartupWorldRuntimeError::none)
                        throw std::runtime_error("人物详情首次说明确认失败");
                    if (n == 99)
                        throw std::runtime_error("人物首次说明有界检查未返回详情");
                }
                reached = true;
                break;
            }
            reached = inspect_page == "world-month" ? current.report_state != 0
                      : inspect_page == "task-team"
                          ? current.scripts.pages.back().legacy_page == 25
                          : inspect_page == "world-active" &&
                                current.scene.world.world.ai.human_order.size() >= 3;
            if (reached)
                break;
        }
        if (!reached)
            throw std::runtime_error("共同世界有界检查未到达真实目标状态");
    }
    if (inspect_page == "world-magic-pot") {
        if (session.open_magic_pot(StartupMagicPotEntry::main_menu) != StartupWorldRuntimeError::none)
            throw std::runtime_error("魔法壶诊断需要已合法解锁的稳定世界文件");
        // 只驱动已证说明/真实初始化，不注入解锁、点数、库存或页面载荷。
        bool ready{};
        for (int n = 0; n < 600; ++n) {
            if (!session.update().candidate) throw std::runtime_error("魔法壶有界入口更新失败");
            const auto p = std::find_if(session.state().scripts.pages.rbegin(), session.state().scripts.pages.rend(),
                [](const auto &page) { return page.lifecycle != 4; });
            if (p == session.state().scripts.pages.rend()) throw std::runtime_error("魔法壶入口空栈");
            if (p->kind == ref::WorldScriptPageKind::raw_page && p->legacy_page == 41 &&
                inspect_startup_world_magic_pot_page(session.state(), p->id)) { ready = true; break; }
            if ((p->kind == ref::WorldScriptPageKind::dialogue || p->kind == ref::WorldScriptPageKind::simple_message) &&
                session.acknowledge_page(p->id) != StartupWorldRuntimeError::none)
                throw std::runtime_error("魔法壶说明输入失败");
            session.take_sound_requests();
        }
        if (!ready) throw std::runtime_error("魔法壶有界诊断未到达41");
        session.take_sound_requests();
    }
    if (!load_file || paused)
        session.set_paused(paused);
    ref::WorldRenderClock clock;
    Vector2 camera{static_cast<float>(startup_evidence().camera.x),
                   static_cast<float>(startup_evidence().camera.y)};
    Vector2 view_offset{};
    int frame_count{}, paragraph_index{};
    int moved_frames{};
    int confirmation_inputs{}, pause_inputs{}, speed_inputs{};
    std::size_t world_pixel_colors{};
    std::map<ref::CharacterId, ref::CombatPoint> previous_positions;
    float body_scroll{}, body_extent{};
    std::uint64_t viewed_page{};
    int task_selection{}, task_scroll{};
    int award_selection{}, prompt_selection{}, rank_selection{};
    int build_tab{}, build_selection{};
    int bonus_selection{}, bonus_scroll{}; // 原5行视窗的研究输入适配，不进入Owner或存档。
    auto build_orientation = ref::FacilityOrientation::first;
    std::string command_feedback;
    while (!WindowShouldClose()) {
        const Vector2 mouse{GetMousePosition().x / scale, GetMousePosition().y / scale};
        const bool pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        const auto hit = [&](Rectangle r) { return pressed && CheckCollisionPointRec(mouse, r); };
        if (hit({4, 295, 44, 22})) {
            ++pause_inputs;
            session.set_paused(!session.state().scene.framework_paused);
        }
        if (hit({176, 295, 58, 22})) {
            ++speed_inputs;
            session.set_speed(session.state().scene.speed_setting == 1 ? 0 : 1);
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            const auto delta = GetMouseDelta();
            view_offset.x -= delta.x / scale;
            view_offset.y += delta.y / scale;
        }
        const auto top_page = [&]() -> const ref::WorldScriptPage * {
            const auto &pages = session.state().scripts.pages;
            for (auto it = pages.rbegin(); it != pages.rend(); ++it)
                if (it->kind != ref::WorldScriptPageKind::scene && it->lifecycle != 4)
                    return &*it;
            return nullptr;
        };
        // 暂停时不派发页面输入，避免把合法的暂停拒绝误报成窗口消费者故障。
        if (const auto *page = top_page(); page && !session.state().scene.framework_paused) {
            if (viewed_page != page->id) {
                viewed_page = page->id;
                paragraph_index = 0;
                body_scroll = body_extent = 0;
                task_selection = task_scroll = 0;
                build_tab = build_selection = 0;
                bonus_selection = bonus_scroll = 0;
                award_selection = prompt_selection = 0;
                if (page->legacy_page == 1 && session.state().task_abort_questions.count(page->id))
                    prompt_selection = 1; // 原主动中止询问默认“否”，不因新页重置变成“是”。
                rank_selection = 0;
                command_feedback.clear();
            }
            const int raw = page->legacy_page;
            const bool task_page = raw >= 22 && raw <= 28;
            if (navigation_menu(raw)) {
                const auto plan=window_navigation_plan(session.state(),page->id,font,true);
                if (plan) {
                    StartupWorldMenuInput input;
                    input.up=IsKeyPressed(KEY_UP);input.down=IsKeyPressed(KEY_DOWN);
                    input.left=IsKeyPressed(KEY_LEFT);input.right=IsKeyPressed(KEY_RIGHT);
                    input.confirm=IsKeyPressed(KEY_ENTER)||hit({166,258,66,22});
                    input.cancel=IsKeyPressed(KEY_ESCAPE)||hit({8,258,62,22});
                    // 维护鼠标短适配：单击选择、Enter确认；不冒认Steam物理ENTER/UP合成已认证。
                    if (!input.up&&!input.down&&!input.left&&!input.right&&!input.confirm&&!input.cancel)
                        for(const auto &touch:plan->touches) if(touch.rectangle&&(touch.component==8||touch.component==9)) {
                            const auto &r=*touch.rectangle;
                            if(hit({float(plan->origin[0]+r[0]),float(plan->origin[1]+r[1]),float(r[2]),float(r[3])}))
                                input.select_row=touch.value&0xffff;
                        }
                    if(input.up||input.down||input.left||input.right||input.confirm||input.cancel||input.select_row) {
                        const auto error=session.input_menu_page(page->id,input);
                        if(error==StartupWorldRuntimeError::none)command_feedback.clear();
                        else command_feedback="输入未接入"; // raw10平台动作显式拒绝，窗口保留现场。
                    }
                }
            } else if (raw==9 || (raw>=34 && raw<=40)) {
                const auto view=inspect_startup_world_information_page(session.state(),page->id);
                if(view) {
                    StartupInformationInput input;
                    input.up=IsKeyPressed(KEY_UP);input.down=IsKeyPressed(KEY_DOWN);
                    input.left=IsKeyPressed(KEY_LEFT);input.right=IsKeyPressed(KEY_RIGHT);
                    input.confirm=IsKeyPressed(KEY_ENTER)||hit({166,258,66,22});
                    input.cancel=IsKeyPressed(KEY_ESCAPE)||hit({8,258,62,22});
                    if(raw==9 && !input.up&&!input.down&&!input.left&&!input.right&&!input.confirm&&!input.cancel)
                        if(const auto plan=window_navigation_plan(session.state(),page->id,font,true))
                            for(const auto &touch:plan->touches)if(touch.component==9 && touch.rectangle) {
                                const auto &r=*touch.rectangle;
                                if(hit({float(plan->origin[0]+r[0]),float(plan->origin[1]+r[1]),float(r[2]),float(r[3])}))
                                    input.select_row=touch.value&0xffff;
                            }
                    if(raw!=9 && raw!=34 && raw!=36 &&
                       !input.up&&!input.down&&!input.left&&!input.right&&!input.confirm&&!input.cancel) {
                        const auto count=view->humans?view->humans->size():view->items?view->items->size():
                            view->equipment?view->equipment->rows.size():view->facilities?view->facilities->size():0U;
                        for(int row=view->first_visible;row<static_cast<int>(count)&&row<view->first_visible+5;++row)
                            if(hit({10,float(86+28*(row-view->first_visible)),220,25}))input.select_row=row;
                    }
                    if(input.up||input.down||input.left||input.right||input.confirm||input.cancel||input.select_row) {
                        const auto error=session.input_information_page(page->id,input);
                        command_feedback=error==StartupWorldRuntimeError::none?"":"输入未接入";
                    }
                }
            } else if (raw >= 41 && raw <= 47) {
                const auto view = inspect_startup_world_magic_pot_page(session.state(), page->id);
                if (view) {
                    using A = StartupMagicPotAction;
                    std::optional<A> action;
                    int selected{};
                    if (raw <= 43) {
                        if (IsKeyPressed(KEY_UP)) action = A::previous;
                        if (IsKeyPressed(KEY_DOWN)) action = A::next;
                        const int count = raw == 41 ? 2 : static_cast<int>(view->entries.size());
                        for (int n = view->first_visible; n < std::min(count, view->first_visible + 5); ++n)
                            if (hit({12, 74.F + (n - view->first_visible) * 27, 216, 26})) {
                                action = A::select;
                                selected = n;
                            }
                    }
                    if (raw == 43) {
                        if (IsKeyPressed(KEY_LEFT)) action = A::previous_tab;
                        if (IsKeyPressed(KEY_RIGHT)) action = A::next_tab;
                    }
                    if (raw != 45 && (IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24}))) action = A::cancel;
                    if (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24})) action = A::confirm;
                    if (action && session.act_magic_pot_page(page->id, *action, selected) != StartupWorldRuntimeError::none)
                        throw std::runtime_error("魔法壶输入事务失败");
                }
            } else if (raw == 72 || raw == 79 || raw == 82) {
                const auto view = inspect_startup_world_facility_catalog_page(session.state(), page->id);
                if (view) {
                    using A = StartupFacilityCatalogAction;
                    std::optional<A> action;
                    int selected{};
                    if (raw == 79) {
                        if (IsKeyPressed(KEY_UP)) action = A::previous;
                        if (IsKeyPressed(KEY_DOWN)) action = A::next;
                        for (int n = view->first_visible;
                             n < std::min(static_cast<int>(view->entries.size()), view->first_visible + 4); ++n)
                            if (hit({12, 74.F + (n - view->first_visible) * 27, 216, 26})) {
                                action = A::select;
                                selected = n;
                            }
                        if (IsKeyPressed(KEY_I) || hit({88, 265, 66, 24})) action = A::inspect;
                    }
                    if (raw != 82) {
                        if (IsKeyPressed(KEY_LEFT) || hit({8, 240, 30, 20})) action = A::previous_tab;
                        if (IsKeyPressed(KEY_RIGHT) || hit({202, 240, 30, 20})) action = A::next_tab;
                    }
                    if (IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24})) action = A::cancel;
                    if (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24})) action = A::confirm;
                    if (action && session.act_facility_catalog_page(page->id, *action, selected) != StartupWorldRuntimeError::none)
                        throw std::runtime_error("设施商品/演出输入事务失败");
                }
            } else if ((raw >= 83 && raw <= 86) || raw == 93) {
                const auto view = inspect_startup_world_commerce_page(session.state(), page->id);
                if (view) {
                    using A = StartupCommerceAction;
                    std::optional<A> action;
                    int selected{};
                    if (raw <= 85) {
                        if (IsKeyPressed(KEY_UP))
                            action = A::previous;
                        if (IsKeyPressed(KEY_DOWN))
                            action = A::next;
                        const int count = raw == 83 ? 3 : static_cast<int>(view->entries.size());
                        const int rows = raw == 84 ? 5 : 3;
                        for (int n = view->first_visible;
                             n < std::min(count, view->first_visible + rows); ++n)
                            if (hit({12, 74.F + (n - view->first_visible) * 27, 216, 26})) {
                                action = A::select;
                                selected = n;
                            }
                        if (IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24}))
                            action = A::cancel;
                    }
                    if (raw == 84 && view->mode == 0) {
                        if (IsKeyPressed(KEY_LEFT))
                            action = A::previous_tab;
                        if (IsKeyPressed(KEY_RIGHT) || hit({148, 45, 80, 22}))
                            action = A::next_tab;
                    }
                    if (raw == 85 && (IsKeyPressed(KEY_I) || hit({96, 265, 52, 24})))
                        action = A::inspect;
                    if (raw != 86 && (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24})))
                        action = A::confirm;
                    if (action && session.act_commerce_page(page->id, *action, selected) !=
                                      StartupWorldRuntimeError::none)
                        throw std::runtime_error("商会输入事务失败");
                }
            } else if (raw >= 75 && raw <= 77) {
                if (session.state().facility_item_pages_initialized.count(page->id) &&
                    valid_startup_world_facility_item_page(session.state(), *page)) {
                    using A = StartupFacilityItemAction;
                    std::optional<A> action;
                    int selected{};
                    if (raw == 75) {
                        const auto &list = session.state().facility_item_page_lists.at(page->id);
                        const int first = std::max(
                            0, session.state().facility_item_page_selections.at(page->id) - 4);
                        if (IsKeyPressed(KEY_UP))
                            action = A::previous;
                        if (IsKeyPressed(KEY_DOWN))
                            action = A::next;
                        for (int n = first; n < std::min(static_cast<int>(list.size()), first + 5);
                             ++n)
                            if (hit({12, 74.F + (n - first) * 27, 216, 26})) {
                                action = A::select;
                                selected = n;
                            }
                        if (IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24}))
                            action = A::cancel;
                    }
                    if (raw != 76 && (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24})))
                        action = A::confirm;
                    if (action && session.act_facility_item_page(page->id, *action, selected) !=
                                      StartupWorldRuntimeError::none)
                        throw std::runtime_error("设施道具输入事务失败");
                }
            } else if (raw >= 51 && raw <= 54) {
                const auto view =
                    inspect_startup_world_village_activity_page(session.state(), page->id);
                if (view) {
                    using A = StartupVillageActivityAction;
                    const auto pid = page->id;
                    std::optional<A> action;
                    int selection{};
                    if (raw != 53) {
                        if (IsKeyPressed(KEY_UP))
                            action = A::previous;
                        if (IsKeyPressed(KEY_DOWN))
                            action = A::next;
                        const int count = raw == 52 ? 2 : static_cast<int>(view->entries.size());
                        for (int row = 0; row < 5 && row + view->first_visible < count; ++row)
                            if (hit({12, 74.F + row * 27, 216, 26})) {
                                action = A::select;
                                selection = row + view->first_visible;
                            }
                        if (IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24}))
                            action = A::cancel;
                    }
                    if ((raw != 53 || view->counter >= 120) &&
                        (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24})))
                        action = A::confirm;
                    if (action) {
                        const auto error =
                            session.act_village_activity_page(pid, *action, selection);
                        if (error != StartupWorldRuntimeError::none &&
                            error != StartupWorldRuntimeError::missing_source)
                            throw std::runtime_error("村办页面输入失败：" +
                                                     std::to_string(static_cast<int>(error)));
                        command_feedback =
                            error == StartupWorldRuntimeError::none ? "" : "尚未接入";
                    }
                }
            } else if (raw == 48) {
                if (IsKeyPressed(KEY_UP))
                    rank_selection = (rank_selection + 4) % 5;
                if (IsKeyPressed(KEY_DOWN))
                    rank_selection = (rank_selection + 1) % 5;
                if (hit({12, 77, 216, 20}))
                    rank_selection = 0;
                for (int n = 0; n < 4; ++n)
                    if (hit({12, 101.F + n * 24, 216, 20}))
                        rank_selection = n + 1;
                const bool cancel = IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24});
                if ((cancel || IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24})) &&
                    session.act_rank_page(page->id, rank_selection, cancel) !=
                        StartupWorldRuntimeError::none)
                    throw std::runtime_error("晋级页面输入失败");
            } else if (raw == 87) {
                const auto &s = session.state();
                const bool ending = s.award_termination_pending.count(page->id) &&
                                    s.award_termination_pending.at(page->id);
                const bool awarding = s.award_pending_humans.count(page->id) != 0;
                std::optional<ref::WorldAwardAction> action;
                if (ending || awarding) {
                    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT))
                        prompt_selection = 1 - prompt_selection;
                    if (hit({24, 194, 96, 28}))
                        prompt_selection = 0;
                    if (hit({120, 194, 96, 28}))
                        prompt_selection = 1;
                    if (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24}))
                        action =
                            ending ? (prompt_selection ? ref::WorldAwardAction::reject_termination
                                                       : ref::WorldAwardAction::confirm_termination)
                                   : (prompt_selection ? ref::WorldAwardAction::reject_award
                                                       : ref::WorldAwardAction::confirm_award);
                } else if (s.award_rankings.count(page->id)) {
                    const int count = static_cast<int>(s.award_rankings.at(page->id).size());
                    if (count && IsKeyPressed(KEY_UP))
                        award_selection = (award_selection + count - 1) % count;
                    if (count && IsKeyPressed(KEY_DOWN))
                        award_selection = (award_selection + 1) % count;
                    const int start = std::max(0, award_selection - 4);
                    for (int n = start; n < count && n < start + 5; ++n)
                        if (hit({12, 72.F + (n - start) * 22, 216, 20}))
                            award_selection = n;
                    if (IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24})) {
                        action = ref::WorldAwardAction::request_termination;
                        prompt_selection = 1; // 原终止询问默认“否”。
                    } else if (count && (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24}))) {
                        action = ref::WorldAwardAction::request_award;
                        prompt_selection = 0;
                    }
                }
                if (action && session.act_award_page(page->id, *action, award_selection) !=
                                  StartupWorldRuntimeError::none)
                    throw std::runtime_error("授勋页面输入失败");
            } else if (raw == 1 && session.state().task_abort_questions.count(page->id)) {
                if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT))
                    prompt_selection = 1 - prompt_selection;
                if (hit({24, 194, 96, 28}))
                    prompt_selection = 0;
                if (hit({120, 194, 96, 28}))
                    prompt_selection = 1;
                if ((IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24})) &&
                    session.act_task_page(page->id, StartupWorldTaskAction::confirm,
                                          prompt_selection)
                            .error != StartupWorldRuntimeError::none)
                    throw std::runtime_error("主动中止答案失败");
            } else if (raw == 80) {
                const auto &list = session.state().residence_page_candidates.at(page->id);
                if (!list.empty()) {
                    if (IsKeyPressed(KEY_UP))
                        task_selection = (task_selection + static_cast<int>(list.size()) - 1) %
                                         static_cast<int>(list.size());
                    if (IsKeyPressed(KEY_DOWN))
                        task_selection = (task_selection + 1) % static_cast<int>(list.size());
                }
                const bool cancel = IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24});
                if (cancel ||
                    (!list.empty() && (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24})))) {
                    const auto r = session.act_residence_page(
                        page->id, list.empty() ? -1 : list.at(task_selection), cancel);
                    if (r.error != StartupWorldRuntimeError::none)
                        throw std::runtime_error("入住选择事务失败");
                }
            } else if ((raw >= 60 && raw <= 66) || raw == 68 || raw == 69 || raw == 70 ||
                       raw == 73) {
                if (startup_world_human_page_ready(session.state(), page->id)) {
                    using A = StartupHumanPageAction;
                    const auto pid = page->id;
                    const auto act = [&](A a, int row = 0) {
                        if (session.act_human_page(pid, a, row) != StartupWorldRuntimeError::none)
                            throw std::runtime_error("人物页面输入失败");
                    };
                    if (raw == 60) {
                        for (int tab = 0; tab < 4; ++tab)
                            if (hit({8.F + tab * 56, 45, 56, 18}))
                                act(A::view_tab, tab);
                        if (IsKeyPressed(KEY_LEFT))
                            act(A::previous);
                        if (IsKeyPressed(KEY_RIGHT))
                            act(A::next);
                        if (hit({14, 232, 96, 24}))
                            act(A::gifts);
                        else if (hit({128, 232, 96, 24}))
                            act(A::professions);
                        else if (IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24}))
                            act(A::cancel);
                    } else if (raw == 61 || raw == 62 || raw == 64 || raw == 73) {
                        const int chosen = session.state().human_page_selections.at(pid);
                        if (raw == 64) {
                            for (int tab = 0; tab < 5; ++tab)
                                if (hit({8.F + tab * 45, 45, 45, 18}))
                                    act(A::equipment_slot, tab);
                        }
                        if (IsKeyPressed(KEY_UP))
                            act(A::previous);
                        if (IsKeyPressed(KEY_DOWN))
                            act(A::next);
                        if (raw == 61 || raw == 64) {
                            const int start = std::max(0, chosen - 4);
                            const auto &list = raw == 61
                                                   ? session.state().human_page_catalogs.at(pid)
                                                   : session.state().equipment_page_catalogs.at(
                                                         pid)[session.state().page_phases.at(pid)];
                            for (int row = 0;
                                 row < 5 && start + row < static_cast<int>(list.size()); ++row)
                                if (hit({12, 74.F + row * 27, 216, 26}))
                                    act(A::select, start + row);
                        }
                        if (raw == 62 || raw == 73) {
                            if (IsKeyPressed(KEY_LEFT))
                                act(A::previous);
                            if (IsKeyPressed(KEY_RIGHT))
                                act(A::next);
                        }
                        if (IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24}))
                            act(A::cancel);
                        else if (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24}))
                            act(A::confirm);
                        else if (raw == 64 && session.state().page_phases.at(pid) < 4 &&
                                 hit({96, 265, 52, 24}))
                            act(A::inspect_equipment);
                    } else if (raw == 65) {
                        if (IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24}))
                            act(A::cancel);
                        else if (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24}))
                            act(A::confirm);
                    } else {
                        if (raw == 70 && session.state().page_phases.at(pid) == 2) {
                            if (hit({14, 225, 98, 24}))
                                act(A::select, 0);
                            if (hit({128, 225, 98, 24}))
                                act(A::select, 1);
                        }
                        if (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24}))
                            act(A::confirm);
                    }
                }
            } else if (raw == 90) {
                const auto act = [&](StartupWorldTaxAction a) {
                    if (session.act_tax_page(page->id, a) != StartupWorldRuntimeError::none)
                        throw std::runtime_error("税收页面输入失败");
                };
                if (IsKeyPressed(KEY_UP))
                    act(StartupWorldTaxAction::previous);
                if (IsKeyPressed(KEY_DOWN))
                    act(StartupWorldTaxAction::next);
                if (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24}))
                    act(StartupWorldTaxAction::confirm);
            } else if (raw == 21) {
                const auto &groups = session.state().build_page_catalogs.at(page->id);
                if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
                    build_tab = (build_tab + (IsKeyPressed(KEY_LEFT) ? 2 : 1)) % 3;
                    build_selection = 0;
                }
                for (int tab = 0; tab < 3; ++tab)
                    if (hit({static_cast<float>(build_menu_x + 3 + tab * 57),
                             static_cast<float>(build_menu_y + 2), 56, 16})) {
                        build_tab = tab;
                        build_selection = 0;
                    }
                const auto &list = groups.at(build_tab);
                const auto count = static_cast<int>(list.size());
                if (count && IsKeyPressed(KEY_UP))
                    build_selection = (build_selection + count - 1) % count;
                if (count && IsKeyPressed(KEY_DOWN))
                    build_selection = (build_selection + 1) % count;
                const int first = std::max(0, build_selection - 4);
                for (int row = first; row < count && row < first + 5; ++row)
                    if (hit({static_cast<float>(build_menu_x + 4),
                             static_cast<float>(build_menu_y + 22 + (row-first)*37), 162, 37}))
                        build_selection = row;
                const auto pid = page->id;
                if (IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24})) {
                    if (session.cancel_build_menu(pid) != StartupWorldRuntimeError::none)
                        throw std::runtime_error("建设目录返回失败");
                } else if (count && (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24}))) {
                    const auto r = session.select_build_menu(pid, list.at(build_selection));
                    if (r.error != StartupWorldRuntimeError::none)
                        throw std::runtime_error("建设选择失败");
                    command_feedback =
                        r.denial == StartupBuildDenial::insufficient_funds ? "金钱不足" : "";
                    build_orientation = ref::FacilityOrientation::first;
                }
            } else if (raw == 74) {
                const auto pid = page->id;
                if (!session.state().facility_definition_page_bindings.count(pid) &&
                    session.state().page_phases.at(pid) == 1) {
                    const auto rows = startup_world_facility_bonus_rows(session.state(), pid);
                    if (!rows) throw std::runtime_error("设施奖励来源载荷非法");
                    const int count = static_cast<int>(rows->size());
                    if (count > 0) {
                        if (IsKeyPressed(KEY_DOWN)) bonus_selection = (bonus_selection + 1) % count;
                        if (IsKeyPressed(KEY_UP)) bonus_selection = (bonus_selection + count - 1) % count;
                        for (int n = 0; n < 5 && n + bonus_scroll < count; ++n)
                            if (hit({17,static_cast<float>(95+n*19),214,18}))
                                bonus_selection = n + bonus_scroll;
                        const auto wheel = GetMouseWheelMove();
                        if (wheel != 0)
                            bonus_selection = std::clamp(bonus_selection + (wheel > 0 ? -1 : 1),0,count-1);
                        bonus_scroll = std::clamp(bonus_scroll,bonus_selection-4,bonus_selection);
                        bonus_scroll = std::clamp(bonus_scroll,0,std::max(count-5,0));
                    }
                }
                if (!session.state().facility_definition_page_bindings.count(pid) &&
                    (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT))) {
                    if (session.act_facility_page(pid, StartupFacilityPageAction::next) !=
                        StartupWorldRuntimeError::none)
                        throw std::runtime_error("设施情报翻页失败");
                }
                if (IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24})) {
                    if (session.act_facility_page(pid, StartupFacilityPageAction::cancel) !=
                        StartupWorldRuntimeError::none)
                        throw std::runtime_error("设施情报返回失败");
                }
                if (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24})) {
                    const auto error =
                        session.act_facility_page(pid, StartupFacilityPageAction::confirm);
                    if (error != StartupWorldRuntimeError::none)
                        command_feedback = "暂不可用";
                }
            } else if (raw == 33) {
                const auto phase = session.state().page_phases.find(page->id);
                const bool animating =
                    phase != session.state().page_phases.end() && phase->second == 1;
                if (!animating) {
                    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT))
                        task_selection = 1 - task_selection;
                    if (hit({24, 194, 96, 28}))
                        task_selection = 0;
                    if (hit({120, 194, 96, 28}))
                        task_selection = 1;
                }
                // 页33无按钮2/Back分支；Esc不伪造取消，选择中止仍走同一50计数演出。
                if (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24})) {
                    ++confirmation_inputs;
                    if (session
                            .act_task_page(page->id, StartupWorldTaskAction::confirm,
                                           task_selection)
                            .error != StartupWorldRuntimeError::none)
                        throw std::runtime_error("任务期限输入消费者失败");
                }
            } else if (task_page) {
                const auto &s = session.state();
                const int count =
                    raw == 22 ? static_cast<int>(s.task_page_lists.at(page->id).size())
                    : (raw == 25 || raw == 26) ? static_cast<int>(s.participants.size()) + 1
                    : raw == 27 ? static_cast<int>(s.task_extra_pages.at(page->id).size())
                                : 0;
                if (count > 0) {
                    if (IsKeyPressed(KEY_UP))
                        task_selection = (task_selection + count - 1) % count;
                    if (IsKeyPressed(KEY_DOWN))
                        task_selection = (task_selection + 1) % count;
                    for (int row = 0; row < 5 && row + task_scroll < count; ++row)
                        if (hit({12, 62.0F + row * 20, 216, 20}))
                            task_selection = row + task_scroll;
                    task_scroll = std::clamp(task_scroll, task_selection - 4, task_selection);
                    task_scroll = std::clamp(task_scroll, 0, std::max(count - 5, 0));
                }
                if (raw != 24 && (IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24}))) {
                    if (session.act_task_page(page->id, StartupWorldTaskAction::cancel).error !=
                        StartupWorldRuntimeError::none)
                        throw std::runtime_error("任务取消消费者失败");
                } else if ((raw == 25 || raw == 26) && hit({86, 265, 68, 24}) &&
                           task_selection < static_cast<int>(s.participants.size())) {
                    if (session
                            .act_task_page(page->id, StartupWorldTaskAction::inspect,
                                           task_selection)
                            .error != StartupWorldRuntimeError::none)
                        throw std::runtime_error("队员详情打开失败");
                } else if (raw != 24 && (hit({176, 265, 58, 24}) || IsKeyPressed(KEY_ENTER))) {
                    auto action = StartupWorldTaskAction::confirm;
                    int selection = task_selection;
                    if (raw == 25)
                        action = selection == static_cast<int>(s.participants.size())
                                     ? StartupWorldTaskAction::add_member
                                     : StartupWorldTaskAction::depart;
                    else if (raw == 26 && selection == static_cast<int>(s.participants.size()))
                        action = StartupWorldTaskAction::add_member;
                    else if (raw == 27) {
                        action = StartupWorldTaskAction::hire;
                        selection = s.task_extra_pages.at(page->id).at(task_selection);
                    }
                    ++confirmation_inputs;
                    if (session.act_task_page(page->id, action, selection).error !=
                        StartupWorldRuntimeError::none)
                        throw std::runtime_error("任务输入消费者失败");
                }
            } else if (page->legacy_page != 58 && page->legacy_page != 97 && page->legacy_page != 98 &&
                       (hit({176, 265, 58, 24}) || IsKeyPressed(KEY_ENTER))) {
                ++confirmation_inputs;
                if (paragraph_index + 1 < static_cast<int>(page->paragraphs.size())) {
                    ++paragraph_index;
                    body_scroll = body_extent = 0;
                } else {
                    const auto error = session.acknowledge_page(page->id);
                    if (error != StartupWorldRuntimeError::none)
                        throw std::runtime_error("共同世界页面输入消费者失败");
                }
            }
            if (CheckCollisionPointRec(mouse, {12, 196, 216, 64}))
                body_scroll = std::clamp(body_scroll - GetMouseWheelMove() * 17, 0.0F,
                                         std::max(body_extent - 64, 0.0F));
        }
        if (!top_page() && !session.state().scene.framework_paused &&
            session.state().scene.scene_state == 1) {
            const int mode = session.state().build_mode;
            const bool rotate_pressed =
                (mode == 0 || mode == 7) && (IsKeyPressed(KEY_R) || hit({176, 46, 60, 20}));
            if (rotate_pressed)
                build_orientation = build_orientation == ref::FacilityOrientation::first
                                        ? ref::FacilityOrientation::second
                                        : ref::FacilityOrientation::first;
            if (IsKeyPressed(KEY_ESCAPE) || hit({52, 295, 52, 22})) {
                if ((mode == 0 ? session.cancel_build() : session.cancel_edit()) !=
                    StartupWorldRuntimeError::none)
                    throw std::runtime_error("建设返回失败");
                command_feedback.clear();
            } else if (!rotate_pressed && (pressed || IsKeyPressed(KEY_ENTER)) && mouse.y >= 35 &&
                       mouse.y < 265) {
                if (const auto cell = pick(mouse, camera)) {
                    const auto r = mode == 0 ? session.confirm_build(*cell, build_orientation)
                                             : session.confirm_edit(*cell, build_orientation);
                    if (r.error != StartupWorldRuntimeError::none)
                        throw std::runtime_error("建设事务失败");
                    command_feedback =
                        r.denial == StartupBuildDenial::none
                            ? session.state().build_feedback_message
                        : r.denial == StartupBuildDenial::insufficient_funds ? "金钱不足"
                        : r.denial == StartupBuildDenial::occupied           ? "有建筑物"
                        : r.denial == StartupBuildDenial::unavailable        ? "不可操作"
                                                                             : "请选择街道内地域";
                    if (mode == 6 && session.state().build_mode == 7 &&
                        session.state().build_moving_facility)
                        build_orientation = session.state()
                                                .scene.world.world.facilities
                                                .at(*session.state().build_moving_facility)
                                                .placement.orientation;
                }
            }
        } else if (!top_page() && session.state().scene.scene_state == 0 &&
                   !session.state().scene.framework_paused) {
            if (hit({52, 295, 52, 22}) || IsKeyPressed(KEY_ESCAPE)) {
                if (session.open_main_menu() != StartupWorldRuntimeError::none)
                    throw std::runtime_error("主菜单打开失败");
            } else if (IsKeyPressed(KEY_B)) {
                if (session.open_build_menu() != StartupWorldRuntimeError::none)
                    throw std::runtime_error("建设目录打开失败");
            } else if (IsKeyPressed(KEY_V) || hit({176, 23, 60, 20})) {
                if (session.open_village_activities() != StartupWorldRuntimeError::none)
                    throw std::runtime_error("村办入口失败");
            } else if ((session.state().scripts.user_flags & 16U) != 0 &&
                       (IsKeyPressed(KEY_S) || hit({176, 46, 60, 20}))) {
                if (session.open_commerce() != StartupWorldRuntimeError::none)
                    throw std::runtime_error("商会入口失败");
            } else if ((session.state().scripts.user_flags & 1U) != 0 &&
                       (IsKeyPressed(KEY_P) || hit({176, 138, 60, 20}))) {
                if (session.open_magic_pot(StartupMagicPotEntry::main_menu) != StartupWorldRuntimeError::none)
                    throw std::runtime_error("魔法壶入口失败");
            } else if (IsKeyPressed(KEY_D) || hit({176, 69, 60, 20})) {
                const auto road = available_road(session.state());
                if (road) {
                    const auto result = session.begin_road(*road);
                    if (result.error != StartupWorldRuntimeError::none)
                        throw std::runtime_error("道路入口事务失败");
                    command_feedback =
                        result.denial == StartupBuildDenial::insufficient_funds ? "金钱不足" : "";
                } else
                    command_feedback = "道路未开放";
            } else if (IsKeyPressed(KEY_M) || hit({176, 92, 60, 20}) || IsKeyPressed(KEY_DELETE) ||
                       hit({176, 115, 60, 20})) {
                const bool move = IsKeyPressed(KEY_M) || hit({176, 92, 60, 20});
                const auto result = session.begin_edit(move);
                if (result.error != StartupWorldRuntimeError::none)
                    throw std::runtime_error("地图编辑入口事务失败");
                command_feedback = result.denial == StartupBuildDenial::insufficient_funds
                                       ? "金钱不足"
                                   : result.denial == StartupBuildDenial::unavailable ? "移动未开放"
                                                                                      : "";
            } else if (pressed && mouse.y >= 35 && mouse.y < 265) {
                bool selected_human{};
                for (const auto actor : session.state().scene.world.world.ai.human_order) {
                    const auto &a = session.state().scene.world.world.ai.battle.actors.at(actor);
                    const auto projection = startup_world_raw_projection(a.position);
                    const Vector2 point{120 + projection.x - camera.x,
                                        160 - projection.y + camera.y};
                    if (a.control.state == 4 ||
                        !CheckCollisionPointRec(mouse, {point.x - 10, point.y - 24, 20, 26}))
                        continue;
                    if (session.open_human_page(a.definition) != StartupWorldRuntimeError::none)
                        throw std::runtime_error("人物详情打开失败");
                    selected_human = true;
                    break;
                }
                if (!selected_human)
                    if (const auto cell = pick(mouse, camera)) {
                        const auto &map = session.state().scene.world.world.map;
                        const auto &binding = map.cells.at(cell->y * map.width + cell->x).facility;
                        if (binding &&
                            session.state()
                                    .scene.world.world.facilities.at(binding->instance_id.value)
                                    .status != 0 &&
                            session.open_facility_page(binding->instance_id.value) !=
                                StartupWorldRuntimeError::none)
                            throw std::runtime_error("设施情报打开失败");
                    }
            }
        }
        if (!top_page() && session.state().scene.scene_state == 0 &&
            (hit({108, 295, 60, 22}) || IsKeyPressed(KEY_T)) &&
            session.open_task_menu() != StartupWorldRuntimeError::none)
            throw std::runtime_error("任务菜单消费者失败");
        if (!top_page() && session.state().scene.scene_state == 0 && session.state().active_task &&
            IsKeyPressed(KEY_X) &&
            session.open_task_control_menu() != StartupWorldRuntimeError::none)
            throw std::runtime_error("任务管理菜单打开失败");
        const auto *held_page = top_page();
        session.set_page_confirm_held(
            held_page && held_page->legacy_page == 24 &&
            (IsKeyDown(KEY_ENTER) || (IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
                                      CheckCollisionPointRec(mouse, {176, 265, 58, 24}))));
        // 框架绘制门槛与逻辑轮数分开；长停顿不补算，倍速由MainScene保存轮数。
        const auto gate =
            ref::prepare_world_render_gate(clock, static_cast<std::int64_t>(GetTime() * 1000));
        if (gate.error != ref::WorldRenderGateError::none)
            throw std::runtime_error("共同世界绘制时钟失败");
        if (gate.clock) {
            clock = *gate.clock;
            const auto result = session.update();
            if (result.error != StartupWorldRuntimeError::none)
                throw std::runtime_error(
                    "共同世界更新失败：" + std::to_string(static_cast<int>(result.error)) + "/" +
                    std::to_string(static_cast<int>(result.scene_error)) + "/" +
                    std::to_string(static_cast<int>(result.world_error)));
        }
        // 本研究窗口暂未实现音频；明确消费一次性请求，不保存无限声音历史。
        (void)session.take_sound_requests();
        const auto &state = session.state();
        camera = {state.camera[0] + view_offset.x, state.camera[1] + view_offset.y};
        const auto &world = state.scene.world.world;
        bool moved{};
        for (const auto &entry : world.ai.battle.actors) {
            const auto old = previous_positions.find(entry.first);
            const auto &p = entry.second.position;
            if (old != previous_positions.end() &&
                (old->second.x != p.x || old->second.z != p.z || old->second.height != p.height))
                moved = true;
            previous_positions[entry.first] = p;
        }
        moved_frames += moved ? 1 : 0;
        BeginTextureMode(canvas.value);
        ClearBackground(Color{83, 141, 77, 255});
        const auto display = [&](int id) -> const StartupDisplay & {
            const auto &list = startup_evidence().displays;
            const auto found =
                std::find_if(list.begin(), list.end(), [id](const auto &d) { return d.id == id; });
            if (found == list.end())
                throw std::runtime_error("共同世界缺少真实显示定义");
            return *found;
        };
        struct Overlay {
            float depth;
            std::function<void()> draw;
        };
        std::vector<Overlay> overlays;
        std::vector<Overlay> patches; // 原道路补块在地表第一遍之后提交。
        constexpr std::array<ref::Position, 6> fence_offsets{
            {{13, 22}, {16, 22}, {13, 21}, {42, 22}, {15, 23}, {16, 9}}};
        constexpr std::array<ref::Position, 4> entry_offsets{
            {{15, 24}, {26, 19}, {15, 18}, {28, 24}}};
        for (int y = world.map.height - 1; y >= 0; --y)
            for (int x = 0; x < world.map.width; ++x) {
                const auto index = static_cast<std::size_t>(y * world.map.width + x);
                const auto &cell = state.surface.at(index);
                const auto p = project({x, y}, camera);
                if (cell.display_definition < 0)
                    continue;
                const auto &record = display(cell.display_definition);
                const float base_depth =
                    ((record.flags & 1U) ? p.y - 50 : p.y + 15) + record.offset_y;
                // c/i.i已经是占地分片帧；逐格绘制双格/四格，不能再只画设施锚点。
                overlays.push_back(
                    {base_depth, [&, p, sprite = record.sprite, frame = cell.variant] {
                         sprites.draw(sprite, frame, p);
                     }});
                if (cell.fragment >= 0 && cell.fragment < 6 &&
                    world.map.cells[index].category == ref::RouteCategory::blocked) {
                    const auto offset = fence_offsets.at(cell.fragment);
                    overlays.push_back(
                        {base_depth + 60, [&, p, offset, frame = cell.fragment] {
                             sprites.draw("fence01" + std::to_string(state.fence_level) + ".seb",
                                          frame, {p.x + offset.x, p.y + offset.y}, WHITE,
                                          SourceSprites::Binding::common);
                         }});
                }
                if (cell.instance >= 0) { // 维护旧字段名instance实际保存原c/i.m外部入口方向。
                    const auto offset = entry_offsets.at(static_cast<std::size_t>(cell.instance));
                    overlays.push_back({p.y + offset.y, [&, p, offset, frame = cell.instance / 2] {
                                            sprites.draw("door00.seb", frame,
                                                         {p.x + offset.x, p.y + offset.y}, WHITE,
                                                         SourceSprites::Binding::common);
                                        }});
                }
                LoadedStartupCell patch_cell;
                patch_cell.display_id = cell.display_definition;
                patch_cell.road_quad = state.road_patches.at(index)[0];
                patch_cell.edge_road_pair = state.road_patches.at(index)[1];
                if (const auto patch = road_patch_draw(patch_cell)) {
                    patches.push_back({p.y + patch->depth_offset, [&, p, patch = *patch] {
                                           sprites.image(patch.asset_path, {p.x + patch.offset_x,
                                                                            p.y + patch.offset_y});
                                       }});
                }
            }
        overlays.insert(overlays.end(), patches.begin(), patches.end());
        const auto draw_actor = [&](ref::CharacterId id) {
            const auto &actor = world.ai.battle.actors.at(id);
            if (actor.control.flags & 1U)
                return; // 原Character.a(o,int)首个绘制守卫，不从状态号猜隐身。
            const auto &p = state.actor_metadata.at(id).render_position; // 原o，不是攻击备份au。
            const Vector2 anchor{120 + (p.x + p.z) * 0.3F - camera.x,
                                 160 + (p.x - p.z) * 0.15F - p.height + camera.y};
            const int direction = actor.control.facing;
            const int action = actor.control.action;
            const int tick = (actor.control.flags & 2U) ? actor.control.action_counter : 0;
            if (actor.kind == ref::ActorKind::human) {
                const auto &meta = state.actor_metadata.at(id);
                const auto profile = startup_world_human_profile(state, actor.definition);
                if (!profile)
                    throw std::runtime_error("窗口人物性别非法");
                const int image = rules.jobs.at(meta.profession).sprites.at(profile->sex);
                constexpr std::array<int, 12> offsets{{0, 4, 4, 4, 20, 4, 8, 12, 16, 20, 24, 20}};
                int phase{};
                if (action == 0 || action == 8)
                    phase = (tick % 16) / 4; // c.b.aZ=4，四个阈值4/8/12/16。
                else if (action == 1)
                    phase = tick % 12 < 4 ? 0 : 1;
                else if (action == 2 || action == 5)
                    phase = 1;
                else if (action == 3)
                    phase = tick % 12 < 6 ? 0 : 1;
                else if (action == 4)
                    phase = tick % 42 < 26 ? 0 : 1;
                else if (action == 10)
                    phase = tick % 18 < 12 ? 0 : 1;
                const int sprite = offsets.at(action) + (action == 5 ? tick % 16 % 4 : direction);
                sprites.actor(false, sprite, image, phase, anchor);
            } else {
                const auto &definition = world.ai.monster_growth.at(actor.definition);
                constexpr std::array<int, 4> lengths{{6, 6, 8, 10}};
                const int body = definition.body;
                int phase = (tick % (lengths.at(body) * 4)) / lengths.at(body);
                constexpr std::array<int, 4> attacks{{2, 4, 6, 8}};
                if (action == 3) {
                    if (tick < 10 || (tick >= 28 && tick < 34))
                        phase = 0;
                    else if (tick < 28)
                        phase = (tick % (attacks.at(body) * 4)) / attacks.at(body);
                } else if (action == 6) {
                    if (tick < 6)
                        phase = (tick % (attacks.at(body) * 4)) / attacks.at(body);
                    else if (tick < 12)
                        phase = 0;
                }
                sprites.actor(true, body * 4 + direction, body * 30 + definition.sprite_variant,
                              action == 9 ? 0 : phase, anchor);
            }
            const auto held = startup_world_equipment_lift_draws(state, id);
            if (!held) throw std::runtime_error("人物举物绘制载荷非法");
            const auto gains = startup_world_attribute_gain_draws(state,id,
                [&](const std::string &value) { return static_cast<int>(font.measure(value)); });
            if (!gains) throw std::runtime_error("人物属性头标绘制载荷非法");
            // 两类计划保留原cd下标，归并后按13背景/文字/数字顺序提交；重复重绘不更新Owner。
            std::size_t h=0,g=0;
            while (h<held->size() || g<gains->size()) {
                if (h<held->size() && !held->at(h).record_index)
                    throw std::runtime_error("人物绘制缺少cd记录身份");
                if (g==gains->size() || (h<held->size() &&
                    *held->at(h).record_index < gains->at(g).record_index)) {
                    sprites.visual(held->at(h++),anchor);
                } else {
                    const auto &gain=gains->at(g++);
                    for (const auto &plan:gain.before_text) sprites.visual(plan,anchor);
                    font.text(gain.text,anchor.x+gain.text_offset[0],anchor.y+gain.text_offset[1],
                              {static_cast<unsigned char>(gain.text_rgb[0]),
                               static_cast<unsigned char>(gain.text_rgb[1]),
                               static_cast<unsigned char>(gain.text_rgb[2]),255});
                    for (const auto &plan:gain.after_text) sprites.visual(plan,anchor);
                }
            }
        };
        for (const auto *roster : {&world.ai.human_order, &world.ai.monster_order})
            for (const auto id : *roster) {
                const auto &position = world.ai.battle.actors.at(id).position;
                const float depth = 160 + (position.x - position.z) * 0.15F + camera.y;
                overlays.push_back({depth, [&, id] { draw_actor(id); }});
            }
        for (const auto id : state.scene.world.facility_order) {
            const auto growth = startup_world_facility_growth_draws(state, id);
            if (!growth) throw std::runtime_error("设施增长头标载荷非法");
            if (!growth->empty()) {
                const auto target = startup_world_runtime_facility_target(state, id);
                if (!target) throw std::runtime_error("设施增长头标锚点非法");
                const Vector2 p{120 + (*target)[0] - camera.x, 160 - (*target)[1] + camera.y};
                overlays.push_back({p.y, [&, p, growth = *growth] {
                    for (const auto &plan : growth) sprites.visual(plan, p);
                }});
            }
            const auto rows = startup_world_inn_rows(state, id);
            if (!rows)
                throw std::runtime_error("旅馆占用人物显示投影无效");
            if (rows->empty())
                continue;
            const auto target = startup_world_runtime_facility_target(state, id);
            if (!target)
                throw std::runtime_error("旅馆真实占地锚点无效");
            // f()是原投影；a.a(f,L)在此研究画布转换为同一镜头，不能取设施anchor格。
            const Vector2 anchor{120 + (*target)[0] - camera.x, 160 - (*target)[1] + camera.y};
            overlays.push_back(
                {anchor.y, [&, anchor, rows = *rows] {
                     for (const auto &row : rows) {
                         const Vector2 p{anchor.x - 26, anchor.y - 42 - row.row * 20};
                         sprites.crop("common/restBar00.png", {0, 0, row.healing ? 17.F : 48.F, 17},
                                      p);
                         if (!row.healing) {
                             DrawRectangle(static_cast<int>(p.x) + 17, static_cast<int>(p.y) + 13,
                                           row.bar_width, 2, {255, 83, 152, 255});
                         } else {
                             const auto value = std::to_string(row.capacity);
                             float x = p.x + 47 - value.size() * 7;
                             for (const auto c : value) {
                                 sprites.draw("number11.seb", c - '0', {x, p.y + 1}, WHITE,
                                              SourceSprites::Binding::common);
                                 x += 7;
                             }
                             DrawRectangle(static_cast<int>(p.x) + 20, static_cast<int>(p.y) + 10,
                                           29, 5, {246, 246, 246, 255});
                             DrawRectangle(static_cast<int>(p.x) + 21, static_cast<int>(p.y) + 11,
                                           27, 3, {39, 53, 74, 255});
                             DrawRectangle(static_cast<int>(p.x) + 22, static_cast<int>(p.y) + 12,
                                           row.bar_width, 2, {83, 255, 0, 255});
                             DrawRectangle(static_cast<int>(p.x) + 22 + row.bar_width,
                                           static_cast<int>(p.y) + 12, 26 - row.bar_width, 2,
                                           {68, 100, 104, 255});
                         }
                         sprites.portrait(row.portrait, {p.x + 1, p.y + 1});
                     }
                 }});
        }
        std::stable_sort(overlays.begin(), overlays.end(),
                         [](const auto &a, const auto &b) { return a.depth < b.depth; });
        for (const auto &overlay : overlays)
            overlay.draw();
        if (state.scene.scene_state == 1) {
            const auto cell =
                inspect_page == "world-editing" ? state.build_anchor : pick(mouse, camera);
            const auto highlight = [&](ref::Position position) {
                const auto p = project(position, camera);
                DrawTriangle({p.x - 30, p.y}, {p.x, p.y + 15}, {p.x + 30, p.y},
                             {250, 220, 55, 120});
                DrawTriangle({p.x - 30, p.y}, {p.x + 30, p.y}, {p.x, p.y - 15},
                             {250, 220, 55, 120});
            };
            if (cell && (state.build_mode == 2 || state.build_mode == 5)) {
                if (const auto segment = startup_world_edit_segment(state, *cell))
                    for (const auto position : *segment)
                        highlight(position);
            } else if (cell && state.build_definition &&
                       (state.build_mode == 0 || state.build_mode == 7)) {
                const auto &d = rules.facilities.at(*state.build_definition);
                const auto footprint = ref::facility_footprint(
                    static_cast<ref::FacilityShape>(d.shape), build_orientation, *cell,
                    world.map.width, world.map.height);
                for (const auto &part : footprint.cells) {
                    highlight(part.position);
                }
                const auto preview = startup_world_building_preview_draws(state,*cell,build_orientation);
                if (!preview) throw std::runtime_error("候选建筑绘制载荷非法");
                sprites.building(*preview,project(*cell,camera));
            } else if (cell)
                highlight(*cell);
            DrawRectangle(5, 265, 230, 24, paper);
            font.text(command_feedback.empty() ? state.build_feedback_message : command_feedback,
                      12, 270);
        }
        for (const auto &effect : state.visual_effects) {
            if (effect.size() < 2 || effect[0] != 2 || effect[1] < 0)
                continue;
            if (effect.size() != 7)
                throw std::runtime_error("现金显示载荷不完整");
            float y = 160 - effect[3] + camera.y;
            y +=
                effect[1] < 6
                    ? -16 + ((effect[5] * effect[1] + effect[6] * effect[1] * (effect[1] + 1) / 2) /
                             1000)
                    : -26;
            const auto amount = std::to_string(effect[4]);
            float x = 120 + effect[2] - camera.x - (amount.size() * 8 + 9) / 2.0F + 28;
            for (const auto digit : amount) {
                sprites.draw("number05.seb", digit - '0', {x, y - 10}, WHITE,
                             SourceSprites::Binding::common);
                x += 8;
            }
            sprites.draw("number05.seb", 20, {x, y - 10}, WHITE, SourceSprites::Binding::common);
        }
        const auto notices = ref::world_notice_placements(state.scripts.notices);
        if (!notices)
            throw std::runtime_error("通知绘制投影无效");
        // 皮肤/成长属性拼片仍另验；这里仅消费已证布局和Owner文本，不从60FPS推进队列。
        BeginScissorMode(0, 35, width, height - 35);
        for (const auto &placement : *notices) {
            const auto &notice = state.scripts.notices.at(placement.index);
            const int y = 294 + placement.offset;
            if (notice.message != 1) {
                DrawRectangle(0, y, width, placement.height, paper);
                font.text(notice.text, 4, y + 3, ink, 10);
            }
        }
        EndScissorMode();
        DrawRectangle(0, 0, width, 35, paper);
        font.text(std::to_string(state.scene.calendar.year + 1) + "年" +
                      std::to_string(state.scene.calendar.month + 1) + "月",
                  6, 5);
        font.text(std::to_string(world.ai.accounting.funds()) + "G", 145, 5);
        font.text("点数 " + std::to_string(state.village_points), 6, 21);
        font.text("人气 " + std::to_string(state.popularity), 116, 21);
        if (state.report_state != 0) {
            // 原r/s月报不占框架页栈、不暂停世界；只展示已提交快照，不再扣款。
            DrawRectangle(10, 45, 220, 112, paper);
            font.text("本月结算", 84, 51);
            if (state.report_state == 1) {
                font.text("打倒怪物", 20, 76);
                font.text(std::to_string(state.report_snapshot[0]), 177, 76);
                font.text("获得村子点数", 20, 102);
                font.text(std::to_string(state.report_snapshot[1]), 177, 102);
            } else {
                constexpr std::array<const char *, 3> labels{{"收入", "支出", "收支"}};
                for (std::size_t n = 0; n < labels.size(); ++n) {
                    font.text(labels[n], 20, 75 + static_cast<int>(n) * 23);
                    const auto amount = std::to_string(state.report_snapshot[n + 2]) + "G";
                    const auto extent = font.measure(amount);
                    font.text(amount, 220 - extent, 75 + static_cast<int>(n) * 23);
                }
            }
        }
        if (const auto *page=top_page(); page && (navigation_menu(page->legacy_page)||page->legacy_page==9)) {
            // 真实菜单栈逐页画；被菜单覆盖的raw3保留底图但不登记主菜单ID8短适配。
            for (const auto &entry:state.scripts.pages)
                if(entry.lifecycle!=4 && entry.kind==ref::WorldScriptPageKind::raw_page &&
                   (navigation_menu(entry.legacy_page)||entry.legacy_page==9))
                    if(const auto plan=window_navigation_plan(state,entry.id,font,entry.id==page->id))
                        draw_navigation_plan(*plan,sprites,font);
            DrawRectangle(8,258,62,22,paper);DrawRectangle(166,258,66,22,paper);
            font.text("返回 Esc",12,263,ink,10);font.text("确定 Enter",170,263,ink,10);
            if(!command_feedback.empty())font.text(command_feedback,8,285,ink,10);
        } else if (const auto *page=top_page(); page && page->legacy_page>=34 && page->legacy_page<=40) {
            // 仅研究短适配：显示Owner真实行/页签，完整Steam详情字体与图元后端另批接入。
            DrawRectangle(5,42,230,247,paper);
            font.text(page->title+" / 研究适配",12,48,ink,11);
            if(const auto view=inspect_startup_world_information_page(state,page->id)) {
                font.text("页签 "+std::to_string(view->selection_or_period+1),12,66,ink,10);
                std::vector<std::string> rows;
                if(view->humans)for(const auto &human:*view->humans)rows.push_back(human.details.name);
                if(view->items)for(const auto &item:*view->items)rows.push_back(item.name+" "+std::to_string(item.inventory));
                if(view->equipment)for(const auto &item:view->equipment->rows)
                    rows.push_back(item.visible?item.visible->name:"????");
                if(view->facilities)for(const auto &facility:*view->facilities)
                    rows.push_back(facility.name+" "+std::to_string(facility.profit));
                for(int row=view->first_visible;row<static_cast<int>(rows.size())&&row<view->first_visible+5;++row) {
                    const int y=88+28*(row-view->first_visible);
                    if(row==view->selection)DrawRectangle(10,y-2,220,25,{219,232,204,255});
                    font.text(rows[row],14,float(y),ink,11);
                }
                if(view->income)for(std::size_t row=0;row<view->income->rows.size();++row) {
                    const auto &entry=view->income->rows[row];
                    font.text(std::string(entry.label)+" "+entry.income_text+" / "+entry.expense_text,
                              14,float(88+28*row),ink,10);
                }
                if(view->raw==34)font.text("Enter 设施列表",14,88,ink,11);
            }
            DrawRectangle(8,258,62,22,paper);DrawRectangle(166,258,66,22,paper);
            font.text("返回 Esc",12,263,ink,10);font.text("确定 Enter",170,263,ink,10);
            if(!command_feedback.empty())font.text(command_feedback,12,282,ink,10);
        } else if (const auto *page = top_page()) {
            if (page->legacy_page != 21) DrawRectangle(5, 170, 230, 119, paper);
            std::string title = page->title;
            if (title.empty() && page->kind == ref::WorldScriptPageKind::raw_page) {
                if (page->legacy_page == 30 || page->legacy_page == 31 || page->legacy_page == 32)
                    title = "成果";
                else if (page->legacy_page == 94 || page->legacy_page == 95)
                    title = "入手!";
                else if (page->legacy_page == 33)
                    title = "任务期限";
                else if (page->legacy_page == 83)
                    title = "商店追加";
                else if (page->legacy_page == 59)
                    title = "活动";
                else if (page->legacy_page >= 22 && page->legacy_page <= 28)
                    title = page->legacy_page == 22   ? "任务列表"
                            : page->legacy_page == 27 ? "追加队员"
                                                      : "征集队伍";
                else if ((page->legacy_page == 99 || page->legacy_page == 100) &&
                         page->monster_definition)
                    title = rules.monsters.at(*page->monster_definition).name;
            }
            if (page->legacy_page != 21)
                font.text(title, 12, 177, ink, font.measure(title) > 208 ? 10 : 12);
            if (page->legacy_page >= 41 && page->legacy_page <= 47) {
                DrawRectangle(8, 42, 224, 214, paper);
                const auto view = inspect_startup_world_magic_pot_page(state, page->id);
                font.text("魔法壶", 16, 48);
                if (view) {
                    const int raw = view->raw;
                    constexpr const char *elements[]{"火", "冰", "雷", "暗", "经验"};
                    if (raw <= 43) {
                        const int count = raw == 41 ? 2 : static_cast<int>(view->entries.size());
                        for (int n = view->first_visible; n < std::min(count, view->first_visible + 5); ++n) {
                            const int y = 74 + (n - view->first_visible) * 27;
                            if (n == view->selection) DrawRectangle(12, y, 216, 26, {219, 232, 204, 255});
                            std::string label, value;
                            if (raw == 41) label = n == 0 ? "投入道具" : "开发";
                            else {
                                const int id = view->entries[n];
                                if (raw == 42) {
                                    label = rules.items.at(id).name;
                                    value = "持有 " + std::to_string(state.items.at(id).inventory);
                                } else {
                                    const auto &r = rules.magic_pot_recipes.at(id);
                                    label = state.magic_pot_recipes.at(id).status == 0 ? "????" : r.name;
                                    value = view->phase == 0 ? std::to_string(r.experience_required) + "经验"
                                        : std::to_string(r.costs[0]) + "/" + std::to_string(r.costs[1]);
                                }
                            }
                            font.text(label, 16, y + 3, ink, 10);
                            font.text(value, 222 - font.measure(value, 9), y + 3, ink, 9);
                        }
                        font.text("等级 " + std::to_string(state.legacy_n[11]) + "  投入 " + std::to_string(state.legacy_n[1]), 16, 216, ink, 10);
                    } else if (raw == 44 || raw == 45) {
                        const int cols = raw == 44 ? 4 : 5;
                        for (int n = 0; n < cols; ++n) {
                            const auto value = std::to_string(state.magic_pot_display[0][n]) + " > " + std::to_string(state.magic_pot_display[1][n]);
                            font.text(elements[n], 16, 78 + n * 24, ink, 10);
                            font.text(value, 94, 78 + n * 24, ink, 10);
                        }
                        if (raw == 44) font.text(state.magic_pot_comment, 16, 207, ink, 9);
                    } else {
                        const auto &r = rules.magic_pot_recipes.at(view->binding);
                        font.text(raw == 46 ? "新配方" : "开发", 16, 74);
                        font.text(r.name, 16, 103);
                        if (raw == 47) for (int n = 0; n < 4; ++n)
                            font.text(std::string(elements[n]) + " " + std::to_string(r.costs[n]), 16, 134 + n * 22, ink, 10);
                    }
                    if (view->raw != 45) font.text("返回", 16, 272, ink, 10);
                    font.text("确定", 184, 272, ink, 10);
                }
            } else if (page->legacy_page == 72 || page->legacy_page == 79 || page->legacy_page == 82) {
                DrawRectangle(8, 42, 224, 214, paper);
                const auto view = inspect_startup_world_facility_catalog_page(state, page->id);
                font.text(page->legacy_page == 79 ? "商品" : page->legacy_page == 72 ? "装备信息" : "设施口碑", 16, 48);
                if (view) {
                    if (view->raw == 82) {
                        font.text(rules.facilities.at(view->binding).name, 16, 74);
                        for (std::size_t n = 0; n < view->entries.size(); ++n)
                            font.text(human_name(view->entries[n]), 16, 103 + n * 24);
                        font.text(std::to_string(view->phase + 1) + "/2", 192, 48, ink, 10);
                        font.text("确认继续", 164, 272, ink, 10);
                    } else {
                        const int kind = view->raw == 79
                            ? (view->mode == 1 ? 1 : view->mode == 4 ? 2 : 3)
                            : (view->mode == 0 ? 1 : view->mode == 3 ? 3 : 2);
                        const auto entry = [&](int id) -> const StartupWorldEquipment & {
                            const auto d = std::find_if(rules.equipment.begin(), rules.equipment.end(),
                                [&](const auto &e) { return e.shop.kind == kind && e.shop.id == id; });
                            if (d == rules.equipment.end()) throw std::runtime_error("商品显示缺定义");
                            return *d;
                        };
                        if (view->raw == 79) {
                            font.text(std::to_string(view->phase + 1) + "/2", 192, 48, ink, 10);
                            for (int n = view->first_visible;
                                 n < std::min(static_cast<int>(view->entries.size()), view->first_visible + 4); ++n) {
                                const auto &e = entry(view->entries[n]);
                                const int y = 74 + (n - view->first_visible) * 27;
                                if (n == view->selection) DrawRectangle(12, y, 216, 26, {219, 232, 204, 255});
                                font.text(e.name, 16, y + 3, ink, 10);
                                if (view->phase == 0) font.text(std::to_string(e.shop.price) + "G", 174, y + 3, ink, 10);
                                else font.text(std::to_string(e.shop.combat[kind == 1 ? 1 : 0]) + "/" + std::to_string(e.shop.combat[kind == 1 ? 3 : 2]), 174, y + 3, ink, 10);
                            }
                            font.text("信息", 96, 272, ink, 10);
                        } else {
                            const auto &e = entry(view->binding);
                            font.text(e.name, 16, 74);
                            font.text(std::to_string(e.shop.price) + "G", 16, 102);
                            font.text("攻击 " + std::to_string(e.shop.combat[1]) + " 防御 " + std::to_string(e.shop.combat[2]), 16, 129, ink, 10);
                            font.text("HP " + std::to_string(e.shop.combat[0]) + " 魔法 " + std::to_string(e.shop.combat[3]), 16, 154, ink, 10);
                        }
                        font.text("<", 16, 242); font.text(">", 214, 242);
                        font.text("返回", 16, 272, ink, 10);
                        font.text("关闭", 184, 272, ink, 10);
                    }
                }
            } else if ((page->legacy_page >= 83 && page->legacy_page <= 86) || page->legacy_page == 93) {
                DrawRectangle(8, 42, 224, 214, paper);
                const auto view = inspect_startup_world_commerce_page(state, page->id);
                font.text("南瓜商会", 16, 48);
                if (view) {
                    const int raw = view->raw;
                    if (raw == 83) {
                        constexpr const char *labels[]{"购买道具", "出售道具", "购买设施"};
                        for (int n = 0; n < 3; ++n) {
                            const float y = 78 + n * 27;
                            if (n == view->selection)
                                DrawRectangle(12, y - 4, 216, 26, {219, 232, 204, 255});
                            font.text(labels[n], 24, y);
                        }
                    } else if (raw == 84 || raw == 85) {
                        const int rows = raw == 85 ? 3 : 5;
                        if (raw == 84 && view->mode == 0)
                            font.text(view->tab == 0 ? "价格 >" : "持有 >", 164, 48, ink, 10);
                        for (int n = view->first_visible;
                             n < std::min(static_cast<int>(view->entries.size()),
                                          view->first_visible + rows);
                             ++n) {
                            const int entry = view->entries[n];
                            const float y = 78 + (n - view->first_visible) * 27;
                            if (n == view->selection)
                                DrawRectangle(12, y - 4, 216, 26, {219, 232, 204, 255});
                            const bool facility = raw == 85;
                            if (!facility) {
                                const auto icon=startup_world_item_icon_draws(state,entry);
                                if (!icon) throw std::runtime_error("商会道具图标载荷非法");
                                for (const auto &plan:*icon) sprites.visual(plan,{18,y});
                            }
                            font.text(facility ? rules.facilities.at(entry).name
                                               : rules.items.at(entry).name,
                                      facility?16:40, y, ink, 10);
                            std::string value;
                            if (facility)
                                value = std::to_string(rules.facility_initial.at(entry).capacity) +
                                        "点";
                            else if (view->mode == 0 && view->tab == 1)
                                value = "持有" + std::to_string(state.items.at(entry).inventory);
                            else {
                                value =
                                    std::to_string(view->mode == 0
                                                       ? state.shop_item_stock.at(entry).quantity
                                                       : state.items.at(entry).inventory) +
                                    " / " +
                                    std::to_string(rules.items.at(entry).commerce_price /
                                                   (view->mode == 0 ? 1 : 2)) +
                                    "G";
                            }
                            font.text(value, 222 - font.measure(value, 9), y, ink, 9);
                        }
                        if (view->feedback_counter > 0)
                            font.text("多谢惠顾!", 16, 225);
                        else
                            font.text(raw == 85         ? "使用村子点数"
                                      : view->mode == 0 ? "购买一件道具"
                                                        : "出售一件道具",
                                      16, 225, ink, 10);
                    } else if (raw == 86) {
                        font.text(rules.items.at(view->binding).name, 16, 90);
                        font.text(view->mode == 0 ? "购买完毕" : "出售完毕", 16, 124);
                    } else {
                        font.text(rules.facilities.at(view->binding).name, 16, 90);
                        font.text("免费建设次数 +1", 16, 124);
                        font.text("确认领取", 16, 153);
                    }
                    if (raw <= 85)
                        font.text("返回", 18, 272);
                    if (raw == 85)
                        font.text("详情", 100, 272);
                    if (raw != 86)
                        font.text("确定", 190, 272);
                }
            } else if (page->legacy_page >= 75 && page->legacy_page <= 77) {
                DrawRectangle(8, 42, 224, 214, paper);
                font.text("使用道具", 16, 48);
                if (state.facility_item_pages_initialized.count(page->id) &&
                    valid_startup_world_facility_item_page(state, *page)) {
                    const int raw = page->legacy_page;
                    if (raw == 75) {
                        const auto &list = state.facility_item_page_lists.at(page->id);
                        const int chosen = state.facility_item_page_selections.at(page->id);
                        const int first = std::max(0, chosen - 4);
                        for (int n = first; n < std::min(static_cast<int>(list.size()), first + 5);
                             ++n) {
                            const float y = 78 + (n - first) * 27;
                            if (n == chosen)
                                DrawRectangle(12, y - 4, 216, 26, {219, 232, 204, 255});
                            const auto icon=startup_world_item_icon_draws(state,list[n]);
                            if (!icon) throw std::runtime_error("设施道具图标载荷非法");
                            for (const auto &plan:*icon) sprites.visual(plan,{18,y});
                            font.text(rules.items.at(list[n]).name, 40, y, ink, 10);
                            font.text("持有 " + std::to_string(state.items.at(list[n]).inventory),
                                      172, y, ink, 9);
                        }
                        font.text("返回", 18, 272);
                    } else {
                        font.text(rules.items.at(state.facility_item_page_items.at(page->id)).name,
                                  16, 82);
                        font.text(raw == 76 ? "设施强化" : "使用结果", 16, 110);
                        constexpr const char *labels[]{"价格", "品质", "魅力"};
                        for (int n = 0; n < 3; ++n)
                            font.text(std::string(labels[n]) + " " +
                                          std::to_string(state.facility_upgrade_display[0][n]) +
                                          " > " +
                                          std::to_string(state.facility_upgrade_display[1][n]),
                                      16, 139 + n * 24, ink, 10);
                    }
                    if (raw != 76)
                        font.text("确定", 190, 272);
                }
            } else if (page->legacy_page == 95) {
                // 简化奖励投影；计数、领取和解锁仍只由Owner消费者提交。
                DrawRectangle(8, 42, 224, 214, paper);
                font.text("获得奖励", 16, 48);
                std::string reward;
                switch (page->legacy_r) {
                case 0:
                    reward = std::to_string(page->legacy_s) + "G";
                    break;
                case 1:
                    reward = "村子点 " + std::to_string(page->legacy_s);
                    break;
                case 3:
                    reward = rules.facilities.at(page->legacy_s).name;
                    break;
                case 4:
                    reward = rules.jobs.at(page->legacy_s).name;
                    break;
                case 9:
                    reward = "配置更替";
                    break;
                case 10:
                    reward = "勋章";
                    break;
                case 11:
                    reward = rules.activities.at(page->legacy_s).name;
                    break;
                default:
                    throw std::runtime_error("未维护的奖励页类型");
                }
                font.paragraph(reward, 16, 96, 208);
                font.text("确认领取", 176, 272, ink, 10);
            } else if (page->legacy_page == 21) {
                DrawRectangle(build_menu_x,build_menu_y,175,210,{79,72,48,255});
                DrawRectangle(build_menu_x+1,build_menu_y+1,173,208,{193,234,94,255});
                DrawRectangle(build_menu_x+4,build_menu_y+22,162,184,paper);
                constexpr std::array<const char *, 3> tabs{{"设备", "一般", "饮食"}};
                for (int tab = 0; tab < 3; ++tab) {
                    const int x = build_menu_x+3+tab*57;
                    DrawRectangle(x,build_menu_y+2,56,16,
                                  tab==build_tab ? Color{3,255,133,255} : Color{223,255,67,255});
                    font.text(tabs[tab], x+(56-font.measure(tabs[tab],10))/2,build_menu_y+5,ink,10);
                }
                const auto &list = state.build_page_catalogs.at(page->id).at(build_tab);
                const int first = std::max(0,build_selection-4);
                for (int row = first; row < static_cast<int>(list.size()) && row < first+5; ++row) {
                    const auto &d = rules.facilities.at(list[row]);
                    const int x = build_menu_x+10, y = build_menu_y+25+(row-first)*37;
                    DrawRectangle(x,y,64,32,{190,242,230,255});
                    const auto icon = startup_world_building_draws(state,d.id,ref::FacilityOrientation::first);
                    if (!icon) throw std::runtime_error("建设目录建筑绘制载荷非法");
                    // 原图大小、原分片顺序；不按包围盒居中或缩放。
                    BeginScissorMode(x,y,64,32);
                    sprites.building(*icon,{static_cast<float>(x+2),static_cast<float>(y+10)});
                    EndScissorMode();
                    if (row == build_selection)
                        DrawRectangle(x+73,y,static_cast<int>(font.measure(d.name,10))+8,15,{248,193,108,255});
                    font.text(d.name,x+73,y+3,ink,10);
                    const auto quote = startup_world_build_quote(state, d.id);
                    if (!quote)
                        throw std::runtime_error("建设目录报价缺失");
                    const auto price=std::to_string(quote->construction_cost)+"G";
                    font.text(price,x+154-font.measure(price,10),y+18,ink,10);
                }
                font.text(command_feedback, 65, 272, ink, 10);
                font.text("返回", 18, 272);
            } else if (page->legacy_page == 74 &&
                       state.facility_definition_page_bindings.count(page->id)) {
                if (!valid_startup_world_facility_page(state, *page))
                    throw std::runtime_error("设施定义详情载荷无效");
                const int definition = state.facility_definition_page_bindings.at(page->id);
                const auto &d = rules.facilities.at(definition);
                DrawRectangle(5, 42, 230, 216, paper);
                const auto title_icon = startup_world_facility_icon_draw(state,d.id);
                if (!title_icon) throw std::runtime_error("设施类别图标载荷非法");
                sprites.visual(*title_icon,{12,48});
                font.text(d.name, 32, 48);
                const auto &art = display(d.display_id);
                sprites.draw(art.sprite, 0, {65, 119});
                if (d.detail == 1 || d.detail == 4 || d.detail == 5) {
                    const int kind = d.detail == 1 ? 1 : d.detail == 4 ? 2 : 3;
                    const auto count = std::count_if(
                        state.catalog.begin(), state.catalog.end(), [&](const auto &entry) {
                            return entry.first.first == kind && entry.second.status != 0;
                        });
                    font.text("商品种类 " + std::to_string(count), 16, 160);
                } else if (d.detail == 6)
                    font.text("募集冒险者入住", 16, 160);
                else if (d.kind == 2)
                    font.text("装饰设施", 16, 160);
                else {
                    const auto &values = state.scripts.facilities.at(definition).attributes;
                    constexpr const char *labels[]{"价格", "品质", "魅力"};
                    for (int n = 0; n < 3; ++n)
                        font.text(std::string(labels[n]) + " " + std::to_string(values[n]), 128,
                                  94 + n * 24, ink, 11);
                }
                font.text("返回", 18, 272);
                font.text("确定", 190, 272);
            } else if (page->legacy_page == 74) {
                const auto id = state.facility_page_bindings.at(page->id);
                const auto &facility = world.facilities.at(id);
                const auto &d = rules.facilities.at(facility.placement.definition_id);
                const auto values = startup_world_facility_values(state, id);
                if (!values)
                    throw std::runtime_error("设施情报经营投影失败");
                DrawRectangle(5, 42, 230, 216, paper);
                const auto title_icon = startup_world_facility_icon_draw(state,d.id);
                if (!title_icon) throw std::runtime_error("设施类别图标载荷非法");
                sprites.visual(*title_icon,{12,48});
                font.text(d.name, 32, 48);
                const auto &growth = world.facility_uses.at(d.id);
                font.text("Lv." + std::to_string(growth.level), 144, 48, ink, 10);
                const auto phase = state.page_phases.at(page->id);
                font.text(std::to_string(phase + 1) + "/" +
                              std::to_string(startup_world_facility_page_count(state, *page)),
                          192, 48, ink, 10);
                constexpr std::array<const char *, 4> labels{{"价格", "品质", "魅力", "维护费"}};
                const int start = 0;
                const int end = phase == 0 ? 3 : 0;
                for (int n = start; n < end; ++n) {
                    const float y = 74 + (n - start) * 23;
                    font.text(labels[n], 16, y);
                    font.text(std::to_string(values->instance_attributes[n]), 170, y);
                }
                if (phase == 1) {
                    const auto view = startup_world_facility_bonus_window(state,page->id,bonus_scroll);
                    if (!view) throw std::runtime_error("设施奖励来源投影失败");
                    font.text("周围设施的奖励", 62, 74, ink, 11);
                    if (view->rows.empty()) font.text("没有奖励",92,97,ink,11);
                    for (int n = 0; n < static_cast<int>(view->rows.size()); ++n) {
                        const auto &row = view->rows[n];
                        const float y = 97 + n * 19;
                        if (n + bonus_scroll == bonus_selection)
                            DrawRectangle(17,static_cast<int>(y)-2,204,18,{219,232,204,255});
                        const auto icon = startup_world_facility_icon_draw(state,row.definition);
                        if (!icon) throw std::runtime_error("设施奖励行图标非法");
                        sprites.visual(*icon,{26,y-2});
                        font.text(row.name,44,y,ink,10);
                        for (const auto &value : row.values) {
                            const float x = value.attribute == 0 ? 121 : value.attribute == 1 ? 171 : 147;
                            font.text(value.label,x,y,{0,101,255,255},9);
                            font.text(value.text,x+font.measure(value.label,9),y,ink,9);
                        }
                    }
                    if (view->total > 5)
                        font.text(std::to_string(bonus_selection+1)+"/"+std::to_string(view->total),190,202,ink,9);
                } else {
                    font.text("使用 " + std::to_string(growth.completed_uses) + "/" +
                                  std::to_string(values->upgrade_uses),
                              16, 150, ink, 10);
                    font.text(facility.status == 0 ? "施工中" : "营业中", 16, 172, ink, 10);
                    const auto &art = display(d.display_id);
                    sprites.draw(art.sprite, 0, {182, 218});
                    if (d.kind == 12 && state.facility_residents.at(id) >= 0)
                        font.text(human_name(state.facility_residents.at(id)), 16, 197,
                                  ink, 10);
                }
                font.text("返回", 18, 272);
                if (d.detail == 6)
                    font.text("入住", 190, 272);
                font.text(command_feedback, 12, 199, ink, 10);
            }
            if (page->kind == ref::WorldScriptPageKind::raw_page && page->legacy_page == 59) {
                // 定义已解锁但实例尚未到访；这里只读当前职业/性别和页面计数绘制。
                const auto &human = rules.humans.at(page->legacy_f);
                const auto profession =
                    world.ai.growth.at(human.identity).definition.current_profession;
                const auto &job = rules.jobs.at(profession);
                const auto count = state.page_counters.find(page->id);
                const int tick = count == state.page_counters.end() ? 0 : count->second;
                DrawRectangle(8, 42, 224, 126, paper);
                const auto portrait = startup_world_portrait(state, human.identity);
                if (!portrait)
                    throw std::runtime_error("到访头像资料非法");
                sprites.actor(false, 2, portrait->image, (tick % 16) / 4, {120, 144});
                if (tick >= 60)
                    font.text("多指教", 98, 91);
                BeginScissorMode(12, 196, 212, 64);
                font.paragraph(job.name + "的\n" + human_name(human.identity) + "可以到访了!", 12, 198, 208);
                EndScissorMode();
            }
            if (!page->paragraphs.empty()) {
                BeginScissorMode(12, 196, 212, 64);
                body_extent =
                    font.paragraph(page->paragraphs.at(static_cast<std::size_t>(paragraph_index)),
                                   12, 198 - body_scroll, 208);
                EndScissorMode();
                if (body_extent > 64) {
                    DrawRectangle(226, 198, 2, 60, LIGHTGRAY);
                    DrawRectangle(226,
                                  198 + static_cast<int>(body_scroll / (body_extent - 64) * 48), 2,
                                  12, GRAY);
                }
            }
            if (page->kind == ref::WorldScriptPageKind::raw_page && page->legacy_page == 31) {
                const auto crew = state.crew_summaries.find(page->id);
                if (crew != state.crew_summaries.end()) {
                    DrawRectangle(10, 48, 220, 119, paper);
                    font.text("姓名", 18, 54);
                    font.text("打倒数", 133, 54);
                    font.text("下降", 196, 54);
                    for (std::size_t n = 0; n < crew->second.size() && n < 5; ++n) {
                        const auto definition = crew->second[n];
                        const auto &human = world.ai.battle.humans.at(definition);
                        const auto &name = human_name(definition);
                        const auto y = 77 + static_cast<int>(n) * 17;
                        font.text(name, 18, y, ink, font.measure(name) > 110 ? 10 : 12);
                        font.text(std::to_string(human.task_kills), 151, y);
                        font.text(std::to_string(human.participant_downs), 209, y);
                    }
                }
            }
            if (page->kind == ref::WorldScriptPageKind::raw_page &&
                (page->legacy_page == 30 || page->legacy_page == 31 || page->legacy_page == 32) &&
                page->task_definition) {
                const auto &task = rules.tasks.at(*page->task_definition);
                BeginScissorMode(12, 196, 212, 64);
                font.paragraph(task.name + "完成!", 12, 198, 208);
                EndScissorMode();
            }
            if (page->legacy_page == 87) {
                const bool ending = state.award_termination_pending.count(page->id) &&
                                    state.award_termination_pending.at(page->id);
                const bool awarding = state.award_pending_humans.count(page->id) != 0;
                DrawRectangle(8, 42, 224, 190, paper);
                font.text("年度贡献 / 勋章 " + std::to_string(state.medal_count), 17, 48);
                if (ending || awarding) {
                    font.paragraph(
                        ending ? "终止授勋仪式？"
                               : "授予" +
                                     human_name(state.award_pending_humans.at(page->id)) +
                                     "勋章？",
                        17, 83, 204);
                    DrawRectangle(prompt_selection == 0 ? 24 : 120, 194, 96, 28,
                                  {219, 232, 204, 255});
                    font.text("是", 62, 202);
                    font.text("否", 158, 202);
                } else if (state.award_rankings.count(page->id)) {
                    const auto &list = state.award_rankings.at(page->id);
                    const int start = std::max(0, award_selection - 4);
                    for (int n = start; n < static_cast<int>(list.size()) && n < start + 5; ++n) {
                        const int human = list[n];
                        const int y = 75 + (n - start) * 22;
                        if (n == award_selection)
                            DrawRectangle(12, y - 3, 216, 20, {219, 232, 204, 255});
                        font.text(human_name(human), 17, y);
                        font.text(std::to_string(state.human_calendar.at(human).contribution), 188,
                                  y);
                    }
                    font.text("终止", 18, 272);
                }
                font.text(ending || awarding ? "确定" : "授予", 190, 272);
            } else if (page->legacy_page == 1 && state.task_abort_questions.count(page->id)) {
                DrawRectangle(prompt_selection == 0 ? 24 : 120, 194, 96, 28, {219, 232, 204, 255});
                font.text("是", 62, 202);
                font.text("否", 158, 202);
                font.text("确定", 190, 272);
            } else if (page->legacy_page == 80) {
                DrawRectangle(8, 42, 224, 190, paper);
                const auto &list = state.residence_page_candidates.at(page->id);
                const int start = std::max(0, task_selection - 3);
                for (int n = start; n < static_cast<int>(list.size()) && n < start + 4; ++n) {
                    const auto &human = rules.humans.at(list[n]);
                    const int y = 65 + (n - start) * 24;
                    if (n == task_selection)
                        DrawRectangle(12, y - 3, 216, 22, {219, 232, 204, 255});
                    font.text(human_name(human.identity), 17, y);
                    font.text(std::to_string(human.residence_fee) + "G", 178, y);
                }
                font.text("返回", 18, 272);
                font.text("入住", 190, 272);
            } else if (page->legacy_page >= 51 && page->legacy_page <= 54) {
                const auto view = inspect_startup_world_village_activity_page(state, page->id);
                DrawRectangle(8, 42, 224, 214, paper);
                font.text("村办活动", 16, 48);
                if (view) {
                    const auto *activity =
                        view->activity ? &rules.activities.at(*view->activity) : nullptr;
                    if (view->raw == 51 || view->raw == 54) {
                        for (int n = view->first_visible;
                             n < std::min(static_cast<int>(view->entries.size()),
                                          view->first_visible + 5);
                             ++n) {
                            const int y = 78 + (n - view->first_visible) * 27;
                            if (n == view->selection)
                                DrawRectangle(12, y - 4, 216, 26, {219, 232, 204, 255});
                            const int entry = view->entries[n];
                            if (view->raw == 51) {
                                const auto &a = rules.activities.at(entry);
                                font.text(a.name, 16, y, ink, 10);
                                font.text(std::to_string(a.parameters[4]) + "点", 184, y, ink, 10);
                            } else {
                                const auto h = startup_world_human_details(state, entry);
                                if (!h || !activity)
                                    throw std::runtime_error("活动结果人物缺失");
                                const int current = activity->parameters[2] == 0
                                                        ? h->satisfaction
                                                        : h->attributes.at(activity->parameters[3]);
                                font.text(human_name(entry), 16, y, ink, 10);
                                font.text(std::to_string(state.human_activity_previous.at(entry)) +
                                              " > " + std::to_string(current),
                                          154, y, ink, 10);
                            }
                        }
                        if (view->raw == 51)
                            font.text("村子点 " + std::to_string(state.village_points) +
                                          "  季度剩余 " + std::to_string(state.quarter_counter),
                                      16, 216, ink, 10);
                        // 展示共享定义当前职业/性别，不创建场上人物实例。
                        for (int n = 0; n < 2; ++n) {
                            const int human = view->display_humans[n];
                            const auto presence = state.human_presence.find(human);
                            if (presence == state.human_presence.end())
                                throw std::runtime_error("活动展示人物引用无效");
                            if (presence->second == 0)
                                continue;
                            const auto portrait = startup_world_portrait(state, human);
                            if (!portrait)
                                throw std::runtime_error("活动展示人物职业或性别无效");
                            sprites.actor(false, 0, portrait->image, 0, {74.F + n * 90, 252});
                        }
                    } else if (view->raw == 52) {
                        if (activity)
                            font.text(activity->name, 98, 48, ink, 10);
                        for (int n = 0; n < 2; ++n) {
                            if (n == view->selection)
                                DrawRectangle(12, 74 + n * 27, 216, 26, {219, 232, 204, 255});
                            font.text(n == 0 ? "开展活动" : "返回", 16, 78 + n * 27);
                        }
                        if (activity)
                            font.paragraph(activity->detail, 16, 152, 208);
                    } else {
                        if (activity)
                            font.paragraph(activity->description, 16, 82, 208);
                        font.text(std::to_string(std::min(view->counter, 120)) + " / 120", 74, 150);
                        DrawRectangle(16, 178, 208, 8, GRAY);
                        DrawRectangle(16, 178, 208 * std::min(view->counter, 120) / 120, 8, GREEN);
                    }
                    if (view->raw != 53)
                        font.text("返回", 18, 272);
                    if (view->raw != 53 || view->counter >= 120)
                        font.text("确定", 190, 272);
                }
                if (!command_feedback.empty())
                    font.text(command_feedback, 92, 272, RED, 10);
            } else if (page->legacy_page == 60 && startup_world_human_page_ready(state, page->id)) {
                const int human = state.page_human_bindings.at(page->id);
                const auto details = startup_world_human_details(state, human);
                if (!details)
                    throw std::runtime_error("人物详情缺少共享定义");
                const int tab = state.page_phases.at(page->id);
                constexpr const char *tabs[]{"概况", "属性", "装备", "魔法"};
                DrawRectangle(8, 42, 224, 214, paper);
                for (int n = 0; n < 4; ++n) {
                    if (tab == n)
                        DrawRectangle(8 + n * 56, 45, 56, 18, {219, 232, 204, 255});
                    font.text(tabs[n], 16 + n * 56, 48, ink, 10);
                }
                if (const auto portrait = startup_world_portrait(state, human))
                    sprites.portrait(*portrait, {16, 74});
                font.text(human_name(human), 40, 74);
                font.text(rules.jobs.at(details->profession).name + " Lv." +
                              std::to_string(details->level),
                          16, 98, ink, 11);
                if (tab == 0) {
                    const int hp = details->live_actor
                                       ? world.ai.battle.actors.at(*details->live_actor).hp.target
                                       : details->combat[0];
                    font.text("HP " + std::to_string(hp) + "/" + std::to_string(details->combat[0]),
                              16, 121, ink, 11);
                    font.text("满足 " + std::to_string(details->satisfaction) + "  努力 " +
                                  std::to_string(details->effort),
                              16, 143, ink, 11);
                    font.text("勋章 " + std::to_string(details->medals) +
                                  (details->resident ? "  居民" : "  冒险者"),
                              16, 165, ink, 11);
                    font.text(details->level >= 10 ? "职业大师"
                                                   : "经验 " + std::to_string(details->experience) +
                                                         "/" + std::to_string(details->threshold),
                              16, 187, ink, 11);
                } else if (tab == 1) {
                    constexpr const char *labels[]{"体力", "力量", "灵活", "结实", "魔力", "运气"};
                    for (int n = 0; n < 6; ++n) {
                        const int x = 16 + (n % 2) * 111, y = 122 + (n / 2) * 24;
                        font.text(labels[n], x, y, ink, 10);
                        font.text(std::to_string(details->attributes[n]), x + 50, y, ink, 10);
                    }
                    font.text("攻击 " + std::to_string(details->combat[1]) + " 防御 " +
                                  std::to_string(details->combat[2]) + " 魔法 " +
                                  std::to_string(details->combat[3]),
                              16, 200, ink, 10);
                } else if (tab == 2) {
                    constexpr const char *labels[]{"武器", "防具1", "防具2", "饰品"};
                    for (int n = 0; n < 4; ++n) {
                        std::string name = "无";
                        if (details->equipment[n]) {
                            const int kind = n == 0 ? 1 : n == 3 ? 3 : 2;
                            for (const auto &e : rules.equipment)
                                if (e.shop.kind == kind && e.shop.id == *details->equipment[n])
                                    name = e.name;
                        }
                        font.text(labels[n], 16, 121 + n * 22, ink, 10);
                        font.text(name, 75, 121 + n * 22, ink, 10);
                    }
                } else {
                    constexpr const char *names[]{"火魔法", "冰魔法", "雷魔法", "回复魔法"};
                    for (int n = 0; n < 4; ++n)
                        font.text(std::string(names[n]) +
                                      (details->spells[n] ? "  可用" : "  未学会"),
                                  16, 121 + n * 22, ink, 10);
                }
                DrawRectangle(14, 232, 96, 24, {219, 232, 204, 255});
                DrawRectangle(128, 232, 96, 24, {219, 232, 204, 255});
                font.text("装备礼物", 23, 238, ink, 11);
                font.text("转职", 164, 238, ink, 11);
                font.text("返回", 18, 272);
            } else if ((page->legacy_page == 61 || page->legacy_page == 62) &&
                       startup_world_human_page_ready(state, page->id)) {
                const auto &list = state.human_page_catalogs.at(page->id);
                const int chosen = state.human_page_selections.at(page->id);
                font.text(page->legacy_page == 61 ? "职业一览" : "转职确认", 16, 48);
                if (page->legacy_page == 61) {
                    const int first = std::max(0, chosen - 4);
                    for (int n = first; n < std::min(static_cast<int>(list.size()), first + 5);
                         ++n) {
                        const auto &job = rules.jobs.at(list[n]);
                        const int y = 74 + (n - first) * 27;
                        if (n == chosen)
                            DrawRectangle(12, y, 216, 26, {219, 232, 204, 255});
                        font.text(job.name, 16, y + 3, ink, 11);
                        font.text(std::to_string(job.change_points) + "点 / " +
                                      std::to_string(job.required_medals) + "勋章",
                                  137, y + 4, ink, 9);
                    }
                    font.text("村子点 " + std::to_string(state.village_points), 16, 222, ink, 10);
                } else {
                    const auto &job = rules.jobs.at(list.at(chosen));
                    font.text(job.name, 16, 76);
                    constexpr const char *labels[]{"体力", "力量", "灵活", "结实", "魔力", "运气"};
                    for (int n = 0; n < 6; ++n)
                        font.text(std::string(labels[n]) + " " +
                                      std::to_string(state.human_attribute_display[0][n]) + " > " +
                                      std::to_string(state.human_attribute_display[1][n]),
                                  16, 100 + n * 18, ink, 10);
                    font.text(std::to_string(job.change_points) + "点 / " +
                                  std::to_string(job.required_medals) + "勋章",
                              16, 222, ink, 10);
                }
                font.text("返回", 18, 272);
                font.text("确定", 190, 272);
            } else if ((page->legacy_page == 64 || page->legacy_page == 65 ||
                        page->legacy_page == 73) &&
                       startup_world_human_page_ready(state, page->id)) {
                DrawRectangle(8, 42, 224, 214, paper);
                const int slot = state.page_phases.at(page->id);
                constexpr const char *tabs[]{"武器", "防具1", "防具2", "饰品", "道具"};
                for (int n = 0; n < 5; ++n) {
                    if (slot == n)
                        DrawRectangle(8 + n * 45, 45, 45, 18, {219, 232, 204, 255});
                    font.text(tabs[n], 12 + n * 45, 48, ink, 10);
                }
                const int kind = slot == 0 ? 1 : slot == 3 ? 3 : 2;
                const auto show_equipment = [&](int item, float y) {
                    const auto found = std::find_if(
                        rules.equipment.begin(), rules.equipment.end(),
                        [&](const auto &e) { return e.shop.kind == kind && e.shop.id == item; });
                    if (found == rules.equipment.end())
                        throw std::runtime_error("装备定义缺失");
                    font.text(found->name, 16, y, ink, 11);
                    const int stock = state.catalog.at({kind, item}).free_purchases;
                    const int quote = kind == 1 ? found->shop.price * 3 / 2 : found->shop.price * 2;
                    font.text(stock > 0 ? "库存 " + std::to_string(stock)
                                        : std::to_string(quote) + "G",
                              166, y, ink, 9);
                    return found->shop.combat;
                };
                if (page->legacy_page == 64) {
                    const int chosen = state.human_page_selections.at(page->id),
                              first = std::max(0, chosen - 4);
                    const auto &list = state.equipment_page_catalogs.at(page->id)[slot];
                    for (int n = first; n < std::min(static_cast<int>(list.size()), first + 5);
                         ++n) {
                        const float y = 78 + (n - first) * 27;
                        if (n == chosen)
                            DrawRectangle(12, y - 4, 216, 26, {219, 232, 204, 255});
                        if (slot == 4) {
                            const auto icon=startup_world_item_icon_draws(state,list[n]);
                            if (!icon) throw std::runtime_error("赠礼道具图标载荷非法");
                            for (const auto &plan:*icon) sprites.visual(plan,{18,y});
                            font.text(rules.items.at(list[n]).name, 40, y, ink, 11);
                            font.text("持有 " + std::to_string(state.items.at(list[n]).inventory),
                                      166, y, ink, 9);
                        } else
                            show_equipment(list[n], y);
                    }
                    if (slot != 4)
                        font.text("详情", 100, 272);
                } else {
                    const int item = page->legacy_page == 65
                                         ? state.human_equipment_choices.at(page->id)[1]
                                         : state.equipment_page_catalogs.at(page->id)[slot].at(
                                               state.human_page_selections.at(page->id));
                    if (slot == 4) {
                        font.text(rules.items.at(item).name, 16, 82);
                        font.text("持有 " + std::to_string(state.items.at(item).inventory), 16,
                                  112);
                    } else {
                        const auto stats = show_equipment(item, 82);
                        constexpr const char *labels[]{"HP", "攻击", "防御", "魔法"};
                        for (int n = 0; n < 4; ++n)
                            font.text(std::string(labels[n]) + " " + std::to_string(stats[n]), 16,
                                      112 + n * 22, ink, 11);
                    }
                    if (page->legacy_page == 65)
                        font.text("赠送", 16, 216);
                }
                font.text("返回", 18, 272);
                font.text("确定", 190, 272);
            } else if ((page->legacy_page == 63 || page->legacy_page == 66 ||
                        page->legacy_page == 68 || page->legacy_page == 69 ||
                        page->legacy_page == 70) &&
                       startup_world_human_page_ready(state, page->id)) {
                DrawRectangle(8, 42, 224, 214, paper);
                const int human = state.page_human_bindings.at(page->id);
                font.text(human_name(human), 16, 66);
                if (const auto portrait = startup_world_portrait(state, human))
                    sprites.portrait(*portrait, {190, 68});
                const int raw = page->legacy_page;
                font.text(raw == 63   ? "转职"
                          : raw == 66 ? "礼物"
                          : raw == 68 ? "装备能力"
                          : raw == 69 ? "道具效果"
                                      : "职业大师",
                          16, 95);
                if (raw == 68) {
                    constexpr const char *labels[]{"HP", "攻击", "防御", "魔法"};
                    for (int n = 0; n < 4; ++n)
                        font.text(std::string(labels[n]) + " " +
                                      std::to_string(state.equipment_attribute_display[0][n]) +
                                      " > " +
                                      std::to_string(state.equipment_attribute_display[1][n]),
                                  16, 126 + n * 22, ink, 10);
                } else if (raw == 69) {
                    constexpr const char *labels[]{"体力", "力量", "灵活", "结实", "魔力", "运气"};
                    for (int n = 0; n < 6; ++n)
                        font.text(std::string(labels[n]) + " " +
                                      std::to_string(state.human_attribute_display[0][n]) + " > " +
                                      std::to_string(state.human_attribute_display[1][n]),
                                  16, 121 + n * 21, ink, 10);
                } else if (raw == 70) {
                    font.text(
                        rules.jobs.at(world.ai.growth.at(human).definition.current_profession).name,
                        16, 126);
                    if (state.page_phases.at(page->id) == 2) {
                        const int selected = state.human_page_selections.at(page->id);
                        DrawRectangle(selected == 0 ? 14 : 128, 225, 98, 24, {219, 232, 204, 255});
                        font.text("转职", 42, 232, ink, 11);
                        font.text("返回", 156, 232, ink, 11);
                    }
                } else if (state.human_gift_messages.count(page->id))
                    font.text(state.human_gift_messages.at(page->id), 16, 126);
                font.text("确定", 190, 272);
            } else if (page->legacy_page == 90) {
                const auto view = inspect_startup_world_tax_page(state, page->id);
                if (!view)
                    throw std::runtime_error("税收详情缺失");
                font.text("居民税收", 16, 48);
                for (int n = view->first_visible;
                     n < std::min(static_cast<int>(view->rows.size()), view->first_visible + 5);
                     ++n) {
                    const float y = 78 + (n - view->first_visible) * 27;
                    if (n == view->selection)
                        DrawRectangle(12, y - 4, 216, 26, {219, 232, 204, 255});
                    font.text(human_name(view->rows[n].definition), 16, y, ink, 11);
                    font.text(std::to_string(view->rows[n].amount) + "G", 164, y, ink, 10);
                }
                font.text("合计 " + std::to_string(view->total) + "G", 16, 226, ink, 11);
                font.text("确定", 190, 272);
            } else if (page->legacy_page == 81) {
                font.text("设施升级", 17, 66);
                for (int n = 0; n < 3; ++n)
                    font.text(std::to_string(state.facility_upgrade_display[0][n]) + " > " +
                                  std::to_string(state.facility_upgrade_display[1][n]),
                              17, 100 + n * 24);
                font.text("确定", 190, 272);
            } else if (page->legacy_page == 50) {
                font.text("城镇等级 " + std::to_string(state.rank), 17, 66);
                const auto cast = state.rank_celebration_participants.find(page->id);
                if (cast != state.rank_celebration_participants.end())
                    for (std::size_t n = 0; n < cast->second.size(); ++n)
                        font.text(human_name(cast->second[n][0]), 17 + (n % 2) * 110,
                                  100 + (n / 2) * 24);
                font.text("确定", 190, 272);
            } else if (page->legacy_page == 48 || page->legacy_page == 49) {
                font.text("城镇等级 " + std::to_string(state.rank), 17, 66);
                if (page->legacy_page == 48) {
                    DrawRectangle(12, rank_selection == 0 ? 77 : 101 + (rank_selection - 1) * 24,
                                  216, 20, {219, 232, 204, 255});
                    font.text("晋级申请", 17, 80);
                    font.text("返回", 18, 272);
                }
                if (state.rank < 5) {
                    static const std::array<std::string, 7> labels{{"月收入", "设施数", "居住数",
                                                                    "指定建设", "任务完成数",
                                                                    "街道人气", "举办活动数"}};
                    const auto terms = ref::fixed_calendar_task_rank_terms().at(state.rank);
                    for (int n = 0; n < 4; ++n)
                        font.text(labels.at(terms[n].type) + " " +
                                      std::to_string(state.rank_values[n]) +
                                      (state.rank_met[n] ? " 达成" : " 未达成"),
                                  17, 104 + n * 24);
                }
                font.text("确定", 190, 272);
            } else if (page->legacy_page == 67 || page->legacy_page == 88 ||
                       page->legacy_page == 96) {
                if (state.page_human_bindings.count(page->id))
                    font.text(human_name(state.page_human_bindings.at(page->id)), 17, 66);
                font.text(page->legacy_page == 67   ? "能力上升"
                          : page->legacy_page == 88 ? "勋章授予"
                                                    : "自宅完成",
                          17, 90);
                for (int n = 0; n < 2; ++n)
                    font.text(std::string(n == 0 ? "满足 " : "努力 ") +
                                  std::to_string(state.reward_display[0][n]) + " > " +
                                  std::to_string(state.reward_display[1][n]),
                              17, 118 + n * 24);
                font.text("确定", 190, 272);
            } else if (page->legacy_page == 33) {
                DrawRectangle(8, 42, 224, 126, paper);
                if (page->task_definition)
                    font.paragraph(rules.tasks.at(*page->task_definition).name, 17, 65, 204);
                font.text("延长费用 " + std::to_string(page->legacy_f) + "G", 17, 100);
                const auto grade = state.deadline_grades.find(page->id);
                if (grade != state.deadline_grades.end()) {
                    static const std::string grades[]{"再加把劲!!", "再加加油!!", "需要补充战力!",
                                                      "重新来过比较好", "应该撤退…"};
                    font.text(grades[grade->second], 17, 122);
                }
                const auto phase = state.page_phases.find(page->id);
                const bool animating = phase != state.page_phases.end() && phase->second == 1;
                if (animating) {
                    const auto counter = state.page_counters.find(page->id);
                    const float ratio = counter == state.page_counters.end()
                                            ? 0.0F
                                            : std::clamp(counter->second / 50.0F, 0.0F, 1.0F);
                    DrawRectangle(24, 199, 192, 8, GRAY);
                    DrawRectangle(24, 199, static_cast<int>(192 * ratio), 8, GREEN);
                } else {
                    DrawRectangle(task_selection == 0 ? 24 : 120, 194, 96, 28,
                                  Color{219, 232, 204, 255});
                    font.text("继续", 52, 202);
                    font.text("中止", 148, 202);
                }
                font.text("确定", 190, 270);
            } else if (page->legacy_page >= 22 && page->legacy_page <= 28) {
                DrawRectangle(8, 42, 224, 126, paper);
                std::vector<std::string> rows;
                if (page->legacy_page == 22) {
                    for (const auto id : state.task_page_lists.at(page->id))
                        rows.push_back(rules.tasks.at(state.tasks.at(id).definition).name);
                } else if (page->legacy_page == 25 || page->legacy_page == 26 ||
                           page->legacy_page == 27) {
                    const auto &list = page->legacy_page != 27
                                           ? state.participants
                                           : state.task_extra_pages.at(page->id);
                    for (const auto id : list) {
                        auto name = human_name(id);
                        if (page->legacy_page == 27)
                            name += " " +
                                    std::to_string(state.human_calendar.at(id).continuation_cost) +
                                    "G";
                        rows.push_back(name);
                    }
                    if (page->legacy_page == 25 || page->legacy_page == 26)
                        rows.emplace_back("追加队员");
                }
                for (int row = 0; row < 5 && row + task_scroll < static_cast<int>(rows.size());
                     ++row) {
                    const auto &text = rows[row + task_scroll];
                    const int y = 65 + row * 20;
                    if (row + task_scroll == task_selection)
                        DrawRectangle(12, y - 3, 216, 19, Color{219, 232, 204, 255});
                    font.text(text, 17, y, ink, font.measure(text) > 204 ? 10 : 12);
                }
                if (page->task_definition && (page->legacy_page == 23 || page->legacy_page == 28)) {
                    const auto &task = rules.tasks.at(*page->task_definition);
                    font.paragraph(task.name, 17, 65, 204);
                    font.text(page->legacy_page == 23
                                  ? "征集费 " + std::to_string(task.recruitment_fee) + "G"
                                  : "队伍评价 " +
                                        std::to_string(state.task_page_predictions.at(page->id)),
                              17, 104);
                }
                if (page->legacy_page == 24) {
                    const auto &animation = state.task_recruitment_pages.at(page->id);
                    font.text(std::to_string(animation.displayed_count) + "人", 17, 66);
                    const float ratio =
                        std::clamp(state.page_counters.at(page->id) /
                                       static_cast<float>(animation.completion_tick),
                                   0.0F, 1.0F);
                    DrawRectangle(17, 100, 198, 7, GRAY);
                    DrawRectangle(17, 100, static_cast<int>(198 * ratio), 7, GREEN);
                    if (!animation.portraits.empty()) {
                        const auto &human = rules.humans.at(animation.portraits.front());
                        font.text(human_name(human.identity), 17, 118);
                        const auto portrait = startup_world_portrait(state, human.identity);
                        if (!portrait)
                            throw std::runtime_error("征集头像资料非法");
                        sprites.actor(false, 0, portrait->image, 0, {196, 141});
                    }
                }
                if (page->legacy_page != 24)
                    font.text("取消", 18, 270);
                if ((page->legacy_page == 25 || page->legacy_page == 26) &&
                    task_selection < static_cast<int>(state.participants.size()))
                    font.text("详情", 100, 270);
                font.text(page->legacy_page == 25
                              ? task_selection == static_cast<int>(state.participants.size())
                                    ? "追加"
                                    : "出发"
                          : page->legacy_page == 26 &&
                                  task_selection == static_cast<int>(state.participants.size())
                              ? "追加"
                              : "确定",
                          190, 270);
            } else if (page->legacy_page == 83)
                font.text("取消", 18, 270);
            else if (page->legacy_page != 97 && page->legacy_page != 72 &&
                     page->legacy_page != 79 && page->legacy_page != 82 &&
                     (page->legacy_page < 41 || page->legacy_page > 47) &&
                     (page->legacy_page != 59 || (state.page_counters.count(page->id) &&
                                                  state.page_counters.at(page->id) >= 70)))
                font.text("确定", 190, 270);
            if ((page->legacy_page == 99 || page->legacy_page == 100) && page->monster_definition) {
                const auto &monster = world.ai.monster_growth.at(*page->monster_definition);
                sprites.actor(true, monster.body * 4 + 1,
                              monster.body * 30 + monster.sprite_variant, 0, {120, 128});
            }
        }
        if (!top_page() && state.scene.scene_state == 0 && !state.scene.framework_paused) {
            DrawRectangle(176, 23, 60, 20, paper);
            font.text("村办 V", 180, 27, ink, 10);
            if ((state.scripts.user_flags & 16U) != 0) {
                DrawRectangle(176, 46, 60, 20, paper);
                font.text("商会 S", 180, 50, ink, 10);
            }
            if (available_road(state)) {
                DrawRectangle(176, 69, 60, 20, paper);
                font.text("道路 D", 180, 73, ink, 10);
            }
            DrawRectangle(176, 92, 60, 20, paper);
            font.text("移动 M", 180, 96, (state.scripts.user_flags & 32U) ? ink : GRAY, 10);
            DrawRectangle(176, 115, 60, 20, paper);
            font.text("撤除 Del", 178, 119, ink, 10);
            if ((state.scripts.user_flags & 1U) != 0) {
                DrawRectangle(176, 138, 60, 20, paper);
                font.text("魔法壶 P", 178, 142, ink, 10);
            }
            if (!command_feedback.empty()) {
                DrawRectangle(5, 265, 230, 24, paper);
                font.text(command_feedback, 12, 270, ink, 11);
            }
        } else if (!top_page() && state.scene.scene_state == 1 &&
                   (state.build_mode == 0 || state.build_mode == 7)) {
            DrawRectangle(176, 46, 60, 20, paper);
            font.text("旋转 R", 178, 50, ink, 10);
        }
        DrawRectangle(0, 294, width, 26, paper);
        font.text(state.scene.framework_paused ? "继续" : "暂停", 8, 301);
        font.text(state.scene.speed_setting == 1 ? "2倍" : "1倍", 193, 301);
        font.text(state.scene.scene_state == 1 ? "返回" : "菜单", 60, 301);
        font.text("任务", 121, 301);
        EndTextureMode();
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(canvas.value.texture, {0, 0, width, -height},
                       {0, 0, width * scale, height * scale}, {0, 0}, 0, WHITE);
        EndDrawing();
        ++frame_count;
        if (frames > 0 && frame_count >= frames) {
            if (screenshot) {
                if (!screenshot->parent_path().empty())
                    std::filesystem::create_directories(screenshot->parent_path());
                auto image = LoadImageFromScreen();
                if (!image.data)
                    throw std::runtime_error("共同世界窗口像素读取失败");
                auto *pixels = LoadImageColors(image);
                if (!pixels) {
                    UnloadImage(image);
                    throw std::runtime_error("共同世界窗口像素解码失败");
                }
                std::set<std::uint32_t> colors;
                // 验证地图区，排除HUD、事件正文和月报主体，防止仅有文字也被当作非空地图。
                for (int y = 160 * scale; y < 170 * scale; ++y)
                    for (int x = 0; x < image.width; ++x) {
                        const auto color = pixels[y * image.width + x];
                        colors.insert((static_cast<std::uint32_t>(color.r) << 16) |
                                      (static_cast<std::uint32_t>(color.g) << 8) | color.b);
                    }
                UnloadImageColors(pixels);
                world_pixel_colors = colors.size();
                if (world_pixel_colors < 8) {
                    UnloadImage(image);
                    throw std::runtime_error("共同世界窗口地图像素疑似空白");
                }
                const bool saved = ExportImage(image, screenshot->string().c_str());
                UnloadImage(image);
                if (!saved)
                    throw std::runtime_error("共同世界截图导出失败");
            }
            break;
        }
    }
    if (save_file) {
        const auto result = save_startup_world_file(*save_file, session, file_metadata);
        if (!result.ok) throw std::runtime_error("保存失败：" + result.error);
        std::cout << "normal save written: " << save_file->string() << '\n';
    }
    std::cout << "world window closed: frames=" << frame_count
              << " updates=" << session.state().scene.world.updates
              << " actors=" << session.state().scene.world.world.ai.human_order.size()
              << " random_draws=" << session.state().scene.random.draws()
              << " funds=" << session.state().scene.world.world.ai.accounting.funds()
              << " moved_frames=" << moved_frames << " map_colors=" << world_pixel_colors
              << " confirm_inputs=" << confirmation_inputs << " pause_inputs=" << pause_inputs
              << " speed_inputs=" << speed_inputs
              << " paused=" << session.state().scene.framework_paused
              << " speed=" << session.state().scene.speed_setting << '\n';
    return 0;
}
} // namespace dungeon_village_prototype
