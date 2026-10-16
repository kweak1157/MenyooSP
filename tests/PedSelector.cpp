#include "Scripting/AddonVehicleModels.h"
#include "Scripting/StockPedModels.h"
#include "Submenus/PedSelectorState.h"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>

static void Require(bool condition, const char* message)
{
	if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

static std::vector<unsigned char> Archive(const std::vector<std::string>& names,
	const std::vector<unsigned char>& child = {})
{
	std::vector<unsigned char> strings;
	std::vector<std::uint32_t> offsets;
	for (const auto& name : names)
	{
		offsets.push_back(static_cast<std::uint32_t>(strings.size()));
		strings.insert(strings.end(), name.begin(), name.end());
		strings.push_back(0);
	}
	if (!child.empty())
	{
		offsets.push_back(static_cast<std::uint32_t>(strings.size()));
		const std::string filename = "peds.rpf";
		strings.insert(strings.end(), filename.begin(), filename.end());
		strings.push_back(0);
	}
	std::vector<unsigned char> data(16 + offsets.size() * 16 + strings.size());
	const auto put = [&data](std::size_t offset, std::uint32_t value)
	{
		for (unsigned i = 0; i < 4; ++i) data[offset + i] = static_cast<unsigned char>(value >> (i * 8));
	};
	put(0, 0x52504637);
	put(4, static_cast<std::uint32_t>(offsets.size()));
	put(8, static_cast<std::uint32_t>(strings.size()));
	put(12, 0x4E45504F);
	for (std::size_t i = 0; i < names.size(); ++i)
	{
		put(16 + i * 16, offsets[i]);
		put(20 + i * 16, 0x80000000);
	}
	std::copy(strings.begin(), strings.end(), data.begin() + 16 + offsets.size() * 16);
	if (!child.empty())
	{
		const auto childOffset = (data.size() + 511) / 512 * 512;
		const auto entry = 16 + names.size() * 16;
		put(entry, offsets.back());
		put(entry + 4, static_cast<std::uint32_t>(childOffset / 512) << 8);
		put(entry + 8, static_cast<std::uint32_t>(child.size()));
		data.resize(childOffset);
		data.insert(data.end(), child.begin(), child.end());
	}
	return data;
}

static void Write(const std::filesystem::path& path, const std::vector<unsigned char>& data)
{
	std::filesystem::create_directories(path.parent_path());
	std::ofstream file(path, std::ios::binary);
	file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
}

int main()
{
	const auto root = std::filesystem::current_path() / "PedSelectorFixture";
	const auto mods = root / "mods/update/x64/dlcpacks";
	Write(mods / "addonpeds/dlc.rpf", Archive({ "hero.YDD", "hero.yft", "hero_hi.yft", "texture.ytd" }, Archive({ "villain.yft" })));
	Write(root / "update/x64/dlcpacks/direct/dlc.rpf", Archive({ "directped.ydd", "hero.ydd" }));
	// Custom ped assets can be inserted in a copied official pack.
	std::filesystem::create_directories(root / "update/x64/dlcpacks/mpcopied");
	Write(mods / "mpcopied/dlc.rpf", Archive({ "insertedped.ydd" }));
	Write(mods / "loose/model.ydd", {});
	auto encrypted = Archive({ "hidden.ydd" });
	encrypted[12] = 0xFF;
	Write(mods / "encrypted/dlc.rpf", encrypted);
	auto malformed = Archive({ "bad.ydd" });
	malformed.resize(20);
	Write(mods / "broken/dlc.rpf", malformed);
	const auto names = FindAddonPedModelNames(root);
	Require(names == std::vector<std::string>({ "directped", "hero", "insertedped", "model", "villain" }),
		"Ped discovery must read both pack folders, nested archives and loose assets, deduplicate and sort, and reject broken/encrypted tables.");
	Require(FindAddonPedModelNames(root / "missing").empty(), "Missing packs should give an empty list.");
	const DWORD stock[] = {
#include "StockPedCatalogue.inl"
	};
	for (DWORD hash : stock) Require(IsStockPedModel(hash), "Stock ped catalogue is incomplete.");
	Require(!IsStockPedModel(0x11223344), "Custom hash was excluded as stock.");
	std::cout << "Ped discovery and stock catalogue checks passed.\n";
}
