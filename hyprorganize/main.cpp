/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-04 20:26:06
 * @LastEditTime: 2025-08-04 22:11:55
 * @Description: Hyprland plugin to organize workspaces
 */
#define WLR_USE_UNSTABLE

#include <unistd.h>

#include <algorithm>
#include <vector>
#include <hyprland/src/includes.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

inline HANDLE PHANDLE = nullptr;

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

static SDispatchResult organizeworkspaces(std::string in) {
    std::vector<WORKSPACEID> _idsFrom;
    for (const auto& _workspace : g_pCompositor->getWorkspaces()) {
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
    std::sort(_idsFrom.begin(), _idsFrom.end(), [](const auto& a, const auto& b) { return a < b; });
    WORKSPACEID _idTo = 0;
    for (const auto& _idFrom : _idsFrom) {
        _idTo++;
        if (_idFrom == _idTo) {
            continue;
        }
        const auto& _workspaceFrom = g_pCompositor->getWorkspaceByID(_idFrom);
        auto        _workspaceTo   = g_pCompositor->getWorkspaceByID(_idTo);
        if (!_workspaceTo) {
            if (!_workspaceFrom->m_monitor) {
                return SDispatchResult{.success = false, .error = "Workspace has no monitor"};
            }
            std::string _name = std::to_string(_idTo);
            _workspaceTo      = g_pCompositor->createNewWorkspace(_idTo, _workspaceFrom->m_monitor->m_id, _name, false);
            if (!_workspaceTo) {
                return SDispatchResult{.success = false, .error = "Failed to create workspace"};
            }
            // g_pCompositor->registerWorkspace(_workspaceTo);
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
    auto success = HyprlandAPI::addDispatcherV2(handle, "plugin:uyani:organize", ::organizeworkspaces);
    if (!success) {
        throw std::runtime_error("hyprorganize failed to register dispatcher");
    }
    return {"hyprorganize", "Organize workspaces from 1 to N, where N is the number of workspaces with at least one window", "Uyanide", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    ;
}