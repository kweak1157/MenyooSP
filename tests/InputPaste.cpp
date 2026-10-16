#include <Windows.h>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

// Compile the real shared input function. Only game and OS inputs are replayed.
static int frame, status, displays, clipboardReads;
static bool shiftPaste, cancelInput, foreground = true;
static std::string clipboard, field;
static std::vector<int> presses;
static std::wstring clipboardWide;
static int clipboardFailure, closes, unlocks;
bool get_key_pressed(DWORD key)
{
    const bool pressed = std::find(presses.begin(), presses.end(), frame) != presses.end();
    return pressed && (shiftPaste ? (key == VK_SHIFT || key == VK_INSERT) : (key == VK_CONTROL || key == 'V'));
}
BOOL TestOpenClipboard(HWND) { ++clipboardReads; return clipboardFailure != 1; }
BOOL TestCloseClipboard() { ++closes; return TRUE; }
HANDLE TestClipboardData(UINT format)
{
    if (clipboardFailure == 2 || format != CF_UNICODETEXT) return nullptr;
    int length = MultiByteToWideChar(CP_UTF8, 0, clipboard.data(), static_cast<int>(clipboard.size()), nullptr, 0);
    clipboardWide.resize(length);
    if (length) MultiByteToWideChar(CP_UTF8, 0, clipboard.data(), static_cast<int>(clipboard.size()), clipboardWide.data(), length);
    return reinterpret_cast<HANDLE>(1);
}
void* TestGlobalLock(HANDLE) { return clipboardFailure == 3 ? nullptr : clipboardWide.data(); }
BOOL TestGlobalUnlock(HANDLE) { ++unlocks; return TRUE; }
SIZE_T TestGlobalSize(HANDLE) { return (clipboardWide.size() + (clipboardFailure == 4 ? 0 : 1)) * sizeof(wchar_t); }
#define OpenClipboard TestOpenClipboard
#define CloseClipboard TestCloseClipboard
#define GetClipboardData TestClipboardData
#define GlobalLock TestGlobalLock
#define GlobalUnlock TestGlobalUnlock
#define GlobalSize TestGlobalSize
HWND TestForegroundWindow() { return reinterpret_cast<HWND>(1); }
DWORD TestWindowProcess(HWND, DWORD* process) { *process = foreground ? 1 : 2; return 1; }
DWORD TestCurrentProcess() { return 1; }
#define GetForegroundWindow TestForegroundWindow
#define GetWindowThreadProcessId TestWindowProcess
#define GetCurrentProcessId TestCurrentProcess
void DISPLAY_ONSCREEN_KEYBOARD(bool, const char*, const char*, const char* text, const char*, const char*, const char*, int)
{
    field = text; status = 0; ++displays;
}
int UPDATE_ONSCREEN_KEYBOARD() { return status; }
void CANCEL_ONSCREEN_KEYBOARD() { status = 2; }
const char* GET_ONSCREEN_KEYBOARD_RESULT() { return status == 1 ? field.c_str() : nullptr; }
void WAIT(int)
{
    if (++frame >= 8) status = cancelInput ? 2 : 1;
    if (frame > 20) { std::cerr << "FAIL: input loop did not finish\n"; std::exit(1); }
}
void SET_TEXT_FONT(int) {}
void SET_TEXT_SCALE(float, float) {}
void SET_TEXT_COLOUR(int, int, int, int) {}
void SET_TEXT_WRAP(float, float) {}
void SET_TEXT_RIGHT_JUSTIFY(BOOL) {}
void SET_TEXT_CENTRE(BOOL) {}
void SET_TEXT_DROPSHADOW(int, int, int, int, int) {}
void SET_TEXT_EDGE(int, int, int, int, int) {}
void SET_TEXT_OUTLINE() {}
bool DOES_TEXT_LABEL_EXIST(const char*) { return false; }
void BEGIN_TEXT_COMMAND_DISPLAY_TEXT(const char*) {}
void ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(const char*) {}
void add_text_component_long_string(const std::string&) {}
void END_TEXT_COMMAND_DISPLAY_TEXT(float, float, int) {}
namespace Language { std::string TranslateToSelected(const std::string& text) { return text; } }
#include "InputPasteProduction.inl"

void Check(bool condition, const char* message)
{
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
void Reset()
{
    frame = status = displays = clipboardReads = 0;
    clipboardFailure = closes = unlocks = 0;
    shiftPaste = cancelInput = false; foreground = true;
    clipboard = "adder"; presses = {1, 2, 3};
}
int main()
{
    Reset();
    Check(Game::InputBox("old", 126, "SEARCH", "old") == "adder", "Ctrl+V must put copied text into the search field");
    Check(clipboardReads == 1 && displays == 2, "holding paste must insert only once");
    Reset(); shiftPaste = true;
    Check(Game::InputBox("", 126, "SEARCH", "") == "adder", "Shift+Insert must paste");
    Reset(); clipboard = "";
    Check(Game::InputBox("old", 126, "SEARCH", "old") == "old" && displays == 1, "empty clipboard must keep current input");
    Reset(); cancelInput = true;
    Check(Game::InputBox("old", 126, "SEARCH", "old") == "old", "Escape after paste must restore the previous search");
    Reset(); clipboard = "123456789";
    Check(Game::InputBox("", 4, "SEARCH", "") == "1234", "paste must respect the input limit");
    Reset(); clipboard = "adder\r\nsecond line";
    Check(Game::InputBox("", 126, "SEARCH", "") == "adder", "single-line search must use the first copied line");
    Reset(); presses = {};
    Check(Game::InputBox("", 126, "SEARCH", "typed") == "typed" && displays == 1, "ordinary input must still work");
    Reset(); foreground = false;
    Check(Game::InputBox("", 126, "SEARCH", "old") == "old" && clipboardReads == 0, "paste outside the game must be ignored");
    Reset(); presses = {1, 2, 4, 5};
    Check(Game::InputBox("", 126, "SEARCH", "") == "adder" && clipboardReads == 2, "a second paste press must work");
    Reset(); clipboard = "caf\xc3\xa9";
    Check(Game::InputBox("", 126, "SEARCH", "") == clipboard, "Unicode clipboard text must survive as UTF-8");
    Reset(); clipboard = "ab\xc3\xa9";
    Check(Game::InputBox("", 3, "SEARCH", "") == "ab", "length limit must not split a UTF-8 character");
    for (int failure = 1; failure <= 4; ++failure) {
        Reset(); clipboardFailure = failure;
        Check(Game::InputBox("old", 126, "SEARCH", "old") == "old", "clipboard failures must preserve input");
        Check(closes == (failure == 1 ? 0 : 1), "all opened clipboards must close, including failure paths");
        Check(unlocks == (failure == 4 ? 1 : 0), "all locked data must unlock, including malformed text");
    }
    Reset();
    Check(GetClipboardTextUtf8() == "adder" && closes == 1 && unlocks == 1, "successful clipboard read must release both resources");
    std::cout << "PASS: search paste, held keys, repeat paste, Shift+Insert, UTF-8, limits, clipboard failures, resource cleanup, cancellation, focus and ordinary input\n";
}
