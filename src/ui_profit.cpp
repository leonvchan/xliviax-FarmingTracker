#include "ui_profit.h"
#include "settings.h"
#include "item_tracker.h"
#include "localization.h"
#include "session_history.h"
#include "ui_common.h"

namespace UIProfit
{
static int s_SummaryPeriod = 0; // 0 = Today, 1 = This Week, 2 = This Month
void RenderProfitTab()
{
    long long tpSell = ItemTracker::CalcTotalTpSellProfit();
    long long tpInstant = ItemTracker::CalcTotalTpInstantProfit();
    long long custom = ItemTracker::CalcTotalCustomProfit();

    ImGui::Separator();
    ImGui::Text("%s", Localization::GetText("profits_label"));
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("profits_tooltip"));

    // Summaries Checkbox (only if Session History is enabled)
    if (g_Settings.enableSessionHistory)
    {
        ImGui::SameLine();
        if (ImGui::Checkbox(Localization::GetText("show_summaries"), &g_Settings.enableSummariesInProfitTab))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("show_summaries_tooltip"));
    }

    ImGui::Separator();

    ImGui::Text("%s", Localization::GetText("approx_profits_label"));
    ImGui::SameLine();
    ImVec4 customColor = custom > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (custom < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
    ImGui::TextColored(customColor, "%s", UICommon::FormatCoin(custom).c_str());
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("approx_profits_tooltip"));

    ImGui::Spacing();
    ImGui::Text("%s", Localization::GetText("approx_gold_per_hour_label"));
    ImGui::SameLine();
    auto duration = ItemTracker::GetSessionDuration();
    long long seconds = duration.count();
    long long profitPerHour = ItemTracker::GetTotalProfitPerHour(duration);
    ImVec4 profitPerHourColor = profitPerHour > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (profitPerHour < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
    ImGui::TextColored(profitPerHourColor, "%s", UICommon::FormatCoin(profitPerHour).c_str());
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("approx_gold_per_hour_tooltip"));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("%s", Localization::GetText("trading_profits_label"));
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("trading_profits_tooltip"));
    ImGui::Separator();

    ImGui::Text("%s", Localization::GetText("approx_trading_profits_listings_label"));
    ImGui::SameLine();
    ImVec4 tpSellColor = tpSell > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (tpSell < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
    ImGui::TextColored(tpSellColor, "%s", UICommon::FormatCoin(tpSell).c_str());
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("approx_trading_profits_listings_tooltip"));

    ImGui::Spacing();
    ImGui::Text("%s", Localization::GetText("approx_trading_profits_instant_label"));
    ImGui::SameLine();
    ImVec4 tpInstantColor = tpInstant > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (tpInstant < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
    ImGui::TextColored(tpInstantColor, "%s", UICommon::FormatCoin(tpInstant).c_str());
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("approx_trading_profits_instant_tooltip"));

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("%s", Localization::GetText("trading_details_label"));
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("trading_details_tooltip"));

    ImGui::Spacing();
    ImGui::Text("%s", Localization::GetText("lost_profit_vs_tp_sell_label"));
    ImGui::SameLine();
    long long opportunityCost = ItemTracker::GetOpportunityCostProfit();
    ImVec4 ocColor = opportunityCost > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (opportunityCost < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
    ImGui::TextColored(ocColor, "%s", UICommon::FormatCoin(opportunityCost).c_str());
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("lost_profit_vs_tp_sell_tooltip"));

    ImGui::Spacing();
    ImGui::Text("%s", Localization::GetText("lost_profit_per_hour_vs_tp_sell_label"));
    ImGui::SameLine();
    long long opportunityCostPerHour = ItemTracker::GetOpportunityCostProfitPerHour(duration);
    ImVec4 ocPerHourColor = opportunityCostPerHour > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (opportunityCostPerHour < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
    ImGui::TextColored(ocPerHourColor, "%s", UICommon::FormatCoin(opportunityCostPerHour).c_str());
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("lost_profit_per_hour_vs_tp_sell_tooltip"));

    // Efficiency Score (Opportunity Cost as percentage)
    if (tpSell > 0)
    {
        float efficiency = (static_cast<float>(tpInstant) / static_cast<float>(tpSell)) * 100.0f;
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImVec4 effColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        if (efficiency >= 95.0f) effColor = ImVec4(0.0f, 1.0f, 0.5f, 1.0f); // Bright Green
        else if (efficiency >= 85.0f) effColor = ImVec4(0.5f, 1.0f, 0.0f, 1.0f); // Lime
        else if (efficiency >= 70.0f) effColor = ImVec4(1.0f, 0.8f, 0.0f, 1.0f); // Gold/Orange
        else effColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Red

        ImGui::Text("%s", Localization::GetText("efficiency_score_label"));
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("efficiency_score_tooltip"));
        ImGui::SameLine();
        ImGui::TextColored(effColor, "%.1f%%", efficiency);

        ImGui::PushStyleColor(ImGuiCol_Text, effColor);
        ImGui::Text(Localization::GetText("efficiency_score_desc"), efficiency);
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Text("%s", Localization::GetText("magic_find"));
        ImGui::SameLine();
        int mf = ItemTracker::GetMagicFind();
        if (mf >= 0)
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%d%%", mf);
        else
            ImGui::TextDisabled("N/A");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("magic_find_tooltip"));
    }

    if (seconds > 0)
    {
        ImGui::Spacing();
        ImGui::Separator();
        char sessionDurationLabel[256];
        snprintf(sessionDurationLabel, sizeof(sessionDurationLabel), Localization::GetText("session_duration_debug_label"), UICommon::FormatDuration(seconds).c_str());
        ImGui::Text("%s", sessionDurationLabel);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("session_duration_debug_tooltip"));
    }

    // Best Drop of Current Session (Trophy Style) - Moved up
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
        
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", bestDrop.second.details.name.c_str());
        ImGui::Text("%s: %lld", Localization::GetText("column_count"), bestDrop.second.count);
        ImGui::Text("%s: %s", Localization::GetText("value"), UICommon::FormatCoin(ItemTracker::GetStatProfit(bestDrop.second)).c_str());
        ImGui::EndGroup();
        ImGui::EndGroup();
    }

    // Summaries Section
    if (g_Settings.enableSessionHistory && g_Settings.enableSummariesInProfitTab)
    {
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("%s", Localization::GetText("summaries_label"));
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("summaries_tooltip"));
        ImGui::Separator();

        // Period Selection
        ImGui::Text("%s", Localization::GetText("summary_period"));
        ImGui::SameLine();
        if (ImGui::RadioButton(Localization::GetText("summary_today"), s_SummaryPeriod == 0))
            s_SummaryPeriod = 0;
        ImGui::SameLine();
        if (ImGui::RadioButton(Localization::GetText("summary_this_week"), s_SummaryPeriod == 1))
            s_SummaryPeriod = 1;
        ImGui::SameLine();
        if (ImGui::RadioButton(Localization::GetText("summary_this_month"), s_SummaryPeriod == 2))
            s_SummaryPeriod = 2;

        ImGui::Spacing();
        ImGui::Separator();

        // Get summary data
        SessionHistory::SummaryPeriod period;
        switch (s_SummaryPeriod)
        {
        case 0: period = SessionHistory::SummaryPeriod::Today; break;
        case 1: period = SessionHistory::SummaryPeriod::ThisWeek; break;
        case 2: period = SessionHistory::SummaryPeriod::ThisMonth; break;
        }
        auto summary = SessionHistory::GetSummary(period);

        // Display summary stats
        ImGui::Text("%s: %s", Localization::GetText("total_profit"), UICommon::FormatCoin(summary.totalProfit).c_str());
        ImGui::Text("%s: %s", Localization::GetText("profit_per_hour"), UICommon::FormatCoin(summary.profitPerHour).c_str());
        ImGui::Text("%s: %d", Localization::GetText("total_drops"), summary.totalDrops);
        ImGui::Text("%s: %d", Localization::GetText("session_count"), summary.sessionCount);
        ImGui::Text("%s: %s", Localization::GetText("total_duration"), UICommon::FormatDuration(summary.totalDurationSeconds).c_str());

        // Comparison with previous period
        if (summary.previousPeriodProfit != 0)
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("%s", Localization::GetText("comparison_previous_period"));
            long long diff = summary.totalProfit - summary.previousPeriodProfit;
            float percentChange = summary.previousPeriodProfit != 0 ? ((float)diff / summary.previousPeriodProfit) * 100.0f : 0.0f;
            ImVec4 diffColor = diff > 0 ? ImVec4(0.f, 1.f, 0.f, 1.f) : (diff < 0 ? ImVec4(1.f, 0.f, 0.f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
            ImGui::TextColored(diffColor, "%s: %s (%.1f%%)", Localization::GetText("profit_change"), UICommon::FormatCoin(diff).c_str(), percentChange);
        }

        // Top Drops
        if (!summary.topDrops.empty())
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("%s", Localization::GetText("top_drops"));
            if (ImGui::BeginTable("ProfitTopDrops_v2", 4, UICommon::DataTableFlags()))
            {
                UICommon::TableColumnFixedAuto(Localization::GetText("column_icon"), ImGuiTableColumnFlags_NoHide);
                UICommon::TableColumnFixedAuto(Localization::GetText("item"), ImGuiTableColumnFlags_NoHide);
                UICommon::TableColumnFixedAuto(Localization::GetText("count"), ImGuiTableColumnFlags_NoHide);
                UICommon::TableColumnStretchAuto(Localization::GetText("value"));
                ImGui::TableHeadersRow();

                for (const auto& drop : summary.topDrops)
                {
                    if (drop.totalValue <= 0) continue;
                    float rowH = UICommon::CalcTableRowHeight(static_cast<float>(g_Settings.iconSize));
                    ImGui::TableNextRow(0, rowH);
                    
                    // Icon
                    ImGui::TableSetColumnIndex(0);
                    UICommon::AlignTableCellIcon(rowH, static_cast<float>(g_Settings.iconSize));
                    Stat st = ItemTracker::GetItemStat(drop.itemId);
                    std::string iconUrl = !drop.iconUrl.empty() ? drop.iconUrl : st.details.iconUrl;
                    UICommon::DrawItemIconCell(drop.itemId, iconUrl, static_cast<float>(g_Settings.iconSize), drop.rarity);

                    ImGui::TableSetColumnIndex(1);
                    UICommon::AlignTableCellText(rowH);
                    ImGui::Text("%s", drop.itemName.c_str());
                    ImGui::TableSetColumnIndex(2);
                    UICommon::AlignTableCellText(rowH);
                    ImGui::Text("%d", drop.count);
                    ImGui::TableSetColumnIndex(3);
                    UICommon::AlignTableCellText(rowH);
                    ImGui::Text("%s", UICommon::FormatCoin(drop.totalValue).c_str());
                }
                ImGui::EndTable();
            }
        }
    }
}
}
