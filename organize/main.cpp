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

static SDispatchResult moveOrExec(std::string in) {
    CVarList vars(in, 0, ',');

    if (!g_pCompositor->m_lastMonitor || !g_pCompositor->m_lastMonitor->m_activeWorkspace)
        return SDispatchResult{.success = false, .error = "No active workspace"};

    const auto PWINDOW = g_pCompositor->getWindowByRegex(vars[0]);

    if (!PWINDOW)
        g_pKeybindManager->spawn(vars[1]);
    else {
        if (g_pCompositor->m_lastMonitor->m_activeWorkspace != PWINDOW->m_workspace)
            g_pCompositor->moveWindowToWorkspaceSafe(PWINDOW, g_pCompositor->m_lastMonitor->m_activeWorkspace);
        else
            g_pCompositor->warpCursorTo(PWINDOW->middle());
        g_pCompositor->focusWindow(PWINDOW);
    }

    return SDispatchResult{};
}

static SDispatchResult throwUnfocused(std::string in) {
    const auto [id, name] = getWorkspaceIDNameFromString(in);

    if (id == WORKSPACE_INVALID)
        return SDispatchResult{.success = false, .error = "Failed to find workspace"};

    if (!g_pCompositor->m_lastWindow || !g_pCompositor->m_lastWindow->m_workspace)
        return SDispatchResult{.success = false, .error = "No valid last window"};

    auto pWorkspace = g_pCompositor->getWorkspaceByID(id);
    if (!pWorkspace)
        pWorkspace = g_pCompositor->createNewWorkspace(id, g_pCompositor->m_lastWindow->m_workspace->monitorID(), name, false);

    for (const auto& w : g_pCompositor->m_windows) {
        if (w == g_pCompositor->m_lastWindow || w->m_workspace != g_pCompositor->m_lastWindow->m_workspace)
            continue;

        g_pCompositor->moveWindowToWorkspaceSafe(w, pWorkspace);
    }

    return SDispatchResult{};
}

static SDispatchResult bringAllFrom(std::string in) {
    const auto [id, name] = getWorkspaceIDNameFromString(in);

    if (id == WORKSPACE_INVALID)
        return SDispatchResult{.success = false, .error = "Failed to find workspace"};

    if (!g_pCompositor->m_lastMonitor || !g_pCompositor->m_lastMonitor->m_activeWorkspace)
        return SDispatchResult{.success = false, .error = "No active monitor"};

    auto pWorkspace = g_pCompositor->getWorkspaceByID(id);
    if (!pWorkspace)
        return SDispatchResult{.success = false, .error = "Workspace isnt open"};

    const auto PLASTWINDOW = g_pCompositor->m_lastWindow.lock();

    for (const auto& w : g_pCompositor->m_windows) {
        if (w->m_workspace != pWorkspace)
            continue;

        g_pCompositor->moveWindowToWorkspaceSafe(w, g_pCompositor->m_lastMonitor->m_activeWorkspace);
    }

    if (PLASTWINDOW) {
        g_pCompositor->focusWindow(PLASTWINDOW);
        g_pCompositor->warpCursorTo(PLASTWINDOW->middle());
    }

    return SDispatchResult{};
}

static SDispatchResult closeUnfocused(std::string in) {
    if (!g_pCompositor->m_lastMonitor)
        return SDispatchResult{.success = false, .error = "No focused monitor"};

    for (const auto& w : g_pCompositor->m_windows) {
        if (w->m_workspace != g_pCompositor->m_lastMonitor->m_activeWorkspace || w->m_monitor != g_pCompositor->m_lastMonitor || !w->m_isMapped || w == g_pCompositor->m_lastWindow)
            continue;

        g_pCompositor->closeWindow(w);
    }

    return SDispatchResult{};
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