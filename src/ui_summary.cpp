#include "ui_summary.h"
#include "settings.h"
#include "item_tracker.h"
#include "drf_client.h"
#include "shared.h"
#include "localization.h"
#include "ignored_items.h"
#include "ui_context_menu.h"
#include "auto_reset.h"
#include <chrono>

namespace UISummary
{
static void RenderProfitSparkline()
{
    if (!g_Settings.showProfitSparkline) return;

    auto history = ItemTracker::GetProfitHistory();
    if (history.size() < 2) return;

    // Extract profit values for plotting
    std::vector<float> values;
    for (const auto& entry : history)
    {
        values.push_back(static_cast<float>(entry.second));
    }

    if (values.empty()) return;

    // Calculate min/max for scaling
    float minVal = *std::min_element(values.begin(), values.end());
    float maxVal = *std::max_element(values.begin(), values.end());

    // Draw sparkline
    ImGui::SameLine();
    float sparklineWidth = 200.0f; // Double the original 100.0f
    float sparklineHeight = 30.0f; // Slightly taller
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImVec2 p_min = cursor;
    ImVec2 p_max = ImVec2(cursor.x + sparklineWidth, cursor.y + sparklineHeight);

    // Draw background
    drawList->AddRectFilled(p_min, p_max, IM_COL32(30, 30, 30, 150), 4.0f); // Rounded corners
    drawList->AddRect(p_min, p_max, IM_COL32(100, 100, 100, 100), 4.0f); // Border

    // Draw line
    if (values.size() > 1)
    {
        for (size_t i = 0; i < values.size() - 1; i++)
        {
            float x1 = p_min.x + (static_cast<float>(i) / (values.size() - 1)) * sparklineWidth;
            float x2 = p_min.x + (static_cast<float>(i + 1) / (values.size() - 1)) * sparklineWidth;
            
            float y1, y2;
            if (maxVal != minVal)
            {
                y1 = p_max.y - ((values[i] - minVal) / (maxVal - minVal)) * sparklineHeight;
                y2 = p_max.y - ((values[i + 1] - minVal) / (maxVal - minVal)) * sparklineHeight;
            }
            else
            {
                y1 = p_min.y + sparklineHeight / 2;
                y2 = p_min.y + sparklineHeight / 2;
            }

            ImU32 color = values[i+1] >= values[i] ? IM_COL32(100, 255, 100, 255) : IM_COL32(255, 100, 100, 255);
            drawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), color, 2.0f);
        }
    }

    // Interactive Hover
    ImGui::SetCursorScreenPos(p_min);
    ImGui::InvisibleButton("Sparkline", ImVec2(sparklineWidth, sparklineHeight));
    if (ImGui::IsItemHovered())
    {
        ImVec2 mousePos = ImGui::GetMousePos();
        float relativeX = (mousePos.x - p_min.x) / sparklineWidth;
        int index = static_cast<int>(relativeX * (values.size() - 1) + 0.5f);
        index = std::clamp(index, 0, (int)values.size() - 1);

        // Draw vertical line at hover
        float hoverX = p_min.x + (static_cast<float>(index) / (values.size() - 1)) * sparklineWidth;
        drawList->AddLine(ImVec2(hoverX, p_min.y), ImVec2(hoverX, p_max.y), IM_COL32(255, 255, 255, 150), 1.0f);

        ImGui::BeginTooltip();
        auto timeSince = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - history[index].first);
        ImGui::Text("%s: %s", Localization::GetText("column_value"), UICommon::FormatCoin(history[index].second).c_str());
        ImGui::Text(Localization::GetText("time_ago_seconds"), timeSince.count());
        ImGui::EndTooltip();
    }

    ImGui::SetCursorScreenPos(ImVec2(p_max.x, cursor.y));
}

void RenderSummaryTab()
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::Indent(4.0f);

    // DRF Status Warning
    DrfStatus status = DrfClient::GetStatus();
    if (status == DrfStatus::Disconnected)
    {
        ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f), "%s", Localization::GetText("warning_drf_not_connected"));
        ImGui::TextDisabled("%s", Localization::GetText("warning_drf_not_connected_desc"));
        ImGui::TextDisabled("%s", Localization::GetText("warning_drf_install"));
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }
    else if (status == DrfStatus::AuthFailed)
    {
        ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f), "%s", Localization::GetText("warning_drf_token_invalid"));
        ImGui::TextDisabled("%s", Localization::GetText("warning_drf_token_invalid_desc"));
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    // API Keys Warning
    if (g_Settings.gw2ApiKey.empty())
    {
        ImGui::TextColored(ImVec4(1.f, 0.6f, 0.0f, 1.f), "%s", Localization::GetText("warning_gw2_api_key_not_set"));
        ImGui::TextDisabled("%s", Localization::GetText("warning_gw2_api_key_not_set_desc"));
        ImGui::Spacing();
        ImGui::Separator();
    }

    // Overview
    ImGui::Text("%s", Localization::GetText("overview_label"));
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("overview_tooltip"));
    ImGui::Separator();

    char overviewTableId[64];
    snprintf(overviewTableId, sizeof(overviewTableId), "OverviewStatsTable_v4");
    if (ImGui::BeginTable(overviewTableId, 2, UICommon::DataTableFlags()))
    {
        UICommon::TableColumnFixedAuto(Localization::GetText("column_label"), ImGuiTableColumnFlags_NoHide);
        UICommon::TableColumnStretchAuto(Localization::GetText("column_value"));
        ImGui::TableHeadersRow();
        float overviewRowH = UICommon::CalcTableRowHeight(0.0f);

        long long totalProfit = ItemTracker::CalcTotalCustomProfit();
        auto duration = ItemTracker::GetSessionDuration();
        long long profitPerHour = ItemTracker::GetTotalProfitPerHour(duration);
        auto items = ItemTracker::GetItemsCopy();
        auto currencies = ItemTracker::GetCurrenciesCopy();

        // Total Profit
        ImGui::TableNextRow(0, overviewRowH);
        ImGui::TableSetColumnIndex(0);
        UICommon::AlignTableCellText(overviewRowH);
        ImGui::Text("%s", Localization::GetText("total_profit_label_simple"));
        ImGui::TableSetColumnIndex(1);
        UICommon::AlignTableCellText(overviewRowH);
        ImVec4 profitColor = totalProfit > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (totalProfit < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
        ImGui::TextColored(profitColor, "%s", UICommon::FormatCoin(totalProfit).c_str());
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("total_profit_tooltip"));

        // Total Items
        ImGui::TableNextRow(0, overviewRowH);
        ImGui::TableSetColumnIndex(0);
        UICommon::AlignTableCellText(overviewRowH);
        ImGui::Text("%s", Localization::GetText("total_items_label_simple"));
        ImGui::TableSetColumnIndex(1);
        UICommon::AlignTableCellText(overviewRowH);
        size_t totalItems = items.size();
        ImVec4 totalItemsColor = totalItems > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (totalItems < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
        ImGui::TextColored(totalItemsColor, "%zu", totalItems);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("total_items_tooltip"));

        // Total Currencies
        ImGui::TableNextRow(0, overviewRowH);
        ImGui::TableSetColumnIndex(0);
        UICommon::AlignTableCellText(overviewRowH);
        ImGui::Text("%s", Localization::GetText("total_currencies_label_simple"));
        ImGui::TableSetColumnIndex(1);
        UICommon::AlignTableCellText(overviewRowH);
        size_t totalCurrencies = currencies.size();
        ImVec4 totalCurrenciesColor = totalCurrencies > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (totalCurrencies < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
        ImGui::TextColored(totalCurrenciesColor, "%zu", totalCurrencies);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("total_currencies_tooltip"));

        // Profit Per Hour
        ImGui::TableNextRow(0, overviewRowH);
        ImGui::TableSetColumnIndex(0);
        UICommon::AlignTableCellText(overviewRowH);
        ImGui::Text("%s", Localization::GetText("profit_per_hour_label_simple"));
        ImGui::TableSetColumnIndex(1);
        UICommon::AlignTableCellText(overviewRowH);
        ImVec4 profitPerHourColor = profitPerHour > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (profitPerHour < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
        ImGui::TextColored(profitPerHourColor, "%s", UICommon::FormatCoin(profitPerHour).c_str());
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("profit_per_hour_tooltip"));
        RenderProfitSparkline();

        // Session Duration
        ImGui::TableNextRow(0, overviewRowH);
        ImGui::TableSetColumnIndex(0);
        UICommon::AlignTableCellText(overviewRowH);
        ImGui::Text("%s", Localization::GetText("session_duration_label_simple"));
        ImGui::TableSetColumnIndex(1);
        UICommon::AlignTableCellText(overviewRowH);
        ImGui::Text("%s", UICommon::FormatDuration(duration.count()).c_str());
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("session_duration_tooltip"));

        // Next Auto-Reset
        ImGui::TableNextRow(0, overviewRowH);
        ImGui::TableSetColumnIndex(0);
        UICommon::AlignTableCellText(overviewRowH);
        ImGui::Text("%s", Localization::GetText("next_reset_label_simple"));
        ImGui::TableSetColumnIndex(1);
        UICommon::AlignTableCellText(overviewRowH);
        char nextResetDisplay[256];
        snprintf(nextResetDisplay, sizeof(nextResetDisplay), "%s", AutoReset::GetNextResetDisplayUtc().c_str());
        ImGui::Text("%s", nextResetDisplay);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("next_reset_tooltip"));

        // TP Sell Listings Profit
        ImGui::TableNextRow(0, overviewRowH);
        ImGui::TableSetColumnIndex(0);
        UICommon::AlignTableCellText(overviewRowH);
        ImGui::Text("%s", Localization::GetText("approx_trading_profits_listings_label"));
        ImGui::TableSetColumnIndex(1);
        UICommon::AlignTableCellText(overviewRowH);
        long long tpSell = ItemTracker::CalcTotalTpSellProfit();
        ImVec4 tpSellColor = tpSell > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (tpSell < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
        ImGui::TextColored(tpSellColor, "%s", UICommon::FormatCoin(tpSell).c_str());
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("approx_trading_profits_listings_tooltip"));

        // TP Instant Sell Profit
        ImGui::TableNextRow(0, overviewRowH);
        ImGui::TableSetColumnIndex(0);
        UICommon::AlignTableCellText(overviewRowH);
        ImGui::Text("%s", Localization::GetText("approx_trading_profits_instant_label"));
        ImGui::TableSetColumnIndex(1);
        UICommon::AlignTableCellText(overviewRowH);
        long long tpInstant = ItemTracker::CalcTotalTpInstantProfit();
        ImVec4 tpInstantColor = tpInstant > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (tpInstant < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
        ImGui::TextColored(tpInstantColor, "%s", UICommon::FormatCoin(tpInstant).c_str());
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("approx_trading_profits_instant_tooltip"));

        // Efficiency Score
        if (tpSell > 0)
        {
            float efficiency = (static_cast<float>(tpInstant) / static_cast<float>(tpSell)) * 100.0f;
            ImGui::TableNextRow(0, overviewRowH);
            ImGui::TableSetColumnIndex(0);
            UICommon::AlignTableCellText(overviewRowH);
            ImGui::Text("%s", Localization::GetText("efficiency_score"));
            ImGui::TableSetColumnIndex(1);
            UICommon::AlignTableCellText(overviewRowH);

            ImVec4 effColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            if (efficiency >= 95.0f) effColor = ImVec4(0.0f, 1.0f, 0.5f, 1.0f); // Bright Green
            else if (efficiency >= 85.0f) effColor = ImVec4(0.5f, 1.0f, 0.0f, 1.0f); // Lime
            else if (efficiency >= 70.0f) effColor = ImVec4(1.0f, 0.8f, 0.0f, 1.0f); // Gold/Orange
            else effColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Red

            ImGui::TextColored(effColor, "%.1f%%", efficiency);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(Localization::GetText("efficiency_score_desc"), efficiency);
        }

        // Magic Find
        ImGui::TableNextRow(0, overviewRowH);
        ImGui::TableSetColumnIndex(0);
        UICommon::AlignTableCellText(overviewRowH);
        ImGui::Text("%s", Localization::GetText("magic_find"));
        ImGui::TableSetColumnIndex(1);
        UICommon::AlignTableCellText(overviewRowH);
        int mf = ItemTracker::GetMagicFind();
        if (mf >= 0)
        {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%d%%", mf);
        }
        else
        {
            ImGui::TextDisabled("N/A");
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("magic_find_tooltip"));

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Spacing();

    // Top Items by Profit (Table)
    char topItemsProfitHeader[256];
    snprintf(topItemsProfitHeader, sizeof(topItemsProfitHeader), "%s##TopItemsProfitHeader", Localization::GetText("top_items_profit_header"));
    if (ImGui::CollapsingHeader(topItemsProfitHeader, g_Settings.showTopItems ? ImGuiTreeNodeFlags_DefaultOpen : 0))
    {
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("top_items_profit_tooltip"));

        char topItemsProfitId[64];
        snprintf(topItemsProfitId, sizeof(topItemsProfitId), "TopItemsProfitTable_v5");
        if (ImGui::BeginTable(topItemsProfitId, 4, UICommon::DataTableFlags()))
        {
            UICommon::TableColumnFixedAuto(Localization::GetText("column_icon"), ImGuiTableColumnFlags_NoHide);
            UICommon::TableColumnFixedAuto(Localization::GetText("column_item"), ImGuiTableColumnFlags_NoHide);
            UICommon::TableColumnFixedAuto(Localization::GetText("column_count"), ImGuiTableColumnFlags_NoHide);
            UICommon::TableColumnStretchAuto(Localization::GetText("column_profit"));
            ImGui::TableHeadersRow();

            auto sortedByProfit = ItemTracker::GetSortedItems(ItemTracker::SortMode::ProfitDesc);
            int count = 0;
            for (auto& [id, st] : sortedByProfit)
            {
                if (count >= 10) break;
                if (st.count == 0) continue;
                long long profit = ItemTracker::GetStatProfit(st);
                if (profit <= 0) continue;

                float rowH = UICommon::CalcTableRowHeight(static_cast<float>(g_Settings.iconSize));
                ImGui::TableNextRow(0, rowH);
                
                // Icon
                ImGui::TableSetColumnIndex(0);
                UICommon::AlignTableCellIcon(rowH, static_cast<float>(g_Settings.iconSize));
                UICommon::DrawItemIconCell(st.apiId, st.details.iconUrl, static_cast<float>(g_Settings.iconSize), st.details.loaded ? st.details.rarity : "");

                // Name
                ImGui::TableSetColumnIndex(1);
                UICommon::AlignTableCellText(rowH);
                std::string name = st.details.loaded ? st.details.name : Localization::GetText("loading");
                ImGui::Text("%s", name.c_str());

                // Right-click context menu for top items
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                {
                    UIContextMenu::OpenContextMenu("TopItemContextMenu", id, st.details.loaded ? st.details.name : "");
                }

                ImGui::TableSetColumnIndex(2);
                UICommon::AlignTableCellText(rowH);
                ImGui::Text("%lld", st.count);
                ImGui::TableSetColumnIndex(3);
                UICommon::AlignTableCellText(rowH);
                ImGui::TextColored(ImVec4(1.f, 0.84f, 0.f, 1.f), "%s", UICommon::FormatCoin(profit).c_str());
                count++;
            }

            // Context menu popup (rendered once outside the loop)
            UIContextMenu::RenderItemContextMenu("TopItemContextMenu", UIContextMenu::ContextMenuType::General);

            ImGui::EndTable();
        }
    }
    if (ImGui::IsItemClicked())
        SettingsManager::Save();

    ImGui::Spacing();
    char topItemsCountHeader[256];
    snprintf(topItemsCountHeader, sizeof(topItemsCountHeader), "%s##TopItemsHeader", Localization::GetText("top_items_count_header"));
    if (ImGui::CollapsingHeader(topItemsCountHeader, g_Settings.showTopItems ? ImGuiTreeNodeFlags_DefaultOpen : 0))
    {
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("top_items_count_tooltip"));

        char topItemsCountId[64];
        snprintf(topItemsCountId, sizeof(topItemsCountId), "TopItemsCountTable_v5");
        if (ImGui::BeginTable(topItemsCountId, 3, UICommon::DataTableFlags()))
        {
            UICommon::TableColumnFixedAuto(Localization::GetText("column_icon"), ImGuiTableColumnFlags_NoHide);
            UICommon::TableColumnFixedAuto(Localization::GetText("column_item"), ImGuiTableColumnFlags_NoHide);
            UICommon::TableColumnStretchAuto(Localization::GetText("column_count"));
            ImGui::TableHeadersRow();

            auto sortedByCount = ItemTracker::GetSortedItems(ItemTracker::SortMode::CountDesc);
            int count = 0;
            for (auto& [id, st] : sortedByCount)
            {
                if (count >= 10) break;
                if (st.count <= 0) continue;

                float rowH = UICommon::CalcTableRowHeight(static_cast<float>(g_Settings.iconSize));
                ImGui::TableNextRow(0, rowH);
                
                // Icon
                ImGui::TableSetColumnIndex(0);
                UICommon::AlignTableCellIcon(rowH, static_cast<float>(g_Settings.iconSize));
                UICommon::DrawItemIconCell(st.apiId, st.details.iconUrl, static_cast<float>(g_Settings.iconSize), st.details.loaded ? st.details.rarity : "");

                // Name
                ImGui::TableSetColumnIndex(1);
                UICommon::AlignTableCellText(rowH);
                std::string name = st.details.loaded ? st.details.name : Localization::GetText("loading");
                ImGui::Text("%s", name.c_str());

                ImGui::TableSetColumnIndex(2);
                UICommon::AlignTableCellText(rowH);
                ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                ImGui::TextColored(countColor, "%lld", st.count);
                count++;
            }
            ImGui::EndTable();
        }
    }
    if (ImGui::IsItemClicked())
        SettingsManager::Save();

    ImGui::Spacing();
    char topCurrenciesCountHeader[256];
    snprintf(topCurrenciesCountHeader, sizeof(topCurrenciesCountHeader), "%s##TopCurrenciesHeader", Localization::GetText("top_currencies_count_header"));
    if (ImGui::CollapsingHeader(topCurrenciesCountHeader, g_Settings.showTopCurrencies ? ImGuiTreeNodeFlags_DefaultOpen : 0))
    {
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("top_currencies_count_tooltip"));

        char topCurrenciesCountId[64];
        snprintf(topCurrenciesCountId, sizeof(topCurrenciesCountId), "TopCurrenciesCountTable_v5");
        if (ImGui::BeginTable(topCurrenciesCountId, 3, UICommon::DataTableFlags()))
        {
            UICommon::TableColumnFixedAuto(Localization::GetText("column_icon"), ImGuiTableColumnFlags_NoHide);
            UICommon::TableColumnFixedAuto(Localization::GetText("column_currency"), ImGuiTableColumnFlags_NoHide);
            UICommon::TableColumnStretchAuto(Localization::GetText("column_count"));
            ImGui::TableHeadersRow();

            auto sortedCurrencies = ItemTracker::GetSortedCurrencies(ItemTracker::SortMode::CountDesc);
            int count = 0;
            for (auto& [id, st] : sortedCurrencies)
            {
                if (count >= 10) break;
                if (st.count <= 0) continue;

                float rowH = UICommon::CalcTableRowHeight(static_cast<float>(g_Settings.iconSize));
                ImGui::TableNextRow(0, rowH);
                
                // Icon
                ImGui::TableSetColumnIndex(0);
                UICommon::AlignTableCellIcon(rowH, static_cast<float>(g_Settings.iconSize));
                UICommon::DrawItemIconCell(st.apiId, st.details.iconUrl, static_cast<float>(g_Settings.iconSize), "");

                // Name
                ImGui::TableSetColumnIndex(1);
                UICommon::AlignTableCellText(rowH);
                std::string name = st.details.loaded ? st.details.name : (id == 1 ? Localization::GetText("coin") : Localization::GetText("loading"));
                ImGui::Text("%s", name.c_str());
                
                ImGui::TableSetColumnIndex(2);
                UICommon::AlignTableCellText(rowH);
                ImVec4 countColor = st.count > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (st.count < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
                ImGui::TextColored(countColor, "%lld", st.count);
                count++;
            }
            ImGui::EndTable();
        }
    }

    // Best Drop of Current Session (Trophy Style)
    auto bestDrop = ItemTracker::GetBestDrop();
    if (bestDrop.first != 0 && bestDrop.second.count > 0)
    {
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Separator();
        
        ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "--- %s ---", Localization::GetText("best_drop"));
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("best_drop_tooltip"));
        
        ImGui::Spacing();
        
        ImGui::BeginGroup();
        // Larger Icon with golden border
        float trophySize = 64.0f;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        UICommon::DrawItemIconCell(bestDrop.first, bestDrop.second.details.iconUrl, trophySize, bestDrop.second.details.rarity);
        
        // Golden Frame (Trophy effect)
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRect(pos, ImVec2(pos.x + trophySize, pos.y + trophySize), IM_COL32(255, 215, 0, 255), 4.0f, 0, 2.0f);
        
        ImGui::SameLine(0, 15);
        ImGui::BeginGroup();
        long long bestDropProfit = ItemTracker::GetStatProfit(bestDrop.second);
        ImGui::Text("%s", bestDrop.second.details.loaded ? bestDrop.second.details.name.c_str() : "Loading...");
        ImGui::TextColored(ImVec4(1.f, 0.84f, 0.f, 1.f), "%s", UICommon::FormatCoin(bestDropProfit).c_str());
        ImGui::Text("%s: %lld", Localization::GetText("count_label"), bestDrop.second.count);
        ImGui::EndGroup();
        ImGui::EndGroup();
        
        ImGui::Separator();
    }
    if (ImGui::IsItemClicked())
        SettingsManager::Save();

    ImGui::Spacing();
    ImGui::Spacing();

    // No Data Warning
    auto items = ItemTracker::GetItemsCopy();
    auto currencies = ItemTracker::GetCurrenciesCopy();
    if (items.empty() && currencies.empty())
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.f, 0.6f, 0.0f, 1.f), "%s", Localization::GetText("warning_no_data"));
        ImGui::TextDisabled("%s", Localization::GetText("warning_no_data_desc"));
    }

    // Export Buttons
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("%s", Localization::GetText("export_label"));
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("export_tooltip"));
    char exportJsonLabel[256];
    snprintf(exportJsonLabel, sizeof(exportJsonLabel), "%s", Localization::GetText("export_json"));
    if (ImGui::Button(exportJsonLabel))
    {
        std::string jsonData = ItemTracker::ExportToJson();
        const char* addonDir = APIDefs ? APIDefs->Paths_GetAddonDirectory("FarmingTracker") : "";
        std::string filename = std::string(addonDir ? addonDir : "") + "\\FarmingTracker_Export_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".json";

        // Use standard file operations to save
        FILE* file = nullptr;
        errno_t err = fopen_s(&file, filename.c_str(), "w");
        if (err == 0 && file)
        {
            if (fprintf(file, "%s", jsonData.c_str()) < 0)
            {
                // Handle write error
            }
            fclose(file);
        }
    }
    ImGui::SameLine();
    char exportCsvLabel[256];
    snprintf(exportCsvLabel, sizeof(exportCsvLabel), "%s", Localization::GetText("export_csv"));
    if (ImGui::Button(exportCsvLabel))
    {
        std::string csvData = ItemTracker::ExportToCsv();
        const char* addonDir = APIDefs ? APIDefs->Paths_GetAddonDirectory("FarmingTracker") : "";
        std::string filename = std::string(addonDir ? addonDir : "") + "\\FarmingTracker_Export_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".csv";

        // Use standard file operations to save
        FILE* file = nullptr;
        errno_t err = fopen_s(&file, filename.c_str(), "w");
        if (err == 0 && file)
        {
            if (fprintf(file, "%s", csvData.c_str()) < 0)
            {
                // Handle write error
            }
            fclose(file);
        }
    }

    ImGui::Unindent(4.0f);
    ImGui::PopStyleVar();
}
}
