#include "gw2_fetcher.h"
#include "gw2_api.h"
#include "item_tracker.h"
#include "settings.h"
#include "shared.h"

#include "../include/nlohmann/json.hpp"

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

using namespace Gw2Fetcher;

static std::atomic<bool> s_Shutdown{ true };
static std::thread       s_Thread;
static std::mutex        s_CvMutex;
static std::condition_variable s_Cv;
static std::atomic<bool> s_Wake{ false };
static std::atomic<bool> s_ForceRefresh{ false };
static std::atomic<Gw2Status> s_Status{ Gw2Status::Disconnected };
static std::atomic<int> s_ReconnectCount{ 0 };
static Gw2Status s_LastLoggedStatus{ Gw2Status::Disconnected };

static void WorkerLoop()
{
    static std::map<std::string, nlohmann::json> s_CurrencyJsonCache; // Cache per language
    static std::string    s_LastApiKey;
    static std::string    s_LastLanguage;
    static auto           s_LastPriceUpdate = std::chrono::steady_clock::now();
    static bool           s_FirstRun = true;

    while (!s_Shutdown.load())
    {
        std::unique_lock<std::mutex> lk(s_CvMutex);
        s_Cv.wait_for(lk, std::chrono::milliseconds(800), [] {
            return s_Shutdown.load() || s_Wake.exchange(false);
        });
        lk.unlock();

        if (s_Shutdown.load())
            break;

        // Check price update interval
        auto now = std::chrono::steady_clock::now();
        auto elapsedMin = std::chrono::duration_cast<std::chrono::minutes>(now - s_LastPriceUpdate).count();
        bool forceUpdate = s_FirstRun || s_ForceRefresh.exchange(false); // Force update on first run or manual refresh
        s_FirstRun = false;

        std::string token = g_Settings.gw2ApiKey;
        if (forceUpdate)
         {
             s_Status.store(Gw2Status::Connecting);
             Gw2Api::Log("Force update triggered - Refreshing all data", "info");
             s_CurrencyJsonCache.clear(); // Clear cache to force re-fetch of currency table
             ItemTracker::ForceReloadAll(); // Mark all active items for re-fetch
             
             // Re-validate token info to show actual "connection" activity
            nlohmann::json tokenInfo;
            std::string err;
            if (Gw2Api::GetJson("/v2/tokeninfo", token, tokenInfo, err))
            {
                Gw2Api::Log("API Key validated successfully: " + tokenInfo.value("name", "unnamed"), "info");
            }
            else
            {
                Gw2Api::Log("API Key validation failed: " + err, "error");
            }
        }

        if (!SettingsManager::IsGw2ApiKeyPlausible(token))
        {
            s_Status.store(Gw2Status::Disconnected);
            if (s_LastLoggedStatus != Gw2Status::Disconnected)
            {
                Gw2Api::Log("Disconnected - No valid API key", "error");
                s_LastLoggedStatus = Gw2Status::Disconnected;
            }
            continue;
        }

        if (token != s_LastApiKey)
        {
            s_LastApiKey     = token;
            s_CurrencyJsonCache.clear(); // Clear cache on API key change
            s_LastLanguage.clear();
            s_ReconnectCount.store(0);
            Gw2Api::Log("Connecting to GW2 API with new API key", "info");
            forceUpdate = true;
        }

        // Check if language changed
        std::string currentLanguage = g_Settings.language;
        if (currentLanguage != s_LastLanguage)
        {
            s_LastLanguage = currentLanguage;
            s_CurrencyJsonCache.clear(); // Clear cache on language change
            Gw2Api::Log("Language changed, clearing currency cache", "info");
            forceUpdate = true;
        }

        bool needCurrencyTable = ItemTracker::NeedCurrencyTable();
        std::vector<int> pending = ItemTracker::CollectPendingItemIds();

        // Throttle only when there is nothing to fetch right now.
        // Important: pending item ids must be fetched immediately, otherwise names/icons appear "stuck"
        // until the next interval or a forced update (e.g. language change).
        if (!forceUpdate &&
            elapsedMin < g_Settings.priceUpdateIntervalMin &&
            !needCurrencyTable &&
            pending.empty())
        {
            continue;
        }

        if (elapsedMin >= g_Settings.priceUpdateIntervalMin)
            s_LastPriceUpdate = now;

        // Set status to Connected if we have a valid API key
        s_Status.store(Gw2Status::Connected);
        if (s_LastLoggedStatus != Gw2Status::Connected)
        {
            Gw2Api::Log("Connected - Valid API key present", "data");
            s_LastLoggedStatus = Gw2Status::Connected;
        }

        if (needCurrencyTable)
        {
            auto& cache = s_CurrencyJsonCache[currentLanguage];
            if (!cache.is_array())
            {
                std::string err;
                if (Gw2Api::FetchCurrenciesAll(token, cache, err))
                {
                    if (!cache.is_array())
                        cache = nlohmann::json::array();
                    s_Status.store(Gw2Status::Connected);
                    s_ReconnectCount.store(0);
                    Gw2Api::Log("Connected - Currency data fetched successfully", "data");
                }
                else
                {
                    Gw2Api::Log("Failed to fetch currency data: " + err, "error");
                    cache = nlohmann::json();
                    s_Status.store(Gw2Status::Error);
                    s_ReconnectCount.fetch_add(1);
                    if (s_LastLoggedStatus != Gw2Status::Error)
                    {
                        Gw2Api::Log("Error - Failed to fetch currency data", "error");
                        s_LastLoggedStatus = Gw2Status::Error;
                    }
                }
            }
            if (cache.is_array())
                ItemTracker::ApplyCurrencyTable(cache);
        }

        if (pending.empty())
            continue;

        nlohmann::json items, prices;
        std::string err;
        if (!Gw2Api::FetchItemsMany(pending, token, items, prices, err))
        {
            Gw2Api::Log("Failed to fetch item data: " + err, "error");
            s_Status.store(Gw2Status::Error);
            s_ReconnectCount.fetch_add(1);
            continue;
        }

        ItemTracker::ApplyItemsFromApi(items, prices);
        s_Status.store(Gw2Status::Connected);
        s_ReconnectCount.store(0);
    }

    s_Status.store(Gw2Status::Disconnected);
}

void Gw2Fetcher::Init()
{
    if (s_Thread.joinable())
        return;

    s_Shutdown.store(false);
    s_Thread = std::thread(WorkerLoop);
}

void Gw2Fetcher::Shutdown()
{
    s_Shutdown.store(true);
    s_Wake.store(true);
    s_Cv.notify_all();

    if (s_Thread.joinable())
    {
        s_Thread.join(); // Wait for thread to finish
    }
}

void Gw2Fetcher::UpdateApiKey()
{
    Gw2Api::Log("Manual GW2 API refresh requested", "info");
    s_ForceRefresh.store(true);
    s_Wake.store(true);
    s_Cv.notify_one();
}

void Gw2Fetcher::NotifyDrfActivity()
{
    s_Wake.store(true);
    s_Cv.notify_one();
}

Gw2Status Gw2Fetcher::GetStatus()
{
    return s_Status.load();
}

int Gw2Fetcher::GetReconnectCount()
{
    return s_ReconnectCount.load();
}
