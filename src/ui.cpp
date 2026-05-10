// ---------------------------------------------------------------------------
// ui.cpp – ImGui rendering for the Farming Tracker Nexus addon
// Refactored: Only contains Init, Shutdown, ProcessKeybind, RenderShortcut, RenderMainWindow
// ---------------------------------------------------------------------------
#include "ui.h"
#include "ui_common.h"
#include "ui_summary.h"
#include "ui_items.h"
#include "ui_currencies.h"
#include "ui_profit.h"
#include "ui_favorites.h"
#include "ui_ignored.h"
#include "ui_filter.h"
#include "ui_custom_profit.h"
#include "ui_debug.h"
#include "ui_settings.h"
#include "ui_session_history.h"
#include "ui_timeline.h"
#include "ui_mini_window.h"
#include "ui_notifications.h"
#include "shared.h"
#include "settings.h"
#include "item_tracker.h"
#include "session_history.h"
#include "drf_client.h"
#include "gw2_fetcher.h"
#include "auto_reset.h"
#include "localization.h"
#include "resource.h"
#include "custom_profit.h"
#include "ignored_items.h"
#include "../include/nexus/Nexus.h"
#include "../include/imgui/imgui.h"
#include "../include/imgui/imgui_internal.h"

void PushAccentColor()
{
    ImVec4 accentColor(g_Settings.accentColorR, g_Settings.accentColorG, g_Settings.accentColorB, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, accentColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(accentColor.x * 1.1f, accentColor.y * 1.1f, accentColor.z * 1.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(accentColor.x * 0.9f, accentColor.y * 0.9f, accentColor.z * 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, accentColor);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(accentColor.x * 1.1f, accentColor.y * 1.1f, accentColor.z * 1.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(accentColor.x * 0.9f, accentColor.y * 0.9f, accentColor.z * 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Tab, accentColor);
    ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(accentColor.x * 1.1f, accentColor.y * 1.1f, accentColor.z * 1.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(accentColor.x * 0.6f, accentColor.y * 0.6f, accentColor.z * 0.6f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, accentColor);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(accentColor.x * 1.1f, accentColor.y * 1.1f, accentColor.z * 1.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(accentColor.x * 0.9f, accentColor.y * 0.9f, accentColor.z * 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ResizeGrip, accentColor);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, ImVec4(accentColor.x * 1.1f, accentColor.y * 1.1f, accentColor.z * 1.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, ImVec4(accentColor.x * 0.9f, accentColor.y * 0.9f, accentColor.z * 0.9f, 1.0f));
}

void PopAccentColor()
{
    ImGui::PopStyleColor(15);
}

#include <string>
#include <algorithm>
#include <cstring>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Forward declaration
static void ProcessKeybind(const char* aIdentifier, bool aIsRelease);
static void RenderShortcut();

// Helper function: Loads bytes directly from the DLL
static std::vector<unsigned char> GetResourceBytes(int resourceId) {
    // Get the module handle of our own DLL
    HMODULE hMod = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCSTR)&GetResourceBytes, &hMod);
    
    if (!hMod) return {};

    // Find resource (Type 10 is RT_RCDATA)
    HRSRC hRes = FindResourceA(hMod, MAKEINTRESOURCEA(resourceId), (LPCSTR)10);
    if (!hRes) return {};

    HGLOBAL hData = LoadResource(hMod, hRes);
    if (!hData) return {};
    DWORD size = SizeofResource(hMod, hRes);
    void* pData = LockResource(hData);

    if (!pData || size == 0) return {};
    
    return std::vector<unsigned char>((unsigned char*)pData, (unsigned char*)pData + size);
}

static const char* GetMainTabIdPrefix()
{
    return "##FT_TAB_";
}

static std::vector<std::string> DefaultMainTabOrderRuntime()
{
    return {"summary", "profit", "timeline", "favorites", "ignored", "items", "currencies", "session_history", "custom_profit", "filter", "debug"};
}

static void EnsureMainTabOrderValidRuntime(std::vector<std::string>& order)
{
    const auto defaults = DefaultMainTabOrderRuntime();

    if (order.empty())
        order = defaults;

    order.erase(
        std::remove_if(order.begin(), order.end(),
            [&](const std::string& key)
            {
                return std::find(defaults.begin(), defaults.end(), key) == defaults.end();
            }),
        order.end());

    for (const auto& key : defaults)
    {
        if (std::find(order.begin(), order.end(), key) == order.end())
            order.push_back(key);
    }
}

static bool IsMainTabEnabled(const std::string& key)
{
    if (key == "summary") return true;
    if (key == "profit") return g_Settings.enableProfitTab;
    if (key == "items") return g_Settings.enableItemsTab;
    if (key == "currencies") return g_Settings.enableCurrenciesTab;
    if (key == "favorites") return g_Settings.enableFavoritesTab;
    if (key == "ignored") return g_Settings.enableIgnoredTab;
    if (key == "session_history") return g_Settings.enableSessionHistoryTab;
    if (key == "timeline") return g_Settings.enableTimelineTab;
    if (key == "custom_profit") return g_Settings.enableCustomProfit;
    if (key == "filter") return g_Settings.enableFilterTab;
    if (key == "debug") return g_Settings.enableDebugTab;
    return false;
}

static std::vector<std::string> MergeVisibleOrder(const std::vector<std::string>& currentOrder, const std::vector<std::string>& visibleOrder)
{
    std::vector<std::string> merged = currentOrder;
    EnsureMainTabOrderValidRuntime(merged);

    size_t writeIndex = 0;
    for (auto& key : merged)
    {
        if (std::find(visibleOrder.begin(), visibleOrder.end(), key) == visibleOrder.end())
            continue;
        if (writeIndex < visibleOrder.size())
            key = visibleOrder[writeIndex++];
    }

    EnsureMainTabOrderValidRuntime(merged);
    return merged;
}

static void PersistMainTabOrderFromImGui(ImGuiTabBar* tabBar)
{
    if (!tabBar) return;

    const char* prefix = GetMainTabIdPrefix();
    const size_t prefixLen = std::strlen(prefix);

    std::vector<std::string> visibleOrder;
    visibleOrder.reserve(static_cast<size_t>(tabBar->Tabs.Size));

    for (int i = 0; i < tabBar->Tabs.Size; i++)
    {
        const ImGuiTabItem* tab = &tabBar->Tabs[i];
        const char* name = tabBar->GetTabName(tab);
        if (!name) continue;
        const char* pos = std::strstr(name, prefix);
        if (!pos) continue;
        visibleOrder.emplace_back(pos + prefixLen);
    }

    static std::vector<std::string> s_LastObservedVisibleOrder;
    static int s_StableFrames = 0;

    if (visibleOrder != s_LastObservedVisibleOrder)
    {
        s_LastObservedVisibleOrder = visibleOrder;
        s_StableFrames = 0;
        return;
    }

    s_StableFrames++;
    if (s_StableFrames < 15)
        return;

    if (visibleOrder.empty())
        return;

    auto merged = MergeVisibleOrder(g_Settings.mainTabOrder, visibleOrder);
    if (merged != g_Settings.mainTabOrder)
    {
        g_Settings.mainTabOrder = merged;
        SettingsManager::Save();
    }
}

static void SafeReset()
{
    ItemTracker::SafeReset();
    const char* addonDir = APIDefs ? APIDefs->Paths_GetAddonDirectory("FarmingTracker") : nullptr;
    ItemTracker::SaveData(addonDir);
}

static void RenderMainWindow()
{
    if (!g_Settings.showMainWindow) return;

    AutoReset::Tick();

    ImGui::SetNextWindowPos(ImVec2(g_Settings.mainWindowPosX, g_Settings.mainWindowPosY), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(g_Settings.mainWindowWidth, g_Settings.mainWindowHeight), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(320, 220), ImVec2(3840, 2160));
    ImGui::SetNextWindowBgAlpha(g_Settings.mainWindowOpacity);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;
    if (g_Settings.mainWindowClickThrough)
        flags |= ImGuiWindowFlags_NoInputs;

    if (ImGui::Begin("Farming Tracker##FT_Main", &g_Settings.showMainWindow, flags))
    {
        PushAccentColor();

        // Gradient background if enabled
        if (g_Settings.enableGradientBackgrounds && !g_Settings.disableComplexVisualsOnLowPerf)
        {
            ImVec2 windowPos = ImGui::GetWindowPos();
            ImVec2 windowSize = ImGui::GetWindowSize();
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            ImVec4 topColor = ImVec4(g_Settings.gradientTopColor[0], g_Settings.gradientTopColor[1], g_Settings.gradientTopColor[2], g_Settings.gradientTopColor[3]);
            ImVec4 bottomColor = ImVec4(g_Settings.gradientBottomColor[0], g_Settings.gradientBottomColor[1], g_Settings.gradientBottomColor[2], g_Settings.gradientBottomColor[3]);

            drawList->AddRectFilledMultiColor(
                windowPos,
                ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y),
                ImGui::ColorConvertFloat4ToU32(topColor),
                ImGui::ColorConvertFloat4ToU32(topColor),
                ImGui::ColorConvertFloat4ToU32(bottomColor),
                ImGui::ColorConvertFloat4ToU32(bottomColor)
            );
        }

        // Save window position and size
        ImVec2 pos = ImGui::GetWindowPos();
        g_Settings.mainWindowPosX = pos.x;
        g_Settings.mainWindowPosY = pos.y;
        ImVec2 size = ImGui::GetWindowSize();
        g_Settings.mainWindowWidth = size.x;
        g_Settings.mainWindowHeight = size.y;

        // --- Top Bar (Session Duration & Reset) ---
        auto duration = ItemTracker::GetSessionDuration();
        ImGui::Text("%s: %s", Localization::GetText("session_duration_label_simple"), UICommon::FormatDuration(duration.count()).c_str());
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Current farming session duration");
        
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        if (ImGui::SmallButton(Localization::GetText("reset_button")))
        {
            SafeReset();
            AutoReset::OnManualReset();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(Localization::GetText("reset_tooltip"));
        
        ImGui::Spacing();

        // --- Bottom Status Bar Calculation ---
        float statusBarHeight = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y;
        float windowHeight = ImGui::GetWindowHeight();
        float windowPaddingY = ImGui::GetStyle().WindowPadding.y;
        float mainContentHeight = windowHeight - statusBarHeight - windowPaddingY * 2.0f - ImGui::GetCursorPosY();

        ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_Reorderable;
        if (g_Settings.lockTabOrder)
            tabBarFlags &= ~ImGuiTabBarFlags_Reorderable;
            
        if (ImGui::BeginTabBar("FT_Tabs", tabBarFlags))
        {
            // All tabs are wrapped in BeginChild with mainContentHeight, making the status bar outside fixed
            float tabContentHeight = mainContentHeight - ImGui::GetFrameHeightWithSpacing(); 

            std::vector<std::string> order = g_Settings.mainTabOrder;
            EnsureMainTabOrderValidRuntime(order);

            for (const auto& key : order)
            {
                if (!IsMainTabEnabled(key))
                    continue;

                const char* labelKey = nullptr;
                const char* childId = nullptr;
                int activeIndex = -1;

                if (key == "summary") { labelKey = "tab_summary"; childId = "SummaryScroll"; activeIndex = 0; }
                else if (key == "profit") { labelKey = "tab_profit"; childId = "ProfitScroll"; activeIndex = 1; }
                else if (key == "items") { labelKey = "tab_items"; childId = "ItemsScroll"; activeIndex = 2; }
                else if (key == "currencies") { labelKey = "tab_currencies"; childId = "CurrenciesScroll"; activeIndex = 3; }
                else if (key == "favorites") { labelKey = "tab_favorites"; childId = "FavoritesScroll"; activeIndex = 4; }
                else if (key == "ignored") { labelKey = "tab_ignored"; childId = "IgnoredScroll"; activeIndex = 5; }
                else if (key == "session_history") { labelKey = "tab_session_history"; childId = "SessionHistoryScroll"; activeIndex = 6; }
                else if (key == "timeline") { labelKey = "tab_timeline"; childId = "TimelineScroll"; activeIndex = 10; }
                else if (key == "custom_profit") { labelKey = "tab_custom_profit"; childId = "CustomProfitScroll"; activeIndex = 7; }
                else if (key == "debug") { labelKey = "tab_debug"; childId = "DebugScroll"; activeIndex = 8; }
                else if (key == "filter") { labelKey = "tab_filter"; childId = "FilterScroll"; activeIndex = 9; }

                if (!labelKey || !childId)
                    continue;

                char tabLabel[256];
                snprintf(tabLabel, sizeof(tabLabel), "%s%s%s", Localization::GetText(labelKey), GetMainTabIdPrefix(), key.c_str());

                if (ImGui::BeginTabItem(tabLabel))
                {
                    if (activeIndex >= 0)
                        g_Settings.activeTab = activeIndex;

                    if (ImGui::BeginChild(childId, ImVec2(0, tabContentHeight), false))
                    {
                        if (key == "summary") UISummary::RenderSummaryTab();
                        else if (key == "profit") UIProfit::RenderProfitTab();
                        else if (key == "items") UIItems::RenderItemsTab();
                        else if (key == "currencies") UICurrencies::RenderCurrenciesTab();
                        else if (key == "favorites") UIFavorites::RenderFavoritesTab();
                        else if (key == "ignored") UIIgnored::RenderIgnoredTab();
                        else if (key == "session_history")
                        {
                            if (g_Settings.enableSessionHistory)
                                UISessionHistory::RenderSessionHistoryTab();
                            else
                                ImGui::TextDisabled("%s", Localization::GetText("enable_session_history"));
                        }
                        else if (key == "timeline") UITimeline::RenderTimelineTab();
                        else if (key == "custom_profit") UICustomProfit::RenderCustomProfitTab();
                        else if (key == "filter") UIFilter::RenderFilterTab();
                        else if (key == "debug") UIDebug::RenderDebugTab();
                    }
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
            }

            ImGuiContext* ctx = ImGui::GetCurrentContext();
            PersistMainTabOrderFromImGui(ctx ? ctx->CurrentTabBar : nullptr);

            ImGui::EndTabBar();
        }

        // --- Status Bar at the Bottom ---
        ImGui::SetCursorPosY(windowHeight - statusBarHeight - windowPaddingY);
        ImGui::Separator();
        
        // Use a small horizontal group for the status bar
        ImGui::BeginGroup();
        
        // DRF Status
        DrfStatus drfStatus = DrfClient::GetStatus();
        ImGui::TextColored(UICommon::StatusColor(drfStatus), "DRF: %s", UICommon::StatusText(drfStatus));
        
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        // GW2 Status
        const char* gw2StatusText = Localization::GetText("status_unknown");
        ImVec4 gw2StatusColor = ImVec4(1.f, 1.f, 1.f, 1.f);
        switch (Gw2Fetcher::GetStatus())
        {
            case Gw2Fetcher::Gw2Status::Disconnected: gw2StatusText = Localization::GetText("status_disconnected"); gw2StatusColor = ImVec4(0.5f, 0.5f, 0.5f, 1.f); break;
            case Gw2Fetcher::Gw2Status::Connecting: gw2StatusText = Localization::GetText("status_connecting"); gw2StatusColor = ImVec4(1.0f, 0.8f, 0.0f, 1.f); break;
            case Gw2Fetcher::Gw2Status::Connected: gw2StatusText = Localization::GetText("status_connected"); gw2StatusColor = ImVec4(0.1f, 0.8f, 0.1f, 1.f); break;
            case Gw2Fetcher::Gw2Status::Error: gw2StatusText = Localization::GetText("status_error"); gw2StatusColor = ImVec4(0.8f, 0.1f, 0.1f, 1.f); break;
        }
        ImGui::TextColored(gw2StatusColor, "GW2: %s", gw2StatusText);
        
        ImGui::EndGroup();

        PopAccentColor();
        ImGui::End();
    }
}

static void RenderShortcut()
{
    ImGui::Checkbox(Localization::GetText("show_main_window"), &g_Settings.showMainWindow);
    ImGui::Checkbox(Localization::GetText("show_mini_window"), &g_Settings.showMiniWindow);
}

static void ProcessKeybind(const char* aIdentifier, bool aIsRelease)
{
    if (!aIsRelease && strcmp(aIdentifier, "FT_TOGGLE_MAIN") == 0)
    {
        g_Settings.showMainWindow = !g_Settings.showMainWindow;
        SettingsManager::Save();
    }
    if (!aIsRelease && strcmp(aIdentifier, "FT_TOGGLE_MINI") == 0)
    {
        g_Settings.showMiniWindow = !g_Settings.showMiniWindow;
        SettingsManager::Save();
    }
}

void UI::Init()
{
    if (!APIDefs) return;

    // 1. Bytes aus der Resource laden
    std::vector<unsigned char> iconBytes = GetResourceBytes(IDB_ICON_FARMINGTRACKER);

    if (!iconBytes.empty()) {
        // 2. Nexus sagen: Erstelle Textur aus diesem Speicherbereich
        APIDefs->Textures_GetOrCreateFromMemory("ICON_FT", iconBytes.data(), iconBytes.size());
        // Hover-Effekt (wir nehmen das gleiche Bild)
        APIDefs->Textures_GetOrCreateFromMemory("ICON_FT_HOVER", iconBytes.data(), iconBytes.size());
    }

    APIDefs->GUI_Register(RT_Render, RenderMainWindow);
    APIDefs->GUI_Register(RT_Render, UIMiniWindow::RenderMiniWindow);
    APIDefs->GUI_Register(RT_Render, UINotifications::Render);
    APIDefs->GUI_Register(RT_OptionsRender, UISettings::RenderOptions);

    APIDefs->QuickAccess_Add(
        "QA_FT",
        "ICON_FT",
        "ICON_FT_HOVER",
        "FT_TOGGLE_MAIN",
        "Farming Tracker");

    APIDefs->QuickAccess_AddContextMenu("QAS_FT", "QA_FT", RenderShortcut);

    APIDefs->InputBinds_RegisterWithString("FT_TOGGLE_MAIN", ProcessKeybind, g_Settings.toggleHotkey.c_str());
    APIDefs->InputBinds_RegisterWithString("FT_TOGGLE_MINI", ProcessKeybind, g_Settings.miniWindowToggleHotkey.c_str());
}

void UI::Shutdown()
{
    if (!APIDefs) return;

    APIDefs->GUI_Deregister(RenderMainWindow);
    APIDefs->GUI_Deregister(UIMiniWindow::RenderMiniWindow);
    APIDefs->GUI_Deregister(UINotifications::Render);
    APIDefs->GUI_Deregister(UISettings::RenderOptions);
    APIDefs->QuickAccess_Remove("QA_FT");
    APIDefs->QuickAccess_RemoveContextMenu("QAS_FT");
    APIDefs->InputBinds_Deregister("FT_TOGGLE_MAIN");
    APIDefs->InputBinds_Deregister("FT_TOGGLE_MINI");
}
