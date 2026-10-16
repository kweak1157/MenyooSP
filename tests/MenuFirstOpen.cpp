#include <Windows.h>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <iostream>

#include "Menu/Menu.h"
#include "Util/keyboard.h"

// Use real menu transitions and keyboard events with deterministic game inputs.
static std::uint64_t tick = 1000;
static bool controllerPressed = false;
static bool controllerTapped = false;
static bool pauseMenuActive = false;
static bool backTapped = false;
static int firstOpenCount = 0;
std::uint64_t GetTickMs() { return tick; }
int GET_GAME_TIMER() { return static_cast<int>(tick); }
bool IS_DISABLED_CONTROL_PRESSED(int, int) { return controllerPressed; }
bool IS_DISABLED_CONTROL_JUST_PRESSED(int, int) { return controllerTapped; }
bool IS_PAUSE_MENU_ACTIVE() { return pauseMenuActive; }
bool IS_GAMEPLAY_HINT_ACTIVE() { return false; }
void STOP_GAMEPLAY_HINT(bool) {}
void DISPLAY_AMMO_THIS_FRAME(int) {}
void DISPLAY_CASH(int) {}
void SET_RADAR_ZOOM(int) {}
bool IS_MOBILE_PHONE_CALL_ONGOING() { return false; }
void STOP_SCRIPTED_CONVERSATION(int) {}
bool HAS_THIS_ADDITIONAL_TEXT_LOADED(const char*, int) { return true; }
void REQUEST_ADDITIONAL_TEXT(const char*, int) {}
void ENABLE_ALL_CONTROL_ACTIONS(int) {}
namespace Game::Sound {
    void PlayFrontend(const char*, const char*) {}
    void PlayFrontend_default(const char*) {}
}
bool MenuPressTimer::IsButtonHeldOrTapped(const Button&) { return false; }
bool MenuPressTimer::IsButtonTapped(const Button& button) { return backTapped && button == Button::Back; }
void Menu::DisableControls() {}
void Menu::set_opened_IB() {}
std::function<void()> Menu::OnSubBack = nullptr;
void Menu::justopened() { ++firstOpenCount; g_menuNotOpenedYet = false; }
void Menu::submenu_switch() { printingop = 10; }
namespace sub::Spooner::SpoonerMode {
    extern bool bEnabled;
    void TurnOff() { bEnabled = false; }
}

// Generated from Menu.cpp by run-menu-first-open.ps1. This keeps the test on
// the production startup values and transitions without linking the GTA runtime.
#include "MenuFirstOpenProduction.inl"

static void Check(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

static void Frame()
{
    Menu::sub_handler();
    ++tick;
    controllerTapped = false;
    backTapped = false;
}

static void TapKey(DWORD key)
{
    OnKeyboardMessage(key, 1, 0, FALSE, FALSE, FALSE, FALSE);
    OnKeyboardMessage(key, 1, 0, FALSE, FALSE, TRUE, TRUE);
    Frame();
}

int main()
{
    Frame();
    Check(Menu::currentsub == SUB::CLOSED, "menu starts closed");
    TapKey(VirtualKey::F8);
    Check(Menu::currentsub == SUB::MAINMENU, "first F8 opens main menu");
    for (int i = 0; i < 120; ++i) {
        Frame();
        Check(Menu::currentsub == SUB::MAINMENU,
            "menu must stay open after the first F8 (no second press required)");
    }
    Check(firstOpenCount == 1, "first-open setup runs once");
    Check(Menu::currentsub_ar_index == 0, "first open does not push a submenu");
    TapKey(VirtualKey::F8);
    Check(Menu::currentsub == SUB::CLOSED, "second F8 closes menu");
    TapKey(VirtualKey::F8);
    Frame();
    Check(Menu::currentsub == SUB::MAINMENU, "third F8 reopens menu");
    Check(firstOpenCount == 1, "reopen does not repeat first-open setup");

    Menu::SetSub_delayed = SUB::SETTINGS;
    Frame();
    Check(Menu::currentsub == SUB::SETTINGS, "queued submenu still opens");
    Check(Menu::SetSub_delayed == -1, "queued submenu is consumed once");
    TapKey(VirtualKey::F8);
    TapKey(VirtualKey::F8);
    Frame();
    Check(Menu::currentsub == SUB::SETTINGS, "reopen restores last submenu");

    // Check a cold controller open, using the production initial pending state.
    Menu::SetSub_closed();
    Menu::LOOCsub = SUB::MAINMENU;
    Menu::SetSub_delayed = initialPendingSubmenu;
    Menu::currentsub_ar_index = 0;
    g_menuNotOpenedYet = true;
    Menu::bit_controller = true;
    controllerPressed = controllerTapped = true;
    Frame();
    Frame();
    Check(Menu::currentsub == SUB::MAINMENU, "first controller press stays open");

    Menu::bit_controller = false;
    menubinds = VirtualKey::F7;
    TapKey(VirtualKey::F7);
    Check(Menu::currentsub == SUB::CLOSED, "custom binding closes menu");
    TapKey(VirtualKey::F7);
    Frame();
    Check(Menu::currentsub == SUB::MAINMENU, "custom binding reopens menu");

    menubinds = VirtualKey::F8;
    for (int i = 0; i < 1000; ++i) {
        TapKey(VirtualKey::F8);
        Check(Menu::currentsub == (i % 2 == 0 ? SUB::CLOSED : SUB::MAINMENU),
            "rapid F8 presses must keep toggling the menu");
        Check(Menu::LOOCsub == SUB::MAINMENU, "rapid toggles preserve the reopen page");
    }

    // A close request and F8 can arrive in the same frame. Later presses must
    // still reopen the menu, without restarting the script or the game.
    pauseMenuActive = true;
    TapKey(VirtualKey::F8);
    pauseMenuActive = false;
    Check(Menu::currentsub == SUB::CLOSED, "pause and F8 close the menu");
    tick += 200;
    TapKey(VirtualKey::F8);
    Check(Menu::currentsub == SUB::MAINMENU, "F8 reopens after simultaneous close requests");

    backTapped = true;
    TapKey(VirtualKey::F8);
    Check(Menu::currentsub == SUB::CLOSED, "back and F8 close the menu");
    TapKey(VirtualKey::F8);
    Check(Menu::currentsub == SUB::MAINMENU, "F8 reopens after back and F8 in one frame");

    Menu::SetSub_new(SUB::SETTINGS);
    const auto savedDepth = Menu::currentsub_ar_index;
    const auto savedSelection = Menu::currentop;
    Menu::SetSub_closed();
    for (int i = 0; i < 1000; ++i) Menu::SetSub_closed();
    TapKey(VirtualKey::F8);
    Check(Menu::currentsub == SUB::SETTINGS, "repeated closes preserve the last submenu");
    Check(Menu::currentsub_ar_index == savedDepth, "repeated closes preserve submenu history");
    Check(Menu::currentop == savedSelection, "repeated closes preserve selection");

    // Recover even if an earlier close already lost the saved reopen page.
    Menu::SetSub_closed();
    Menu::LOOCsub = SUB::CLOSED;
    TapKey(VirtualKey::F8);
    Check(Menu::currentsub == SUB::MAINMENU, "F8 recovers a closed reopen page");
    Check(Menu::currentsub_ar_index == 0, "recovery starts at the main menu root");
    Check(Menu::currentop == 1 && *Menu::currentopATM == 1, "recovery selects the first option");
    Check(firstOpenCount == 2, "rapid toggles and recovery do not repeat first-open setup");

    namespace Spooner = sub::Spooner::SpoonerMode;
    const auto TapSpoonerKey = [](DWORD key) {
        OnKeyboardMessage(key, 1, 0, FALSE, FALSE, FALSE, FALSE);
        OnKeyboardMessage(key, 1, 0, FALSE, FALSE, TRUE, TRUE);
        if (Spooner::IsHotkeyPressed()) Spooner::Toggle();
        Frame();
    };
    Menu::SetSub_closed();
    g_menuNotOpenedYet = true;
    TapSpoonerKey(VirtualKey::F9);
    Check(Spooner::bEnabled, "first F9 enables Spooner without opening F8 first");
    Check(firstOpenCount == 3, "first F9 runs menu setup once");
    Check(Menu::currentsub == SUB::SPOONER_MAIN, "first F9 opens Object Spooner");
    Check(Menu::currentop == 2 && *Menu::currentopATM == 2 && Menu::currentop_w_breaks == 2,
        "Spooner opens with Spawn Entity Into World selected");
    for (int i = 0; i < 120; ++i) Frame();
    Check(Menu::currentsub == SUB::SPOONER_MAIN, "Object Spooner stays open");
    Check(Menu::currentsub_ar_index == 1, "Spooner has one parent menu");
    Menu::SetSub_previous();
    Check(Menu::currentsub == SUB::MAINMENU && Menu::currentsub_ar_index == 0,
        "back from Spooner returns to the Menyoo main menu");
    TapSpoonerKey(VirtualKey::F9);
    Check(!Spooner::bEnabled, "F9 still turns Spooner off");
    Menu::SetSub_new(SUB::SETTINGS);
    Menu::SetSub_delayed = SUB::SETTINGS;
    TapSpoonerKey(VirtualKey::F9);
    Check(Spooner::bEnabled && Menu::currentsub == SUB::SPOONER_MAIN,
        "F9 starts at Object Spooner from another submenu");
    Check(Menu::SetSub_delayed == -1 && Menu::currentsub_ar_index == 1,
        "Spooner discards the previous pending page and navigation history");
    Check(firstOpenCount == 3, "later F9 presses do not repeat setup");
    Menu::SetSub_closed();
    Spooner::TurnOff();
    Spooner::bindsKeyboard = VirtualKey::F6;
    TapSpoonerKey(VirtualKey::F6);
    Check(Spooner::bEnabled && Menu::currentsub == SUB::SPOONER_MAIN,
        "custom Spooner binding also opens Object Spooner");
    Menu::SetSub_closed();
    Spooner::TurnOff();
    Menu::bit_controller = true;
    controllerPressed = controllerTapped = true;
    if (Spooner::IsHotkeyPressed()) Spooner::Toggle();
    controllerTapped = false;
    Frame();
    Check(Spooner::bEnabled && Menu::currentsub == SUB::SPOONER_MAIN,
        "controller Spooner binding also opens Object Spooner");
    std::cout << "Menu toggles, recovery, navigation, and Spooner first-open checks passed.\n";
}
