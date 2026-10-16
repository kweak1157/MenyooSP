#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <future>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "Submenus/PedSelectorState.h"

using Hash = unsigned long;
static std::uint64_t now = 1000;
static int blockingLoads = 0, changes = 0, options = 0, worldSpawns = 0;
struct Model
{
	Hash hash = 0;
	Model() = default;
	Model(Hash value) : hash(value) {}
	Model(const std::string& name) : hash(static_cast<Hash>(std::hash<std::string>{}(name))) {}
	static inline std::unordered_set<Hash> loaded;
	bool IsInCdImage() const { return hash != 0; }
	bool IsPed() const { return hash != 0; }
	bool Load(unsigned long) const { ++blockingLoads; return true; }
	void Load() const {}
	bool IsLoaded() const { return loaded.count(hash) != 0; }
	void Unload() const {}
};
struct Vector2 { float x, y; Vector2(float a, float b) : x(a), y(b) {} };
struct RGBA { int R = 0, G = 0, B = 0, A = 255; static RGBA AllWhite() { return {}; } };
static RGBA BG;
static Vector2 menuPos(0, 0);
static float OptionY = 0;
static int g_selectedPed = 0;
namespace SUB { constexpr int PEDSELECTOR = 1, SPOONER_SPAWN_ADDON_PEDS = 2; }
enum class EntityType { ALL, PED };
struct MouseSupport { static inline std::vector<int> vItems; };
struct Menu
{
	static inline int printingop = 0, currentop = 1, totalop = 0;
	static inline int currentsub = SUB::PEDSELECTOR;
	static inline int* currentopATM = &currentop;
};
namespace DxHookIMG { struct DxTexture { void Load(const std::string&) {} bool Exists() { return true; } void Draw(int, Vector2, Vector2, int, RGBA) {} }; }
namespace Game
{
	namespace Print { void PrintError_InvalidModel() {} void PrintBottomLeft(const std::string&) {} }
	std::string InputBox(const std::string& query, int, const std::string&, const std::string&) { return query; }
}
namespace sub { void ChangeModel_(Model) { ++changes; } }
namespace sub::Spooner
{
	namespace EntityManagement { void AddPed(Model, const std::string&) { ++worldSpawns; } }
	namespace SpoonerMode
	{
		struct Preview { EntityType entityType = EntityType::ALL; Model model; };
		static Preview ModelPreviewInfo;
	}
}
enum class Pathff { Main };
std::wstring ExePathW() { return L"missing"; }
std::wstring GetPathffW(Pathff) { return L"missing"; }
bool IsStockPedModel(Hash) { return false; }
void RegisterAddonPedModelName(Hash, const std::string&, const std::string&) {}
static int fullScans = 0;
std::vector<std::string> FindAddonPedModelNames(const std::filesystem::path&) { ++fullScans; return {}; }
static std::vector<std::pair<std::string, std::string>> g_pedModels;
int PLAYER_PED_ID() { return 1; }
std::uint64_t GetTickMs() { return now; }
void DRAW_RECT(float, float, float, float, int, int, int, int, bool) {}
void AddTitle(const std::string&) {}
void AddOption(const std::string&, bool&) { ++options; ++Menu::printingop; }
void AddToggle(const std::string&, bool&, bool&, bool&) { ++options; ++Menu::printingop; }
void AddBreak(const std::string&) { ++Menu::printingop; }
struct Position { float x = 0, y = 0, z = 0; };
Position GET_ENTITY_COORDS(int, bool) { return {}; }
static int createdPeds = 0, deletedPeds = 0, releasedHeadshots = 0;
int CREATE_PED(int, Hash, float, float, float, int, bool, bool) { return ++createdPeds; }
void DELETE_PED(int* ped) { ++deletedPeds; *ped = 0; }
void SET_ENTITY_VISIBLE(int, bool, bool) {}
void SET_ENTITY_COLLISION(int, bool, bool) {}
void FREEZE_ENTITY_POSITION(int, bool) {}
void SET_ENTITY_INVINCIBLE(int, bool) {}
void SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(int, bool) {}
void SET_PED_DEFAULT_COMPONENT_VARIATION(int) {}
int REGISTER_PEDHEADSHOT(int ped) { return ped; }
void UNREGISTER_PEDHEADSHOT(int) { ++releasedHeadshots; }
bool IS_PEDHEADSHOT_VALID(int) { return true; }
static bool headshotsReady = true;
bool IS_PEDHEADSHOT_READY(int) { return headshotsReady; }
const char* GET_PEDHEADSHOT_TXD_STRING(int) { return "headshot"; }
float GET_ASPECT_RATIO(bool) { return 1.777f; }
void DRAW_SPRITE(const char*, const char*, float, float, float, float, int, int, int, int, int, bool, int) {}

// Compile the complete production selector with only GTA and drawing replaced.
#include "PedSelectorPerformanceProduction.inl"

int main()
{
	initialized = true;
	for (int i = 0; i < 10000; ++i)
	{
		cards.push_back({ "model" + std::to_string(i), "Model " + std::to_string(i), Model(static_cast<Hash>(i + 1)), {}, {} });
	}
	Menu::totalop = static_cast<int>(cards.size());
	sub::PedSelector();
	const int rendered = options;
	SelectPed(cards.front());
	std::cout << "Rows processed for 10000 peds: " << rendered << "; blocking model loads on selection: " << blockingLoads << '\n';
	if (rendered > 20 || blockingLoads != 0 || changes != 0)
	{
		std::cerr << "FAIL: selection waits for streaming or the menu processes every hidden row.\n";
		return 1;
	}
	std::cout << "Menu work stays bounded and unloaded models are requested without blocking.\n";
	for (int selection : { 1, 8, 500, 9999, 10000 })
	{
		options = 0;
		Menu::printingop = 0;
		Menu::currentop = selection;
		sub::PedSelector();
		if (options > 14 || Menu::printingop != 10000) return 10;
	}
	// Streaming completion, replacement requests and timeout all use the production state machine.
	SelectPed(cards[1]);
	Model::loaded.insert(cards[0].model.hash);
	PollSelection();
	if (changes != 0) return 2;
	Model::loaded.insert(cards[1].model.hash);
	PollSelection();
	if (changes != 1 || pendingSelection.hash) return 3;
	SelectPed(cards[2]);
	now += 4000;
	PollSelection();
	if (changes != 1 || pendingSelection.hash) return 4;
	SelectPed(cards[3]);
	Menu::currentsub = 0;
	sub::TickPedSelector();
	if (pendingSelection.hash || changes != 1) return 11;
	Menu::currentsub = SUB::SPOONER_SPAWN_ADDON_PEDS;
	SelectPed(cards[1]);
	sub::TickPedSelector();
	if (worldSpawns != 1 || changes != 1 || pendingSelection.hash) return 14;
	Menu::printingop = 0;
	Menu::currentop = 1;
	sub::PedSelector();
	if (sub::Spooner::SpoonerMode::ModelPreviewInfo.entityType != EntityType::PED ||
		sub::Spooner::SpoonerMode::ModelPreviewInfo.model.hash != cards[0].model.hash) return 15;
	Menu::currentsub = SUB::PEDSELECTOR;
	SelectPed(cards[1]);
	Menu::currentsub = SUB::SPOONER_SPAWN_ADDON_PEDS;
	sub::TickPedSelector();
	if (pendingSelection.hash || worldSpawns != 1 || changes != 1) return 16;
	Menu::currentsub = SUB::PEDSELECTOR;

	// Portrait creation is debounced, asynchronous and cleaned up when the menu closes.
	FocusPreview(&cards[0]);
	PollPreview();
	if (createdPeds) return 5;
	now += 201;
	PollPreview();
	if (createdPeds != 1 || deletedPeds != 1 || portraits.size() != 1) return 6;
	Menu::currentsub = 0;
	sub::TickPedSelector();
	if (releasedHeadshots != 1 || !portraits.empty()) return 7;
	Menu::currentsub = SUB::PEDSELECTOR;
	headshotsReady = false;
	FocusPreview(&cards[1]);
	now += 201;
	PollPreview();
	now += 4000;
	PollPreview();
	if (createdPeds != 2 || deletedPeds != 2 || releasedHeadshots != 2) return 8;

	const auto cache = std::filesystem::current_path() / "PedSelectorTestCache.txt";
	{ std::ofstream file(cache); file << "MenyooPedCache1\nmodel1\n"; }
	const auto cached = Discover("missing", cache, "missing", true);
	if (!cached.cached || cached.entries.size() != 1 || cached.entries[0].name != "model1" || fullScans) return 9;
	ClearPreview();
	failedPortraits.clear();
	headshotsReady = true;
	for (int i = 0; i < 10; ++i)
	{
		Model::loaded.insert(cards[i].model.hash);
		FocusPreview(&cards[i]);
		now += 201;
		PollPreview();
	}
	if (portraits.size() != 8) return 12;
	Menu::currentsub = 0;
	sub::TickPedSelector();
	if (!portraits.empty() || createdPeds != deletedPeds || releasedHeadshots != createdPeds) return 13;
	std::cout << "Deferred switching, timeout, portrait cleanup and warm cache checks passed.\n";
	std::cout << "Spooner add-on spawning, world preview and selector transition checks passed.\n";
}
