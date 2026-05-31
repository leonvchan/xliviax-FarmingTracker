#pragma once

namespace UILayout
{
    // Load imgui_layout.ini from addonDir. Call after ImGui context is set.
    void Init(const char* addonDir);

    // Save Farming Tracker table/window layout to addonDir/imgui_layout.ini.
    void Save();

    // Call each frame; saves layout ~5s after the last column/window resize.
    void Tick();

    // Delete saved layout file (e.g. reset table column widths).
    void Clear();
}
