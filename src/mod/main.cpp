// src/main.cpp
#include "ll/api/LLAPI.h"
#include "ll/api/LoggerAPI.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerJoinEvent.h"
#include "ll/api/command/Command.h"
#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/actor/animation/ActorSkeletalAnimation.h"[reference:8]

#include <string>
#include <unordered_map>

// 创建一个日志记录器，用于输出插件信息
static Logger logger("DisgustPlugin");

// 存储玩家是否开启了嫌弃模式
std::unordered_map<std::string, bool> disgustModeMap;

// ------ 核心功能1: 通过Hook实现捂鼻子动画 ------
// 使用 LL_AUTO_INSTANCE_HOOK 宏自动注册一个实例函数钩子[reference:9]
// 这里假设我们要Hook的动画播放函数原型是:
// void playAnimation(ActorSkeletalAnimation const& animation);
// 注意: 实际的函数签名和标识符需要根据BDS版本查找Fake Header确定[reference:10]
LL_AUTO_INSTANCE_HOOK(
    DisgustAnimationHook,                           // Hook类型名[reference:11]
    ll::memory::HookPriority::Normal,              // Hook优先级[reference:12]
    Player,                                        // 目标类
    "??$playAnimation@V?$shared_ptr@VActorSkeletalAnimation@@@std@@@Player@@QEAAXAEBV?$shared_ptr@VActorSkeletalAnimation@@@std@@@Z", // 函数标识符(修饰名)[reference:13]
    void,                                          // 返回值类型[reference:14]
    class std::shared_ptr<ActorSkeletalAnimation> const& animation // 参数列表[reference:15]
) {
    // 这是Hook的回调函数，会在原函数执行前被调用
    void operator()(class std::shared_ptr<ActorSkeletalAnimation> const& animation) {
        // 获取当前玩家对象 (this指向Player实例)
        Player* player = (Player*)this;

        // 检查该玩家是否开启了嫌弃模式
        auto it = disgustModeMap.find(player->getRealName());
        if (it != disgustModeMap.end() && it->second) {
            // 如果开启了嫌弃模式，拦截并替换动画
            // 这里的逻辑是: 将原本要播放的动画替换为"捂鼻子"动画
            // 具体如何构造"捂鼻子"动画，需要深入研究BDS的动画系统
            // 此处仅为概念演示，实际可能需要加载自定义动画或修改现有动画参数
            
            // 例如，我们可以简单地记录日志，并阻止原动画播放
            // 注意: 如果直接return，原动画将不会被执行，达到"替换"效果
            logger.info("Player {} is in disgust mode, playing nose-covering animation instead.", player->getRealName());

            // TODO: 在这里调用播放"捂鼻子"动画的逻辑
            
            return; // 阻止原动画播放
        }

        // 如果玩家未开启嫌弃模式，则调用原函数（通过`origin`）
        origin(animation);
    }
};

// ------ 核心功能2: 注册命令和事件 ------
void registerCommands() {
    auto& commandReg = ll::command::CommandRegistrar::getInstance();
    auto& command = commandReg.getOrCreateCommand("disgust", "Toggle disgust mode");

    // 设置命令重载
    command.overload()
        .execute<[](
            CommandOrigin const& origin,
            CommandOutput& output
        ) {
            // 检查命令执行者是否为玩家
            if (origin.getOriginType() != CommandOriginType::Player) {
                output.error("This command can only be executed by a player.");
                return;
            }

            auto* player = origin.getPlayer();
            if (!player) {
                output.error("Failed to get player.");
                return;
            }

            std::string playerName = player->getRealName();
            auto it = disgustModeMap.find(playerName);

            // 切换嫌弃模式状态
            if (it != disgustModeMap.end() && it->second) {
                // 如果已开启，则关闭
                disgustModeMap[playerName] = false;
                output.success("§aDisabled disgust mode.");
            } else {
                // 如果未开启，则开启
                disgustModeMap[playerName] = true;
                output.success("§aEnabled disgust mode!");
            }
        }>;
}

// 插件入口点
extern "C" {
    MOD_EXPORT void PluginInit() {[reference:16]
        logger.info("DisgustPlugin loaded!");

        // 注册命令
        registerCommands();

        // 注册玩家加入事件，用于初始化玩家状态
        auto& eventBus = ll::event::EventBus::getInstance();
        eventBus.emplaceListener<ll::event::PlayerJoinEvent>([](ll::event::PlayerJoinEvent& ev) {
            auto* player = ev.getPlayer();
            if (player) {
                // 玩家加入时，默认关闭嫌弃模式
                disgustModeMap[player->getRealName()] = false;
                logger.info("Player {} joined, disgust mode is off by default.", player->getRealName());
            }
        });

        // 注意: Hook已经通过 LL_AUTO_INSTANCE_HOOK 宏自动注册，无需手动调用[reference:17]
    }

    // 可选：卸载时的清理工作
    MOD_EXPORT void PluginUnload() {
        logger.info("DisgustPlugin unloaded!");
        // 清理数据
        disgustModeMap.clear();
    }
}