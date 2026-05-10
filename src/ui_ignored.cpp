#include "ui_ignored.h"
#include "settings.h"
#include "item_tracker.h"
#include "ignored_items.h"
#include "localization.h"
#include "ui_context_menu.h"

namespace UIIgnored
{
void RenderIgnoredTab()
{
    static int ignoredSubTab = 0; // 0 = Items, 1 = Currencies

    ImGui::BeginTabBar("IgnoredSubTabs");
    if (ImGui::BeginTabItem(Localization::GetText("tab_items")))
    {
        ignoredSubTab = 0;
        ImGui::Text("%s", Localization::GetText("manage_ignored_items"));

        auto ignoredItems = IgnoredItemsManager::GetIgnoredItems();

        size_t ignoredItemsCount = ignoredItems.size();
        ImVec4 ignoredItemsColor = ignoredItemsCount > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (ignoredItemsCount < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
        ImGui::TextColored(ignoredItemsColor, "%s %zu", Localization::GetText("ignored_items_label"), ignoredItemsCount);

        ImGui::Spacing();
        if (ImGui::Button(Localization::GetText("clear_all_ignored_items")))
        {
            for (auto& id : ignoredItems)
                IgnoredItemsManager::UnignoreItem(id);
        }

        ImGui::Spacing();
        ImGui::Separator();

        // Display ignored items in a table
        if (ImGui::BeginTable("##IgnoredItemsTable_v3", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings))
        {
            float iconColumnWidth = (static_cast<float>(g_Settings.iconSize) + 10.0f > 70.0f) ? (static_cast<float>(g_Settings.iconSize) + 10.0f) : 70.0f;
            ImGui::TableSetupColumn(Localization::GetText("column_icon"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, iconColumnWidth);
            ImGui::TableSetupColumn(Localization::GetText("column_name"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 430.0f);
            ImGui::TableSetupColumn(Localization::GetText("column_ignore"), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (auto& id : ignoredItems)
            {
                Stat st = ItemTracker::GetItemStat(id);
                
                float rowH = UICommon::CalcTableRowHeight(static_cast<float>(g_Settings.iconSize));
                ImGui::TableNextRow(0, rowH);

                ImGui::TableSetColumnIndex(0);
                UICommon::AlignTableCellIcon(rowH, static_cast<float>(g_Settings.iconSize));
                UICommon::DrawItemIconCell(id, st.details.iconUrl, static_cast<float>(g_Settings.iconSize), st.details.loaded ? st.details.rarity : "");

                // Right-click context menu
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                {
                    UIContextMenu::OpenContextMenu("IgnoredItemContextMenu", id, st.details.loaded ? st.details.name : "");
                }

                ImGui::TableSetColumnIndex(1);
                UICommon::AlignTableCellText(rowH);
                std::string name = st.details.loaded ? st.details.name : Localization::GetText("loading");
                ImGui::Text("%s", name.c_str());

                // Right-click context menu for name
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                {
                    UIContextMenu::OpenContextMenu("IgnoredItemContextMenu", id, st.details.loaded ? st.details.name : "");
                }

                ImGui::TableSetColumnIndex(2);
                UICommon::AlignTableCellFrame(rowH);
                bool isIgnored = true;
                if (ImGui::Checkbox(("##unign_" + std::to_string(id)).c_str(), &isIgnored))
                {
                    if (!isIgnored)
                        IgnoredItemsManager::UnignoreItem(id);
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", Localization::GetText("unignore_item"));
            }

            // Context menu popup (rendered once outside the loop)
            UIContextMenu::RenderItemContextMenu("IgnoredItemContextMenu", UIContextMenu::ContextMenuType::Ignored);

            ImGui::EndTable();
        }

        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem(Localization::GetText("tab_currencies")))
    {
        ignoredSubTab = 1;
        ImGui::Text("%s", Localization::GetText("manage_ignored_currencies"));

        auto ignoredCurrencies = IgnoredItemsManager::GetIgnoredCurrencies();

        size_t ignoredCurrenciesCount = ignoredCurrencies.size();
        ImVec4 ignoredCurrenciesColor = ignoredCurrenciesCount > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (ignoredCurrenciesCount < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
        ImGui::TextColored(ignoredCurrenciesColor, "%s %zu", Localization::GetText("ignored_currencies_label"), ignoredCurrenciesCount);

        ImGui::Spacing();
        if (ImGui::Button(Localization::GetText("clear_all_ignored_currencies")))
        {
            for (auto& id : ignoredCurrencies)
                IgnoredItemsManager::UnignoreCurrency(id);
        }

        ImGui::Spacing();
        ImGui::Separator();

        // Display ignored currencies in a table
        if (ImGui::BeginTable("##IgnoredCurrenciesTable_v3", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings))
        {
            float iconColumnWidth = (static_cast<float>(g_Settings.iconSize) + 10.0f > 70.0f) ? (static_cast<float>(g_Settings.iconSize) + 10.0f) : 70.0f;
            ImGui::TableSetupColumn(Localization::GetText("column_icon"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, iconColumnWidth);
            ImGui::TableSetupColumn(Localization::GetText("column_currency"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 430.0f);
            ImGui::TableSetupColumn(Localization::GetText("column_count"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 150.0f);
            ImGui::TableSetupColumn(Localization::GetText("column_ignore"), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (auto& id : ignoredCurrencies)
            {
                Stat st = ItemTracker::GetCurrencyStat(id);
                
                float rowH = UICommon::CalcTableRowHeight(static_cast<float>(g_Settings.iconSize));
                ImGui::TableNextRow(0, rowH);

                ImGui::TableSetColumnIndex(0);
                UICommon::AlignTableCellIcon(rowH, static_cast<float>(g_Settings.iconSize));
                std::string iconUrl = st.details.iconUrl;
                if (id == 1 && iconUrl.empty())
                    iconUrl = "https://wiki.guildwars2.com/images/e/eb/Copper_coin.png";
                UICommon::DrawItemIconCell(id, iconUrl, static_cast<float>(g_Settings.iconSize), st.details.loaded ? st.details.rarity : "");

                // Right-click context menu
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                {
                    UIContextMenu::OpenContextMenu("IgnoredCurrencyContextMenu", id, st.details.loaded ? st.details.name : "");
                }

                ImGui::TableSetColumnIndex(1);
                UICommon::AlignTableCellText(rowH);
                std::string name = st.details.loaded ? st.details.name : (id == 1 ? Localization::GetText("coin") : Localization::GetText("loading"));
                ImGui::Text("%s", name.c_str());

                // Right-click context menu for name
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                {
                    UIContextMenu::OpenContextMenu("IgnoredCurrencyContextMenu", id, st.details.loaded ? st.details.name : "");
                }

                ImGui::TableSetColumnIndex(2);
                // Special display for Coin (ID 1) - show Gold/Silver/Copper with colored icons
                if (id == 1)
                {
                    UICommon::AlignTableCellFrame(rowH);
                    UICommon::DrawCoinDisplay(st.count);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Gold: %lld", st.count);
                }
                else
                {
                    UICommon::AlignTableCellText(rowH);
                    ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                    ImGui::TextColored(countColor, "%lld", st.count);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Count: %lld", st.count);
                }

                ImGui::TableSetColumnIndex(3);
                UICommon::AlignTableCellFrame(rowH);
                bool isIgnored = true;
                if (ImGui::Checkbox(("##unign_cur_" + std::to_string(id)).c_str(), &isIgnored))
                {
                    if (!isIgnored)
                        IgnoredItemsManager::UnignoreCurrency(id);
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", Localization::GetText("unignore_currency"));
            }

            // Context menu popup (rendered once outside the loop)
            UIContextMenu::RenderCurrencyContextMenu("IgnoredCurrencyContextMenu", UIContextMenu::ContextMenuType::Ignored);

            ImGui::EndTable();
        }

        ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
}
}
