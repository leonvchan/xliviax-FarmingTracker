#include "ui_layout.h"

#include "../include/imgui/imgui.h"
#include "../include/imgui/imgui_internal.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "settings.h"

namespace
{
    std::string s_LayoutPath;

    bool SectionBelongsToFarmingTracker(const std::string& sectionHeader)
    {
        if (sectionHeader.find("Farming Tracker") != std::string::npos)
            return true;

        static const char* kMarkers[] = {
            "OverviewStatsTable",
            "TopItemsProfitTable",
            "TopItemsCountTable",
            "TopCurrenciesCountTable",
            "ItemsTable_v3",
            "RarityTable_v3",
            "TypeTable_",
            "FavoriteItemsTable_v3",
            "FavoriteCurrenciesTable_v3",
            "CurrenciesTable_v3",
            "IgnoredItemsTable_v3",
            "IgnoredCurrenciesTable_v3",
            "ProfitTopDrops_v2",
            "SessionHistoryTable_v2",
            "SessionDropsTable",
            "CustomProfitItemsTable",
            "CustomProfitCurrenciesTable",
            "FT_Tabs",
            "FT_Main",
            "FT_Mini",
        };

        for (const char* marker : kMarkers)
        {
            if (sectionHeader.find(marker) != std::string::npos)
                return true;
        }

        return false;
    }

    std::string FilterIniForFarmingTracker(const char* iniData, size_t iniSize)
    {
        if (!iniData || iniSize == 0)
            return {};

        std::string filtered;
        std::istringstream stream(std::string(iniData, iniSize));
        std::string line;
        bool keepSection = false;

        while (std::getline(stream, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            if (!line.empty() && line.front() == '[')
                keepSection = SectionBelongsToFarmingTracker(line);

            if (keepSection)
            {
                if (!filtered.empty())
                    filtered += '\n';
                filtered += line;
            }
        }

        if (!filtered.empty())
            filtered += '\n';

        return filtered;
    }

    void LoadFromDisk()
    {
        if (s_LayoutPath.empty())
            return;

        std::ifstream file(s_LayoutPath, std::ios::binary);
        if (!file)
            return;

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (content.empty())
            return;

        ImGui::LoadIniSettingsFromMemory(content.c_str(), content.size());
    }
}

void UILayout::Init(const char* addonDir)
{
    if (!addonDir || !addonDir[0])
        return;

    s_LayoutPath = std::string(addonDir) + "/imgui_layout.ini";

    if (g_Settings.resetTableSettings)
    {
        Clear();
        g_Settings.resetTableSettings = false;
        SettingsManager::Save();
        return;
    }

    LoadFromDisk();
}

void UILayout::Save()
{
    if (s_LayoutPath.empty() || !ImGui::GetCurrentContext())
        return;

    size_t iniSize = 0;
    const char* iniData = ImGui::SaveIniSettingsToMemory(&iniSize);
    if (!iniData || iniSize == 0)
        return;

    const std::string filtered = FilterIniForFarmingTracker(iniData, iniSize);
    if (filtered.empty())
        return;

    std::ofstream file(s_LayoutPath, std::ios::binary | std::ios::trunc);
    if (!file)
        return;

    file.write(filtered.data(), static_cast<std::streamsize>(filtered.size()));
}

void UILayout::Tick()
{
    if (s_LayoutPath.empty() || !ImGui::GetCurrentContext())
        return;

    ImGuiContext& g = *GImGui;
    static float s_LastSettingsDirtyTimer = 0.0f;

    // ImGui starts SettingsDirtyTimer (default 5s) when tables/windows change; UpdateSettings()
    // decrements it each NewFrame and saves Nexus imgui.ini when it hits zero. Mirror that here.
    const float timer = g.SettingsDirtyTimer;
    if (s_LastSettingsDirtyTimer > 0.0f && timer <= 0.0f)
        Save();

    s_LastSettingsDirtyTimer = timer;
}

void UILayout::Clear()
{
    if (s_LayoutPath.empty())
        return;

    std::remove(s_LayoutPath.c_str());
}
