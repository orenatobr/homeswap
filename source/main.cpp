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
#include <vpad/input.h>
#include <wups.h>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/config_api.h>

#include "logic.h"

static_assert(GX2_SCAN_TARGET_TV == kScanTV && GX2_SCAN_TARGET_DRC == kScanDRC, "scan target values changed");
static_assert(VPAD_BUTTON_X == kButtonX, "X button value changed");

WUPS_PLUGIN_NAME("HomeSwap");
WUPS_PLUGIN_DESCRIPTION("Home on the TV, Mii plaza on the GamePad (Wii U Menu only)");
WUPS_PLUGIN_VERSION("v0.4.2");
WUPS_PLUGIN_AUTHOR("orenatobr");
WUPS_PLUGIN_LICENSE("GPLv3");

WUPS_USE_STORAGE("HomeSwap");

static bool sEnabled    = true;
static bool sConfigOpen = false; // keep the Aroma config menu on its usual screens
static bool sInHome     = false;
static XTracker sX; // X in the Home cycles the three screen layouts; not saved, so every boot starts in A
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
    sX      = XTracker{}; // the menu always opens in its normal layout, so the cycle restarts in A
}

// What the plugin sees from the GamePad, saved to its config file (HomeSwap.json) when the first
// sample arrives and on every X press, so a misbehaving console can be checked over FTP.
static int32_t sSeenCalls = 0;
static int32_t sSeenPresses = 0;

static void saveSeen()
{
    WUPSStorageAPI::Store("seen_calls", sSeenCalls);
    WUPSStorageAPI::Store("seen_x_presses", sSeenPresses);
    WUPSStorageAPI::Store("seen_x_state", (int32_t) sX.state);
    WUPSStorageAPI::SaveStorage();
}

DECL_FUNCTION(int32_t, VPADRead, VPADChan chan, VPADStatus *buffers, uint32_t count, VPADReadError *outError)
{
    VPADReadError error = VPAD_READ_NO_SAMPLES;
    const int32_t result = real_VPADRead(chan, buffers, count, &error);
    if (outError != nullptr) {
        *outError = error;
    }

    if (chan == VPAD_CHAN_0 && error == VPAD_READ_SUCCESS && result > 0) {
        bool changed = false;
        if (sInHome && sSeenCalls++ == 0) {
            changed = true;
        }
        for (int32_t i = 0; i < result; i++) {
            if (sInHome && (buffers[i].trigger & kButtonX)) {
                sSeenPresses++;
                changed = true;
            }
            trackX(sX, sInHome, sConfigOpen, buffers[i].hold, buffers[i].trigger, buffers[i].release);
        }
        if (changed) {
            saveSeen();
        }
    }
    return result;
}

// Scratch surface for the AA resolve, kept between frames: allocating and freeing a few MB
// of mapped memory on every frame fragments the heap and shows up as occasional stutter.
// Only the GX2 render thread touches it. The GPU runs commands in order, so overwriting it
// on the next frame cannot race with the previous copy.
static void *sScratch          = nullptr;
static uint32_t sScratchSize   = 0;
static uint32_t sScratchAlign  = 0;

static void freeScratch()
{
    if (sScratch != nullptr) {
        MEMFreeToMappedMemory(sScratch);
        sScratch      = nullptr;
        sScratchSize  = 0;
        sScratchAlign = 0;
    }
}

static void *scratchFor(uint32_t size, uint32_t align)
{
    if (!canReuseScratch(sScratchSize, sScratchAlign, size, align)) {
        freeScratch();
        sScratch = MEMAllocFromMappedMemoryForGX2Ex(size, align);
        if (sScratch != nullptr) {
            sScratchSize  = size;
            sScratchAlign = align;
        }
    }
    return sScratch;
}

ON_APPLICATION_ENDS()
{
    freeScratch();
}

DECL_FUNCTION(void, GX2CopyColorBufferToScanBuffer, GX2ColorBuffer *colorBuffer, GX2ScanTarget scan_target)
{
    const bool swap = shouldSwap(sEnabled, sConfigOpen, sInHome, sX.state);
    scan_target     = (GX2ScanTarget) swappedTarget(swap, (uint32_t) scan_target);

    if (swap && colorBuffer->surface.aa != GX2_AA_MODE1X) {
        // An anti-aliased buffer has to be resolved before it can go to the other screen.
        GX2Surface tempSurface = colorBuffer->surface;
        tempSurface.aa         = GX2_AA_MODE1X;
        GX2CalcSurfaceSizeAndAlignment(&tempSurface);

        tempSurface.image = scratchFor(tempSurface.imageSize, tempSurface.alignment);
        if (tempSurface.image != nullptr) {
            GX2ResolveAAColorBuffer(colorBuffer, &tempSurface, 0, 0);

            GX2Surface surfaceCpy = colorBuffer->surface;
            colorBuffer->surface  = tempSurface;
            real_GX2CopyColorBufferToScanBuffer(colorBuffer, scan_target);
            colorBuffer->surface = surfaceCpy;
            return;
        }
        OSReport("HomeSwap: failed to allocate %d bytes to resolve AA\n", tempSurface.imageSize);
    }

    real_GX2CopyColorBufferToScanBuffer(colorBuffer, scan_target);
}

WUPS_MUST_REPLACE(VPADRead, WUPS_LOADER_LIBRARY_VPAD, VPADRead);
WUPS_MUST_REPLACE(GX2CopyColorBufferToScanBuffer, WUPS_LOADER_LIBRARY_GX2, GX2CopyColorBufferToScanBuffer);
