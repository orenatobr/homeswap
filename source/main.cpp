// HomeSwap: shows the Wii U Menu (Home) on the TV and the Mii plaza on the GamePad.
// The swap only runs inside the Wii U Menu; games and other apps are untouched.
// The screen swap and the anti-aliasing workaround follow SwipSwapMe by Maschell (GPLv3);
// the "only for one title" filter follows Better Settings by Fangal (GPLv3).
#include <coreinit/debug.h>
#include <coreinit/title.h>
#include <gx2/enum.h>
#include <gx2/surface.h>
#include <memory/mappedmemory.h>
#include <sysapp/title.h>
#include <wups.h>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/config_api.h>

#include "logic.h"

static_assert(GX2_SCAN_TARGET_TV == kScanTV && GX2_SCAN_TARGET_DRC == kScanDRC, "scan target values changed");

WUPS_PLUGIN_NAME("HomeSwap");
WUPS_PLUGIN_DESCRIPTION("Home on the TV, Mii plaza on the GamePad (Wii U Menu only)");
WUPS_PLUGIN_VERSION("v0.1.0");
WUPS_PLUGIN_AUTHOR("orenatobr");
WUPS_PLUGIN_LICENSE("GPLv3");

WUPS_USE_STORAGE("HomeSwap");

static bool sEnabled    = true;
static bool sConfigOpen = false; // keep the Aroma config menu on its usual screens
static bool sInHome     = false;
static uint64_t sMenuTitleId = 0;

static void enabledChanged(ConfigItemBoolean *, bool value)
{
    sEnabled = value;
    WUPSStorageAPI::Store("enabled", value);
}

static WUPSConfigAPICallbackStatus configOpened(WUPSConfigCategoryHandle rootHandle)
{
    sConfigOpen = true;
    WUPSConfigCategory root(rootHandle);
    try {
        root.add(WUPSConfigItemBoolean::Create("enabled", "Home on the TV (Wii U Menu only)", true, sEnabled, enabledChanged));
    } catch (...) {
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }
    return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
}

static void configClosed()
{
    WUPSStorageAPI::SaveStorage();
    sConfigOpen = false;
}

INITIALIZE_PLUGIN()
{
    sMenuTitleId = _SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_WII_U_MENU);

    WUPSConfigAPIOptionsV1 opts = {.name = "HomeSwap"};
    if (WUPSConfigAPI_Init(opts, configOpened, configClosed) != WUPSCONFIG_API_RESULT_SUCCESS) {
        OSReport("HomeSwap: failed to init config api\n");
    }
    WUPSStorageAPI::GetOrStoreDefault("enabled", sEnabled, true);
    WUPSStorageAPI::SaveStorage();
}

ON_APPLICATION_START()
{
    sInHome = OSGetTitleID() == sMenuTitleId;
}

DECL_FUNCTION(void, GX2CopyColorBufferToScanBuffer, GX2ColorBuffer *colorBuffer, GX2ScanTarget scan_target)
{
    const bool swap = shouldSwap(sEnabled, sConfigOpen, sInHome);
    scan_target     = (GX2ScanTarget) swappedTarget(swap, (uint32_t) scan_target);

    if (swap && colorBuffer->surface.aa != GX2_AA_MODE1X) {
        // An anti-aliased buffer has to be resolved before it can go to the other screen.
        GX2Surface tempSurface = colorBuffer->surface;
        tempSurface.aa         = GX2_AA_MODE1X;
        GX2CalcSurfaceSizeAndAlignment(&tempSurface);

        tempSurface.image = MEMAllocFromMappedMemoryForGX2Ex(tempSurface.imageSize, tempSurface.alignment);
        if (tempSurface.image != nullptr) {
            GX2ResolveAAColorBuffer(colorBuffer, &tempSurface, 0, 0);

            GX2Surface surfaceCpy = colorBuffer->surface;
            colorBuffer->surface  = tempSurface;
            real_GX2CopyColorBufferToScanBuffer(colorBuffer, scan_target);
            colorBuffer->surface = surfaceCpy;

            MEMFreeToMappedMemory(tempSurface.image);
            return;
        }
        OSReport("HomeSwap: failed to allocate %d bytes to resolve AA\n", tempSurface.imageSize);
    }

    real_GX2CopyColorBufferToScanBuffer(colorBuffer, scan_target);
}

WUPS_MUST_REPLACE(GX2CopyColorBufferToScanBuffer, WUPS_LOADER_LIBRARY_GX2, GX2CopyColorBufferToScanBuffer);
