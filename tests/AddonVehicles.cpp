#include "Scripting/StockVehicleModels.h"
#include "Scripting/AddonVehicleModels.h"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <vector>

using Hash = DWORD;
struct Model
{
	Hash hash;
	bool available = true;
	bool vehicle = true;
	std::string displayName = "Adder";
	bool IsInCdImage() const { return available; }
	bool IsVehicle() const { return vehicle; }
};

std::vector<Model> g_vehHashes;
std::vector<Model> g_vehHashes_ADDON;
static std::unordered_map<DWORD, std::string> addonVehicleModelNames;

static std::filesystem::path fixtureRoot = std::filesystem::current_path() / "AddonVehicleFixture";
std::wstring ExePathW() { return fixtureRoot.wstring(); }
Hash GET_HASH_KEY(const std::string& name)
{
	if (name == "customone") return 0x11223344;
	if (name == "customtwo") return 0x55667788;
	if (name == "unavailable") return 0x12345678;
	if (name == "nonvehicle") return 0x23456789;
	if (name == "official_unknown") return 0x34567890;
	if (name == "adder") return VEHICLE_ADDER;
	return 0;
}

#include "AddonVehicleProduction.inl"

static void Require(bool condition, const char* message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		std::exit(1);
	}
}

static void WriteArchive(const std::filesystem::path& path, const std::vector<std::string>& names,
	const std::vector<unsigned char>& child = {})
{
	std::filesystem::create_directories(path.parent_path());
	const auto count = static_cast<std::uint32_t>(names.size() + (child.empty() ? 0 : 1));
	std::vector<unsigned char> nameData;
	std::vector<std::uint16_t> offsets;
	for (const auto& name : names)
	{
		offsets.push_back(static_cast<std::uint16_t>(nameData.size()));
		nameData.insert(nameData.end(), name.begin(), name.end());
		nameData.push_back(0);
	}
	const auto childNameOffset = static_cast<std::uint16_t>(nameData.size());
	if (!child.empty())
	{
		const std::string name = "vehicles.rpf";
		nameData.insert(nameData.end(), name.begin(), name.end());
		nameData.push_back(0);
	}
	std::vector<unsigned char> data(16 + count * 16 + nameData.size());
	const auto put32 = [&data](std::size_t offset, std::uint32_t value)
	{
		for (unsigned i = 0; i < 4; ++i) data[offset + i] = static_cast<unsigned char>(value >> (i * 8));
	};
	put32(0, 0x52504637);
	put32(4, count);
	put32(8, static_cast<std::uint32_t>(nameData.size()));
	put32(12, 0x4E45504F);
	for (std::size_t i = 0; i < names.size(); ++i)
	{
		put32(16 + i * 16, offsets[i]);
		put32(20 + i * 16, 0x80000000);
	}
	if (!child.empty())
	{
		const auto childOffset = (data.size() + 511) / 512 * 512;
		const auto entry = 16 + names.size() * 16;
		put32(entry, childNameOffset);
		put32(entry + 4, static_cast<std::uint32_t>(childOffset / 512) << 8);
		put32(entry + 8, static_cast<std::uint32_t>(child.size()));
		data.resize(childOffset);
		data.insert(data.end(), child.begin(), child.end());
	}
	std::copy(nameData.begin(), nameData.end(), data.begin() + 16 + count * 16);
	std::ofstream file(path, std::ios::binary);
	file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
}

int main(int argc, char** argv)
{
	if (argc == 2)
	{
		for (const auto& name : FindAddonVehicleModelNames(argv[1])) std::cout << name << '\n';
		return 0;
	}
	const auto packs = fixtureRoot / "mods/update/x64/dlcpacks";
	WriteArchive(packs / "custompack/dlc.rpf", { "customone.yft", "customone_hi.yft", "adder.yft", "unavailable.yft", "nonvehicle.yft", "not_a_vehicle.ytd" });
	WriteArchive(fixtureRoot / "child.rpf", { "customtwo.YFT" });
	std::ifstream childFile(fixtureRoot / "child.rpf", std::ios::binary);
	std::vector<unsigned char> child((std::istreambuf_iterator<char>(childFile)), std::istreambuf_iterator<char>());
	WriteArchive(packs / "different_pack_name/dlc.rpf", {}, child);
	std::filesystem::create_directories(fixtureRoot / "update/x64/dlcpacks/mpofficial");
	WriteArchive(packs / "mpofficial/dlc.rpf", { "official_unknown.yft" });
	WriteArchive(packs / "encrypted/dlc.rpf", { "encrypted_model.yft" });
	{
		std::fstream encrypted(packs / "encrypted/dlc.rpf", std::ios::in | std::ios::out | std::ios::binary);
		encrypted.seekp(12);
		const unsigned char encryption[] = { 0xFF, 0xFF, 0xEF, 0x0F };
		encrypted.write(reinterpret_cast<const char*>(encryption), sizeof(encryption));
	}
	std::filesystem::create_directories(packs / "broken");
	std::ofstream(packs / "broken/dlc.rpf") << "custom_fake.yft";
	const auto found = FindAddonVehicleModelNames(fixtureRoot);
	Require(std::find(found.begin(), found.end(), "customtwo") != found.end(), "Nested archive model was not detected.");
	Require(std::find(found.begin(), found.end(), "official_unknown") == found.end(), "A copied official pack was scanned.");
	Require(std::find(found.begin(), found.end(), "customone_hi") == found.end(), "High-detail duplicate was not excluded.");
	Require(std::find(found.begin(), found.end(), "encrypted_model") == found.end(), "An encrypted archive table was accepted.");
	Require(std::find(found.begin(), found.end(), "custom_fake") == found.end(), "A malformed archive produced a model.");
	Require(FindAddonVehicleModelNames(fixtureRoot / "missing").empty(), "Missing dlcpacks must produce an empty list.");
	const DWORD stockModels[] = {
#include "StockVehicleCatalogue.inl"
	};
	for (DWORD hash : stockModels)
	{
		Require(IsStockVehicleModel(hash), "Official vehicle is missing from the stock catalogue.");
		g_vehHashes.push_back({ hash });
	}
	PopulateAddonVehicleModelsArray();
	Require(g_vehHashes_ADDON.empty(), "A standard or DLC vehicle appeared in the add-on list.");

	// Models outside the user's custom packs must never be inferred to be add-ons.
	g_vehHashes = { { 0x34567890 } };
	PopulateAddonVehicleModelsArray();
	Require(g_vehHashes_ADDON.empty(), "A vehicle absent from custom dlcpacks appeared in the add-on list.");

	constexpr Hash addonOne = 0x11223344;
	constexpr Hash addonTwo = 0x55667788;
	Require(!IsStockVehicleModel(addonOne) && !IsStockVehicleModel(addonTwo), "Test model hashes overlap stock models.");
	// Both add-ons share a stock display name. Only their model hashes matter.
	g_vehHashes.push_back({ addonOne });
	g_vehHashes.push_back({ addonTwo });
	g_vehHashes.push_back({ addonOne });
	g_vehHashes.push_back({ 0x12345678, false, true });
	g_vehHashes.push_back({ 0x23456789, true, false });
	PopulateAddonVehicleModelsArray();
	Require(g_vehHashes_ADDON.size() == 2, "Add-ons must be valid, available, and unique.");
	Require(g_vehHashes_ADDON[0].hash == addonOne && g_vehHashes_ADDON[1].hash == addonTwo,
		"Add-on filtering changed the source list order.");
	Require(addonVehicleModelNames.at(addonOne) == "customone", "Installed model name was lost.");

	// A rescan must discard removed packs and models that are no longer available.
	g_vehHashes = { { addonTwo, false, true }, { VEHICLE_ADDER } };
	PopulateAddonVehicleModelsArray();
	Require(g_vehHashes_ADDON.empty(), "Rescan retained a removed or unavailable add-on.");
	g_vehHashes.clear();
	PopulateAddonVehicleModelsArray();
	Require(g_vehHashes_ADDON.empty(), "Empty input must produce an empty add-on list.");
	std::cout << "Addon vehicle checks passed; " << std::size(stockModels) << " official models excluded.\n";
}
