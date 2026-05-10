#include "ui_settings.h"
#include "ui_notifications.h"
#include "settings.h"
#include "item_tracker.h"
#include "drf_client.h"
#include "gw2_fetcher.h"
#include "auto_reset.h"
#include "session_history.h"
#include "backup_restore.h"
#include "localization.h"
#include "shared.h"
#include <imgui/imgui_internal.h>
#include <algorithm>
#include <cstring>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")

namespace UISettings
{
void RenderOptions()
{
    ImGui::TextUnformatted(Localization::GetText("farming_tracker_title"));
    ImGui::Separator();

    // === Quick Actions ===
    ImGui::Text("%s", Localization::GetText("quick_actions"));
    if (ImGui::Button(Localization::GetText("reset_all")))
    {
        ImGui::OpenPopup("Reset Confirm");
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("reset_all_tooltip"));

    if (ImGui::BeginPopup("Reset Confirm"))
    {
        ImGui::Text("%s", Localization::GetText("reset_confirm"));
        ImGui::Text("%s", Localization::GetText("reset_warning"));
        ImGui::Spacing();
        if (ImGui::Button(Localization::GetText("yes_reset")))
        {
            SettingsManager::ResetToDefaults();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(Localization::GetText("cancel")))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button(Localization::GetText("export")))
    {
        ImGui::OpenPopup("Export Settings");
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("export_tooltip"));

    if (ImGui::BeginPopup("Export Settings"))
    {
        static char exportPath[MAX_PATH] = "";
        if (exportPath[0] == '\0')
        {
            strcpy_s(exportPath, "farming_tracker_settings_export.json");
        }
        ImGui::Text("%s", Localization::GetText("export_settings"));
        ImGui::InputText("##ExportPath", exportPath, sizeof(exportPath));
        ImGui::Spacing();
        if (ImGui::Button(Localization::GetText("export")))
        {
            SettingsManager::ExportToFile(exportPath);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(Localization::GetText("cancel")))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button(Localization::GetText("import")))
    {
        ImGui::OpenPopup("Import Settings");
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("import_tooltip"));

    if (ImGui::BeginPopup("Import Settings"))
    {
        static char importPath[MAX_PATH] = "";
        ImGui::Text("%s", Localization::GetText("import_settings"));
        ImGui::InputText("##ImportPath", importPath, sizeof(importPath));
        ImGui::Spacing();
        if (ImGui::Button(Localization::GetText("import")))
        {
            SettingsManager::ImportFromFile(importPath);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(Localization::GetText("cancel")))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button(Localization::GetText("full_backup")))
    {
        ImGui::OpenPopup("FullBackupConfirm");
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("full_backup_tooltip"));

    if (ImGui::BeginPopup("FullBackupConfirm"))
    {
        static char backupPath[MAX_PATH] = "";
        if (backupPath[0] == '\0')
        {
            strcpy_s(backupPath, "farming_tracker_full_backup.json");
        }
        ImGui::Text("%s", Localization::GetText("full_backup"));
        ImGui::InputText("##BackupPath", backupPath, sizeof(backupPath));
        ImGui::Spacing();
        if (ImGui::Button(Localization::GetText("backup")))
        {
            const char* addonDir = APIDefs ? APIDefs->Paths_GetAddonDirectory("FarmingTracker") : "";
            std::string filename = std::string(addonDir ? addonDir : "") + "\\" + backupPath;
            BackupRestore::SaveBackupToFile(filename);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(Localization::GetText("cancel")))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button(Localization::GetText("full_restore")))
    {
        ImGui::OpenPopup("FullRestoreConfirm");
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("full_restore_tooltip"));

    if (ImGui::BeginPopup("FullRestoreConfirm"))
    {
        static char restorePath[MAX_PATH] = "";
        if (restorePath[0] == '\0')
        {
            strcpy_s(restorePath, "farming_tracker_full_backup.json");
        }
        ImGui::Text("%s", Localization::GetText("full_restore"));
        ImGui::InputText("##RestorePath", restorePath, sizeof(restorePath));
        ImGui::Spacing();
        if (ImGui::Button(Localization::GetText("restore")))
        {
            const char* addonDir = APIDefs ? APIDefs->Paths_GetAddonDirectory("FarmingTracker") : "";
            std::string filename = std::string(addonDir ? addonDir : "") + "\\" + restorePath;
            BackupRestore::LoadBackupFromFile(filename);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(Localization::GetText("cancel")))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button(Localization::GetText("save")))
    {
        SettingsManager::Save();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", Localization::GetText("save_tooltip"));

    ImGui::Spacing();
    ImGui::Separator();

    if (ImGui::Checkbox(Localization::GetText("show_main_window"), &g_Settings.showMainWindow))
        SettingsManager::Save();
    if (ImGui::Checkbox(Localization::GetText("show_mini_window"), &g_Settings.showMiniWindow))
        SettingsManager::Save();

    ImGui::Spacing();
    ImGui::Separator();

    // === General Settings ===
    if (ImGui::CollapsingHeader(Localization::GetText("general_settings"), ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Language Selection
        ImGui::TextWrapped(Localization::GetText("language_settings"));
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(Localization::GetText("language_tooltip"));

        const char* languageItems[] = {
            Localization::GetText("language_english"),
            Localization::GetText("language_german"),
            Localization::GetText("language_french"),
            Localization::GetText("language_spanish"),
            Localization::GetText("language_chinese"),
            Localization::GetText("language_czech"),
            Localization::GetText("language_italian"),
            Localization::GetText("language_polish"),
            Localization::GetText("language_portuguese"),
            Localization::GetText("language_russian")
        };

        int currentLangIndex = 0;
        if (g_Settings.language == "German") currentLangIndex = 1;
        else if (g_Settings.language == "French") currentLangIndex = 2;
        else if (g_Settings.language == "Spanish") currentLangIndex = 3;
        else if (g_Settings.language == "Chinese") currentLangIndex = 4;
        else if (g_Settings.language == "Czech") currentLangIndex = 5;
        else if (g_Settings.language == "Italian") currentLangIndex = 6;
        else if (g_Settings.language == "Polish") currentLangIndex = 7;
        else if (g_Settings.language == "Portuguese") currentLangIndex = 8;
        else if (g_Settings.language == "Russian") currentLangIndex = 9;

        if (ImGui::Combo("##Language", &currentLangIndex, languageItems, 10))
        {
            switch (currentLangIndex)
            {
                case 0: g_Settings.language = "English"; break;
                case 1: g_Settings.language = "German"; break;
                case 2: g_Settings.language = "French"; break;
                case 3: g_Settings.language = "Spanish"; break;
                case 4: g_Settings.language = "Chinese"; break;
                case 5: g_Settings.language = "Czech"; break;
                case 6: g_Settings.language = "Italian"; break;
                case 7: g_Settings.language = "Polish"; break;
                case 8: g_Settings.language = "Portuguese"; break;
                case 9: g_Settings.language = "Russian"; break;
            }
            Localization::SetLanguage(Localization::StringToLanguage(g_Settings.language));
            // Clear item details to force reload in new language
            ItemTracker::ClearItemDetails();
            SettingsManager::Save();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::Spacing();
    ImGui::Separator();

    // === Account Management ===
    if (ImGui::CollapsingHeader(Localization::GetText("account_management"), ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Account Selection Dropdown
        if (!g_Settings.accounts.empty())
        {
            std::vector<const char*> accountNames;
            for (const auto& acc : g_Settings.accounts)
                accountNames.push_back(acc.name.c_str());

            int currentAccountIndex = g_Settings.currentAccountIndex;
            if (currentAccountIndex < 0 || currentAccountIndex >= g_Settings.accounts.size())
                currentAccountIndex = 0;

            if (ImGui::Combo("##AccountSelect", &currentAccountIndex, accountNames.data(), static_cast<int>(accountNames.size())))
            {
                if (currentAccountIndex != g_Settings.currentAccountIndex)
                {
                    // Switch account logic
                    g_Settings.currentAccountIndex = currentAccountIndex;

                    // Load new token/keys from selected account
                    if (!g_Settings.accounts.empty() && g_Settings.currentAccountIndex >= 0 && g_Settings.currentAccountIndex < g_Settings.accounts.size())
                    {
                        g_Settings.drfToken = g_Settings.accounts[g_Settings.currentAccountIndex].drfToken;
                        g_Settings.gw2ApiKey = g_Settings.accounts[g_Settings.currentAccountIndex].gw2ApiKey;

                        // Update input buffers
                        strncpy_s(UICommon::s_AccountNameBuf, g_Settings.accounts[g_Settings.currentAccountIndex].name.c_str(), sizeof(UICommon::s_AccountNameBuf));
                        strncpy_s(UICommon::s_AccountDrfBuf, g_Settings.accounts[g_Settings.currentAccountIndex].drfToken.c_str(), sizeof(UICommon::s_AccountDrfBuf));
                        strncpy_s(UICommon::s_AccountGw2Buf, g_Settings.accounts[g_Settings.currentAccountIndex].gw2ApiKey.c_str(), sizeof(UICommon::s_AccountGw2Buf));

                        // Connect with new token (automatically cancels any current connection)
                        if (!g_Settings.drfToken.empty() && SettingsManager::IsTokenValid(g_Settings.drfToken))
                        {
                            DrfClient::Connect(g_Settings.drfToken);
                        }

                        // Update GW2 API key
                        Gw2Fetcher::UpdateApiKey();
                    }

                    // Reset farming session
                    ItemTracker::Reset();
                    const char* addonDir = APIDefs ? APIDefs->Paths_GetAddonDirectory("FarmingTracker") : nullptr;
                    ItemTracker::ClearPersistedData(addonDir);
                    SettingsManager::Save();
                }
            }
        }
        else
        {
            ImGui::TextDisabled("%s", Localization::GetText("no_accounts_configured"));
        }

        // Add/Remove Account Buttons
        if (ImGui::Button(Localization::GetText("add_account")))
        {
            // Add new account with default name
            Account newAccount;
            newAccount.name = std::string(Localization::GetText("account_prefix")) + " " + std::to_string(g_Settings.accounts.size() + 1);
            g_Settings.accounts.push_back(newAccount);
            g_Settings.currentAccountIndex = static_cast<int>(g_Settings.accounts.size() - 1);
            SettingsManager::Save();
        }
        ImGui::SameLine();
        if (ImGui::Button(Localization::GetText("remove_account")) && !g_Settings.accounts.empty())
        {
            if (g_Settings.accounts.size() > 1)
            {
                g_Settings.accounts.erase(g_Settings.accounts.begin() + g_Settings.currentAccountIndex);
                if (g_Settings.currentAccountIndex >= static_cast<int>(g_Settings.accounts.size()))
                    g_Settings.currentAccountIndex = static_cast<int>(g_Settings.accounts.size() - 1);
                SettingsManager::Save();
            }
        }

        // Edit Current Account
        if (!g_Settings.accounts.empty() && g_Settings.currentAccountIndex >= 0 && g_Settings.currentAccountIndex < g_Settings.accounts.size())
        {
            ImGui::Spacing();
            ImGui::Separator();
            char editAccountLabel[256];
            snprintf(editAccountLabel, sizeof(editAccountLabel), Localization::GetText("edit_account"), g_Settings.accounts[g_Settings.currentAccountIndex].name.c_str());
            ImGui::Text("%s", editAccountLabel);

            // Initialize buffers with current account data when opening this section
            static bool accountSectionOpen = false;
            bool sectionWasOpen = accountSectionOpen;
            accountSectionOpen = true;

            if (!sectionWasOpen)
            {
                strncpy_s(UICommon::s_AccountNameBuf, g_Settings.accounts[g_Settings.currentAccountIndex].name.c_str(), sizeof(UICommon::s_AccountNameBuf));
                strncpy_s(UICommon::s_AccountDrfBuf, g_Settings.accounts[g_Settings.currentAccountIndex].drfToken.c_str(), sizeof(UICommon::s_AccountDrfBuf));
                strncpy_s(UICommon::s_AccountGw2Buf, g_Settings.accounts[g_Settings.currentAccountIndex].gw2ApiKey.c_str(), sizeof(UICommon::s_AccountGw2Buf));
            }

            // Account Name
            ImGui::Text("%s", Localization::GetText("account_name"));
            if (ImGui::InputText("##AccountName", UICommon::s_AccountNameBuf, sizeof(UICommon::s_AccountNameBuf)))
            {
                g_Settings.accounts[g_Settings.currentAccountIndex].name = UICommon::s_AccountNameBuf;
            }

            // DRF Token
            ImGui::Text("%s", Localization::GetText("drf_token_label"));
            if (ImGui::InputText("##AccountDrfToken", UICommon::s_AccountDrfBuf, sizeof(UICommon::s_AccountDrfBuf)))
            {
                g_Settings.accounts[g_Settings.currentAccountIndex].drfToken = UICommon::s_AccountDrfBuf;
            }

            // GW2 API Key
            ImGui::Text("%s", Localization::GetText("gw2_api_key_label"));
            if (!SettingsManager::IsGw2ApiKeyPlausible(UICommon::s_AccountGw2Buf) && UICommon::s_AccountGw2Buf[0] != '\0')
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "(Invalid Format: 9 Blocks required)");
            }
            if (ImGui::InputText("##AccountGw2Key", UICommon::s_AccountGw2Buf, sizeof(UICommon::s_AccountGw2Buf)))
            {
                g_Settings.accounts[g_Settings.currentAccountIndex].gw2ApiKey = UICommon::s_AccountGw2Buf;
            }

            // Save Account Button
            if (ImGui::Button(Localization::GetText("save_account")))
            {
                // Update current account settings with input values
                g_Settings.accounts[g_Settings.currentAccountIndex].name = UICommon::s_AccountNameBuf;
                g_Settings.accounts[g_Settings.currentAccountIndex].drfToken = UICommon::s_AccountDrfBuf;
                g_Settings.accounts[g_Settings.currentAccountIndex].gw2ApiKey = UICommon::s_AccountGw2Buf;

                // Load new token/keys from current account
                g_Settings.drfToken = g_Settings.accounts[g_Settings.currentAccountIndex].drfToken;
                g_Settings.gw2ApiKey = g_Settings.accounts[g_Settings.currentAccountIndex].gw2ApiKey;

                // Connect with new token (automatically cancels any current connection)
                if (!g_Settings.drfToken.empty() && SettingsManager::IsTokenValid(g_Settings.drfToken))
                {
                    DrfClient::Connect(g_Settings.drfToken);
                }

                // Update GW2 API key
                Gw2Fetcher::UpdateApiKey();

                SettingsManager::Save();
            }

            ImGui::Spacing();
            ImGui::Separator();

            // Reload Configuration
            ImGui::Text("%s", Localization::GetText("reload_config"));
            if (ImGui::Button(Localization::GetText("reload_drf_token")))
            {
                DrfClient::Connect(g_Settings.drfToken);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Localization::GetText("reconnect_drf_token"));
            ImGui::SameLine();
            if (ImGui::Button(Localization::GetText("reload_gw2_api_key")))
            {
                Gw2Fetcher::UpdateApiKey();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Localization::GetText("reload_gw2_api_key_tooltip"));
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    // === Reset Settings ===
    if (ImGui::CollapsingHeader(Localization::GetText("reset_settings"), ImGuiTreeNodeFlags_DefaultOpen))
    {
        const char* resetModes[] = {
            Localization::GetText("auto_reset_never"),
            Localization::GetText("auto_reset_on_load"),
            Localization::GetText("auto_reset_daily"),
            Localization::GetText("auto_reset_weekly"),
            Localization::GetText("auto_reset_weekly_na_wvw"),
            Localization::GetText("auto_reset_weekly_eu_wvw"),
            Localization::GetText("auto_reset_weekly_map_bonus"),
            Localization::GetText("auto_reset_minutes_unload"),
            Localization::GetText("auto_reset_custom_days")
        };
        ImGui::Text("%s", Localization::GetText("auto_reset_label"));
        if (ImGui::Combo("##AutoReset", &g_Settings.automaticResetMode, resetModes, 9))
        {
            SettingsManager::Save();
            AutoReset::RefreshSchedule();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("auto_reset_tooltip"));

        if (g_Settings.automaticResetMode == 7)
        {
            char minrstLabel[256];
            snprintf(minrstLabel, sizeof(minrstLabel), "%s##minrst", Localization::GetText("minutes_after_unload_tooltip"));
            if (ImGui::InputInt(minrstLabel, &g_Settings.minutesUntilResetAfterShutdown))
            {
                g_Settings.minutesUntilResetAfterShutdown =
                    std::clamp(g_Settings.minutesUntilResetAfterShutdown, 1, 24 * 60);
                SettingsManager::Save();
                AutoReset::RefreshSchedule();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Localization::GetText("minutes_after_unload_tooltip"));
        }

        if (g_Settings.automaticResetMode == 8)
        {
            int sliderDays = g_Settings.customResetDays;
            char customdaysLabel[256];
            snprintf(customdaysLabel, sizeof(customdaysLabel), "%s##customdays", Localization::GetText("reset_interval_days"));
            if (ImGui::SliderInt(customdaysLabel, &sliderDays, 1, 30))
            {
                g_Settings.customResetDays = sliderDays;
                SettingsManager::Save();
                AutoReset::RefreshSchedule();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Localization::GetText("reset_interval_days_tooltip"));
        }

        char nextResetLabel[256];
            snprintf(nextResetLabel, sizeof(nextResetLabel), Localization::GetText("next_reset_utc"), AutoReset::GetNextResetDisplayUtc().c_str());
            ImGui::Text("%s", nextResetLabel);
    }

    ImGui::Spacing();
    ImGui::Separator();

    // === Session History ===
    if (ImGui::CollapsingHeader(Localization::GetText("session_history")))
    {
        if (ImGui::Checkbox(Localization::GetText("enable_session_history"), &g_Settings.enableSessionHistory))
        {
            SettingsManager::Save();
            SessionHistory::SetEnabled(g_Settings.enableSessionHistory);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("enable_session_history_tooltip"));

        if (ImGui::Checkbox(Localization::GetText("overwrite_session_history"), &g_Settings.overwriteSessionHistory))
        {
            SettingsManager::Save();
            SessionHistory::SetOverwrite(g_Settings.overwriteSessionHistory);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("overwrite_session_history_tooltip"));

        ImGui::Text("%s", Localization::GetText("max_session_history"));
        if (ImGui::SliderInt("##MaxSessionHistory", &g_Settings.maxSessionHistory, 1, 50, "%d"))
        {
            SettingsManager::Save();
            SessionHistory::SetMaxSessions(g_Settings.maxSessionHistory);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("max_session_history_tooltip"));
    }

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::Spacing();
    ImGui::Separator();

    // === Favorites Settings ===
    if (ImGui::CollapsingHeader(Localization::GetText("favorites_settings")))
    {
        ImGui::Spacing();
        ImGui::Separator();

        // Favorites UI
        ImGui::Text("%s", Localization::GetText("favorites_ui"));

        if (ImGui::Checkbox(Localization::GetText("enable_favorites_tab"), &g_Settings.enableFavoritesTab))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("enable_favorites_tab_tooltip"));

        ImGui::Spacing();
        ImGui::Separator();

        // Favorites Colors
        ImGui::Text("%s", Localization::GetText("favorites_colors"));

        if (ImGui::Checkbox(Localization::GetText("enable_favorite_text_color"), &g_Settings.enableFavoriteTextColor))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("enable_favorite_text_color_tooltip"));
        if (g_Settings.enableFavoriteTextColor)
        {
            ImGui::SameLine();
            if (ImGui::ColorEdit3("##FavoriteTextColor", g_Settings.favoriteTextColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel))
            {
                SettingsManager::Save();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Localization::GetText("text_color"));
        }

        if (ImGui::Checkbox(Localization::GetText("enable_favorite_row_color"), &g_Settings.enableFavoriteRowColor))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("enable_favorite_row_color_tooltip"));
        if (g_Settings.enableFavoriteRowColor)
        {
            ImGui::SameLine();
            if (ImGui::ColorEdit3("##FavoriteRowColor", g_Settings.favoriteRowColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel))
            {
                SettingsManager::Save();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Localization::GetText("row_color"));
        }

        ImGui::Spacing();

        // Best Drop Highlight
        if (ImGui::Checkbox(Localization::GetText("enable_best_drop_highlight"), &g_Settings.enableBestDropHighlight))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("enable_best_drop_highlight_tooltip"));
    }

    ImGui::Spacing();
    ImGui::Separator();

    // === Other Tabs ===
    if (ImGui::CollapsingHeader(Localization::GetText("tabs_settings")))
    {
        ImGui::Text("%s", Localization::GetText("tab_settings"));
        ImGui::TextDisabled("%s", Localization::GetText("tab_settings_description"));

        ImGui::Spacing();

        if (ImGui::Checkbox(Localization::GetText("lock_tab_order"), &g_Settings.lockTabOrder))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("lock_tab_order_tooltip"));

        ImGui::Spacing();
        ImGui::Separator();

        ImGui::Text("%s", Localization::GetText("tabs_description"));

        ImGui::Spacing();
        ImGui::Separator();

        if (ImGui::Checkbox(Localization::GetText("enable_profit_tab"), &g_Settings.enableProfitTab))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("enable_profit_tab_tooltip"));

        if (ImGui::Checkbox(Localization::GetText("enable_items_tab"), &g_Settings.enableItemsTab))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("enable_items_tab_tooltip"));

        if (ImGui::Checkbox(Localization::GetText("enable_currencies_tab"), &g_Settings.enableCurrenciesTab))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("enable_currencies_tab_tooltip"));

        if (ImGui::Checkbox(Localization::GetText("enable_favorites_tab"), &g_Settings.enableFavoritesTab))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("enable_favorites_tab_tooltip"));

        // Ignored Tab
        if (ImGui::Checkbox(Localization::GetText("enable_ignored_tab"), &g_Settings.enableIgnoredTab))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("enable_ignored_tab_tooltip"));

        if (ImGui::Checkbox(Localization::GetText("enable_session_history_tab"), &g_Settings.enableSessionHistoryTab))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("enable_session_history_tab_tooltip"));

        if (ImGui::Checkbox(Localization::GetText("enable_timeline_tab"), &g_Settings.enableTimelineTab))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("enable_timeline_tab_tooltip"));

        if (ImGui::Checkbox(Localization::GetText("enable_filter_tab"), &g_Settings.enableFilterTab))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("enable_filter_tab_tooltip"));

        if (ImGui::Checkbox(Localization::GetText("enable_custom_profit"), &g_Settings.enableCustomProfit))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("enable_custom_profit_tooltip"));
    }

    ImGui::Spacing();
    ImGui::Separator();

    // === Main Window Settings ===
    if (ImGui::CollapsingHeader(Localization::GetText("main_window_settings")))
    {
        if (ImGui::Checkbox(Localization::GetText("main_window_click_through"), &g_Settings.mainWindowClickThrough))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("main_window_click_through_tooltip"));

        ImGui::Text("%s", Localization::GetText("main_window_opacity"));
        float mainOpacityPercent = (1.0f - g_Settings.mainWindowOpacity) * 100.0f;
        if (ImGui::SliderFloat("##MainWindowOpacity", &mainOpacityPercent, 0.0f, 100.0f, "%.0f%%"))
        {
            g_Settings.mainWindowOpacity = 1.0f - (mainOpacityPercent / 100.0f);
            SettingsManager::Save();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("main_window_opacity_tooltip"));
    }

    ImGui::Spacing();
    ImGui::Separator();

    // === Mini Window Settings ===
    if (ImGui::CollapsingHeader(Localization::GetText("mini_window_settings")))
    {
        if (ImGui::Checkbox(Localization::GetText("mini_window_click_through"), &g_Settings.miniWindowClickThrough))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("mini_window_click_through_tooltip"));

        if (ImGui::Checkbox(Localization::GetText("mini_window_hide_title_bar"), &g_Settings.miniWindowHideTitleBar))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("mini_window_hide_title_bar_tooltip"));

        if (ImGui::Checkbox(Localization::GetText("mini_window_locked"), &g_Settings.miniWindowLocked))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("mini_window_locked_tooltip"));

        ImGui::Text("%s", Localization::GetText("mini_window_widget"));

        if (ImGui::Checkbox(Localization::GetText("mini_window_show_profit"), &g_Settings.miniWindowShowProfit))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("mini_window_show_profit_tooltip"));
        if (ImGui::Checkbox(Localization::GetText("mini_window_show_profit_per_hour"), &g_Settings.miniWindowShowProfitPerHour))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("mini_window_show_profit_per_hour_tooltip"));
        if (ImGui::Checkbox(Localization::GetText("mini_window_show_tp_sell"), &g_Settings.miniWindowShowTradingProfitSell))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("mini_window_show_tp_sell_tooltip"));
        if (ImGui::Checkbox(Localization::GetText("mini_window_show_tp_instant"), &g_Settings.miniWindowShowTradingProfitInstant))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("mini_window_show_tp_instant_tooltip"));
        if (ImGui::Checkbox(Localization::GetText("mini_window_show_total_items"), &g_Settings.miniWindowShowTotalItems))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("mini_window_show_total_items_tooltip"));
        if (ImGui::Checkbox(Localization::GetText("mini_window_show_session_duration"), &g_Settings.miniWindowShowSessionDuration))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("mini_window_show_session_duration_tooltip"));

        ImGui::Spacing();

        // Best Drop in Mini Window
        if (ImGui::Checkbox(Localization::GetText("enable_best_drop_in_mini_window"), &g_Settings.enableBestDropInMiniWindow))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("enable_best_drop_in_mini_window_tooltip"));

        ImGui::Spacing();

        // Mini Window Transparency
        ImGui::Text("%s", Localization::GetText("mini_window_opacity"));
        float miniOpacityPercent = (1.0f - g_Settings.miniWindowOpacity) * 100.0f;
        if (ImGui::SliderFloat("##MiniWindowOpacity", &miniOpacityPercent, 0.0f, 100.0f, "%.0f%%"))
        {
            g_Settings.miniWindowOpacity = 1.0f - (miniOpacityPercent / 100.0f);
            SettingsManager::Save();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("mini_window_opacity_tooltip"));
    }

    ImGui::Spacing();
    ImGui::Separator();

    // === Visual Settings ===
    if (ImGui::CollapsingHeader(Localization::GetText("visual_settings")))
    {
        // Visual Enhancement Settings
        ImGui::Text("%s", Localization::GetText("visual_enhancements"));

        // Custom Accent Color
        ImGui::Text("%s:", Localization::GetText("accent_color"));
        ImGui::SameLine();
        ImVec4 accentColor(g_Settings.accentColorR, g_Settings.accentColorG, g_Settings.accentColorB, 1.0f);
        if (ImGui::ColorEdit3("##AccentColor", (float*)&accentColor, ImGuiColorEditFlags_NoInputs))
        {
            g_Settings.accentColorR = accentColor.x;
            g_Settings.accentColorG = accentColor.y;
            g_Settings.accentColorB = accentColor.z;
            SettingsManager::Save();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("accent_color_tooltip"));

        ImGui::Spacing();

        if (ImGui::Checkbox(Localization::GetText("gradient_backgrounds"), &g_Settings.enableGradientBackgrounds))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("gradient_backgrounds_tooltip"));

        if (g_Settings.enableGradientBackgrounds)
        {
            ImGui::SameLine();
            ImGui::Text("%s:", Localization::GetText("top_gradient_color"));
            ImGui::SameLine();
            ImGui::ColorEdit3("##TopGradient", g_Settings.gradientTopColor, ImGuiColorEditFlags_NoInputs);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Localization::GetText("top_gradient_color_tooltip"));
            ImGui::SameLine();
            ImGui::Text("%s:", Localization::GetText("bottom_gradient_color"));
            ImGui::SameLine();
            ImGui::ColorEdit3("##BottomGradient", g_Settings.gradientBottomColor, ImGuiColorEditFlags_NoInputs);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Localization::GetText("bottom_gradient_color_tooltip"));
            if (ImGui::IsItemDeactivatedAfterEdit())
                SettingsManager::Save();
        }

        if (ImGui::Checkbox(Localization::GetText("show_profit_sparkline"), &g_Settings.showProfitSparkline))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("show_profit_sparkline_tooltip"));

        ImGui::Spacing();
        ImGui::Separator();

        // Basic Settings
        ImGui::Checkbox(Localization::GetText("show_item_icons"), &g_Settings.showItemIcons);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("show_item_icons_tooltip"));
        if (ImGui::SliderInt(Localization::GetText("icon_size"), &g_Settings.iconSize, 16, 96))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("icon_size_tooltip"));
        if (ImGui::Checkbox(Localization::GetText("show_rarity_borders"), &g_Settings.showRarityBorder))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("show_rarity_borders_tooltip"));
        if (g_Settings.showRarityBorder)
        {
            if (ImGui::SliderFloat(Localization::GetText("border_size"), &g_Settings.rarityBorderSize, 0.0f, 10.0f))
                SettingsManager::Save();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Localization::GetText("border_size_tooltip"));
        }

        ImGui::Spacing();
        ImGui::Separator();

        // Grid View Icon Sizes
        if (ImGui::SliderInt(Localization::GetText("grid_icon_size_items"), &g_Settings.gridIconSize, 16, 128))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("grid_icon_size_items_tooltip"));
        if (ImGui::SliderInt(Localization::GetText("grid_icon_size_currencies"), &g_Settings.gridIconSizeCurrencies, 16, 128))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("grid_icon_size_currencies_tooltip"));

        ImGui::Spacing();
        ImGui::Separator();

        // Timeline Tab Icon Sizes
        if (ImGui::SliderInt(Localization::GetText("timeline_icon_size_items"), &g_Settings.timelineIconSizeItems, 16, 96))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("timeline_icon_size_items_tooltip"));
        if (ImGui::SliderInt(Localization::GetText("timeline_icon_size_currencies"), &g_Settings.timelineIconSizeCurrencies, 16, 48))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("timeline_icon_size_currencies_tooltip"));
    }

    ImGui::Spacing();
    ImGui::Separator();

    // === Notification Settings ===
    if (ImGui::CollapsingHeader(Localization::GetText("notification_settings")))
    {
        if (ImGui::Checkbox(Localization::GetText("enable_notifications"), &g_Settings.enableNotifications))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("enable_notifications_tooltip"));

        if (g_Settings.enableNotifications)
        {
            ImGui::Indent();

            // --- General Settings ---
            if (ImGui::TreeNodeEx(Localization::GetText("notification_general"), ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::Checkbox(Localization::GetText("show_notification_setup"), &g_Settings.showNotificationSetup))
                    SettingsManager::Save();
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", Localization::GetText("show_notification_setup_tooltip"));

                if (ImGui::SliderFloat(Localization::GetText("notification_duration"), &g_Settings.notificationDuration, 1.0f, 20.0f, "%.1f s"))
                    SettingsManager::Save();
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", Localization::GetText("notification_duration_tooltip"));

                if (ImGui::Checkbox(Localization::GetText("notification_stacking"), &g_Settings.notificationStacking))
                    SettingsManager::Save();
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", Localization::GetText("notification_stacking_tooltip"));

                ImGui::Spacing();
                ImGui::Separator();

                // --- Sound Settings ---
                if (ImGui::Checkbox(Localization::GetText("notification_play_sound"), &g_Settings.notificationPlaySound))
                    SettingsManager::Save();
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", Localization::GetText("notification_play_sound_tooltip"));

                if (g_Settings.notificationPlaySound)
                {
                    int volPercent = static_cast<int>(g_Settings.notificationVolume * 100.0f);
                    if (ImGui::SliderInt(Localization::GetText("notification_volume"), &volPercent, 1, 100, "%d%%"))
                    {
                        g_Settings.notificationVolume = static_cast<float>(volPercent) / 100.0f;
                        UINotifications::SetVolume(g_Settings.notificationVolume);
                        SettingsManager::Save();
                    }

                    auto drawSoundRow = [](const char* labelKey, std::string& path, float& volume, bool isPrecursor, bool isInfusion, bool isAlert) {
                        ImGui::PushID(labelKey);
                        ImGui::Text("%s:", Localization::GetText(labelKey));
                        
                        char buf[512];
                        strncpy_s(buf, sizeof(buf), path.c_str(), _TRUNCATE);
                        ImGui::SetNextItemWidth(250.0f);
                        if (ImGui::InputTextWithHint("##path", Localization::GetText("sound_path_hint"), buf, sizeof(buf))) {
                            path = buf;
                            SettingsManager::Save();
                        }
                        
                        ImGui::SameLine();
                        if (ImGui::Button("...")) {
                            OPENFILENAMEA ofn;
                            char szFile[512] = { 0 };
                            ZeroMemory(&ofn, sizeof(ofn));
                            ofn.lStructSize = sizeof(ofn);
                            ofn.hwndOwner = NULL;
                            ofn.lpstrFile = szFile;
                            ofn.nMaxFile = sizeof(szFile);
                            ofn.lpstrFilter = "Audio Files\0*.wav;*.mp3;*.flac\0All Files\0*.*\0";
                            ofn.nFilterIndex = 1;
                            ofn.lpstrFileTitle = NULL;
                            ofn.nMaxFileTitle = 0;
                            ofn.lpstrInitialDir = NULL;
                            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

                            if (GetOpenFileNameA(&ofn)) {
                                path = szFile;
                                SettingsManager::Save();
                            }
                        }
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Localization::GetText("browse_for_file"));

                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(80.0f);
                        int volPercent = static_cast<int>(volume * 100.0f);
                        if (ImGui::SliderInt("##vol", &volPercent, 0, 100, "%d%%")) {
                            volume = static_cast<float>(volPercent) / 100.0f;
                            SettingsManager::Save();
                        }
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Individuelle Lautstärke");

                        ImGui::SameLine();
                        if (ImGui::Button(Localization::GetText("sound_test"))) {
                            UINotifications::PlayNotificationSound(isPrecursor, isInfusion, isAlert);
                        }
                        ImGui::PopID();
                    };

                    drawSoundRow("sound_standard", g_Settings.soundPathStandard, g_Settings.notificationVolumeStandard, false, false, false);
                    drawSoundRow("sound_precursor", g_Settings.soundPathPrecursor, g_Settings.notificationVolumePrecursor, true, false, false);
                    drawSoundRow("sound_infusion", g_Settings.soundPathInfusion, g_Settings.notificationVolumeInfusion, false, true, false);
                    drawSoundRow("sound_alert", g_Settings.soundPathAlert, g_Settings.notificationVolumeAlert, false, false, true);
                }

                ImGui::TreePop();
            }

            ImGui::Spacing();
            ImGui::Separator();

            // --- Item Alerts ---
            if (ImGui::TreeNode(Localization::GetText("notification_item_alerts")))
            {
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", Localization::GetText("trigger_drops"));
                
                // Value Filter
                ImGui::Checkbox("##EnableValue", &g_Settings.notificationEnableMinValue);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Aktiviere/Deaktiviere den Goldwert-Filter");
                ImGui::SameLine();
                bool disableValue = !g_Settings.notificationEnableMinValue;
                if (disableValue) {
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                }
                if (ImGui::InputFloat(Localization::GetText("notification_min_value"), &g_Settings.notificationMinValueGold, 0.1f, 1.0f, "%.2f g"))
                {
                    if (g_Settings.notificationMinValueGold < 0) g_Settings.notificationMinValueGold = 0;
                    SettingsManager::Save();
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", Localization::GetText("notification_min_value_tooltip"));
                if (disableValue) {
                    ImGui::PopStyleVar();
                    ImGui::PopItemFlag();
                }

                // Rarity Filter
                ImGui::Checkbox("##EnableRarity", &g_Settings.notificationEnableMinRarity);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Aktiviere/Deaktiviere den Seltenheits-Filter");
                ImGui::SameLine();
                bool disableRarity = !g_Settings.notificationEnableMinRarity;
                if (disableRarity) {
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                }
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
                if (ImGui::Combo(Localization::GetText("notification_min_rarity"), &g_Settings.notificationMinRarity, rarityLabels, 8))
                    SettingsManager::Save();
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", Localization::GetText("notification_min_rarity_tooltip"));
                if (disableRarity) {
                    ImGui::PopStyleVar();
                    ImGui::PopItemFlag();
                }

                // Combine Logic
                bool disableCombine = !g_Settings.notificationEnableMinValue || !g_Settings.notificationEnableMinRarity;
                if (disableCombine) {
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                }
                if (ImGui::Checkbox(Localization::GetText("notification_combine_logic"), &g_Settings.notificationCombineValueAndRarity))
                    SettingsManager::Save();
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", Localization::GetText("notification_combine_logic_tooltip"));
                if (disableCombine) {
                    ImGui::PopStyleVar();
                    ImGui::PopItemFlag();
                }

                if (ImGui::Checkbox(Localization::GetText("notification_include_non_profit"), &g_Settings.notificationIncludeNonProfit))
                    SettingsManager::Save();
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", Localization::GetText("notification_include_non_profit_tooltip"));

                ImGui::Spacing();
                ImGui::Separator();

                // Special Alerts
                if (ImGui::Checkbox(Localization::GetText("notification_precursor_alert"), &g_Settings.notificationPrecursorAlert))
                    SettingsManager::Save();
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", Localization::GetText("notification_precursor_alert_tooltip"));

                if (ImGui::Checkbox(Localization::GetText("notification_infusion_alert"), &g_Settings.notificationInfusionAlert))
                    SettingsManager::Save();
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", Localization::GetText("notification_infusion_alert_tooltip"));

                if (g_Settings.notificationInfusionAlert)
                {
                    ImGui::Indent();
                    if (ImGui::Checkbox(Localization::GetText("notification_include_agony"), &g_Settings.notificationIncludeAgonyInfusions))
                        SettingsManager::Save();
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", Localization::GetText("notification_include_agony_tooltip"));
                    ImGui::Unindent();
                }

                ImGui::TreePop();
            }

            // --- Session & Time Alerts ---
            if (ImGui::TreeNode(Localization::GetText("notification_session_alerts")))
            {
                // Profit Goal
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "%s", Localization::GetText("trigger_profit_goal"));
                if (ImGui::Checkbox(Localization::GetText("notify_profit_goal"), &g_Settings.notifyProfitGoal))
                    SettingsManager::Save();
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", Localization::GetText("notify_profit_goal_tooltip"));

                if (g_Settings.notifyProfitGoal)
                {
                    float goldAmount = static_cast<float>(g_Settings.profitGoalAmount) / 10000.0f;
                    if (ImGui::InputFloat(Localization::GetText("profit_goal_amount"), &goldAmount, 1.0f, 10.0f, "%.2f g"))
                    {
                        g_Settings.profitGoalAmount = static_cast<int>(goldAmount * 10000);
                        SettingsManager::Save();
                    }
                }

                ImGui::Spacing();
                ImGui::Separator();

                // Time / Reset
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "%s", Localization::GetText("trigger_time_reset"));
                if (ImGui::Checkbox(Localization::GetText("notify_reset_warning"), &g_Settings.notifyResetWarning))
                    SettingsManager::Save();

                if (g_Settings.notifyResetWarning)
                {
                    if (ImGui::SliderInt(Localization::GetText("reset_warning_minutes"), &g_Settings.resetWarningMinutes, 1, 60))
                        SettingsManager::Save();
                }

                if (ImGui::Checkbox(Localization::GetText("notify_session_complete"), &g_Settings.notifySessionComplete))
                    SettingsManager::Save();

                if (g_Settings.notifySessionComplete)
                {
                    if (ImGui::SliderInt(Localization::GetText("session_complete_hours"), &g_Settings.sessionCompleteHours, 1, 24))
                        SettingsManager::Save();
                }
                ImGui::TreePop();
            }

            ImGui::Unindent();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    // === Performance Settings ===
    if (ImGui::CollapsingHeader(Localization::GetText("performance_settings")))
    {
        // Low Perf Mode
        if (ImGui::Checkbox(Localization::GetText("disable_complex_visuals"), &g_Settings.disableComplexVisualsOnLowPerf))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("disable_complex_visuals_tooltip"));

        ImGui::Spacing();

        // Icon Cache
        if (ImGui::Checkbox(Localization::GetText("enable_icon_cache"), &g_Settings.enableIconCache))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("enable_icon_cache_tooltip"));

        if (g_Settings.enableIconCache)
        {
            ImGui::Indent();
            if (ImGui::InputInt(Localization::GetText("icon_cache_max_icons"), &g_Settings.iconCacheMaxIcons, 10, 50))
            {
                if (g_Settings.iconCacheMaxIcons < 10) g_Settings.iconCacheMaxIcons = 10;
                if (g_Settings.iconCacheMaxIcons > 2000) g_Settings.iconCacheMaxIcons = 2000;
                SettingsManager::Save();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Localization::GetText("icon_cache_max_icons_tooltip"));
            ImGui::Unindent();
        }

        ImGui::Spacing();
        ImGui::Separator();

        // History Limit
        if (ImGui::SliderInt(Localization::GetText("max_history_items_limit"), &g_Settings.maxHistoryItems, 50, 2000, "%d Items"))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("max_history_items_limit_tooltip"));

        // API Update Interval
        if (ImGui::SliderInt(Localization::GetText("api_update_interval"), &g_Settings.priceUpdateIntervalMin, 5, 15, "%d Min"))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("api_update_interval_tooltip"));
    }

    ImGui::Spacing();
    ImGui::Separator();

    // === Settings Profiles ===
    if (ImGui::CollapsingHeader(Localization::GetText("settings_profiles")))
    {
        ImGui::Text("%s", Localization::GetText("profiles_description"));

        ImGui::Spacing();

        // Profile Selection
        if (!g_Settings.settingsProfiles.empty())
        {
            std::vector<const char*> profileNames;
            profileNames.push_back(Localization::GetText("default_no_profile"));
            for (const auto& profile : g_Settings.settingsProfiles)
                profileNames.push_back(profile.name.c_str());

            int currentProfileDisplay = g_Settings.currentProfileIndex + 1;
            if (ImGui::Combo("##ProfileSelect", &currentProfileDisplay, profileNames.data(), static_cast<int>(profileNames.size())))
            {
                if (currentProfileDisplay == 0)
                {
                    g_Settings.currentProfileIndex = -1;
                }
                else
                {
                    SettingsManager::ApplyProfile(currentProfileDisplay - 1);
                }
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Localization::GetText("select_profile_tooltip"));
        }
        else
        {
            ImGui::TextDisabled("%s", Localization::GetText("no_profiles_created"));
        }

        ImGui::Spacing();

        // Profile Management Buttons
        ImGui::Text("%s", Localization::GetText("create_new_profile"));
        ImGui::InputText("##NewProfileName", UICommon::s_NewProfileNameBuf, sizeof(UICommon::s_NewProfileNameBuf));
        ImGui::SameLine();
        if (ImGui::Button(Localization::GetText("create")))
        {
            if (UICommon::s_NewProfileNameBuf[0] != '\0')
            {
                SettingsManager::CreateProfile(UICommon::s_NewProfileNameBuf);
                UICommon::s_NewProfileNameBuf[0] = '\0';
            }
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("create_tooltip"));

        if (g_Settings.currentProfileIndex >= 0 && g_Settings.currentProfileIndex < static_cast<int>(g_Settings.settingsProfiles.size()))
        {
            ImGui::Spacing();
            char currentProfileLabel[256];
            snprintf(currentProfileLabel, sizeof(currentProfileLabel), Localization::GetText("current_profile"), g_Settings.settingsProfiles[g_Settings.currentProfileIndex].name.c_str());
            ImGui::Text("%s", currentProfileLabel);
            if (ImGui::Button(Localization::GetText("update_profile")))
            {
                SettingsManager::UpdateProfile(g_Settings.currentProfileIndex);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Localization::GetText("update_profile_tooltip"));
            ImGui::SameLine();
            if (ImGui::Button(Localization::GetText("delete_profile")))
            {
                SettingsManager::DeleteProfile(g_Settings.currentProfileIndex);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Localization::GetText("delete_profile_tooltip"));
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    // === Automatic Backups ===
    if (ImGui::CollapsingHeader(Localization::GetText("automatic_backups")))
    {
        ImGui::Text("%s", Localization::GetText("auto_backup"));

        ImGui::Spacing();
        ImGui::Separator();

        if (ImGui::Checkbox(Localization::GetText("enable_automatic_backups"), &g_Settings.enableAutoBackups))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("enable_automatic_backups_tooltip"));

        if (g_Settings.enableAutoBackups)
        {
            ImGui::Spacing();

            const char* backupFreqItems[] = {Localization::GetText("backup_manual_only"), Localization::GetText("backup_daily"), Localization::GetText("backup_weekly")};
            if (ImGui::Combo("Backup frequency##BackupFreq", &g_Settings.backupFrequency, backupFreqItems, 3))
                SettingsManager::Save();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Localization::GetText("backup_frequency_tooltip"));

            ImGui::Spacing();

            char backupCountLabel[256];
            snprintf(backupCountLabel, sizeof(backupCountLabel), "%s##BackupCount", Localization::GetText("max_backup_count"));
            if (ImGui::SliderInt(backupCountLabel, &g_Settings.maxBackupCount, 1, 20))
                SettingsManager::Save();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", Localization::GetText("max_backup_count_tooltip"));
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    // === Debug Settings ===
    if (ImGui::CollapsingHeader(Localization::GetText("debug_settings")))
    {
        ImGui::Text("%s", Localization::GetText("debug_settings"));

        ImGui::Spacing();
        ImGui::Separator();

        if (ImGui::Checkbox(Localization::GetText("enable_debug_tab"), &g_Settings.enableDebugTab))
            SettingsManager::Save();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", Localization::GetText("enable_debug_tab_tooltip"));
    }
}
}
