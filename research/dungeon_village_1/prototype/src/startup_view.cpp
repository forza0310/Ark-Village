#include "dungeon_village_prototype/startup_view.hpp"
#include "startup_window_host.hpp"
#include "dungeon_village_prototype/road_render.hpp"
#include "dungeon_village_prototype/startup.hpp"
#include "dungeon_village_prototype/startup_world_building.hpp"
#include "dungeon_village_prototype/startup_world_commerce.hpp"
#include "dungeon_village_prototype/startup_world_editing.hpp"
#include "dungeon_village_prototype/startup_world_facility_items.hpp"
#include "dungeon_village_prototype/startup_world_facility_catalog.hpp"
#include "dungeon_village_prototype/startup_world_magic_pot.hpp"
#include "dungeon_village_prototype/steam_magic_pot_skin.hpp"
#include "dungeon_village_prototype/steam_magic_pot_effect_skin.hpp"
#include "dungeon_village_prototype/steam_task_selection_skin.hpp"
#include "dungeon_village_prototype/steam_task_roster_skin.hpp"
#include "dungeon_village_prototype/steam_task_result_skin.hpp"
#include "dungeon_village_prototype/steam_task_deadline_skin.hpp"
#include "dungeon_village_prototype/steam_clear_score_skin.hpp"
#include "dungeon_village_prototype/steam_new_game_skin.hpp"
#include "dungeon_village_prototype/steam_config_skin.hpp"
#include "dungeon_village_prototype/steam_commerce_skin.hpp"
#include "dungeon_village_prototype/steam_platform_button_skin.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"
#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include "dungeon_village_prototype/startup_world_persistence.hpp"
#include "dungeon_village_prototype/startup_world_runtime_tasks.hpp"
#include "dungeon_village_prototype/startup_world_menu.hpp"
#include "dungeon_village_prototype/startup_world_save.hpp"
#include "dungeon_village_prototype/startup_world_manual.hpp"
#include "dungeon_village_prototype/steam_manual_skin.hpp"
#include "dungeon_village_prototype/startup_title_actor_skin.hpp"
#include "dungeon_village_prototype/startup_world_information.hpp"
#include "dungeon_village_prototype/steam_main_menu_skin.hpp"
#include "dungeon_village_prototype/steam_information_skin.hpp"
#include "dungeon_village_prototype/startup_world_tax.hpp"
#include "dungeon_village_prototype/startup_world_village_activity.hpp"
#include "dungeon_village_prototype/startup_world_visuals.hpp"
#include "dungeon_village_prototype/steam_facility_skin.hpp"
#include "dungeon_village_prototype/steam_human_skin.hpp"
#include "dungeon_village_prototype/steam_facility_catalog_skin.hpp"
#include "dungeon_village_prototype/steam_facility_effect_skin.hpp"
#include "dungeon_village_prototype/steam_facility_reputation_skin.hpp"
#include "dungeon_village_prototype/steam_village_activity_skin.hpp"
#include "dungeon_village_prototype/steam_town_progress_skin.hpp"
#include "dungeon_village_reference/world_notices.hpp"
#include "dungeon_village_reference/world_village_activity.hpp"
#include "dungeon_village_tools/sprite.hpp"
#include "dungeon_village_tools/table.hpp"

#include <raylib.h>

#include <algorithm>
#include <type_traits>
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
        if(platform_blend_)UnloadShader(*platform_blend_);
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
              bool published = false, std::optional<Vector2> extent = {},
              std::array<bool,2> flip = {}) {
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
        const Rectangle painted{source.x,source.y,flip[0]?-source.width:source.width,
                                flip[1]?-source.height:source.height};
        if(extent) {
            if(!std::isfinite(extent->x) || !std::isfinite(extent->y) || extent->x<=0 || extent->y<=0)
                throw std::runtime_error("原图缩放尺寸拒绝");
            DrawTexturePro(found->second,painted,{position.x,position.y,extent->x,extent->y},{0,0},0,WHITE);
        } else DrawTexturePro(found->second,painted,{position.x,position.y,source.width,source.height},{0,0},0,WHITE);
    }
    // 原arrow02单层单部件边界；说明用左右帧，人物目录另用上下帧。
    std::array<int,4> manual_arrow_bounds(int frame) {
        if(frame<0 || frame>3)throw std::runtime_error("箭头边界帧拒绝");
        const auto key=std::string("common/arrow02.seb");
        auto found=sprites_.find(key);
        if(found==sprites_.end())found=sprites_.emplace(key,dungeon_village_tools::parse_legacy_seb(read_bytes(root_/key))).first;
        if(found->second.layers.size()!=1)throw std::runtime_error("说明箭头边界源层数变化");
        const dungeon_village_tools::SpritePart *part{};
        for(const auto &candidate:found->second.layers.front().parts)if(candidate.frame==frame) {
            if(part)throw std::runtime_error("说明箭头边界出现复合部件");
            part=&candidate;
        }
        if(!part || part->image_index!=74 || part->flip_x || part->flip_y)
            throw std::runtime_error("说明箭头边界源拒绝");
        return {part->offset_x,part->offset_y,part->width,part->height};
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
        for (const auto &digit : *digits) facility_image(digit,origin);
    }
    // 正式Steam资源绑定；SEB偏移恰一次，不能借common槽回落到APK差异PNG。
    void facility_image(const SteamFacilityImage &request,Vector2 origin) {
        const auto resource=steam_facility_resource(request.asset);
        if(!resource)throw std::runtime_error("设施Steam资源映射缺失");
        published_sprite(resource->image,resource->published_image,resource->published_sprite,
                         request.position,request.frame,request.crop,origin);
    }
    void human_image(const SteamHumanImage &request,Vector2 origin) {
        const auto resource=steam_human_resource(request.asset);
        if(!resource)throw std::runtime_error("人物Steam资源映射缺失");
        published_sprite(resource->image,resource->published_image,resource->published_sprite,
                         request.position,request.frame,request.crop,origin);
    }
    void platform_button(const SteamPlatformButtonBody &button) {
        if(!button.reset_origin||button.joystick)throw std::runtime_error("平台按钮附加请求尚未完整消费");
        const auto paint=[&] {
            const auto &c=button.resource.crop;const auto &d=button.destination;
            crop(button.resource.image_path,{float(c[0]),float(c[1]),float(c[2]),float(c[3])},
                 {d[0],d[1]},true,Vector2{d[2],d[3]});
        };
        if(!button.blend){paint();return;}
        if(button.blend->mode!=1||button.blend->rgba!=std::array<int,4>{255,255,255,128})
            throw std::runtime_error("平台按钮混色尚未支持");
        if(!platform_blend_) {
            // 已核原生成shader三层混色的研究GLSL适配；保留纹理alpha，不能用白色半透明tint。
            // 不宣称等价于当前Steam运行时选中的compiled shader或GPU色域。
            const auto shader=LoadShaderFromMemory(nullptr,R"GLSL(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float platformBlendAlpha;
out vec4 finalColor;
void main() {
    vec4 texel=texture(texture0,fragTexCoord);
    vec3 rgb=mix(texel.rgb,vec3(texel.a),platformBlendAlpha);
    finalColor=vec4(rgb,texel.a)*colDiffuse*fragColor;
}
)GLSL");
            const int location=GetShaderLocation(shader,"platformBlendAlpha");
            if(!IsShaderValid(shader)||location<0) {
                UnloadShader(shader);
                throw std::runtime_error("平台按钮混色shader无法创建，拒绝默认shader回落");
            }
            const float alpha=128.F/255.F;
            SetShaderValue(shader,location,&alpha,SHADER_UNIFORM_FLOAT);
            platform_blend_=shader;
        }
        BeginShaderMode(*platform_blend_);
        try {paint();} catch(...) {EndShaderMode();throw;}
        EndShaderMode();
    }
    void published_sprite(int image_id,const char *png,const char *seb,std::array<int,2> anchor,
                          int frame,std::optional<std::array<int,4>> rectangle,Vector2 origin) {
        const Vector2 position{origin.x+anchor[0],origin.y+anchor[1]};
        if(rectangle) {
            const auto &c=*rectangle;
            if(!c[2]||!c[3])return; // DrawWindow整40宽尾项保留空请求，无像素。
            crop(png,{float(c[0]),float(c[1]),float(c[2]),float(c[3])},position,true);
            return;
        }
        if(!seb) {
            // 具名映射明确无SEB的NEW／Lv／MAX等原DrawImage，直接消费整PNG。
            if(frame!=0)throw std::runtime_error("整图资源不能选择SEB帧");
            image(std::filesystem::path(png).lexically_relative("original"),position);
            return;
        }
        const auto key=std::string(seb);
        auto found=sprites_.find(key);
        if(found==sprites_.end())found=sprites_.emplace(key,dungeon_village_tools::parse_legacy_seb(
            read_bytes(root_.parent_path()/key))).first;
        // 原SEB缺帧不绘；保留请求，不补猜测减号或逗号。
        for(const auto &layer:found->second.layers)for(const auto &part:layer.parts) {
            if(part.frame!=frame||part.image_index<0||part.width<=0||part.height<=0)continue;
            if(part.image_index!=image_id||part.flip_x<0||part.flip_x>1||part.flip_y<0||part.flip_y>1)
                throw std::runtime_error("正式SEB图片身份/翻转无效");
            crop(png,{float(part.source_x),float(part.source_y),float(part.width),float(part.height)},
                 {position.x+part.offset_x,position.y+part.offset_y},true,{},
                 {part.flip_x!=0,part.flip_y!=0});
        }
    }

  private:
    std::filesystem::path root_;
    std::map<int, std::filesystem::path> images_;
    std::map<std::string, std::map<int, std::filesystem::path>> actor_images_;
    std::map<std::string, std::vector<std::string>> actor_sprites_;
    std::map<std::string, dungeon_village_tools::SpriteDefinition> sprites_;
    std::map<std::string, Texture2D> textures_;
    std::optional<Shader> platform_blend_;
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
                             "设施情报品质魅力维护费收入建设完毕加成返回图范围"
                             "信息距下一等级还有达到最高级周围设施的奖励没有使用道具"
                             "按确认键赠送礼物习得魔法精通特定职业可以一种两种三种全部"
                             "南瓜商会欢迎光临购买出售道具设施所持金了解已价格可使用村子点数";
        for (int i = 32; i < 127; ++i)
            glyphs += static_cast<char>(i);
        for (const auto &v : startup_evidence().definitions)
            glyphs += v.name;
        // 商会包含尚未建造的定义，不能仅用开局已见设施裁字体字符集。
        for (const auto &v : startup_world_rules().facilities)
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
    // 仅执行本批原manual实际使用的br/co标签，保持真实测宽；不是完整Unity TextLayout。
    void manual_text(const std::string &value,float x,float y,float max_width,
                     float size,int spacing,Color base) const {
        Color current=base;
        std::string line;
        for(std::size_t at=0;at<value.size();) {
            if(value.compare(at,4,"<br>")==0 || value[at]=='\n') {
                at+=value[at]=='\n'?1:4;line.clear();y+=size+spacing;continue;
            }
            if(value.compare(at,5,"</co>")==0) {current=base;at+=5;continue;}
            if(value.compare(at,4,"<co=")==0) {
                const auto end=value.find('>',at+4);
                if(end!=at+10)throw std::runtime_error("说明颜色标签尚未接入");
                const auto digits=value.substr(at+4,6);
                if(digits.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos)
                    throw std::runtime_error("说明颜色参数拒绝");
                const auto rgb=std::stoul(digits,nullptr,16);
                current={static_cast<unsigned char>(rgb>>16),static_cast<unsigned char>(rgb>>8),
                         static_cast<unsigned char>(rgb),255};at=end+1;continue;
            }
            if(value[at]=='<')throw std::runtime_error("说明出现未支持的文字标签");
            const auto lead=static_cast<unsigned char>(value[at]);
            const auto next=value.substr(at,lead<128?1:lead<224?2:lead<240?3:4);
            if(!line.empty() && measure(line+next,size)>max_width) {line.clear();y+=size+spacing;}
            text(next,x+measure(line,size),y,current,size);line+=next;at+=next.size();
        }
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
    case 20:return "保存";case 21:return "设置变更";case 22:return "游戏方法";case 23:return "排行榜";
    case 24:return "结束游戏";default:throw std::runtime_error("菜单标签缺文字适配");
    }
}
struct WindowNavigationPlan {
    std::array<int,2> origin{};
    std::optional<SteamMainMenuSkinPlan> main;
    std::optional<SteamInformationSkinPlan> system;
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
        SteamNavigationMenuSkinOptions options;options.canvas={width,height};options.origin=result.origin;
        options.on_top=on_top;
        if(view->frame==3) {
            std::vector<int> widths;
            for(const int tag:view->tags)
                widths.push_back(static_cast<int>(font.measure(menu_label(tag))));
            options.measured_text_widths=std::move(widths);
        }
        result.system=steam_navigation_menu_skin(state,id,options);
        if(!result.system)return {};
        for(const auto &touch:result.system->touches)result.touches.push_back(touch);
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
}
std::optional<SteamStartupSkinPlan> window_save_plan(const StartupWorldSavePageView &view,
                                                    const ChineseFont &font) {
    SteamSavePageSkinInput input;
    input.width=width;input.height=height;input.stage=view.stage;
    if(view.stage==2)input.measured_button_width=font.measure("了解");
    return steam_save_page_skin(input);
}
// 执行正式raw14图元；12号Noto字体及中文译文是研究窗口适配，不认证Unity字体。
// 普通同字号正文按原TextLayout中心对齐；标签、混合字号及fallback仍由后续后端接入。
void draw_save_plan(const SteamStartupSkinPlan &plan,const StartupWorldSavePageView &view,
                    SourceSprites &sprites,const ChineseFont &font) {
    const Vector2 origin{float(plan.origin[0]),float(plan.origin[1])};
    // GameForm..cctor的已核静态初值；只由Window/Box使用，正文和按钮不再加此偏移。
    constexpr int initial_view_y=23;
    const auto color=[](const std::array<int,3> &rgb) {
        return Color{static_cast<unsigned char>(rgb[0]),static_cast<unsigned char>(rgb[1]),
                     static_cast<unsigned char>(rgb[2]),255};
    };
    const auto label=[&](const SteamStartupText &part) {
        if(!part.rgb)throw std::runtime_error("保存页文字缺来源颜色");
        const std::string value=part.role==SteamStartupTextRole::message_title ? "消息"
            : part.role==SteamStartupTextRole::message_body ? view.text : "了解";
        const auto &r=part.rectangle;
        const float size=part.font_size?float(part.font_size):12.F;
        if(part.placement==SteamStartupTextPlacement::layout) {
            std::vector<std::string> lines;
            std::string line;
            for(std::size_t at=0;at<value.size();) {
                const auto lead=static_cast<unsigned char>(value[at]);
                const std::size_t count=lead<128?1:lead<224?2:lead<240?3:4;
                const auto next=value.substr(at,count);at+=count;
                if(next=="\n") { lines.push_back(std::move(line));line.clear();continue; }
                if(!line.empty() && font.measure(line+next,size)>r[2]) {
                    lines.push_back(std::move(line));line.clear();
                }
                line+=next;
            }
            lines.push_back(std::move(line));
            const float block_height=float(lines.size())*size+float(lines.size()-1)*part.line_space;
            float y=origin.y+float(r[1])+std::max(0.F,(float(r[3])-block_height)/2);
            for(const auto &row:lines) {
                font.text(row,origin.x+float(r[0])+(float(r[2])-font.measure(row,size))/2,
                          y,color(*part.rgb),size);
                y+=size+part.line_space;
            }
        } else {
            const float x=origin.x+float(r[0])-(part.anchor==2?font.measure(value,size)/2:0.F);
            font.text(value,x,origin.y+float(r[1]),color(*part.rgb),size);
        }
    };
    const auto frame=[&](const std::optional<SteamStartupFramePlan> &parts) {
        if(!parts)throw std::runtime_error("保存页源窗框拒绝");
        for(const auto &part:parts->draws) {
            if(const auto *rect=std::get_if<StartupSkinRect>(&part)) {
                const auto &r=rect->rect;
                const Rectangle bounds{origin.x+r[0],origin.y+r[1],float(r[2]),float(r[3])};
                if(rect->outline)DrawRectangleLinesEx(bounds,1,color(rect->rgb));
                else DrawRectangleRec(bounds,color(rect->rgb));
            } else if(const auto *image=std::get_if<StartupSkinDraw>(&part)) {
                if(image->package!=StartupSkinPackage::common)
                    throw std::runtime_error("保存页窗框出现未知资源组");
                if(image->sprite<0 && (!image->crop[2] || !image->crop[3]))continue;
                sprites.visual({StartupVisualResource::common,image->sprite,image->image,
                    image->frame,image->layer,image->crop,image->offset},origin);
            } else if(const auto *text=std::get_if<SteamStartupText>(&part))label(*text);
        }
    };
    for(const auto &part:plan.draws) {
        if(const auto *window=std::get_if<SteamStartupWindow>(&part))
            frame(steam_startup_window_skin(*window,initial_view_y,std::array<int,2>{
                int(font.measure("消息")),int(font.measure("消息"))}));
        else if(const auto *box=std::get_if<SteamStartupBox>(&part))frame(steam_startup_box_skin(*box,initial_view_y));
        else if(const auto *fill=std::get_if<SteamStartupFill>(&part)) {
            const auto &r=fill->rectangle;
            DrawRectangleRec({origin.x+float(r[0]),origin.y+float(r[1]),float(r[2]),float(r[3])},color(fill->rgb));
        } else if(const auto *text=std::get_if<SteamStartupText>(&part))label(*text);
        else throw std::runtime_error("保存页出现未接图元");
    }
}
std::optional<SteamManualSkinPlan> window_manual_plan(const StartupWorldRuntimeState &state,
                                                     const StartupManualPageView &view) {
    if(!state.rules)return {};
    SteamManualSkinInput input;input.width=width;input.height=height;
    input.body_pages=static_cast<int>(state.rules->manual_pages.size());input.page=view.index;
    input.frame=view.counter;input.body_text=view.text;input.localize_text=view.localize_text;
    input.frozen_definitions=view.decorations;input.definition_count=static_cast<int>(state.rules->humans.size());
    return steam_manual_skin(input);
}
// 原图元顺序的研究执行器：Noto中文/正文标签及基础人物单独限定，不伪造平台scratch历史。
void draw_manual_plan(const SteamManualSkinPlan &plan,const StartupWorldRuntimeState &state,
                      SourceSprites &sprites,const ChineseFont &font) {
    const Vector2 origin{float(plan.origin[0]),float(plan.origin[1])};
    const auto color=[](const std::array<int,3> &rgb) {
        return Color{static_cast<unsigned char>(rgb[0]),static_cast<unsigned char>(rgb[1]),
                     static_cast<unsigned char>(rgb[2]),255};
    };
    const auto rectangle=[&](const StartupSkinRect &part) {
        const auto &r=part.rect;const Rectangle bounds{origin.x+r[0],origin.y+r[1],float(r[2]),float(r[3])};
        if(part.outline)DrawRectangleLinesEx(bounds,1,color(part.rgb));
        else DrawRectangleRec(bounds,color(part.rgb));
    };
    const auto image=[&](const StartupSkinDraw &part) {
        if(part.package!=StartupSkinPackage::common)throw std::runtime_error("说明图元资源组拒绝");
        if(part.sprite<0 && (!part.crop[2] || !part.crop[3]))return;
        sprites.visual({StartupVisualResource::common,part.sprite,part.image,part.frame,part.layer,
                        part.crop,part.offset},origin);
    };
    const auto frame=[&](const std::optional<SteamStartupFramePlan> &parts,const std::string &title) {
        if(!parts)throw std::runtime_error("说明窗框拒绝");
        for(const auto &draw:parts->draws) {
            if(const auto *r=std::get_if<StartupSkinRect>(&draw))rectangle(*r);
            else if(const auto *i=std::get_if<StartupSkinDraw>(&draw))image(*i);
            else if(const auto *t=std::get_if<SteamStartupText>(&draw)) {
                if(!t->rgb)throw std::runtime_error("说明标题缺色彩");
                font.text(title,origin.x+float(t->rectangle[0]),origin.y+float(t->rectangle[1]),color(*t->rgb));
            }
        }
    };
    bool clipped{};
    for(const auto &draw:plan.draws) {
        if(const auto *w=std::get_if<SteamManualWindow>(&draw)) {
            const auto title="游戏方法 "+std::to_string(w->page_number)+"/"+std::to_string(w->total_pages);
            frame(steam_startup_window_skin(w->source,23,std::array<int,2>{int(font.measure(title)),int(font.measure(title))}),title);
        } else if(const auto *b=std::get_if<SteamStartupBox>(&draw))frame(steam_startup_box_skin(*b,23),{});
        else if(const auto *request=std::get_if<SteamManualArrowRequest>(&draw)) {
            const auto arrow=steam_manual_arrow_skin(*request,sprites.manual_arrow_bounds(request->bounds_frame),128);
            if(!arrow)throw std::runtime_error("说明按钮效果计划拒绝");
            const auto &s=arrow->crop;const auto &d=arrow->destination;
            // 明确使用已出版mdpi原图；不声称原运行时当前选择的资源密度已核。
            sprites.crop("steam-common/buttoneffect.png",{float(s[0]),float(s[1]),float(s[2]),float(s[3])},
                         {origin.x+d[0],origin.y+d[1]},true,Vector2{d[2],d[3]});
        } else if(const auto *t=std::get_if<SteamManualText>(&draw)) {
            const auto &r=t->rectangle;
            const float size=t->font_size?float(t->font_size):12.F;
            if(t->role==SteamManualTextRole::body)
                font.manual_text(t->body,origin.x+r[0],origin.y+r[1],float(r[2]),size,t->line_space,color(t->rgb));
            else {
                const std::string value=t->role==SteamManualTextRole::about?"关于本游戏":
                    t->role==SteamManualTextRole::copyright?"(C)开罗软件":
                    t->trial_version?"冒险迷宫村 体验版":"冒险迷宫村";
                font.text(value,origin.x+r[0]-(t->anchor==2?font.measure(value,size)/2:0.F),origin.y+r[1],color(t->rgb),size);
            }
        } else if(const auto *c=std::get_if<SteamManualClip>(&draw)) {
            if(c->push) {
                if(clipped)throw std::runtime_error("说明裁剪嵌套拒绝");
                BeginScissorMode(int(origin.x)+c->rectangle[0],int(origin.y)+c->rectangle[1],c->rectangle[2],c->rectangle[3]);
                clipped=true;
            } else {if(!clipped)throw std::runtime_error("说明裁剪栈缺失");EndScissorMode();clipped=false;}
        } else if(const auto *i=std::get_if<StartupSkinDraw>(&draw))image(*i);
        else if(const auto *a=std::get_if<SteamManualActorRequest>(&draw)) {
            const auto details=startup_world_human_details(state,a->definition);
            if(!details)throw std::runtime_error("说明人物定义缺失");
            const auto actor=startup_title_actor_skin(details->profession,details->sex,details->equipment[0].value_or(-1),
                                                     a->anime_index,a->direction,false);
            if(!actor)throw std::runtime_error("说明基础人物计划拒绝");
            const Vector2 position{origin.x+a->position[0],origin.y+a->position[1]};
            if(actor->weapon)sprites.visual(*actor->weapon,position);
            sprites.actor(false,actor->body.sprite,actor->body.image,actor->body.frame,position);
        } else if(const auto *r=std::get_if<StartupSkinRect>(&draw))rectangle(*r);
        else if(!std::holds_alternative<SteamStartupTouch>(draw))throw std::runtime_error("说明未接绘制命令");
    }
    if(clipped){EndScissorMode();throw std::runtime_error("说明裁剪未闭合");}
}
// raw74标签属于研究中文适配，字体仍非Unity后端；数值/名称由正式计划提供。
std::string facility_detail_text(const SteamFacilityDetailText &t) {
    using R=SteamFacilityDetailTextRole;
    switch(t.role) {
    case R::title:return t.argument==0?"设施信息":"设施信息 "+std::to_string(t.argument)+"/2";
    case R::name:case R::bonus_name:return t.value;
    case R::price:return "价格";
    case R::parameter:return t.argument==1?"品质":"魅力";
    case R::remaining:return "距下一等级还有";
    case R::maximum_level:return "达到最高级";
    case R::no_effects:return "---";
    case R::bonus_title:return "设施奖励";
    case R::maintenance:return "维护费";
    case R::no_bonus:return "没有奖励";
    case R::bonus_value:return std::string(t.argument==0?"价格":t.argument==1?"品质":"魅力")+t.value;
    case R::bottom:return !t.value.empty()?t.value:t.argument==0?"使用道具":"周围设施的奖励";
    case R::equipment_count:return "种类";
    case R::sale_description:case R::plant_description:case R::recruitment_status:return t.value;
    case R::plant_price:return t.argument==0?"价格":t.value;
    case R::plant_quality:return t.argument==0?"品质":t.value;
    }
    throw std::runtime_error("设施详情文字角色未消费");
}
constexpr Vector2 facility_detail_origin{0,(height-240)/2.F};
std::optional<SteamFacilityDetailSkinPlan> window_facility_detail_plan(
    const StartupWorldRuntimeState &s,std::uint64_t page,SteamFacilityBonusCursor cursor,const ChineseFont &font) {
    const auto phase=s.page_phases.find(page);
    if(phase==s.page_phases.end())return {};
    SteamFacilityDetailSkinOptions options;
    options.view_y=23; // 固定样本GameForm静态初值，不把历史VIEW_Y0条件图当默认。
    options.cursor=cursor;
    // 静态研究窗口手形使用SEB21帧0；不从页面frame猜全局SEB调度。
    options.finger_frame=0;
    const auto title="设施信息 "+std::to_string(phase->second+1)+"/2";
    options.title_widths=std::array<int,2>{int(font.measure(title)),int(font.measure(title))};
    options.bottom_width=int(font.measure(phase->second==0?"使用道具":"周围设施的奖励"));
    return steam_facility_detail_skin(s,page,options);
}
bool ordinary_facility_preview(const StartupWorldRuntimeState &s,const ref::WorldScriptPage &page) {
    if(!s.rules||page.legacy_page!=74||
       (page.legacy_g!=1&&!s.facility_definition_page_bindings.count(page.id)))return false;
    const auto d=std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),
        [&](const auto &value){return value.id==page.legacy_f;});
    return d!=s.rules->facilities.end()&&d->kind!=2&&d->kind!=12&&d->detail!=1&&
        d->detail!=4&&d->detail!=5&&d->detail!=6;
}
std::optional<SteamFacilityDetailSkinPlan> window_facility_definition_plan(
    const StartupWorldRuntimeState &s,std::uint64_t page,const ChineseFont &font) {
    SteamFacilityDetailSkinOptions options;options.view_y=23;
    const int width=int(font.measure("设施信息"));options.title_widths=std::array<int,2>{width,width};
    // 预览没有互动底钮/手形，不提供无消费者的底部测宽或假邻接游标。
    return steam_facility_definition_skin(s,page,options);
}
bool special_facility_page(const StartupWorldRuntimeState &s,const ref::WorldScriptPage &page) {
    if(!s.rules||page.legacy_page!=74)return false;
    const auto d=std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),
        [&](const auto &value){return value.id==page.legacy_f;});
    return d!=s.rules->facilities.end()&&d->kind!=12&&
        (d->detail==1||d->detail==4||d->detail==5||d->detail==6||d->kind==2);
}
// Steam MyFormBase.YEN("G",long)：有符号三位逗号串+半角G；外层'+'由皮肤保留。
std::string facility_gold_text(std::int64_t amount) {
    auto value=std::to_string(amount);
    const std::size_t first=amount<0?1:0;
    for(auto offset=value.size();offset>first+3;) {offset-=3;value.insert(offset,",");}
    return value+"G";
}
std::optional<SteamFacilityDetailSkinPlan> window_facility_special_plan(
    const StartupWorldRuntimeState &s,std::uint64_t id,const ChineseFont &font) {
    if(!s.rules)return {};
    const auto p=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),[&](const auto &page){return page.id==id;});
    if(p==s.scripts.pages.end()||!special_facility_page(s,*p))return {};
    const auto d=std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),[&](const auto &value){return value.id==p->legacy_f;});
    SteamFacilitySpecialSkinOptions options;options.view_y=23;options.finger_frame=0;
    const int title_width=int(font.measure("设施信息"));options.title_widths=std::array<int,2>{title_width,title_width};
    // 中文文案沿固定汉化APK字面适配，不能据此认证Steam翻译表或原字体。
    if(d->detail==1||d->detail==4||d->detail==5) {
        options.sale_text=std::string(d->detail==1?"武器":d->detail==4?"防具":"饰品")+"出售中";
        if(p->legacy_g!=1) {options.bottom_text="查看商品";options.bottom_width=int(font.measure(*options.bottom_text));}
    } else if(d->detail==6) {
        const auto candidates=startup_world_residence_candidates(s);
        if(!candidates)return {};
        options.applicant_text=candidates->empty()?"现在没有希望入住的人":
            "现在有"+std::to_string(candidates->size())+"人希望入住";
        if(p->legacy_g!=1) {options.bottom_text="入住希望者";options.bottom_width=int(font.measure(*options.bottom_text));}
    } else {
        if(d->neighbour_effects.size()<2)return {};
        options.plant_money_text=facility_gold_text(d->neighbour_effects[0].delta);
        if(p->legacy_g!=1)options.bottom_text="会给周围带来好的影响";
    }
    // 正式桥负责实例/父85/冻结目录与坏状态；这里不补载荷、不造实例或重算属性。
    return steam_facility_special_skin(s,id,options);
}
SteamManualArrowPlan facility_detail_arrow(
    const SteamFacilityArrowTouch &request,SourceSprites &sprites,bool selected=false) {
    const auto out=steam_manual_arrow_skin({request.position,request.frame,selected},
                                         sprites.manual_arrow_bounds(request.frame),128);
    if(!out)throw std::runtime_error("设施标题箭头helper拒绝");
    return *out;
}
void draw_facility_detail(const SteamFacilityDetailSkinPlan &plan,SourceSprites &sprites,
                          const ChineseFont &font,std::optional<int> pressed_arrow={},int first=0,
                          bool scroll_touch=false) {
    const auto color=[](std::array<int,3> c){return Color{static_cast<unsigned char>(c[0]),
        static_cast<unsigned char>(c[1]),static_cast<unsigned char>(c[2]),255};};
    std::vector<std::array<int,4>> clips;
    const auto set_clip=[&](std::array<int,4> r) {
        BeginScissorMode(int(facility_detail_origin.x)+r[0],int(facility_detail_origin.y)+r[1],r[2],r[3]);
    };
    const auto intersect=[](std::array<int,4> a,std::array<int,4> b) {
        const int x=std::max(a[0],b[0]),y=std::max(a[1],b[1]);
        return std::array<int,4>{x,y,std::max(0,std::min(a[0]+a[2],b[0]+b[2])-x),
                               std::max(0,std::min(a[1]+a[3],b[1]+b[3])-y)};
    };
    for(const auto &op:plan.operations) {
        if(const auto *draw=std::get_if<SteamFacilityDraw>(&op)) {
            if(const auto *r=std::get_if<StartupSkinRect>(draw)) {
                const Rectangle box{facility_detail_origin.x+r->rect[0],facility_detail_origin.y+r->rect[1],float(r->rect[2]),float(r->rect[3])};
                if(r->outline)DrawRectangleLinesEx(box,1,color(r->rgb));else DrawRectangleRec(box,color(r->rgb));
            } else if(const auto *i=std::get_if<SteamFacilityImage>(draw))sprites.facility_image(*i,facility_detail_origin);
            else if(const auto *n=std::get_if<SteamFacilityNumber>(draw))sprites.menu_number(*n,facility_detail_origin);
            else if(const auto *m=std::get_if<SteamFacilityMapchip2>(draw)) {
                const auto pieces=steam_facility_mapchip2_draws(*m);
                if(!pieces)throw std::runtime_error("设施大图pattern拒绝");
                for(const auto &part:*pieces)sprites.draw(part.sprite,part.frame,
                    {facility_detail_origin.x+part.position[0],facility_detail_origin.y+part.position[1]});
            } else if(const auto *c=std::get_if<SteamFacilityClip>(draw)) {
                if(c->kind==SteamFacilityClipKind::push_intersect) {
                    clips.push_back(clips.empty()?c->rectangle:intersect(clips.back(),c->rectangle));set_clip(clips.back());
                } else {
                    if(clips.empty())throw std::runtime_error("设施裁剪下溢");
                    clips.pop_back();EndScissorMode();if(!clips.empty())set_clip(clips.back());
                }
            } else throw std::runtime_error("设施详情未接通用图元");
        } else if(const auto *t=std::get_if<SteamFacilityDetailText>(&op)) {
            const auto visible=facility_detail_text(*t);
            const float size=t->font_size?float(t->font_size):12.F;
            float x=facility_detail_origin.x+t->rectangle[0],y=facility_detail_origin.y+t->rectangle[1];
            const bool layout=t->method==SteamFacilityTextMethod::text_layout;
            if(t->anchor.value_or(0)&2)x+=(layout?float(t->rectangle[2]):0.F)/2-font.measure(visible,size)/2;
            else if(t->anchor.value_or(0)&4)x+=(layout?float(t->rectangle[2]):0.F)-font.measure(visible,size);
            if(layout) {
                y+=std::max(0.F,(float(t->rectangle[3])-size)/2);
                set_clip(clips.empty()?t->rectangle:intersect(clips.back(),t->rectangle));
            }
            if(t->role==SteamFacilityDetailTextRole::bonus_value) {
                const auto split=visible.size()-t->value.size();
                const auto rich="<co=0065FF>"+visible.substr(0,split)+"</co>"+t->value;
                font.manual_text(rich,x,y,layout?float(t->rectangle[2]):1000.F,size,0,color(t->rgb));
            } else font.text(visible,x,y,color(t->rgb),size);
            if(layout){EndScissorMode();if(!clips.empty())set_clip(clips.back());}
        } else if(const auto *a=std::get_if<SteamFacilityArrowTouch>(&op)) {
            // 使用已核GameView纹理消费者，common74 SEB只给bounds；无悬停假选中。
            const auto arrow=facility_detail_arrow(*a,sprites,pressed_arrow==a->value);
            const auto &c=arrow.crop;const auto &d=arrow.destination;
            sprites.crop("steam-common/buttoneffect.png",{float(c[0]),float(c[1]),float(c[2]),float(c[3])},
                {facility_detail_origin.x+d[0],facility_detail_origin.y+d[1]},true,Vector2{d[2],d[3]});
        } else if(const auto *s=std::get_if<SteamFacilityScrollRequest>(&op)) {
            const auto scroll=steam_facility_scroll_skin(*s,first,scroll_touch);
            if(!scroll)throw std::runtime_error("设施滚动条载荷拒绝");
            for(const auto &r:scroll->rectangles)
                DrawRectangleRec({facility_detail_origin.x+r.rect[0],facility_detail_origin.y+r.rect[1],
                    float(r.rect[2]),float(r.rect[3])},color(r.rgb));
        } else if(!std::holds_alternative<SteamStartupTouch>(op))
            throw std::runtime_error("设施详情未消费请求");
    }
    if(!clips.empty()){EndScissorMode();throw std::runtime_error("设施详情裁剪未退休");}
    font.text("返回",188,272); // 框架右软标签研究适配；左侧不再伪造确定/返回。
}
constexpr Vector2 human_overview_origin{0,40};
const char *human_overview_label(SteamHumanOverviewTextRole role,bool recovery,int argument) {
    switch(role) {
    case SteamHumanOverviewTextRole::satisfaction:return "满意度";
    case SteamHumanOverviewTextRole::effort:return "努力";
    case SteamHumanOverviewTextRole::gift:return recovery?"赠送回复道具":"赠送礼物";
    case SteamHumanOverviewTextRole::profession_change:return "转职";
    case SteamHumanOverviewTextRole::attribute: {
        constexpr std::array<const char *,6> labels{"体力","力量","灵巧","耐力","魔力","运气"};
        if(argument<0||argument>=6)throw std::runtime_error("人物属性标签拒绝");
        return labels[argument];
    }
    case SteamHumanOverviewTextRole::equipment_slot: {
        constexpr std::array<const char *,4> labels{"武器","防具1","防具2","饰品"};
        if(argument<0||argument>=4)throw std::runtime_error("人物装备槽标签拒绝");return labels[argument];
    }
    case SteamHumanOverviewTextRole::equipment_empty:return "未装备";
    case SteamHumanOverviewTextRole::gift_change_prompt:return "赠礼更换装备";
    case SteamHumanOverviewTextRole::gift_accept:return "赠送礼物";
    case SteamHumanOverviewTextRole::gift_result_suffix:return "作为礼物赠送";
    case SteamHumanOverviewTextRole::profession_current:return "当前职业";
    case SteamHumanOverviewTextRole::profession_insufficient:return "村子点数不足";
    case SteamHumanOverviewTextRole::equipment_hint:return "按确认键赠送礼物"; // btn=0,0的研究文字适配，非原控制图标。
    case SteamHumanOverviewTextRole::magic_title:return "习得魔法";
    case SteamHumanOverviewTextRole::spell: {
        constexpr std::array<const char *,4> labels{"火魔法","冰魔法","雷魔法","回复魔法"};
        if(argument<0||argument>=4)throw std::runtime_error("人物魔法标签拒绝");return labels[argument];
    }
    case SteamHumanOverviewTextRole::magic_status: {
        constexpr std::array<const char *,5> labels{"精通特定职业可以习得","可以使用一种魔法","可以使用两种魔法","可以使用三种魔法","可以使用全部魔法"};
        if(argument<0||argument>=5)throw std::runtime_error("人物魔法状态拒绝");return labels[argument];
    }
    default:return "";
    }
}
std::optional<SteamHumanOverviewPlan> window_human_overview_plan(
    const StartupWorldRuntimeState &s,std::uint64_t id,const ChineseFont &font) {
    const auto view=inspect_startup_world_human_presentation(s,id);
    if(!view)return {};
    SteamHumanOverviewOptions options;options.view_y=23;options.finger_frame=0;
    const auto title="冒险者信息";
    options.title_widths=std::array<int,2>{int(font.measure(title)),int(font.measure(title))};
    const auto &job=s.rules->jobs[view->human.profession].name;
    options.profession_width=int(font.measure(job));
    if(*options.profession_width>=53)options.profession_repeat_width=int(font.measure(job));
    options.bottom_widths={int(font.measure("赠送礼物")),int(font.measure("赠送回复道具"))};
    options.profession_change_width=int(font.measure("转职"));
    options.equipment_hint_width=int(font.measure("按确认键赠送礼物"));
    options.magic_hint_width=int(font.measure("精通特定职业可以习得"));
    if(view->live&&view->live->state==7)options.down_text_width=int(font.measure("倒下中"));
    return steam_human_information_skin(s,id,options);
}
std::optional<SteamHumanOverviewPlan> window_human_gift_confirmation_plan(
    const StartupWorldRuntimeState &s,std::uint64_t id,const ChineseFont &font) {
    const auto choice=s.human_equipment_choices.find(id);
    const auto binding=s.page_human_bindings.find(id);
    if(!s.rules||choice==s.human_equipment_choices.end()||binding==s.page_human_bindings.end()||
       choice->second[0]<0||choice->second[0]>3)return {};
    const auto human=startup_world_human_details(s,binding->second);if(!human)return {};
    const int slot=choice->second[0],kind=slot==0?1:slot==3?3:2;
    SteamHumanOverviewOptions o;o.view_y=23;o.finger_frame=0;
    o.title_widths=std::array<int,2>{int(font.measure("赠送礼物")),int(font.measure("赠送礼物"))};
    o.gift_accept_width=int(font.measure("赠送礼物"));
    for(int side=0;side<2;++side) {
        const auto equipment=side==0?human->equipment[slot]:std::optional<int>{choice->second[1]};
        std::string name="未装备";
        if(equipment) {
            const auto d=std::find_if(s.rules->equipment.begin(),s.rules->equipment.end(),[&](const auto &e){return e.shop.kind==kind&&e.shop.id==*equipment;});
            if(d==s.rules->equipment.end())return {};
            name=d->name;
        }
        o.comparison_name_widths[side]=int(font.measure(name,10));
    }
    return steam_human_gift_confirmation_skin(s,id,o);
}
void draw_human_overview(const SteamHumanOverviewPlan &plan,SourceSprites &sprites,const ChineseFont &font,Vector2 origin=human_overview_origin) {
    const auto color=[](std::array<int,3> c) {return Color{static_cast<unsigned char>(c[0]),
        static_cast<unsigned char>(c[1]),static_cast<unsigned char>(c[2]),255};};
    const auto rectangle=[&](const StartupSkinRect &r) {
        const Rectangle box{origin.x+r.rect[0],origin.y+r.rect[1],float(r.rect[2]),float(r.rect[3])};
        if(r.outline)DrawRectangleLinesEx(box,1,color(r.rgb));else DrawRectangleRec(box,color(r.rgb));
    };
    const auto text=[&](const std::string &value,std::array<int,4> r,int anchor,int size,
                        bool layout,std::array<int,3> rgb) {
        const float fs=size?float(size):12.F;
        float x=origin.x+r[0],y=origin.y+r[1];
        if(anchor&2)x+=(layout?float(r[2]):0.F)/2-font.measure(value,fs)/2;
        else if(anchor&4)x+=(layout?float(r[2]):0.F)-font.measure(value,fs);
        if(!layout&&(anchor&32))y-=fs/2; // 原---使用anchor0x22，研究字形以实字号中心适配。
        const bool bounded_layout=layout&&r[3]>0; // 宽度重载的h0不是零高裁剪；原TextLayout完整换行仍另验。
        if(bounded_layout) {
            y+=std::max(0.F,(float(r[3])-fs)/2);
            BeginScissorMode(int(origin.x)+r[0],int(origin.y)+r[1],r[2],r[3]);
        }
        font.text(value,x,y,color(rgb),fs);
        if(bounded_layout)EndScissorMode();
    };
    const auto human_parts=[&](const SteamHumanSkinPlan &p) {
        for(const auto &part:p.draws) {
            if(const auto *r=std::get_if<StartupSkinRect>(&part))rectangle(*r);
            else if(const auto *image=std::get_if<SteamHumanImage>(&part))sprites.human_image(*image,origin);
            else if(const auto *t=std::get_if<SteamHumanText>(&part))
                text("倒下中",{t->position[0],t->position[1],0,0},t->anchor,0,false,t->rgb);
        }
    };
    for(const auto &op:plan.operations) {
        if(const auto *frame=std::get_if<SteamStartupFrameDraw>(&op)) {
            if(const auto *r=std::get_if<StartupSkinRect>(frame))rectangle(*r);
            else if(const auto *image=std::get_if<StartupSkinDraw>(frame)) {
                if(image->image==121&&image->sprite==6) {
                    StartupVisualDraw small_corner;
                    small_corner.resource=StartupVisualResource::common;small_corner.image=121;
                    small_corner.sprite=6;small_corner.frame=image->frame;small_corner.offset=image->offset;
                    sprites.visual(small_corner,origin);
                    continue;
                }
                const auto asset=image->image==74?SteamFacilityAsset::title_arrows:image->image==28?SteamFacilityAsset::wood:
                    image->image==29?SteamFacilityAsset::title_bar:SteamFacilityAsset::corner;
                if(image->image!=28&&image->image!=29&&image->image!=30&&image->image!=74)
                    throw std::runtime_error("人物窗框资源载荷拒绝");
                sprites.facility_image({asset,image->offset,image->frame,
                    image->sprite<0?std::optional<std::array<int,4>>(image->crop):std::nullopt},origin);
            } else if(const auto *t=std::get_if<SteamStartupText>(frame))
                text(plan.title.empty()?"冒险者信息":plan.title,{int(t->rectangle[0]),int(t->rectangle[1]),int(t->rectangle[2]),int(t->rectangle[3])},
                    t->anchor.value_or(0),t->font_size,t->placement==SteamStartupTextPlacement::layout,
                    t->rgb.value_or(std::array<int,3>{0,0,0}));
        } else if(const auto *image=std::get_if<SteamHumanImage>(&op))sprites.human_image(*image,origin);
        else if(const auto *image=std::get_if<SteamFacilityImage>(&op))sprites.facility_image(*image,origin);
        else if(const auto *number=std::get_if<SteamFacilityNumber>(&op))sprites.menu_number(*number,origin);
        else if(const auto *t=std::get_if<SteamHumanOverviewText>(&op))
            text(t->role==SteamHumanOverviewTextRole::profession_transition?"转职为"+t->value:
                 t->value.empty()?human_overview_label(t->role,plan.recovery_gift,t->argument):t->value,
                 t->rectangle,t->anchor,t->font_size,t->layout,t->rgb);
        else if(const auto *p=std::get_if<SteamHumanPortrait>(&op)) {
            if(p->clip) {
                const auto &r=*p->clip;
                BeginScissorMode(int(origin.x)+r[0],int(origin.y)+r[1],r[2],r[3]);
            }
            const Vector2 anchor{origin.x+p->position[0],origin.y+p->position[1]};
            if(p->actor.shadow)sprites.visual(*p->actor.shadow,anchor);
            if(p->actor.weapon)sprites.visual(*p->actor.weapon,anchor);
            sprites.actor(false,p->actor.body.sprite,p->actor.body.image,p->actor.body.frame,anchor);
            if(p->clip)EndScissorMode();
        }
        else if(const auto *p=std::get_if<SteamHumanDetailSkin>(&op)) {
            human_parts(p->medals);
            const Vector2 anchor{origin.x+p->portrait_position[0],origin.y+p->portrait_position[1]};
            if(p->portrait.shadow)throw std::runtime_error("人物详情不允许标题阴影");
            if(p->portrait.weapon)sprites.visual(*p->portrait.weapon,anchor);
            sprites.actor(false,p->portrait.body.sprite,p->portrait.body.image,p->portrait.body.frame,anchor);
            human_parts(p->after_portrait);
        } else if(const auto *scroll=std::get_if<SteamHumanOverviewScroll>(&op)) {
            for(const auto &r:scroll->rectangles)rectangle(r);
        } else if(const auto *scroll=std::get_if<SteamHumanCatalogueScroll>(&op)) {
            for(const auto &r:scroll->rectangles)rectangle(r);
        } else if(const auto *learning=std::get_if<SteamHumanLearningText>(&op)) {
            const std::string name=human_overview_label(SteamHumanOverviewTextRole::spell,false,learning->spell);
            const std::string suffix="习得";
            const float x=origin.x+learning->position[0]-font.measure(name+suffix);
            font.text(name,x,origin.y+learning->position[1],color(learning->name_rgb));
            font.text(suffix,x+font.measure(name),origin.y+learning->position[1],color(learning->suffix_rgb));
        } else if(const auto *a=std::get_if<SteamFacilityArrowTouch>(&op)) {
            const auto arrow=facility_detail_arrow(*a,sprites);
            const auto &c=arrow.crop;const auto &d=arrow.destination;
            sprites.crop("steam-common/buttoneffect.png",{float(c[0]),float(c[1]),float(c[2]),float(c[3])},
                {origin.x+d[0],origin.y+d[1]},true,Vector2{d[2],d[3]});
        } else if(!std::holds_alternative<SteamStartupTouch>(op)&&!std::holds_alternative<SteamHumanEquipmentRow>(op))
            throw std::runtime_error("人物详情未消费图元");
    }
    if(plan.soft_labels[0]==8)font.text("追踪",18,272);
    else if(plan.soft_labels[0]==2)font.text("返回",18,272);
    else if(plan.soft_labels[0]==7)font.text("详细",18,272);
    if(plan.soft_labels[1]==2)font.text("返回",188,272);
}
// 两种设施演出共用有序执行器，保留裁剪栈；每个variant恰消费一次。
template<class Plan> void draw_facility_ceremony(const Plan &plan,SourceSprites &sprites,const ChineseFont &font,Vector2 origin=human_overview_origin) {
    std::vector<std::array<int,4>> clips;
    const auto apply_clip=[&] {
        if(clips.empty())return;
        const auto &r=clips.back();
        BeginScissorMode(int(origin.x)+r[0],int(origin.y)+r[1],r[2],r[3]);
    };
    const auto intersect=[](std::array<int,4> a,std::array<int,4> b) {
        const int x=std::max(a[0],b[0]),y=std::max(a[1],b[1]);
        return std::array<int,4>{x,y,std::max(0,std::min(a[0]+a[2],b[0]+b[2])-x),
                                   std::max(0,std::min(a[1]+a[3],b[1]+b[3])-y)};
    };
    for(const auto &op:plan.operations)std::visit([&](const auto &value) {
        using T=std::decay_t<decltype(value)>;
        if constexpr(std::is_same_v<T,SteamTownClip>||std::is_same_v<T,SteamMagicPotClip>) {
            using Kind=decltype(value.kind);
            if(value.kind==Kind::push_intersect) {
                clips.push_back(clips.empty()?value.rectangle:intersect(clips.back(),value.rectangle));apply_clip();
            } else if(value.kind==Kind::replace) {
                if(clips.empty())clips.push_back(value.rectangle);else clips.back()=value.rectangle;
                EndScissorMode();apply_clip();
            } else {
                if(clips.empty())throw std::runtime_error("城镇演出裁剪下溢");
                clips.pop_back();EndScissorMode();apply_clip();
            }
        } else if constexpr(std::is_same_v<T,StartupSkinDraw>) {
            if(value.package!=StartupSkinPackage::event||value.image!=9||value.sprite!=-1)
                throw std::runtime_error("启动背景来源拒绝");
            const auto &r=value.crop;
            sprites.crop("event/event_backGlad.png",{float(r[0]),float(r[1]),float(r[2]),float(r[3])},
                {origin.x+value.offset[0],origin.y+value.offset[1]});
        } else if constexpr(std::is_same_v<T,SteamCommerceImage>) {
            const auto r=steam_commerce_resource(value.asset);
            if(!r)throw std::runtime_error("商会资源映射拒绝");
            sprites.published_sprite(r->image,r->published_image,r->published_sprite,
                value.position,value.frame,value.crop,origin);
        } else if constexpr(std::is_same_v<T,SteamConfigImage>) {
            const auto controls=steam_config_image_skin(value,128);
            if(!controls)throw std::runtime_error("设置控件来源拒绝");
            const auto &c=controls->crop;const auto &d=controls->destination;
            sprites.crop("steam-common/buttoneffect.png",{float(c[0]),float(c[1]),float(c[2]),float(c[3])},
                {origin.x+d[0],origin.y+d[1]},true,Vector2{d[2],d[3]});
        } else if constexpr(std::is_same_v<T,SteamConfigTouch>) {
            // 条件窗口只消费绘制；完整设置输入、音量输出与系统保存尚未接Owner。
        } else if constexpr(std::is_same_v<T,SteamConfigPlatformKeyboard>) {
            const auto button=steam_platform_button_skin({SteamPlatformButtonKind::keyboard_guide,
                value.anchor,value.component,true,false,value.touched,{}});
            if(!button||!button->body)throw std::runtime_error("设置平台键盘请求拒绝");
            sprites.platform_button(*button->body);
        } else if constexpr(std::is_same_v<T,SteamNewGameArrow>) {
            const auto arrow=steam_manual_arrow_skin(value.source,sprites.manual_arrow_bounds(value.source.bounds_frame),128);
            if(!arrow)throw std::runtime_error("新局性别箭头来源拒绝");
            const auto &c=arrow->crop;const auto &d=arrow->destination;
            sprites.crop("steam-common/buttoneffect.png",{float(c[0]),float(c[1]),float(c[2]),float(c[3])},
                {origin.x+d[0],origin.y+d[1]},true,Vector2{d[2],d[3]});
        } else if constexpr(std::is_same_v<T,SteamVillageActivityImage>||std::is_same_v<T,SteamTownImage>||std::is_same_v<T,SteamMagicPotImage>||std::is_same_v<T,SteamTaskSelectionImage>||std::is_same_v<T,SteamTaskRosterImage>||std::is_same_v<T,SteamTaskDeadlineImage>) {
            const auto resource=[&] {
                if constexpr(std::is_same_v<T,SteamVillageActivityImage>)return steam_village_activity_resource(value.asset);
                else if constexpr(std::is_same_v<T,SteamMagicPotImage>)return steam_magic_pot_resource(value.asset);
                else if constexpr(std::is_same_v<T,SteamTaskSelectionImage>)return steam_task_selection_resource(value.asset);
                else if constexpr(std::is_same_v<T,SteamTaskRosterImage>)return steam_task_roster_resource(value.asset);
                else if constexpr(std::is_same_v<T,SteamTaskDeadlineImage>)return steam_task_deadline_resource(value.asset);
                else return steam_town_resource(value.asset);
            }();
            if(!resource)throw std::runtime_error("城镇活动资源映射缺失");
            const auto crop=[&]() -> std::optional<std::array<int,4>> {
                if constexpr(std::is_same_v<T,SteamTaskDeadlineImage>)return {};
                else return value.crop;
            }();
            if(!resource->published_sprite&&!crop) {
                // 具名整图节点没有SEB，按原图大小绘制；不为缺SEB的其它节点猜帧。
                const auto relative=std::filesystem::path(resource->published_image).lexically_relative("original");
                sprites.image(relative,{origin.x+value.position[0],origin.y+value.position[1]});
            } else sprites.published_sprite(resource->image,resource->published_image,resource->published_sprite,
                value.position,value.frame,crop,origin);
        } else if constexpr(std::is_same_v<T,SteamVillageActivityText>||std::is_same_v<T,SteamTownText>||std::is_same_v<T,SteamMagicPotText>||std::is_same_v<T,SteamTaskSelectionText>) {
            const bool rich=[&] {if constexpr(std::is_same_v<T,SteamTaskSelectionText>)return value.value.find('<')!=std::string::npos;else return value.rich;}();
            std::string visible=value.value;
            if(rich)for(std::size_t at=0;(at=visible.find('<',at))!=std::string::npos;) {
                const auto end=visible.find('>',at);if(end==std::string::npos)throw std::runtime_error("活动富文本标签未闭合");
                visible.erase(at,end-at+1);
            }
            float size=value.font_size?float(value.font_size):12.F;
            const bool width_only=value.layout&&value.rectangle[3]==0;
            if(width_only&&(value.anchor&0x60)&&value.rectangle[2]>0)
                while(size>3.F&&font.measure(visible,size)>value.rectangle[2])size-=1.F;
            float x=origin.x+value.rectangle[0],y=origin.y+value.rectangle[1];
            // 原DrawText宽度重载先按实际usedSize垂直锚定，再转TextLayout。
            if(width_only) {if(value.anchor&32)y-=size/2;else if(value.anchor&64)y-=size;}
            if(value.anchor&2)x+=(value.layout?float(value.rectangle[2])/2:0.F)-font.measure(visible,size)/2;
            else if(value.anchor&4)x+=(value.layout?float(value.rectangle[2]):0.F)-font.measure(visible,size);
            const bool bounded=value.layout&&value.rectangle[3]>0;
            if(bounded) {
                y+=std::max(0.F,(value.rectangle[3]-size)/2);
                const auto box=clips.empty()?value.rectangle:intersect(clips.back(),value.rectangle);
                BeginScissorMode(int(origin.x)+box[0],int(origin.y)+box[1],box[2],box[3]);
            }
            const auto &c=value.rgb;const Color color{static_cast<unsigned char>(c[0]),static_cast<unsigned char>(c[1]),static_cast<unsigned char>(c[2]),255};
            if(rich||value.layout)font.manual_text(value.value,x,y,value.layout?float(value.rectangle[2]):1000.F,size,0,color);
            else font.text(value.value,x,y,color,size);
            if(bounded){EndScissorMode();apply_clip();}
        } else if constexpr(std::is_same_v<T,SteamTaskSelectionClip>||std::is_same_v<T,SteamHumanClip>) {
            const bool push=[&] {if constexpr(std::is_same_v<T,SteamHumanClip>)return value.kind==SteamHumanClipKind::push_intersect;else return value.push;}();
            if(push) {clips.push_back(clips.empty()?value.rectangle:intersect(clips.back(),value.rectangle));apply_clip();}
            else {if(clips.empty())throw std::runtime_error("任务演出裁剪下溢");clips.pop_back();EndScissorMode();apply_clip();}
        } else if constexpr(std::is_same_v<T,SteamFacilityClip>) {
            if(value.kind==SteamFacilityClipKind::push_intersect) {
                clips.push_back(clips.empty()?value.rectangle:intersect(clips.back(),value.rectangle));apply_clip();
            } else {
                if(clips.empty())throw std::runtime_error("设施演出裁剪下溢");
                clips.pop_back();EndScissorMode();apply_clip();
            }
        } else if constexpr(std::is_same_v<T,SteamFacilityMapchip2>) {
            const auto pieces=steam_facility_mapchip2_draws(value);
            if(!pieces)throw std::runtime_error("设施演出地图块来源拒绝");
            for(const auto &p:*pieces)sprites.draw(p.sprite,p.frame,
                {origin.x+p.position[0],origin.y+p.position[1]});
        } else if constexpr(std::is_same_v<T,SteamFacilityMapchipDraw>) {
            sprites.draw(value.sprite,value.frame,{origin.x+value.position[0],origin.y+value.position[1]});
        } else if constexpr(std::is_same_v<T,SteamTaskDeadlineMonster>) {
            sprites.actor(true,value.sprite,value.image,value.frame,{origin.x+value.position[0],origin.y+value.position[1]});
        } else if constexpr(std::is_same_v<T,SteamFacilityEffectImage>) {
            const auto r=steam_facility_effect_resource(value.asset);
            if(!r)throw std::runtime_error("设施改良特效映射缺失");
            sprites.published_sprite(r->image,r->published_image,r->published_sprite,value.position,value.frame,{},origin);
        } else if constexpr(std::is_same_v<T,SteamFacilityReputationImage>) {
            const auto r=steam_facility_reputation_resource(value.asset);
            if(!r)throw std::runtime_error("口碑角色映射缺失");
            sprites.published_sprite(r->image,r->published_image,r->published_sprite,value.position,value.frame,{},origin);
        } else if constexpr(std::is_same_v<T,SteamFacilityEffectScroll>) {
            SteamHumanOverviewPlan part;part.title=plan.title;part.soft_labels={0,0};
            for(const auto &r:value.skin.rectangles)part.operations.emplace_back(SteamStartupFrameDraw{r});
            draw_human_overview(part,sprites,font,origin);
        } else if constexpr(std::is_same_v<T,SteamTaskDepartureActor>||std::is_same_v<T,SteamTaskVictoryActor>) {
            const auto actor=[&] {
                if constexpr(std::is_same_v<T,SteamTaskVictoryActor>)return steam_task_victory_actor_skin(value);
                else return steam_task_departure_actor_skin(value);
            }();if(!actor)throw std::runtime_error("任务人物动作拒绝");
            const Vector2 position{origin.x+value.position[0],origin.y+value.position[1]};
            if(actor->weapon)sprites.visual(*actor->weapon,position);
            sprites.actor(false,actor->body.sprite,actor->body.image,actor->body.frame,position);
        } else if constexpr(std::is_same_v<T,SteamTaskRosterTouch>||std::is_same_v<T,SteamFacilityEffectTouch>||std::is_same_v<T,SteamStartupTouch>) {
            // 注册由窗口输入消费者执行，不在绘制时产生操作。
        } else {
            SteamHumanOverviewPlan part;part.title=plan.title;part.soft_labels={0,0};
            if constexpr(std::is_same_v<T,SteamHumanOverviewText>) {
                if(value.rich) {
                    std::string visible=value.value;
                    for(std::size_t at=0;(at=visible.find('<',at))!=std::string::npos;) {
                        const auto end=visible.find('>',at);if(end==std::string::npos)throw std::runtime_error("人物富文标签未闭合");
                        visible.erase(at,end-at+1);
                    }
                    const float size=value.font_size?float(value.font_size):12.F;
                    float x=origin.x+value.rectangle[0];
                    if(value.anchor&2)x-=font.measure(visible,size)/2;else if(value.anchor&4)x-=font.measure(visible,size);
                    const auto &c=value.rgb;
                    font.manual_text(value.value,x,origin.y+value.rectangle[1],1000.F,size,0,
                        {static_cast<unsigned char>(c[0]),static_cast<unsigned char>(c[1]),static_cast<unsigned char>(c[2]),255});
                    return;
                }
                part.operations.emplace_back(value);
            } else if constexpr(std::is_same_v<T,SteamFacilityEffectText>)part.operations.emplace_back(SteamHumanOverviewText{
                SteamHumanOverviewTextRole::result_message,value.value,value.rectangle,value.anchor,value.font_size,value.layout,value.rgb});
            else part.operations.emplace_back(value);
            draw_human_overview(part,sprites,font,origin);apply_clip();
        }
    },op);
    if(!clips.empty())throw std::runtime_error("设施演出裁剪未释放");
    if(plan.soft_labels[0]==6)font.text("查看位置",12,272);
    else if(plan.soft_labels[0]==7)font.text("详细",12,272);
    if(plan.soft_labels[1]==12)font.text("开始",188,272);
    if(plan.soft_labels[1]==2)font.text("返回",188,272);
    if(plan.soft_labels[1]==10)font.text("终止",188,272);
}
std::optional<SteamFacilityEffectPlan> window_facility_effect_plan(
    const StartupWorldRuntimeState &s,std::uint64_t id,const ChineseFont &font,bool first_touch=false) {
    SteamFacilityEffectOptions o;o.view_y=23;o.scroll_first_touch=first_touch;
    o.title_widths=std::array<int,2>{int(font.measure("设施强化")),int(font.measure("设施强化"))};
    return steam_facility_effect_skin(s,id,o);
}
std::optional<SteamFacilityReputationPlan> window_facility_reputation_plan(
    const StartupWorldRuntimeState &s,std::uint64_t id,const ChineseFont &font) {
    SteamFacilityReputationOptions o;o.view_y=23;
    o.title_widths=std::array<int,2>{int(font.measure("传说中的商店")),int(font.measure("传说中的商店"))};
    o.increase_suffix_width=int(font.measure("上升!"));
    return steam_facility_reputation_skin(s,id,o);
}
std::optional<SteamVillageActivityPlan> window_village_activity_plan(
    const StartupWorldRuntimeState &s,std::uint64_t id,const ChineseFont &font,bool touching=false) {
    const auto view=inspect_startup_world_village_activity_page(s,id);if(!view||!s.rules)return {};
    const int definition=view->activity.value_or(view->entries.empty()?-1:view->entries[view->selection]);
    const auto d=std::find_if(s.rules->activities.begin(),s.rules->activities.end(),[=](const auto &a){return a.identity==definition;});
    if(d==s.rules->activities.end())return {};
    SteamVillageActivityOptions o;o.view_y=23;o.scroll_first_touch=touching;
    const std::string title=view->raw==51?"开展活动":view->raw==54?"活动结束":d->name;
    o.title_widths=std::array<int,2>{int(font.measure(title)),int(font.measure(title))};
    o.description_width=int(font.measure(d->description));o.increase_suffix_width=int(font.measure("上升了!"));
    const bool execute=(s.activity_flags.at(definition)&8U)!=0;
    o.button_widths={int(font.measure(execute?"实施":"开展")),int(font.measure(execute?"不实施":"不开展"))};
    if(d->parameters[2]==1) {
        constexpr const char *names[]{"体力","力","器用","丈夫","魔力","运"};
        const int attribute=d->parameters[3];if(attribute<0||attribute>=6)return {};
        o.attribute_widths={int(font.measure(view->raw==53?"冒险者们的":"冒险者的")),
            int(font.measure(names[attribute])),int(font.measure("上升"))};
    }
    return steam_village_activity_skin(s,id,o);
}
SteamTownProgressOptions window_town_options(const StartupWorldRuntimeState &s,int raw,const ChineseFont &font) {
    SteamTownProgressOptions o;o.view_y=23;
    const std::string title=raw==50?"街道等级上升":raw>=93&&raw<=95?"活动":"街道升级";
    o.title_widths=std::array<int,2>{int(font.measure(title)),int(font.measure(title))};
    o.condition_suffix_width=int(font.measure("的升级条件"));
    const bool ready=std::all_of(s.rank_met.begin(),s.rank_met.end(),[](bool met){return met;});
    o.bottom_width=int(font.measure(raw==49?"了解":ready?"升级!":"申请"));
    o.promoted_suffix_width=int(font.measure("升级！"));
    o.bubble_widths={int(font.measure("恭喜!")),int(font.measure("太好了!")),int(font.measure("万岁")),int(font.measure("请"))};
    return o;
}
std::optional<SteamTownProgressPlan> window_town_progress_plan(
    const StartupWorldRuntimeState &s,std::uint64_t id,const ChineseFont &font) {
    const auto p=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &page){return page.id==id&&page.lifecycle!=4;});
    if(p==s.scripts.pages.end())return {};
    auto options=window_town_options(s,p->legacy_page,font);
    options.reward_number_width=int(font.measure(std::to_string(p->legacy_s)));
    if(p->legacy_page==94&&p->legacy_r==4)options.bubble_widths[3]=int(font.measure("注目!"));
    if(p->legacy_page==48||p->legacy_page==49)return steam_town_conditions_skin(s,id,options);
    if(p->legacy_page==50)return steam_town_celebration_skin(s,id,options);
    if(p->legacy_page>=93&&p->legacy_page<=95)return steam_town_gift_skin(s,id,options);
    return {};
}
// 从已绘制的正式组件展开PC热区；设备只产生DTO，不保存第二份业务选择。
SteamTaskSelectionOptions window_task_options(int raw,const ChineseFont &font,bool touching=false) {
    SteamTaskSelectionOptions o;o.view_y=23;o.scroll_first_touch=touching;
    const std::string title=raw==22?"任务选择":"寻求！冒险者";
    const int width=int(font.measure(title));o.title_widths=std::array<int,2>{width,width};
    o.bottom_width=int(font.measure(raw==22?"请选择任务":"募集冒险者"));
    constexpr const char *phrases[]{"交给我!","多指教","来了!","我会加油的!","我要参加!","好期待"};
    for(int i=0;i<6;++i)o.phrase_widths[i]=int(font.measure(phrases[i]));
    return o;
}
std::optional<SteamTaskSelectionPlan> window_task_selection_plan(
    const StartupWorldRuntimeState &s,std::uint64_t id,const ChineseFont &font,bool touching=false) {
    const auto page=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &p){return p.id==id;});
    if(page==s.scripts.pages.end())return {};
    return steam_task_selection_skin(s,id,window_task_options(page->legacy_page,font,touching));
}
// 无世界运行的显示条件：原工厂任务费用／宝箱等级，人物取新局定义，募集时点明确指定。
std::optional<SteamTaskSelectionPlan> task_selection_example(const std::string &name,const ChineseFont &font) {
    StartupSession initial;StartupWorldRuntimeSession session(initial.state(),ref::WorldRandomStream::from_java_seed(1));
    auto s=session.state();
    const auto made=ref::prepare_world_task_creation(startup_world_runtime_factory(s),0);
    if(!made.candidate||!made.candidate->created_task||!write_startup_world_runtime_factory(s,made.candidate->state))return {};
    const auto &task=s.tasks.at(*made.candidate->created_task);
    const auto d=std::find_if(s.rules->tasks.begin(),s.rules->tasks.end(),[&](const auto &v){return v.factory.identity==task.definition;});
    if(d==s.rules->tasks.end())return {};
    SteamTaskSelectionInput in;in.raw=name=="task-selection"?22:name=="task-offer"?23:24;
    in.options=window_task_options(in.raw,font);in.fee=d->recruitment_fee;
    in.affordable=s.scene.world.world.ai.accounting.funds()>=in.fee;
    in.rows.push_back({task.identity,d->factory.kind,task.difficulty,in.fee,task.newly_available,in.affordable,d->name});
    if(in.raw==24) {
        const auto human=startup_world_human_details(s,0);if(!human)return {};
        in.counter=35;in.humans.push_back({0,human->profession,human->sex,human->equipment[0].value_or(-1),human->level,human->medals,human->name});
        ref::TaskRecruitmentAnimation animation;animation.entries={{0,35,0}};animation.portraits={0};
        animation.completion_tick=130;animation.portrait_timer=20;animation.displayed_count=1;in.recruitment=animation;
    }
    return steam_task_selection_skin(in);
}
std::optional<SteamTaskRosterPlan> task_roster_example(const std::string &name,const ChineseFont &font) {
    StartupSession initial;StartupWorldRuntimeSession session(initial.state(),ref::WorldRandomStream::from_java_seed(1));
    const auto &s=session.state();SteamTaskRosterInput in;
    in.raw=name=="task-roster"?25:(name=="task-extra"||name=="task-extra-stats")?27:28;
    in.tab=name=="task-extra-stats"?1:0;in.phase=name=="task-departing"?1:0;
    in.counter=in.phase?24:0;in.secondary=10;in.participant_count=2;in.prediction=3;
    in.task_name=s.rules->tasks.front().name;in.frozen_humans={0,1};if(in.raw==25)in.frozen_humans.push_back(-1);
    for(int id:{0,1}) {
        const auto h=startup_world_human_details(s,id);if(!h)return {};
        SteamTaskRosterHuman human;human.identity=id;human.profession=h->profession;human.sex=h->sex;
        human.weapon=h->equipment[0].value_or(-1);human.level=h->level;human.effort=h->effort;human.medals=h->medals;human.name=h->name;
        human.fee=s.human_calendar.at(id).continuation_cost;human.affordable=s.scene.world.world.ai.accounting.funds()>=human.fee;
        human.combat=h->combat;in.humans.push_back(human);
    }
    in.options.view_y=23;const std::string title=in.raw==25?"参加者名单":in.raw==27?"参加委托 "+std::to_string(in.tab+1)+"/2":"任务开始";
    const int width=int(font.measure(title));in.options.title_widths=std::array<int,2>{width,width};
    in.options.prediction_width=int(font.measure("我觉得会好的。"));
    in.options.button_widths={int(font.measure("立即开始")),int(font.measure("重新考虑"))};
    return steam_task_roster_skin(in);
}
SteamTaskInfoOptions window_task_info_options(const ChineseFont &font,bool scene) {
    SteamTaskInfoOptions o;o.view_y=23;o.bottom_width=int(font.measure(scene?"攻略":"了解"));
    if(scene)o.view_layout=std::array<int,4>{0,23,240,217};return o;
}
std::optional<SteamTaskSelectionPlan> task_info_example(bool scene,const ChineseFont &font) {
    StartupSession initial;StartupWorldRuntimeSession session(initial.state(),ref::WorldRandomStream::from_java_seed(1));
    auto s=session.state();const auto made=ref::prepare_world_task_creation(startup_world_runtime_factory(s),0);
    if(!made.candidate||!made.candidate->created_task||!write_startup_world_runtime_factory(s,made.candidate->state))return {};
    if(scene) {
        s.scripts.pages.front().lifecycle=2;
        if(open_startup_world_runtime_task_menu(s)!=StartupWorldRuntimeError::none)return {};
        s.scripts.pages.back().lifecycle=2;StartupTaskPageInput input;input.track=true;
        if(input_startup_world_runtime_task_page(s,s.scripts.pages.back().id,input).error!=StartupWorldRuntimeError::none)return {};
        for(int n=0;n<128&&s.scene.scene_state==5;++n) {
            const auto moved=update_startup_world_runtime_task_tracking_scene(s);if(!moved)return {};s=*moved;
        }
        return steam_task_information_scene_skin(s,window_task_info_options(font,true));
    }
    // raw29暂无已核维护开页入口，只展示其纯DTO，不伪造任务页面栈。
    const auto &t=s.tasks.at(*made.candidate->created_task);
    const auto d=std::find_if(s.rules->tasks.begin(),s.rules->tasks.end(),[&](const auto &v){return v.factory.identity==t.definition;});
    if(d==s.rules->tasks.end())return {};
    SteamTaskInfoInput in;in.source=SteamTaskInfoSource::raw29;in.kind=d->factory.kind;in.treasure_level=t.difficulty;
    in.monster_rank=d->factory.exploration_stage;in.fee=d->recruitment_fee;in.name=d->name;
    in.options=window_task_info_options(font,false);return steam_task_information_skin(in);
}
std::optional<SteamTaskResultPlan> task_result_example(const std::string &name,const ChineseFont &font) {
    // 冻结名单/奖励为明确条件；源定义及图标取新局，只展示，不生成战斗或发放奖励。
    StartupSession initial;StartupWorldRuntimeSession session(initial.state(),ref::WorldRandomStream::from_java_seed(1));
    const auto &s=session.state();SteamTaskResultInput in;in.raw=name=="task-battle-result"?31:32;
    in.task_name=s.rules->tasks.front().name;in.first=1;in.selection=in.raw==31?3:0;in.difficulty=1;
    for(int id=0;id<6;++id) {
        const auto h=startup_world_human_details(s,id);if(!h)return {};
        const SteamTaskResultHuman human{id,h->profession,h->sex,h->name};
        if(id<2)in.footer[id]=human;
        if(in.raw==31) {const auto &b=s.scene.world.world.ai.battle.humans.at(id);in.crew.push_back({human,b.task_kills,b.participant_downs});}
    }
    if(in.raw==32) {
        for(int id=0;id<3;++id) {
            const auto d=std::find_if(s.rules->items.begin(),s.rules->items.end(),[=](const auto &v){return v.identity==id;});
            const auto icon=startup_world_item_icon_draws(s,id);if(d==s.rules->items.end()||!icon)return {};
            in.rewards.push_back({0,id,d->commerce_price,d->name,*icon});
        }
        for(int kind=1;kind<=3;++kind) {
            const auto d=std::find_if(s.rules->equipment.begin(),s.rules->equipment.end(),[=](const auto &v){return v.shop.kind==kind;});
            if(d==s.rules->equipment.end())return {};
            const auto icon=startup_world_equipment_icon_draws(s,kind,d->shop.id);if(!icon)return {};
            in.rewards.push_back({kind,d->shop.id,d->shop.price,d->name,*icon});
        }
    }
    in.options.view_y=23;const int width=int(font.measure("成果"));in.options.title_widths=std::array<int,2>{width,width};
    in.options.bottom_width=int(font.measure(in.raw==31?in.task_name+"完成!":"获得了宝物!"));
    return steam_task_result_skin(in);
}
std::optional<SteamHumanOverviewPlan> residence_candidates_example(const ChineseFont &font) {
    // 六行是明确目录条件，人物与入住费取新局；不伪造自然达到入住满足度。
    StartupSession initial;StartupWorldRuntimeSession session(initial.state(),ref::WorldRandomStream::from_java_seed(1));
    const auto &s=session.state();SteamResidenceInput in;in.cursor={3,0};
    for(int id=0;id<6;++id) {
        const auto human=startup_world_human_details(s,id);
        const auto source=std::find_if(s.rules->humans.begin(),s.rules->humans.end(),[=](const auto &v){return v.identity==id;});
        if(!human||source==s.rules->humans.end())return {};
        in.order.push_back(id);in.humans.push_back({id,human->profession,human->sex,source->residence_fee,human->name,
            s.scene.world.world.ai.accounting.funds()>=source->residence_fee});
    }
    in.options.view_y=23;const int width=int(font.measure("选择入住者"));
    in.options.title_widths=std::array<int,2>{width,width};in.options.bottom_width=int(font.measure("请选择入住者"));
    return steam_residence_skin(in);
}
SteamHumanOverviewOptions residence_result_options(const ChineseFont &font,std::optional<int> character_frame) {
    SteamHumanOverviewOptions o;o.view_y=23;o.character_frame=character_frame;
    const int title=int(font.measure("自宅完成"));o.title_widths=std::array<int,2>{title,title};return o;
}
std::optional<SteamHumanOverviewPlan> residence_result_example(const std::string &name,const ChineseFont &font) {
    StartupSession initial;StartupWorldRuntimeSession session(initial.state(),ref::WorldRandomStream::from_java_seed(1));
    const auto &s=session.state();const auto human=startup_world_human_details(s,1);
    const auto home=std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),[](const auto &v){return v.kind==12;});
    if(!human||home==s.rules->facilities.end())return {};
    // 前后值及原请求量是局部奖励显示条件，不给世界注入入住或满足度。
    SteamHumanResidenceResultInput in;in.phase=name=="residence-reward"?1:0;
    in.counter=in.phase?77:50;in.secondary=18;in.mapchip=home->display_id;
    in.human={human->profession,human->sex,human->equipment[0].value_or(-1),in.phase,in.counter,human->name,
        {{{20,30},{40,40},{20,10}}}};
    return steam_human_residence_result_skin(in,residence_result_options(font,0));
}
std::optional<SteamHumanOverviewPlan> award_catalogue_example(const std::string &name,const ChineseFont &font) {
    StartupSession initial;StartupWorldRuntimeSession session(initial.state(),ref::WorldRandomStream::from_java_seed(1));
    const auto &s=session.state();SteamHumanAwardInput in;
    in.cursor={4,1,name=="award-income"?1:name=="award-residents"?2:0};in.frame=8;in.medals_left=1;
    // 六人目录／一个待授勋章为纯显示条件；各人物数据读取原新局，不执行年度授予。
    for(int id=0;id<6;++id) {
        const auto h=startup_world_human_details(s,id);if(!h)return {};
        in.rank_order.push_back(id);in.humans.push_back({id,h->profession,h->sex,s.human_calendar.at(id).contribution,h->effort,
            s.scene.world.world.ai.battle.humans.at(id).killed_stat1,s.scene.world.world.human_spending.at(id),
            s.human_homes.at(id)[2],h->medals,h->name});
    }
    const auto title="勋章授予式 "+std::to_string(in.cursor.tab+1)+"/3";
    in.options.view_y=23;const int width=int(font.measure(title));in.options.title_widths=std::array<int,2>{width,width};
    in.options.footer_widths=std::array<int,2>{int(font.measure("1个")),int(font.measure("还有"))};
    return steam_human_award_skin(in);
}
SteamHumanOverviewOptions effort_result_options(const ChineseFont &font) {
    SteamHumanOverviewOptions o;o.view_y=23;const int width=int(font.measure("努力奖励"));
    o.title_widths=std::array<int,2>{width,width};return o;
}
SteamHumanOverviewOptions award_result_options(const ChineseFont &font,std::optional<int> character_frame) {
    SteamHumanOverviewOptions o;o.view_y=23;o.character_frame=character_frame;
    const int width=int(font.measure("勋章授予式"));o.title_widths=std::array<int,2>{width,width};
    o.ceremony_message_widths={int(font.measure("恭喜!")),int(font.measure("太好了!"))};return o;
}
std::optional<SteamHumanOverviewPlan> award_result_example(const std::string &name,const ChineseFont &font) {
    StartupSession initial;StartupWorldRuntimeSession session(initial.state(),ref::WorldRandomStream::from_java_seed(1));
    const auto h=startup_world_human_details(session.state(),1);if(!h)return {};
    // 授予／举章／庆祝／奖励四个局部时点；不执行年度奖励或推进世界。
    SteamHumanAwardResultInput in;in.phase=name=="award-reward"?1:0;
    in.counter=in.phase?212:name=="award-celebration"?100:name=="award-lift"?65:35;
    in.human={h->profession,h->sex,h->equipment[0].value_or(-1),in.phase,in.counter,h->name,
        {{{20,30},{40,40},{20,10}}}};
    return steam_human_award_result_skin(in,award_result_options(font,12));
}
std::optional<SteamHumanOverviewPlan> effort_result_example(const ChineseFont &font) {
    StartupSession initial;StartupWorldRuntimeSession session(initial.state(),ref::WorldRandomStream::from_java_seed(1));
    const auto h=startup_world_human_details(session.state(),1);if(!h)return {};
    // 跨努力十位门槛和四战斗值为明确显示条件，不向世界发放奖励或重算人物。
    SteamHumanEffortResultInput in{h->profession,h->sex,73,19,25,h->name,
        {{{50,10,8,5},{55,11,9,6},{5,1,1,1}}}};
    return steam_human_effort_result_skin(in,effort_result_options(font));
}
SteamHumanOverviewOptions mastery_options(const ChineseFont &font,int bonus,int value) {
    SteamHumanOverviewOptions o;o.view_y=23;const int width=int(font.measure("职业大师"));
    o.title_widths=std::array<int,2>{width,width};
    constexpr const char *attributes[]{"体力","力","器用","丈夫","魔力","运"};
    constexpr const char *spells[]{"火魔法","冰魔法","雷魔法","回复魔法"};
    if(bonus>=0&&bonus<6)o.mastery_widths[0]=int(font.measure(attributes[bonus]));
    else if(bonus>=10&&bonus<14)o.mastery_widths[0]=int(font.measure(spells[bonus-10]));
    o.mastery_widths[1]=int(font.measure(std::to_string(value)));o.mastery_widths[2]=int(font.measure("上升了!"));
    o.mastery_button_widths={int(font.measure("转职")),int(font.measure("保持现状"))};return o;
}
std::optional<SteamHumanOverviewPlan> window_mastery_plan(
    const StartupWorldRuntimeState &s,std::uint64_t id,const ChineseFont &font) {
    const auto bound=s.page_human_bindings.find(id);if(bound==s.page_human_bindings.end()||!s.rules)return {};
    const auto h=startup_world_human_details(s,bound->second);
    if(!h||h->profession<0||h->profession>=int(s.rules->jobs.size()))return {};
    const auto &job=s.rules->jobs[h->profession];
    return steam_human_mastery_skin(s,id,mastery_options(font,job.mastery_attribute,job.mastery_value));
}
SteamClearScoreOptions clear_score_options(const SteamClearScoreInput &in,const ChineseFont &font) {
    SteamClearScoreOptions options;options.view_y=23;
    const int title=int(font.measure("通关点数发表"));options.title_widths=std::array<int,2>{title,title};
    if(in.stage==3&&in.stage_counter>=15&&in.rows&&in.row>=0&&in.row<6) {
        const auto number=steam_clear_score_number((*in.rows)[in.row].score);
        if(number)options.score_widths={int(font.measure(*number)),int(font.measure(*number))};
    }
    return options;
}
std::optional<SteamClearScorePlan> clear_score_example(const std::string &name,const ChineseFont &font) {
    StartupSession initial;StartupWorldRuntimeSession session(initial.state(),ref::WorldRandomStream::from_java_seed(1));
    const auto rows=startup_world_clear_score(session.state());if(!rows.candidate)return {};
    // 用原新局真实六类统计展示计分皮肤；仅阶段是条件，不将世界日期推到通关。
    SteamClearScoreInput in;in.rows=*rows.candidate;in.row=2;in.main_counter=31;
    if(name=="clear-count") {in.stage=1;in.stage_counter=45;}
    else if(name=="clear-score") {in.stage=3;in.stage_counter=45;}
    else {in.stage=name=="clear-record"?4:6;in.stage_counter=in.stage==4?1:65;in.row=6;}
    const int accounted=name=="clear-count"?2:name=="clear-score"?3:6;
    for(int row=0;row<accounted;++row)in.sum+=(*in.rows)[row].score;
    return steam_clear_score_skin(in,clear_score_options(in,font));
}
std::optional<SteamClearScorePlan> window_clear_score_plan(
    const StartupApplication &app,std::uint64_t id,const ChineseFont &font) {
    const auto input=steam_clear_score_input(app,id);
    return input?steam_clear_score_skin(*input,clear_score_options(*input,font)):std::nullopt;
}
SteamCommerceSkinOptions commerce_options(const ChineseFont &font) {
    SteamCommerceSkinOptions o;o.view_y=23;
    o.text.title="南瓜商会";o.text.welcome="欢迎光临";
    o.text.names={"购买道具","出售道具","购买设施"};
    o.text.explanations={"购买道具","出售道具","购买设施"};
    o.text.title_widths=std::array<int,2>{int(font.measure(o.text.title)),int(font.measure(o.text.title))};
    return o;
}
std::optional<SteamCommerceSkinPlan> window_commerce_plan(
    const StartupWorldRuntimeState &state,std::uint64_t id,const ChineseFont &font) {
    const auto input=steam_commerce_input(state,id);
    return input?steam_commerce_skin(*input,commerce_options(font)):std::nullopt;
}
std::optional<SteamCommerceSkinPlan> window_commerce_result_plan(
    const StartupWorldRuntimeState &state,std::uint64_t id,const ChineseFont &font) {
    const auto view=inspect_startup_world_commerce_page(state,id);
    if(!view||view->raw!=86)return {};
    const auto item=std::find_if(state.rules->items.begin(),state.rules->items.end(),
        [&](const auto &d){return d.identity==view->binding;});
    if(item==state.rules->items.end())return {};
    SteamCommerceResultOptions o;o.view_y=23;
    o.text.title="商店";o.text.cash_label="所持金";o.text.acknowledgement="了解";
    o.text.messages={"已购买","已出售"};
    o.text.title_widths=std::array<int,2>{int(font.measure(o.text.title)),int(font.measure(o.text.title))};
    o.text.name_width=int(font.measure(item->name));
    o.text.acknowledgement_width=int(font.measure(o.text.acknowledgement));
    return steam_commerce_result_skin(state,id,o);
}
std::optional<SteamCommerceSkinPlan> commerce_result_example(bool sale,const ChineseFont &font) {
    StartupSession initial;StartupWorldRuntimeSession session(initial.state(),ref::WorldRandomStream::from_java_seed(1));
    auto s=session.state();s.scripts.pages.front().lifecycle=3;s.scripts.executing_page.reset();
    // 仅开放位/已阅教程/补货是条件；实际83→84交易产生86，不把条件图称自然取得。
    s.scripts.user_flags|=16U;s.scripts.event_calls[97]=1;s.shop_item_stock.at(0).quantity=2;
    if(open_startup_world_commerce(s)!=StartupWorldRuntimeError::none||!initialize_startup_world_commerce_pages(s))return {};
    const auto parent=s.scripts.pages.back().id;
    StartupCommerceTopInput input;input.selection=sale?1:0;input.confirm=true;
    if(input_startup_world_commerce_top(s,parent,input)!=StartupWorldRuntimeError::none||
       !initialize_startup_world_commerce_pages(s))return {};
    const auto catalogue=s.scripts.pages.back().id;
    if(act_startup_world_commerce_page(s,catalogue,StartupCommerceAction::confirm)!=StartupWorldRuntimeError::none||
       !initialize_startup_world_commerce_pages(s))return {};
    return window_commerce_result_plan(s,s.scripts.pages.back().id,font);
}
std::optional<SteamCommerceSkinPlan> window_commerce_facility_plan(
    const StartupWorldRuntimeState &state,std::uint64_t id,const ChineseFont &font,bool first_touch=false) {
    SteamCommerceFacilityOptions o;o.view_y=23;o.scroll_first_touch=first_touch;
    o.text.title="购买设施";o.text.facility_header="设施";o.text.price_header="价格";
    o.text.explanation="可使用村子点数购买";
    o.text.title_widths=std::array<int,2>{int(font.measure(o.text.title)),int(font.measure(o.text.title))};
    o.text.price_width=int(font.measure(o.text.price_header));
    return steam_commerce_facility_skin(state,id,o);
}
std::optional<StartupWorldRuntimeState> commerce_facility_example_state(const std::string &name) {
    StartupSession initial;StartupWorldRuntimeSession session(initial.state(),ref::WorldRandomStream::from_java_seed(1));
    auto s=session.state();s.scripts.pages.front().lifecycle=3;s.scripts.executing_page.reset();
    if(name=="facility-recruitment"||name=="facility-recruitment-ready") {
        // 原24募集的100G与一次施工调用；不跳多年经营，不伪造设施实例。
        const auto started=begin_startup_world_build(s,24);
        if(started.error!=StartupWorldRuntimeError::none||started.denial!=StartupBuildDenial::none)return {};
        std::optional<std::uint64_t> site;
        for(int y=0;y<24&&!site;++y)for(int x=0;x<24&&!site;++x) {
            const auto placed=confirm_startup_world_build(s,{x,y},ref::FacilityOrientation::first);
            if(placed.error!=StartupWorldRuntimeError::none)return {};
            site=placed.created;
        }
        if(!site||cancel_startup_world_build(s)!=StartupWorldRuntimeError::none)return {};
        const auto adapter=startup_world_runtime_adapter();
        const auto built=ref::prepare_world_facility_update(adapter.facilities.read(s),*site,adapter.catalog);
        if(!built.candidate||!adapter.facilities.write(s,built.candidate->state))return {};
        if(name=="facility-recruitment-ready")
            s.shop_humans.at(1).satisfaction=s.rules->humans.at(1).residence_threshold;
        // ready只设满足度资格条件；不补人物实例、住宅、资金或原表，不宣称自然申请。
        if(open_startup_world_facility_page(s,*site)!=StartupWorldRuntimeError::none)return {};
        return s;
    }
    if(name=="facility-shop"||name=="facility-plant") {
        const int definition=name=="facility-shop"?30:66;
        const auto instance=std::find_if(s.scene.world.world.facilities.begin(),s.scene.world.world.facilities.end(),
            [&](const auto &entry){return entry.second.placement.definition_id==definition;});
        if(instance==s.scene.world.world.facilities.end()||
           open_startup_world_facility_page(s,instance->first)!=StartupWorldRuntimeError::none)return {};
        return s; // 原初局武器店30/向日葵66，只打开已有实例，不补建或改世界数值。
    }
    // 只读图例的开放位／点数／星级条件，不运行经营长前缀，不支付或领取建筑。
    s.scripts.user_flags|=16U;s.scripts.event_calls[97]=1;
    if(name=="commerce-buildings-funded")s.village_points=999;
    if(name=="commerce-buildings-tail")s.rank=2;
    const bool special_preview=name=="commerce-shop-preview"||name=="commerce-plant-preview";
    if(name=="commerce-building-preview"||special_preview)s.rank=5;
    if(open_startup_world_commerce(s)!=StartupWorldRuntimeError::none||!initialize_startup_world_commerce_pages(s))return {};
    StartupCommerceTopInput input;input.selection=2;input.confirm=true;
    if(input_startup_world_commerce_top(s,s.scripts.pages.back().id,input)!=StartupWorldRuntimeError::none||
       !initialize_startup_world_commerce_pages(s))return {};
    const auto id=s.scripts.pages.back().id;
    if(name=="commerce-buildings-tail") {
        StartupCommerceFacilityInput tail;tail.up=true;
        if(input_startup_world_commerce_facility(s,id,tail)!=StartupWorldRuntimeError::none)return {};
    }
    if(special_preview) {
        const auto view=inspect_startup_world_commerce_page(s,id);
        if(!view)return {};
        const auto chosen=std::find_if(view->entries.begin(),view->entries.end(),[&](int definition) {
            const auto d=std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),
                [&](const auto &value){return value.id==definition;});
            if(d==s.rules->facilities.end()||d->kind==12||d->detail==6)return false;
            const bool shop=d->detail==1||d->detail==4||d->detail==5;
            return name=="commerce-shop-preview"?shop:!shop&&d->kind==2;
        });
        if(chosen==view->entries.end())return {};
        StartupCommerceFacilityInput inspect;inspect.selection=static_cast<int>(chosen-view->entries.begin());inspect.inspect=true;
        if(input_startup_world_commerce_facility(s,id,inspect)!=StartupWorldRuntimeError::none)return {};
    }
    return s;
}
std::optional<SteamCommerceSkinPlan> commerce_facility_example(const std::string &name,const ChineseFont &font) {
    const auto state=commerce_facility_example_state(name);
    if(!state)return {};
    return window_commerce_facility_plan(*state,state->scripts.pages.back().id,font);
}
std::optional<SteamFacilityDetailSkinPlan> commerce_building_preview_example(const ChineseFont &font) {
    auto state=commerce_facility_example_state("commerce-building-preview");
    if(!state)return {};
    const auto parent=state->scripts.pages.back().id;
    const auto view=inspect_startup_world_commerce_page(*state,parent);
    if(!view||view->raw!=85)return {};
    const auto selected=std::find(view->entries.begin(),view->entries.end(),34);
    if(selected==view->entries.end())return {};
    // 原冻结p0目录中选择34，经实际85详情输入生成74；不安装建筑/邻接或付点。
    StartupCommerceFacilityInput input;input.selection=static_cast<int>(selected-view->entries.begin());input.inspect=true;
    if(input_startup_world_commerce_facility(*state,parent,input)!=StartupWorldRuntimeError::none)return {};
    return window_facility_definition_plan(*state,state->scripts.pages.back().id,font);
}
std::optional<SteamFacilityDetailSkinPlan> facility_special_example(const std::string &name,const ChineseFont &font) {
    const auto state=commerce_facility_example_state(name);
    if(!state)return {};
    return window_facility_special_plan(*state,state->scripts.pages.back().id,font);
}
std::optional<SteamConfigSkinPlan> config_example(const std::string &name,const ChineseFont &font) {
    // 显式条件值不是平台默认设置；不创建世界或触发实际音量/画质/保存。
    SteamConfigSkinInput in{{0,0,3,5},0,false};
    if(name=="config-locked")in.selection=1;
    if(name=="config-speed"){in.selection=1;in.speed_unlocked=true;in.values[1]=1;}
    if(name=="config-volume"){in.selection=3;in.values[2]=0;in.values[3]=5;}
    SteamConfigSkinOptions o;o.view_y=23;o.graphics_origin={0,30};o.on_top=true;o.steam_platform=true;
    o.platform_keyboard_touched=name=="config-volume";
    o.text.title="环境设置";o.text.names={"画面","速度","BGM","效果音"};
    o.text.choices={{{"画质优先","速度优先"},{"高速","普通"}}};
    if(in.selection==0)o.text.explanation={"优先速度时会省略部分","演出及显示效果"};
    else if(in.selection==1)o.text.explanation={"更改游戏的运行速度","通关游戏后可以更改"};
    else o.text.explanation={"设置游戏中的","音量大小"};
    const int width=int(font.measure(o.text.title));o.text.title_widths=std::array<int,2>{width,width};
    if(name=="config-volume")o.arrow_selected[1][1]=true;
    return steam_config_skin(in,o);
}
std::optional<SteamNewGamePlan> new_game_example(const std::string &name,const ChineseFont &font) {
    const auto &jobs=startup_world_rules().jobs;
    const auto adventure=std::find_if(jobs.begin(),jobs.end(),[](const auto &job){return (job.flags&4U)!=0;});
    if(adventure==jobs.end())return {};
    const int selection=name=="new-game-village"?0:name=="new-game-name"?1:name=="new-game-sex"?2:3;
    const StartupTitleDraft draft;
    // 草稿初值及已确认的男女默认名用于条件展示；不创建应用目录、修改原档或开始新局。
    SteamNewGameInput in{draft.village,selection<2?draft.main_character.name:"冒险花子",
        selection<2?0:1,selection,9,int(adventure-jobs.begin())};
    SteamNewGameOptions options;options.view_y=23;options.finger_frame=0;
    const int title=int(font.measure("重新开始"));options.title_widths=std::array<int,2>{title,title};
    options.start_width=int(font.measure("开始游戏"));
    return steam_new_game_skin(in,options);
}
std::optional<SteamHumanOverviewPlan> mastery_example(const std::string &name,const ChineseFont &font) {
    StartupSession initial;StartupWorldRuntimeSession session(initial.state(),ref::WorldRandomStream::from_java_seed(1));
    const auto &s=session.state();const auto h=startup_world_human_details(s,4);if(!h)return {};
    const auto &job=s.rules->jobs.at(h->profession);
    // 原人物/职业奖励定义加局部演出条件，不将新局人物升级到大师或执行奖励。
    SteamHumanMasteryInput in{h->profession,h->sex,name=="mastery-bonus"?1:name=="mastery-choice"?2:0,
        40,18,0,job.mastery_attribute,job.mastery_value,h->name,job.name};
    return steam_human_mastery_skin(in,mastery_options(font,in.bonus,in.value));
}
std::optional<SteamTownProgressPlan> gift_claim_example(const std::string &name,const ChineseFont &font) {
    StartupSession initial;StartupWorldRuntimeSession session(initial.state(),ref::WorldRandomStream::from_java_seed(1));
    const auto &s=session.state();const auto h=startup_world_human_details(s,4);if(!h)return {};
    SteamTownGiftInput in;in.raw_page=name=="gift-facility"?93:94;in.frame=40;
    in.options=window_town_options(s,in.raw_page,font);
    if(in.raw_page==93) {
        const auto &d=s.rules->facilities.at(33);in.kind=3;in.value=d.id;in.name=d.name;in.mapchip=d.display_id;
    } else {
        in.giver=SteamTownGiftInput::Giver{h->profession,h->sex,h->name};
        if(name=="gift-profession") {
            in.kind=4;in.value=h->profession;in.sex=h->sex;in.name=s.rules->jobs.at(in.value).name;
            in.options.bubble_widths[3]=int(font.measure("注目!"));
        } else if(name=="gift-item") {
            in.kind=8;in.value=0;in.name=s.rules->items.at(0).name;
            const auto icon=startup_world_item_icon_draws(s,0);if(!icon)return {};in.reward_icon=*icon;
        } else {
            in.kind=5;in.value=0;
            const auto d=std::find_if(s.rules->equipment.begin(),s.rules->equipment.end(),[](const auto &e){return e.shop.kind==1&&e.shop.id==0;});
            const auto icon=startup_world_equipment_icon_draws(s,1,0);if(d==s.rules->equipment.end()||!icon)return {};
            in.name=d->name;in.reward_icon=*icon;
        }
    }
    // 仅赠与来源和满40时点的显示条件，不实际开放/消费/增加库存。
    return steam_town_gift_skin(in);
}
SteamTaskVictoryOptions window_task_victory_options(const ChineseFont &font) {
    SteamTaskVictoryOptions o;o.view_y=23;
    const int width=int(font.measure("胜利！"));o.title_widths=std::array<int,2>{width,width};
    o.reputation_suffix_width=int(font.measure("上升了!"));return o;
}
std::optional<SteamTaskVictoryPlan> task_victory_example(const std::string &name,const ChineseFont &font) {
    // 数字与时钟是演出条件；只查询真实新局定义，不伪造自然任务结算。
    StartupSession initial;StartupWorldRuntimeSession session(initial.state(),ref::WorldRandomStream::from_java_seed(1));
    const auto &s=session.state();SteamTaskVictoryInput in;
    in.phase=name=="task-victory-reputation"?1:0;in.counter=50;in.secondary=18;
    in.experience=123;in.reputation=20;in.task_name=s.rules->tasks.front().name;
    for(int id=0;id<3;++id) {
        const auto human=startup_world_human_details(s,id);if(!human)return {};
        in.humans.push_back({id,human->profession,human->sex,human->equipment[0].value_or(-1)});
    }
    in.options=window_task_victory_options(font);return steam_task_victory_skin(in);
}
std::optional<SteamTaskDeadlinePlan> task_deadline_example(const std::string &name,const ChineseFont &font) {
    // 局部期限条件展示，地图与人物取原定义；不注入真实会话或推进自然期限。
    StartupSession initial;StartupWorldRuntimeSession session(initial.state(),ref::WorldRandomStream::from_java_seed(1));
    const auto &s=session.state();SteamTaskDeadlineInput in;
    in.kind=name=="task-deadline-battle"?1:0;in.phase=name=="task-deadline-continue"?1:0;
    in.counter=in.phase?24:12;in.grade=0;in.fee=100;in.affordable=true;
    if(in.phase)in.answer=0;
    const auto task=std::find_if(s.rules->tasks.begin(),s.rules->tasks.end(),[&](const auto &t){return t.factory.kind==in.kind&&(in.kind==0||t.factory.monster==0);});
    if(task==s.rules->tasks.end())return {};in.task_name=task->name;
    for(int id=0;id<3;++id) {
        const auto h=startup_world_human_details(s,id);if(!h)return {};
        in.participants.push_back(id);in.humans.push_back({id,h->profession,h->equipment[0].value_or(-1),0});
    }
    if(in.kind==0) {
        if(task->factory.facilities.empty())return {};
        const auto d=std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),[&](const auto &f){return f.id==task->factory.facilities.front();});
        if(d==s.rules->facilities.end())return {};in.mapchip=d->display_id;in.progress=35;
    } else {
        in.remaining=3;in.main_monster=task->factory.monster;
        const auto d=std::find_if(s.rules->monsters.begin(),s.rules->monsters.end(),[&](const auto &m){return m.identity==*in.main_monster;});
        if(d==s.rules->monsters.end())return {};
        // 影子独立读取原monster列5；三个相同定义仅为绘制条件。
        in.monsters.push_back({d->identity,d->initial.body,d->initial.sprite_variant,d->shadow});
        in.encounter_monsters={d->identity};
    }
    auto &o=in.options;o.view_y=23;const int title=int(font.measure("任务继续确认"));o.title_widths=std::array<int,2>{title,title};
    o.down_number_width=int(font.measure("0"));o.prediction_width=int(font.measure("再加把劲!!"));
    o.bubble_width=int(font.measure("加油哦!"));o.button_widths={int(font.measure("继续")),int(font.measure("中止"))};
    return steam_task_deadline_skin(in);
}
using TaskFollowupPlan=std::variant<SteamTaskRosterPlan,SteamTaskResultPlan,SteamTaskDeadlinePlan>;
std::optional<TaskFollowupPlan> window_task_followup_plan(
    const StartupWorldRuntimeState &s,std::uint64_t id,const ChineseFont &font,bool touching=false) {
    const auto page=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &p){return p.id==id;});
    const auto ui=s.task_page_ui.find(id);if(page==s.scripts.pages.end()||ui==s.task_page_ui.end())return {};
    const int raw=page->legacy_page;
    if(raw>=25&&raw<=28) {
        SteamTaskRosterOptions o;o.view_y=23;o.scroll_first_touch=touching;
        const std::string title=raw<=26?"参加者名单":raw==27?"参加委托 "+std::to_string(ui->second.tab+1)+"/2":"任务开始";
        const int width=int(font.measure(title));o.title_widths=std::array<int,2>{width,width};
        if(raw==28) {
            constexpr const char *predictions[]{"有些困难吧","有点担心","这些人真是了不起","我觉得会好的。","值得期待！","能轻松通关！","他们能做到!"};
            const auto grade=s.task_page_predictions.find(id);
            if(grade==s.task_page_predictions.end()||grade->second<0||grade->second>6)return {};
            o.prediction_width=int(font.measure(predictions[grade->second]));
            o.button_widths={int(font.measure("立即开始")),int(font.measure("重新考虑"))};
        }
        const auto plan=steam_task_roster_skin(s,id,o);if(plan)return TaskFollowupPlan{*plan};
    } else if(raw==31||raw==32) {
        auto in=steam_task_result_input(s,id);if(!in)return {};
        auto &o=in->options;o.view_y=23;o.scroll_first_touch=touching;
        const int width=int(font.measure("成果"));o.title_widths=std::array<int,2>{width,width};
        o.bottom_width=int(font.measure(raw==31?in->task_name+"完成!":"获得了宝物!"));
        const auto plan=steam_task_result_skin(*in);if(plan)return TaskFollowupPlan{*plan};
    } else if(raw==33) {
        auto in=steam_task_deadline_input(s,id);if(!in)return {};
        auto &o=in->options;o.view_y=23;
        const int width=int(font.measure("任务继续确认"));o.title_widths=std::array<int,2>{width,width};
        std::int64_t downs=0;
        for(const int human:in->participants) {
            const auto h=std::find_if(in->humans.begin(),in->humans.end(),[=](const auto &v){return v.identity==human;});
            if(h==in->humans.end()||h->downs<0)return {};downs+=h->downs;
        }
        constexpr const char *predictions[]{"再加把劲!!","再加加油!!","需要补充战力!","重新来过比较好","应该撤退…"};
        if(in->grade<0||in->grade>4)return {};
        o.down_number_width=int(font.measure(std::to_string(downs)));o.prediction_width=int(font.measure(predictions[in->grade]));
        o.bubble_width=int(font.measure(in->answer.value_or(0)==0?"加油哦!":"遗憾…"));
        o.button_widths={int(font.measure("继续")),int(font.measure("中止"))};
        const auto plan=steam_task_deadline_skin(*in);if(plan)return TaskFollowupPlan{*plan};
    }
    return {};
}
std::optional<SteamMagicPotPlan> window_magic_pot_plan(
    const StartupWorldRuntimeState &s,std::uint64_t id,const ChineseFont &font,bool first_touch=false) {
    const auto view=inspect_startup_world_magic_pot_page(s,id);
    if(!view||!s.rules)return {};
    SteamMagicPotOptions o;o.view_y=23;o.scroll_first_touch=first_touch;
    const std::string title=view->raw==43?"选择调和书 "+std::to_string(view->phase+1)+"/2":
        view->raw==45?"研究报告":view->raw>=46?"调和":"魔法壶";
    o.title_widths=std::array<int,2>{int(font.measure(title)),int(font.measure(title))};
    if(view->raw<=43) {
        o.button_widths={int(font.measure("放入道具")),int(font.measure("查看调和书"))};
        o.hint_width=int(font.measure(view->selection?"使用魔法点数生成道具":"放入道具获得魔法点数"));
        return steam_magic_pot_skin(s,id,o);
    }
    SteamMagicPotEffectOptions effect;effect.common=o;
    if(view->raw==44)effect.comment_width=int(font.measure(s.magic_pot_comment));
    if(view->raw==45)effect.report_header_widths=std::array<int,4>{int(font.measure(std::to_string(s.legacy_n[2]))),
        int(font.measure("素材",11)),int(font.measure("消费",11)),int(font.measure("素材",11))};
    if(view->raw>=46) {
        const auto r=std::find_if(s.rules->magic_pot_recipes.begin(),s.rules->magic_pot_recipes.end(),
            [&](const auto &v){return v.identity==view->binding;});
        if(r==s.rules->magic_pot_recipes.end())return {};
        std::string name;bool affordable=true;
        for(int i=0;i<4;++i)affordable&=s.legacy_n[i+3]>=r->costs[i];
        if(r->reward_kind==0) {
            const auto d=std::find_if(s.rules->items.begin(),s.rules->items.end(),[&](const auto &v){return v.identity==r->reward_definition;});
            if(d==s.rules->items.end())return {};name=d->name;
        } else if(r->reward_kind<4) {
            const auto d=std::find_if(s.rules->equipment.begin(),s.rules->equipment.end(),[&](const auto &v){return v.shop.kind==r->reward_kind&&v.shop.id==r->reward_definition;});
            if(d==s.rules->equipment.end())return {};name=d->name;
        } else {
            const auto d=std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),[&](const auto &v){return v.id==r->reward_definition;});
            if(d==s.rules->facilities.end())return {};name=d->name;
        }
        effect.reward_name_width=int(font.measure(name));
        effect.confirm_width=int(font.measure(view->raw==47&&affordable?"制作!":"了解"));
    }
    return steam_magic_pot_effect_skin(s,id,effect);
}
// 仅有界参考图：明确条件Owner，借实际菜单／投入消费者取得载荷，不安装运行Session。
StartupWorldRuntimeState magic_pot_example(const std::string &name) {
    StartupSession initial;StartupWorldRuntimeSession session(initial.state(),ref::WorldRandomStream::from_java_seed(1));
    auto s=session.state();s.scripts.pages.front().lifecycle=3;s.scripts.executing_page.reset();
    s.scripts.user_flags|=3U;s.scripts.event_calls[101]=1;
    const auto ready=[&] {
        if(!initialize_startup_world_magic_pot_pages(s))throw std::runtime_error("Magic example initialization rejected");
        auto &page=s.scripts.pages.back();page.lifecycle=2;
        if(!inspect_startup_world_magic_pot_page(s,page.id))throw std::runtime_error("Magic example payload rejected");
        return page.id;
    };
    if(open_startup_world_magic_pot(s,StartupMagicPotEntry::main_menu)!=StartupWorldRuntimeError::none)
        throw std::runtime_error("Magic example entry rejected");
    const auto root=ready();
    if(name=="magic-pot-main")return s;
    const auto act=[&](std::uint64_t id,StartupMagicPotAction action,int value=0) {
        if(act_startup_world_magic_pot_page(s,id,action,value)!=StartupWorldRuntimeError::none)
            throw std::runtime_error("Magic example actual action rejected");
    };
    if(name=="magic-pot-items"||name=="magic-pot-deposit") {
        act(root,StartupMagicPotAction::confirm);const auto list=ready();
        if(name=="magic-pot-items")return s;
        act(list,StartupMagicPotAction::confirm);const auto animation=ready();
        s.page_counters.at(animation)=97;s.page_secondary_counters.at(animation)=17;return s;
    }
    if(name=="magic-pot-report") {
        // 原满容量但零待元素的补1分支；输入条件与自然经营分开。
        auto pot=s.legacy_n;pot[1]=*ref::world_magic_pot_capacity(pot[11]);
        const ref::WorldMagicPotDate date{s.scene.calendar.year,s.scene.calendar.month,s.scene.calendar.subperiod};
        pot[12]=*ref::world_magic_pot_date(date).value-pot[1];
        const auto result=ref::prepare_world_magic_pot_processing(pot,date);
        if(!result.candidate||!result.candidate->changed)throw std::runtime_error("Magic report fixture rejected");
        s.legacy_n=result.candidate->state;s.magic_pot_display=result.candidate->display;s.magic_pot_output=result.candidate->produced;
        ref::WorldScriptPage page;page.id=s.scripts.next_page_id++;page.kind=ref::WorldScriptPageKind::raw_page;
        page.legacy_page=45;page.lifecycle=0;s.scripts.pages.push_back(page);
        const auto id=ready();s.page_counters.at(id)=83;return s;
    }
    const auto recipe=std::find_if(s.rules->magic_pot_recipes.begin(),s.rules->magic_pot_recipes.end(),
        [](const auto &r){return (r.flags&2U)&&r.reward_kind==0;});
    if(recipe==s.rules->magic_pot_recipes.end())throw std::runtime_error("Magic recipe fixture source missing");
    if(name=="magic-pot-discovery") {
        s.magic_pot_recipes.at(recipe->identity).status=0;
        ref::WorldScriptPage page;page.id=s.scripts.next_page_id++;page.kind=ref::WorldScriptPageKind::raw_page;
        page.legacy_page=46;page.legacy_g=recipe->identity;page.lifecycle=0;s.scripts.pages.push_back(page);
        const auto id=ready();s.page_counters.at(id)=7;return s;
    }
    act(root,StartupMagicPotAction::select,1);act(root,StartupMagicPotAction::confirm);const auto catalogue=ready();
    if(name=="magic-pot-recipes")return s;
    if(name=="magic-pot-recipe-costs"){act(catalogue,StartupMagicPotAction::next_tab);return s;}
    if(name!="magic-pot-confirm")throw std::runtime_error("Unknown magic example");
    s.magic_pot_recipes.at(recipe->identity).status=1;
    for(int i=0;i<4;++i)s.legacy_n[3+i]=recipe->costs[i];
    const auto &list=s.magic_pot_page_lists.at(catalogue);const auto row=std::find(list.begin(),list.end(),recipe->identity);
    if(row==list.end())throw std::runtime_error("Magic recipe not in actual catalogue");
    act(catalogue,StartupMagicPotAction::select,int(row-list.begin()));act(catalogue,StartupMagicPotAction::confirm);
    const auto id=ready();s.page_counters.at(id)=7;return s;
}
template<class Plan> std::vector<SteamStartupTouch> management_touches(const Plan &plan,SourceSprites &sprites) {
    std::vector<SteamStartupTouch> result;
    for(const auto &op:plan.operations)std::visit([&](const auto &value) {
        using T=std::decay_t<decltype(value)>;
        if constexpr(std::is_same_v<T,SteamStartupTouch>)result.push_back(value);
        else if constexpr(std::is_same_v<T,SteamFacilityArrowTouch>) {
            const auto arrow=facility_detail_arrow(value,sprites);
            result.push_back({1,value.value,arrow.touch_rectangle,{},0});
        }
        else if constexpr(std::is_same_v<T,SteamHumanEquipmentRow>||std::is_same_v<T,SteamFacilityEffectTouch>||std::is_same_v<T,SteamTaskRosterTouch>) {
            auto touch=value.touch;
            if(!touch.rectangle&&touch.image_draw&&*touch.image_draw<plan.operations.size()) {
                const auto *image=std::get_if<SteamFacilityImage>(&plan.operations[*touch.image_draw]);
                if(!image||image->asset!=SteamFacilityAsset::title_arrows)throw std::runtime_error("管理箭头来源拒绝");
                auto r=sprites.manual_arrow_bounds(image->frame);r[0]+=image->position[0];r[1]+=image->position[1];touch.rectangle=r;
            }
            if(touch.rectangle) {
                auto &r=*touch.rectangle;r[0]+=value.margin[0];r[1]+=value.margin[2];
                r[2]+=value.margin[1]-value.margin[0];r[3]+=value.margin[3]-value.margin[2];
            }
            result.push_back(touch);
        }
    },op);
    return result;
}
std::optional<SteamHumanOverviewPlan> window_commodity_plan(
    const StartupWorldRuntimeState &s,std::uint64_t id,const ChineseFont &font,bool first_touch=false) {
    const auto view=inspect_startup_world_facility_catalog_page(s,id);
    if(!view||(view->raw!=79&&view->raw!=72))return {};
    SteamHumanOverviewOptions o;o.view_y=23;o.finger_frame=0;o.catalogue_scroll_first_touch=first_touch;
    const auto title=view->raw==79?"商品 "+std::to_string(view->phase+1)+"/2":
        "装备信息 "+std::to_string(view->phase+1)+"/"+std::to_string(view->entries.size());
    o.title_widths=std::array<int,2>{int(font.measure(title)),int(font.measure(title))};
    if(view->raw==72)return steam_facility_commodity_detail_skin(s,id,o);
    const auto bottom=std::to_string(view->entries.size())+"种"+(view->mode==1?"武器":view->mode==4?"防具":"饰品")+"出售中";
    o.bottom_widths[0]=int(font.measure(bottom));
    return steam_facility_commodity_skin(s,id,o);
}
std::optional<SteamHumanOverviewPlan> window_human_catalogue_plan(
    const StartupWorldRuntimeState &s,std::uint64_t id,const ChineseFont &font,bool first_touch=false) {
    const auto page=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &p){return p.id==id;});
    const auto cursor=inspect_startup_world_human_catalogue_cursor(s,id);
    if(!cursor||page==s.scripts.pages.end())return {};
    SteamHumanOverviewOptions o;o.view_y=23;o.finger_frame=0;o.catalogue_scroll_first_touch=first_touch;
    const int phase=s.page_phases.find(id)->second;
    const auto title=page->legacy_page==61?std::string("职业一览 ")+std::to_string(phase+1)+"/2":
        std::string("赠送礼物 ")+std::to_string(phase+1)+"/5";
    o.title_widths=std::array<int,2>{int(font.measure(title)),int(font.measure(title))};
    if(page->legacy_page==61)return steam_human_profession_catalogue_skin(s,id,o);
    if(phase==4&&cursor->count) {
        const int item=s.equipment_page_catalogs.find(id)->second[4][cursor->selection];
        const auto definition=std::find_if(s.rules->items.begin(),s.rules->items.end(),[=](const auto &d){return d.identity==item;});
        if(definition==s.rules->items.end())return {};
        if(definition->effect>=0&&definition->effect<6) {
            constexpr std::array<const char *,6> names{"体力","力量","灵巧","耐久","魔力","运气"};
            o.item_explain_widths[0]=int(font.measure(names[definition->effect]));
            o.item_explain_widths[1]=int(font.measure(definition->effect==0?"最大值上升":"上升"));
        } else if(definition->effect==9) {
            if(definition->spell<0||definition->spell>3)return {};
            o.item_explain_widths[2]=int(font.measure(std::string(human_overview_label(
                SteamHumanOverviewTextRole::spell,false,definition->spell))+"习得"));
        }
    }
    return steam_human_gift_catalogue_skin(s,id,o);
}
std::optional<SteamHumanOverviewPlan> window_human_profession_plan(
    const StartupWorldRuntimeState &s,std::uint64_t id,const ChineseFont &font) {
    const auto p=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &v){return v.id==id;});
    const auto binding=s.page_human_bindings.find(id),job=s.page_job_bindings.find(id);
    if(!s.rules||p==s.scripts.pages.end()||binding==s.page_human_bindings.end()||job==s.page_job_bindings.end()||
       job->second<0||job->second>=static_cast<int>(s.rules->jobs.size()))return {};
    SteamHumanOverviewOptions o;o.view_y=23;o.finger_frame=0;
    if(p->legacy_page==62) {
        const auto h=startup_world_human_details(s,binding->second);
        const auto list=s.human_page_catalogs.find(id);
        const auto selected=s.human_page_selections.find(id);
        if(!h||list==s.human_page_catalogs.end()||selected==s.human_page_selections.end())return {};
        const auto title="职业一览 "+std::to_string(selected->second+1)+"/"+std::to_string(list->second.size());
        o.title_widths=std::array<int,2>{int(font.measure(title)),int(font.measure(title))};
        o.profession_change_width=int(font.measure(h->profession==job->second?"当前职业":"转职"));
        o.insufficient_points_width=int(font.measure("村子点数不足"));
        return steam_human_profession_preview_skin(s,id,o);
    }
    if(p->legacy_page!=63)return {};
    o.title_widths=std::array<int,2>{int(font.measure("转职")),int(font.measure("转职"))};
    const auto message=s.human_gift_messages.find(id);if(message==s.human_gift_messages.end())return {};
    o.message_width=int(font.measure(message->second));
    o.character_frame=steam_human_shared_character_frame(s);
    return steam_human_profession_result_skin(s,id,o);
}
std::optional<SteamHumanOverviewPlan> window_human_gift_result_plan(
    const StartupWorldRuntimeState &s,std::uint64_t id,const ChineseFont &font) {
    const auto message=s.human_gift_messages.find(id);
    const auto counter=s.page_counters.find(id);
    const auto choice=s.human_equipment_choices.find(id);
    if(!s.rules||message==s.human_gift_messages.end()||counter==s.page_counters.end()||choice==s.human_equipment_choices.end())return {};
    std::string text=message->second;
    if(choice->second[0]==4) {
        const auto item=std::find_if(s.rules->items.begin(),s.rules->items.end(),[&](const auto &d){return d.identity==choice->second[1];});
        if(item==s.rules->items.end())return {};
        if(item->recovery>0&&counter->second>=115)text="HP回复!";
        else if(item->effect==9) {
            constexpr const char *magic[]{"火魔法","冰魔法","雷魔法","回复魔法"};
            if(item->spell<0||item->spell>3)return {};
            text=std::string(magic[item->spell])+"习得";
        }
    }
    SteamHumanOverviewOptions o;o.view_y=23;o.character_frame=steam_human_shared_character_frame(s);o.message_width=int(font.measure(text));
    o.title_widths=std::array<int,2>{int(font.measure("赠送礼物")),int(font.measure("赠送礼物"))};
    return steam_human_gift_result_skin(s,id,o);
}
std::optional<SteamHumanOverviewPlan> window_human_attribute_result_plan(
    const StartupWorldRuntimeState &s,std::uint64_t id,const ChineseFont &font) {
    const auto page=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &p){return p.id==id;});
    const auto choice=s.human_equipment_choices.find(id);
    if(!s.rules||page==s.scripts.pages.end()||choice==s.human_equipment_choices.end())return {};
    const int raw=page->legacy_page,slot=choice->second[0],item=choice->second[1];
    std::string name;
    if(raw==68&&slot>=0&&slot<4) {
        const int kind=slot==0?1:slot==3?3:2;
        const auto d=std::find_if(s.rules->equipment.begin(),s.rules->equipment.end(),[=](const auto &e){return e.shop.kind==kind&&e.shop.id==item;});
        if(d==s.rules->equipment.end())return {};name=d->name;
    } else if(raw==69&&slot==4) {
        const auto d=std::find_if(s.rules->items.begin(),s.rules->items.end(),[=](const auto &e){return e.identity==item;});
        if(d==s.rules->items.end())return {};name=d->name;
    } else return {};
    const std::string title=raw==68?"装备变更":"能力上升",suffix=raw==68?"装备":"赠送";
    SteamHumanOverviewOptions o;o.view_y=23;
    o.title_widths=std::array<int,2>{int(font.measure(title)),int(font.measure(title))};
    o.result_name_width=int(font.measure(name,raw==68?11.F:12.F));
    o.result_suffix_width=int(font.measure(suffix,raw==68?11.F:12.F));
    return steam_human_attribute_result_skin(s,id,o);
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
    const bool magic_fixture=inspect_page.rfind("magic-pot-",0)==0;
    const bool roster_fixture=inspect_page=="task-roster"||inspect_page=="task-extra"||inspect_page=="task-extra-stats"||inspect_page=="task-departure"||inspect_page=="task-departing";
    const bool info_fixture=inspect_page=="task-map-info"||inspect_page=="task-info";
    const bool result_fixture=inspect_page=="task-battle-result"||inspect_page=="task-reward-result";
    const bool victory_fixture=inspect_page=="task-victory"||inspect_page=="task-victory-reputation";
    const bool deadline_fixture=inspect_page=="task-deadline"||inspect_page=="task-deadline-battle"||inspect_page=="task-deadline-continue";
    const bool residence_fixture=inspect_page=="residence-candidates"||inspect_page=="residence-completed"||inspect_page=="residence-reward";
    const bool award_fixture=inspect_page=="award-contribution"||inspect_page=="award-income"||inspect_page=="award-residents";
    const bool effort_fixture=inspect_page=="effort-result";
    const bool mastery_fixture=inspect_page=="mastery-announcement"||inspect_page=="mastery-bonus"||inspect_page=="mastery-choice";
    const bool clear_fixture=inspect_page=="clear-count"||inspect_page=="clear-score"||inspect_page=="clear-record"||inspect_page=="clear-closing";
    const bool new_game_fixture=inspect_page=="new-game-village"||inspect_page=="new-game-name"||inspect_page=="new-game-sex"||inspect_page=="new-game-start";
    const bool config_fixture=inspect_page=="config-quality"||inspect_page=="config-locked"||inspect_page=="config-speed"||inspect_page=="config-volume";
    const bool commerce_fixture=inspect_page=="commerce-buy"||inspect_page=="commerce-sell"||inspect_page=="commerce-facility";
    const bool commerce_result_fixture=inspect_page=="commerce-bought"||inspect_page=="commerce-sold";
    const bool commerce_facility_fixture=inspect_page=="commerce-buildings"||inspect_page=="commerce-buildings-funded"||inspect_page=="commerce-buildings-tail";
    const bool commerce_preview_fixture=inspect_page=="commerce-building-preview";
    const bool facility_special_fixture=inspect_page=="facility-shop"||inspect_page=="facility-plant"||
        inspect_page=="facility-recruitment"||inspect_page=="facility-recruitment-ready"||
        inspect_page=="commerce-shop-preview"||inspect_page=="commerce-plant-preview";
    const bool gift_claim_fixture=inspect_page=="gift-facility"||inspect_page=="gift-profession"||inspect_page=="gift-equipment"||inspect_page=="gift-item";
    const bool award_result_fixture=inspect_page=="award-giving"||inspect_page=="award-lift"||inspect_page=="award-celebration"||inspect_page=="award-reward";
    const bool task_fixture=info_fixture||result_fixture||victory_fixture||deadline_fixture||roster_fixture||inspect_page=="task-selection"||inspect_page=="task-offer"||inspect_page=="task-recruitment";
    const bool management_fixture=inspect_page.rfind("village-",0)==0||inspect_page.rfind("town-",0)==0;
    const bool reputation_fixture=inspect_page=="facility-reputation"||inspect_page=="facility-reputation-result";
    std::string reputation_glyphs;
    if(reputation_fixture) {
        reputation_glyphs="传说中的商店冒险者们中间的话题传了开来街道人气上升口碑绘制夹具条件载荷无自然触发认证";
        for(const auto &d:startup_world_rules().facilities)if(d.id==33)reputation_glyphs+=d.name;
    }
    if(management_fixture) {
        reputation_glyphs+="开展活动活动结束价格还有辛苦了姓名上升满足冒险者的街道的人气提升了不开展升级申请了解的升级条件街道等级上升月收入设施数居住数建设任务完成数举办活动数恭喜太好了万岁请可以开展了可实施了研究绘制夹具无自然触发认证○×「」";
        for(const auto &a:startup_world_rules().activities)reputation_glyphs+=a.name+a.detail+a.description;
        for(const auto &d:startup_world_rules().facilities)reputation_glyphs+=d.name;
        for(const auto &h:startup_world_rules().humans)reputation_glyphs+=h.name;
    }
    if(magic_fixture) {
        reputation_glyphs+="选择调和书放入道具查看使用魔法点数生成获得作成物持有数研究报告素材消费好好煮一煮可以制作了要制作吗不足了解知识火冰雷暗相性似乎不错成功感觉就那样吧嗯魔法壶姓名剩余未获得研究绘制夹具无自然触发认证？？？？？";
        for(const auto &d:startup_world_rules().items)reputation_glyphs+=d.name;
        for(const auto &d:startup_world_rules().equipment)reputation_glyphs+=d.name;
        for(const auto &d:startup_world_rules().facilities)reputation_glyphs+=d.name;
    }
    if(task_fixture) {
        reputation_glyphs+="胜利任务继续确认下降次数再加把劲再加加油需要补充战力重新来过比较好应该撤退中止遗憾…哦因为街道的人气上升了冒险者们辛苦攻略了解成果打倒数下降完成获得了宝物参加者名单参加委托任务开始立即开始重新考虑我觉得会好的出发追加加油详细开始。任务选择寻求冒险者姓名费用请选择募集交给我多指教来了我会加油的我要参加好期待合计人研究绘制夹具无自然触发认证查看位置！!";
        for(const auto &d:startup_world_rules().tasks)reputation_glyphs+=d.name;
        for(const auto &d:startup_world_rules().humans)reputation_glyphs+=d.name;
        for(const auto &d:startup_world_rules().items)reputation_glyphs+=d.name;
        for(const auto &d:startup_world_rules().equipment)reputation_glyphs+=d.name;
    }
    if(residence_fixture||award_fixture||effort_fixture||award_result_fixture||mastery_fixture||gift_claim_fixture) {
        reputation_glyphs+="选择入住者姓名费用请研究绘制夹具无自然触发认证自宅完成的满意努力上升勋章授予式贡献获得消费居住还有个详细终止奖励基础能力恭喜太好了!职业大师体力器用丈夫魔法运回复保持现状转职学会火冰雷→";
        for(const auto &human:startup_world_rules().humans)reputation_glyphs+=human.name;
        for(const auto &job:startup_world_rules().jobs)reputation_glyphs+=job.name;
        reputation_glyphs+="活动注目可以建设因为的介绍可转职被获得了。「」";
        for(const auto &d:startup_world_rules().facilities)reputation_glyphs+=d.name;
        for(const auto &d:startup_world_rules().items)reputation_glyphs+=d.name;
        for(const auto &d:startup_world_rules().equipment)reputation_glyphs+=d.name;
    }
    if(clear_fixture)reputation_glyphs+="通关点数发表那么赶紧开始计算吧任务完成数设施发现数冒险者全员合计自宅努力度总和追加排行榜更新接下来就是这样当前最高分研究绘制夹具无自然触发认证…！";
    if(new_game_fixture)reputation_glyphs+="重新开始城镇名称冒险者姓名性别男女开始游戏口袋冒险村太郎花子研究绘制夹具无自然触发认证";
    if(config_fixture)reputation_glyphs+="环境设置画面速度效果音画质优先高速普通更改游戏的运行通关后可以设置中大小部分演出及显示研究绘制夹具无自然触发认证省略";
    if(commerce_fixture)reputation_glyphs+="南瓜商会欢迎光临购买出售道具设施研究绘制夹具无自然触发认证返回";
    if(commerce_result_fixture) {
        reputation_glyphs+="研究绘制夹具无自然触发认证";
        for(const auto &d:startup_world_rules().items)reputation_glyphs+=d.name;
    }
    if(commerce_facility_fixture)reputation_glyphs+="研究绘制夹具无自然触发认证详细";
    if(commerce_preview_fixture) {
        reputation_glyphs+="设施信息价格品质魅力距下一等级还有达到最高级返回研究绘制夹具无自然触发认证";
        for(const auto &d:startup_world_rules().facilities)if(d.id==34)reputation_glyphs+=d.name;
    }
    if(facility_special_fixture) {
        reputation_glyphs+="现在没有希望入住的人现在有人希望入住入住希望者";
        reputation_glyphs+="设施信息种类武器防具饰品出售中查看商品价格品质会给周围带来好的影响返回研究绘制夹具无自然触发认证";
        for(const auto &d:startup_world_rules().facilities)reputation_glyphs+=d.name;
    }
    ChineseFont font(font_path,reputation_glyphs);
    if(magic_fixture||task_fixture||residence_fixture||award_fixture||effort_fixture||award_result_fixture||mastery_fixture||gift_claim_fixture||clear_fixture||new_game_fixture||config_fixture||commerce_fixture||commerce_result_fixture||commerce_facility_fixture||commerce_preview_fixture||facility_special_fixture) {
        std::variant<SteamMagicPotPlan,SteamTaskSelectionPlan,SteamTaskRosterPlan,SteamTaskResultPlan,SteamTaskVictoryPlan,SteamTaskDeadlinePlan,SteamHumanOverviewPlan,SteamTownProgressPlan,SteamNewGamePlan,SteamConfigSkinPlan,SteamCommerceSkinPlan,SteamFacilityDetailSkinPlan> plan;
        if(facility_special_fixture) {
            const auto value=facility_special_example(inspect_page,font);
            if(!value)throw std::runtime_error("Facility special example rejected");plan=*value;
        } else if(commerce_preview_fixture) {
            const auto value=commerce_building_preview_example(font);
            if(!value)throw std::runtime_error("Commerce building preview example rejected");plan=*value;
        } else if(commerce_facility_fixture) {
            const auto value=commerce_facility_example(inspect_page,font);
            if(!value)throw std::runtime_error("Commerce facility example rejected");plan=*value;
        } else if(commerce_result_fixture) {
            const auto value=commerce_result_example(inspect_page=="commerce-sold",font);
            if(!value)throw std::runtime_error("Commerce result example rejected");plan=*value;
        } else if(commerce_fixture) {
            // 只读表现条件；不创建或修改原版经营进度。
            const int row=inspect_page=="commerce-buy"?0:inspect_page=="commerce-sell"?1:2;
            const auto value=steam_commerce_skin({row,0,row==0,row==2},commerce_options(font));
            if(!value)throw std::runtime_error("Commerce example rejected");plan=*value;
        } else if(config_fixture) {
            const auto value=config_example(inspect_page,font);
            if(!value)throw std::runtime_error("Config example rejected");plan=*value;
        } else if(new_game_fixture) {
            const auto value=new_game_example(inspect_page,font);
            if(!value)throw std::runtime_error("New game example rejected");plan=*value;
        } else if(clear_fixture) {
            const auto value=clear_score_example(inspect_page,font);
            if(!value)throw std::runtime_error("Clear score example rejected");plan=*value;
        } else if(gift_claim_fixture) {
            const auto value=gift_claim_example(inspect_page,font);
            if(!value)throw std::runtime_error("Gift claim example rejected");plan=*value;
        } else if(mastery_fixture) {
            const auto value=mastery_example(inspect_page,font);
            if(!value)throw std::runtime_error("Mastery example rejected");plan=*value;
        } else if(award_result_fixture) {
            const auto value=award_result_example(inspect_page,font);
            if(!value)throw std::runtime_error("Award ceremony example rejected");plan=*value;
        } else if(effort_fixture) {
            const auto value=effort_result_example(font);
            if(!value)throw std::runtime_error("Effort example rejected");plan=*value;
        } else if(award_fixture) {
            const auto value=award_catalogue_example(inspect_page,font);
            if(!value)throw std::runtime_error("Award catalogue example rejected");plan=*value;
        } else if(residence_fixture) {
            const auto value=inspect_page=="residence-candidates"?residence_candidates_example(font):residence_result_example(inspect_page,font);
            if(!value)throw std::runtime_error("Residence candidate example rejected");plan=*value;
        } else if(magic_fixture) {
            const auto source=magic_pot_example(inspect_page);
            const auto value=window_magic_pot_plan(source,source.scripts.pages.back().id,font);
            if(!value)throw std::runtime_error("Magic example plan rejected");plan=*value;
        } else if(info_fixture) {
            const auto value=task_info_example(inspect_page=="task-map-info",font);
            if(!value)throw std::runtime_error("Task info example plan rejected");plan=*value;
        } else if(result_fixture) {
            const auto value=task_result_example(inspect_page,font);
            if(!value)throw std::runtime_error("Task result example plan rejected");plan=*value;
        } else if(victory_fixture) {
            const auto value=task_victory_example(inspect_page,font);
            if(!value)throw std::runtime_error("Task victory example plan rejected");plan=*value;
        } else if(deadline_fixture) {
            const auto value=task_deadline_example(inspect_page,font);
            if(!value)throw std::runtime_error("Task deadline example plan rejected");plan=*value;
        } else if(roster_fixture) {
            const auto value=task_roster_example(inspect_page,font);
            if(!value)throw std::runtime_error("Task roster example plan rejected");plan=*value;
        } else {
            const auto value=task_selection_example(inspect_page,font);
            if(!value)throw std::runtime_error("Task example plan rejected");plan=*value;
        }
        int count{};
        while(!WindowShouldClose()&&count<frames) {
            BeginTextureMode(canvas.value);ClearBackground(Color{83,141,77,255});
            font.text("研究绘制夹具",8,10,WHITE,10);std::visit([&](const auto &value){
                if constexpr(std::is_same_v<std::decay_t<decltype(value)>,SteamFacilityDetailSkinPlan>)draw_facility_detail(value,sprites,font);
                else draw_facility_ceremony(value,sprites,font,info_fixture?Vector2{0,0}:human_overview_origin);
            },plan);
            font.text("无自然触发认证",8,new_game_fixture?300:260,WHITE,9);EndTextureMode();BeginDrawing();ClearBackground(BLACK);
            DrawTexturePro(canvas.value.texture,{0,0,width,-height},{0,0,width*scale,height*scale},{0,0},0,WHITE);
            EndDrawing();++count;
            if(count==frames&&screenshot) {
                if(!screenshot->parent_path().empty())std::filesystem::create_directories(screenshot->parent_path());
                auto pixels=LoadImageFromScreen();if(!pixels.data)throw std::runtime_error("Magic screenshot missing");
                const bool saved=ExportImage(pixels,screenshot->string().c_str());UnloadImage(pixels);
                if(!saved)throw std::runtime_error("Magic screenshot refused");
            }
        }
        std::cout<<"drawing fixture closed: page="<<inspect_page<<" frames="<<count<<" world_updates=0 natural_trigger_verified=false\n";
        return 0;
    }
    if(management_fixture) {
        // 仅例图的条件载荷，原定义和初值来自固定表；不安装世界或宣称自然经营。
        StartupSession original;
        StartupWorldRuntimeSession base(original.state(),ref::WorldRandomStream::from_java_seed(1));
        auto source=base.state();
        std::variant<SteamVillageActivityPlan,SteamTownProgressPlan> plan;
        if(inspect_page.rfind("village-",0)==0) {
            const auto d=std::find_if(source.rules->activities.begin(),source.rules->activities.end(),[](const auto &a){return a.identity==0;});
            const auto h=startup_world_human_profile(source,0);
            if(d==source.rules->activities.end()||!h)throw std::runtime_error("Village example source missing");
            SteamVillageActivitySkinInput in;
            in.raw=inspect_page=="village-event-info"?52:inspect_page=="village-event-animation"?53:54;
            in.counter=in.raw==53?120:40;in.points=d->parameters[4];in.quarter_remaining=3;
            in.activity=SteamVillageActivityDefinition{d->identity,d->parameters[0],d->parameters[1],d->parameters[2],d->parameters[3],
                d->parameters[4],d->parameters[5],d->name,d->detail,d->description,d->flags,false};
            const auto &growth=source.scene.world.world.ai.growth.at(0);
            SteamVillageActivityHuman human{0,growth.definition.current_profession,h->sex,0,0,h->name};
            if(in.raw==54) {
                const auto effect=ref::prepare_world_village_human_effect(
                    {d->identity,1,d->flags,0,d->parameters[2],d->parameters[3],d->parameters[4],d->parameters[5]},
                    growth.definition,source.scene.world.world.ai.professions,growth.derived,source.shop_humans.at(0).satisfaction);
                if(!effect.candidate)throw std::runtime_error("Village example actual effect refused");
                human.current=effect.candidate->satisfaction;human.previous=effect.candidate->previous_value;
                in.humans.push_back(human);
            }
            in.display_humans={human,human};in.options.view_y=23;
            const std::string title=in.raw==54?"活动结束":d->name;
            in.options.title_widths=std::array<int,2>{int(font.measure(title)),int(font.measure(title))};
            in.options.description_width=int(font.measure(d->description));
            in.options.button_widths={int(font.measure("开展")),int(font.measure("不开展"))};
            const auto result=steam_village_activity_skin(in);
            if(!result)throw std::runtime_error("Village example plan refused");plan=*result;
        } else {
            auto options=window_town_options(source,inspect_page=="town-rank-celebration"?50:inspect_page=="town-activity-reward"?95:48,font);
            std::optional<SteamTownProgressPlan> result;
            if(inspect_page=="town-rank-conditions") {
                if(!refresh_startup_world_runtime_rank(source))throw std::runtime_error("Town example criteria refused");
                SteamTownConditionsInput in;in.raw_page=48;in.rank=0;in.village_name=source.scripts.village_name;in.options=options;
                const auto terms=ref::fixed_calendar_task_rank_terms().at(0);
                for(int row=0;row<4;++row) {
                    if(source.rank_values[row]<std::numeric_limits<int>::min()||source.rank_values[row]>std::numeric_limits<int>::max())throw std::runtime_error("Town example criterion overflow");
                    in.rows[row]={terms[row].type,int(source.rank_values[row]),terms[row].threshold,source.rank_met[row],{}};
                    if(terms[row].type==3) {
                        const auto facility=std::find_if(source.rules->facilities.begin(),source.rules->facilities.end(),[&](const auto &d){return d.id==terms[row].threshold;});
                        if(facility==source.rules->facilities.end())throw std::runtime_error("Town criterion definition missing");
                        in.rows[row].facility_name=facility->name;
                    }
                }
                result=steam_town_conditions_skin(in);
            } else if(inspect_page=="town-rank-celebration") {
                SteamTownCelebrationInput in;in.rank=1;in.phase=2;in.frame=90;in.frame2=17;in.options=options;
                auto cast=ref::prepare_world_rank_celebration({0},source.scene.random);
                const auto profile=startup_world_human_profile(source,0);
                if(!cast||!profile)throw std::runtime_error("Town example cast refused");
                for(const auto &actor:cast->participants)in.participants.push_back({actor[0],source.scene.world.world.ai.growth.at(0).definition.current_profession,
                    profile->sex,{actor[1],actor[2],actor[3],actor[4]}});
                result=steam_town_celebration_skin(in);
            } else {
                const auto a=std::find_if(source.rules->activities.begin(),source.rules->activities.end(),[](const auto &activity){return activity.identity==30;});
                if(a==source.rules->activities.end())throw std::runtime_error("Town reward definition missing");
                SteamTownGiftInput in;in.kind=11;in.value=30;in.frame=40;in.name=a->name;in.activity_icon=a->parameters[0];in.activity_flags=a->flags;in.options=options;
                result=steam_town_gift_skin(in);
            }
            if(!result)throw std::runtime_error("Town example plan refused");plan=*result;
        }
        int count{};
        while(!WindowShouldClose()&&(frames==0||count<frames)) {
            BeginTextureMode(canvas.value);ClearBackground(Color{83,141,77,255});
            font.text("研究绘制夹具",8,10,WHITE,10);
            std::visit([&](const auto &value){draw_facility_ceremony(value,sprites,font);},plan);
            font.text("无自然触发认证",8,260,WHITE,9);
            EndTextureMode();BeginDrawing();ClearBackground(BLACK);
            DrawTexturePro(canvas.value.texture,{0,0,width,-height},{0,0,width*scale,height*scale},{0,0},0,WHITE);
            EndDrawing();++count;
            if(count==frames&&screenshot) {
                if(!screenshot->parent_path().empty())std::filesystem::create_directories(screenshot->parent_path());
                auto pixels=LoadImageFromScreen();
                if(!pixels.data)throw std::runtime_error("Management example screenshot missing");
                const bool saved=ExportImage(pixels,screenshot->string().c_str());UnloadImage(pixels);
                if(!saved)throw std::runtime_error("Management example screenshot refused");
            }
        }
        std::cout<<"management drawing fixture closed: page="<<inspect_page<<" frames="<<count<<" world_updates=0 natural_trigger_verified=false\n";
        return 0;
    }
    if(reputation_fixture) {
        // 独立纯绘制条件载荷，不创建/安装世界Owner，也不伪造原自然36个月与五次使用前缀。
        const auto &rules=startup_world_rules();
        const auto facility=std::find_if(rules.facilities.begin(),rules.facilities.end(),[](const auto &d){return d.id==33;});
        const auto human=std::find_if(rules.humans.begin(),rules.humans.end(),[](const auto &d){return d.identity==0;});
        if(facility==rules.facilities.end()||human==rules.humans.end()||human->sex<0||human->sex>1||
           human->definition.current_profession<0||human->definition.current_profession>=static_cast<int>(rules.jobs.size()))
            throw std::runtime_error("口碑绘制夹具缺固定定义");
        SteamFacilityReputationInput input;
        input.definition=facility->id;input.mapchip=facility->display_id;input.name=facility->name;
        input.phase=inspect_page=="facility-reputation-result"?1:0;input.frame=40;input.frame2=16;input.view_y=23;
        input.participants.push_back({human->identity,rules.jobs[human->definition.current_profession].sprites[human->sex]});
        input.title_widths=std::array<int,2>{int(font.measure("传说中的商店")),int(font.measure("传说中的商店"))};
        input.increase_suffix_width=int(font.measure("上升!"));
        const auto plan=steam_facility_reputation_skin(input);
        if(!plan)throw std::runtime_error("口碑纯绘制夹具拒绝");
        int count=0;
        while(!WindowShouldClose()&&(frames==0||count<frames)) {
            BeginTextureMode(canvas.value);ClearBackground(Color{83,141,77,255});
            font.text("口碑 / 绘制夹具",8,10,WHITE,10);
            draw_facility_ceremony(*plan,sprites,font);
            font.text("条件载荷 / 无自然触发认证",8,260,WHITE,9);
            EndTextureMode();BeginDrawing();ClearBackground(BLACK);
            DrawTexturePro(canvas.value.texture,{0,0,width,-height},{0,0,width*scale,height*scale},{0,0},0,WHITE);
            EndDrawing();++count;
            if(count==frames&&screenshot) {
                if(!screenshot->parent_path().empty())std::filesystem::create_directories(screenshot->parent_path());
                auto image=LoadImageFromScreen();
                if(!image.data)throw std::runtime_error("口碑夹具像素读取失败");
                const bool saved=ExportImage(image,screenshot->string().c_str());UnloadImage(image);
                if(!saved)throw std::runtime_error("口碑夹具截图导出失败");
            }
        }
        std::cout<<"facility reputation drawing fixture closed: frames="<<count<<" phase="<<input.phase
                 <<" world_updates=0 natural_trigger_verified=false\n";
        return 0;
    }
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
                             const std::optional<std::filesystem::path> &save_file,
                             const std::optional<StartupWindowApplicationOptions> &application) {
    const bool manual_inspection=inspect_page=="world-manual" || inspect_page=="world-manual-about";
    if(application && (load_file || save_file ||
       (!inspect_page.empty() && inspect_page!="world-save" && !manual_inspection) || application->slot<0 || application->slot>1))
        throw std::invalid_argument("应用窗口参数组合非法");
    Window window;
    Canvas canvas;
    SourceSprites sprites(assets / "original");
    const auto &rules = startup_world_rules();
    std::string glyphs = rules.script_sources.talks + rules.script_sources.news +
                         rules.script_sources.event_messages;
    for(const auto &body:rules.manual_pages)glyphs+=body;
    glyphs+="游戏方法关于本游戏冒险迷宫村体验版开罗软件返回";
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
    glyphs += "选择调和书放入道具查看使用魔法点数生成获得作成物持有数研究报告素材消费好好煮一煮可以制作了要制作吗不足了解知识？？？？？";
    glyphs += "菜单冒险情报系统任务进度中止任务赠送礼物保存设置变更游戏方法排行榜结束游戏输入未接入"
              "研究适配页签设施列表持有物品装备一览村情报";
    glyphs += "村办季度剩余次数村子点开展活动完成等待尚未接入获得奖励金币配置更替确认领取设备一般";
    glyphs += "保存中保存完成保存失败本窗口仅预览保存页消息了解";
    glyphs += "通关点数发表那么赶紧开始计算吧追加排行榜更新接下来就是这样当前最高分…！";
    glyphs += "体力力量灵活结实魔力运气努力奖励基础能力→";
    glyphs += "周围设施的奖励没有";
    glyphs += "出售中查看商品会给周围带来好的影响";
    glyphs += "传说中的商店冒险者们中间的话题传了开来街道人气上升";
    glyphs += "南瓜商会购买出售持有剩余价格道具使用强化反应赠送多谢惠顾免费建设返回";
    glyphs += "道路移动撤除旋转请选择起点终点未开放不可操作街道内地域商品种类装饰信息口碑关闭继续";
    glyphs += "任务选择寻求冒险者请选择任务募集交给我多指教来了我会加油的我要参加好期待查看位置！胜利因为完成任务街道的人气上升了";
    glyphs += "任务列表征集队伍征集费出发追加取消候选队伍评价休息成果商店追加"
              "年度贡献勋章授予终止是非满足努力能力上升自宅完成设施升级城镇等级晋级申请"
              "月收入设施数居住数指定建设任务完成数街道人气举办活动达成未暂不可用"
              "概况属性装备魔法体力力量灵活结实魔力运气攻击防御经验职业大师转职"
              "武器防具饰品装备礼物居民税收合计库存可用学会火冰雷恢复营业施工使用支出加成"
              "通关计分研究适配累计原最高阶段街道人气任务完成数设施发现数冒险者全员LV合计自宅数努力度总和";
    // 两种宿主互斥；应用模式只读其唯一世界，不复制Session以绕过文件事务。
    auto session=[&]() {
        if(application) {
            StartupApplication app({application->root},ref::WorldRandomStream::from_java_seed(1));
            if(!app.error().empty())throw std::runtime_error(app.error());
            std::string error;
            if(application->load)error=app.load_world(application->slot);
            else {
                for(const auto &entry:app.records().save_directory[application->slot])
                    if(entry.reference || entry.packed_date!=-1 || !entry.village.empty() || entry.cash!=0)
                        throw std::runtime_error("应用新局仅允许空栏，隐藏记录仍保留，不能覆盖");
                error=app.request_new_game(application->slot);
                if(error.empty())error=app.start_game();
            }
            if(!error.empty())throw std::runtime_error(error);
            return StartupWindowHost(std::move(app));
        }
        StartupSession initial;
        if(inspect_page=="visitor")for(int n=0;n<420;++n)initial.update();
        return StartupWindowHost(StartupWorldRuntimeSession(initial.state(),ref::WorldRandomStream::from_java_seed(1)));
    }();
    // 平台当前只作静默音频sink；初始化B0/G也在首次保存请求前恰一次消费。
    (void)session.take_audio_requests();
    StartupWorldSaveMetadata file_metadata;
    if (load_file) {
        auto loaded = load_startup_world_file(*load_file, rules, StartupWorldSavePurpose::normal);
        if (!loaded.snapshot) throw std::runtime_error("读取失败：" + loaded.error);
        file_metadata = std::move(loaded.snapshot->metadata);
        session = StartupWindowHost(std::move(loaded.snapshot->session));
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
    if (inspect_page == "world-menu" || inspect_page == "world-system" || inspect_page == "world-information" ||
        inspect_page == "world-adventure" || inspect_page == "world-village" || inspect_page == "world-save" || manual_inspection) {
        // 有界验收只使用真实入口与输入，不注入选择、页计数、资金或随机。
        if (session.open_main_menu()!=StartupWorldRuntimeError::none)
            throw std::runtime_error("导航检查主菜单入口失败");
        for(int step=0;step<3;++step)
            if(!session.update().committed)throw std::runtime_error("导航检查主菜单初始化失败");
        if(inspect_page!="world-menu") {
            const auto id=session.state().scripts.pages.back().id;
            const auto view=inspect_startup_world_menu_page(session.state(),id);
            if(!view)throw std::runtime_error("导航检查主菜单投影失败");
            const int tag=(inspect_page=="world-system"||inspect_page=="world-save"||manual_inspection)?6:inspect_page=="world-information"?5:
                          inspect_page=="world-adventure"?1:2;
            const auto entry=std::find(view->tags.begin(),view->tags.end(),tag);
            if(entry==view->tags.end())throw std::runtime_error("导航检查所需标签不存在");
            StartupWorldMenuInput selection;selection.select_row=static_cast<int>(entry-view->tags.begin());
            StartupWorldMenuInput confirm;confirm.confirm=true;
            if(session.input_menu_page(id,selection)!=StartupWorldRuntimeError::none ||
               session.input_menu_page(id,confirm)!=StartupWorldRuntimeError::none)
                throw std::runtime_error("导航检查实际选行确认失败");
            for(int step=0;step<3;++step)
                if(!session.update().committed)throw std::runtime_error("导航检查子菜单初始化失败");
            if(inspect_page=="world-save") {
                // Session停在请求；应用宿主消费声音后通过已验事务真正写当前栏。
                const auto save_menu=session.state().scripts.pages.back().id;
                if(session.input_menu_page(save_menu,confirm)!=StartupWorldRuntimeError::none ||
                   !session.update().committed)
                    throw std::runtime_error("保存页真实请求检查失败");
            }
            if(manual_inspection) {
                const auto system=session.state().scripts.pages.back().id;
                const auto menu=inspect_startup_world_menu_page(session.state(),system);
                if(!menu)throw std::runtime_error("说明诊断缺系统菜单");
                const auto entry=std::find(menu->tags.begin(),menu->tags.end(),22);
                if(entry==menu->tags.end())throw std::runtime_error("说明诊断缺真实条目");
                StartupWorldMenuInput select;select.select_row=static_cast<int>(entry-menu->tags.begin());
                if(session.input_menu_page(system,select)!=StartupWorldRuntimeError::none ||
                   session.input_menu_page(system,confirm)!=StartupWorldRuntimeError::none)
                    throw std::runtime_error("说明诊断菜单入口拒绝");
                for(int step=0;step<3;++step)
                    if(!session.update().committed)throw std::runtime_error("说明诊断初始化拒绝");
                if(inspect_page=="world-manual-about") {
                    StartupManualInput input;input.left=true;
                    if(session.input_manual_page(session.state().scripts.pages.back().id,input)!=StartupWorldRuntimeError::none)
                        throw std::runtime_error("说明诊断末页翻页拒绝");
                }
            }
        }
        session.take_audio_requests();
        if(inspect_page=="world-save" && session.application_mode()) {
            if(!session.update().committed)throw std::runtime_error("应用保存检查更新失败");
            const auto page=session.state().scripts.pages.back().id;
            const auto saved=inspect_startup_world_save_page(session.state(),page);
            if(!saved || saved->stage!=2 || saved->saved!=true)
                throw std::runtime_error("应用保存检查没有成功结果");
        }
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
            if (!update.committed) {
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
            if (!session.update().committed)
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
    } else if(inspect_page=="world-facility-items"||inspect_page=="world-facility-improvement"||inspect_page=="world-facility-result") {
        std::optional<std::uint64_t> target;
        for(const auto id:session.state().scene.world.facility_order) {
            const auto &f=session.state().scene.world.world.facilities.at(id);
            if(f.placement.definition_id==33&&f.status!=0){target=id;break;}
        }
        if(!target||session.open_facility_page(*target)!=StartupWorldRuntimeError::none)
            throw std::runtime_error("初装设施改良入口缺失");
        const auto parent=session.state().scripts.pages.back().id;
        if(session.act_facility_page(parent,StartupFacilityPageAction::confirm)!=StartupWorldRuntimeError::none)
            throw std::runtime_error("初装设施道具入口拒绝");
        const auto directory=session.state().scripts.pages.back().id;
        if(!session.update().committed||!session.state().facility_item_pages_initialized.count(directory))
            throw std::runtime_error("道具目录初始化拒绝");
        if(inspect_page!="world-facility-items") {
            if(session.act_facility_item_page(directory,StartupFacilityItemAction::confirm,-1)!=StartupWorldRuntimeError::none)
                throw std::runtime_error("初始道具实际消费拒绝");
            const int raw=inspect_page=="world-facility-improvement"?76:77;
            const int count=raw==76?25:49;
            bool reached=false;
            for(int step=0;step<120;++step) {
                if(!session.update().committed)throw std::runtime_error("改良演出推进拒绝");
                session.take_audio_requests();
                const auto &p=session.state().scripts.pages.back();
                if(p.legacy_page==raw&&session.state().page_counters.at(p.id)>=count&&
                   session.state().facility_item_pages_initialized.count(p.id)){reached=true;break;}
            }
            if(!reached)throw std::runtime_error("改良有界示例未达到指定阶段");
        }
    } else if (inspect_page == "world-facility-bonuses"||inspect_page=="world-facility-details") {
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
        if ((inspect_page=="world-facility-bonuses"&&session.act_facility_page(page, StartupFacilityPageAction::next) !=
                StartupWorldRuntimeError::none) ||
            !startup_world_facility_bonus_rows(session.state(), page))
            throw std::runtime_error("真实设施详情载荷拒绝");
    } else if (inspect_page == "world-building") {
        if (session.open_build_menu() != StartupWorldRuntimeError::none)
            throw std::runtime_error("真实新局建设目录检查入口失败");
    } else if (inspect_page == "world-goods" || inspect_page == "world-goods-attributes" || inspect_page == "world-equipment-info" || inspect_page == "world-shop-details") {
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
                if (!session.update().committed) throw std::runtime_error("自然商品检查施工推进失败");
                const auto p = session.state().scripts.pages.back();
                if (p.kind != ref::WorldScriptPageKind::scene &&
                    session.acknowledge_page(p.id) != StartupWorldRuntimeError::none)
                    throw std::runtime_error("自然商品检查前置页面确认失败");
                (void)session.take_sound_requests();
            }
        }
        if (session.open_facility_page(*shop) != StartupWorldRuntimeError::none)
            throw std::runtime_error("真实武器店74打开失败");
        if(inspect_page!="world-shop-details") {
            const auto parent = session.state().scripts.pages.back().id;
            if (session.act_facility_page(parent, StartupFacilityPageAction::confirm) != StartupWorldRuntimeError::none)
                throw std::runtime_error("真实武器店74商品选择失败");
            const auto goods = session.state().scripts.pages.back().id;
            if (!inspect_startup_world_facility_catalog_page(session.state(), goods))
                throw std::runtime_error("自然79检查未形成合法目录");
            if(inspect_page=="world-goods-attributes"&&session.act_facility_catalog_page(goods,StartupFacilityCatalogAction::next_tab)!=StartupWorldRuntimeError::none)
                throw std::runtime_error("商品属性页检查失败");
            if (inspect_page == "world-equipment-info") {
                if (session.act_facility_catalog_page(goods, StartupFacilityCatalogAction::inspect) != StartupWorldRuntimeError::none ||
                    !session.update().committed)
                    throw std::runtime_error("自然79信息72检查失败");
            }
        }
    } else if (inspect_page == "world-details") {
        if (session.open_facility_page(4) != StartupWorldRuntimeError::none)
            throw std::runtime_error("真实初局旅店详情检查入口失败");
    } else if (inspect_page == "world-month" || inspect_page == "world-active" ||
               inspect_page == "task-team" || inspect_page == "world-award" ||
               inspect_page == "world-human" || inspect_page == "world-human-directory" || inspect_page == "world-human-attributes" ||
               inspect_page == "world-human-equipment" || inspect_page == "world-human-magic" || inspect_page == "world-human-professions" || inspect_page == "world-human-gifts" || inspect_page == "world-human-items" || inspect_page == "world-human-equipment-result" || inspect_page == "world-human-item-result" || inspect_page == "world-human-equipment-info" ||
               inspect_page == "world-human-gift-confirm"||inspect_page=="world-human-gift-opening"||inspect_page=="world-human-gift-result"||
               inspect_page=="world-human-profession-preview"||inspect_page=="world-human-profession-result") {
        bool reached{};
        // 显式窗口检查策略：只给真实页栈逐轮确认，不改人物/日期/随机/资金。
        for (int step = 0; step < (inspect_page == "world-award" ? 50000 : 20000); ++step) {
            const auto result = session.update();
            if (!result.committed)
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
            if ((inspect_page == "world-human" || inspect_page == "world-human-directory" || inspect_page == "world-human-attributes" ||
                 inspect_page == "world-human-equipment" || inspect_page == "world-human-magic" || inspect_page == "world-human-professions" || inspect_page == "world-human-gifts" || inspect_page == "world-human-items" || inspect_page == "world-human-equipment-result" || inspect_page == "world-human-item-result" || inspect_page == "world-human-equipment-info" ||
                 inspect_page == "world-human-gift-confirm"||inspect_page=="world-human-gift-opening"||inspect_page=="world-human-gift-result"||
                 inspect_page=="world-human-profession-preview"||inspect_page=="world-human-profession-result") && top.kind == ref::WorldScriptPageKind::scene &&
                top.lifecycle != 4 && current.scene.scene_state == 0 &&
                !current.scene.world.world.ai.human_order.empty()) {
                const int human = current.scene.world.world.ai.battle.actors
                                      .at(current.scene.world.world.ai.human_order.front())
                                      .definition;
                if (inspect_page == "world-human-directory") {
                    // 自然首访后实际9→35→60；只输入原目录，不注入W或页面载荷。
                    if(session.open_information_menu()!=StartupWorldRuntimeError::none)
                        throw std::runtime_error("自然人物情报入口失败");
                    for(int n=0;n<3;++n)if(!session.update().committed)
                        throw std::runtime_error("自然人物情报初始化失败");
                    StartupInformationInput confirm;confirm.confirm=true;
                    if(session.input_information_page(session.state().scripts.pages.back().id,confirm)!=StartupWorldRuntimeError::none)
                        throw std::runtime_error("自然人物目录入口失败");
                    for(int n=0;n<3;++n)if(!session.update().committed)
                        throw std::runtime_error("自然人物目录初始化失败");
                    if(session.input_information_page(session.state().scripts.pages.back().id,confirm)!=StartupWorldRuntimeError::none)
                        throw std::runtime_error("自然人物目录详情确认失败");
                } else if (session.open_human_page(human) != StartupWorldRuntimeError::none)
                    throw std::runtime_error("自然人物详情检查入口失败");
                for (int n = 0; n < 100; ++n) {
                    if (!session.update().committed)
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
                if(inspect_page=="world-human-directory") {
                    const auto detail=session.state().scripts.pages.back().id;
                    const auto view=inspect_startup_world_human_presentation(session.state(),detail);
                    if(!view||!view->tracking_available||session.act_human_page(detail,StartupHumanPageAction::cancel)!=StartupWorldRuntimeError::none)
                        throw std::runtime_error("目录详情追踪来源或返回拒绝");
                    for(int n=0;n<3;++n)if(!session.update().committed)
                        throw std::runtime_error("人物详情返回退休失败");
                    const auto parent=session.state().scripts.pages.back().id;
                    const auto directory=inspect_startup_world_information_page(session.state(),parent);
                    if(!directory||directory->raw!=35||directory->selection!=0||directory->first_visible!=0||
                       session.state().human_detail_contexts.count(detail))
                        throw std::runtime_error("人物目录返回选择或引用退休失败");
                    StartupInformationInput confirm;confirm.confirm=true;
                    if(session.input_information_page(parent,confirm)!=StartupWorldRuntimeError::none)
                        throw std::runtime_error("人物目录重进详情拒绝");
                    for(int n=0;n<3;++n)if(!session.update().committed)
                        throw std::runtime_error("人物目录重进初始化失败");
                    if(!startup_world_human_page_ready(session.state(),session.state().scripts.pages.back().id))
                        throw std::runtime_error("人物目录重进未就绪");
                }
                if(inspect_page=="world-human-professions"||inspect_page=="world-human-attributes"||inspect_page=="world-human-profession-preview"||inspect_page=="world-human-profession-result") {
                    const auto parent=session.state().scripts.pages.back().id;
                    if(session.act_human_page(parent,StartupHumanPageAction::next)!=StartupWorldRuntimeError::none||
                       session.act_human_page(parent,StartupHumanPageAction::professions)!=StartupWorldRuntimeError::none)
                        throw std::runtime_error("人物六属性转职入口拒绝");
                    bool catalogue_ready{};
                    for(int n=0;n<100;++n) {
                        if(!session.update().committed)throw std::runtime_error("人物首次转职说明推进失败");
                        const auto current_page=session.state().scripts.pages.back();
                        if(current_page.legacy_page==61&&startup_world_human_page_ready(session.state(),current_page.id)) {
                            const auto child=current_page.id;
                            if(inspect_page=="world-human-professions") {catalogue_ready=true;break;}
                            if(inspect_page!="world-human-attributes") {
                                const auto h=startup_world_human_details(session.state(),human);
                                if(!h)throw std::runtime_error("职业诊断人物载荷拒绝");
                                const auto &list=session.state().human_page_catalogs.at(child);
                                const auto target=std::find_if(list.begin(),list.end(),[&](int j){
                                    const auto &job=session.state().rules->jobs.at(j);
                                    return j!=h->profession&&job.change_points<=session.state().village_points&&
                                        (inspect_page=="world-human-profession-preview"||job.required_medals<=h->medals);
                                });
                                if(target==list.end())throw std::runtime_error("真实首访尚无符合资格的其他职业，不注入点数/勋章");
                                const int selection=static_cast<int>(target-list.begin());
                                if(session.act_human_page(child,StartupHumanPageAction::select,selection)!=StartupWorldRuntimeError::none||
                                   session.act_human_page(child,StartupHumanPageAction::confirm)!=StartupWorldRuntimeError::none)
                                    throw std::runtime_error("真实职业预览打开失败");
                                for(int k=0;k<3;++k)if(!session.update().committed)throw std::runtime_error("职业预览初始化失败");
                                if(inspect_page=="world-human-profession-result") {
                                    const auto preview=session.state().scripts.pages.back().id;
                                    if(session.act_human_page(preview,StartupHumanPageAction::confirm)!=StartupWorldRuntimeError::none)
                                        throw std::runtime_error("真实职业确认失败");
                                    bool animation_ready{};
                                    for(int k=0;k<300;++k) {
                                        if(!session.update().committed)throw std::runtime_error("职业演出推进失败");
                                        const auto active=session.state().scripts.pages.back();
                                        if(active.legacy_page==63&&startup_world_human_page_ready(session.state(),active.id)&&
                                           session.state().page_counters.at(active.id)>=197){animation_ready=true;break;}
                                        if(active.legacy_page!=63&&active.kind!=ref::WorldScriptPageKind::scene&&
                                           session.acknowledge_page(active.id)!=StartupWorldRuntimeError::none)
                                            throw std::runtime_error("职业奖励覆盖页确认失败");
                                    }
                                    if(!animation_ready)throw std::runtime_error("职业演出未有界到达197");
                                }
                                catalogue_ready=true;break;
                            }
                            if(session.act_human_page(child,StartupHumanPageAction::cancel)!=StartupWorldRuntimeError::none)
                                throw std::runtime_error("人物职业目录取消拒绝");
                            for(int k=0;k<3;++k)if(!session.update().committed)
                                throw std::runtime_error("人物职业目录返回推进失败");
                            const auto view=inspect_startup_world_human_presentation(session.state(),parent);
                            if(!view||view->tab!=1||session.state().scripts.pages.back().id!=parent||
                               session.state().page_human_bindings.count(child)||session.state().human_page_catalogs.count(child))
                                throw std::runtime_error("六属性父页恢复或职业引用退休失败");
                            catalogue_ready=true;break;
                        }
                        if(current_page.kind==ref::WorldScriptPageKind::dialogue&&
                           session.acknowledge_page(current_page.id)!=StartupWorldRuntimeError::none)
                            throw std::runtime_error("人物首次转职说明确认失败");
                    }
                    if(!catalogue_ready)throw std::runtime_error("人物六属性有界诊断未返回父页");
                }
                if(inspect_page=="world-human-equipment-result"||inspect_page=="world-human-item-result"||inspect_page=="world-human-gifts"||inspect_page=="world-human-items"||inspect_page=="world-human-equipment"||inspect_page=="world-human-magic"||inspect_page=="world-human-equipment-info"||
                   inspect_page=="world-human-gift-confirm"||inspect_page=="world-human-gift-opening"||inspect_page=="world-human-gift-result") {
                    const auto detail=session.state().scripts.pages.back().id;
                    for(int n=0;n<2;++n)if(session.act_human_page(detail,StartupHumanPageAction::next)!=StartupWorldRuntimeError::none)
                        throw std::runtime_error("装备页真实导航拒绝");
                    if(inspect_page!="world-human-magic") {
                        if(session.act_human_page(detail,StartupHumanPageAction::select,inspect_page=="world-human-equipment"?2:0)!=StartupWorldRuntimeError::none||
                           session.act_human_page(detail,StartupHumanPageAction::gifts)!=StartupWorldRuntimeError::none)
                            throw std::runtime_error("装备槽2赠礼入口拒绝");
                        const auto child=session.state().scripts.pages.back().id;
                        bool ready{};
                        for(int n=0;n<100;++n) {
                            if(!session.update().committed)throw std::runtime_error("槽2赠礼初始化失败");
                            if(startup_world_human_page_ready(session.state(),child)){ready=true;break;}
                            const auto top=session.state().scripts.pages.back();
                            if(top.kind==ref::WorldScriptPageKind::dialogue&&
                               session.acknowledge_page(top.id)!=StartupWorldRuntimeError::none)
                                throw std::runtime_error("首次赠礼说明确认失败");
                        }
                        if(!ready)throw std::runtime_error("槽2赠礼初始化未有界完成");
                        if(inspect_page=="world-human-item-result") {
                            if(session.act_human_page(child,StartupHumanPageAction::equipment_slot,4)!=StartupWorldRuntimeError::none)
                                throw std::runtime_error("item result tab refused");
                            const auto &items=session.state().equipment_page_catalogs.at(child)[4];
                            const auto chosen=std::find_if(items.begin(),items.end(),[&](int item){
                                const auto d=std::find_if(rules.items.begin(),rules.items.end(),[=](const auto &v){return v.identity==item;});
                                return d!=rules.items.end()&&d->effect>=0&&d->effect<6;
                            });
                            if(chosen==items.end())throw std::runtime_error("no actual owned attribute gift");
                            const int selection=int(chosen-items.begin());
                            if(session.act_human_page(child,StartupHumanPageAction::select,selection)!=StartupWorldRuntimeError::none||
                               session.act_human_page(child,StartupHumanPageAction::confirm)!=StartupWorldRuntimeError::none)
                                throw std::runtime_error("actual item gift refused");
                            bool result_ready{};
                            for(int k=0;k<300;++k) {
                                if(!session.update().committed)throw std::runtime_error("item result update refused");
                                const auto active=session.state().scripts.pages.back();
                                if(active.legacy_page==69&&startup_world_human_page_ready(session.state(),active.id)&&
                                   session.state().page_counters.at(active.id)>=45){result_ready=true;break;}
                                if(active.legacy_page!=69&&active.kind!=ref::WorldScriptPageKind::scene&&
                                   session.acknowledge_page(active.id)!=StartupWorldRuntimeError::none)
                                    throw std::runtime_error("item result covering page refused");
                            }
                            if(!result_ready)throw std::runtime_error("bounded item result not reached");
                            reached=true;break;
                        }
                        if(inspect_page=="world-human-gifts"||inspect_page=="world-human-items") {
                            if(inspect_page=="world-human-items"&&session.act_human_page(child,StartupHumanPageAction::equipment_slot,4)!=StartupWorldRuntimeError::none)
                                throw std::runtime_error("item catalogue tab refused");
                            reached=true;break;
                        }
                        if(inspect_page=="world-human-equipment-result"||inspect_page=="world-human-equipment-info"||inspect_page=="world-human-gift-confirm"||
                           inspect_page=="world-human-gift-opening"||inspect_page=="world-human-gift-result") {
                            if(session.act_human_page(child,inspect_page!="world-human-equipment-info"?StartupHumanPageAction::confirm:
                                StartupHumanPageAction::inspect_equipment)!=StartupWorldRuntimeError::none)
                                throw std::runtime_error("赠礼目录打开装备信息失败");
                            for(int n=0;n<3;++n)if(!session.update().committed)
                                throw std::runtime_error("装备信息初始化失败");
                            if(inspect_page=="world-human-equipment-result"||inspect_page=="world-human-gift-opening"||inspect_page=="world-human-gift-result") {
                                const auto question=session.state().scripts.pages.back().id;
                                if(session.act_human_page(question,StartupHumanPageAction::confirm)!=StartupWorldRuntimeError::none)
                                    throw std::runtime_error("真实赠礼65回答失败");
                                bool result_ready{};
                                const int result_raw=inspect_page=="world-human-equipment-result"?68:66;
                                const int required=result_raw==68?73:inspect_page=="world-human-gift-opening"?45:162;
                                for(int n=0;n<300;++n) {
                                    if(!session.update().committed)throw std::runtime_error("真实赠礼结果推进失败");
                                    const auto active=session.state().scripts.pages.back();
                                    if(active.legacy_page==result_raw&&startup_world_human_page_ready(session.state(),active.id)&&
                                       session.state().page_counters.at(active.id)>=required){result_ready=true;break;}
                                    if(active.legacy_page!=64&&active.legacy_page!=result_raw&&active.kind!=ref::WorldScriptPageKind::scene&&
                                       session.acknowledge_page(active.id)!=StartupWorldRuntimeError::none)
                                        throw std::runtime_error("赠礼奖励覆盖页确认失败");
                                }
                                if(!result_ready)throw std::runtime_error("赠礼结果未有界到达目标计数");
                            }
                            reached=true;break;
                        }
                        if(session.act_human_page(child,StartupHumanPageAction::cancel)!=StartupWorldRuntimeError::none)
                            throw std::runtime_error("槽2赠礼取消失败");
                        for(int n=0;n<3;++n)if(!session.update().committed)
                            throw std::runtime_error("槽2赠礼返回失败");
                        const auto view=inspect_startup_world_human_presentation(session.state(),detail);
                        if(!view||view->tab!=2||view->equipment_selection!=2||session.state().equipment_page_catalogs.count(child))
                            throw std::runtime_error("装备页父选择或子引用退休失败");
                    } else for(int slot=0;slot<4;++slot)
                        if(session.act_human_page(detail,StartupHumanPageAction::next)!=StartupWorldRuntimeError::none)
                            throw std::runtime_error("装备四槽到魔法页失败");
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
            if (!session.update().committed) throw std::runtime_error("魔法壶有界入口更新失败");
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
    if ((!application && !load_file) || paused)
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
    int residence_selection{}; // 80旧目录适配；任务25–33游标由Owner保存。
    int award_selection{}, prompt_selection{};
    int build_tab{}, build_selection{};
    int bonus_selection{}, bonus_scroll{}; // 原5行视窗的研究输入适配，不进入Owner或存档。
    std::map<std::uint64_t,SteamFacilityBonusCursor> bonus_views; // 父74覆盖后恢复，退休页及时回收。
    std::optional<std::pair<std::uint64_t,int>> facility_arrow_press; // 原CheckTouch的研究按下适配，非悬停。
    struct FacilityScrollDrag {std::uint64_t page;float start_y;int first;float grid;};
    std::optional<FacilityScrollDrag> facility_scroll_drag; // 只绑定当前页，释放/切页/退休清理。
    std::optional<FacilityScrollDrag> human_scroll_drag; // 只保存平台拖动手势，目录游标在唯一Owner。
    std::optional<std::pair<std::uint64_t,int>> human_catalogue_marker;
    std::optional<std::pair<std::uint64_t,int>> facility_item_marker; // raw75 marker16，同页同绝对行的第二次UP。
    std::optional<std::pair<std::uint64_t,int>> task_item_marker; // raw27同一行第二次UP，覆盖/暂停/失焦释放。
    std::optional<std::pair<std::uint64_t,int>> magic_item_marker; // raw42 marker16，跨页／拖动／暂停清除。
    std::optional<std::pair<std::uint64_t,int>> commerce_item_marker; // raw85只保存鼠标标记，真实选择归Owner。
    bool human_scroll_gesture{};
    bool facility_scroll_gesture{}; // 即使切页清引用，仍吞掉本次拖动的最终release。
    auto build_orientation = ref::FacilityOrientation::first;
    std::string command_feedback;
    std::optional<std::uint64_t> clear_confirm_pending; // 绑定计分页，按逻辑门槛只消费一次。
    while (!WindowShouldClose()) {
        const Vector2 mouse{GetMousePosition().x / scale, GetMousePosition().y / scale};
        const bool pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        if(pressed){facility_scroll_gesture=false;facility_scroll_drag.reset();human_scroll_gesture=false;human_scroll_drag.reset();}
        if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT))facility_arrow_press.reset();
        const auto current_top=std::find_if(session.state().scripts.pages.rbegin(),session.state().scripts.pages.rend(),
                                            [](const auto &page){return page.lifecycle!=4;});
        const bool application_clear=session.application_mode() && current_top!=session.state().scripts.pages.rend() &&
            current_top->kind==ref::WorldScriptPageKind::raw_page && current_top->legacy_page==17;
        if(human_scroll_drag&&(session.state().scene.framework_paused||
           current_top==session.state().scripts.pages.rend()||current_top->id!=human_scroll_drag->page||
           (current_top->legacy_page!=22&&current_top->legacy_page!=25&&current_top->legacy_page!=26&&current_top->legacy_page!=27&&current_top->legacy_page!=31&&current_top->legacy_page!=32&&current_top->legacy_page!=42&&current_top->legacy_page!=43&&current_top->legacy_page!=51&&current_top->legacy_page!=54&&current_top->legacy_page!=60&&current_top->legacy_page!=61&&current_top->legacy_page!=64&&current_top->legacy_page!=75&&current_top->legacy_page!=79&&current_top->legacy_page!=85)))human_scroll_drag.reset();
        if(task_item_marker&&(session.state().scene.framework_paused||current_top==session.state().scripts.pages.rend()||
           current_top->id!=task_item_marker->first||current_top->legacy_page!=27||!IsWindowFocused()))task_item_marker.reset();
        if(magic_item_marker&&(session.state().scene.framework_paused||current_top==session.state().scripts.pages.rend()||
           current_top->id!=magic_item_marker->first||current_top->legacy_page!=42||!IsWindowFocused()))magic_item_marker.reset();
        if(commerce_item_marker&&(session.state().scene.framework_paused||current_top==session.state().scripts.pages.rend()||
           current_top->id!=commerce_item_marker->first||current_top->legacy_page!=85||!IsWindowFocused()))commerce_item_marker.reset();
        if(human_catalogue_marker&&(current_top==session.state().scripts.pages.rend()||
           current_top->id!=human_catalogue_marker->first))human_catalogue_marker.reset();
        if(facility_item_marker&&(session.state().scene.framework_paused||current_top==session.state().scripts.pages.rend()||
           current_top->id!=facility_item_marker->first||current_top->legacy_page!=75))facility_item_marker.reset();
        bool pointer_consumed=facility_scroll_gesture||human_scroll_gesture;
        const auto hit = [&](Rectangle r) { return pressed && !pointer_consumed && CheckCollisionPointRec(mouse, r); };
        if (!application_clear && hit({4, 295, 44, 22})) {
            pointer_consumed=true;
            ++pause_inputs;
            session.set_paused(!session.state().scene.framework_paused);
        }
        if (!application_clear && hit({176, 295, 58, 22})) {
            // 全局研究控件先消费该click，不能同时落入raw14原有的大触摸区域。
            pointer_consumed=true;
            ++speed_inputs;
            session.set_speed(session.state().scene.speed_setting == 1 ? 0 : 1);
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            const auto delta = GetMouseDelta();
            view_offset.x -= delta.x / scale;
            view_offset.y += delta.y / scale;
        }
        const auto top_page = [&]() -> std::optional<ref::WorldScriptPage> {
            const auto &pages = session.state().scripts.pages;
            for (auto it = pages.rbegin(); it != pages.rend(); ++it)
                if (it->kind != ref::WorldScriptPageKind::scene && it->lifecycle != 4)
                    return *it; // 命令会安装新候选；不让页面指针跨命令失效。
            return {};
        };
        const bool started_with_modal=top_page().has_value();
        // 暂停时不派发页面输入，避免把合法的暂停拒绝误报成窗口消费者故障。
        if (const auto page = top_page(); page && !session.state().scene.framework_paused) {
            if (viewed_page != page->id) {
                viewed_page = page->id;
                paragraph_index = 0;
                body_scroll = body_extent = 0;
                residence_selection = 0;
                build_tab = build_selection = 0;
                const auto old_bonus=bonus_views.find(page->id);
                bonus_selection=old_bonus==bonus_views.end()?0:old_bonus->second.selection;
                bonus_scroll=old_bonus==bonus_views.end()?0:old_bonus->second.first;
                award_selection = prompt_selection = 0;
                if (page->legacy_page == 1 && session.state().task_abort_questions.count(page->id))
                    prompt_selection = 1; // 原主动中止询问默认“否”，不因新页重置变成“是”。
                command_feedback.clear();
            }
            const int raw = page->legacy_page;
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
                        else command_feedback=session.last_error().empty()?"输入未接入":session.last_error();
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
            } else if(raw==29) {
                const auto plan=steam_task_information_skin(session.state(),page->id,window_task_info_options(font,false));
                if(plan) {
                    bool confirm=IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_ESCAPE)||hit({176,265,58,24});
                    for(const auto &touch:management_touches(*plan,sprites))if(touch.component==4&&touch.rectangle&&!pointer_consumed&&IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                        const auto &r=*touch.rectangle;confirm|=CheckCollisionPointRec(mouse,{float(r[0]),float(r[1]),float(r[2]),float(r[3])});
                    }
                    if(confirm&&session.acknowledge_page(page->id)!=StartupWorldRuntimeError::none)throw std::runtime_error("Task info acknowledgement rejected");
                }
            } else if (raw>=22&&raw<=24) {
                const auto pid=page->id;
                const auto plan=window_task_selection_plan(session.state(),pid,font,human_scroll_drag&&human_scroll_drag->page==pid);
                if(plan&&raw!=24) {
                    StartupTaskPageInput input;
                    input.up=raw==22&&IsKeyPressed(KEY_UP);input.down=raw==22&&IsKeyPressed(KEY_DOWN);
                    input.left=raw==23&&IsKeyPressed(KEY_LEFT);input.right=raw==23&&IsKeyPressed(KEY_RIGHT);
                    input.confirm=IsKeyPressed(KEY_ENTER);
                    input.cancel=IsKeyPressed(KEY_ESCAPE)||hit({176,265,58,24});
                    input.track=raw==22&&(IsKeyPressed(KEY_T)||hit({8,265,82,24}));
                    const auto point=[&](std::array<int,4> r){return Rectangle{human_overview_origin.x+r[0],human_overview_origin.y+r[1],float(r[2]),float(r[3])};};
                    for(const auto &touch:management_touches(*plan,sprites))
                        if(touch.rectangle&&!input.cancel&&!input.track&&!pointer_consumed&&IsMouseButtonReleased(MOUSE_BUTTON_LEFT)&&CheckCollisionPointRec(mouse,point(*touch.rectangle))) {
                            if(touch.component==11) {input.selection=touch.value&0xffff;input.confirm=true;}
                            else if(touch.component==4)input.confirm=true;
                        }
                    if(raw==22) {
                        const int count=static_cast<int>(session.state().task_page_lists.at(pid).size());
                        const int first=session.state().page_secondary_counters.at(pid);
                        for(const auto &op:plan->operations)if(const auto *bar=std::get_if<SteamHumanCatalogueScroll>(&op)) {
                            const auto &r=bar->rectangles[1].rect;auto area=r;area[0]-=3;area[2]+=23;
                            if(count>5&&hit(point(area))) {
                                const float grid=std::max(float(111-(r[3]-1))/(count-5),1.F);
                                human_scroll_drag=FacilityScrollDrag{pid,mouse.y,first,grid};
                                human_scroll_gesture=true;pointer_consumed=true;
                            } else if(count>5&&bar->track_touch.rectangle&&hit(point(*bar->track_touch.rectangle))) {
                                input.scroll=StartupTaskPageInput::Scroll{false,mouse.y<human_overview_origin.y+r[1]?-5:5};pointer_consumed=true;
                            }
                        }
                        const float wheel=GetMouseWheelMove();
                        if(count>5&&wheel!=0)input.scroll=StartupTaskPageInput::Scroll{false,wheel>0?-1:1};
                        if(human_scroll_drag&&human_scroll_drag->page==pid) {
                            const double wanted=human_scroll_drag->first+std::trunc(double(mouse.y-human_scroll_drag->start_y)/human_scroll_drag->grid);
                            input.scroll=StartupTaskPageInput::Scroll{true,int(std::clamp(wanted,0.,double(std::max(count-5,0))))};
                        }
                        if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT))human_scroll_drag.reset();
                    }
                    if(input.up||input.down||input.left||input.right||input.confirm||input.cancel||input.track||input.selection||input.scroll)
                        if(session.input_task_page(pid,input).error!=StartupWorldRuntimeError::none)throw std::runtime_error("Task selection input transaction rejected");
                } else if(!plan&&page->lifecycle!=0)throw std::runtime_error("Task input source rejected");
            } else if (raw >= 41 && raw <= 47) {
                const auto view=inspect_startup_world_magic_pot_page(session.state(),page->id);
                if(view) {
                    const auto pid=page->id;
                    const auto plan=window_magic_pot_plan(session.state(),pid,font,human_scroll_drag&&human_scroll_drag->page==pid);
                    if(!plan)throw std::runtime_error("Magic pot input plan rejected");
                    StartupMagicPotInput input;
                    input.confirm=IsKeyPressed(KEY_ENTER);
                    input.cancel=raw!=45&&(IsKeyPressed(KEY_ESCAPE)||hit({176,265,58,24}));
                    input.up=(raw==42||raw==43)&&IsKeyPressed(KEY_UP);
                    input.down=(raw==42||raw==43)&&IsKeyPressed(KEY_DOWN);
                    input.left=(raw==41||raw==43)&&IsKeyPressed(KEY_LEFT);
                    input.right=(raw==41||raw==43)&&IsKeyPressed(KEY_RIGHT);
                    const auto point=[&](std::array<int,4> r){return Rectangle{human_overview_origin.x+r[0],human_overview_origin.y+r[1],float(r[2]),float(r[3])};};
                    for(const auto &touch:management_touches(*plan,sprites)) {
                        if(!touch.rectangle) {
                            if(touch.component==2&&touch.option==2&&!pointer_consumed&&IsMouseButtonReleased(MOUSE_BUTTON_LEFT))input.confirm=true;
                            continue;
                        }
                        if(!input.cancel&&!pointer_consumed&&IsMouseButtonReleased(MOUSE_BUTTON_LEFT)&&CheckCollisionPointRec(mouse,point(*touch.rectangle))) {
                            if(touch.component==3||touch.component==11) {
                                const int chosen=touch.value&0xffff;input.selection=chosen;
                                input.confirm|=touch.option==0||magic_item_marker==std::make_pair(pid,chosen);
                                if(raw==42)magic_item_marker=std::make_pair(pid,chosen);
                            } else if(touch.component==4)input.confirm=true;
                            else if(touch.component==1){input.left|=touch.value==16;input.right|=touch.value==18;}
                            else if(touch.component==23){input.up|=touch.value==17;input.down|=touch.value==19;}
                            else if(touch.component==2&&raw==42){input.up|=(*touch.rectangle)[1]<100;input.down|=(*touch.rectangle)[1]>=100;}
                        }
                    }
                    if(raw==42||raw==43) {
                        const int count=static_cast<int>(view->entries.size());
                        for(const auto &op:plan->operations)if(const auto *bar=std::get_if<SteamHumanCatalogueScroll>(&op)) {
                            const auto &r=bar->rectangles[1].rect;auto area=r;area[0]-=3;area[2]+=23;
                            if(count>5&&hit(point(area))) {
                                const float grid=std::max(float(111-(r[3]-1))/(count-5),1.F);
                                human_scroll_drag=FacilityScrollDrag{pid,mouse.y,view->first_visible,grid};
                                human_scroll_gesture=true;pointer_consumed=true;magic_item_marker.reset();
                            } else if(count>5&&bar->track_touch.rectangle&&hit(point(*bar->track_touch.rectangle))) {
                                input.scroll=StartupMagicPotInput::Scroll{false,mouse.y<human_overview_origin.y+r[1]?-5:5};
                                pointer_consumed=true;magic_item_marker.reset();
                            }
                        }
                        const float wheel=GetMouseWheelMove();
                        if(count>5&&wheel!=0){input.scroll=StartupMagicPotInput::Scroll{false,wheel>0?-1:1};magic_item_marker.reset();}
                        if(human_scroll_drag&&human_scroll_drag->page==pid) {
                            const double wanted=human_scroll_drag->first+std::trunc(double(mouse.y-human_scroll_drag->start_y)/human_scroll_drag->grid);
                            input.scroll=StartupMagicPotInput::Scroll{true,int(std::clamp(wanted,0.,double(std::max(count-5,0))))};
                        }
                        if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT))human_scroll_drag.reset();
                    }
                    if(input.up||input.down||input.left||input.right||input.cancel)magic_item_marker.reset();
                    if(input.up||input.down||input.left||input.right||input.confirm||input.cancel||input.selection||input.scroll)
                        if(session.input_magic_pot_page(pid,input)!=StartupWorldRuntimeError::none)throw std::runtime_error("Magic pot input transaction rejected");
                }
            } else if (raw == 72 || raw == 79 || raw == 82) {
                const auto view=inspect_startup_world_facility_catalog_page(session.state(),page->id);
                if(view) {
                    const auto pid=page->id;
                    StartupFacilityCatalogInput input;
                    input.up=raw==79&&IsKeyPressed(KEY_UP);input.down=raw==79&&IsKeyPressed(KEY_DOWN);
                    input.left=raw!=82&&IsKeyPressed(KEY_LEFT);input.right=raw!=82&&IsKeyPressed(KEY_RIGHT);
                    input.confirm=IsKeyPressed(KEY_ENTER)||hit({176,265,58,24});
                    input.cancel=IsKeyPressed(KEY_ESCAPE)||hit({8,265,58,24});
                    input.inspect=raw==79&&(IsKeyPressed(KEY_I)||hit({88,265,66,24}));
                    if(raw!=82) {
                        const auto plan=window_commodity_plan(session.state(),pid,font,human_scroll_drag&&human_scroll_drag->page==pid);
                        if(!plan)throw std::runtime_error("Invalid commodity input source");
                        for(const auto &op:plan->operations) {
                            if(const auto *arrow=std::get_if<SteamFacilityArrowTouch>(&op)) {
                                const auto part=facility_detail_arrow(*arrow,sprites);const auto &r=part.destination;
                                if(hit({float(r[0]),human_overview_origin.y+r[1],float(r[2]),float(r[3])})) {
                                    input.left|=arrow->value==16;input.right|=arrow->value==18;
                                }
                            } else if(const auto *row=std::get_if<SteamHumanEquipmentRow>(&op)) {
                                const auto &r=*row->touch.rectangle;
                                const Rectangle area{float(r[0]-row->margin[0]),human_overview_origin.y+r[1]-row->margin[2],
                                    float(r[2]+row->margin[0]+row->margin[1]),float(r[3]+row->margin[2]+row->margin[3])};
                                if(hit(area)) {
                                    input.selection=row->touch.value&0xffff;
                                    human_catalogue_marker=std::pair<std::uint64_t,int>{pid,*input.selection};
                                }
                                if(!pointer_consumed&&IsMouseButtonReleased(MOUSE_BUTTON_LEFT)&&human_catalogue_marker&&
                                   human_catalogue_marker->first==pid&&human_catalogue_marker->second==(row->touch.value&0xffff)&&
                                   CheckCollisionPointRec(mouse,area)) {input.selection=row->touch.value&0xffff;input.confirm=true;}
                            } else if(const auto *scroll=std::get_if<SteamHumanCatalogueScroll>(&op)) {
                                const auto &r=*scroll->touch.rectangle;
                                if(hit({float(r[0]-3),human_overview_origin.y+r[1],float(r[2]+23),float(r[3])})&&view->entries.size()>4) {
                                    const float grid=110.F/static_cast<float>(view->entries.size());
                                    human_scroll_drag=FacilityScrollDrag{pid,mouse.y,view->first_visible,grid};
                                    human_scroll_gesture=true;pointer_consumed=true;human_catalogue_marker.reset();
                                }
                            }
                        }
                        if(raw==79) {
                            const float wheel=GetMouseWheelMove();
                            if(wheel!=0)input.scroll=StartupFacilityCatalogInput::Scroll{false,-static_cast<int>(wheel)};
                            if(human_scroll_drag&&human_scroll_drag->page==pid) {
                                const double wanted=human_scroll_drag->first+std::trunc(double(mouse.y-human_scroll_drag->start_y)/human_scroll_drag->grid);
                                input.scroll=StartupFacilityCatalogInput::Scroll{true,static_cast<int>(std::clamp(wanted,0.0,double(std::max(0,int(view->entries.size())-4))))};
                                pointer_consumed=true;
                            }
                        }
                    }
                    if(input.up||input.down||input.left||input.right||input.confirm||input.cancel||input.inspect||input.selection||input.scroll)
                        if(session.input_facility_catalog_page(pid,input)!=StartupWorldRuntimeError::none)
                            throw std::runtime_error("Commodity input transaction failed");
                    if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT)){human_scroll_drag.reset();human_catalogue_marker.reset();}
                }
            } else if(raw==93||raw==94) {
                const auto plan=window_town_progress_plan(session.state(),page->id,font);
                if(plan) {
                    bool confirm=IsKeyPressed(KEY_ENTER);
                    for(const auto &touch:management_touches(*plan,sprites))
                        if(touch.component==2&&!pointer_consumed&&IsMouseButtonReleased(MOUSE_BUTTON_LEFT)&&
                           CheckCollisionPointRec(mouse,{0,0,float(width),float(height)}))confirm=true;
                    if(confirm) {
                        const auto error=raw==93?session.act_commerce_page(page->id,StartupCommerceAction::confirm):session.acknowledge_page(page->id);
                        if(error!=StartupWorldRuntimeError::none)throw std::runtime_error("Reward claim rejected");
                    }
                } else if(page->lifecycle!=0)throw std::runtime_error("Reward input source rejected");
            } else if(raw==83) {
                const auto plan=window_commerce_plan(session.state(),page->id,font);
                if(plan) {
                    StartupCommerceTopInput input;
                    input.up=IsKeyPressed(KEY_UP);input.down=IsKeyPressed(KEY_DOWN);
                    input.confirm=IsKeyPressed(KEY_ENTER);input.cancel=IsKeyPressed(KEY_ESCAPE)||hit({176,265,58,24});
                    for(const auto &op:plan->operations)if(const auto *touch=std::get_if<SteamStartupTouch>(&op)) {
                        if(touch->component!=11||!touch->rectangle)continue;
                        const auto &r=*touch->rectangle;
                        if(!pointer_consumed&&IsMouseButtonReleased(MOUSE_BUTTON_LEFT)&&
                           CheckCollisionPointRec(mouse,{human_overview_origin.x+r[0],human_overview_origin.y+r[1],float(r[2]),float(r[3])})) {
                            input.selection=touch->value&0xffff;input.confirm=true;
                        }
                    }
                    if(input.up||input.down||input.confirm||input.cancel||input.selection)
                        if(session.input_commerce_top(page->id,input)!=StartupWorldRuntimeError::none)
                            throw std::runtime_error("Commerce top input transaction rejected");
                } else if(page->lifecycle!=0)throw std::runtime_error("Commerce top input source rejected");
            } else if(raw==85) {
                const auto pid=page->id;
                const auto view=inspect_startup_world_commerce_page(session.state(),pid);
                const auto plan=window_commerce_facility_plan(session.state(),pid,font,human_scroll_drag&&human_scroll_drag->page==pid);
                if(view&&plan) {
                    StartupCommerceFacilityInput input;
                    input.up=IsKeyPressed(KEY_UP);input.down=IsKeyPressed(KEY_DOWN);
                    input.confirm=IsKeyPressed(KEY_ENTER);input.cancel=IsKeyPressed(KEY_ESCAPE)||hit({176,265,58,24});
                    input.inspect=IsKeyPressed(KEY_I)||hit({8,265,58,24});
                    if(input.up||input.down||input.confirm||input.cancel||input.inspect)commerce_item_marker.reset();
                    const int count=static_cast<int>(view->entries.size());
                    const auto point=[&](std::array<int,4> r){return Rectangle{human_overview_origin.x+r[0],human_overview_origin.y+r[1],float(r[2]),float(r[3])};};
                    for(const auto &op:plan->operations)if(const auto *scroll=std::get_if<SteamHumanCatalogueScroll>(&op)) {
                        const auto &r=scroll->rectangles[1].rect;auto expanded=r;expanded[0]-=3;expanded[2]+=23;
                        if(count>3&&hit(point(expanded))) {
                            const float grid=std::max(float(110-(r[3]-1))/(count-3),1.F);
                            human_scroll_drag=FacilityScrollDrag{pid,mouse.y,view->first_visible,grid};
                            human_scroll_gesture=true;pointer_consumed=true;
                        } else if(count>3&&scroll->track_touch.rectangle&&hit(point(*scroll->track_touch.rectangle))) {
                            input.scroll=StartupCommerceFacilityInput::Scroll{false,mouse.y<human_overview_origin.y+r[1]?-3:3};
                            human_scroll_gesture=true;pointer_consumed=true;
                        }
                    }
                    const float wheel=GetMouseWheelMove();
                    if(count>3&&wheel!=0)input.scroll=StartupCommerceFacilityInput::Scroll{false,wheel>0?-1:1};
                    if(human_scroll_drag&&human_scroll_drag->page==pid) {
                        const double wanted=human_scroll_drag->first+std::trunc(double(mouse.y-human_scroll_drag->start_y)/human_scroll_drag->grid);
                        input.scroll=StartupCommerceFacilityInput::Scroll{true,int(std::clamp(wanted,0.,double(std::max(count-3,0))))};
                    }
                    if(input.scroll||human_scroll_gesture)commerce_item_marker.reset();
                    for(const auto &touch:management_touches(*plan,sprites))if(touch.component==11&&touch.rectangle&&
                        !input.cancel&&!input.inspect&&!input.scroll&&!pointer_consumed&&IsMouseButtonReleased(MOUSE_BUTTON_LEFT)&&
                        CheckCollisionPointRec(mouse,point(*touch.rectangle))) {
                        const int row=touch.value&0xffff;
                        input.selection=row;
                        input.confirm=input.confirm||(commerce_item_marker==std::make_pair(pid,row)&&view->selection==row);
                        commerce_item_marker=std::make_pair(pid,row);
                    }
                    if(input.up||input.down||input.confirm||input.cancel||input.inspect||input.selection||input.scroll)
                        if(session.input_commerce_facility(pid,input)!=StartupWorldRuntimeError::none)
                            throw std::runtime_error("Commerce facility input transaction rejected");
                    if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT))human_scroll_drag.reset();
                } else if(page->lifecycle!=0)throw std::runtime_error("Commerce facility input source rejected");
            } else if (raw == 84 || raw == 86) {
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
                        const int count = static_cast<int>(view->entries.size());
                        const int rows = 5;
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
                    if (raw != 86 && (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24})))
                        action = A::confirm;
                    if (action && session.act_commerce_page(page->id, *action, selected) !=
                                      StartupWorldRuntimeError::none)
                        throw std::runtime_error("商会输入事务失败");
                }
            } else if (raw >= 75 && raw <= 77) {
                const auto pid=page->id;
                if(session.state().facility_item_pages_initialized.count(pid)&&
                   valid_startup_world_facility_item_page(session.state(),*page)) {
                    StartupFacilityItemInput input;
                    input.confirm=raw==77&&IsKeyPressed(KEY_ENTER);
                    if(raw==75) {
                        input.up=IsKeyPressed(KEY_UP);input.down=IsKeyPressed(KEY_DOWN);
                        input.confirm=IsKeyPressed(KEY_ENTER);
                        input.cancel=IsKeyPressed(KEY_ESCAPE)||hit({176,265,58,24});
                        const int first=session.state().facility_item_page_scroll.at(pid);
                        const int count=static_cast<int>(session.state().facility_item_page_lists.at(pid).size());
                        const auto plan=window_facility_effect_plan(session.state(),pid,font,human_scroll_drag&&human_scroll_drag->page==pid);
                        if(!plan)throw std::runtime_error("Facility item input plan rejected");
                        const auto point=[&](std::array<int,4> r) {return Rectangle{
                            human_overview_origin.x+r[0],human_overview_origin.y+r[1],float(r[2]),float(r[3])};};
                        for(const auto &op:plan->operations) {
                            if(const auto *row=std::get_if<SteamFacilityEffectTouch>(&op)) {
                                std::optional<std::array<int,4>> bounds=row->touch.rectangle;
                                if(!bounds&&row->touch.image_draw&&*row->touch.image_draw<plan->operations.size()) {
                                    const auto *image=std::get_if<SteamFacilityImage>(&plan->operations[*row->touch.image_draw]);
                                    if(!image||image->asset!=SteamFacilityAsset::title_arrows)throw std::runtime_error("Facility arrow source rejected");
                                    auto r=sprites.manual_arrow_bounds(image->frame);r[0]+=image->position[0];r[1]+=image->position[1];bounds=r;
                                }
                                if(!bounds)throw std::runtime_error("Facility item row has no logical bounds");
                                auto r=*bounds;r[0]+=row->margin[0];r[1]+=row->margin[2];
                                r[2]+=row->margin[1]-row->margin[0];r[3]+=row->margin[3]-row->margin[2];
                                if(row->touch.component==11&&!input.cancel&&!pointer_consumed&&
                                   IsMouseButtonReleased(MOUSE_BUTTON_LEFT)&&CheckCollisionPointRec(mouse,point(r))) {
                                    const int chosen=row->touch.value&0xffff;
                                    input.selection=chosen;
                                    input.confirm|=row->touch.option==0||facility_item_marker==std::make_pair(pid,chosen);
                                    facility_item_marker=std::make_pair(pid,chosen);
                                } else if(row->touch.component==23&&hit(point(r))) {
                                    input.up|=row->touch.value==17;input.down|=row->touch.value==19;
                                }
                            } else if(const auto *scroll=std::get_if<SteamFacilityEffectScroll>(&op)) {
                                const auto &r=scroll->skin.rectangles[1].rect;
                                auto expanded=r;expanded[0]-=3;expanded[2]+=23;
                                if(count>5&&hit(point(expanded))) {
                                    const float grid=std::max(float(110-(r[3]-1))/(count-5),1.F);
                                    human_scroll_drag=FacilityScrollDrag{pid,mouse.y,first,grid};
                                    human_scroll_gesture=true;pointer_consumed=true;facility_item_marker.reset();
                                } else if(count>5&&scroll->track_touch.rectangle&&hit(point(*scroll->track_touch.rectangle))) {
                                    input.scroll=StartupFacilityItemInput::Scroll{false,mouse.y<human_overview_origin.y+r[1]?-5:5};
                                    pointer_consumed=true;facility_item_marker.reset();
                                }
                            } else if(const auto *touch=std::get_if<SteamStartupTouch>(&op)) {
                                if(touch->rectangle&&hit(point(*touch->rectangle))) {
                                    input.up|=(*touch->rectangle)[1]<100;input.down|=(*touch->rectangle)[1]>100;
                                }
                            }
                        }
                        const float wheel=GetMouseWheelMove();
                        if(count>5&&wheel!=0) {
                            input.scroll=StartupFacilityItemInput::Scroll{false,wheel>0?-1:1};
                            facility_item_marker.reset();
                        }
                        if(human_scroll_drag&&human_scroll_drag->page==pid) {
                            const double wanted=human_scroll_drag->first+std::trunc(double(mouse.y-human_scroll_drag->start_y)/human_scroll_drag->grid);
                            input.scroll=StartupFacilityItemInput::Scroll{true,int(std::clamp(wanted,0.,double(std::max(count-5,0))))};
                        }
                        if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT))human_scroll_drag.reset();
                    }
                    if(input.up||input.down||input.confirm||input.cancel||input.selection||input.scroll)
                        if(session.input_facility_item_page(pid,input)!=StartupWorldRuntimeError::none)
                            throw std::runtime_error("Facility item input transaction failed");
                }
            } else if (raw >= 51 && raw <= 54) {
                const auto view=inspect_startup_world_village_activity_page(session.state(),page->id);
                if(view) {
                    const auto pid=page->id;
                    const auto plan=window_village_activity_plan(session.state(),pid,font,human_scroll_drag&&human_scroll_drag->page==pid);
                    if(!plan)throw std::runtime_error("Village activity input plan rejected");
                    StartupVillageActivityInput input;
                    input.confirm=IsKeyPressed(KEY_ENTER);
                    const auto point=[&](std::array<int,4> r){return Rectangle{human_overview_origin.x+r[0],human_overview_origin.y+r[1],float(r[2]),float(r[3])};};
                    if(raw!=53) {
                        input.up=IsKeyPressed(raw==52?KEY_LEFT:KEY_UP);
                        input.down=IsKeyPressed(raw==52?KEY_RIGHT:KEY_DOWN);
                        input.cancel=IsKeyPressed(KEY_ESCAPE)||hit({176,265,58,24});
                    }
                    for(const auto &touch:management_touches(*plan,sprites)) {
                        if(!touch.rectangle)continue;
                        const auto r=point(*touch.rectangle);
                        if(!pointer_consumed&&IsMouseButtonReleased(MOUSE_BUTTON_LEFT)&&CheckCollisionPointRec(mouse,r)) {
                            if(touch.component==11||touch.component==3) {
                                input.selection=touch.value&0xffff;input.confirm=true;
                            } else if(touch.component==23||touch.component==2) {
                                input.up|=touch.component==23?touch.value==17:(*touch.rectangle)[1]<100;
                                input.down|=touch.component==23?touch.value==19:(*touch.rectangle)[1]>=100;
                            }
                        }
                    }
                    const int count=static_cast<int>(view->entries.size());
                    if(raw==51||raw==54) {
                        for(const auto &op:plan->operations)if(const auto *bar=std::get_if<SteamHumanCatalogueScroll>(&op)) {
                            const auto &r=bar->rectangles[1].rect;
                            auto expanded=r;expanded[0]-=3;expanded[2]+=23;
                            if(count>5&&hit(point(expanded))) {
                                const float grid=std::max(float(111-(r[3]-1))/(count-5),1.F);
                                human_scroll_drag=FacilityScrollDrag{pid,mouse.y,view->first_visible,grid};
                                human_scroll_gesture=true;pointer_consumed=true;
                            } else if(count>5&&bar->track_touch.rectangle&&hit(point(*bar->track_touch.rectangle))) {
                                input.scroll=StartupVillageActivityInput::Scroll{false,mouse.y<human_overview_origin.y+r[1]?-5:5};pointer_consumed=true;
                            }
                        }
                        const float wheel=GetMouseWheelMove();
                        if(count>5&&wheel!=0)input.scroll=StartupVillageActivityInput::Scroll{false,wheel>0?-1:1};
                        if(human_scroll_drag&&human_scroll_drag->page==pid) {
                            const double wanted=human_scroll_drag->first+std::trunc(double(mouse.y-human_scroll_drag->start_y)/human_scroll_drag->grid);
                            input.scroll=StartupVillageActivityInput::Scroll{true,int(std::clamp(wanted,0.,double(std::max(count-5,0))))};
                        }
                        if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT))human_scroll_drag.reset();
                    }
                    if(raw==53&&view->counter>=120&&!pointer_consumed&&IsMouseButtonReleased(MOUSE_BUTTON_LEFT))input.confirm=true;
                    if(input.up||input.down||input.confirm||input.cancel||input.selection||input.scroll)
                        if(session.input_village_activity_page(pid,input)!=StartupWorldRuntimeError::none)
                            throw std::runtime_error("Village activity transaction rejected");
                }
            } else if (raw == 48) {
                const auto plan=window_town_progress_plan(session.state(),page->id,font);
                if(plan) {
                    StartupTownRankInput input;input.up=IsKeyPressed(KEY_UP);input.down=IsKeyPressed(KEY_DOWN);
                    input.confirm=IsKeyPressed(KEY_ENTER);input.cancel=IsKeyPressed(KEY_ESCAPE)||hit({176,265,58,24});
                    for(const auto &touch:management_touches(*plan,sprites))if(touch.rectangle&&
                        !pointer_consumed&&IsMouseButtonReleased(MOUSE_BUTTON_LEFT)&&CheckCollisionPointRec(mouse,
                            {human_overview_origin.x+(*touch.rectangle)[0],human_overview_origin.y+(*touch.rectangle)[1],float((*touch.rectangle)[2]),float((*touch.rectangle)[3])})) {
                        input.selection=touch.value&0xffff;input.confirm=true;
                    }
                    if(input.up||input.down||input.selection||input.confirm||input.cancel)
                        if(session.input_rank_page(page->id,input)!=StartupWorldRuntimeError::none)throw std::runtime_error("Town rank input rejected");
                }
            } else if(raw==49||raw==50||raw==95) {
                bool confirm=IsKeyPressed(KEY_ENTER);
                if(const auto plan=window_town_progress_plan(session.state(),page->id,font)) {
                    for(const auto &touch:management_touches(*plan,sprites)) {
                        if(touch.rectangle&&touch.component==4&&hit({human_overview_origin.x+(*touch.rectangle)[0],human_overview_origin.y+(*touch.rectangle)[1],float((*touch.rectangle)[2]),float((*touch.rectangle)[3])}))confirm=true;
                        if(!touch.rectangle&&touch.component==2&&!pointer_consumed&&IsMouseButtonReleased(MOUSE_BUTTON_LEFT))confirm=true;
                    }
                }
                if(confirm&&session.acknowledge_page(page->id)!=StartupWorldRuntimeError::none)throw std::runtime_error("Town progress confirmation rejected");
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
                        residence_selection = (residence_selection + static_cast<int>(list.size()) - 1) %
                                         static_cast<int>(list.size());
                    if (IsKeyPressed(KEY_DOWN))
                        residence_selection = (residence_selection + 1) % static_cast<int>(list.size());
                }
                const bool cancel = IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24});
                if (cancel ||
                    (!list.empty() && (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24})))) {
                    const auto r = session.act_residence_page(
                        page->id, list.empty() ? -1 : list.at(residence_selection), cancel);
                    if (r.error != StartupWorldRuntimeError::none)
                        throw std::runtime_error("入住选择事务失败");
                }
            } else if(raw==70) {
                const auto plan=window_mastery_plan(session.state(),page->id,font);
                if(plan) {
                    StartupHumanCatalogueInput input;
                    input.left=IsKeyPressed(KEY_LEFT);input.right=IsKeyPressed(KEY_RIGHT);
                    input.confirm=IsKeyPressed(KEY_ENTER);
                    // Steam已核专属Update只读确认；APK的Back等价路径不扩成此窗口Esc操作。
                    for(const auto &touch:management_touches(*plan,sprites)) {
                        if(pointer_consumed||!IsMouseButtonReleased(MOUSE_BUTTON_LEFT))continue;
                        if(touch.rectangle) {
                            const auto &r=*touch.rectangle;
                            if(touch.component==3&&CheckCollisionPointRec(mouse,{human_overview_origin.x+r[0],human_overview_origin.y+r[1],float(r[2]),float(r[3])})) {
                                input.selection=touch.value&0xffff;input.confirm=true;
                            }
                        } else if(touch.component==2&&CheckCollisionPointRec(mouse,{0,0,float(width),float(height)}))input.confirm=true;
                    }
                    if(input.left||input.right||input.confirm||input.cancel||input.selection)
                        if(session.input_human_mastery_page(page->id,input)!=StartupWorldRuntimeError::none)
                            throw std::runtime_error("Mastery input transaction rejected");
                } else if(page->lifecycle!=0)throw std::runtime_error("Mastery input source rejected");
            } else if ((raw >= 60 && raw <= 66) || raw == 68 || raw == 69 ||
                       raw == 73) {
                if (startup_world_human_page_ready(session.state(), page->id)) {
                    using A = StartupHumanPageAction;
                    const auto pid = page->id;
                    const auto act = [&](A a, int row = 0) {
                        if (session.act_human_page(pid, a, row) != StartupWorldRuntimeError::none)
                            throw std::runtime_error("人物页面输入失败");
                    };
                    if (raw == 60) {
                        const auto view=inspect_startup_world_human_presentation(session.state(),pid);
                        if(!view)throw std::runtime_error("人物详情显示载荷拒绝");
                        {
                            const auto overview=window_human_overview_plan(session.state(),pid,font);
                            if(!overview)throw std::runtime_error("人物概览输入计划拒绝");
                            const bool cancel_pointer=hit({176,265,58,24});
                            bool gift_pointer{};
                            std::optional<int> equipment_row;
                            StartupHumanCatalogueInput input;
                            for(const auto &op:overview->operations)if(const auto *touch=std::get_if<SteamStartupTouch>(&op)) {
                                if(touch->component==4&&touch->rectangle&&!cancel_pointer) {
                                    const auto &r=*touch->rectangle;
                                    gift_pointer=hit({human_overview_origin.x+r[0],human_overview_origin.y+r[1],float(r[2]),float(r[3])});
                                }
                            } else if(const auto *scroll=std::get_if<SteamHumanOverviewScroll>(&op)) {
                                const auto point=[&](const std::array<int,4> &r) {return Rectangle{
                                    human_overview_origin.x+r[0],human_overview_origin.y+r[1],float(r[2]),float(r[3])};};
                                auto thumb=scroll->thumb_touch;thumb[0]-=3;thumb[2]+=23;
                                if(hit(point(thumb))) {
                                    human_scroll_drag=FacilityScrollDrag{pid,mouse.y,view->tab,32.F};
                                    human_scroll_gesture=true;pointer_consumed=true;
                                } else if(hit(point({221,69,3,127})))
                                    input.scroll=StartupHumanCatalogueInput::Scroll{false,mouse.y<human_overview_origin.y+thumb[1]?-1:1};
                            } else if(const auto *row=std::get_if<SteamHumanEquipmentRow>(&op)) {
                                if(!row->touch.rectangle)throw std::runtime_error("人物装备行缺热区");
                                const auto &r=*row->touch.rectangle;
                                if(hit({human_overview_origin.x+r[0]+row->margin[0],human_overview_origin.y+r[1]+row->margin[2],
                                        float(r[2]+row->margin[1]-row->margin[0]),float(r[3]+row->margin[3]-row->margin[2])}))
                                    equipment_row=row->touch.value&0xFFFF;
                            }
                            // 原同轮只选择一次页2分支，不能用两次Owner调用重新判中途页。
                            input.selection=equipment_row;input.up=IsKeyPressed(KEY_UP);input.down=IsKeyPressed(KEY_DOWN);
                            input.confirm=IsKeyPressed(KEY_ENTER)||gift_pointer||equipment_row.has_value();
                            input.cancel=IsKeyPressed(KEY_ESCAPE)||cancel_pointer;
                            const float wheel=GetMouseWheelMove();
                            if(wheel!=0)input.scroll=StartupHumanCatalogueInput::Scroll{false,wheel>0?-1:1};
                            if(human_scroll_drag&&human_scroll_drag->page==pid) {
                                const double wanted=human_scroll_drag->first+std::trunc(double(mouse.y-human_scroll_drag->start_y)/human_scroll_drag->grid);
                                input.scroll=StartupHumanCatalogueInput::Scroll{true,int(std::clamp(wanted,0.,3.))};
                            }
                            if(input.up||input.down||input.confirm||input.cancel||input.selection||input.scroll) {
                                if(session.input_human_detail_page(pid,input)!=StartupWorldRuntimeError::none)
                                    throw std::runtime_error("人物详情同轮输入失败");
                            } else if(view->tracking_available&&(IsKeyPressed(KEY_T)||hit({8,265,58,24})))act(A::track);
                            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT))human_scroll_drag.reset();
                        }
                    } else if(raw==61||raw==64) {
                        const auto cursor=inspect_startup_world_human_catalogue_cursor(session.state(),pid);
                        const auto plan=window_human_catalogue_plan(session.state(),pid,font,human_scroll_drag&&human_scroll_drag->page==pid);
                        if(!cursor||!plan)throw std::runtime_error("人物目录输入载荷拒绝");
                        StartupHumanCatalogueInput input;
                        input.up=IsKeyPressed(KEY_UP);input.down=IsKeyPressed(KEY_DOWN);
                        input.left=IsKeyPressed(KEY_LEFT);input.right=IsKeyPressed(KEY_RIGHT);
                        input.confirm=IsKeyPressed(KEY_ENTER);
                        input.cancel=IsKeyPressed(KEY_ESCAPE)||hit({176,265,58,24});
                        input.inspect_equipment=raw==64&&plan->soft_labels[0]==7&&(IsKeyPressed(KEY_I)||hit({8,265,58,24}));
                        const auto point=[&](std::array<int,4> r) {return Rectangle{
                            human_overview_origin.x+r[0],human_overview_origin.y+r[1],float(r[2]),float(r[3])};};
                        for(const auto &op:plan->operations) {
                            if(const auto *row=std::get_if<SteamHumanEquipmentRow>(&op)) {
                                std::optional<std::array<int,4>> bounds=row->touch.rectangle;
                                if(!bounds&&row->touch.image_draw&&*row->touch.image_draw<plan->operations.size()) {
                                    const auto *frame=std::get_if<SteamStartupFrameDraw>(&plan->operations[*row->touch.image_draw]);
                                    const auto *image=frame?std::get_if<StartupSkinDraw>(frame):nullptr;
                                    if(!image||image->image!=74||image->sprite!=3)throw std::runtime_error("人物目录箭头热区身份拒绝");
                                    auto b=sprites.manual_arrow_bounds(image->frame);
                                    b[0]+=image->offset[0];b[1]+=image->offset[1];bounds=b;
                                }
                                if(!bounds)throw std::runtime_error("人物目录行缺热区");
                                auto r=*bounds;r[0]+=row->margin[0];r[1]+=row->margin[2];
                                r[2]+=row->margin[1]-row->margin[0];r[3]+=row->margin[3]-row->margin[2];
                                if(row->touch.component==11&&!input.cancel&&!input.inspect_equipment&&!pointer_consumed&&
                                   IsMouseButtonReleased(MOUSE_BUTTON_LEFT)&&CheckCollisionPointRec(mouse,point(r))) {
                                    const int selected=row->touch.value&0xffff;
                                    input.selection=selected;
                                    // 原Touch11只在UP发确认；道具marker16要求同key再次UP。
                                    input.confirm=input.confirm||row->touch.option==0||human_catalogue_marker==std::make_pair(pid,selected);
                                    human_catalogue_marker=std::make_pair(pid,selected);
                                } else if(row->touch.component==23&&hit(point(r))) {
                                    input.up=row->touch.value==17;input.down=row->touch.value==19;
                                }
                            } else if(const auto *arrow=std::get_if<SteamFacilityArrowTouch>(&op)) {
                                const auto expanded=facility_detail_arrow(*arrow,sprites);
                                if(!pointer_consumed&&IsMouseButtonReleased(MOUSE_BUTTON_LEFT)&&CheckCollisionPointRec(mouse,point(expanded.touch_rectangle)))
                                    {input.left=input.left||arrow->value==16;input.right=input.right||arrow->value==18;}
                            } else if(const auto *scroll=std::get_if<SteamHumanCatalogueScroll>(&op)) {
                                const auto &r=scroll->rectangles[1].rect;
                                auto expanded=r;expanded[0]-=3;expanded[2]+=23;
                                if(cursor->count>cursor->visible&&hit(point(expanded))) {
                                    const float grid=std::max(float(110-(r[3]-1))/(cursor->count-cursor->visible),1.F);
                                    human_scroll_drag=FacilityScrollDrag{pid,mouse.y,cursor->first,grid};
                                    human_scroll_gesture=true;pointer_consumed=true;
                                } else if(cursor->count>cursor->visible&&scroll->track_touch.rectangle&&hit(point(*scroll->track_touch.rectangle)))
                                    input.scroll=StartupHumanCatalogueInput::Scroll{false,mouse.y<human_overview_origin.y+r[1]?-cursor->visible:cursor->visible};
                            }
                        }
                        const float wheel=GetMouseWheelMove();
                        if(cursor->count>cursor->visible&&wheel!=0)input.scroll=StartupHumanCatalogueInput::Scroll{false,wheel>0?-1:1};
                        if(human_scroll_drag&&human_scroll_drag->page==pid) {
                            const double wanted=human_scroll_drag->first+std::trunc(double(mouse.y-human_scroll_drag->start_y)/human_scroll_drag->grid);
                            input.scroll=StartupHumanCatalogueInput::Scroll{true,int(std::clamp(wanted,0.,double(cursor->count-cursor->visible)))};
                        }
                        if(input.up||input.down||input.left||input.right||input.confirm||input.cancel||input.inspect_equipment||input.selection||input.scroll)
                            if(session.input_human_catalogue_page(pid,input)!=StartupWorldRuntimeError::none)
                                throw std::runtime_error("人物目录同轮输入失败");
                        if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT))human_scroll_drag.reset();
                    } else if (raw == 62 || raw == 73) {
                        bool confirm_pointer{};
                        if (raw == 62 || raw == 73) {
                            std::optional<int> pointer;
                            if(raw==73||raw==62) {
                                SteamHumanOverviewOptions options;options.view_y=23;options.title_widths=std::array<int,2>{0,0};
                                const auto plan=raw==73?steam_human_equipment_detail_skin(session.state(),pid,options):
                                    window_human_profession_plan(session.state(),pid,font);
                                if(!plan)throw std::runtime_error("装备信息输入载荷拒绝");
                                for(const auto &op:plan->operations)if(const auto *arrow=std::get_if<SteamFacilityArrowTouch>(&op)) {
                                    const auto expanded=facility_detail_arrow(*arrow,sprites);
                                    const auto &r=expanded.touch_rectangle;
                                    if(hit({human_overview_origin.x+r[0],human_overview_origin.y+r[1],float(r[2]),float(r[3])}))pointer=arrow->value;
                                }
                                if(raw==62)for(const auto &op:plan->operations)if(const auto *touch=std::get_if<SteamStartupTouch>(&op))
                                    if(touch->component==4&&touch->rectangle) {
                                        const auto &r=*touch->rectangle;
                                        confirm_pointer=hit({human_overview_origin.x+r[0],human_overview_origin.y+r[1],float(r[2]),float(r[3])});
                                    }
                            }
                            if (IsKeyPressed(KEY_LEFT)||pointer==16)
                                act(A::previous);
                            if (IsKeyPressed(KEY_RIGHT)||pointer==18)
                                act(A::next);
                        }
                        if (IsKeyPressed(KEY_ESCAPE) || (raw!=62&&hit({8, 265, 58, 24}))||((raw==73||raw==62)&&hit({176,265,58,24})))
                            act(A::cancel);
                        else if (IsKeyPressed(KEY_ENTER) || confirm_pointer||((raw!=62&&raw!=73)&&hit({176, 265, 58, 24})))
                            act(A::confirm);
                        else if (raw == 64 && session.state().page_phases.at(pid) < 4 &&
                                 hit({96, 265, 52, 24}))
                            act(A::inspect_equipment);
                    } else if (raw == 65) {
                        const auto plan=window_human_gift_confirmation_plan(session.state(),pid,font);
                        if(!plan)throw std::runtime_error("赠礼确认输入载荷拒绝");
                        const bool cancel=IsKeyPressed(KEY_ESCAPE)||hit({176,265,58,24});
                        bool confirm{};
                        for(const auto &op:plan->operations)if(const auto *touch=std::get_if<SteamStartupTouch>(&op)) {
                            if(touch->component==4&&touch->rectangle&&!cancel) {
                                const auto &r=*touch->rectangle;
                                confirm=hit({human_overview_origin.x+r[0],human_overview_origin.y+r[1],float(r[2]),float(r[3])});
                            }
                        }
                        if(cancel)
                            act(A::cancel);
                        else if(IsKeyPressed(KEY_ENTER)||confirm)
                            act(A::confirm);
                    } else {
                        if (IsKeyPressed(KEY_ENTER) || (raw!=63&&raw!=66&&hit({176, 265, 58, 24})))
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
                const bool ordinary=startup_world_facility_page_count(session.state(),*page)==2;
                const bool preview=ordinary_facility_preview(session.state(),*page);
                const bool special=special_facility_page(session.state(),*page);
                const bool formal=ordinary||preview||special;
                const auto detail_phase=session.state().page_phases.find(pid);
                if(formal&&detail_phase==session.state().page_phases.end())
                    throw std::runtime_error("设施详情缺已初始化页码");
                if(facility_scroll_drag&&(facility_scroll_drag->page!=pid||!ordinary||
                    detail_phase->second!=1))facility_scroll_drag.reset();
                const bool scroll_release=facility_scroll_drag&&IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
                if(facility_scroll_drag)pointer_consumed=true;
                const bool pointer_return=formal&&hit({176,265,58,24});
                bool row_confirm{},button_confirm{},left_click{},right_click{};
                if(formal) {
                    const auto plan=special?window_facility_special_plan(session.state(),pid,font):
                        preview?window_facility_definition_plan(session.state(),pid,font):
                        window_facility_detail_plan(session.state(),pid,{bonus_selection,bonus_scroll},font);
                    if(!plan)throw std::runtime_error("设施详情输入计划拒绝");
                    const auto point=[&](std::array<int,4> r) {
                        return Rectangle{facility_detail_origin.x+r[0],facility_detail_origin.y+r[1],float(r[2]),float(r[3])};
                    };
                    for(const auto &op:plan->operations) {
                        if(const auto *touch=std::get_if<SteamStartupTouch>(&op)) {
                            if(!touch->rectangle)continue;
                            if(touch->component==11&&!pointer_consumed&&IsMouseButtonReleased(MOUSE_BUTTON_LEFT)&&
                               CheckCollisionPointRec(mouse,point(*touch->rectangle))) {
                                bonus_selection=touch->value&0xffff;row_confirm=true;
                            // 源按钮扩展基矩形可能重叠框架适配返回，不把返回误投给道具。
                            } else if(touch->component==4&&!pointer_return&&hit(point(*touch->rectangle)))button_confirm=true;
                        } else if(const auto *arrow=std::get_if<SteamFacilityArrowTouch>(&op)) {
                            const auto expanded=facility_detail_arrow(*arrow,sprites);
                            if(hit(point(expanded.touch_rectangle))) {
                                if(arrow->value==16)left_click=true;else right_click=true;
                                facility_arrow_press=std::make_pair(pid,arrow->value);
                            }
                        } else if(const auto *s=std::get_if<SteamFacilityScrollRequest>(&op)) {
                            const auto bar=steam_facility_scroll_skin(*s,bonus_scroll);
                            if(!bar)throw std::runtime_error("设施滚动条输入载荷拒绝");
                            if(s->count>5&&hit(point(bar->thumb_touch))) {
                                const float grid=std::max(float(s->rectangle[3]-bar->thumb_touch[3])/(s->count-5),1.F);
                                facility_scroll_drag=FacilityScrollDrag{pid,mouse.y,bonus_scroll,grid};
                                facility_scroll_gesture=true;
                                pointer_consumed=true;
                            }
                        }
                    }
                }
                if (!session.state().facility_definition_page_bindings.count(pid) &&
                    session.state().page_phases.at(pid) == 1) {
                    const auto rows = startup_world_facility_bonus_rows(session.state(), pid);
                    if (!rows) throw std::runtime_error("设施奖励来源载荷非法");
                    const int count = static_cast<int>(rows->size());
                    if (count > 0) {
                        const auto cursor=steam_facility_bonus_cursor(count,{bonus_selection,bonus_scroll},
                            IsKeyPressed(KEY_UP),IsKeyPressed(KEY_DOWN));
                        if(!cursor)throw std::runtime_error("设施奖励选择/视窗非法");
                        bonus_selection=cursor->selection;bonus_scroll=cursor->first;
                        const auto wheel = GetMouseWheelMove();
                        if (wheel != 0)
                            bonus_selection = std::clamp(bonus_selection + (wheel > 0 ? -1 : 1),0,count-1);
                        bonus_scroll = std::clamp(bonus_scroll,bonus_selection-4,bonus_selection);
                        bonus_scroll = std::clamp(bonus_scroll,0,std::max(count-5,0));
                        if(facility_scroll_drag&&facility_scroll_drag->page==pid) {
                            // 此屏幕位移→绝对逻辑事件是研究适配；原Surface事件合成另验。
                            const double wanted=facility_scroll_drag->first+
                                std::trunc(double(mouse.y-facility_scroll_drag->start_y)/facility_scroll_drag->grid);
                            const auto next=steam_facility_bonus_scroll(count,{bonus_selection,bonus_scroll},
                                SteamFacilityScrollEvent::absolute,int(std::clamp(wanted,0.,double(count-5))));
                            if(!next)throw std::runtime_error("设施拖动光标载荷拒绝");
                            bonus_selection=next->selection;bonus_scroll=next->first;
                        }
                    }
                }
                // Steam两个repeat独立处理；同轮左右会切两次，不合并为一次OR。
                if (!session.state().facility_definition_page_bindings.count(pid))
                    for(const bool pulse:{IsKeyPressed(KEY_LEFT)||left_click,IsKeyPressed(KEY_RIGHT)||right_click})
                        if(pulse&&session.act_facility_page(pid,StartupFacilityPageAction::next)!=StartupWorldRuntimeError::none)
                            throw std::runtime_error("设施情报翻页失败");
                // raw74无marker16；列表release选择并发确认，第二页由Owner消费为no-op。
                if (IsKeyPressed(KEY_ENTER)||row_confirm||button_confirm||(!formal&&hit({176,265,58,24}))) {
                    const auto error =
                        session.act_facility_page(pid, StartupFacilityPageAction::confirm);
                    if (error != StartupWorldRuntimeError::none)
                        command_feedback = session.last_error().empty()?"暂不可用":session.last_error();
                } else if (IsKeyPressed(KEY_ESCAPE)||pointer_return||(!formal&&hit({8,265,58,24}))) {
                    // 原确认优先，第二页确认no-op时不能再消费同轮返回。
                    if (session.act_facility_page(pid, StartupFacilityPageAction::cancel) !=
                        StartupWorldRuntimeError::none)
                        throw std::runtime_error("设施情报返回失败");
                }
                if(ordinary)bonus_views[pid]={bonus_selection,bonus_scroll};
                if(scroll_release||!IsMouseButtonDown(MOUSE_BUTTON_LEFT)||
                   !session.state().page_phases.count(pid)||session.state().page_phases.at(pid)!=1)
                    facility_scroll_drag.reset();
            } else if ((raw>=25&&raw<=28)||(raw>=31&&raw<=33)) {
                const auto pid=page->id;const auto &state=session.state();
                const auto plan=window_task_followup_plan(state,pid,font,human_scroll_drag&&human_scroll_drag->page==pid);
                if(plan) {
                    StartupTaskPageInput input;
                    input.up=(raw<=27||raw==31||raw==32)&&IsKeyPressed(KEY_UP);
                    input.down=(raw<=27||raw==31||raw==32)&&IsKeyPressed(KEY_DOWN);
                    input.left=(raw==27||raw==28||raw==33)&&IsKeyPressed(KEY_LEFT);
                    input.right=(raw==27||raw==28||raw==33)&&IsKeyPressed(KEY_RIGHT);
                    input.confirm=IsKeyPressed(KEY_ENTER);
                    input.inspect=raw<=27&&(IsKeyPressed(KEY_I)||hit({8,265,82,24}));
                    input.depart=raw==25&&(IsKeyPressed(KEY_S)||hit({176,265,58,24}));
                    input.cancel=raw!=33&&(IsKeyPressed(KEY_ESCAPE)||(raw!=25&&hit({176,265,58,24})));
                    const auto point=[&](std::array<int,4> r){return Rectangle{human_overview_origin.x+r[0],human_overview_origin.y+r[1],float(r[2]),float(r[3])};};
                    // 原滚条只读Owner first；平台仅存拖动起点，先消费手势避免松开同时确认。
                    std::visit([&](const auto &value) {
                        if constexpr(!std::is_same_v<std::decay_t<decltype(value)>,SteamTaskDeadlinePlan>) {
                        for(const auto &op:value.operations)if(const auto *bar=std::get_if<SteamHumanCatalogueScroll>(&op)) {
                            const int count=bar->arguments[0],visible=bar->arguments[1];
                            if(count<=visible)continue;
                            const int first=state.task_page_ui.at(pid).first;
                            const auto &r=bar->rectangles[1].rect;auto area=r;area[0]-=3;area[2]+=23;
                            if(hit(point(area))) {
                                const float grid=std::max(float(bar->rectangles[0].rect[3]-(r[3]-1))/(count-visible),1.F);
                                human_scroll_drag=FacilityScrollDrag{pid,mouse.y,first,grid};
                                human_scroll_gesture=true;pointer_consumed=true;task_item_marker.reset();
                            } else if(bar->track_touch.rectangle&&hit(point(*bar->track_touch.rectangle))) {
                                input.scroll=StartupTaskPageInput::Scroll{false,mouse.y<human_overview_origin.y+r[1]?-visible:visible};pointer_consumed=true;
                            }
                            const float wheel=GetMouseWheelMove();
                            if(wheel!=0)input.scroll=StartupTaskPageInput::Scroll{false,wheel>0?-1:1};
                            if(human_scroll_drag&&human_scroll_drag->page==pid) {
                                const double wanted=human_scroll_drag->first+std::trunc(double(mouse.y-human_scroll_drag->start_y)/human_scroll_drag->grid);
                                input.scroll=StartupTaskPageInput::Scroll{true,int(std::clamp(wanted,0.,double(count-visible)))};
                            }
                        }
                        }
                        for(const auto &touch:management_touches(value,sprites)) {
                            if(pointer_consumed||!IsMouseButtonReleased(MOUSE_BUTTON_LEFT))continue;
                            if(!touch.rectangle) {
                                if(touch.component==2&&touch.option==2&&CheckCollisionPointRec(mouse,{0,0,float(width),float(height)}))input.confirm=true;
                                continue;
                            }
                            if(!CheckCollisionPointRec(mouse,point(*touch.rectangle)))continue;
                            if(touch.component==11||touch.component==3) {
                                const int row=touch.value&0xffff;
                                if(raw!=33||state.page_phases.at(pid)==0)input.selection=row;
                                input.confirm|=touch.option==0||task_item_marker==std::make_pair(pid,row);
                                if(raw==27)task_item_marker=std::make_pair(pid,row);
                            } else if(touch.component==1) {input.left|=touch.value==16;input.right|=touch.value==18;}
                        }
                    },*plan);
                    if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT))human_scroll_drag.reset();
                    if(input.up||input.down||input.left||input.right||input.scroll||input.cancel||input.inspect)task_item_marker.reset();
                    if(input.confirm||input.depart)++confirmation_inputs;
                    if(input.up||input.down||input.left||input.right||input.confirm||input.cancel||input.inspect||input.depart||input.selection||input.scroll)
                        if(session.input_task_page(pid,input).error!=StartupWorldRuntimeError::none)
                            throw std::runtime_error("Task followup input transaction rejected");
                } else if(page->lifecycle!=0)throw std::runtime_error("Task followup input source rejected");
            } else if (raw==13) {
                StartupManualInput input;
                input.confirm=IsKeyPressed(KEY_ENTER);
                input.right=IsKeyPressed(KEY_RIGHT);
                input.left=IsKeyPressed(KEY_LEFT);
                input.cancel=IsKeyPressed(KEY_ESCAPE) || hit({8,265,58,24});
                if(input.confirm||input.right||input.left||input.cancel) {
                    if(session.input_manual_page(page->id,input)!=StartupWorldRuntimeError::none)
                        throw std::runtime_error("说明翻页输入拒绝");
                }
            } else if (raw==17 && session.application_mode()) {
                // 应用确认本身推进计分；只在下面获准的Update里消费，避免绘制帧双推进。
                const auto plan=window_clear_score_plan(*session.application(),page->id,font);
                if(plan) {
                    bool confirm=IsKeyPressed(KEY_ENTER);
                    for(const auto &touch:management_touches(*plan,sprites))
                        if(touch.component==2&&!touch.rectangle&&!pointer_consumed&&
                           IsMouseButtonReleased(MOUSE_BUTTON_LEFT)&&
                           CheckCollisionPointRec(mouse,{0,0,float(width),float(height)}))confirm=true;
                    if(confirm){clear_confirm_pending=page->id;++confirmation_inputs;}
                } else if(page->lifecycle!=0)throw std::runtime_error("Clear score input source rejected");
            } else if (raw==96||raw==67||raw==88) {
                const auto plan=raw==88?steam_human_award_result_skin(session.state(),page->id,
                    award_result_options(font,steam_human_shared_character_frame(session.state()))):
                    raw==67?steam_human_effort_result_skin(session.state(),page->id,effort_result_options(font)):
                    steam_human_residence_result_skin(session.state(),page->id,
                    residence_result_options(font,steam_human_shared_character_frame(session.state())));
                const bool clicked=plan&&!pointer_consumed&&IsMouseButtonReleased(MOUSE_BUTTON_LEFT)&&
                    CheckCollisionPointRec(mouse,{0,0,float(width),float(height)});
                if(plan&&(clicked||IsKeyPressed(KEY_ENTER))) {
                    ++confirmation_inputs;pointer_consumed=pointer_consumed||clicked;
                    if(session.acknowledge_page(page->id)!=StartupWorldRuntimeError::none)
                        throw std::runtime_error("人物仪式页面输入拒绝");
                }
            } else if (raw==30) {
                // 原Touch2无显式矩形；此240×320适配在画布内松开确认，原OS投递另验。
                const auto plan=steam_task_victory_skin(session.state(),page->id,window_task_victory_options(font));
                const bool clicked=plan&&!pointer_consumed&&IsMouseButtonReleased(MOUSE_BUTTON_LEFT)&&
                    CheckCollisionPointRec(mouse,{0,0,float(width),float(height)});
                if(plan&&(clicked||IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_ESCAPE))) {
                    ++confirmation_inputs;pointer_consumed=pointer_consumed||clicked;
                    if(session.acknowledge_page(page->id)!=StartupWorldRuntimeError::none)
                        throw std::runtime_error("胜利页面输入拒绝");
                }
            } else if (raw==14) {
                const auto view=inspect_startup_world_save_page(session.state(),page->id);
                bool clicked{};
                if(view)if(const auto plan=window_save_plan(*view,font))
                    for(const auto &touch:plan->touches)if(touch.component==3 && touch.rectangle) {
                        const auto &r=*touch.rectangle;
                        clicked=clicked || hit({float(plan->origin[0]+r[0]),float(plan->origin[1]+r[1]),
                                                float(r[2]),float(r[3])});
                    }
                // 原单按钮基矩形；鼠标短适配不替代完整Steam surface的ENTER/UP认证。
                if(clicked || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
                    ++confirmation_inputs;
                    if(session.acknowledge_page(page->id)!=StartupWorldRuntimeError::none)
                        throw std::runtime_error("保存结果确认失败");
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
        if(!started_with_modal&&!top_page()&&!session.state().scene.framework_paused&&session.state().scene.scene_state==4) {
            StartupTaskPageInput input;
            input.left=IsKeyPressed(KEY_LEFT);input.right=IsKeyPressed(KEY_RIGHT);
            input.confirm=IsKeyPressed(KEY_ENTER);
            input.cancel=IsKeyPressed(KEY_ESCAPE)||hit({176,265,58,24});input.track=IsKeyPressed(KEY_T);
            const auto plan=steam_task_information_scene_skin(session.state(),window_task_info_options(font,true));
            if(!plan)throw std::runtime_error("Task map information source rejected");
            for(const auto &touch:management_touches(*plan,sprites))if(touch.rectangle&&!pointer_consumed&&IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                const auto &r=*touch.rectangle;
                if(CheckCollisionPointRec(mouse,{float(r[0]),float(r[1]),float(r[2]),float(r[3])})) {
                    if(touch.component==4)input.confirm=true;
                    else if(touch.component==1){input.left|=touch.value==16;input.right|=touch.value==18;}
                }
            }
            if(input.left||input.right||input.confirm||input.cancel||input.track)
                if(session.input_task_tracking_scene(input)!=StartupWorldRuntimeError::none)throw std::runtime_error("Task scene input transaction rejected");
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
        const auto held_page = top_page();
        if(!session.application_mode() || !held_page || held_page->legacy_page!=17)
            session.set_page_confirm_held(
            held_page && held_page->legacy_page == 24 &&
            (IsKeyDown(KEY_ENTER) || (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !pointer_consumed && IsWindowFocused())));
        // 框架绘制门槛与逻辑轮数分开；长停顿不补算，倍速由MainScene保存轮数。
        const auto gate =
            ref::prepare_world_render_gate(clock, static_cast<std::int64_t>(GetTime() * 1000));
        if (gate.error != ref::WorldRenderGateError::none)
            throw std::runtime_error("共同世界绘制时钟失败");
        if (gate.clock) {
            clock = *gate.clock;
            (void)session.take_audio_requests(); // 命令可能产生输出，先消费再进入应用保存事务。
            const auto page = top_page();
            const auto save=page && page->kind==ref::WorldScriptPageKind::raw_page && page->legacy_page==14
                ? inspect_startup_world_save_page(session.state(),page->id) : std::nullopt;
            if(session.application_mode() || !save || save->stage!=1) {
                const bool clear_confirm=page && clear_confirm_pending && page->id==*clear_confirm_pending &&
                    page->kind==ref::WorldScriptPageKind::raw_page && page->legacy_page==17;
                const auto result = session.update(clear_confirm);
                clear_confirm_pending.reset();
                if (result.error != StartupWorldRuntimeError::none)
                    throw std::runtime_error(
                        "共同世界更新失败：" + std::to_string(static_cast<int>(result.error)) + "/" +
                        std::to_string(static_cast<int>(result.scene_error)) + "/" +
                        std::to_string(static_cast<int>(result.world_error)));
            }
        }
        // 本研究窗口暂未实现音频；明确消费一次性请求，不保存无限声音历史。
        (void)session.take_sound_requests();
        const auto &state = session.state();
        if(facility_scroll_drag) {
            const auto active=std::find_if(state.scripts.pages.rbegin(),state.scripts.pages.rend(),
                [](const auto &p){return p.lifecycle!=4;});
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT)||state.scene.framework_paused||
                active==state.scripts.pages.rend()||active->id!=facility_scroll_drag->page||
                !state.page_phases.count(active->id)||state.page_phases.at(active->id)!=1)
                facility_scroll_drag.reset();
        }
        if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT)){facility_scroll_gesture=false;human_scroll_gesture=false;human_scroll_drag.reset();}
        for(auto it=bonus_views.begin();it!=bonus_views.end();) {
            const auto page=std::find_if(state.scripts.pages.begin(),state.scripts.pages.end(),
                [&](const auto &p){return p.id==it->first&&p.legacy_page==74&&p.lifecycle!=4;});
            if(page==state.scripts.pages.end()||!state.facility_page_neighbours.count(it->first))it=bonus_views.erase(it);
            else ++it;
        }
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
        if(!top_page()&&state.scene.scene_state==4) {
            const auto plan=steam_task_information_scene_skin(state,window_task_info_options(font,true));
            if(!plan)throw std::runtime_error("Task scene drawing source rejected");
            draw_facility_ceremony(*plan,sprites,font,{0,0});
        }
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
        if (const auto page = top_page(); page && (navigation_menu(page->legacy_page)||page->legacy_page==9)) {
            // 真实菜单栈逐页画；被菜单覆盖的raw3保留底图但不登记主菜单ID8短适配。
            for (const auto &entry:state.scripts.pages)
                if(entry.lifecycle!=4 && entry.kind==ref::WorldScriptPageKind::raw_page &&
                   (navigation_menu(entry.legacy_page)||entry.legacy_page==9))
                    if(const auto plan=window_navigation_plan(state,entry.id,font,entry.id==page->id))
                        draw_navigation_plan(*plan,sprites,font);
            DrawRectangle(8,258,62,22,paper);DrawRectangle(166,258,66,22,paper);
            font.text("返回 Esc",12,263,ink,10);font.text("确定 Enter",170,263,ink,10);
            if(!command_feedback.empty())font.text(command_feedback,8,285,ink,10);
        } else if (const auto page = top_page(); page && page->legacy_page==13) {
            const auto view=inspect_startup_world_manual_page(state,page->id);
            if(!view)throw std::runtime_error("说明页面载荷拒绝");
            const auto plan=window_manual_plan(state,*view);
            if(!plan)throw std::runtime_error("说明页面皮肤拒绝");
            draw_manual_plan(*plan,state,sprites,font);
            font.text("返回 Esc",12,272,ink,10);
        } else if (const auto page = top_page(); page && page->legacy_page==14) {
            if(const auto view=inspect_startup_world_save_page(state,page->id)) {
                const auto plan=window_save_plan(*view,font);
                if(!plan)throw std::runtime_error("保存页皮肤拒绝");
                draw_save_plan(*plan,*view,sprites,font);
                if(view->stage==1 && !session.application_mode())font.text("本窗口仅预览保存页",12,270,ink,11);
            }
        } else if (const auto page = top_page(); page && page->legacy_page==17 && session.application_mode()) {
            const auto plan=window_clear_score_plan(*session.application(),page->id,font);
            if(plan)draw_human_overview(*plan,sprites,font);
            else if(page->lifecycle!=0)throw std::runtime_error("Clear score drawing source rejected");
        } else if (const auto page = top_page(); page && page->legacy_page>=34 && page->legacy_page<=40) {
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
        } else if (const auto page = top_page()) {
            const bool steam_info=page->legacy_page==29;
            const bool steam_task=(page->legacy_page>=22&&page->legacy_page<=28)||(page->legacy_page>=30&&page->legacy_page<=33);
            const bool steam_residence=page->legacy_page==96||page->legacy_page==67||page->legacy_page==88||page->legacy_page==70;
            const bool steam_gift_claim=page->legacy_page==93||page->legacy_page==94;
            const bool steam_magic=page->legacy_page>=41&&page->legacy_page<=47;
            const bool steam_commodity=page->legacy_page==72||page->legacy_page==79;
            const bool steam_commerce_top=page->legacy_page==83;
            const bool steam_commerce_result=page->legacy_page==86;
            const bool steam_commerce_facility=page->legacy_page==85;
            const bool steam_reputation=page->legacy_page==82;
            const bool steam_effect=page->legacy_page>=75&&page->legacy_page<=77;
            const bool steam_facility=page->legacy_page==74&&
                (startup_world_facility_page_count(state,*page)==2||ordinary_facility_preview(state,*page)||
                 special_facility_page(state,*page));
            const auto human_view=page->legacy_page==60?inspect_startup_world_human_presentation(state,page->id):std::nullopt;
            const bool steam_human=human_view.has_value();
            const bool steam_equipment=page->legacy_page==73&&startup_world_human_page_ready(state,page->id);
            const bool steam_catalogue=(page->legacy_page==61||page->legacy_page==64)&&startup_world_human_page_ready(state,page->id);
            const bool steam_gift=page->legacy_page==65&&startup_world_human_page_ready(state,page->id);
            const bool steam_gift_result=page->legacy_page==66&&startup_world_human_page_ready(state,page->id);
            const bool steam_attribute_result=(page->legacy_page==68||page->legacy_page==69)&&startup_world_human_page_ready(state,page->id);
            const bool steam_profession=(page->legacy_page==62||page->legacy_page==63)&&startup_world_human_page_ready(state,page->id);
            if (page->legacy_page != 21&&!steam_commerce_top&&!steam_commerce_result&&!steam_commerce_facility&&!steam_gift_claim&&!steam_info&&!steam_task&&!steam_residence&&!steam_magic&&!steam_reputation&&!steam_effect&&!steam_commodity&&!steam_facility&&!steam_human&&!steam_catalogue&&!steam_equipment&&!steam_gift&&!steam_profession&&!steam_gift_result&&!steam_attribute_result) DrawRectangle(5, 170, 230, 119, paper);
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
            if (page->legacy_page != 21&&!steam_commerce_top&&!steam_commerce_result&&!steam_commerce_facility&&!steam_gift_claim&&!steam_info&&!steam_task&&!steam_residence&&!steam_magic&&!steam_reputation&&!steam_effect&&!steam_commodity&&!steam_facility&&!steam_human&&!steam_catalogue&&!steam_equipment&&!steam_gift&&!steam_profession&&!steam_gift_result&&!steam_attribute_result)
                font.text(title, 12, 177, ink, font.measure(title) > 208 ? 10 : 12);
            if(steam_gift_claim) {
                const auto plan=window_town_progress_plan(state,page->id,font);
                if(plan)draw_facility_ceremony(*plan,sprites,font);
                else if(page->lifecycle!=0)throw std::runtime_error("Reward drawing source rejected");
            } else if(steam_residence) {
                const auto plan=page->legacy_page==70?window_mastery_plan(state,page->id,font):
                    page->legacy_page==88?steam_human_award_result_skin(state,page->id,
                    award_result_options(font,steam_human_shared_character_frame(state))):
                    page->legacy_page==67?steam_human_effort_result_skin(state,page->id,effort_result_options(font)):
                    steam_human_residence_result_skin(state,page->id,
                    residence_result_options(font,steam_human_shared_character_frame(state)));
                if(plan)draw_facility_ceremony(*plan,sprites,font);
                else if(page->lifecycle!=0)throw std::runtime_error("Human ceremony drawing rejected");
            } else if(steam_info) {
                const auto plan=steam_task_information_skin(state,page->id,window_task_info_options(font,false));
                if(plan)draw_facility_ceremony(*plan,sprites,font,{0,0});
                else if(page->lifecycle!=0)throw std::runtime_error("Task info drawing source rejected");
            } else if (steam_task) {
                if(page->legacy_page==30) {
                    const auto plan=steam_task_victory_skin(state,page->id,window_task_victory_options(font));
                    if(plan)draw_facility_ceremony(*plan,sprites,font);
                    else if(page->lifecycle!=0)throw std::runtime_error("Task victory drawing source rejected");
                } else if((page->legacy_page>=25&&page->legacy_page<=28)||page->legacy_page>=31) {
                    const auto plan=window_task_followup_plan(state,page->id,font,human_scroll_drag&&human_scroll_drag->page==page->id);
                    if(plan)std::visit([&](const auto &value){draw_facility_ceremony(value,sprites,font);},*plan);
                    else if(page->lifecycle!=0)throw std::runtime_error("Task followup drawing source rejected");
                } else if(const auto plan=window_task_selection_plan(state,page->id,font,human_scroll_drag&&human_scroll_drag->page==page->id))draw_facility_ceremony(*plan,sprites,font);
                else if(page->lifecycle!=0)throw std::runtime_error("Task selection drawing source rejected");
            } else if (steam_magic) {
                if(const auto plan=window_magic_pot_plan(state,page->id,font,human_scroll_drag&&human_scroll_drag->page==page->id))
                    draw_facility_ceremony(*plan,sprites,font);
                else if(page->lifecycle!=0)throw std::runtime_error("Magic pot drawing source rejected");
            } else if (steam_commodity) {
                const auto plan=window_commodity_plan(state,page->id,font,human_scroll_drag&&human_scroll_drag->page==page->id);
                if(!plan)throw std::runtime_error("Steam commodity skin rejected its source");
                draw_human_overview(*plan,sprites,font);
            } else if (steam_reputation) {
                const auto plan=window_facility_reputation_plan(state,page->id,font);
                if(!plan)throw std::runtime_error("Steam facility reputation source rejected");
                draw_facility_ceremony(*plan,sprites,font);
            } else if(steam_commerce_top) {
                const auto plan=window_commerce_plan(state,page->id,font);
                if(plan)draw_facility_ceremony(*plan,sprites,font);
                else if(page->lifecycle!=0)throw std::runtime_error("Commerce top drawing source rejected");
            } else if(steam_commerce_result) {
                const auto plan=window_commerce_result_plan(state,page->id,font);
                if(plan)draw_facility_ceremony(*plan,sprites,font);
                else if(page->lifecycle!=0)throw std::runtime_error("Commerce result drawing source rejected");
            } else if(steam_commerce_facility) {
                const auto plan=window_commerce_facility_plan(state,page->id,font,human_scroll_drag&&human_scroll_drag->page==page->id);
                if(plan)draw_facility_ceremony(*plan,sprites,font);
                else if(page->lifecycle!=0)throw std::runtime_error("Commerce facility drawing source rejected");
            } else if (page->legacy_page == 84) {
                DrawRectangle(8, 42, 224, 214, paper);
                const auto view = inspect_startup_world_commerce_page(state, page->id);
                font.text("南瓜商会", 16, 48);
                if (view) {
                    const int raw = view->raw;
                    {
                        const int rows = 5;
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
                            {
                                const auto icon=startup_world_item_icon_draws(state,entry);
                                if (!icon) throw std::runtime_error("商会道具图标载荷非法");
                                for (const auto &plan:*icon) sprites.visual(plan,{18,y});
                            }
                            font.text(rules.items.at(entry).name,40,y,ink,10);
                            std::string value;
                            if (view->mode == 0 && view->tab == 1)
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
                            font.text(view->mode == 0 ? "购买一件道具"
                                                        : "出售一件道具",
                                      16, 225, ink, 10);
                    }
                    if (raw <= 85)
                        font.text("返回", 18, 272);
                    font.text("确定", 190, 272);
                }
            } else if (steam_effect) {
                const auto plan=window_facility_effect_plan(state,page->id,font,human_scroll_drag&&human_scroll_drag->page==page->id);
                if(!plan)throw std::runtime_error("Steam facility effect source rejected");
                draw_facility_ceremony(*plan,sprites,font);
            } else if (page->legacy_page == 95) {
                if(const auto plan=window_town_progress_plan(state,page->id,font))draw_facility_ceremony(*plan,sprites,font);
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
                if(ordinary_facility_preview(state,*page)) {
                    const auto plan=window_facility_definition_plan(state,page->id,font);
                    if(!plan)throw std::runtime_error("Steam普通设施定义预览载荷拒绝");
                    draw_facility_detail(*plan,sprites,font);
                } else if(special_facility_page(state,*page)) {
                    const auto plan=window_facility_special_plan(state,page->id,font);
                    if(!plan)throw std::runtime_error("Steam特殊设施定义预览载荷拒绝");
                    draw_facility_detail(*plan,sprites,font);
                } else {
                // 住宅仍属旧研究显示，不作为已接正式模板的失败兜底。
                    const int definition = state.facility_definition_page_bindings.at(page->id);
                    const auto &d = rules.facilities.at(definition);
                    DrawRectangle(5, 42, 230, 216, paper);
                    const auto title_icon = startup_world_facility_icon_draw(state,d.id);
                    if (!title_icon) throw std::runtime_error("设施类别图标载荷非法");
                    sprites.visual(*title_icon,{12,48});
                    font.text(d.name, 32, 48);
                    const auto &art = display(d.display_id);
                    sprites.draw(art.sprite, 0, {65, 119});
                    if (d.detail == 6)
                        font.text("募集冒险者入住", 16, 160);
                    else {
                        const auto &values = state.scripts.facilities.at(definition).attributes;
                        constexpr const char *labels[]{"价格", "品质", "魅力"};
                        for (int n = 0; n < 3; ++n)
                            font.text(std::string(labels[n]) + " " + std::to_string(values[n]), 128,
                                      94 + n * 24, ink, 11);
                    }
                    font.text("返回", 18, 272);
                    font.text("确定", 190, 272);
                }
            } else if (page->legacy_page == 74) {
                if(startup_world_facility_page_count(state,*page)==2) {
                    const auto plan=window_facility_detail_plan(state,page->id,{bonus_selection,bonus_scroll},font);
                    if(!plan)throw std::runtime_error("Steam普通设施详情载荷拒绝");
                    draw_facility_detail(*plan,sprites,font,
                        facility_arrow_press&&facility_arrow_press->first==page->id
                            ?std::optional<int>(facility_arrow_press->second):std::nullopt,bonus_scroll,
                        facility_scroll_drag&&facility_scroll_drag->page==page->id);
                } else if(special_facility_page(state,*page)) {
                    const auto plan=window_facility_special_plan(state,page->id,font);
                    if(!plan)throw std::runtime_error("Steam特殊设施实例详情载荷拒绝");
                    draw_facility_detail(*plan,sprites,font);
                } else {
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
                    const int end = 3;
                    for (int n = start; n < end; ++n) {
                        const float y = 74 + (n - start) * 23;
                        font.text(labels[n], 16, y);
                        font.text(std::to_string(values->instance_attributes[n]), 170, y);
                    }
                    font.text("使用 " + std::to_string(growth.completed_uses) + "/" +
                                  std::to_string(values->upgrade_uses),
                              16, 150, ink, 10);
                    font.text(facility.status == 0 ? "施工中" : "营业中", 16, 172, ink, 10);
                    const auto &art = display(d.display_id);
                    sprites.draw(art.sprite, 0, {182, 218});
                    if (d.kind == 12 && state.facility_residents.at(id) >= 0)
                        font.text(human_name(state.facility_residents.at(id)), 16, 197,
                                  ink, 10);
                    font.text("返回", 18, 272);
                    if (d.detail == 6)
                        font.text("入住", 190, 272);
                    font.text(command_feedback, 12, 199, ink, 10);
                }
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
                const int start = std::max(0, residence_selection - 3);
                for (int n = start; n < static_cast<int>(list.size()) && n < start + 4; ++n) {
                    const auto &human = rules.humans.at(list[n]);
                    const int y = 65 + (n - start) * 24;
                    if (n == residence_selection)
                        DrawRectangle(12, y - 3, 216, 22, {219, 232, 204, 255});
                    font.text(human_name(human.identity), 17, y);
                    font.text(std::to_string(human.residence_fee) + "G", 178, y);
                }
                font.text("返回", 18, 272);
                font.text("入住", 190, 272);
            } else if (page->legacy_page >= 51 && page->legacy_page <= 54) {
                if(const auto plan=window_village_activity_plan(state,page->id,font,human_scroll_drag&&human_scroll_drag->page==page->id))
                    draw_facility_ceremony(*plan,sprites,font);
            } else if (page->legacy_page == 60 && startup_world_human_page_ready(state, page->id)) {
                const auto plan=window_human_overview_plan(state,page->id,font);
                if(!plan)throw std::runtime_error("Steam人物详情显示载荷拒绝");
                draw_human_overview(*plan,sprites,font);
            } else if(steam_profession) {
                const auto plan=window_human_profession_plan(state,page->id,font);
                if(!plan)throw std::runtime_error("Steam职业页面载荷拒绝");
                draw_human_overview(*plan,sprites,font);
            } else if(steam_catalogue) {
                const auto plan=window_human_catalogue_plan(state,page->id,font,human_scroll_drag&&human_scroll_drag->page==page->id);
                if(!plan)throw std::runtime_error("Steam human catalogue payload rejected");
                draw_human_overview(*plan,sprites,font);
            } else if(steam_equipment) {
                SteamHumanOverviewOptions options;options.view_y=23;
                const auto phase=state.page_phases.find(page->id),selection=state.human_page_selections.find(page->id);
                const auto catalogue=state.equipment_page_catalogs.find(page->id);
                if(phase==state.page_phases.end()||selection==state.human_page_selections.end()||catalogue==state.equipment_page_catalogs.end()||
                   phase->second<0||phase->second>3)throw std::runtime_error("装备标题载荷拒绝");
                const auto &list=catalogue->second[phase->second];
                const auto title="装备信息"+(list.size()>1?" "+std::to_string(selection->second+1)+"/"+std::to_string(list.size()):std::string{});
                options.title_widths=std::array<int,2>{int(font.measure(title)),int(font.measure(title))};
                const auto plan=steam_human_equipment_detail_skin(state,page->id,options);
                if(!plan)throw std::runtime_error("Steam装备详情载荷拒绝");
                draw_human_overview(*plan,sprites,font);
            } else if(steam_gift) {
                const auto plan=window_human_gift_confirmation_plan(state,page->id,font);
                if(!plan)throw std::runtime_error("Steam赠礼确认绘制载荷拒绝");
                draw_human_overview(*plan,sprites,font);
            } else if(steam_gift_result) {
                const auto plan=window_human_gift_result_plan(state,page->id,font);
                if(!plan)throw std::runtime_error("Steam赠礼结果载荷拒绝");
                draw_human_overview(*plan,sprites,font);
            } else if(steam_attribute_result) {
                const auto plan=window_human_attribute_result_plan(state,page->id,font);
                if(!plan)throw std::runtime_error("Steam attribute result payload rejected");
                draw_human_overview(*plan,sprites,font);
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
            } else if (page->legacy_page == 48 || page->legacy_page == 49 || page->legacy_page == 50 || page->legacy_page == 95) {
                if(const auto plan=window_town_progress_plan(state,page->id,font))draw_facility_ceremony(*plan,sprites,font);
            } else if (page->legacy_page == 88) {
                if (state.page_human_bindings.count(page->id))
                    font.text(human_name(state.page_human_bindings.at(page->id)), 17, 66);
                font.text("勋章授予",17,90);
                for (int n = 0; n < 2; ++n)
                    font.text(std::string(n == 0 ? "满足 " : "努力 ") +
                                  std::to_string(state.reward_display[0][n]) + " > " +
                                  std::to_string(state.reward_display[1][n]),
                              17, 118 + n * 24);
                font.text("确定", 190, 272);
            } else if (page->legacy_page == 83)
                font.text("取消", 18, 270);
            else if (!steam_gift_claim&&!steam_info&&!steam_task&&!steam_residence&&!steam_effect&&!steam_facility&&page->legacy_page != 97 && page->legacy_page != 72 &&
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
        const auto result = save_startup_world_file(*save_file, session.standalone(), file_metadata);
        if (!result.ok) throw std::runtime_error("保存失败：" + result.error);
        std::cout << "normal save written: " << save_file->string() << '\n';
    }
    std::cout << "world window closed: host=" << (session.application_mode()?"application":"session")
              << " frames=" << frame_count
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
