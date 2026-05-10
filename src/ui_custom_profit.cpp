#include "ui_custom_profit.h"
#include "settings.h"
#include "custom_profit.h"
#include "item_tracker.h"
#include "ignored_items.h"
#include "localization.h"
#include "ui_common.h"
#include "ui_context_menu.h"
#include <vector>

namespace UICustomProfit
{
void RenderCustomProfitTab()
{
    ImGui::Text("%s", Localization::GetText("custom_profit_system"));
    ImGui::Separator();

    ImGui::Spacing();

    // Clear All Button
    if (ImGui::Button(Localization::GetText("clear_all_custom_profits")))
    {
        CustomProfitManager::ClearAll();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("clear_all_custom_profits_tooltip"));

    ImGui::Spacing();

    // Add Custom Profit for Item
    ImGui::Text("%s", Localization::GetText("add_custom_profit_item"));
    static int itemIdInput = 0;
    static int itemGoldInput = 0;
    static int itemSilverInput = 0;
    static int itemCopperInput = 0;
    
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt("Item ID##AddItem", &itemIdInput, 0)) {}
    
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    ImGui::InputInt("G##AddItemGold", &itemGoldInput, 0);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    ImGui::InputInt("S##AddItemSilver", &itemSilverInput, 0);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    ImGui::InputInt("C##AddItemCopper", &itemCopperInput, 0);

    ImGui::SameLine();
    if (ImGui::Button(Localization::GetText("custom_profit_set_profit")))
    {
        if (itemIdInput > 0)
        {
            long long totalCopper = (long long)itemGoldInput * 10000 + (long long)itemSilverInput * 100 + itemCopperInput;
            CustomProfitManager::SetCustomProfit(itemIdInput, totalCopper, StatType::Item);
        }
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("custom_profit_set_tooltip"));

    ImGui::Spacing();
    ImGui::Separator();

    // Add Custom Profit for Currency
    ImGui::Text("%s", Localization::GetText("add_custom_profit_currency"));
    static int currencyIdInput = 0;
    static int currencyGoldInput = 0;
    static int currencySilverInput = 0;
    static int currencyCopperInput = 0;
    
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt("Currency ID##AddCurrency", &currencyIdInput, 0)) {}
    
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    ImGui::InputInt("G##AddCurrencyGold", &currencyGoldInput, 0);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    ImGui::InputInt("S##AddCurrencySilver", &currencySilverInput, 0);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    ImGui::InputInt("C##AddCurrencyCopper", &currencyCopperInput, 0);

    ImGui::SameLine();
    char setCurrencyProfitButtonLabel[256];
    snprintf(setCurrencyProfitButtonLabel, sizeof(setCurrencyProfitButtonLabel), "%s##Currency", Localization::GetText("custom_profit_set_profit"));
    if (ImGui::Button(setCurrencyProfitButtonLabel))
    {
        if (currencyIdInput > 0)
        {
            long long totalCopper = (long long)currencyGoldInput * 10000 + (long long)currencySilverInput * 100 + currencyCopperInput;
            CustomProfitManager::SetCustomProfit(currencyIdInput, totalCopper, StatType::Currency);
        }
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("custom_profit_set_tooltip"));

    ImGui::Spacing();
    ImGui::Separator();

    // Items with Custom Profit
    ImGui::Text("%s", Localization::GetText("custom_profit_items_header"));
    ImGui::Separator();

    auto allCustomProfitsDetailed = CustomProfitManager::GetAllCustomProfitsDetailed();
    std::vector<std::pair<int, long long>> customProfitItems;
    for (auto& [id, entry] : allCustomProfitsDetailed)
    {
        if (entry.type == StatType::Item)
        {
            customProfitItems.push_back({id, entry.customProfitCopper});
        }
    }

    if (customProfitItems.empty())
    {
        ImGui::TextDisabled("%s", Localization::GetText("no_custom_profit_items"));
    }
    else
    {
        if (ImGui::BeginTable("CustomProfitItemsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn(Localization::GetText("column_icon"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 40.0f);
            ImGui::TableSetupColumn(Localization::GetText("column_name"), ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoHide, 150.0f);
            ImGui::TableSetupColumn(Localization::GetText("custom_profit_value"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 150.0f);
            ImGui::TableSetupColumn(Localization::GetText("custom_profit_remove"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 100.0f);
            ImGui::TableHeadersRow();

            for (auto& [id, profit] : customProfitItems)
            {
                Stat st = ItemTracker::GetItemStat(id);

                float rowH = UICommon::CalcTableRowHeight(32.0f);
                ImGui::TableNextRow(0, rowH);

                ImGui::TableSetColumnIndex(0);
                UICommon::AlignTableCellIcon(rowH, 32.0f);
                UICommon::DrawItemIconCell(id, st.details.iconUrl, 32.0f, st.details.loaded ? st.details.rarity : "");

                // Right-click context menu
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                {
                    UIContextMenu::OpenContextMenu("CustomProfitItemContextMenu", id, st.details.loaded ? st.details.name : "");
                }

                ImGui::TableSetColumnIndex(1);
                UICommon::AlignTableCellText(rowH);
                ImGui::Text("%s", st.details.loaded ? st.details.name.c_str() : "Loading...");

                // Right-click context menu for name
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                {
                    UIContextMenu::OpenContextMenu("CustomProfitItemContextMenu", id, st.details.loaded ? st.details.name : "");
                }

                ImGui::TableSetColumnIndex(2);
                UICommon::AlignTableCellText(rowH);
                ImGui::Text("%s", UICommon::FormatCoin(profit).c_str());

                ImGui::TableSetColumnIndex(3);
                UICommon::AlignTableCellFrame(rowH);
                char removeButtonLabel[256];
                snprintf(removeButtonLabel, sizeof(removeButtonLabel), "Remove##%d", id);
                if (ImGui::Button(removeButtonLabel))
                {
                    CustomProfitManager::RemoveCustomProfit(id);
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", Localization::GetText("custom_profit_remove_tooltip"));
            }

            // Context menu popup (rendered once outside the loop)
            UIContextMenu::RenderItemContextMenu("CustomProfitItemContextMenu", UIContextMenu::ContextMenuType::CustomProfit);

            ImGui::EndTable();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    // Currencies with Custom Profit
    ImGui::Text("%s", Localization::GetText("custom_profit_currencies_header"));
    ImGui::Separator();

    std::vector<std::pair<int, long long>> customProfitCurrencies;
    for (auto& [id, entry] : allCustomProfitsDetailed)
    {
        if (entry.type == StatType::Currency)
        {
            customProfitCurrencies.push_back({id, entry.customProfitCopper});
        }
    }

    if (customProfitCurrencies.empty())
    {
        ImGui::TextDisabled("%s", Localization::GetText("no_custom_profit_currencies"));
    }
    else
    {
        if (ImGui::BeginTable("CustomProfitCurrenciesTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn(Localization::GetText("column_icon"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 40.0f);
            ImGui::TableSetupColumn(Localization::GetText("column_name"), ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoHide, 150.0f);
            ImGui::TableSetupColumn(Localization::GetText("custom_profit_value"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 150.0f);
            ImGui::TableSetupColumn(Localization::GetText("custom_profit_remove"), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 100.0f);
            ImGui::TableHeadersRow();

            for (auto& [id, profit] : customProfitCurrencies)
            {
                Stat st = ItemTracker::GetCurrencyStat(id);

                float rowH = UICommon::CalcTableRowHeight(32.0f);
                ImGui::TableNextRow(0, rowH);

                ImGui::TableSetColumnIndex(0);
                UICommon::AlignTableCellIcon(rowH, 32.0f);
                std::string iconUrl = st.details.iconUrl;
                if (id == 1 && iconUrl.empty())
                    iconUrl = "https://wiki.guildwars2.com/images/e/eb/Copper_coin.png";
                UICommon::DrawItemIconCell(id, iconUrl, 32.0f, st.details.loaded ? st.details.rarity : "");

                // Right-click context menu
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                {
                    UIContextMenu::OpenContextMenu("CustomProfitCurrencyContextMenu", id, st.details.loaded ? st.details.name : "");
                }

                ImGui::TableSetColumnIndex(1);
                UICommon::AlignTableCellText(rowH);
                ImGui::Text("%s", st.details.loaded ? st.details.name.c_str() : (id == 1 ? Localization::GetText("coin") : "Loading..."));

                // Right-click context menu for name
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                {
                    UIContextMenu::OpenContextMenu("CustomProfitCurrencyContextMenu", id, st.details.loaded ? st.details.name : "");
                }

                ImGui::TableSetColumnIndex(2);
                UICommon::AlignTableCellText(rowH);
                ImGui::Text("%s", UICommon::FormatCoin(profit).c_str());

                ImGui::TableSetColumnIndex(3);
                UICommon::AlignTableCellFrame(rowH);
                char removeButtonLabel[256];
                snprintf(removeButtonLabel, sizeof(removeButtonLabel), "Remove##%d", id);
                if (ImGui::Button(removeButtonLabel))
                {
                    CustomProfitManager::RemoveCustomProfit(id);
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", Localization::GetText("custom_profit_remove_tooltip"));
            }

            // Context menu popup (rendered once outside the loop)
            UIContextMenu::RenderCurrencyContextMenu("CustomProfitCurrencyContextMenu", UIContextMenu::ContextMenuType::CustomProfit);

            ImGui::EndTable();
        }
    }
}
}
