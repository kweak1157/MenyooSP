#include "PedSelector.h"
#include "PedSelectorState.h"
#include "PedModelChanger.h"
#include "Spooner\EntityManagement.h"
#include "Spooner\SpoonerEntity.h"
#include "Spooner\SpoonerMode.h"
#include "..\Menu\Menu.h"
#include "..\Menu\Routine.h"
#include "..\Natives\natives2.h"
#include "..\Scripting\AddonVehicleModels.h"
#include "..\Scripting\DxHookIMG.h"
#include "..\Scripting\Game.h"
#include "..\Scripting\Model.h"
#include "..\Scripting\ModelNames.h"
#include "..\Scripting\StockPedModels.h"
#include "..\Util\Clock.h"
#include "..\Util\ExePath.h"
#include "..\Util\GTAmath.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <unordered_map>
#include <unordered_set>

namespace
{
	struct PedCard
	{
		std::string name, caption;
		Model model;
		std::string image;
		std::string label;
	};
	struct Candidate { std::string name, image; };
	struct ScanResult { std::vector<Candidate> entries; bool cached = false; };
	std::vector<PedCard> cards;
	std::unordered_map<std::string, std::string> captions;
	std::unordered_map<std::string, DxHookIMG::DxTexture> textures;
	std::future<ScanResult> scan;
	std::future<void> cacheWrite;
	std::vector<Candidate> candidates;
	std::unordered_set<Hash> added;
	std::size_t nextCandidate = 0;
	bool initialized = false, scanFailed = false, cachedScan = false, captionsReady = false;
	Model pendingSelection;
	int pendingMenu = SUB::PEDSELECTOR;
	std::string pendingName;
	std::uint64_t selectionDeadline = 0;

	std::filesystem::path CachePath() { return std::filesystem::path(GetPathffW(Pathff::Main)) / "PedSelectorCache.txt"; }

	bool Scanning() { return scan.valid() || nextCandidate < candidates.size(); }

	ScanResult Discover(const std::filesystem::path& root, const std::filesystem::path& cache,
		const std::filesystem::path& previews, bool useCache)
	{
		std::vector<std::string> names;
		if (useCache)
		{
			std::ifstream file(cache);
			std::string line;
			if (std::getline(file, line) && line == "MenyooPedCache1")
				while (std::getline(file, line))
					if (!line.empty() && line.size() <= 256 && line.find_first_of("/\\") == std::string::npos) names.push_back(line);
		}
		ScanResult result;
		result.cached = !names.empty();
		if (!result.cached) names = FindAddonPedModelNames(root);
		result.entries.reserve(names.size());
		for (auto& name : names)
		{
			std::error_code error;
			const auto image = previews / (name + ".png");
			result.entries.push_back({ std::move(name), std::filesystem::is_regular_file(image, error) ? image.string() : std::string() });
		}
		return result;
	}

	void StartScan(bool useCache)
	{
		const auto root = std::filesystem::path(ExePathW());
		const auto cache = CachePath();
		const auto previews = std::filesystem::path(GetPathffW(Pathff::Main)) / "PedPreviews";
		scan = std::async(std::launch::async, [root, cache, previews, useCache] { return Discover(root, cache, previews, useCache); });
	}

	void PollScan()
	{
		if (!captionsReady)
		{
			for (const auto& known : g_pedModels) captions.emplace(PedSelectorUI::Lower(known.first), known.second);
			captionsReady = true;
		}
		if (scan.valid() && scan.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			try
			{
				auto result = scan.get();
				candidates = std::move(result.entries);
				cachedScan = result.cached;
				nextCandidate = 0;
			}
			catch (...) { scanFailed = true; }
		}
		// Bound native validation to at most 64 candidates and two milliseconds per frame.
		const auto deadline = GetTickMs() + 2;
		const auto end = (std::min)(candidates.size(), nextCandidate + 64);
		while (nextCandidate < end && GetTickMs() < deadline)
		{
			const auto& candidate = candidates[nextCandidate++];
			const auto& name = candidate.name;
			Model model(name);
			if (IsStockPedModel(model.hash) || added.count(model.hash) || !model.IsInCdImage() || !model.IsPed()) continue;
			added.insert(model.hash);
			PedCard card{ name, name, model, candidate.image, {} };
			const auto known = captions.find(name);
			if (known != captions.end() && !known->second.empty()) card.caption = known->second;
			card.label = card.caption;
			std::replace(card.label.begin(), card.label.end(), '~', ' ');
			RegisterAddonPedModelName(model.hash, card.name, card.label);
			cards.push_back(std::move(card));
		}
		if (!scan.valid() && nextCandidate == candidates.size() && !candidates.empty())
		{
			candidates.clear();
			nextCandidate = 0;
			if (cachedScan) { cachedScan = false; StartScan(false); }
			else if (!cacheWrite.valid() || cacheWrite.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
			{
				std::vector<std::string> names;
				for (const auto& ped : cards) names.push_back(ped.name);
				const auto path = CachePath();
				cacheWrite = std::async(std::launch::async, [path, names = std::move(names)]
				{
					std::ofstream file(path, std::ios::trunc);
					file << "MenyooPedCache1\n";
					for (const auto& name : names) file << name << '\n';
				});
			}
		}
	}

	void SelectPed(const PedCard& ped)
	{
		if (!ped.model.IsInCdImage() || !ped.model.IsPed()) { Game::Print::PrintError_InvalidModel(); return; }
		if (pendingSelection.hash) pendingSelection.Unload();
		pendingSelection = ped.model;
		pendingMenu = Menu::currentsub;
		pendingName = ped.caption;
		selectionDeadline = GetTickMs() + 4000;
		pendingSelection.Load();
	}

	void PollSelection()
	{
		if (!pendingSelection.hash) return;
		if (pendingSelection.IsLoaded())
		{
			const auto model = pendingSelection;
			pendingSelection = Model();
			if (pendingMenu == SUB::SPOONER_SPAWN_ADDON_PEDS)
				sub::Spooner::EntityManagement::AddPed(model, pendingName);
			else
			{
				sub::ChangeModel_(model); // Already streamed: the existing changer does not wait.
				g_selectedPed = PLAYER_PED_ID();
			}
		}
		else if (GetTickMs() >= selectionDeadline)
		{
			pendingSelection.Unload();
			pendingSelection = Model();
			Game::Print::PrintBottomLeft("Unable to load this ped. Check the installed pack.");
		}
	}

	// Only the focused model is streamed for a portrait. Keep eight headshot handles at most.
	struct Portrait { int handle; std::uint64_t used; };
	std::unordered_map<Hash, Portrait> portraits;
	std::unordered_set<Hash> failedPortraits;
	Model previewModel;
	int previewPed = 0, previewHeadshot = -1;
	bool previewRequested = false;
	bool previewNeedsPortrait = false;
	std::uint64_t previewSince = 0;

	void ClearPreview()
	{
		if (previewHeadshot >= 0) UNREGISTER_PEDHEADSHOT(previewHeadshot);
		if (previewPed) DELETE_PED(&previewPed);
		if (previewRequested && previewModel.hash != pendingSelection.hash) previewModel.Unload();
		previewModel = Model();
		previewHeadshot = -1;
		previewRequested = false;
	}

	void FocusPreview(const PedCard* ped)
	{
		const Hash target = ped ? ped->model.hash : 0;
		if (target == previewModel.hash) return;
		ClearPreview();
		previewModel = Model(target);
		previewNeedsPortrait = ped && ped->image.empty();
		previewSince = GetTickMs();
	}

	void PollPreview()
	{
		if (!previewModel.hash || failedPortraits.count(previewModel.hash)) return;
		const auto elapsed = GetTickMs() - previewSince;
		if (elapsed < 200 || pendingSelection.hash) return;
		if (previewRequested && previewModel.IsLoaded() && (!previewNeedsPortrait || portraits.count(previewModel.hash))) return;
		if (elapsed >= 4000 && (!previewModel.IsLoaded() || (previewNeedsPortrait && !portraits.count(previewModel.hash))))
		{
			failedPortraits.insert(previewModel.hash);
			ClearPreview();
			return;
		}
		if (!previewRequested) { previewModel.Load(); previewRequested = true; }
		if (!previewModel.IsLoaded()) return;
		if (!previewNeedsPortrait || portraits.count(previewModel.hash)) return;
		if (!previewPed)
		{
			const auto pos = GET_ENTITY_COORDS(PLAYER_PED_ID(), true);
			previewPed = CREATE_PED(4, previewModel.hash, pos.x, pos.y, pos.z - 10.0f, 0, false, true);
			if (!previewPed) { failedPortraits.insert(previewModel.hash); ClearPreview(); return; }
			SET_ENTITY_VISIBLE(previewPed, false, false);
			SET_ENTITY_COLLISION(previewPed, false, false);
			FREEZE_ENTITY_POSITION(previewPed, true);
			SET_ENTITY_INVINCIBLE(previewPed, true);
			SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(previewPed, true);
			SET_PED_DEFAULT_COMPONENT_VARIATION(previewPed);
			previewHeadshot = REGISTER_PEDHEADSHOT(previewPed);
		}
		if (previewHeadshot >= 0 && IS_PEDHEADSHOT_VALID(previewHeadshot) && IS_PEDHEADSHOT_READY(previewHeadshot))
		{
			if (portraits.size() >= 8)
			{
				const auto oldest = std::min_element(portraits.begin(), portraits.end(),
					[](const auto& a, const auto& b) { return a.second.used < b.second.used; });
				UNREGISTER_PEDHEADSHOT(oldest->second.handle);
				portraits.erase(oldest);
			}
			portraits.emplace(previewModel.hash, Portrait{ previewHeadshot, GetTickMs() });
			previewHeadshot = -1; // Cache owns this handle until eviction or menu close.
			DELETE_PED(&previewPed);
			// Keep the focused model streamed so selecting it does not restart loading.
		}
	}

	void DrawPreview(const PedCard& ped)
	{
		const float x = menuPos.x > 0.45f ? menuPos.x - 0.003f : 0.324f + menuPos.x;
		// Anchor beside the top of the menu instead of following the selected row.
		const float y = 0.205f + menuPos.y;
		if (!ped.image.empty())
		{
			auto [it, inserted] = textures.try_emplace(ped.image);
			if (inserted) it->second.Load(ped.image);
			if (!it->second.Exists()) return;
			DRAW_RECT(x, y, 0.103f, 0.0919f, BG.R, BG.G, BG.B, BG.A, false);
			it->second.Draw(0, Vector2(x, y), Vector2(0.1f, 0.04945f), 0, RGBA::AllWhite());
		}
		else
		{
			const auto found = portraits.find(ped.model.hash);
			if (found == portraits.end()) return;
			found->second.used = GetTickMs();
			const char* dict = GET_PEDHEADSHOT_TXD_STRING(found->second.handle);
			if (!dict || !*dict) return;
			const float width = 0.09f / GET_ASPECT_RATIO(false);
			DRAW_RECT(x, y, width + 0.003f, 0.093f, BG.R, BG.G, BG.B, BG.A, false);
			DRAW_SPRITE(dict, dict, x, y, width, 0.09f, 0, 255, 255, 255, 255, false, 0);
		}
	}
}

namespace sub
{
	void StartPedSelector()
	{
		if (initialized) return;
		initialized = true;
		StartScan(true);
	}

	void TickPedSelector()
	{
		PollScan();
		const bool selectorOpen = Menu::currentsub == SUB::PEDSELECTOR || Menu::currentsub == SUB::SPOONER_SPAWN_ADDON_PEDS;
		if (selectorOpen && Menu::currentsub == pendingMenu) PollSelection();
		else if (pendingSelection.hash) { pendingSelection.Unload(); pendingSelection = Model(); }
		if (selectorOpen) PollPreview();
		else
		{
			ClearPreview();
			for (const auto& portrait : portraits) UNREGISTER_PEDHEADSHOT(portrait.second.handle);
			portraits.clear();
			failedPortraits.clear();
		}
	}

	void PedSelector()
	{
		const bool spawnIntoWorld = Menu::currentsub == SUB::SPOONER_SPAWN_ADDON_PEDS;
		AddTitle(spawnIntoWorld ? "Addon Peds" : "Addon Ped Selector");
		if (!initialized) StartPedSelector();
		MouseSupport::vItems.clear();

		const int firstOption = Menu::printingop + 1;
		const int visible = PedSelectorUI::FirstVisibleOption(Menu::currentop, Menu::totalop);
		const int first = std::clamp(visible - firstOption, 0, static_cast<int>(cards.size()));
		const int end = std::clamp(visible + 14 - firstOption, first, static_cast<int>(cards.size()));
		Menu::printingop += first;
		const PedCard* selectedPed = nullptr;
		const PedCard* focusedPed = nullptr;
		for (int slot = first; slot < end; ++slot)
		{
			const auto& ped = cards[slot];
			bool selected = false;
			AddOption(ped.label, selected);
			if (selected) selectedPed = &ped;
			if (Menu::printingop == *Menu::currentopATM)
			{
				focusedPed = &ped;
				DrawPreview(ped);
				if (spawnIntoWorld)
				{
					auto& preview = Spooner::SpoonerMode::ModelPreviewInfo;
					preview.entityType = EntityType::PED;
					preview.model = ped.model;
				}
			}
		}
		Menu::printingop += static_cast<int>(cards.size()) - end;
		if (cards.empty()) AddBreak(Scanning() ? "Loading peds..." : scanFailed ? "Unable to load peds." : "No add-on peds found.");
		FocusPreview(focusedPed);
		if (selectedPed) SelectPed(*selectedPed);
	}
}
