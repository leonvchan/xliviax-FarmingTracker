#pragma once

#include "../include/nexus/Nexus.h"

// Global pointer to the Nexus API - set in AddonLoad, cleared in AddonUnload
extern AddonAPI_t* APIDefs;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
HMODULE GetModule();
