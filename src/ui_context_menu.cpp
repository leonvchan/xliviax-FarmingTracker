#include "ui_context_menu.h"
#include "item_tracker.h"
#include "ignored_items.h"
#include "localization.h"
#include "imgui/imgui.h"
#include <cstdio>

namespace UIContextMenu
{
    // Static variables to store context for each popup
    static int s_ContextItemId = 0;
    static std::string s_ContextItemName;

    void OpenContextMenu(const char* popupName, int itemId, const std::string& itemName)
    {
        ImGui::OpenPopup(popupName);
        s_ContextItemId = itemId;
        s_ContextItemName = itemName;
    }

    void RenderItemContextMenu(const char* popupName, ContextMenuType type)
    {
        if (ImGui::BeginPopup(popupName))
        {
            bool isFavorite = ItemTracker::IsFavorite(s_ContextItemId);
            bool isIgnored = IgnoredItemsManager::IsItemIgnored(s_ContextItemId);

            // Favorites options
            if (type == ContextMenuType::General || type == ContextMenuType::CustomProfit || type == ContextMenuType::Favorites || type == ContextMenuType::Ignored)
            {
                if (!isFavorite)
                {
                    if (ImGui::MenuItem(Localization::GetText("context_menu_add_favorites")))
                    {
                        ItemTracker::SetFavorite(s_ContextItemId, true);
                    }
                }
                else
                {
                    if (ImGui::MenuItem(Localization::GetText("context_menu_remove_favorites")))
                    {
                        ItemTracker::SetFavorite(s_ContextItemId, false);
                    }
                }
            }

            ImGui::Separator();

            // Ignore options
            if (type == ContextMenuType::General || type == ContextMenuType::CustomProfit || type == ContextMenuType::Favorites || type == ContextMenuType::Ignored)
            {
                if (!isIgnored)
                {
                    if (ImGui::MenuItem(Localization::GetText("context_menu_ignore")))
                    {
                        IgnoredItemsManager::IgnoreItem(s_ContextItemId);
                    }
                }
                else
                {
                    if (ImGui::MenuItem(Localization::GetText("context_menu_unignore")))
                    {
                        IgnoredItemsManager::UnignoreItem(s_ContextItemId);
                    }
                }
            }

            ImGui::Separator();

            // Copy options
            if (ImGui::MenuItem(Localization::GetText("context_menu_copy_name")))
            {
                ImGui::SetClipboardText(s_ContextItemName.c_str());
            }
            if (ImGui::MenuItem(Localization::GetText("context_menu_copy_id")))
            {
                char idStr[32];
                snprintf(idStr, sizeof(idStr), "%d", s_ContextItemId);
                ImGui::SetClipboardText(idStr);
            }

            ImGui::EndPopup();
        }
    }

    void RenderCurrencyContextMenu(const char* popupName, ContextMenuType type)
    {
        if (ImGui::BeginPopup(popupName))
        {
            bool isFavorite = ItemTracker::IsFavorite(s_ContextItemId);
            bool isIgnored = IgnoredItemsManager::IsCurrencyIgnored(s_ContextItemId);

            // Favorites options
            if (type == ContextMenuType::General || type == ContextMenuType::CustomProfit || type == ContextMenuType::Favorites || type == ContextMenuType::Ignored)
            {
                if (!isFavorite)
                {
                    if (ImGui::MenuItem(Localization::GetText("context_menu_add_favorites")))
                    {
                        ItemTracker::SetFavorite(s_ContextItemId, true);
                    }
                }
                else
                {
                    if (ImGui::MenuItem(Localization::GetText("context_menu_remove_favorites")))
                    {
                        ItemTracker::SetFavorite(s_ContextItemId, false);
                    }
                }
            }

            ImGui::Separator();

            // Ignore options
            if (type == ContextMenuType::General || type == ContextMenuType::CustomProfit || type == ContextMenuType::Favorites || type == ContextMenuType::Ignored)
            {
                if (!isIgnored)
                {
                    if (ImGui::MenuItem(Localization::GetText("context_menu_ignore")))
                    {
                        IgnoredItemsManager::IgnoreCurrency(s_ContextItemId);
                    }
                }
                else
                {
                    if (ImGui::MenuItem(Localization::GetText("context_menu_unignore")))
                    {
                        IgnoredItemsManager::UnignoreCurrency(s_ContextItemId);
                    }
                }
            }

            ImGui::Separator();

            // Copy options
            if (ImGui::MenuItem(Localization::GetText("context_menu_copy_name")))
            {
                ImGui::SetClipboardText(s_ContextItemName.c_str());
            }
            if (ImGui::MenuItem(Localization::GetText("context_menu_copy_id")))
            {
                char idStr[32];
                snprintf(idStr, sizeof(idStr), "%d", s_ContextItemId);
                ImGui::SetClipboardText(idStr);
            }

            ImGui::EndPopup();
        }
    }
}
