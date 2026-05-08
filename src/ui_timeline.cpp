#include "ui_timeline.h"
#include "item_tracker.h"
#include "session_history.h"
#include "settings.h"
#include "localization.h"
#include "ui_common.h"
#include <algorithm>
#include <ctime>
#include <map>

namespace UITimeline
{
    void RenderTimelineTab()
    {
        // 1. Header Info (Profit, G/h, etc. like in the photo)
        long long tpSell = ItemTracker::CalcTotalTpSellProfit();
        long long tpInstant = ItemTracker::CalcTotalTpInstantProfit();
        auto duration = ItemTracker::GetSessionDuration();
        long long seconds = duration.count();

        ImGui::Columns(2, "TimelineHeader", false);
        ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.4f);

        // Left side: Profits
        long long custom = ItemTracker::CalcTotalCustomProfit();
        ImGui::Text("%s", Localization::GetText("approx_profits_label"));
        ImGui::SameLine();
        ImVec4 customColor = custom > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (custom < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
        ImGui::TextColored(customColor, "%s", UICommon::FormatCoin(custom).c_str());
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("approx_profits_tooltip"));

        ImGui::Text("%s", Localization::GetText("approx_gold_per_hour_label"));
        ImGui::SameLine();
        long long profitPerHour = ItemTracker::GetTotalProfitPerHour(duration);
        ImVec4 profitPerHourColor = profitPerHour > 0 ? ImVec4(1.f, 0.84f, 0.f, 1.f) : (profitPerHour < 0 ? ImVec4(0.9f, 0.2f, 0.2f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f));
        ImGui::TextColored(profitPerHourColor, "%s", UICommon::FormatCoin(profitPerHour).c_str());
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("approx_gold_per_hour_tooltip"));

        ImGui::Text("%s", Localization::GetText("approx_trading_profits_listings_label"));
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "%s", UICommon::FormatCoin(tpSell).c_str());

        ImGui::Text("%s", Localization::GetText("approx_trading_profits_instant_label"));
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "%s", UICommon::FormatCoin(tpInstant).c_str());

        ImGui::NextColumn();

        // Right side: Gold per hour
        if (seconds > 0)
        {
            long long gphSell = (tpSell * 3600) / seconds;
            long long gphInstant = (tpInstant * 3600) / seconds;

            ImGui::Text("%s", Localization::GetText("timeline_profit_hour_listings"));
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "%s", UICommon::FormatCoin(gphSell).c_str());

            ImGui::Text("%s", Localization::GetText("timeline_profit_hour_instant"));
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "%s", UICommon::FormatCoin(gphInstant).c_str());
        }

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::Spacing();

        // 2. Timeline List (Grouped by Timestamp)
        auto drops = ItemTracker::GetSessionDropsCopy();
        
        // Group drops by timestamp
        std::map<std::string, std::vector<SessionHistory::DropEntry>> groupedDrops;
        std::vector<std::string> timestamps; // Keep order
        
        for (const auto& drop : drops)
        {
            if (groupedDrops.find(drop.timestamp) == groupedDrops.end())
            {
                timestamps.push_back(drop.timestamp);
            }
            groupedDrops[drop.timestamp].push_back(drop);
        }
        
        std::reverse(timestamps.begin(), timestamps.end()); // Newest first

        if (ImGui::BeginChild("TimelineDropsScroll", ImVec2(0, 0), true))
        {
            if (timestamps.empty())
            {
                ImGui::TextDisabled("%s", Localization::GetText("timeline_no_drops"));
            }
            else
            {
                for (const auto& ts : timestamps)
                {
                    const auto& group = groupedDrops[ts];
                    
                    ImGui::PushID(ts.c_str());
                    
                    // Calculate total value and check for MF
                    long long groupValue = 0;
                    int groupMF = -1;
                    for (const auto& d : group)
                    {
                        groupValue += d.totalValue;
                        if (d.magicFind >= 0) groupMF = d.magicFind;
                    }

                    // 1. Time Header
                    ImGui::TextColored(ImVec4(0.4f, 0.6f, 1.0f, 1.0f), "X Livia X"); // Placeholder Name
                    ImGui::SameLine();
                    ImGui::TextDisabled("%s", ts.c_str());

                    // 2. Item Drops Label + MF
                    ImGui::Text("%s", Localization::GetText("timeline_item_drops"));
                    if (groupMF >= 0)
                    {
                        ImGui::SameLine();
                        ImGui::TextDisabled("MF: %d%%", groupMF);
                    }

                    // 3. Gold Value and Currencies in one row below "Item Drops"
                    ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "%s", UICommon::FormatCoin(groupValue).c_str());
                    
                    // Check if there are currencies to display
                    bool hasCurrencies = false;
                    for (const auto& d : group) if (d.isCurrency) { hasCurrencies = true; break; }
                    
                    if (hasCurrencies)
                    {
                        ImGui::SameLine();
                        ImGui::TextDisabled("|");
                        ImGui::SameLine();
                        ImGui::TextDisabled("%s:", Localization::GetText("timeline_currencies"));
                        ImGui::SameLine();
                        for (const auto& d : group)
                        {
                            if (!d.isCurrency) continue;
                            ImGui::Text("%d", d.count);
                            ImGui::SameLine();
                            
                            // Get currency details for icon
                            auto st = ItemTracker::GetCurrencyStat(d.itemId);
                            UICommon::DrawItemIconCell(d.itemId, st.details.iconUrl, 16.0f, st.details.rarity);
                            
                            if (ImGui::IsItemHovered())
                                ImGui::SetTooltip("%s (x%d)", d.itemName.c_str(), d.count);
                            ImGui::SameLine();
                        }
                    }

                    // 4. Icons Row
                    ImGui::Spacing();
                    float iconSize = 40.0f;
                    for (const auto& d : group)
                    {
                        if (d.isCurrency) continue;

                        ImGui::BeginGroup();
                        UICommon::DrawItemIconCell(d.itemId, d.iconUrl, iconSize, d.rarity);
                        
                        // Draw count with shadow
                        char countStr[32];
                        snprintf(countStr, sizeof(countStr), "%d", d.count);
                        
                        ImVec2 pos = ImGui::GetItemRectMin();
                        float fontSize = iconSize * 0.4f;
                        ImGui::SetWindowFontScale(fontSize / ImGui::GetFontSize());
                        
                        // Count position similar to grid view
                        ImVec2 textSize = ImGui::CalcTextSize(countStr);
                        ImVec2 countPos = ImVec2(pos.x + iconSize - textSize.x - iconSize * 0.1f, 
                                                 pos.y + iconSize - textSize.y - iconSize * 0.1f);

                        ImVec2 shadowOffsets[] = { {-1, -1}, {1, -1}, {-1, 1}, {1, 1} };
                        for (int s = 0; s < 4; s++) {
                            ImGui::GetWindowDrawList()->AddText(ImVec2(countPos.x + shadowOffsets[s].x, countPos.y + shadowOffsets[s].y), 
                                IM_COL32(0, 0, 0, 255), countStr);
                        }
                        ImGui::GetWindowDrawList()->AddText(countPos, IM_COL32(255, 255, 255, 255), countStr);
                        
                        ImGui::SetWindowFontScale(1.0f);
                        
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("%s (x%d)\nValue: %s", d.itemName.c_str(), d.count, UICommon::FormatCoin(d.totalValue).c_str());
                        
                        ImGui::EndGroup();
                        ImGui::SameLine();
                    }
                    ImGui::NewLine();

                    ImGui::PopID();
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                }
            }
        }
        ImGui::EndChild();
    }
}
