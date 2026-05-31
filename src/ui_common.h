#pragma once

#include "settings.h"
#include "item_tracker.h"
#include "drf_client.h"
#include "localization.h"
#include "../include/imgui/imgui.h"
#include <string>
#include <vector>
#include <utility>

namespace UICommon
{
    // Shared UI state variables
    extern char s_SearchBuf[256];
    extern bool s_ShowMainWindow;
    extern char s_AccountNameBuf[128];
    extern char s_AccountDrfBuf[512];
    extern char s_AccountGw2Buf[512];
    extern char s_NewProfileNameBuf[128];

    // Icon disk-cache
    void InitIconCache(const char* addonDir);
    void EnforceIconCacheLimit();

    void EnsureItemIconTexture(int itemId, const std::string& url);
    void DrawItemIconCell(int itemId, const std::string& url, float sz, const std::string& rarity = "");
    const char* StatusText(DrfStatus s);
    ImVec4 StatusColor(DrfStatus s);
    std::string FormatCoin(long long copper);
    void DrawCoinDisplay(long long copper);
    std::string FormatDuration(long long seconds);
    void TextWithTooltip(const char* text, float maxWidth = 200.0f, const ImVec4& color = ImVec4(1.f, 1.f, 1.f, 1.f));
    bool PassesRarityFilter(const Stat& st);
    void SortVisible(std::vector<std::pair<int, Stat>>& v);
    float CalcTableRowHeight(float contentHeight);
    void AlignTableCell(float rowHeight, float itemHeight);
    void AlignTableCellText(float rowHeight);
    void AlignTableCellFrame(float rowHeight);
    void AlignTableCellIcon(float rowHeight, float iconSize);

    // Data tables: first load auto-fits fixed columns to content; saved widths come from imgui_layout.ini.
    ImGuiTableFlags DataTableFlags();
    void TableColumnFixedAuto(const char* label, ImGuiTableColumnFlags extraFlags = ImGuiTableColumnFlags_None);
    void TableColumnStretchAuto(const char* label, ImGuiTableColumnFlags extraFlags = ImGuiTableColumnFlags_None);
}
