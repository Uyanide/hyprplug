/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-04 20:26:06
 * @LastEditTime: 2025-08-04 21:36:42
 * @Description: Hyprland plugin to organize workspaces
 */
#define WLR_USE_UNSTABLE

#include <unistd.h>

#include <algorithm>
#include <hyprland/src/includes.hpp>
#include <sstream>
#include <vector>

#define private public
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/desktop/Window.hpp>
#include <hyprland/src/managers/KeybindManager.hpp>
#include <hyprland/src/render/Renderer.hpp>
#undef private

#include <hyprutils/string/VarList.hpp>
using namespace Hyprutils::String;

#include <hyprland/src/plugins/PluginAPI.hpp>

inline HANDLE PHANDLE = nullptr;

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

static SDispatchResult organizeworkspaces(std::string in) {
    std::vector<WORKSPACEID> _idsFrom;
    for (const auto& _workspace : g_pCompositor->m_workspaces) {
        // case not normal workspace
        if (_workspace->m_isSpecialWorkspace || _workspace->m_id < 0) {
            continue;
        }
        // case no windows
        if (!_workspace->getWindows()) {
            continue;
        }
        _idsFrom.push_back(_workspace->m_id);
    }
    if (_idsFrom.empty()) {
        return SDispatchResult{.success = false, .error = "No workspaces to organize"};
    }
    std::sort(_idsFrom.begin(), _idsFrom.end(), [](const auto& a, const auto& b) {
        return a < b;
    });
    WORKSPACEID _idTo = 0;
    for (const auto& _idFrom : _idsFrom) {
        _idTo++;
        if (_idFrom == _idTo) {
            continue;
        }
        const auto& _workspaceFrom = g_pCompositor->getWorkspaceByID(_idFrom);
        auto _workspaceTo          = g_pCompositor->getWorkspaceByID(_idTo);
        if (!_workspaceTo) {
            _workspaceTo = g_pCompositor->createNewWorkspace(_idTo, _workspaceFrom->m_monitor->m_id, _workspaceFrom->m_name, false);
            if (!_workspaceTo) {
                return SDispatchResult{.success = false, .error = "Failed to create workspace"};
            }
            g_pCompositor->registerWorkspace(_workspaceTo);
        }
        for (const auto _window : g_pCompositor->m_windows) {
            if (_window->m_workspace == _workspaceFrom) {
                g_pCompositor->moveWindowToWorkspaceSafe(_window, _workspaceTo);
            }
        }
    }
    return SDispatchResult{.success = true, .error = ""};
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    HyprlandAPI::addDispatcherV2(handle, "plugin:uyani:organize", ::organizeworkspaces);
    return {"hyprorganize", "Organize workspaces from 1 to N, where N is the number of workspaces with at least one window", "Uyanide", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    ;
}