#include "item_tracker.h"
#include "custom_profit.h"
#include "ignored_items.h"
#include "search_manager.h"
#include "settings.h"
#include "session_history.h"
#include "gw2_api.h"
#include "ui_notifications.h"
#include "localization.h"
#include "../include/nlohmann/json.hpp"

#include <mutex>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <set>
#include <algorithm>
#include <sstream>
#include <deque>
#include <fstream>
#include <atomic>

using json = nlohmann::json;

static std::mutex s_Mutex;
static std::map<int, Stat> s_Items;
static std::map<int, Stat> s_Currencies;
static std::atomic<int> s_MagicFind{ -1 };

static std::chrono::system_clock::time_point s_SessionStart =
    std::chrono::system_clock::now();
static std::mutex s_SessionStartMutex;

// Track all drops with timestamps for session history
static std::vector<SessionHistory::DropEntry> s_SessionDrops;
static std::mutex s_SessionDropsMutex;

// Salvage Kit data structure
struct SalvageKitInfo
{
    int goldPrice;      // Price in copper
    int karmaPrice;     // Price in karma
    int uses;           // Number of uses
    bool infinite;      // If true, uses cost per use (not calculated)
    int costPerUse;     // For infinite kits only (in copper)
};

// Salvage Kits data (ID -> Info)
static std::map<int, SalvageKitInfo> s_SalvageKits = {
    // Infinite Salvage Kits
    {44602, {0, 0, 0, true, 3}},    // Copper-Fed Salvage-o-Matic: 3 copper per use
    {67027, {0, 0, 0, true, 60}},   // Silver-Fed Salvage-o-Matic: 60 copper per use
    {89409, {0, 0, 0, true, 30}},   // Runecrafter's Salvage-o-Matic: 30 copper per use

    // Basic Salvage Kits
    {23038, {32, 28, 15, false, 0}},   // Crude Salvage Kit: 32c, 28 Karma, 15 uses
    {23040, {88, 77, 25, false, 0}},   // Basic Salvage Kit: 88c, 77 Karma, 25 uses
    {23041, {288, 252, 25, false, 0}}, // Fine Salvage Kit: 2s 88c, 252 Karma, 25 uses
    {23042, {800, 2800, 25, false, 0}}, // Journeyman Salvage Kit: 8s, 2800 Karma, 25 uses
    {23043, {1536, 5600, 25, false, 0}}, // Master Salvage Kit: 15s 36c, 5600 Karma, 25 uses
    {20185, {2624, 8652, 250, false, 0}} // Mystic Salvage Kit: 26s 24c, 8652 Karma, 250 uses
};

// Moving Average for Profit per Hour (like drf.rs)
struct ProfitHistoryEntry
{
    std::chrono::system_clock::time_point timestamp;
    long long profitPerHour;
};

static std::deque<ProfitHistoryEntry> s_ProfitHistory;
static std::chrono::system_clock::time_point s_LastHistoryUpdate =
    std::chrono::system_clock::now();
static std::mutex s_ProfitHistoryMutex; // Protect profit history access
static const int HISTORY_UPDATE_INTERVAL_SECONDS = 10; // Update every 10 seconds
static const int MAX_HISTORY_ENTRIES = 10; // Keep last 10 entries

static bool s_ProfitGoalReached = false;

static void CheckAndTriggerNotification(int apiId, Stat& st)
{
    if (!g_Settings.enableNotifications || !st.details.loaded) return;
    if (!st.notificationPending) return;

    bool shouldNotify = false;
    std::string specialText = "";

    // 1. Check for Pre-Cursor
    if (g_Settings.notificationPrecursorAlert)
    {
        std::string lowerName = st.details.name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        std::string lowerDesc = st.details.description;
        std::transform(lowerDesc.begin(), lowerDesc.end(), lowerDesc.begin(), ::tolower);
        
        // Check name, description and specific exotic weapon patterns (unbound, no vendor value, high rarity)
        if (lowerName.find("precursor") != std::string::npos || 
            lowerDesc.find("precursor") != std::string::npos ||
            (st.details.rarity == "Exotic" && st.details.itemType == ItemType::Weapon && st.details.vendorValue == 0 && !st.details.accountBound))
        {
            shouldNotify = true;
            specialText = Localization::GetText("precursor_drop_label");
        }
    }

    // 2. Check for Infusion
    if (!shouldNotify && g_Settings.notificationInfusionAlert)
    {
        std::string lowerName = st.details.name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        std::string lowerDesc = st.details.description;
        std::transform(lowerDesc.begin(), lowerDesc.end(), lowerDesc.begin(), ::tolower);

        bool isInfusion = (lowerName.find("infusion") != std::string::npos || 
                          lowerDesc.find("infusion") != std::string::npos);
        
        // Filter out Agony Infusions if not enabled
        if (isInfusion && !g_Settings.notificationIncludeAgonyInfusions)
        {
            if (lowerName.find("agony infusion") != std::string::npos || 
                lowerName.find("qual-infusion") != std::string::npos) // German check
            {
                isInfusion = false;
            }
        }

        if (isInfusion)
        {
            shouldNotify = true;
            specialText = Localization::GetText("infusion_drop_label");
        }
    }

    // 3. Check for Value and Rarity
    if (!shouldNotify)
    {
        long long value = ItemTracker::GetStatProfit(st) / (st.count != 0 ? std::abs(st.count) : 1);
        int rarityRank = ItemTracker::RarityRank(st.details.rarity);

        bool valueMet = (g_Settings.notificationEnableMinValue && value >= static_cast<long long>(g_Settings.notificationMinValueGold * 10000));
        bool rarityMet = (g_Settings.notificationEnableMinRarity && rarityRank >= g_Settings.notificationMinRarity);

        if (g_Settings.notificationCombineValueAndRarity)
        {
            // Combined logic: Both must be met if both are enabled
            if (g_Settings.notificationEnableMinValue && g_Settings.notificationEnableMinRarity)
            {
                shouldNotify = (valueMet && rarityMet);
            }
            else
            {
                // If only one is enabled, treat it as if that one must be met
                shouldNotify = (valueMet || rarityMet);
            }
        }
        else
        {
            // Standard logic: Any one of them met is enough
            shouldNotify = (valueMet || rarityMet);
        }
    }

    if (shouldNotify)
    {
        UINotifications::AddNotification(apiId, st, specialText);
    }
    st.notificationPending = false;
}

static void UpdateOrInsert(std::map<int, Stat>& map,
                           int apiId, long long delta, StatType type)
{
    auto it = map.find(apiId);
    if (it != map.end())
    {
        it->second.count += delta;
        if (delta > 0)
        {
            it->second.notificationPending = true;
            CheckAndTriggerNotification(apiId, it->second);
        }
    }
    else
    {
        Stat s;
        s.apiId = apiId;
        s.type  = type;
        s.count = delta;
        if (delta > 0) s.notificationPending = true;
        map[apiId] = s;
        if (delta > 0) CheckAndTriggerNotification(apiId, map[apiId]);
    }
}

void ItemTracker::AddDrop(const std::map<int, long long>& items,
                          const std::map<int, long long>& currencies)
{
    std::lock_guard<std::mutex> lock(s_Mutex);

    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &now_time);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm);

    // Always record drops with timestamps (will only be saved if flag is enabled)
    // This prevents data loss if flag is changed mid-session
    std::lock_guard<std::mutex> dropsLock(s_SessionDropsMutex);

    for (auto& [id, delta] : items)
    {
        SessionHistory::DropEntry drop;
        drop.itemId = id;
        drop.itemName = ""; // Will be filled when saving
        drop.isCurrency = false;
        drop.rarity = "";
        drop.count = static_cast<int>(delta);
        drop.totalValue = 0; // Will be calculated when saving
        drop.magicFind = s_MagicFind.load();
        drop.timestamp = timestamp;
        s_SessionDrops.push_back(drop);
    }

    for (auto& [id, delta] : currencies)
    {
        SessionHistory::DropEntry drop;
        drop.itemId = id;
        drop.itemName = ""; // Will be filled when saving
        drop.isCurrency = true;
        drop.rarity = "";
        drop.count = static_cast<int>(delta);
        drop.totalValue = 0; // Will be calculated when saving
        drop.magicFind = s_MagicFind.load();
        drop.timestamp = timestamp;
        s_SessionDrops.push_back(drop);
    }

    for (auto& [id, delta] : items)
    {
        UpdateOrInsert(s_Items, id, delta, StatType::Item);
        s_Items[id].lastMagicFind = s_MagicFind.load();
    }

    for (auto& [id, delta] : currencies)
    {
        UpdateOrInsert(s_Currencies, id, delta, StatType::Currency);
        s_Currencies[id].lastMagicFind = s_MagicFind.load();
    }
}

void ItemTracker::SetMagicFind(int magicFind)
{
    s_MagicFind.store(magicFind);
}

int ItemTracker::GetMagicFind()
{
    return s_MagicFind.load();
}

void ItemTracker::SaveCurrentSession()
{
    if (!g_Settings.enableSessionHistory)
        return;

    // Collect session data
    SessionHistory::SessionData sessionData;

    // Get session duration
    auto duration = GetSessionDuration();
    sessionData.durationSeconds = static_cast<int>(duration.count());
    sessionData.averageMagicFind = s_MagicFind.load();

    // Get session start and end time
    auto now = std::chrono::system_clock::now();
    auto nowTimeT = std::chrono::system_clock::to_time_t(now);
    struct tm timeInfo;
    localtime_s(&timeInfo, &nowTimeT);
    char timeBuffer[64];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &timeInfo);
    sessionData.endTime = timeBuffer;

    // Calculate start time
    auto startTime = now - duration;
    auto startTimeT = std::chrono::system_clock::to_time_t(startTime);
    localtime_s(&timeInfo, &startTimeT);
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &timeInfo);
    sessionData.startTime = timeBuffer;

    // Get profit data
    {
        std::lock_guard<std::mutex> profitLock(s_ProfitHistoryMutex);
        long long totalProfit = 0;
        for (const auto& entry : s_ProfitHistory)
        {
            totalProfit += entry.profitPerHour;
        }
        sessionData.totalProfit = totalProfit;

        if (duration.count() > 0)
        {
            sessionData.profitPerHour = (totalProfit * 3600) / duration.count();
        }
        else
        {
            sessionData.profitPerHour = 0;
        }
    }

    // Get items and collect top drops and rarity counts
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        sessionData.totalDrops = static_cast<int>(s_Items.size());

        // Collect top drops (by value)
        std::vector<std::pair<long long, SessionHistory::DropEntry>> drops;
        for (const auto& [id, stat] : s_Items)
        {
            long long value = GetStatProfit(stat);
            SessionHistory::DropEntry drop;
            drop.itemId = id;
            drop.itemName = stat.details.loaded ? stat.details.name : "Unknown";
            drop.iconUrl = stat.details.loaded ? stat.details.iconUrl : "";
            drop.isCurrency = false;
            drop.rarity = stat.details.loaded ? stat.details.rarity : "Unknown";
            drop.count = static_cast<int>(stat.count);
            drop.totalValue = value;
            drops.push_back({value, drop});

            // Collect rarity counts
            std::string rarity = stat.details.loaded ? stat.details.rarity : "Unknown";
            sessionData.rarityCounts[rarity]++;
        }

        // Sort by value descending and take top 10
        std::sort(drops.begin(), drops.end(), [](const auto& a, const auto& b) {
            return a.first > b.first;
        });

        for (size_t i = 0; i < std::min<size_t>(drops.size(), 10); i++)
        {
            sessionData.topDrops.push_back(drops[i].second);
        }

        // Populate allDrops with item details
        std::lock_guard<std::mutex> dropsLock(s_SessionDropsMutex);
        for (auto& drop : s_SessionDrops)
        {
            // Find item details from current session items
            auto it = s_Items.find(drop.itemId);
            if (it != s_Items.end())
            {
                drop.itemName = it->second.details.loaded ? it->second.details.name : "Unknown";
                drop.iconUrl = it->second.details.loaded ? it->second.details.iconUrl : "";
                drop.rarity = it->second.details.loaded ? it->second.details.rarity : "Unknown";
                drop.totalValue = GetStatProfit(it->second);
            }
            else
            {
                // Check currencies
                auto currIt = s_Currencies.find(drop.itemId);
                if (currIt != s_Currencies.end())
                {
                    drop.itemName = currIt->second.details.loaded ? currIt->second.details.name : "Unknown";
                    drop.iconUrl = currIt->second.details.loaded ? currIt->second.details.iconUrl : "";
                    drop.rarity = currIt->second.details.loaded ? currIt->second.details.rarity : "Unknown";
                    drop.totalValue = GetStatProfit(currIt->second);
                }
            }
            sessionData.allDrops.push_back(drop);
        }

        // Map name (placeholder - would need DRF or GW2 API for actual map)
        sessionData.mapName = "Unknown";
    }

    // Save session
    SessionHistory::SaveSession(sessionData);

    // Clear session drops after saving
    {
        std::lock_guard<std::mutex> dropsLock(s_SessionDropsMutex);
        s_SessionDrops.clear();
    }
}

void ItemTracker::Reset()
{
    // Save session history before resetting
    SaveCurrentSession();

    // Lock profit history FIRST to avoid deadlock with UpdateProfitHistory
    std::lock_guard<std::mutex> profitLock(s_ProfitHistoryMutex);
    s_ProfitHistory.clear();
    s_LastHistoryUpdate = std::chrono::system_clock::now();
    s_ProfitGoalReached = false;

    std::lock_guard<std::mutex> sessionLock(s_SessionStartMutex);
    s_SessionStart = std::chrono::system_clock::now();

    // Clear session drops
    {
        std::lock_guard<std::mutex> dropsLock(s_SessionDropsMutex);
        s_SessionDrops.clear();
    }

    // Lock items/currencies LAST (after profit history)
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_Items.clear();
    s_Currencies.clear();
}

void ItemTracker::ClearPersistedData(const char* addonDir)
{
    if (!addonDir)
        return;

    std::string dataPath = std::string(addonDir) + "\\farming_data.json";
    
    // Delete the persistence file
    std::remove(dataPath.c_str());
}

std::map<int, Stat> ItemTracker::GetItemsCopy()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    return s_Items;
}

std::map<int, Stat> ItemTracker::GetCurrenciesCopy()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    return s_Currencies;
}

Stat ItemTracker::GetItemStat(int itemId)
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    auto it = s_Items.find(itemId);
    if (it != s_Items.end()) return it->second;
    Stat empty;
    empty.apiId = itemId;
    empty.type = StatType::Item;
    return empty;
}

Stat ItemTracker::GetCurrencyStat(int currencyId)
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    auto it = s_Currencies.find(currencyId);
    if (it != s_Currencies.end()) return it->second;
    Stat empty;
    empty.apiId = currencyId;
    empty.type = StatType::Currency;
    return empty;
}

std::chrono::seconds ItemTracker::GetSessionDuration()
{
    std::lock_guard<std::mutex> lock(s_SessionStartMutex);
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - s_SessionStart);
}

// === Advanced Features Implementation ===

// Stat method implementations
bool Stat::HasCustomProfit() const
{
    return CustomProfitManager::HasCustomProfit(apiId);
}

long long Stat::GetCustomProfit() const
{
    return CustomProfitManager::GetCustomProfit(apiId);
}

long long Stat::GetMaxProfit() const
{
    if (HasCustomProfit())
        return GetCustomProfit();
    
    if (IsCoin())
        return count;
    
    if (!details.loaded)
        return 0;
    
    long long tpProfit = ItemTracker::TpSellProceedsPerUnitCopper(details);
    long long vendorProfit = details.noSell ? 0 : details.vendorValue;
    return std::max(tpProfit, vendorProfit);
}

// Favorites System
void ItemTracker::SetFavorite(int apiId, bool favorite)
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    
    // Update in items
    auto itemIt = s_Items.find(apiId);
    if (itemIt != s_Items.end())
        itemIt->second.isFavorite = favorite;
    
    // Update in currencies
    auto currencyIt = s_Currencies.find(apiId);
    if (currencyIt != s_Currencies.end())
        currencyIt->second.isFavorite = favorite;

    // If adding to favorites, remove from ignored
    if (favorite)
    {
        IgnoredItemsManager::UnignoreItem(apiId);
        IgnoredItemsManager::UnignoreCurrency(apiId);
    }
}

bool ItemTracker::IsFavorite(int apiId)
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    
    auto itemIt = s_Items.find(apiId);
    if (itemIt != s_Items.end())
        return itemIt->second.isFavorite;
    
    auto currencyIt = s_Currencies.find(apiId);
    if (currencyIt != s_Currencies.end())
        return currencyIt->second.isFavorite;
    
    return false;
}

std::map<int, Stat> ItemTracker::GetFavoriteItems()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    std::map<int, Stat> favorites;
    
    for (const auto& [id, stat] : s_Items)
        if (stat.isFavorite)
            favorites[id] = stat;
    
    return favorites;
}

std::map<int, Stat> ItemTracker::GetFavoriteCurrencies()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    std::map<int, Stat> favorites;
    
    for (const auto& [id, stat] : s_Currencies)
        if (stat.isFavorite)
            favorites[id] = stat;
    
    return favorites;
}

std::vector<SessionHistory::DropEntry> ItemTracker::GetSessionDropsCopy()
{
    std::lock_guard<std::mutex> lock(s_SessionDropsMutex);
    std::vector<SessionHistory::DropEntry> drops = s_SessionDrops;

    // Fill in details from current stats if possible
    std::lock_guard<std::mutex> statsLock(s_Mutex);
    for (auto& drop : drops)
    {
        if (drop.isCurrency)
        {
            auto it = s_Currencies.find(drop.itemId);
            if (it != s_Currencies.end() && it->second.details.loaded)
            {
                drop.itemName = it->second.details.name;
                drop.iconUrl = it->second.details.iconUrl;
                drop.rarity = it->second.details.rarity;
                drop.totalValue = GetStatProfit(it->second) / (it->second.count > 0 ? it->second.count : 1) * drop.count;
            }
        }
        else
        {
            auto it = s_Items.find(drop.itemId);
            if (it != s_Items.end() && it->second.details.loaded)
            {
                drop.itemName = it->second.details.name;
                drop.iconUrl = it->second.details.iconUrl;
                drop.rarity = it->second.details.rarity;
                drop.totalValue = GetStatProfit(it->second) / (it->second.count > 0 ? it->second.count : 1) * drop.count;
            }
        }
    }

    return drops;
}

std::string ItemTracker::GetCurrencyCategory(int currencyId)
{
    switch (currencyId)
    {
        case 1: // Gold
        case 2: // Karma
        case 3: // Laurels
        case 4: // Gems
        case 18: // Transmutation Charges
        case 23: // Spirit Shards
        case 61: // Research Notes
        case 72: // Astral Acclaim
            return Localization::GetText("currency_cat_common");

        case 7: // Fractal Relics
        case 24: // Pristine Fractal Relics
        case 50: // Unstable Fractal Essence
            return Localization::GetText("currency_cat_fractal");

        case 28: // Magnetite Shards
        case 35: // Gaeting Crystals
        case 53: // Prophet Shards
        case 57: // Green Prophet Shards
        case 70: // Legendary Insight
            return Localization::GetText("currency_cat_raid_strike");

        case 15: // Badges of Honor
        case 26: // WvW Skirmish Claim Tickets
        case 31: // Proofs of Heroics
        case 36: // Testimony of Desert Heroics
        case 65: // Testimony of Jade Heroics
        case 82: // Testimony of Castoran Heroics
            return Localization::GetText("currency_cat_wvw");

        case 30: // PvP League Tickets
        case 33: // Ascended Shards of Glory
            return Localization::GetText("currency_cat_pvp");

        case 19: // Airship Parts
        case 20: // Ley Line Crystals
        case 22: // Geodes
        case 32: // Unbound Magic
        case 38: // Trade Contracts
        case 39: // Elegy Mosaics
        case 45: // Volatile Magic
        case 52: // Tyrian Defense Seals
        case 62: // Unusual Coins
        case 64: // Jade Slivers
        case 66: // Ancient Coins
        case 67: // Canach Coins
        case 68: // Imperial Favor
        case 69: // Tales of Dungeon Delving
        case 73: // Pinch of Stardust
        case 75: // Calcified Gasps
        case 78: // Static Charges
            return Localization::GetText("currency_cat_map");

        case 76: // Ursus Oblige
        case 77: // Gaeting Crystal (Janthir)
        case 81: // Antiquated Ducat
        case 83: // Aether-Rich Sap
            return Localization::GetText("currency_cat_janthir");

        default:
            return Localization::GetText("currency_cat_other");
    }
}

// Ignored Items (delegates to IgnoredItemsManager)
bool ItemTracker::IsItemIgnored(int apiId)
{
    return IgnoredItemsManager::IsItemIgnored(apiId);
}

bool ItemTracker::IsCurrencyIgnored(int apiId)
{
    return IgnoredItemsManager::IsCurrencyIgnored(apiId);
}

// Advanced Filtering
bool ItemTracker::PassesFilter(const Stat& stat)
{
    // Count filter - but always allow coins like DRF does
    if (stat.IsCoin()) return true; // Always include coins in profit calculations
    
    // Ignored items filter
    if (stat.IsItem() && IsItemIgnored(stat.apiId) && !g_Settings.filterIgnored) return false;
    if (stat.IsCurrency() && IsCurrencyIgnored(stat.apiId) && !g_Settings.filterIgnored) return false;
    if (stat.IsItem() && !IsItemIgnored(stat.apiId) && !g_Settings.filterNotIgnored) return false;
    if (stat.IsCurrency() && !IsCurrencyIgnored(stat.apiId) && !g_Settings.filterNotIgnored) return false;

    // Favorite filter
    if (stat.IsItem() && stat.isFavorite && !g_Settings.filterFavorite) return false;
    if (stat.IsItem() && !stat.isFavorite && !g_Settings.filterNotFavorite) return false;
    if (stat.IsCurrency() && stat.isFavorite && !g_Settings.filterFavorite) return false;
    if (stat.IsCurrency() && !stat.isFavorite && !g_Settings.filterNotFavorite) return false;

    // API knowledge filter
    if (!stat.details.knownByApi && !g_Settings.filterUnknownByApi) return false;
    if (stat.details.knownByApi && !g_Settings.filterKnownByApi) return false;
    
    // Sell method filters
    if (stat.IsItem() && stat.details.loaded)
    {
        bool canSellToVendor = CanSellToVendor(stat.details);
        bool canSellOnTp = CanSellOnTp(stat.details);
        bool hasCustomProfit = stat.HasCustomProfit();

        if (canSellToVendor && !g_Settings.filterSellableToVendor) return false;
        if (canSellOnTp && !g_Settings.filterSellableOnTp) return false;
        if (hasCustomProfit && !g_Settings.filterCustomProfit) return false;

        // Account-bound filter
        if (stat.details.accountBound && !g_Settings.filterAccountBound) return false;
        if (!stat.details.accountBound && !g_Settings.filterNotAccountBound) return false;

        // NoSell filter
        if (stat.details.noSell && !g_Settings.filterNoSell) return false;
        if (!stat.details.noSell && !g_Settings.filterNotNoSell) return false;
    }
    
    // Item type filter
    if (stat.IsItem() && stat.details.loaded)
    {
        switch (stat.details.itemType)
        {
            case ItemType::Armor: if (!g_Settings.filterTypeArmor) return false; break;
            case ItemType::Weapon: if (!g_Settings.filterTypeWeapon) return false; break;
            case ItemType::Trinket: if (!g_Settings.filterTypeTrinket) return false; break;
            case ItemType::Gizmo: if (!g_Settings.filterTypeGizmo) return false; break;
            case ItemType::CraftingMaterial: if (!g_Settings.filterTypeCraftingMaterial) return false; break;
            case ItemType::Consumable: if (!g_Settings.filterTypeConsumable) return false; break;
            case ItemType::GatheringTool: if (!g_Settings.filterTypeGatheringTool) return false; break;
            case ItemType::Bag: if (!g_Settings.filterTypeBag) return false; break;
            case ItemType::Container: if (!g_Settings.filterTypeContainer) return false; break;
            case ItemType::MiniPet: if (!g_Settings.filterTypeMiniPet) return false; break;
            case ItemType::GizmoContainer: if (!g_Settings.filterTypeGizmoContainer) return false; break;
            case ItemType::Backpack: if (!g_Settings.filterTypeBackpack) return false; break;
            case ItemType::UpgradeComponent: if (!g_Settings.filterTypeUpgradeComponent) return false; break;
            case ItemType::Tool: if (!g_Settings.filterTypeTool) return false; break;
            case ItemType::Trophy: if (!g_Settings.filterTypeTrophy) return false; break;
            case ItemType::Unlock: if (!g_Settings.filterTypeUnlock) return false; break;
            default: break;
        }
    }
    
    // Currency filter
    if (stat.IsCurrency())
    {
        switch (stat.apiId)
        {
            case 1: break; // Gold - always show
            case 2: if (!g_Settings.filterKarma) return false; break;
            case 3: if (!g_Settings.filterLaurel) return false; break;
            case 4: if (!g_Settings.filterGem) return false; break;
            case 7: if (!g_Settings.filterFractalRelic) return false; break;
            case 15: if (!g_Settings.filterBadgeOfHonor) return false; break;
            case 16: if (!g_Settings.filterGuildCommendation) return false; break;
            case 18: if (!g_Settings.filterTransmutationCharge) return false; break;
            case 23: if (!g_Settings.filterSpiritShards) return false; break;
            case 32: if (!g_Settings.filterUnboundMagic) return false; break;
            case 45: if (!g_Settings.filterVolatileMagic) return false; break;
            case 19: if (!g_Settings.filterAirshipParts) return false; break;
            case 22: if (!g_Settings.filterGeode) return false; break;
            case 20: if (!g_Settings.filterLeyLineCrystals) return false; break;
            case 38: if (!g_Settings.filterTradeContracts) return false; break;
            case 39: if (!g_Settings.filterElegyMosaic) return false; break;
            case 72: if (!g_Settings.filterAstralAcclaim) return false; break;
            case 24: if (!g_Settings.filterPristineFractalRelics) return false; break;
            case 50: if (!g_Settings.filterUnstableFractalEssence) return false; break;
            case 28: if (!g_Settings.filterMagnetiteShards) return false; break;
            case 35: if (!g_Settings.filterGaetingCrystals) return false; break;
            case 53: if (!g_Settings.filterProphetShards) return false; break;
            case 57: if (!g_Settings.filterGreenProphetShards) return false; break;
            case 26: if (!g_Settings.filterWvWSkirmishTickets) return false; break;
            case 31: if (!g_Settings.filterProofsOfHeroics) return false; break;
            case 30: if (!g_Settings.filterPvpLeagueTickets) return false; break;
            case 33: if (!g_Settings.filterAscendedShardsOfGlory) return false; break;
            case 61: if (!g_Settings.filterResearchNotes) return false; break;
            case 52: if (!g_Settings.filterTyrianDefenseSeal) return false; break;
            case 36: if (!g_Settings.filterTestimonyOfDesertHeroics) return false; break;
            case 65: if (!g_Settings.filterTestimonyOfJadeHeroics) return false; break;
            case 82: if (!g_Settings.filterTestimonyOfCastoranHeroics) return false; break;
            case 70: if (!g_Settings.filterLegendaryInsight) return false; break;
            case 69: if (!g_Settings.filterTalesOfDungeonDelving) return false; break;
            case 68: if (!g_Settings.filterImperialFavor) return false; break;
            case 67: if (!g_Settings.filterCanachCoins) return false; break;
            case 66: if (!g_Settings.filterAncientCoin) return false; break;
            case 62: if (!g_Settings.filterUnusualCoin) return false; break;
            case 64: if (!g_Settings.filterJadeSliver) return false; break;
            case 78: if (!g_Settings.filterStaticCharge) return false; break;
            case 73: if (!g_Settings.filterPinchOfStardust) return false; break;
            case 75: if (!g_Settings.filterCalcifiedGasp) return false; break;
            case 76: if (!g_Settings.filterUrsusOblige) return false; break;
            case 77: if (!g_Settings.filterGaetingCrystalJanthir) return false; break;
            case 81: if (!g_Settings.filterAntiquatedDucat) return false; break;
            case 83: if (!g_Settings.filterAetherRichSap) return false; break;
            default: break;
        }
    }

    // Price range filter
    if (stat.IsItem() && stat.details.loaded)
    {
        long long pricePerUnit = GetStatProfit(stat) / (stat.count != 0 ? stat.count : 1);
        
        // Convert min price to copper
        long long minPriceCopper = g_Settings.filterMinPriceGold * 10000 + 
                                    g_Settings.filterMinPriceSilver * 100 + 
                                    g_Settings.filterMinPriceCopper;
        // Convert max price to copper
        long long maxPriceCopper = g_Settings.filterMaxPriceGold * 10000 + 
                                    g_Settings.filterMaxPriceSilver * 100 + 
                                    g_Settings.filterMaxPriceCopper;
        
        if (minPriceCopper > 0 && pricePerUnit < minPriceCopper) return false;
        if (maxPriceCopper > 0 && pricePerUnit > maxPriceCopper) return false;
    }

    // Quantity range filter
    if (g_Settings.filterMinQuantity > 0 && std::abs(stat.count) < g_Settings.filterMinQuantity) return false;
    if (g_Settings.filterMaxQuantity > 0 && std::abs(stat.count) > g_Settings.filterMaxQuantity) return false;

    // Rarity filter
    if (stat.IsItem() && g_Settings.itemRarityFilterMin > 0 && stat.details.loaded)
    {
        if (RarityRank(stat.details.rarity) < g_Settings.itemRarityFilterMin) return false;
    }

    return true;
}

std::map<int, Stat> ItemTracker::GetFilteredItems()
{
    std::map<int, Stat> filtered;
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        
        // If limit is set, we need to handle it. 
        // We'll collect all valid items first, then apply limit if necessary.
        std::vector<std::pair<long long, Stat>> candidates;

        for (const auto& [id, stat] : s_Items)
        {
            Stat copy = stat;
            copy.isIgnored = IgnoredItemsManager::IsItemIgnored(id);
            if (PassesFilter(copy))
            {
                // We store the timestamp or order if we had one, 
                // but since we don't have a "last drop time" per unique item type in Stat,
                // we'll use the current count or just the fact that it's in the map.
                // For a "History Limit", we'll sort by the order they were added if possible,
                // but s_Items is a map (sorted by ID).
                candidates.push_back({0, copy}); // No specific order for now
            }
        }

        // Apply history limit if enabled (> 0)
        // Since we don't have timestamps, we just take the first N for now.
        // In a real history, we'd want the newest drops.
        size_t limit = static_cast<size_t>(g_Settings.maxHistoryItems);
        if (limit > 0 && candidates.size() > limit)
        {
            // If we have more than the limit, we'll just take the last 'limit' items
            // (this is a bit arbitrary without timestamps, but better than nothing)
            for (size_t i = candidates.size() - limit; i < candidates.size(); ++i)
            {
                filtered[candidates[i].second.apiId] = candidates[i].second;
            }
        }
        else
        {
            for (const auto& cand : candidates)
            {
                filtered[cand.second.apiId] = cand.second;
            }
        }
    }
    return filtered;
}

std::map<int, Stat> ItemTracker::GetFilteredCurrencies()
{
    auto currencies = GetCurrenciesCopy();
    std::map<int, Stat> filtered;
    for (auto& [id, st] : currencies)
    {
        if (id == 44602 || id == 67027 || id == 89409)
            continue;

        st.isIgnored = IgnoredItemsManager::IsCurrencyIgnored(id);
        if (PassesFilter(st))
            filtered[id] = st;
    }
    return filtered;
}

// Search functionality
std::map<int, Stat> ItemTracker::GetSearchedItems(const std::string& searchTerm)
{
    auto items = GetFilteredItems();
    if (searchTerm.empty())
        return items;
    
    std::map<int, Stat> searched;
    for (const auto& [id, stat] : items)
        if (SearchManager::MatchesSearch(stat.details.name, searchTerm))
            searched[id] = stat;
    
    return searched;
}

std::map<int, Stat> ItemTracker::GetSearchedCurrencies(const std::string& searchTerm)
{
    auto currencies = GetFilteredCurrencies();
    if (searchTerm.empty())
        return currencies;
    
    std::map<int, Stat> searched;
    for (const auto& [id, stat] : currencies)
        if (SearchManager::MatchesSearchCurrency(stat.details.name, searchTerm))
            searched[id] = stat;
    
    return searched;
}

// Multi-Sort implementation
std::vector<std::pair<int, Stat>> ItemTracker::GetSortedItems(SortMode mode, bool)
{
    auto items = GetFilteredItems();
    std::vector<std::pair<int, Stat>> sorted(items.begin(), items.end());

    std::sort(sorted.begin(), sorted.end(), [mode](const auto& a, const auto& b) {
        const auto& statA = a.second;
        const auto& statB = b.second;

        // Favorites first if enabled
        if (g_Settings.favoritesFirst)
        {
            if (statA.isFavorite != statB.isFavorite)
                return statA.isFavorite > statB.isFavorite;
        }

        switch (mode)
        {
            case SortMode::PriceDesc:
                return GetStatProfit(statA) > GetStatProfit(statB);
            case SortMode::PriceAsc:
                return GetStatProfit(statA) < GetStatProfit(statB);
            case SortMode::CountDesc:
                return std::abs(statA.count) > std::abs(statB.count);
            case SortMode::CountAsc:
                return std::abs(statA.count) < std::abs(statB.count);
            case SortMode::NameAZ:
                return statA.details.name < statB.details.name;
            case SortMode::NameZA:
                return statA.details.name > statB.details.name;
            case SortMode::ProfitDesc:
                return statA.GetMaxProfit() > statB.GetMaxProfit();
            case SortMode::ProfitAsc:
                return statA.GetMaxProfit() < statB.GetMaxProfit();
            case SortMode::RarityDesc:
                return RarityRank(statA.details.rarity) > RarityRank(statB.details.rarity);
            case SortMode::RarityAsc:
                return RarityRank(statA.details.rarity) < RarityRank(statB.details.rarity);
            case SortMode::TypeAZ:
                return static_cast<int>(statA.details.itemType) < static_cast<int>(statB.details.itemType);
            case SortMode::TypeZA:
                return static_cast<int>(statA.details.itemType) > static_cast<int>(statB.details.itemType);
            default:
                return false;
        }
    });

    return sorted;
}

std::vector<std::pair<int, Stat>> ItemTracker::GetSortedCurrencies(SortMode mode, bool)
{
    auto currencies = GetFilteredCurrencies();
    std::vector<std::pair<int, Stat>> sorted(currencies.begin(), currencies.end());

    std::sort(sorted.begin(), sorted.end(), [mode](const auto& a, const auto& b) {
        const auto& statA = a.second;
        const auto& statB = b.second;

        // Favorites first if enabled
        if (g_Settings.favoritesFirst)
        {
            if (statA.isFavorite != statB.isFavorite)
                return statA.isFavorite > statB.isFavorite;
        }

        switch (mode)
        {
            case SortMode::PriceDesc:
                return GetStatProfit(statA) > GetStatProfit(statB);
            case SortMode::PriceAsc:
                return GetStatProfit(statA) < GetStatProfit(statB);
            case SortMode::CountDesc:
                return std::abs(statA.count) > std::abs(statB.count);
            case SortMode::CountAsc:
                return std::abs(statA.count) < std::abs(statB.count);
            case SortMode::NameAZ:
                return statA.details.name < statB.details.name;
            case SortMode::NameZA:
                return statA.details.name > statB.details.name;
            case SortMode::ProfitDesc:
                return statA.GetMaxProfit() > statB.GetMaxProfit();
            case SortMode::ProfitAsc:
                return statA.GetMaxProfit() < statB.GetMaxProfit();
            default:
                return false;
        }
    });

    return sorted;
}

// Custom Profit Integration
long long ItemTracker::GetStatProfit(const Stat& stat)
{
    // Use custom profit if set
    if (stat.HasCustomProfit())
        return CustomProfitManager::GetCustomProfit(stat.apiId) * stat.count;

    // Use MAX profit from Vendor or TP Sell (TP Buy is buy price, not profit)
    if (stat.IsCoin())
        return stat.count; // Coins are counted directly

    // Salvage kits are not calculated as cost, just show negative count
    if (s_SalvageKits.find(stat.apiId) != s_SalvageKits.end())
        return 0; // Salvage kits only show usage count, no profit calculation

    // Calculate all possible sell values per unit
    long long vendorPrice = stat.details.vendorValue;
    long long tpSellPrice = static_cast<long long>(stat.details.tpSellPrice * 85.0 / 100.0); // 15% fee on sell

    // Check vendor price (if not noSell)
    long long maxPrice = 0;
    if (!stat.details.noSell && vendorPrice > 0) {
        maxPrice = vendorPrice;
    }

    // Check TP price (API is source of truth - if it returns a price, the item is tradable)
    if (tpSellPrice > maxPrice) {
        maxPrice = tpSellPrice;
    }

    // If no price is available, item is not tradeable
    if (maxPrice == 0)
        return 0;

    return maxPrice * stat.count;
}

long long ItemTracker::GetStatProfitPerHour(const Stat& stat, std::chrono::seconds sessionDuration)
{
    long long totalProfit = GetStatProfit(stat);
    
    if (totalProfit == 0)
        return 0;
    
    double sessionSeconds = static_cast<double>(sessionDuration.count());
    if (sessionSeconds < 1.0) // session just started - avoid inflated values
        return 0;
    
    double hours = sessionSeconds / 3600.0;
    return static_cast<long long>(totalProfit / hours);
}

long long ItemTracker::GetTotalProfitPerHour(std::chrono::seconds sessionDuration)
{
    // Update profit history first
    UpdateProfitHistory();

    // Use moving average immediately if we have history
    if (!s_ProfitHistory.empty())
        return GetMovingAverageProfitPerHour();

    // Fallback: simple calculation if no history yet
    long long totalProfit = CalcTotalCustomProfit();

    double sessionSeconds = static_cast<double>(sessionDuration.count());
    if (sessionSeconds < 1.0) // session just started - avoid inflated values
        return 0;

    double hours = sessionSeconds / 3600.0;
    if (hours < 0.001) // avoid division by very small numbers
        return 0;

    long long currentProfitPerHour = static_cast<long long>(totalProfit / hours);

    return currentProfitPerHour;
}

long long ItemTracker::GetTpSellProfitPerHour(std::chrono::seconds sessionDuration)
{
    long long tpSellProfit = CalcTotalTpSellProfit();

    double sessionSeconds = static_cast<double>(sessionDuration.count());
    if (sessionSeconds < 1.0)
        return 0;

    double hours = sessionSeconds / 3600.0;
    if (hours < 0.001)
        return 0;

    long long tpSellPerHour = static_cast<long long>(tpSellProfit / hours);

    return tpSellPerHour;
}

long long ItemTracker::GetOpportunityCostProfit()
{
    // Opportunity Cost = TP Sell Price - Actual Profit
    // Calculate TP Sell Profit (max possible)
    long long tpSellProfit = CalcTotalTpSellProfit();
    long long actualProfit = CalcTotalCustomProfit();

    // Opportunity Cost is the difference
    return tpSellProfit - actualProfit;
}

long long ItemTracker::GetOpportunityCostProfitPerHour(std::chrono::seconds sessionDuration)
{
    long long opportunityCost = GetOpportunityCostProfit();

    double sessionSeconds = static_cast<double>(sessionDuration.count());
    if (sessionSeconds < 1.0)
        return 0;

    double hours = sessionSeconds / 3600.0;
    if (hours < 0.001)
        return 0;

    long long opportunityCostPerHour = static_cast<long long>(opportunityCost / hours);

    return opportunityCostPerHour;
}

std::string ItemTracker::ExportToJson()
{
    nlohmann::json exportData;
    exportData["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
    exportData["sessionDuration"] = GetSessionDuration().count();
    exportData["totalProfit"] = CalcTotalCustomProfit();
    exportData["totalTpSellProfit"] = CalcTotalTpSellProfit();
    exportData["totalVendorProfit"] = CalcTotalVendorProfit();

    nlohmann::json itemsArray = nlohmann::json::array();
    auto items = GetItemsCopy();
    for (auto& [id, stat] : items)
    {
        nlohmann::json item;
        item["apiId"] = id;
        item["count"] = stat.count;
        item["name"] = stat.details.loaded ? stat.details.name : "Unknown";
        item["rarity"] = stat.details.loaded ? stat.details.rarity : "Unknown";
        item["type"] = stat.details.loaded ? static_cast<int>(stat.details.itemType) : 0;
        item["profit"] = GetStatProfit(stat);
        item["isFavorite"] = stat.isFavorite;
        item["isIgnored"] = stat.isIgnored;
        item["vendorValue"] = stat.details.loaded ? stat.details.vendorValue : 0;
        item["tpSellPrice"] = stat.details.loaded ? stat.details.tpSellPrice : 0;
        item["tpBuyPrice"] = stat.details.loaded ? stat.details.tpBuyPrice : 0;
        itemsArray.push_back(item);
    }
    exportData["items"] = itemsArray;

    nlohmann::json currenciesArray = nlohmann::json::array();
    auto currencies = GetCurrenciesCopy();
    for (auto& [id, stat] : currencies)
    {
        nlohmann::json currency;
        currency["apiId"] = id;
        currency["count"] = stat.count;
        currency["name"] = stat.details.loaded ? stat.details.name : "Unknown";
        currency["isFavorite"] = stat.isFavorite;
        currency["isIgnored"] = stat.isIgnored;
        currenciesArray.push_back(currency);
    }
    exportData["currencies"] = currenciesArray;

    return exportData.dump(4);
}

std::string ItemTracker::ExportToCsv()
{
    std::stringstream csv;
    csv << "Type,API ID,Name,Count,Profit,Rarity,Is Favorite,Is Ignored,Vendor Value,TP Sell Price,TP Buy Price\n";

    auto items = GetItemsCopy();
    for (auto& [id, stat] : items)
    {
        std::string name = stat.details.loaded ? stat.details.name : "Unknown";
        std::string rarity = stat.details.loaded ? stat.details.rarity : "Unknown";
        long long profit = GetStatProfit(stat);
        int vendorValue = stat.details.loaded ? stat.details.vendorValue : 0;
        int tpSellPrice = stat.details.loaded ? stat.details.tpSellPrice : 0;
        int tpBuyPrice = stat.details.loaded ? stat.details.tpBuyPrice : 0;

        csv << "Item," << id << ",\"" << name << "\"," << stat.count << "," << profit << ",\""
            << rarity << "\"," << (stat.isFavorite ? "Yes" : "No") << ","
            << (stat.isIgnored ? "Yes" : "No") << "," << vendorValue << ","
            << tpSellPrice << "," << tpBuyPrice << "\n";
    }

    auto currencies = GetCurrenciesCopy();
    for (auto& [id, stat] : currencies)
    {
        std::string name = stat.details.loaded ? stat.details.name : "Unknown";
        csv << "Currency," << id << ",\"" << name << "\"," << stat.count << ",0,N/A,"
            << (stat.isFavorite ? "Yes" : "No") << ","
            << (stat.isIgnored ? "Yes" : "No") << ",0,0,0\n";
    }

    return csv.str();
}

long long ItemTracker::CalcTotalCustomProfit()
{
    // Copy data first to avoid holding mutex while calling PassesFilter
    std::map<int, Stat> itemsCopy, currenciesCopy;
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        itemsCopy = s_Items;
        currenciesCopy = s_Currencies;
    }
    
    long long total = 0;
    
    for (const auto& [id, stat] : itemsCopy)
        if (PassesFilter(stat))
            total += GetStatProfit(stat);
    
    for (const auto& [id, stat] : currenciesCopy)
        if (PassesFilter(stat))
            total += GetStatProfit(stat);
    
    return total;
}

void ItemTracker::UpdateProfitHistory()
{
    auto now = std::chrono::system_clock::now();
    
    std::lock_guard<std::mutex> lock(s_ProfitHistoryMutex);
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - s_LastHistoryUpdate);
    
    if (duration.count() < HISTORY_UPDATE_INTERVAL_SECONDS)
        return; // Not time to update yet
    
    s_LastHistoryUpdate = now;
    
    // Copy data first to avoid deadlock with CalcTotalCustomProfit
    auto sessionDuration = GetSessionDuration();
    
    if (sessionDuration.count() < 1)
        return;
    
    // Calculate profit outside of lock to avoid deadlock
    long long totalProfit = CalcTotalCustomProfit();
    
    // Check Profit Goal Notification
    if (g_Settings.notifyProfitGoal && !s_ProfitGoalReached)
    {
        if (totalProfit >= static_cast<long long>(g_Settings.profitGoalAmount))
        {
            char msg[256];
            int gold = g_Settings.profitGoalAmount / 10000;
            snprintf(msg, sizeof(msg), Localization::GetText("profit_goal_reached_msg"), gold);
            UINotifications::AddGenericNotification(Localization::GetText("profit_goal_reached_title"), msg, "", "Legendary", true);
            s_ProfitGoalReached = true;
        }
    }

    double hours = static_cast<double>(sessionDuration.count()) / 3600.0;
    long long currentProfitPerHour = static_cast<long long>(totalProfit / hours);
    
    // Add new entry
    ProfitHistoryEntry entry;
    entry.timestamp = now;
    entry.profitPerHour = currentProfitPerHour;
    
    s_ProfitHistory.push_back(entry);

    // Keep only last MAX_HISTORY_ENTRIES entries
    while (s_ProfitHistory.size() > MAX_HISTORY_ENTRIES)
        s_ProfitHistory.pop_front();
}

long long ItemTracker::GetMovingAverageProfitPerHour()
{
    std::lock_guard<std::mutex> lock(s_ProfitHistoryMutex);
    if (s_ProfitHistory.empty())
        return 0;
    
    long long sum = 0;
    for (const auto& entry : s_ProfitHistory)
        sum += entry.profitPerHour;
    
    return sum / static_cast<long long>(s_ProfitHistory.size());
}

std::vector<std::pair<std::chrono::system_clock::time_point, long long>> ItemTracker::GetProfitHistory()
{
    std::lock_guard<std::mutex> lock(s_ProfitHistoryMutex);
    std::vector<std::pair<std::chrono::system_clock::time_point, long long>> result;
    for (const auto& entry : s_ProfitHistory)
    {
        result.push_back({entry.timestamp, entry.profitPerHour});
    }
    return result;
}

long long ItemTracker::TpSellProceedsPerUnitCopper(const ApiDetails& d)
{
    if (d.tpSellPrice <= 0) return 0;
    // Apply 15% TP fee (85/100)
    return static_cast<long long>(d.tpSellPrice * 85.0 / 100.0);
}

long long ItemTracker::TpBuyProceedsPerUnitCopper(const ApiDetails& d)
{
    if (d.tpBuyPrice <= 0) return 0;
    // TP Instant Sell also has 15% fee (5% listing + 10% exchange)
    return static_cast<long long>(d.tpBuyPrice * 85.0 / 100.0);
}

bool ItemTracker::CanSellOnTp(const ApiDetails& d)
{
    // Account-bound items cannot be sold on TP
    if (d.accountBound)
        return false;

    return TpSellProceedsPerUnitCopper(d) > 0 || TpBuyProceedsPerUnitCopper(d) > 0;
}

bool ItemTracker::CanSellToVendor(const ApiDetails& d)
{
    return d.vendorValue > 0 && !d.noSell;
}

long long ItemTracker::CalcTotalTpSellProfit()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    long long total = 0;
    for (auto& [id, stat] : s_Items)
    {
        if (!stat.details.loaded) continue;
        long long per = TpSellProceedsPerUnitCopper(stat.details);
        if (per > 0) total += stat.count * per;
    }
    return total;
}

long long ItemTracker::CalcTotalTpInstantProfit()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    long long total = 0;
    for (auto& [id, stat] : s_Items)
    {
        if (!stat.details.loaded) continue;
        long long per = TpBuyProceedsPerUnitCopper(stat.details);
        if (per > 0) total += stat.count * per;
    }
    return total;
}

long long ItemTracker::CalcTotalVendorProfit()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    long long total = 0;
    for (auto& [id, stat] : s_Items)
    {
        if (!stat.details.loaded) continue;
        if (CanSellToVendor(stat.details))
            total += stat.count * stat.details.vendorValue;
    }
    return total;
}

int ItemTracker::RarityRank(const std::string& rarity)
{
    static const char* order[] = {
        "Junk", "Basic", "Fine", "Masterwork", "Rare", "Exotic", "Ascended", "Legendary"
    };
    for (int i = 0; i < 8; ++i)
    {
        if (rarity == order[i]) return i;
    }
    return 0;
}

// === Persistence Functions ===
void ItemTracker::SaveData(const char* addonDir)
{
    if (!addonDir)
        return;

    std::string dataPath = std::string(addonDir) + "\\farming_data.json";
    
    nlohmann::json data;
    data["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
    data["sessionStart"] = std::chrono::system_clock::to_time_t(s_SessionStart);
    data["magicFind"] = s_MagicFind.load();

    // Save items
    nlohmann::json itemsArray = nlohmann::json::array();
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        for (const auto& [id, stat] : s_Items)
        {
            nlohmann::json item;
            item["apiId"] = id;
            item["count"] = stat.count;
            item["isFavorite"] = stat.isFavorite;
            item["lastMagicFind"] = stat.lastMagicFind;
            itemsArray.push_back(item);
        }
    }
    data["items"] = itemsArray;

    // Save currencies
    nlohmann::json currenciesArray = nlohmann::json::array();
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        for (const auto& [id, stat] : s_Currencies)
        {
            nlohmann::json currency;
            currency["apiId"] = id;
            currency["count"] = stat.count;
            currency["isFavorite"] = stat.isFavorite;
            currency["lastMagicFind"] = stat.lastMagicFind;
            currenciesArray.push_back(currency);
        }
    }
    data["currencies"] = currenciesArray;

    // Save session drops (for Timeline tab)
    nlohmann::json dropsArray = nlohmann::json::array();
    {
        std::lock_guard<std::mutex> lock(s_SessionDropsMutex);
        for (const auto& drop : s_SessionDrops)
        {
            nlohmann::json dropJson;
            dropJson["itemId"] = drop.itemId;
            dropJson["itemName"] = drop.itemName;
            dropJson["iconUrl"] = drop.iconUrl;
            dropJson["isCurrency"] = drop.isCurrency;
            dropJson["rarity"] = drop.rarity;
            dropJson["count"] = drop.count;
            dropJson["totalValue"] = drop.totalValue;
            dropJson["magicFind"] = drop.magicFind;
            dropJson["timestamp"] = drop.timestamp;
            dropsArray.push_back(dropJson);
        }
    }
    data["sessionDrops"] = dropsArray;

    // Write to file
    std::ofstream file(dataPath);
    if (file.is_open())
    {
        file << data.dump(4);
        file.close();
    }
}

void ItemTracker::LoadData(const char* addonDir)
{
    if (!addonDir)
        return;

    std::string dataPath = std::string(addonDir) + "\\farming_data.json";
    
    std::ifstream file(dataPath);
    if (!file.is_open())
        return;

    try
    {
        nlohmann::json data;
        file >> data;
        file.close();

        // Load session start if available
        if (data.contains("sessionStart"))
        {
            std::lock_guard<std::mutex> sessionLock(s_SessionStartMutex);
            time_t sessionStartT = data["sessionStart"].get<time_t>();
            s_SessionStart = std::chrono::system_clock::from_time_t(sessionStartT);
        }

        // Load Magic Find
        if (data.contains("magicFind"))
        {
            s_MagicFind.store(data["magicFind"].get<int>());
        }

        // Load items
        if (data.contains("items") && data["items"].is_array())
        {
            std::lock_guard<std::mutex> lock(s_Mutex);
            for (const auto& itemJson : data["items"])
            {
                int apiId = itemJson["apiId"].get<int>();
                long long count = itemJson["count"].get<long long>();
                bool isFavorite = itemJson.value("isFavorite", false);
                int lastMF = itemJson.value("lastMagicFind", -1);

                Stat s;
                s.apiId = apiId;
                s.type = StatType::Item;
                s.count = count;
                s.isFavorite = isFavorite;
                s.lastMagicFind = lastMF;
                s_Items[apiId] = s;
            }
        }

        // Load currencies
        if (data.contains("currencies") && data["currencies"].is_array())
        {
            std::lock_guard<std::mutex> lock(s_Mutex);
            for (const auto& currencyJson : data["currencies"])
            {
                int apiId = currencyJson["apiId"].get<int>();
                long long count = currencyJson["count"].get<long long>();
                bool isFavorite = currencyJson.value("isFavorite", false);
                int lastMF = currencyJson.value("lastMagicFind", -1);

                Stat s;
                s.apiId = apiId;
                s.type = StatType::Currency;
                s.count = count;
                s.isFavorite = isFavorite;
                s.lastMagicFind = lastMF;
                s_Currencies[apiId] = s;
            }
        }

        // Load session drops (for Timeline tab)
        if (data.contains("sessionDrops") && data["sessionDrops"].is_array())
        {
            std::lock_guard<std::mutex> lock(s_SessionDropsMutex);
            s_SessionDrops.clear();
            for (const auto& dropJson : data["sessionDrops"])
            {
                SessionHistory::DropEntry drop;
                drop.itemId = dropJson.value("itemId", 0);
                drop.itemName = dropJson.value("itemName", "");
                drop.iconUrl = dropJson.value("iconUrl", "");
                drop.isCurrency = dropJson.value("isCurrency", false);
                drop.rarity = dropJson.value("rarity", "");
                drop.count = dropJson.value("count", 0);
                drop.totalValue = dropJson.value("totalValue", 0LL);
                drop.magicFind = dropJson.value("magicFind", -1);
                drop.timestamp = dropJson.value("timestamp", "");
                s_SessionDrops.push_back(drop);
            }
        }
    }
    catch (...)
    {
        // If loading fails, just continue with empty data
    }
}

std::vector<int> ItemTracker::CollectPendingItemIds()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    std::set<int> ids;
    for (auto& [id, st] : s_Items)
    {
        if (st.count == 0) continue;
        // Only load data for items that are not loaded yet
        if (!st.details.loaded)
            ids.insert(id);
    }
    // Also load data for salvage kits that are tracked as currencies
    for (auto& [id, st] : s_Currencies)
    {
        if (st.count == 0) continue;
        if (s_SalvageKits.find(id) != s_SalvageKits.end() && !st.details.loaded)
            ids.insert(id);
    }
    return std::vector<int>(ids.begin(), ids.end());
}

bool ItemTracker::NeedCurrencyTable()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    for (auto& [id, st] : s_Currencies)
    {
        if (st.count == 0 || id == 1) continue;
        if (!st.details.loaded) return true;
    }
    return false;
}

static bool JsonHasNoSell(const nlohmann::json& item)
{
    if (!item.contains("flags") || !item["flags"].is_array()) return false;
    for (auto& f : item["flags"])
    {
        if (f.is_string())
        {
            std::string flag = f.get<std::string>();
            // AccountBound and SoulbindOnAcquire block TP sale only, not vendor sale
            if (flag == "NoSell")
                return true;
        }
    }
    return false;
}

static bool JsonHasAccountBound(const nlohmann::json& item)
{
    if (!item.contains("flags") || !item["flags"].is_array()) return false;
    for (auto& f : item["flags"])
    {
        if (f.is_string())
        {
            std::string flag = f.get<std::string>();
            if (flag == "AccountBound" || flag == "SoulbindOnAcquire")
                return true;
        }
    }
    return false;
}

static std::string BuildIconUrl(const std::string& iconField)
{
    if (iconField.empty()) return {};
    if (iconField.find("http://") == 0 || iconField.find("https://") == 0)
        return iconField;
    if (iconField[0] == '/')
        return "https://render.guildwars2.com" + iconField;
    return "https://render.guildwars2.com/" + iconField;
}

void ItemTracker::ApplyItemsFromApi(const json& itemsArray, const json& pricesArray)
{
    if (!itemsArray.is_array() || !pricesArray.is_array()) return;

    std::lock_guard<std::mutex> lock(s_Mutex);

    for (auto& item : itemsArray)
    {
        if (!item.contains("id")) continue;
        int id = item["id"].get<int>();

        // Check if this is an item or a salvage kit tracked as currency
        auto it = s_Items.find(id);
        Stat* st = nullptr;
        if (it != s_Items.end())
        {
            st = &it->second;
        }
        else if (s_SalvageKits.find(id) != s_SalvageKits.end())
        {
            // Salvage kit tracked as currency
            auto curIt = s_Currencies.find(id);
            if (curIt != s_Currencies.end())
                st = &curIt->second;
        }

        if (!st) continue;

        st->details.name        = item.value("name", "");
        st->details.description = item.value("description", "");
        st->details.vendorValue = item.value("vendor_value", 0);
        st->details.rarity      = item.value("rarity", std::string());
        st->details.noSell      = JsonHasNoSell(item);
        st->details.accountBound = JsonHasAccountBound(item);
        if (item.contains("icon") && item["icon"].is_string())
            st->details.iconUrl = BuildIconUrl(item["icon"].get<std::string>());
        st->details.loaded      = true;
        st->details.knownByApi  = true;

        // Set item type from API data
        if (item.contains("type") && item["type"].is_string())
        {
            std::string t = item["type"].get<std::string>();
            if (t == "Armor") st->details.itemType = ItemType::Armor;
            else if (t == "Weapon") st->details.itemType = ItemType::Weapon;
            else if (t == "Trinket") st->details.itemType = ItemType::Trinket;
            else if (t == "Gizmo") st->details.itemType = ItemType::Gizmo;
            else if (t == "CraftingMaterial") st->details.itemType = ItemType::CraftingMaterial;
            else if (t == "Consumable") st->details.itemType = ItemType::Consumable;
            else if (t == "GatheringTool") st->details.itemType = ItemType::GatheringTool;
            else if (t == "Bag") st->details.itemType = ItemType::Bag;
            else if (t == "Container") st->details.itemType = ItemType::Container;
            else if (t == "MiniPet") st->details.itemType = ItemType::MiniPet;
            else if (t == "GizmoContainer") st->details.itemType = ItemType::GizmoContainer;
            else if (t == "Backpack") st->details.itemType = ItemType::Backpack;
            else if (t == "UpgradeComponent") st->details.itemType = ItemType::UpgradeComponent;
            else if (t == "Tool") st->details.itemType = ItemType::Tool;
            else if (t == "Trophy") st->details.itemType = ItemType::Trophy;
            else if (t == "Unlock") st->details.itemType = ItemType::Unlock;
        }

        // Trigger notification now that details are loaded
        CheckAndTriggerNotification(id, *st);
    }

    for (auto& pr : pricesArray)
    {
        if (!pr.contains("id")) continue;
        int id = pr["id"].get<int>();

        // Check if this is an item or a salvage kit tracked as currency
        auto it = s_Items.find(id);
        Stat* st = nullptr;
        if (it != s_Items.end())
        {
            st = &it->second;
        }
        else if (s_SalvageKits.find(id) != s_SalvageKits.end())
        {
            // Salvage kit tracked as currency
            auto curIt = s_Currencies.find(id);
            if (curIt != s_Currencies.end())
                st = &curIt->second;
        }

        if (!st)
        {
            // Item not found in tracker - might be a price for an item we don't have
            continue;
        }

        bool hasSellPrice = false;
        bool hasBuyPrice = false;

        if (pr.contains("sells") && pr["sells"].contains("unit_price"))
        {
            st->details.tpSellPrice = pr["sells"]["unit_price"].get<int>();
            hasSellPrice = true;
        }

        if (pr.contains("buys") && pr["buys"].contains("unit_price"))
        {
            st->details.tpBuyPrice = pr["buys"]["unit_price"].get<int>();
            hasBuyPrice = true;
        }

        // Log if item has no prices but is in inventory
        if (st->count > 0 && !hasSellPrice && !hasBuyPrice)
        {
            static int loggedCount = 0;
            if (loggedCount < 5)
            {
                Gw2Api::Log("Item " + std::to_string(id) + " (" + st->details.name + ") has no TP prices (no buy/sell orders)", "warning");
                loggedCount++;
            }
        }

        // Re-check notification with updated prices
        CheckAndTriggerNotification(id, *st);
    }

    // Mark items that were not returned by the API as still loaded (don't reset them)
    // This preserves details for items that are no longer in inventory (salvaged/destroyed)
}

void ItemTracker::ClearItemDetails()
{
    std::lock_guard<std::mutex> lock(s_Mutex);

    // Clear all item details
    for (auto& [id, st] : s_Items)
    {
        st.details.loaded = false;
        st.details.name.clear();
        st.details.description.clear();
        st.details.iconUrl.clear();
        st.details.vendorValue = 0;
        st.details.tpBuyPrice = 0;
        st.details.tpSellPrice = 0;
        st.details.noSell = false;
        st.details.accountBound = false;
        st.details.rarity.clear();
        st.details.itemType = ItemType::Unknown;
        st.details.knownByApi = false;
    }

    // Clear all currency details
    for (auto& [id, st] : s_Currencies)
    {
        st.details.loaded = false;
        st.details.name.clear();
        st.details.description.clear();
        st.details.iconUrl.clear();
        st.details.vendorValue = 0;
        st.details.tpBuyPrice = 0;
        st.details.tpSellPrice = 0;
        st.details.noSell = false;
        st.details.accountBound = false;
        st.details.rarity.clear();
        st.details.itemType = ItemType::Unknown;
        st.details.knownByApi = false;
    }
}

void ItemTracker::ForceReloadAll()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    for (auto& [id, st] : s_Items)
    {
        if (st.count != 0)
            st.details.loaded = false;
    }
    for (auto& [id, st] : s_Currencies)
    {
        if (st.count != 0)
            st.details.loaded = false;
    }
}

void ItemTracker::ApplyCurrencyTable(const json& currenciesArray)
{
    if (!currenciesArray.is_array()) return;

    std::lock_guard<std::mutex> lock(s_Mutex);

    for (auto& c : currenciesArray)
    {
        if (!c.contains("id")) continue;
        int id = c["id"].get<int>();
        auto it = s_Currencies.find(id);
        if (it == s_Currencies.end()) continue;

        it->second.details.name = c.value("name", "");
        it->second.details.description = c.value("description", "");
        if (c.contains("icon") && c["icon"].is_string())
            it->second.details.iconUrl = BuildIconUrl(c["icon"].get<std::string>());
        it->second.details.loaded = true;
    }
}

ItemTracker::CoinSplit ItemTracker::SplitCoin(long long copperValue)
{
    CoinSplit result{};
    result.negative = copperValue < 0;
    long long abs_val = std::abs(copperValue);
    result.gold   = static_cast<int>(abs_val / 10000);
    result.silver = static_cast<int>((abs_val % 10000) / 100);
    result.copper = static_cast<int>(abs_val % 100);
    return result;
}

std::pair<int, Stat> ItemTracker::GetBestDrop()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    
    std::pair<int, Stat> bestDrop = {0, Stat()};
    long long maxProfit = 0;
    
    for (const auto& [id, stat] : s_Items)
    {
        if (stat.count == 0) continue;
        if (!PassesFilter(stat)) continue;
        
        long long profit = GetStatProfit(stat);
        if (profit > maxProfit)
        {
            maxProfit = profit;
            bestDrop = {id, stat};
        }
    }
    
    return bestDrop;
}
