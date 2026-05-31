#include "ui_items.h"
#include "settings.h"
#include "item_tracker.h"
#include "ignored_items.h"
#include "localization.h"
#include "ui_context_menu.h"
#include <algorithm>
#include <cstring>

namespace
{
    void SetupStandardItemTableColumns()
    {
        UICommon::TableColumnFixedAuto(Localization::GetText("column_icon"), ImGuiTableColumnFlags_NoHide);
        UICommon::TableColumnFixedAuto(Localization::GetText("column_name"), ImGuiTableColumnFlags_NoHide);
        UICommon::TableColumnFixedAuto(Localization::GetText("column_count"), ImGuiTableColumnFlags_NoHide);
        UICommon::TableColumnFixedAuto(Localization::GetText("column_profit"), ImGuiTableColumnFlags_NoHide);
        UICommon::TableColumnStretchAuto(Localization::GetText("magic_find"));
        UICommon::TableColumnFixedAuto(Localization::GetText("column_favorite"), ImGuiTableColumnFlags_NoHide);
        UICommon::TableColumnFixedAuto(Localization::GetText("column_ignore"), ImGuiTableColumnFlags_NoHide);
    }

    void SetupCompactItemTableColumns()
    {
        UICommon::TableColumnFixedAuto(Localization::GetText("column_icon"), ImGuiTableColumnFlags_NoHide);
        UICommon::TableColumnStretchAuto(Localization::GetText("column_name"), ImGuiTableColumnFlags_NoHide);
        UICommon::TableColumnFixedAuto(Localization::GetText("column_count"), ImGuiTableColumnFlags_NoHide);
        UICommon::TableColumnFixedAuto(Localization::GetText("column_profit"), ImGuiTableColumnFlags_NoHide);
        UICommon::TableColumnFixedAuto(Localization::GetText("magic_find"), ImGuiTableColumnFlags_NoHide);
        UICommon::TableColumnFixedAuto(Localization::GetText("column_favorite"), ImGuiTableColumnFlags_NoHide);
        UICommon::TableColumnFixedAuto(Localization::GetText("column_ignore"), ImGuiTableColumnFlags_NoHide);
    }
}

namespace UIItems
{
void RenderItemsTab()
{
    const char* sortLabels[] = {
        Localization::GetText("sort_price_down"),
        Localization::GetText("sort_price_up"),
        Localization::GetText("sort_count_high"),
        Localization::GetText("sort_count_low"),
        Localization::GetText("sort_name_az"),
        Localization::GetText("sort_name_za"),
        Localization::GetText("sort_profit_high"),
        Localization::GetText("sort_profit_low"),
        Localization::GetText("sort_rarity_high"),
        Localization::GetText("sort_rarity_low")
    };
    if (ImGui::Combo("##SortItems", &g_Settings.itemSortMode, sortLabels, 10))
        SettingsManager::Save();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("sort_tooltip"));

    ImGui::SameLine();

    const char* rarityLabels[] = {
        Localization::GetText("rarity_all"),
        Localization::GetText("rarity_basic"),
        Localization::GetText("rarity_fine"),
        Localization::GetText("rarity_masterwork"),
        Localization::GetText("rarity_rare"),
        Localization::GetText("rarity_exotic"),
        Localization::GetText("rarity_ascended"),
        Localization::GetText("rarity_legendary")
    };
    int rarityCombo = std::clamp(g_Settings.itemRarityFilterMin, 0, 7);
    if (ImGui::Combo("##RarityF", &rarityCombo, rarityLabels, 8))
    {
        g_Settings.itemRarityFilterMin = rarityCombo;
        SettingsManager::Save();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("rarity_tooltip"));

    ImGui::Spacing();

    // Search bar and Mass Actions
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 140.0f);
    if (ImGui::InputTextWithHint("##Search", Localization::GetText("search_items_hint"), UICommon::s_SearchBuf, sizeof(UICommon::s_SearchBuf)))
    {
        g_Settings.searchTerm = UICommon::s_SearchBuf;
        SettingsManager::Save();
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();

    if (ImGui::Button(Localization::GetText("mass_actions_label"), ImVec2(130, 0)))
        ImGui::OpenPopup("MassActionsPopup");

    if (ImGui::BeginPopup("MassActionsPopup"))
    {
        auto items = ItemTracker::GetItemsCopy();
        
        auto renderMenuItem = [&](const char* labelKey, const std::string& rarityName) {
            if (ImGui::MenuItem(Localization::GetText(labelKey)))
            {
                for (const auto& [id, st] : items)
                {
                    if (st.details.rarity == rarityName)
                    {
                        IgnoredItemsManager::IgnoreItem(id);
                    }
                }
                SettingsManager::Save();
            }
        };

        // Rarity options ascending (Junk to Legendary)
        renderMenuItem("mass_actions_ignore_junk", "Junk");
        renderMenuItem("mass_actions_ignore_basic", "Basic");
        renderMenuItem("mass_actions_ignore_fine", "Fine");
        renderMenuItem("mass_actions_ignore_masterwork", "Masterwork");
        renderMenuItem("mass_actions_ignore_rare", "Rare");
        renderMenuItem("mass_actions_ignore_exotic", "Exotic");
        renderMenuItem("mass_actions_ignore_ascended", "Ascended");
        renderMenuItem("mass_actions_ignore_legendary", "Legendary");

        ImGui::Separator();

        if (ImGui::MenuItem(Localization::GetText("mass_actions_clear_ignore")))
        {
            IgnoredItemsManager::ClearAll();
            SettingsManager::Save();
        }
        ImGui::EndPopup();
    }

    ImGui::Spacing();

    // Grid View Toggle
    if (ImGui::Checkbox(Localization::GetText("enable_grid_view_items"), &g_Settings.enableGridViewItems))
        SettingsManager::Save();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("enable_grid_view_items_tooltip"));

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

    auto sortedItems = ItemTracker::GetSortedItems(static_cast<ItemTracker::SortMode>(g_Settings.itemSortMode));

    if (g_Settings.enableGridViewItems)
    {
        // Grid View
        float cellSize = static_cast<float>(g_Settings.gridIconSize) + 10.0f; // icon + padding
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float scrollbarWidth = 20.0f; // Safer buffer for scrollbar
        
        auto getColumns = [&](float width) {
            return std::max(1, static_cast<int>((width - scrollbarWidth + spacing) / (cellSize + spacing)));
        };

        // Group by Rarity options
        if (ImGui::Checkbox(Localization::GetText("group_by_rarity"), &g_Settings.groupByRarity))
        {
            if (g_Settings.groupByRarity) g_Settings.groupByType = false;
            SettingsManager::Save();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", "Group items by rarity with collapsible sections or tabs");

        ImGui::SameLine();
        if (g_Settings.groupByRarity)
        {
            if (ImGui::Checkbox(Localization::GetText("show_rarity_as_tabs"), &g_Settings.showRarityAsTabs))
                SettingsManager::Save();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", "Show rarity groups as tabs instead of collapsible sections");
        }
        else
        {
            ImGui::TextDisabled("%s", Localization::GetText("show_rarity_as_tabs"));
        }

        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();

        // Group by Category options
        if (ImGui::Checkbox(Localization::GetText("group_by_type"), &g_Settings.groupByType))
        {
            if (g_Settings.groupByType) g_Settings.groupByRarity = false;
            SettingsManager::Save();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", "Group items by category (type) with collapsible sections or tabs");

        ImGui::SameLine();
        if (g_Settings.groupByType)
        {
            if (ImGui::Checkbox(Localization::GetText("show_type_as_tabs"), &g_Settings.showTypeAsTabs))
                SettingsManager::Save();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", "Show category groups as tabs instead of collapsible sections");
        }
        else
        {
            ImGui::TextDisabled("%s", Localization::GetText("show_type_as_tabs"));
        }

        ImGui::Spacing();

        static int s_GridPendingId = -1;
        static std::string s_GridPendingName;

        if (ImGui::BeginChild("##ItemsGrid", ImVec2(0, 0), true))
        {
            if (g_Settings.groupByRarity)
            {
                // Rarity order (highest to lowest)
                std::vector<std::string> rarityOrder = {
                    Localization::GetText("rarity_name_legendary"),
                    Localization::GetText("rarity_name_ascended"),
                    Localization::GetText("rarity_name_exotic"),
                    Localization::GetText("rarity_name_rare"),
                    Localization::GetText("rarity_name_masterwork"),
                    Localization::GetText("rarity_name_fine"),
                    Localization::GetText("rarity_name_basic"),
                    Localization::GetText("rarity_name_junk"),
                    Localization::GetText("rarity_name_unknown")
                };

                // Group items by localized rarity name
                std::map<std::string, std::vector<std::pair<int, Stat>>> rarityGroups;
                for (auto& [id, st] : sortedItems)
                {
                    std::string apiRarity = st.details.loaded ? st.details.rarity : "Unknown";
                    std::string localizedRarity;
                    if (apiRarity == "Legendary") localizedRarity = Localization::GetText("rarity_name_legendary");
                    else if (apiRarity == "Ascended") localizedRarity = Localization::GetText("rarity_name_ascended");
                    else if (apiRarity == "Exotic") localizedRarity = Localization::GetText("rarity_name_exotic");
                    else if (apiRarity == "Rare") localizedRarity = Localization::GetText("rarity_name_rare");
                    else if (apiRarity == "Masterwork") localizedRarity = Localization::GetText("rarity_name_masterwork");
                    else if (apiRarity == "Fine") localizedRarity = Localization::GetText("rarity_name_fine");
                    else if (apiRarity == "Basic") localizedRarity = Localization::GetText("rarity_name_basic");
                    else if (apiRarity == "Junk") localizedRarity = Localization::GetText("rarity_name_junk");
                    else localizedRarity = Localization::GetText("rarity_name_unknown");

                    rarityGroups[localizedRarity].push_back({id, st});
                }

                if (g_Settings.showRarityAsTabs)
                {
                    static int selectedRarityTab = 0;
                    if (ImGui::BeginTabBar("##RarityTabs"))
                    {
                        int tabIndex = 0;
                        for (const auto& rarityName : rarityOrder)
                        {
                            auto it = rarityGroups.find(rarityName);
                            if (it == rarityGroups.end() || it->second.empty()) continue;

                            bool isSelected = (selectedRarityTab == tabIndex);
                            if (ImGui::BeginTabItem(rarityName.c_str(), &isSelected))
                            {
                                selectedRarityTab = tabIndex;
                                int columns = getColumns(ImGui::GetContentRegionAvail().x);
                                int col = 0;
                                for (auto& [id, st] : it->second)
                                {
                                    if (col > 0) ImGui::SameLine();
                                    ImGui::PushID(id);
                                    if (ImGui::BeginChild("##ItemCell", ImVec2(cellSize, cellSize), true, ImGuiWindowFlags_NoScrollbar))
                                    {
                                        ImVec2 cursor = ImGui::GetCursorPos();
                                        char iconButtonId[64];
                                        snprintf(iconButtonId, sizeof(iconButtonId), "##IconBtn_%d", id);
                                        if (ImGui::InvisibleButton(iconButtonId, ImVec2(static_cast<float>(g_Settings.gridIconSize), static_cast<float>(g_Settings.gridIconSize)))) {}
                                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
                                            s_GridPendingId = id;
                                            s_GridPendingName = st.details.loaded ? st.details.name : "";
                                        }
                                        ImGui::SetCursorPos(cursor);
                                        UICommon::DrawItemIconCell(id, st.details.iconUrl, static_cast<float>(g_Settings.gridIconSize), st.details.loaded ? st.details.rarity : "");
                                        if (ImGui::IsItemHovered() && st.details.loaded) {
                                            ImGui::BeginTooltip();
                                            ImGui::Text("%s", st.details.name.c_str());
                                            ImGui::Separator();
                                            ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                                            ImGui::TextColored(countColor, "%s %lld", Localization::GetText("count_label"), st.count);
                                            long long profit = ItemTracker::GetStatProfit(st);
                                            if (profit > 0) ImGui::TextColored(ImVec4(1.f, 0.84f, 0.f, 1.f), "%s %s", Localization::GetText("profit_label"), UICommon::FormatCoin(profit).c_str());
                                            else if (profit < 0) ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.f), "%s %s", Localization::GetText("profit_label"), UICommon::FormatCoin(profit).c_str());
                                            ImGui::Separator();
                                            char rarityLabel[256];
                                            snprintf(rarityLabel, sizeof(rarityLabel), Localization::GetText("rarity_label"), st.details.rarity.c_str());
                                            ImGui::Text("%s", rarityLabel);
                                            ImGui::EndTooltip();
                                        }
                                        ImVec2 iconSize = ImVec2(static_cast<float>(g_Settings.gridIconSize), static_cast<float>(g_Settings.gridIconSize));
                                        char countStr[32];
                                        snprintf(countStr, sizeof(countStr), "%lld", st.count);
                                        
                                        float fontSize = static_cast<float>(g_Settings.gridIconSize) * 0.45f; // Increased from 0.4f
                                        ImGui::PushFont(ImGui::GetFont());
                                        ImGui::SetWindowFontScale(fontSize / ImGui::GetFontSize());
                                        
                                        ImVec2 textSize = ImGui::CalcTextSize(countStr);
                                        ImVec2 countPos = ImVec2(cursor.x + iconSize.x - textSize.x - 2.0f, cursor.y + iconSize.y - textSize.y - 2.0f);
                                        
                                        ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                                        
                                        // Draw shadow/outline for better readability
                                        ImVec2 shadowOffsets[] = { {-1, -1}, {1, -1}, {-1, 1}, {1, 1}, {0, -1}, {0, 1}, {-1, 0}, {1, 0} };
                                        for (int s = 0; s < 8; s++) {
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
                                    if (col >= columns) col = 0;
                                }
                                ImGui::EndTabItem();
                            }
                            tabIndex++;
                        }
                        ImGui::EndTabBar();
                    }
                }
                else
                {
                    for (const auto& rarityName : rarityOrder)
                    {
                        auto it = rarityGroups.find(rarityName);
                        if (it == rarityGroups.end() || it->second.empty()) continue;
                        ImGui::Spacing();
                        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 0.5f));
                        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.5f));
                        if (ImGui::CollapsingHeader(rarityName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::PopStyleColor(2);
                            int columns = getColumns(ImGui::GetContentRegionAvail().x);
                            int col = 0;
                            for (auto& [id, st] : it->second)
                            {
                                if (col > 0) ImGui::SameLine();
                                ImGui::PushID(id);
                                if (ImGui::BeginChild("##ItemCell", ImVec2(cellSize, cellSize), true, ImGuiWindowFlags_NoScrollbar))
                                {
                                    ImVec2 cursor = ImGui::GetCursorPos();
                                    char iconButtonId[64];
                                    snprintf(iconButtonId, sizeof(iconButtonId), "##IconBtn_%d", id);
                                    if (ImGui::InvisibleButton(iconButtonId, ImVec2(static_cast<float>(g_Settings.gridIconSize), static_cast<float>(g_Settings.gridIconSize)))) {}
                                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
                                        s_GridPendingId = id;
                                        s_GridPendingName = st.details.loaded ? st.details.name : "";
                                    }
                                    ImGui::SetCursorPos(cursor);
                                    UICommon::DrawItemIconCell(id, st.details.iconUrl, static_cast<float>(g_Settings.gridIconSize), st.details.loaded ? st.details.rarity : "");
                                    if (ImGui::IsItemHovered() && st.details.loaded) {
                                        ImGui::BeginTooltip();
                                        ImGui::Text("%s", st.details.name.c_str());
                                        ImGui::Separator();
                                        ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                                        ImGui::TextColored(countColor, "%s %lld", Localization::GetText("count_label"), st.count);
                                        long long profit = ItemTracker::GetStatProfit(st);
                                        if (profit > 0) ImGui::TextColored(ImVec4(1.f, 0.84f, 0.f, 1.f), "%s %s", Localization::GetText("profit_label"), UICommon::FormatCoin(profit).c_str());
                                        else if (profit < 0) ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.f), "%s %s", Localization::GetText("profit_label"), UICommon::FormatCoin(profit).c_str());
                                        ImGui::Separator();
                                        char rarityLabel[256];
                                        snprintf(rarityLabel, sizeof(rarityLabel), Localization::GetText("rarity_label"), st.details.rarity.c_str());
                                        ImGui::Text("%s", rarityLabel);
                                        ImGui::EndTooltip();
                                    }
                                    ImVec2 iconSize = ImVec2(static_cast<float>(g_Settings.gridIconSize), static_cast<float>(g_Settings.gridIconSize));
                                    ImVec2 countPos = ImVec2(cursor.x + iconSize.x - ImGui::CalcTextSize("999").x - iconSize.x*0.15f, cursor.y + iconSize.y - ImGui::CalcTextSize("999").y - iconSize.y*0.15f);
                                    ImGui::SetCursorPos(countPos);
                                    ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                                    float fontSize = static_cast<float>(g_Settings.gridIconSize) * 0.4f;
                                    ImGui::PushFont(ImGui::GetFont());
                                    ImGui::SetWindowFontScale(fontSize / ImGui::GetFontSize());
                                    char countStr[32];
                                    snprintf(countStr, sizeof(countStr), "%lld", st.count);
                                    ImVec2 shadowOffsets[] = { {-1, -1}, {1, -1}, {-1, 1}, {1, 1} };
                                    for (int s = 0; s < 4; s++) {
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
                                if (col >= columns) col = 0;
                            }
                        }
                        else { ImGui::PopStyleColor(2); }
                    }
                }
            }
            else if (g_Settings.groupByType)
            {
                // Define ItemType order (logical sorting)
                std::vector<ItemType> typeOrder = {
                    ItemType::Weapon, ItemType::Armor, ItemType::Trinket, ItemType::Backpack,
                    ItemType::CraftingMaterial, ItemType::Consumable, ItemType::Container, ItemType::Bag,
                    ItemType::UpgradeComponent, ItemType::Trophy, ItemType::Gizmo, ItemType::Tool,
                    ItemType::GatheringTool, ItemType::MiniPet, ItemType::Unlock, ItemType::Unknown
                };

                // Helper to get localized type name
                auto getTypeName = [](ItemType type) -> std::string {
                    switch (type) {
                        case ItemType::Armor: return Localization::GetText("type_armor");
                        case ItemType::Weapon: return Localization::GetText("type_weapon");
                        case ItemType::Trinket: return Localization::GetText("type_trinket");
                        case ItemType::Gizmo: return Localization::GetText("type_gizmo");
                        case ItemType::CraftingMaterial: return Localization::GetText("type_crafting_material");
                        case ItemType::Consumable: return Localization::GetText("type_consumable");
                        case ItemType::GatheringTool: return Localization::GetText("type_gathering_tool");
                        case ItemType::Bag: return Localization::GetText("type_bag");
                        case ItemType::Container: return Localization::GetText("type_container");
                        case ItemType::MiniPet: return Localization::GetText("type_mini_pet");
                        case ItemType::GizmoContainer: return Localization::GetText("type_gizmo_container");
                        case ItemType::Backpack: return Localization::GetText("type_backpack");
                        case ItemType::UpgradeComponent: return Localization::GetText("type_upgrade_component");
                        case ItemType::Tool: return Localization::GetText("type_tool");
                        case ItemType::Trophy: return Localization::GetText("type_trophy");
                        case ItemType::Unlock: return Localization::GetText("type_unlock");
                        default: return Localization::GetText("rarity_name_unknown");
                    }
                };

                // Group items by type
                std::map<ItemType, std::vector<std::pair<int, Stat>>> typeGroups;
                for (auto& [id, st] : sortedItems) {
                    typeGroups[st.details.itemType].push_back({id, st});
                }

                if (g_Settings.showTypeAsTabs)
                {
                    static int selectedTypeTab = 0;
                    if (ImGui::BeginTabBar("##TypeTabs"))
                    {
                        int tabIndex = 0;
                        for (auto type : typeOrder)
                        {
                            auto it = typeGroups.find(type);
                            if (it == typeGroups.end() || it->second.empty()) continue;

                            std::string typeName = getTypeName(type);
                            bool isSelected = (selectedTypeTab == tabIndex);
                            if (ImGui::BeginTabItem(typeName.c_str(), &isSelected))
                            {
                                selectedTypeTab = tabIndex;
                                int columns = getColumns(ImGui::GetContentRegionAvail().x);
                                int col = 0;
                                for (auto& [id, st] : it->second)
                                {
                                    if (col > 0) ImGui::SameLine();
                                    ImGui::PushID(id);
                                    if (ImGui::BeginChild("##ItemCell", ImVec2(cellSize, cellSize), true, ImGuiWindowFlags_NoScrollbar))
                                    {
                                        ImVec2 cursor = ImGui::GetCursorPos();
                                        char iconButtonId[64];
                                        snprintf(iconButtonId, sizeof(iconButtonId), "##IconBtn_%d", id);
                                        if (ImGui::InvisibleButton(iconButtonId, ImVec2(static_cast<float>(g_Settings.gridIconSize), static_cast<float>(g_Settings.gridIconSize)))) {}
                                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
                                            s_GridPendingId = id;
                                            s_GridPendingName = st.details.loaded ? st.details.name : "";
                                        }
                                        ImGui::SetCursorPos(cursor);
                                        UICommon::DrawItemIconCell(id, st.details.iconUrl, static_cast<float>(g_Settings.gridIconSize), st.details.loaded ? st.details.rarity : "");
                                        if (ImGui::IsItemHovered() && st.details.loaded) {
                                            ImGui::BeginTooltip();
                                            ImGui::Text("%s", st.details.name.c_str());
                                            ImGui::Separator();
                                            ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                                            ImGui::TextColored(countColor, "%s %lld", Localization::GetText("count_label"), st.count);
                                            long long profit = ItemTracker::GetStatProfit(st);
                                            if (profit > 0) ImGui::TextColored(ImVec4(1.f, 0.84f, 0.f, 1.f), "%s %s", Localization::GetText("profit_label"), UICommon::FormatCoin(profit).c_str());
                                            else if (profit < 0) ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.f), "%s %s", Localization::GetText("profit_label"), UICommon::FormatCoin(profit).c_str());
                                            ImGui::Separator();
                                            char rarityLabel[256];
                                            snprintf(rarityLabel, sizeof(rarityLabel), Localization::GetText("rarity_label"), st.details.rarity.c_str());
                                            ImGui::Text("%s", rarityLabel);
                                            ImGui::EndTooltip();
                                        }
                                        ImVec2 iconSize = ImVec2(static_cast<float>(g_Settings.gridIconSize), static_cast<float>(g_Settings.gridIconSize));
                                        ImVec2 countPos = ImVec2(cursor.x + iconSize.x - ImGui::CalcTextSize("999").x - iconSize.x*0.15f, cursor.y + iconSize.y - ImGui::CalcTextSize("999").y - iconSize.y*0.15f);
                                        ImGui::SetCursorPos(countPos);
                                        ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                                        float fontSize = static_cast<float>(g_Settings.gridIconSize) * 0.4f;
                                        ImGui::PushFont(ImGui::GetFont());
                                        ImGui::SetWindowFontScale(fontSize / ImGui::GetFontSize());
                                        char countStr[32];
                                        snprintf(countStr, sizeof(countStr), "%lld", st.count);
                                        ImVec2 shadowOffsets[] = { {-1, -1}, {1, -1}, {-1, 1}, {1, 1} };
                                        for (int s = 0; s < 4; s++) {
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
                                    if (col >= columns) col = 0;
                                }
                                ImGui::EndTabItem();
                            }
                            tabIndex++;
                        }
                        ImGui::EndTabBar();
                    }
                }
                else
                {
                    for (auto type : typeOrder)
                    {
                        auto it = typeGroups.find(type);
                        if (it == typeGroups.end() || it->second.empty()) continue;
                        ImGui::Spacing();
                        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 0.5f));
                        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.5f));
                        if (ImGui::CollapsingHeader(getTypeName(type).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::PopStyleColor(2);
                            int columns = getColumns(ImGui::GetContentRegionAvail().x);
                            int col = 0;
                            for (auto& [id, st] : it->second)
                            {
                                if (col > 0) ImGui::SameLine();
                                ImGui::PushID(id);
                                if (ImGui::BeginChild("##ItemCell", ImVec2(cellSize, cellSize), true, ImGuiWindowFlags_NoScrollbar))
                                {
                                    ImVec2 cursor = ImGui::GetCursorPos();
                                    char iconButtonId[64];
                                    snprintf(iconButtonId, sizeof(iconButtonId), "##IconBtn_%d", id);
                                    if (ImGui::InvisibleButton(iconButtonId, ImVec2(static_cast<float>(g_Settings.gridIconSize), static_cast<float>(g_Settings.gridIconSize)))) {}
                                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
                                        s_GridPendingId = id;
                                        s_GridPendingName = st.details.loaded ? st.details.name : "";
                                    }
                                    ImGui::SetCursorPos(cursor);
                                    UICommon::DrawItemIconCell(id, st.details.iconUrl, static_cast<float>(g_Settings.gridIconSize), st.details.loaded ? st.details.rarity : "");
                                    if (ImGui::IsItemHovered() && st.details.loaded) {
                                        ImGui::BeginTooltip();
                                        ImGui::Text("%s", st.details.name.c_str());
                                        ImGui::Separator();
                                        ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                                        ImGui::TextColored(countColor, "%s %lld", Localization::GetText("count_label"), st.count);
                                        long long profit = ItemTracker::GetStatProfit(st);
                                        if (profit > 0) ImGui::TextColored(ImVec4(1.f, 0.84f, 0.f, 1.f), "%s %s", Localization::GetText("profit_label"), UICommon::FormatCoin(profit).c_str());
                                        else if (profit < 0) ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.f), "%s %s", Localization::GetText("profit_label"), UICommon::FormatCoin(profit).c_str());
                                        ImGui::Separator();
                                        char rarityLabel[256];
                                        snprintf(rarityLabel, sizeof(rarityLabel), Localization::GetText("rarity_label"), st.details.rarity.c_str());
                                        ImGui::Text("%s", rarityLabel);
                                        ImGui::EndTooltip();
                                    }
                                    ImVec2 iconSize = ImVec2(static_cast<float>(g_Settings.gridIconSize), static_cast<float>(g_Settings.gridIconSize));
                                    ImVec2 countPos = ImVec2(cursor.x + iconSize.x - ImGui::CalcTextSize("999").x - iconSize.x*0.15f, cursor.y + iconSize.y - ImGui::CalcTextSize("999").y - iconSize.y*0.15f);
                                    ImGui::SetCursorPos(countPos);
                                    ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                                    float fontSize = static_cast<float>(g_Settings.gridIconSize) * 0.4f;
                                    ImGui::PushFont(ImGui::GetFont());
                                    ImGui::SetWindowFontScale(fontSize / ImGui::GetFontSize());
                                    char countStr[32];
                                    snprintf(countStr, sizeof(countStr), "%lld", st.count);
                                    ImVec2 shadowOffsets[] = { {-1, -1}, {1, -1}, {-1, 1}, {1, 1} };
                                    for (int s = 0; s < 4; s++) {
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
                                if (col >= columns) col = 0;
                            }
                        }
                        else { ImGui::PopStyleColor(2); }
                    }
                }
            }
            else
            {
                // Normal grid view without grouping
                int columns = getColumns(ImGui::GetContentRegionAvail().x);
                int col = 0;
                for (auto& [id, st] : sortedItems)
                {
                    if (col > 0)
                        ImGui::SameLine();

                    ImGui::PushID(id);
                    if (ImGui::BeginChild("##ItemCell", ImVec2(cellSize, cellSize), true, ImGuiWindowFlags_NoScrollbar))
                    {
                        // Icon as button for right-click
                        ImVec2 cursor = ImGui::GetCursorPos();
                        char iconButtonId[64];
                        snprintf(iconButtonId, sizeof(iconButtonId), "##IconBtn_%d", id);
                        if (ImGui::InvisibleButton(iconButtonId, ImVec2(static_cast<float>(g_Settings.gridIconSize), static_cast<float>(g_Settings.gridIconSize))))
                        {
                            // Left click - could add functionality here
                        }
                        
                        // Right-click context menu
                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                        {
                            s_GridPendingId = id;
                            s_GridPendingName = st.details.loaded ? st.details.name : "";
                        }
                        
                        // Draw icon on top of button
                        ImGui::SetCursorPos(cursor);
                        UICommon::DrawItemIconCell(id, st.details.iconUrl, static_cast<float>(g_Settings.gridIconSize), st.details.loaded ? st.details.rarity : "");

                        // Tooltip for icon
                        if (ImGui::IsItemHovered() && st.details.loaded)
                        {
                            ImGui::BeginTooltip();
                            ImGui::Text("%s", st.details.name.c_str());
                            ImGui::Separator();
                            ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                            ImGui::TextColored(countColor, "%s %lld", Localization::GetText("count_label"), st.count);
                            long long profit = ItemTracker::GetStatProfit(st);
                            if (profit > 0)
                                ImGui::TextColored(ImVec4(1.f, 0.84f, 0.f, 1.f), "%s %s", Localization::GetText("profit_label"), UICommon::FormatCoin(profit).c_str());
                            else if (profit < 0)
                                ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.f), "%s %s", Localization::GetText("profit_label"), UICommon::FormatCoin(profit).c_str());
                            else
                                ImGui::TextUnformatted(Localization::GetText("no_profit"));
                            ImGui::Separator();
                            char rarityLabel[256];
                            snprintf(rarityLabel, sizeof(rarityLabel), Localization::GetText("rarity_label"), st.details.rarity.c_str());
                            ImGui::Text("%s", rarityLabel);
                            ImGui::EndTooltip();
                        }

                        // Count text (bottom right, proportional to icon size)
                        ImVec2 iconSize = ImVec2(static_cast<float>(g_Settings.gridIconSize), static_cast<float>(g_Settings.gridIconSize));
                        float offsetX = iconSize.x * 0.15f; // 15% offset from right
                        float offsetY = iconSize.y * 0.15f; // 15% offset from bottom
                        ImVec2 countPos = ImVec2(cursor.x + iconSize.x - ImGui::CalcTextSize("999").x - offsetX, cursor.y + iconSize.y - ImGui::CalcTextSize("999").y - offsetY);
                        ImGui::SetCursorPos(countPos);

                        ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                        float fontSize = static_cast<float>(g_Settings.gridIconSize) * 0.4f; // Increased from 0.3f to 0.4f
                        
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
        }
        ImGui::EndChild();

        if (s_GridPendingId != -1)
        {
            UIContextMenu::OpenContextMenu("ItemContextMenu", s_GridPendingId, s_GridPendingName);
            s_GridPendingId = -1;
        }
        UIContextMenu::RenderItemContextMenu("ItemContextMenu", UIContextMenu::ContextMenuType::General);
    }
    else
    {
        // Group by Rarity options
        if (ImGui::Checkbox(Localization::GetText("group_by_rarity"), &g_Settings.groupByRarity))
        {
            if (g_Settings.groupByRarity) g_Settings.groupByType = false;
            SettingsManager::Save();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", "Group items by rarity with collapsible sections or tabs");

        ImGui::SameLine();
        if (g_Settings.groupByRarity)
        {
            if (ImGui::Checkbox(Localization::GetText("show_rarity_as_tabs"), &g_Settings.showRarityAsTabs))
                SettingsManager::Save();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", "Show rarity groups as tabs instead of collapsible sections");
        }
        else
        {
            ImGui::TextDisabled("%s", Localization::GetText("show_rarity_as_tabs"));
        }

        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();

        // Group by Category options
        if (ImGui::Checkbox(Localization::GetText("group_by_type"), &g_Settings.groupByType))
        {
            if (g_Settings.groupByType) g_Settings.groupByRarity = false;
            SettingsManager::Save();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", "Group items by category (type) with collapsible sections or tabs");

        ImGui::SameLine();
        if (g_Settings.groupByType)
        {
            if (ImGui::Checkbox(Localization::GetText("show_type_as_tabs"), &g_Settings.showTypeAsTabs))
                SettingsManager::Save();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", "Show category groups as tabs instead of collapsible sections");
        }
        else
        {
            ImGui::TextDisabled("%s", Localization::GetText("show_type_as_tabs"));
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Items Table with enhanced features
        int itemTableColumnCount = 7;

        if (!g_Settings.groupByRarity && !g_Settings.groupByType)
        {
            // Get best drop for highlighting
            auto bestDrop = ItemTracker::GetBestDrop();
            int bestDropId = bestDrop.first;

            // Normal view without grouping
            if (ImGui::BeginTable("##ItemsTable_v3", itemTableColumnCount, UICommon::DataTableFlags()))
            {
                SetupStandardItemTableColumns();
                ImGui::TableHeadersRow();

                for (auto& [id, st] : sortedItems)
                {
                    float rowH = UICommon::CalcTableRowHeight(static_cast<float>(g_Settings.iconSize));
                    ImGui::TableNextRow(0, rowH);

                    // Apply favorite row background color if enabled
                    if (st.isFavorite && g_Settings.enableFavoriteRowColor)
                    {
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(g_Settings.favoriteRowColor[0], g_Settings.favoriteRowColor[1], g_Settings.favoriteRowColor[2], g_Settings.favoriteRowColor[3])));
                    }

                    // Apply best drop golden border if enabled
                    if (g_Settings.enableBestDropHighlight && id == bestDropId && st.count > 0)
                    {
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(1.0f, 0.84f, 0.0f, 0.15f))); // Gold with low alpha
                    }

                    ImGui::TableSetColumnIndex(0);
                    UICommon::AlignTableCellIcon(rowH, static_cast<float>(g_Settings.iconSize));
                    UICommon::DrawItemIconCell(id, st.details.iconUrl, static_cast<float>(g_Settings.iconSize), st.details.loaded ? st.details.rarity : "");

                    // Helper lambda for rendering the same tooltip
                    auto renderItemTooltip = [&]() {
                        if (st.details.loaded)
                        {
                            ImGui::BeginTooltip();
                            ImGui::Text("%s", st.details.name.c_str());
                            ImGui::Separator();
                            char rarityLabel[256];
                            snprintf(rarityLabel, sizeof(rarityLabel), Localization::GetText("rarity_label"), st.details.rarity.c_str());
                            ImGui::Text("%s", rarityLabel);
                            char typeLabel[256];
                            snprintf(typeLabel, sizeof(typeLabel), Localization::GetText("type_label"), static_cast<int>(st.details.itemType));
                            ImGui::Text("%s", typeLabel);
                            // Only show vendor value if the item is actually sellable to vendor
                            if (ItemTracker::CanSellToVendor(st.details))
                            {
                                ImVec4 vendorColor = ImVec4(1.f, 0.84f, 0.f, 1.f);
                                ImGui::TextColored(vendorColor, Localization::GetText("vendor_value_format"), UICommon::FormatCoin(st.details.vendorValue).c_str());
                            }
                            ImVec4 tpSellGrossColor = st.details.tpSellPrice > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.details.tpSellPrice < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                            ImGui::TextColored(tpSellGrossColor, Localization::GetText("tp_sell_gross_format"), UICommon::FormatCoin(st.details.tpSellPrice).c_str());
                            long long tpSellNet = static_cast<long long>(st.details.tpSellPrice * 85.0 / 100.0);
                            ImVec4 tpSellNetColor = tpSellNet > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (tpSellNet < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                            ImGui::TextColored(tpSellNetColor, Localization::GetText("tp_sell_net_format"), UICommon::FormatCoin(tpSellNet).c_str());
                            ImVec4 tpBuyGrossColor = st.details.tpBuyPrice > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.details.tpBuyPrice < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                            ImGui::TextColored(tpBuyGrossColor, Localization::GetText("tp_buy_gross_format"), UICommon::FormatCoin(st.details.tpBuyPrice).c_str());
                            long long tpBuyNet = static_cast<long long>(st.details.tpBuyPrice * 85.0 / 100.0);
                            ImVec4 tpBuyNetColor = tpBuyNet > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (tpBuyNet < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                            ImGui::TextColored(tpBuyNetColor, Localization::GetText("tp_buy_net_format"), UICommon::FormatCoin(tpBuyNet).c_str());
                            char accountBoundLabel[256];
                            snprintf(accountBoundLabel, sizeof(accountBoundLabel), Localization::GetText("account_bound_label"), st.details.accountBound ? Localization::GetText("yes_label") : Localization::GetText("no_label"));
                            ImGui::Text("%s", accountBoundLabel);
                            char noSellLabel[256];
                            snprintf(noSellLabel, sizeof(noSellLabel), Localization::GetText("nosell_label"), st.details.noSell ? Localization::GetText("yes_label") : Localization::GetText("no_label"));
                            ImGui::Text("%s", noSellLabel);
                            ImGui::Separator();
                            char itemIdLabel[256];
                            snprintf(itemIdLabel, sizeof(itemIdLabel), Localization::GetText("item_id_label"), id);
                            ImGui::Text("%s", itemIdLabel);
                            ImGui::EndTooltip();
                        }
                    };

                    // Show tooltip on icon
                    if (ImGui::IsItemHovered())
                    {
                        renderItemTooltip();
                    }

                    // Right-click context menu
                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                    {
                        UIContextMenu::OpenContextMenu("ItemContextMenu", id, st.details.loaded ? st.details.name : "");
                    }

                    ImGui::TableSetColumnIndex(1);
                    UICommon::AlignTableCellText(rowH);
                    std::string name = st.details.loaded ? st.details.name : "Loading...";

                    // Add star icon for favorites
                    if (st.isFavorite)
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "* ");
                        ImGui::SameLine();
                    }

                    ImVec4 col = ImVec4(1.f, 1.f, 1.f, 1.f);
                    if (st.details.loaded && !st.details.rarity.empty())
                    {
                        // Rarity color (enhanced) - according to GW2 Wiki
                        if (st.details.rarity == "Junk") col = ImVec4(0.7f, 0.7f, 0.7f, 1.f);
                        else if (st.details.rarity == "Basic") col = ImVec4(1.f, 1.f, 1.f, 1.f);
                        else if (st.details.rarity == "Fine") col = ImVec4(0.0f, 0.5f, 1.f, 1.f);
                        else if (st.details.rarity == "Masterwork") col = ImVec4(0.2f, 0.8f, 0.2f, 1.f);
                        else if (st.details.rarity == "Rare") col = ImVec4(1.f, 0.9f, 0.0f, 1.f);
                        else if (st.details.rarity == "Exotic") col = ImVec4(1.f, 0.6f, 0.0f, 1.f);
                        else if (st.details.rarity == "Ascended") col = ImVec4(0.9f, 0.3f, 0.9f, 1.f);
                        else if (st.details.rarity == "Legendary") col = ImVec4(1.0f, 0.5f, 0.8f, 1.f);
                    }

                    // Apply favorite text color if enabled
                    if (st.isFavorite && g_Settings.enableFavoriteTextColor)
                        col = ImVec4(g_Settings.favoriteTextColor[0], g_Settings.favoriteTextColor[1], g_Settings.favoriteTextColor[2], g_Settings.favoriteTextColor[3]);

                    UICommon::TextWithTooltip(name.c_str(), 200.0f, col);
                    if (ImGui::IsItemHovered())
                    {
                        renderItemTooltip();
                    }

                    // Right-click context menu for name
                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                    {
                        UIContextMenu::OpenContextMenu("ItemContextMenu", id, st.details.loaded ? st.details.name : "");
                    }

                    ImGui::TableSetColumnIndex(2);
                    UICommon::AlignTableCellText(rowH);
                    ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                    ImGui::TextColored(countColor, "%lld", st.count);

                    ImGui::TableSetColumnIndex(3);
                    UICommon::AlignTableCellText(rowH);
                    long long profit = ItemTracker::GetStatProfit(st);
                    if (profit > 0)
                        ImGui::TextColored(ImVec4(1.f, 0.84f, 0.f, 1.f), "%s", UICommon::FormatCoin(profit).c_str());
                    else if (profit < 0)
                        ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.f), "%s", UICommon::FormatCoin(profit).c_str());
                    else
                        ImGui::TextUnformatted(Localization::GetText("no_profit"));

                    ImGui::TableSetColumnIndex(4);
                    UICommon::AlignTableCellText(rowH);
                    if (st.lastMagicFind >= 0)
                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%d%%", st.lastMagicFind);
                    else
                        ImGui::TextDisabled("N/A");
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", Localization::GetText("magic_find_tooltip"));

                    ImGui::TableSetColumnIndex(5);
                    UICommon::AlignTableCellFrame(rowH);
                    bool isFavorite = st.isFavorite;
                    if (ImGui::Checkbox(("##fav_" + std::to_string(id)).c_str(), &isFavorite))
                    {
                        ItemTracker::SetFavorite(id, isFavorite);
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", Localization::GetText("toggle_favorite"));

                    ImGui::TableSetColumnIndex(6);
                    UICommon::AlignTableCellFrame(rowH);
                    bool isIgnored = st.isIgnored;
                    if (ImGui::Checkbox(("##ign_" + std::to_string(id)).c_str(), &isIgnored))
                    {
                        if (isIgnored)
                            IgnoredItemsManager::IgnoreItem(id);
                        else
                            IgnoredItemsManager::UnignoreItem(id);
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", Localization::GetText("toggle_ignore"));
                }

                // Context menu popup (rendered once outside the loop)
                UIContextMenu::RenderItemContextMenu("ItemContextMenu", UIContextMenu::ContextMenuType::General);

                ImGui::EndTable();
            }
        }
        else if (g_Settings.groupByRarity)
        {
            // Group by Rarity view
            // Get best drop for highlighting
            auto bestDrop = ItemTracker::GetBestDrop();
            int bestDropId = bestDrop.first;

            // Rarity order (highest to lowest)
            std::vector<std::string> rarityOrder = {
                Localization::GetText("rarity_name_legendary"),
                Localization::GetText("rarity_name_ascended"),
                Localization::GetText("rarity_name_exotic"),
                Localization::GetText("rarity_name_rare"),
                Localization::GetText("rarity_name_masterwork"),
                Localization::GetText("rarity_name_fine"),
                Localization::GetText("rarity_name_basic"),
                Localization::GetText("rarity_name_junk"),
                Localization::GetText("rarity_name_unknown")
            };

            // Group items by localized rarity name
            std::map<std::string, std::vector<std::pair<int, Stat>>> rarityGroups;
            for (auto& [id, st] : sortedItems)
            {
                std::string apiRarity = st.details.loaded ? st.details.rarity : "Unknown";
                std::string localizedRarity;
                if (apiRarity == "Legendary") localizedRarity = Localization::GetText("rarity_name_legendary");
                else if (apiRarity == "Ascended") localizedRarity = Localization::GetText("rarity_name_ascended");
                else if (apiRarity == "Exotic") localizedRarity = Localization::GetText("rarity_name_exotic");
                else if (apiRarity == "Rare") localizedRarity = Localization::GetText("rarity_name_rare");
                else if (apiRarity == "Masterwork") localizedRarity = Localization::GetText("rarity_name_masterwork");
                else if (apiRarity == "Fine") localizedRarity = Localization::GetText("rarity_name_fine");
                else if (apiRarity == "Basic") localizedRarity = Localization::GetText("rarity_name_basic");
                else if (apiRarity == "Junk") localizedRarity = Localization::GetText("rarity_name_junk");
                else localizedRarity = Localization::GetText("rarity_name_unknown");

                rarityGroups[localizedRarity].push_back({id, st});
            }

            // Rarity colors (mapped to localized names)
            std::map<std::string, ImVec4> rarityColors;
            rarityColors[Localization::GetText("rarity_name_legendary")] = ImVec4(1.0f, 0.5f, 0.8f, 1.f);
            rarityColors[Localization::GetText("rarity_name_ascended")] = ImVec4(0.9f, 0.3f, 0.9f, 1.f);
            rarityColors[Localization::GetText("rarity_name_exotic")] = ImVec4(1.f, 0.6f, 0.0f, 1.f);
            rarityColors[Localization::GetText("rarity_name_rare")] = ImVec4(1.f, 0.9f, 0.0f, 1.f);
            rarityColors[Localization::GetText("rarity_name_masterwork")] = ImVec4(0.2f, 0.8f, 0.2f, 1.f);
            rarityColors[Localization::GetText("rarity_name_fine")] = ImVec4(0.0f, 0.5f, 1.f, 1.f);
            rarityColors[Localization::GetText("rarity_name_basic")] = ImVec4(1.f, 1.f, 1.f, 1.f);
            rarityColors[Localization::GetText("rarity_name_junk")] = ImVec4(0.7f, 0.7f, 0.7f, 1.f);
            rarityColors[Localization::GetText("rarity_name_unknown")] = ImVec4(0.5f, 0.5f, 0.5f, 1.f);

            if (!g_Settings.showRarityAsTabs)
            {
                // Sections mode
                for (const auto& rarity : rarityOrder)
                {
                    if (rarityGroups.find(rarity) == rarityGroups.end() || rarityGroups[rarity].empty())
                        continue;

                    char headerLabel[256];
                    snprintf(headerLabel, sizeof(headerLabel), "%s (%zu)", rarity.c_str(), rarityGroups[rarity].size());

                    ImVec4 headerColor = rarityColors.count(rarity) ? rarityColors[rarity] : ImVec4(1.f, 1.f, 1.f, 1.f);
                    ImGui::PushStyleColor(ImGuiCol_Text, headerColor);
                    // Add dark semi-transparent background for better visibility on any accent color
                    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 0.8f));
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.15f, 0.15f, 0.15f, 0.8f));
                    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));

                    if (ImGui::CollapsingHeader(headerLabel, ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::PopStyleColor(4);

                        if (ImGui::BeginTable(("##RarityTable_v3_" + rarity).c_str(), itemTableColumnCount, UICommon::DataTableFlags()))
                        {
                            SetupStandardItemTableColumns();
                            ImGui::TableHeadersRow();

                            for (auto& [id, st] : rarityGroups[rarity])
                            {
                                float rowH = UICommon::CalcTableRowHeight(static_cast<float>(g_Settings.iconSize));
                                ImGui::TableNextRow(0, rowH);

                                // Apply favorite row background color if enabled
                                if (st.isFavorite && g_Settings.enableFavoriteRowColor)
                                {
                                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(g_Settings.favoriteRowColor[0], g_Settings.favoriteRowColor[1], g_Settings.favoriteRowColor[2], g_Settings.favoriteRowColor[3])));
                                }

                                // Apply best drop golden border if enabled
                                if (g_Settings.enableBestDropHighlight && id == bestDropId && st.count > 0)
                                {
                                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(1.0f, 0.84f, 0.0f, 0.15f))); // Gold with low alpha
                                }

                                ImGui::TableSetColumnIndex(0);
                                UICommon::AlignTableCellIcon(rowH, static_cast<float>(g_Settings.iconSize));
                                UICommon::DrawItemIconCell(id, st.details.iconUrl, static_cast<float>(g_Settings.iconSize), st.details.loaded ? st.details.rarity : "");

                                // Right-click context menu
                                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                                {
                                    UIContextMenu::OpenContextMenu("ItemContextMenu", id, st.details.loaded ? st.details.name : "");
                                }

                                ImGui::TableSetColumnIndex(1);
                                UICommon::AlignTableCellText(rowH);
                                std::string name = st.details.loaded ? st.details.name : "Loading...";

                                // Add star icon for favorites
                                if (st.isFavorite)
                                {
                                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "* ");
                                    ImGui::SameLine();
                                }

                                ImVec4 col = ImVec4(1.f, 1.f, 1.f, 1.f);
                                if (st.details.loaded && !st.details.rarity.empty())
                                {
                                    if (st.details.rarity == "Junk") col = ImVec4(0.7f, 0.7f, 0.7f, 1.f);
                                    else if (st.details.rarity == "Basic") col = ImVec4(1.f, 1.f, 1.f, 1.f);
                                    else if (st.details.rarity == "Fine") col = ImVec4(0.0f, 0.5f, 1.f, 1.f);
                                    else if (st.details.rarity == "Masterwork") col = ImVec4(0.2f, 0.8f, 0.2f, 1.f);
                                    else if (st.details.rarity == "Rare") col = ImVec4(1.f, 0.9f, 0.0f, 1.f);
                                    else if (st.details.rarity == "Exotic") col = ImVec4(1.f, 0.6f, 0.0f, 1.f);
                                    else if (st.details.rarity == "Ascended") col = ImVec4(0.9f, 0.3f, 0.9f, 1.f);
                                    else if (st.details.rarity == "Legendary") col = ImVec4(1.0f, 0.5f, 0.8f, 1.f);
                                }

                                if (st.isFavorite && g_Settings.enableFavoriteTextColor)
                                    col = ImVec4(g_Settings.favoriteTextColor[0], g_Settings.favoriteTextColor[1], g_Settings.favoriteTextColor[2], g_Settings.favoriteTextColor[3]);

                                UICommon::TextWithTooltip(name.c_str(), 200.0f, col);

                                // Right-click context menu for name
                                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                                {
                                    UIContextMenu::OpenContextMenu("ItemContextMenu", id, st.details.loaded ? st.details.name : "");
                                }

                                ImGui::TableSetColumnIndex(2);
                                UICommon::AlignTableCellText(rowH);
                                ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                                ImGui::TextColored(countColor, "%lld", st.count);

                                ImGui::TableSetColumnIndex(3);
                                UICommon::AlignTableCellText(rowH);
                                long long profit = ItemTracker::GetStatProfit(st);
                                if (profit > 0)
                                    ImGui::TextColored(ImVec4(1.f, 0.84f, 0.f, 1.f), "%s", UICommon::FormatCoin(profit).c_str());
                                else if (profit < 0)
                                    ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.f), "%s", UICommon::FormatCoin(profit).c_str());
                                else
                                    ImGui::TextUnformatted(Localization::GetText("no_profit"));

                                ImGui::TableSetColumnIndex(4);
                                UICommon::AlignTableCellText(rowH);
                                if (st.lastMagicFind >= 0)
                                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%d%%", st.lastMagicFind);
                                else
                                    ImGui::TextDisabled("N/A");
                                if (ImGui::IsItemHovered())
                                    ImGui::SetTooltip("%s", Localization::GetText("magic_find_tooltip"));

                                ImGui::TableSetColumnIndex(5);
                                UICommon::AlignTableCellFrame(rowH);
                                bool isFavorite = st.isFavorite;
                                if (ImGui::Checkbox(("##fav_" + std::to_string(id)).c_str(), &isFavorite))
                                {
                                    ItemTracker::SetFavorite(id, isFavorite);
                                }
                                if (ImGui::IsItemHovered())
                                    ImGui::SetTooltip("%s", Localization::GetText("toggle_favorite"));

                                ImGui::TableSetColumnIndex(6);
                                UICommon::AlignTableCellFrame(rowH);
                                bool isIgnored = st.isIgnored;
                                if (ImGui::Checkbox(("##ign_" + std::to_string(id)).c_str(), &isIgnored))
                                {
                                    if (isIgnored)
                                        IgnoredItemsManager::IgnoreItem(id);
                                    else
                                        IgnoredItemsManager::UnignoreItem(id);
                                }
                                if (ImGui::IsItemHovered())
                                    ImGui::SetTooltip("%s", Localization::GetText("toggle_ignore"));
                            }

                            // Context menu popup (rendered once outside the loop)
                            UIContextMenu::RenderItemContextMenu("ItemContextMenu", UIContextMenu::ContextMenuType::General);

                            ImGui::EndTable();
                        }
                    }
                    else
                    {
                        ImGui::PopStyleColor();
                    }
                }
            }
            else
            {
                // Tabs mode
                if (ImGui::BeginTabBar("##RarityTabs"))
                {
                    for (const auto& rarity : rarityOrder)
                    {
                        if (rarityGroups.find(rarity) == rarityGroups.end() || rarityGroups[rarity].empty())
                            continue;

                        char tabLabel[256];
                        snprintf(tabLabel, sizeof(tabLabel), "%s (%zu)", rarity.c_str(), rarityGroups[rarity].size());

                        ImVec4 tabColor = rarityColors.count(rarity) ? rarityColors[rarity] : ImVec4(1.f, 1.f, 1.f, 1.f);
                        ImGui::PushStyleColor(ImGuiCol_Text, tabColor);

                        if (ImGui::BeginTabItem(tabLabel))
                        {
                            ImGui::PopStyleColor();

                            if (ImGui::BeginTable(("##RarityTable_" + rarity).c_str(), itemTableColumnCount, UICommon::DataTableFlags()))
                            {
                                SetupCompactItemTableColumns();
                                ImGui::TableHeadersRow();

                                for (auto& [id, st] : rarityGroups[rarity])
                                {
                                    float rowH = UICommon::CalcTableRowHeight(static_cast<float>(g_Settings.iconSize));
                                    ImGui::TableNextRow(0, rowH);

                                    // Apply favorite row background color if enabled
                                    if (st.isFavorite && g_Settings.enableFavoriteRowColor)
                                    {
                                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(g_Settings.favoriteRowColor[0], g_Settings.favoriteRowColor[1], g_Settings.favoriteRowColor[2], g_Settings.favoriteRowColor[3])));
                                    }

                                    // Apply best drop golden border if enabled
                                    if (g_Settings.enableBestDropHighlight && id == bestDropId && st.count > 0)
                                    {
                                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(1.0f, 0.84f, 0.0f, 0.15f))); // Gold with low alpha
                                    }

                                    ImGui::TableSetColumnIndex(0);
                                    UICommon::AlignTableCellIcon(rowH, static_cast<float>(g_Settings.iconSize));
                                    UICommon::DrawItemIconCell(id, st.details.iconUrl, static_cast<float>(g_Settings.iconSize), st.details.loaded ? st.details.rarity : "");

                                    // Right-click context menu
                                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                                    {
                                        UIContextMenu::OpenContextMenu("ItemContextMenu", id, st.details.loaded ? st.details.name : "");
                                    }

                                    ImGui::TableSetColumnIndex(1);
                                    UICommon::AlignTableCellText(rowH);
                                    std::string name = st.details.loaded ? st.details.name : "Loading...";

                                    // Add star icon for favorites
                                    if (st.isFavorite)
                                    {
                                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "* ");
                                        ImGui::SameLine();
                                    }

                                    ImVec4 col = ImVec4(1.f, 1.f, 1.f, 1.f);
                                    if (st.details.loaded && !st.details.rarity.empty())
                                    {
                                        if (st.details.rarity == "Junk") col = ImVec4(0.7f, 0.7f, 0.7f, 1.f);
                                        else if (st.details.rarity == "Basic") col = ImVec4(1.f, 1.f, 1.f, 1.f);
                                        else if (st.details.rarity == "Fine") col = ImVec4(0.0f, 0.5f, 1.f, 1.f);
                                        else if (st.details.rarity == "Masterwork") col = ImVec4(0.2f, 0.8f, 0.2f, 1.f);
                                        else if (st.details.rarity == "Rare") col = ImVec4(1.f, 0.9f, 0.0f, 1.f);
                                        else if (st.details.rarity == "Exotic") col = ImVec4(1.f, 0.6f, 0.0f, 1.f);
                                        else if (st.details.rarity == "Ascended") col = ImVec4(0.9f, 0.3f, 0.9f, 1.f);
                                        else if (st.details.rarity == "Legendary") col = ImVec4(1.0f, 0.5f, 0.8f, 1.f);
                                    }

                                    if (st.isFavorite && g_Settings.enableFavoriteTextColor)
                                        col = ImVec4(g_Settings.favoriteTextColor[0], g_Settings.favoriteTextColor[1], g_Settings.favoriteTextColor[2], g_Settings.favoriteTextColor[3]);

                                    UICommon::TextWithTooltip(name.c_str(), 200.0f, col);

                                    // Right-click context menu for name
                                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                                    {
                                        UIContextMenu::OpenContextMenu("ItemContextMenu", id, st.details.loaded ? st.details.name : "");
                                    }

                                    ImGui::TableSetColumnIndex(2);
                                    UICommon::AlignTableCellText(rowH);
                                    ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                                    ImGui::TextColored(countColor, "%lld", st.count);

                                    ImGui::TableSetColumnIndex(3);
                                    UICommon::AlignTableCellText(rowH);
                                    long long profit = ItemTracker::GetStatProfit(st);
                                    if (profit > 0)
                                        ImGui::TextColored(ImVec4(1.f, 0.84f, 0.f, 1.f), "%s", UICommon::FormatCoin(profit).c_str());
                                    else if (profit < 0)
                                        ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.f), "%s", UICommon::FormatCoin(profit).c_str());
                                    else
                                        ImGui::TextUnformatted(Localization::GetText("no_profit"));

                                    ImGui::TableSetColumnIndex(4);
                                    UICommon::AlignTableCellText(rowH);
                                    if (st.lastMagicFind >= 0)
                                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%d%%", st.lastMagicFind);
                                    else
                                        ImGui::TextDisabled("N/A");
                                    if (ImGui::IsItemHovered())
                                        ImGui::SetTooltip("%s", Localization::GetText("magic_find_tooltip"));

                                    ImGui::TableSetColumnIndex(5);
                                    UICommon::AlignTableCellFrame(rowH);
                                    bool isFavorite = st.isFavorite;
                                    if (ImGui::Checkbox(("##fav_" + std::to_string(id)).c_str(), &isFavorite))
                                    {
                                        ItemTracker::SetFavorite(id, isFavorite);
                                    }
                                    if (ImGui::IsItemHovered())
                                        ImGui::SetTooltip("%s", Localization::GetText("toggle_favorite"));

                                    ImGui::TableSetColumnIndex(6);
                                    UICommon::AlignTableCellFrame(rowH);
                                    bool isIgnored = st.isIgnored;
                                    if (ImGui::Checkbox(("##ign_" + std::to_string(id)).c_str(), &isIgnored))
                                    {
                                        if (isIgnored)
                                            IgnoredItemsManager::IgnoreItem(id);
                                        else
                                            IgnoredItemsManager::UnignoreItem(id);
                                    }
                                    if (ImGui::IsItemHovered())
                                        ImGui::SetTooltip("%s", Localization::GetText("toggle_ignore"));
                                }

                                // Context menu popup (rendered once outside the loop)
                                UIContextMenu::RenderItemContextMenu("ItemContextMenu", UIContextMenu::ContextMenuType::General);

                                ImGui::EndTable();
                            }
                            ImGui::EndTabItem();
                        }
                        else
                        {
                            ImGui::PopStyleColor();
                        }
                    }
                    ImGui::EndTabBar();
                }
            }
        }
        else if (g_Settings.groupByType)
        {
            // Define ItemType order (logical sorting)
            std::vector<ItemType> typeOrder = {
                ItemType::Weapon, ItemType::Armor, ItemType::Trinket, ItemType::Backpack,
                ItemType::CraftingMaterial, ItemType::Consumable, ItemType::Container, ItemType::Bag,
                ItemType::UpgradeComponent, ItemType::Trophy, ItemType::Gizmo, ItemType::Tool,
                ItemType::GatheringTool, ItemType::MiniPet, ItemType::Unlock, ItemType::Unknown
            };

            // Helper to get localized type name
            auto getTypeName = [](ItemType type) -> std::string {
                switch (type) {
                    case ItemType::Armor: return Localization::GetText("type_armor");
                    case ItemType::Weapon: return Localization::GetText("type_weapon");
                    case ItemType::Trinket: return Localization::GetText("type_trinket");
                    case ItemType::Gizmo: return Localization::GetText("type_gizmo");
                    case ItemType::CraftingMaterial: return Localization::GetText("type_crafting_material");
                    case ItemType::Consumable: return Localization::GetText("type_consumable");
                    case ItemType::GatheringTool: return Localization::GetText("type_gathering_tool");
                    case ItemType::Bag: return Localization::GetText("type_bag");
                    case ItemType::Container: return Localization::GetText("type_container");
                    case ItemType::MiniPet: return Localization::GetText("type_mini_pet");
                    case ItemType::GizmoContainer: return Localization::GetText("type_gizmo_container");
                    case ItemType::Backpack: return Localization::GetText("type_backpack");
                    case ItemType::UpgradeComponent: return Localization::GetText("type_upgrade_component");
                    case ItemType::Tool: return Localization::GetText("type_tool");
                    case ItemType::Trophy: return Localization::GetText("type_trophy");
                    case ItemType::Unlock: return Localization::GetText("type_unlock");
                    default: return Localization::GetText("rarity_name_unknown");
                }
            };

            // Group items by type
            std::map<ItemType, std::vector<std::pair<int, Stat>>> typeGroups;
            for (auto& [id, st] : sortedItems) {
                typeGroups[st.details.itemType].push_back({id, st});
            }

            if (!g_Settings.showTypeAsTabs)
            {
                // Get best drop for highlighting
                auto bestDrop = ItemTracker::GetBestDrop();
                int bestDropId = bestDrop.first;

                for (auto type : typeOrder)
                {
                    auto it = typeGroups.find(type);
                    if (it == typeGroups.end() || it->second.empty()) continue;

                    ImGui::Spacing();
                    // Add dark semi-transparent background for better visibility on any accent color
                    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 0.8f));
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.15f, 0.15f, 0.15f, 0.8f));
                    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));

                    char headerLabel[256];
                    snprintf(headerLabel, sizeof(headerLabel), "%s (%zu)", getTypeName(type).c_str(), it->second.size());

                    if (ImGui::CollapsingHeader(headerLabel, ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::PopStyleColor(3);
                        if (ImGui::BeginTable(("##TypeTable_" + std::to_string(static_cast<int>(type))).c_str(), itemTableColumnCount, UICommon::DataTableFlags()))
                        {
                            SetupStandardItemTableColumns();
                            ImGui::TableHeadersRow();

                            for (auto& [id, st] : it->second)
                            {
                                float rowH = UICommon::CalcTableRowHeight(static_cast<float>(g_Settings.iconSize));
                                ImGui::TableNextRow(0, rowH);

                                // Apply favorite row background color if enabled
                                if (st.isFavorite && g_Settings.enableFavoriteRowColor)
                                {
                                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(g_Settings.favoriteRowColor[0], g_Settings.favoriteRowColor[1], g_Settings.favoriteRowColor[2], g_Settings.favoriteRowColor[3])));
                                }

                                // Apply best drop golden border if enabled
                                if (g_Settings.enableBestDropHighlight && id == bestDropId && st.count > 0)
                                {
                                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(1.0f, 0.84f, 0.0f, 0.15f))); // Gold with low alpha
                                }

                                ImGui::TableSetColumnIndex(0);
                                UICommon::AlignTableCellIcon(rowH, static_cast<float>(g_Settings.iconSize));
                                UICommon::DrawItemIconCell(id, st.details.iconUrl, static_cast<float>(g_Settings.iconSize), st.details.loaded ? st.details.rarity : "");

                                // Right-click context menu
                                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                                {
                                    UIContextMenu::OpenContextMenu("ItemContextMenu", id, st.details.loaded ? st.details.name : "");
                                }

                                ImGui::TableSetColumnIndex(1);
                                UICommon::AlignTableCellText(rowH);
                                std::string name = st.details.loaded ? st.details.name : "Loading...";

                                // Add star icon for favorites
                                if (st.isFavorite)
                                {
                                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "* ");
                                    ImGui::SameLine();
                                }

                                ImVec4 col = ImVec4(1.f, 1.f, 1.f, 1.f);
                                if (st.details.loaded && !st.details.rarity.empty())
                                {
                                    if (st.details.rarity == "Junk") col = ImVec4(0.7f, 0.7f, 0.7f, 1.f);
                                    else if (st.details.rarity == "Basic") col = ImVec4(1.f, 1.f, 1.f, 1.f);
                                    else if (st.details.rarity == "Fine") col = ImVec4(0.0f, 0.5f, 1.f, 1.f);
                                    else if (st.details.rarity == "Masterwork") col = ImVec4(0.2f, 0.8f, 0.2f, 1.f);
                                    else if (st.details.rarity == "Rare") col = ImVec4(1.f, 0.9f, 0.0f, 1.f);
                                    else if (st.details.rarity == "Exotic") col = ImVec4(1.f, 0.6f, 0.0f, 1.f);
                                    else if (st.details.rarity == "Ascended") col = ImVec4(0.9f, 0.3f, 0.9f, 1.f);
                                    else if (st.details.rarity == "Legendary") col = ImVec4(1.0f, 0.5f, 0.8f, 1.f);
                                }

                                if (st.isFavorite && g_Settings.enableFavoriteTextColor)
                                    col = ImVec4(g_Settings.favoriteTextColor[0], g_Settings.favoriteTextColor[1], g_Settings.favoriteTextColor[2], g_Settings.favoriteTextColor[3]);

                                UICommon::TextWithTooltip(name.c_str(), 200.0f, col);

                                // Right-click context menu for name
                                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                                {
                                    UIContextMenu::OpenContextMenu("ItemContextMenu", id, st.details.loaded ? st.details.name : "");
                                }

                                ImGui::TableSetColumnIndex(2);
                                UICommon::AlignTableCellText(rowH);
                                ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                                ImGui::TextColored(countColor, "%lld", st.count);

                                ImGui::TableSetColumnIndex(3);
                                UICommon::AlignTableCellText(rowH);
                                long long profit = ItemTracker::GetStatProfit(st);
                                if (profit > 0)
                                    ImGui::TextColored(ImVec4(1.f, 0.84f, 0.f, 1.f), "%s", UICommon::FormatCoin(profit).c_str());
                                else if (profit < 0)
                                    ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.f), "%s", UICommon::FormatCoin(profit).c_str());
                                else
                                    ImGui::TextUnformatted(Localization::GetText("no_profit"));

                                ImGui::TableSetColumnIndex(4);
                                UICommon::AlignTableCellText(rowH);
                                if (st.lastMagicFind >= 0)
                                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%d%%", st.lastMagicFind);
                                else
                                    ImGui::TextDisabled("N/A");
                                if (ImGui::IsItemHovered())
                                    ImGui::SetTooltip("%s", Localization::GetText("magic_find_tooltip"));

                                ImGui::TableSetColumnIndex(5);
                                UICommon::AlignTableCellFrame(rowH);
                                bool isFavorite = st.isFavorite;
                                if (ImGui::Checkbox(("##fav_" + std::to_string(id)).c_str(), &isFavorite))
                                {
                                    ItemTracker::SetFavorite(id, isFavorite);
                                }
                                if (ImGui::IsItemHovered())
                                    ImGui::SetTooltip("%s", Localization::GetText("toggle_favorite"));

                                ImGui::TableSetColumnIndex(6);
                                UICommon::AlignTableCellFrame(rowH);
                                bool isIgnored = st.isIgnored;
                                if (ImGui::Checkbox(("##ign_" + std::to_string(id)).c_str(), &isIgnored))
                                {
                                    if (isIgnored)
                                        IgnoredItemsManager::IgnoreItem(id);
                                    else
                                        IgnoredItemsManager::UnignoreItem(id);
                                }
                                if (ImGui::IsItemHovered())
                                    ImGui::SetTooltip("%s", Localization::GetText("toggle_ignore"));
                            }
                            ImGui::EndTable();
                        }
                    }
                    else { ImGui::PopStyleColor(3); }
                }
            }
            else
            {
                // Get best drop for highlighting
                auto bestDrop = ItemTracker::GetBestDrop();
                int bestDropId = bestDrop.first;

                if (ImGui::BeginTabBar("##TypeTabs"))
                {
                    for (auto type : typeOrder)
                    {
                        auto it = typeGroups.find(type);
                        if (it == typeGroups.end() || it->second.empty()) continue;

                        char tabLabel[256];
                        snprintf(tabLabel, sizeof(tabLabel), "%s (%zu)", getTypeName(type).c_str(), it->second.size());

                        if (ImGui::BeginTabItem(tabLabel))
                        {
                            if (ImGui::BeginTable(("##TypeTable_" + std::to_string(static_cast<int>(type))).c_str(), itemTableColumnCount, UICommon::DataTableFlags()))
                            {
                                SetupStandardItemTableColumns();
                                ImGui::TableHeadersRow();

                                for (auto& [id, st] : it->second)
                                {
                                    float rowH = UICommon::CalcTableRowHeight(static_cast<float>(g_Settings.iconSize));
                                    ImGui::TableNextRow(0, rowH);

                                    // Apply favorite row background color if enabled
                                    if (st.isFavorite && g_Settings.enableFavoriteRowColor)
                                    {
                                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(g_Settings.favoriteRowColor[0], g_Settings.favoriteRowColor[1], g_Settings.favoriteRowColor[2], g_Settings.favoriteRowColor[3])));
                                    }

                                    // Apply best drop golden border if enabled
                                    if (g_Settings.enableBestDropHighlight && id == bestDropId && st.count > 0)
                                    {
                                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(1.0f, 0.84f, 0.0f, 0.15f))); // Gold with low alpha
                                    }

                                    ImGui::TableSetColumnIndex(0);
                                    UICommon::AlignTableCellIcon(rowH, static_cast<float>(g_Settings.iconSize));
                                    UICommon::DrawItemIconCell(id, st.details.iconUrl, static_cast<float>(g_Settings.iconSize), st.details.loaded ? st.details.rarity : "");

                                    // Right-click context menu
                                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                                    {
                                        UIContextMenu::OpenContextMenu("ItemContextMenu", id, st.details.loaded ? st.details.name : "");
                                    }

                                    ImGui::TableSetColumnIndex(1);
                                    UICommon::AlignTableCellText(rowH);
                                    std::string name = st.details.loaded ? st.details.name : "Loading...";

                                    // Add star icon for favorites
                                    if (st.isFavorite)
                                    {
                                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "* ");
                                        ImGui::SameLine();
                                    }

                                    ImVec4 col = ImVec4(1.f, 1.f, 1.f, 1.f);
                                    if (st.details.loaded && !st.details.rarity.empty())
                                    {
                                        if (st.details.rarity == "Junk") col = ImVec4(0.7f, 0.7f, 0.7f, 1.f);
                                        else if (st.details.rarity == "Basic") col = ImVec4(1.f, 1.f, 1.f, 1.f);
                                        else if (st.details.rarity == "Fine") col = ImVec4(0.0f, 0.5f, 1.f, 1.f);
                                        else if (st.details.rarity == "Masterwork") col = ImVec4(0.2f, 0.8f, 0.2f, 1.f);
                                        else if (st.details.rarity == "Rare") col = ImVec4(1.f, 0.9f, 0.0f, 1.f);
                                        else if (st.details.rarity == "Exotic") col = ImVec4(1.f, 0.6f, 0.0f, 1.f);
                                        else if (st.details.rarity == "Ascended") col = ImVec4(0.9f, 0.3f, 0.9f, 1.f);
                                        else if (st.details.rarity == "Legendary") col = ImVec4(1.0f, 0.5f, 0.8f, 1.f);
                                    }

                                    if (st.isFavorite && g_Settings.enableFavoriteTextColor)
                                        col = ImVec4(g_Settings.favoriteTextColor[0], g_Settings.favoriteTextColor[1], g_Settings.favoriteTextColor[2], g_Settings.favoriteTextColor[3]);

                                    UICommon::TextWithTooltip(name.c_str(), 200.0f, col);

                                    // Right-click context menu for name
                                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                                    {
                                        UIContextMenu::OpenContextMenu("ItemContextMenu", id, st.details.loaded ? st.details.name : "");
                                    }

                                    ImGui::TableSetColumnIndex(2);
                                    UICommon::AlignTableCellText(rowH);
                                    ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                                    ImGui::TextColored(countColor, "%lld", st.count);

                                    ImGui::TableSetColumnIndex(3);
                                    UICommon::AlignTableCellText(rowH);
                                    long long profit = ItemTracker::GetStatProfit(st);
                                    if (profit > 0)
                                        ImGui::TextColored(ImVec4(1.f, 0.84f, 0.f, 1.f), "%s", UICommon::FormatCoin(profit).c_str());
                                    else if (profit < 0)
                                        ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.f), "%s", UICommon::FormatCoin(profit).c_str());
                                    else
                                        ImGui::TextUnformatted(Localization::GetText("no_profit"));

                                    ImGui::TableSetColumnIndex(4);
                                    UICommon::AlignTableCellText(rowH);
                                    if (st.lastMagicFind >= 0)
                                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%d%%", st.lastMagicFind);
                                    else
                                        ImGui::TextDisabled("N/A");
                                    if (ImGui::IsItemHovered())
                                        ImGui::SetTooltip("%s", Localization::GetText("magic_find_tooltip"));

                                    ImGui::TableSetColumnIndex(5);
                                    UICommon::AlignTableCellFrame(rowH);
                                    bool isFavorite = st.isFavorite;
                                    if (ImGui::Checkbox(("##fav_" + std::to_string(id)).c_str(), &isFavorite))
                                    {
                                        ItemTracker::SetFavorite(id, isFavorite);
                                    }
                                    if (ImGui::IsItemHovered())
                                        ImGui::SetTooltip("%s", Localization::GetText("toggle_favorite"));

                                    ImGui::TableSetColumnIndex(6);
                                    UICommon::AlignTableCellFrame(rowH);
                                    bool isIgnored = st.isIgnored;
                                    if (ImGui::Checkbox(("##ign_" + std::to_string(id)).c_str(), &isIgnored))
                                    {
                                        if (isIgnored)
                                            IgnoredItemsManager::IgnoreItem(id);
                                        else
                                            IgnoredItemsManager::UnignoreItem(id);
                                    }
                                    if (ImGui::IsItemHovered())
                                        ImGui::SetTooltip("%s", Localization::GetText("toggle_ignore"));
                                }
                                ImGui::EndTable();
                            }
                            ImGui::EndTabItem();
                        }
                    }
                    ImGui::EndTabBar();
                }
            }
        }
    }

    // Context menu popup (rendered once outside the loop)
    UIContextMenu::RenderItemContextMenu("ItemContextMenu", UIContextMenu::ContextMenuType::General);
}
}
