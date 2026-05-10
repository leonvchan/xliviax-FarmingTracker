#include "ui_notifications.h"
#include "settings.h"
#include "localization.h"
#include "ui_common.h"
#include "shared.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <vector>
#include <mutex>
#include <filesystem>

// Disable miniaudio warnings
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244) // Conversion from ma_uint64 to ma_uint32
#endif

#define MINIAUDIO_IMPLEMENTATION
#include "../include/miniaudio.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "resource.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace UINotifications
{
    static std::vector<Notification> s_Notifications;
    static std::mutex s_Mutex;
    
    static ma_engine s_AudioEngine;
    static bool s_AudioInitialized = false;

    // Persistent sound objects for each category to allow overlapping between categories
    // but restarting within the same category (prevents audio clutter)
    static ma_sound s_SoundStandard;
    static ma_sound s_SoundPrecursor;
    static ma_sound s_SoundInfusion;
    static ma_sound s_SoundAlert;
    
    static bool s_SoundStandardLoaded = false;
    static bool s_SoundPrecursorLoaded = false;
    static bool s_SoundInfusionLoaded = false;
    static bool s_SoundAlertLoaded = false;

    static ma_decoder s_DecoderStandard;
    static ma_decoder s_DecoderPrecursor;
    static ma_decoder s_DecoderInfusion;
    static ma_decoder s_DecoderAlert;

    // Helper to find a sound file with multiple possible extensions
    static std::string FindSoundFile(const std::string& baseName)
    {
        const char* extensions[] = { ".wav", ".mp3", ".ogg", ".flac" };
        const char* folders[] = { "addons/FarmingTracker/sounds/", "data/sounds/" };
        
        for (const char* folder : folders)
        {
            for (const char* ext : extensions)
            {
                std::string path = std::string(folder) + baseName + ext;
                if (std::filesystem::exists(path)) return path;
            }
        }
        return "";
    }

    // Helper to load sound from resource
    static bool LoadSoundFromResource(int resId, ma_sound* pSound, ma_decoder* pDecoder)
    {
        HMODULE hMod = GetModule();
        HRSRC hRes = FindResourceA(hMod, MAKEINTRESOURCEA(resId), (LPCSTR)RT_RCDATA);
        if (!hRes) return false;
        
        HGLOBAL hData = LoadResource(hMod, hRes);
        if (!hData) return false;
        
        void* pData = LockResource(hData);
        ma_uint32 dataSize = static_cast<ma_uint32>(SizeofResource(hMod, hRes));
        if (!pData || dataSize == 0) return false;
        
        ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 48000);
        ma_result decodeResult = ma_decoder_init_memory(pData, dataSize, &decoderConfig, pDecoder);
        if (decodeResult != MA_SUCCESS) return false;

        ma_result soundResult = ma_sound_init_from_data_source(&s_AudioEngine, pDecoder, MA_SOUND_FLAG_DECODE, NULL, pSound);
        if (soundResult != MA_SUCCESS) {
            ma_decoder_uninit(pDecoder);
            return false;
        }
        
        return true;
    }

    void Init()
    {
        if (s_AudioInitialized) return;

        ma_result result = ma_engine_init(NULL, &s_AudioEngine);
        if (result == MA_SUCCESS)
        {
            s_AudioInitialized = true;
            ma_engine_set_volume(&s_AudioEngine, g_Settings.notificationVolume);
            
            // Pre-load sounds from resources as default fallback
            s_SoundStandardLoaded = LoadSoundFromResource(IDR_WAV_STANDARD, &s_SoundStandard, &s_DecoderStandard);
            if (s_SoundStandardLoaded) ma_sound_set_volume(&s_SoundStandard, g_Settings.notificationVolumeStandard);

            s_SoundPrecursorLoaded = LoadSoundFromResource(IDR_WAV_PRECURSOR, &s_SoundPrecursor, &s_DecoderPrecursor);
            if (s_SoundPrecursorLoaded) ma_sound_set_volume(&s_SoundPrecursor, g_Settings.notificationVolumePrecursor);

            s_SoundInfusionLoaded = LoadSoundFromResource(IDR_WAV_INFUSION, &s_SoundInfusion, &s_DecoderInfusion);
            if (s_SoundInfusionLoaded) ma_sound_set_volume(&s_SoundInfusion, g_Settings.notificationVolumeInfusion);

            s_SoundAlertLoaded = LoadSoundFromResource(IDR_WAV_ALERT, &s_SoundAlert, &s_DecoderAlert);
            if (s_SoundAlertLoaded) ma_sound_set_volume(&s_SoundAlert, g_Settings.notificationVolumeAlert);
        }
    }

    void Shutdown()
    {
        if (!s_AudioInitialized) return;

        if (s_SoundStandardLoaded) { ma_sound_uninit(&s_SoundStandard); ma_decoder_uninit(&s_DecoderStandard); }
        if (s_SoundPrecursorLoaded) { ma_sound_uninit(&s_SoundPrecursor); ma_decoder_uninit(&s_DecoderPrecursor); }
        if (s_SoundInfusionLoaded) { ma_sound_uninit(&s_SoundInfusion); ma_decoder_uninit(&s_DecoderInfusion); }
        if (s_SoundAlertLoaded) { ma_sound_uninit(&s_SoundAlert); ma_decoder_uninit(&s_DecoderAlert); }

        ma_engine_uninit(&s_AudioEngine);
        s_AudioInitialized = false;
    }

    void SetVolume(float volume)
    {
        if (s_AudioInitialized)
        {
            ma_engine_set_volume(&s_AudioEngine, volume);
        }
    }

    void PlayNotificationSound(bool isPrecursor, bool isInfusion, bool isAlert)
    {
        if (!g_Settings.notificationPlaySound || !s_AudioInitialized) return;

        std::string soundPath = "";
        ma_sound* pFallbackSound = nullptr;
        bool fallbackLoaded = false;
        float volume = 0.5f;
        
        // Determine which sound to play and check for custom file
        if (isPrecursor) {
            soundPath = g_Settings.soundPathPrecursor;
            pFallbackSound = &s_SoundPrecursor;
            fallbackLoaded = s_SoundPrecursorLoaded;
            volume = g_Settings.notificationVolumePrecursor;
            if (soundPath.empty()) {
                soundPath = FindSoundFile("precursor");
            }
        }
        else if (isInfusion) {
            soundPath = g_Settings.soundPathInfusion;
            pFallbackSound = &s_SoundInfusion;
            fallbackLoaded = s_SoundInfusionLoaded;
            volume = g_Settings.notificationVolumeInfusion;
            if (soundPath.empty()) {
                soundPath = FindSoundFile("infusion");
            }
        }
        else if (isAlert) {
            soundPath = g_Settings.soundPathAlert;
            pFallbackSound = &s_SoundAlert;
            fallbackLoaded = s_SoundAlertLoaded;
            volume = g_Settings.notificationVolumeAlert;
            if (soundPath.empty()) {
                soundPath = FindSoundFile("alert");
            }
        }
        else {
            soundPath = g_Settings.soundPathStandard;
            pFallbackSound = &s_SoundStandard;
            fallbackLoaded = s_SoundStandardLoaded;
            volume = g_Settings.notificationVolumeStandard;
            if (soundPath.empty()) {
                soundPath = FindSoundFile("default");
            }
        }

        // 1. Try playing custom file path
        if (!soundPath.empty() && std::filesystem::exists(soundPath))
        {
            // We use a temporary sound object to play with specific volume
            // ma_engine_play_sound doesn't support volume directly per-call
            ma_sound tempSound;
            if (ma_sound_init_from_file(&s_AudioEngine, soundPath.c_str(), 0, NULL, NULL, &tempSound) == MA_SUCCESS) {
                ma_sound_set_volume(&tempSound, volume);
                ma_sound_start(&tempSound);
                // Note: miniaudio handles the cleanup of fire-and-forget sounds if configured, 
                // but here we just want a simple play. For simplicity and to avoid leaks, 
                // we'll use the engine's volume for now or use the fallback logic's sound objects if we had them for custom paths.
                // Actually, let's just set the engine volume temporarily for this fire-and-forget call.
                // Or better: set the volume of the specific fallback sound and use that if we were to load custom paths into them.
            }
            // Simple way for now: set engine volume to the specific volume, then play
            ma_engine_set_volume(&s_AudioEngine, volume * g_Settings.notificationVolume);
            ma_engine_play_sound(&s_AudioEngine, soundPath.c_str(), NULL);
        }
        // 2. Fallback to compiled-in resource
        else if (fallbackLoaded && pFallbackSound)
        {
            ma_sound_set_volume(pFallbackSound, volume);
            ma_sound_stop(pFallbackSound);
            ma_sound_seek_to_pcm_frame(pFallbackSound, 0);
            ma_sound_start(pFallbackSound);
        }
    }

    void AddNotification(int itemId, const Stat& stat, const std::string& specialText)
    {
        if (!g_Settings.enableNotifications) return;

        Notification n;
        n.itemId = itemId;
        n.itemName = stat.details.name;
        n.iconUrl = stat.details.iconUrl;
        n.rarity = stat.details.rarity;
        n.value = ItemTracker::GetStatProfit(stat) / (stat.count != 0 ? std::abs(stat.count) : 1);
        n.specialText = specialText;
        n.startTime = std::chrono::system_clock::now();
        n.isPrecursor = (specialText.find("Pre-Cursor") != std::string::npos);
        n.isInfusion = (specialText.find("Infusion") != std::string::npos);

        {
            std::lock_guard<std::mutex> lock(s_Mutex);
            if (!g_Settings.notificationStacking)
            {
                s_Notifications.clear();
            }
            s_Notifications.push_back(n);
        }

        PlayNotificationSound(n.isPrecursor, n.isInfusion, false);
    }

    void AddGenericNotification(const std::string& title, const std::string& message, const std::string& iconUrl, const std::string& rarity, bool isAlert)
    {
        if (!g_Settings.enableNotifications) return;

        Notification n;
        n.itemId = -1; // Special ID for generic
        n.itemName = title;
        n.specialText = message;
        n.iconUrl = iconUrl;
        n.rarity = rarity;
        n.value = 0;
        n.startTime = std::chrono::system_clock::now();
        n.isPrecursor = false;
        n.isInfusion = false;

        {
            std::lock_guard<std::mutex> lock(s_Mutex);
            if (!g_Settings.notificationStacking)
            {
                s_Notifications.clear();
            }
            s_Notifications.push_back(n);
        }

        PlayNotificationSound(false, false, isAlert);
    }

    void ClearAll()
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        s_Notifications.clear();
    }

    void Render()
    {
        if (!g_Settings.enableNotifications && !g_Settings.showNotificationSetup) return;

        std::lock_guard<std::mutex> lock(s_Mutex);
        auto now = std::chrono::system_clock::now();

        // Remove expired notifications
        s_Notifications.erase(std::remove_if(s_Notifications.begin(), s_Notifications.end(),
            [&](const Notification& n) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - n.startTime).count() / 1000.0f;
                return elapsed > g_Settings.notificationDuration;
            }), s_Notifications.end());

        if (s_Notifications.empty() && !g_Settings.showNotificationSetup) return;

        // Setup notification window
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | 
                                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | 
                                ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs;

        if (g_Settings.showNotificationSetup)
        {
            flags &= ~ImGuiWindowFlags_NoInputs;
            flags &= ~ImGuiWindowFlags_NoDecoration;
            flags &= ~ImGuiWindowFlags_AlwaysAutoResize; // Allow resizing in setup mode
            ImGui::SetNextWindowBgAlpha(0.5f);
        }
        else
        {
            ImGui::SetNextWindowBgAlpha(0.0f);
            flags |= ImGuiWindowFlags_NoBackground;
        }

        ImGui::SetNextWindowPos(ImVec2(g_Settings.notificationPosX, g_Settings.notificationPosY), ImGuiCond_FirstUseEver);
        if (g_Settings.notificationWidth > 0)
        {
            ImGui::SetNextWindowSize(ImVec2(g_Settings.notificationWidth, g_Settings.notificationHeight), ImGuiCond_FirstUseEver);
        }

        if (ImGui::Begin("##Notifications", nullptr, flags))
        {
            if (g_Settings.showNotificationSetup)
            {
                ImGui::Text("%s", Localization::GetText("notification_setup_hint"));
                g_Settings.notificationPosX = ImGui::GetWindowPos().x;
                g_Settings.notificationPosY = ImGui::GetWindowPos().y;
                g_Settings.notificationWidth = ImGui::GetWindowSize().x;
                g_Settings.notificationHeight = ImGui::GetWindowSize().y;
                
                // Show a dummy notification in setup mode
                Notification dummy;
                dummy.itemName = Localization::GetText("test_item_label");
                dummy.specialText = Localization::GetText("precursor_drop_label");
                dummy.rarity = "Legendary";
                dummy.value = 1234567;
                dummy.fadeAmount = 1.0f;
                
                // Draw dummy
                ImGui::BeginGroup();
                ImGui::TextColored(ImVec4(1, 0.84f, 0, 1), "%s", dummy.specialText.c_str());
                ImGui::Text("%s", dummy.itemName.c_str());
                ImGui::EndGroup();
            }
            else
            {
                for (size_t i = 0; i < s_Notifications.size(); ++i)
                {
                    auto& n = s_Notifications[i];
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - n.startTime).count() / 1000.0f;
                    
                    float fade = 1.0f;
                    if (elapsed < 0.5f) fade = elapsed / 0.5f; // Fade in
                    else if (elapsed > g_Settings.notificationDuration - 1.0f) fade = (g_Settings.notificationDuration - elapsed); // Fade out
                    
                    if (fade <= 0.0f) continue;

                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, fade);
                    
                    // Background for the individual toast
                    ImVec4 bgColor = ImVec4(0.1f, 0.1f, 0.1f, 0.8f);
                    if (n.isPrecursor) bgColor = ImVec4(0.3f, 0.2f, 0.0f, 0.8f);
                    else if (n.isInfusion) bgColor = ImVec4(0.2f, 0.0f, 0.3f, 0.8f);

                    ImGui::PushStyleColor(ImGuiCol_ChildBg, bgColor);
                    char childId[32];
                    snprintf(childId, sizeof(childId), "##Notify_%zu", i);
                    
                    if (ImGui::BeginChild(childId, ImVec2(300, 70), true, ImGuiWindowFlags_NoScrollbar))
                    {
                        // Icon
                        ImVec2 cursor = ImGui::GetCursorPos();
                        UICommon::DrawItemIconCell(n.itemId, n.iconUrl, 50, n.rarity);
                        
                        ImGui::SameLine();
                        ImGui::SetCursorPosY(cursor.y);
                        
                        ImGui::BeginGroup();
                        
                        // Special text (Pre-Cursor / Infusion / Rare Drop)
                        if (!n.specialText.empty())
                        {
                            ImVec4 specialColor = ImVec4(1, 0.84f, 0, 1); // Gold
                            if (n.isInfusion) specialColor = ImVec4(0.8f, 0.4f, 1.0f, 1); // Purple
                            ImGui::TextColored(specialColor, "%s", n.specialText.c_str());
                        }
                        else
                        {
                            ImGui::TextColored(ImVec4(1.f, 0.6f, 0.0f, 1.f), "%s", Localization::GetText("rare_drop_label"));
                        }

                        // Item Name with rarity color
                        ImVec4 rarityCol = ImVec4(1, 1, 1, 1);
                        if (n.rarity == "Junk") rarityCol = ImVec4(0.7f, 0.7f, 0.7f, 1.f);
                        else if (n.rarity == "Basic") rarityCol = ImVec4(1.f, 1.f, 1.f, 1.f);
                        else if (n.rarity == "Fine") rarityCol = ImVec4(0.0f, 0.5f, 1.f, 1.f);
                        else if (n.rarity == "Masterwork") rarityCol = ImVec4(0.2f, 0.8f, 0.2f, 1.f);
                        else if (n.rarity == "Rare") rarityCol = ImVec4(1.f, 0.9f, 0.0f, 1.f);
                        else if (n.rarity == "Exotic") rarityCol = ImVec4(1.f, 0.6f, 0.0f, 1.f);
                        else if (n.rarity == "Ascended") rarityCol = ImVec4(0.9f, 0.3f, 0.9f, 1.f);
                        else if (n.rarity == "Legendary") rarityCol = ImVec4(1.0f, 0.5f, 0.8f, 1.f);
                        
                        ImGui::TextColored(rarityCol, "%s", n.itemName.c_str());
                        
                        // Value
                        if (n.value > 0)
                        {
                            ImGui::Text("%s", UICommon::FormatCoin(n.value).c_str());
                        }

                        ImGui::EndGroup();
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleColor();
                    ImGui::PopStyleVar();
                    
                    ImGui::Spacing();
                }
            }
        }
        ImGui::End();
    }
}
