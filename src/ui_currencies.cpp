#include "ui_currencies.h"
#include "settings.h"
#include "item_tracker.h"
#include "ignored_items.h"
#include "localization.h"
#include "ui_context_menu.h"
#include <algorithm>
#include <cstring>

namespace UICurrencies
{
void RenderCurrenciesTab()
{
    int pendingContextId = 0;
    std::string pendingContextName;

    // Search bar
    if (ImGui::InputTextWithHint("##SearchCurrencies", Localization::GetText("search_currencies_hint"), UICommon::s_SearchBuf, sizeof(UICommon::s_SearchBuf)))
    {
        g_Settings.searchTerm = UICommon::s_SearchBuf;
        SettingsManager::Save();
    }
    ImGui::SameLine();
    char clearSearchCurrencies[256];
    snprintf(clearSearchCurrencies, sizeof(clearSearchCurrencies), "%s##ClearSearchCurrencies", Localization::GetText("clear_search"));
    if (ImGui::Button(clearSearchCurrencies))
    {
        memset(UICommon::s_SearchBuf, 0, sizeof(UICommon::s_SearchBuf));
        g_Settings.searchTerm = "";
        SettingsManager::Save();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("clear_search_tooltip"));
    ImGui::Spacing();

    // Grid View Toggle
    if (ImGui::Checkbox(Localization::GetText("enable_grid_view_currencies"), &g_Settings.enableGridViewCurrencies))
        SettingsManager::Save();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("enable_grid_view_currencies_tooltip"));
    
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    // Toggle for Favorites First
    if (ImGui::Checkbox(Localization::GetText("favorites_first"), &g_Settings.favoritesFirst))
    {
        SettingsManager::Save();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("favorites_first_tooltip"));

    ImGui::Spacing();

    // Group by Category options
    if (ImGui::Checkbox(Localization::GetText("currency_group_by_category"), &g_Settings.currencyGroupByCategory))
    {
        SettingsManager::Save();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("currency_group_by_category_tooltip"));

    ImGui::SameLine();
    if (g_Settings.currencyGroupByCategory)
    {
        if (ImGui::Checkbox(Localization::GetText("currency_show_as_tabs"), &g_Settings.currencyShowAsTabs))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("currency_show_as_tabs_tooltip"));
    }
    else
    {
        ImGui::TextDisabled("%s", Localization::GetText("currency_show_as_tabs"));
    }

    ImGui::Spacing();

    auto sortedCurrencies = ItemTracker::GetSortedCurrencies(static_cast<ItemTracker::SortMode>(g_Settings.itemSortMode));

    if (g_Settings.enableGridViewCurrencies)
    {
        // Grid View for Currencies
        float cellSize = static_cast<float>(g_Settings.gridIconSizeCurrencies) + 10.0f; // icon + padding
        ImVec2 windowSize = ImGui::GetWindowSize();
        int columns = std::clamp(static_cast<int>(windowSize.x / cellSize), 2, 50);

        if (ImGui::BeginChild("##CurrenciesGrid", ImVec2(0, 0), true))
        {
            if (g_Settings.currencyGroupByCategory)
            {
                // Group by Category logic
                std::vector<std::string> categories = {
                    Localization::GetText("currency_cat_common"),
                    Localization::GetText("currency_cat_fractal"),
                    Localization::GetText("currency_cat_raid_strike"),
                    Localization::GetText("currency_cat_wvw"),
                    Localization::GetText("currency_cat_pvp"),
                    Localization::GetText("currency_cat_map"),
                    Localization::GetText("currency_cat_janthir"),
                    Localization::GetText("currency_cat_other")
                };

                if (g_Settings.currencyShowAsTabs)
                {
                    if (ImGui::BeginTabBar("##CurrencyCategoryTabsGrid"))
                    {
                        for (const auto& cat : categories)
                        {
                            if (ImGui::BeginTabItem(cat.c_str()))
                            {
                                int col = 0;
                                for (auto& [id, st] : sortedCurrencies)
                                {
                                    if (ItemTracker::GetCurrencyCategory(id) != cat) continue;

                                    if (col > 0) ImGui::SameLine();
                                    
                                    ImGui::PushID(id);
                                    // ... Render Currency Cell (using a helper or just repeating for now)
                                    if (ImGui::BeginChild("##CurrencyCell", ImVec2(cellSize, cellSize), true, ImGuiWindowFlags_NoScrollbar))
                                    {
                                        ImVec2 cursor = ImGui::GetCursorPos();
                                        std::string iconUrl = st.details.iconUrl;
                                        if (id == 1 && iconUrl.empty()) iconUrl = "https://wiki.guildwars2.com/images/e/eb/Copper_coin.png";
                                        UICommon::DrawItemIconCell(id, iconUrl, static_cast<float>(g_Settings.gridIconSizeCurrencies), st.details.loaded ? st.details.rarity : "");
                                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                                        {
                                            pendingContextId = id;
                                            pendingContextName = st.details.loaded ? st.details.name : "";
                                        }
                                        if (ImGui::IsItemHovered() && st.details.loaded)
                                        {
                                            ImGui::BeginTooltip();
                                            ImGui::Text("%s", st.details.name.c_str());
                                            ImGui::Separator();
                                            ImVec4 tooltipCountColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                                            ImGui::TextColored(tooltipCountColor, "%s %lld", Localization::GetText("count_label"), st.count);
                                            ImGui::Separator();
                                            char currencyIdLabel[256];
                                            snprintf(currencyIdLabel, sizeof(currencyIdLabel), Localization::GetText("currency_id_label"), id);
                                            ImGui::Text("%s", currencyIdLabel);
                                            ImGui::EndTooltip();
                                        }
                                        ImVec2 iconSize = ImVec2(static_cast<float>(g_Settings.gridIconSizeCurrencies), static_cast<float>(g_Settings.gridIconSizeCurrencies));
                                        float offsetX = iconSize.x * 0.15f;
                                        float offsetY = iconSize.y * 0.15f;
                                        ImVec2 countPos = ImVec2(cursor.x + iconSize.x - ImGui::CalcTextSize("999").x - offsetX, cursor.y + iconSize.y - ImGui::CalcTextSize("999").y - offsetY);
                                        ImGui::SetCursorPos(countPos);
                                        ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                                        float fontSize = static_cast<float>(g_Settings.gridIconSizeCurrencies) * 0.4f;
                                        ImGui::PushFont(ImGui::GetFont());
                                        ImGui::SetWindowFontScale(fontSize / ImGui::GetFontSize());
                                        char countStr[32]; snprintf(countStr, sizeof(countStr), "%lld", st.count);
                                        ImVec2 shadowOffsets[] = { {-1, -1}, {1, -1}, {-1, 1}, {1, 1} };
                                        for (int s = 0; s < 4; s++) { ImGui::SetCursorPos(ImVec2(countPos.x + shadowOffsets[s].x, countPos.y + shadowOffsets[s].y)); ImGui::TextColored(ImVec4(0, 0, 0, 1), "%s", countStr); }
                                        ImGui::SetCursorPos(countPos); ImGui::TextColored(countColor, "%s", countStr);
                                        ImGui::SetWindowFontScale(1.0f); ImGui::PopFont();
                                    }
                                    ImGui::EndChild();
                                    ImGui::PopID();

                                    col++;
                                    if (col >= columns) col = 0;
                                }
                                ImGui::EndTabItem();
                            }
                        }
                        ImGui::EndTabBar();
                    }
                }
                else
                {
                    // Collapsible sections
                    for (const auto& cat : categories)
                    {
                        bool hasItems = false;
                        for (const auto& [id, st] : sortedCurrencies) { if (ItemTracker::GetCurrencyCategory(id) == cat) { hasItems = true; break; } }
                        if (!hasItems) continue;

                        if (ImGui::CollapsingHeader(cat.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            int col = 0;
                            for (auto& [id, st] : sortedCurrencies)
                            {
                                if (ItemTracker::GetCurrencyCategory(id) != cat) continue;
                                if (col > 0) ImGui::SameLine();
                                
                                ImGui::PushID(id);
                                if (ImGui::BeginChild("##CurrencyCell", ImVec2(cellSize, cellSize), true, ImGuiWindowFlags_NoScrollbar))
                                {
                                    ImVec2 cursor = ImGui::GetCursorPos();
                                    std::string iconUrl = st.details.iconUrl;
                                    if (id == 1 && iconUrl.empty()) iconUrl = "https://wiki.guildwars2.com/images/e/eb/Copper_coin.png";
                                    UICommon::DrawItemIconCell(id, iconUrl, static_cast<float>(g_Settings.gridIconSizeCurrencies), st.details.loaded ? st.details.rarity : "");
                                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                                    {
                                        pendingContextId = id;
                                        pendingContextName = st.details.loaded ? st.details.name : "";
                                    }
                                    if (ImGui::IsItemHovered() && st.details.loaded)
                                    {
                                        ImGui::BeginTooltip();
                                        ImGui::Text("%s", st.details.name.c_str());
                                        ImGui::Separator();
                                        ImVec4 tooltipCountColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                                        ImGui::TextColored(tooltipCountColor, "%s %lld", Localization::GetText("count_label"), st.count);
                                        ImGui::Separator();
                                        char currencyIdLabel[256];
                                        snprintf(currencyIdLabel, sizeof(currencyIdLabel), Localization::GetText("currency_id_label"), id);
                                        ImGui::Text("%s", currencyIdLabel);
                                        ImGui::EndTooltip();
                                    }
                                    ImVec2 iconSize = ImVec2(static_cast<float>(g_Settings.gridIconSizeCurrencies), static_cast<float>(g_Settings.gridIconSizeCurrencies));
                                    float offsetX = iconSize.x * 0.15f;
                                    float offsetY = iconSize.y * 0.15f;
                                    ImVec2 countPos = ImVec2(cursor.x + iconSize.x - ImGui::CalcTextSize("999").x - offsetX, cursor.y + iconSize.y - ImGui::CalcTextSize("999").y - offsetY);
                                    ImGui::SetCursorPos(countPos);
                                    ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                                    float fontSize = static_cast<float>(g_Settings.gridIconSizeCurrencies) * 0.4f;
                                    ImGui::PushFont(ImGui::GetFont());
                                    ImGui::SetWindowFontScale(fontSize / ImGui::GetFontSize());
                                    char countStr[32]; snprintf(countStr, sizeof(countStr), "%lld", st.count);
                                    ImVec2 shadowOffsets[] = { {-1, -1}, {1, -1}, {-1, 1}, {1, 1} };
                                    for (int s = 0; s < 4; s++) { ImGui::SetCursorPos(ImVec2(countPos.x + shadowOffsets[s].x, countPos.y + shadowOffsets[s].y)); ImGui::TextColored(ImVec4(0, 0, 0, 1), "%s", countStr); }
                                    ImGui::SetCursorPos(countPos); ImGui::TextColored(countColor, "%s", countStr);
                                    ImGui::SetWindowFontScale(1.0f); ImGui::PopFont();
                                }
                                ImGui::EndChild();
                                ImGui::PopID();

                                col++;
                                if (col >= columns) col = 0;
                            }
                        }
                    }
                }
            }
            else
            {
                // No grouping
                int col = 0;
                for (auto& [id, st] : sortedCurrencies)
                {
                    if (col > 0)
                        ImGui::SameLine();

                    ImGui::PushID(id);
                    if (ImGui::BeginChild("##CurrencyCell", ImVec2(cellSize, cellSize), true, ImGuiWindowFlags_NoScrollbar))
                    {
                        // Icon
                        ImVec2 cursor = ImGui::GetCursorPos();
                        std::string iconUrl = st.details.iconUrl;
                        if (id == 1 && iconUrl.empty())
                            iconUrl = "https://wiki.guildwars2.com/images/e/eb/Copper_coin.png";
                        UICommon::DrawItemIconCell(id, iconUrl, static_cast<float>(g_Settings.gridIconSizeCurrencies), st.details.loaded ? st.details.rarity : "");

                        // Right-click context menu
                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                        {
                            pendingContextId = id;
                            pendingContextName = st.details.loaded ? st.details.name : "";
                        }

                        // Tooltip for icon
                        if (ImGui::IsItemHovered() && st.details.loaded)
                        {
                            ImGui::BeginTooltip();
                            ImGui::Text("%s", st.details.name.c_str());
                            ImGui::Separator();
                            ImVec4 tooltipCountColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                            ImGui::TextColored(tooltipCountColor, "%s %lld", Localization::GetText("count_label"), st.count);
                            ImGui::Separator();
                            char currencyIdLabel[256];
                            snprintf(currencyIdLabel, sizeof(currencyIdLabel), Localization::GetText("currency_id_label"), id);
                            ImGui::Text("%s", currencyIdLabel);
                            ImGui::EndTooltip();
                        }

                        // Count text (bottom right, proportional to icon size)
                        ImVec2 iconSize = ImVec2(static_cast<float>(g_Settings.gridIconSizeCurrencies), static_cast<float>(g_Settings.gridIconSizeCurrencies));
                        float offsetX = iconSize.x * 0.15f;
                        float offsetY = iconSize.y * 0.15f;
                        ImVec2 countPos = ImVec2(cursor.x + iconSize.x - ImGui::CalcTextSize("999").x - offsetX, cursor.y + iconSize.y - ImGui::CalcTextSize("999").y - offsetY);
                        ImGui::SetCursorPos(countPos);

                        ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                        float fontSize = static_cast<float>(g_Settings.gridIconSizeCurrencies) * 0.4f;
                        
                        // Draw shadow/outline for better readability on light icons
                        ImGui::PushFont(ImGui::GetFont());
                        ImGui::SetWindowFontScale(fontSize / ImGui::GetFontSize());
                        
                        char countStr[32];
                        snprintf(countStr, sizeof(countStr), "%lld", st.count);
                        
                        // Simple 4-way shadow
                        ImVec2 shadowOffsets[] = { {-1, -1}, {1, -1}, {-1, 1}, {1, 1} };
                        for (int s = 0; s < 4; s++)
                        {
                            ImGui::SetCursorPos(ImVec2(countPos.x + shadowOffsets[s].x, countPos.y + shadowOffsets[s].y));
                            ImGui::TextColored(ImVec4(0, 0, 0, 1), "%s", countStr);
                        }
                        
                        ImGui::SetCursorPos(countPos);
                        ImGui::TextColored(countColor, "%s", countStr);
                        
                        ImGui::SetWindowFontScale(1.0f);
                        ImGui::PopFont();
                    }
                    ImGui::EndChild();
                    ImGui::PopID();

                    col++;
                    if (col >= columns)
                    {
                        col = 0;
                    }
                }
            }

            // Context menu popup (rendered once outside the loop)
            if (pendingContextId != 0)
                UIContextMenu::OpenContextMenu("CurrencyContextMenu", pendingContextId, pendingContextName);
            UIContextMenu::RenderCurrencyContextMenu("CurrencyContextMenu", UIContextMenu::ContextMenuType::General);
        }
        ImGui::EndChild();
    }
    else
    {
        // Table View for Currencies
        auto renderCurrencyRow = [&](int id, const Stat& st) {
            float rowH = UICommon::CalcTableRowHeight(static_cast<float>(g_Settings.iconSize));
            ImGui::TableNextRow(0, rowH);
            if (st.isFavorite && g_Settings.enableFavoriteRowColor) ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(g_Settings.favoriteRowColor[0], g_Settings.favoriteRowColor[1], g_Settings.favoriteRowColor[2], g_Settings.favoriteRowColor[3])));
            ImGui::TableSetColumnIndex(0);
            UICommon::AlignTableCellIcon(rowH, static_cast<float>(g_Settings.iconSize));
            std::string iconUrl = st.details.iconUrl;
            if (id == 1 && iconUrl.empty()) iconUrl = "https://wiki.guildwars2.com/images/e/eb/Copper_coin.png";
            UICommon::DrawItemIconCell(id, iconUrl, static_cast<float>(g_Settings.iconSize), st.details.loaded ? st.details.rarity : "");
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
            {
                pendingContextId = id;
                pendingContextName = st.details.loaded ? st.details.name : "";
            }
            if (ImGui::IsItemHovered() && st.details.loaded)
            {
                ImGui::BeginTooltip();
                ImGui::Text("%s", st.details.name.c_str());
                ImGui::Separator();
                ImVec4 tooltipCountColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                ImGui::TextColored(tooltipCountColor, "%s %lld", Localization::GetText("count_label"), st.count);
                ImGui::Separator();
                char currencyIdLabel[256];
                snprintf(currencyIdLabel, sizeof(currencyIdLabel), Localization::GetText("currency_id_label"), id);
                ImGui::Text("%s", currencyIdLabel);
                ImGui::EndTooltip();
            }
            ImGui::TableSetColumnIndex(1);
            UICommon::AlignTableCellText(rowH);
            std::string name = st.details.loaded ? st.details.name : (id == 1 ? Localization::GetText("coin") : Localization::GetText("loading"));
            if (st.isFavorite) { ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "*"); ImGui::SameLine(); }
            ImGui::Text("%s", name.c_str());
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
            {
                pendingContextId = id;
                pendingContextName = st.details.loaded ? st.details.name : "";
            }
            if (ImGui::IsItemHovered() && st.details.loaded)
            {
                ImGui::BeginTooltip();
                ImGui::Text("%s", st.details.name.c_str());
                ImGui::Separator();
                ImVec4 tooltipCountColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                ImGui::TextColored(tooltipCountColor, "%s %lld", Localization::GetText("count_label"), st.count);
                ImGui::Separator();
                char currencyIdLabel[256];
                snprintf(currencyIdLabel, sizeof(currencyIdLabel), Localization::GetText("currency_id_label"), id);
                ImGui::Text("%s", currencyIdLabel);
                ImGui::EndTooltip();
            }
            ImGui::TableSetColumnIndex(2);
            UICommon::AlignTableCellText(rowH);
            ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
            ImGui::TextColored(countColor, "%lld", st.count);
            ImGui::TableSetColumnIndex(3);
            UICommon::AlignTableCellFrame(rowH);
            if (ImGui::Checkbox(("##fav_cur_" + std::to_string(id)).c_str(), const_cast<bool*>(&st.isFavorite))) { ItemTracker::SetFavorite(id, st.isFavorite); SettingsManager::Save(); }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Localization::GetText("currency_table_favorite_tooltip"));
            ImGui::TableSetColumnIndex(4);
            UICommon::AlignTableCellFrame(rowH);
            if (ImGui::Checkbox(("##ign_cur_" + std::to_string(id)).c_str(), const_cast<bool*>(&st.isIgnored))) { if (st.isIgnored) IgnoredItemsManager::IgnoreCurrency(id); else IgnoredItemsManager::UnignoreCurrency(id); SettingsManager::Save(); }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Localization::GetText("currency_table_ignore_tooltip"));
        };

        if (g_Settings.currencyGroupByCategory)
        {
            std::vector<std::string> categories = {
                Localization::GetText("currency_cat_common"),
                Localization::GetText("currency_cat_fractal"),
                Localization::GetText("currency_cat_raid_strike"),
                Localization::GetText("currency_cat_wvw"),
                Localization::GetText("currency_cat_pvp"),
                Localization::GetText("currency_cat_map"),
                Localization::GetText("currency_cat_janthir"),
                Localization::GetText("currency_cat_other")
            };

            if (g_Settings.currencyShowAsTabs)
            {
                if (ImGui::BeginTabBar("##CurrencyCategoryTabsList"))
                {
                    for (const auto& cat : categories)
                    {
                        if (ImGui::BeginTabItem(cat.c_str()))
                        {
                            if (ImGui::BeginTable("##CurrenciesTable_v3_Tabs", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings))
                            {
                                float iconColumnWidth = (static_cast<float>(g_Settings.iconSize) + 10.0f > 70.0f) ? (static_cast<float>(g_Settings.iconSize) + 10.0f) : 70.0f;
                                ImGui::TableSetupColumn(Localization::GetText("column_icon"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, iconColumnWidth);
                                ImGui::TableSetupColumn(Localization::GetText("column_name"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 430.0f);
                                ImGui::TableSetupColumn(Localization::GetText("column_count"), ImGuiTableColumnFlags_WidthStretch);
                                ImGui::TableSetupColumn(Localization::GetText("column_favorite"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 150.0f);
                                ImGui::TableSetupColumn(Localization::GetText("column_ignore"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 180.0f);
                                ImGui::TableHeadersRow();

                                for (auto& [id, st] : sortedCurrencies)
                                {
                                    if (ItemTracker::GetCurrencyCategory(id) == cat)
                                        renderCurrencyRow(id, st);
                                }
                                ImGui::EndTable();
                            }
                            ImGui::EndTabItem();
                        }
                    }
                    ImGui::EndTabBar();
                }
            }
            else
            {
                for (const auto& cat : categories)
                {
                    bool hasItems = false;
                    for (const auto& [id, st] : sortedCurrencies) { if (ItemTracker::GetCurrencyCategory(id) == cat) { hasItems = true; break; } }
                    if (!hasItems) continue;

                    if (ImGui::CollapsingHeader(cat.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        if (ImGui::BeginTable(("##CurrenciesTable_v3_" + cat).c_str(), 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings))
                        {
                            float iconColumnWidth = (static_cast<float>(g_Settings.iconSize) + 10.0f > 70.0f) ? (static_cast<float>(g_Settings.iconSize) + 10.0f) : 70.0f;
                            ImGui::TableSetupColumn(Localization::GetText("column_icon"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, iconColumnWidth);
                            ImGui::TableSetupColumn(Localization::GetText("column_name"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 430.0f);
                            ImGui::TableSetupColumn(Localization::GetText("column_count"), ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn(Localization::GetText("column_favorite"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 150.0f);
                            ImGui::TableSetupColumn(Localization::GetText("column_ignore"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 180.0f);
                            ImGui::TableHeadersRow();

                            for (auto& [id, st] : sortedCurrencies)
                            {
                                if (ItemTracker::GetCurrencyCategory(id) == cat)
                                    renderCurrencyRow(id, st);
                            }
                            ImGui::EndTable();
                        }
                    }
                }
            }
        }
        else
        {
            // Table View for Currencies
            int currencyTableColumnCount = 5;
            if (ImGui::BeginTable("##CurrenciesTable_v3", currencyTableColumnCount, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings))
            {
                float iconColumnWidth = (static_cast<float>(g_Settings.iconSize) + 10.0f > 70.0f) ? (static_cast<float>(g_Settings.iconSize) + 10.0f) : 70.0f;
                ImGui::TableSetupColumn(Localization::GetText("column_icon"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, iconColumnWidth);
                ImGui::TableSetupColumn(Localization::GetText("column_name"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 430.0f);
                ImGui::TableSetupColumn(Localization::GetText("column_count"), ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn(Localization::GetText("column_favorite"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 150.0f);
                ImGui::TableSetupColumn(Localization::GetText("column_ignore"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 180.0f);
                ImGui::TableHeadersRow();

                for (auto& [id, st] : sortedCurrencies)
                {
                    renderCurrencyRow(id, st);
                }
                ImGui::EndTable();
            }
        }
        if (pendingContextId != 0)
            UIContextMenu::OpenContextMenu("CurrencyContextMenu", pendingContextId, pendingContextName);
        UIContextMenu::RenderCurrencyContextMenu("CurrencyContextMenu", UIContextMenu::ContextMenuType::General);
    }
}
}
