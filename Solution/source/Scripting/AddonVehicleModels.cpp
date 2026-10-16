#include "AddonVehicleModels.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <unordered_set>

namespace
{
	using Bytes = std::vector<unsigned char>;
	using ModelNames = std::unordered_set<std::string>;

	std::uint32_t Read32(const unsigned char* data)
	{
		return std::uint32_t(data[0]) | (std::uint32_t(data[1]) << 8)
			| (std::uint32_t(data[2]) << 16) | (std::uint32_t(data[3]) << 24);
	}

	std::string Lower(std::string name)
	{
		std::transform(name.begin(), name.end(), name.begin(),
			[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
		return name;
	}

	void AddModelName(const std::string& filename, ModelNames& models, bool peds)
	{
		if (filename.size() <= 4 || !(filename.ends_with(".yft") || (peds && filename.ends_with(".ydd"))))
			return;
		const auto name = filename.substr(0, filename.size() - 4);
		if (!name.ends_with("_hi"))
			models.insert(name);
	}

	bool ReadAt(std::ifstream& file, std::uint64_t offset, unsigned char* data, std::size_t size)
	{
		file.clear();
		file.seekg(static_cast<std::streamoff>(offset));
		return static_cast<bool>(file.read(reinterpret_cast<char*>(data), static_cast<std::streamsize>(size)));
	}

	// RPF7 table layout reference:
	// https://github.com/dexyfex/CodeWalker/blob/master/CodeWalker.Core/GameFiles/RpfFile.cs
	// Only read archive tables; vehicle resources do not need to be decompressed.
	void ScanArchive(std::ifstream& file, std::uint64_t start, std::uint64_t length,
		ModelNames& models, bool peds, unsigned depth = 0)
	{
		if (depth > 16 || length < 16)
			return;
		std::array<unsigned char, 16> header{};
		if (!ReadAt(file, start, header.data(), header.size()) || Read32(header.data()) != 0x52504637)
			return;
		const auto entries = Read32(header.data() + 4);
		const auto namesLength = Read32(header.data() + 8);
		const auto encryption = Read32(header.data() + 12);
		// NONE and OpenIV OPEN archives have unencrypted directory tables.
		if (encryption != 0 && encryption != 0x4E45504F)
			return;
		const std::uint64_t tableLength = std::uint64_t(entries) * 16;
		if (entries > 262144 || namesLength > 16 * 1024 * 1024 || tableLength + namesLength > length - 16)
			return;
		Bytes table(static_cast<std::size_t>(tableLength));
		Bytes names(namesLength);
		if (!ReadAt(file, start + 16, table.data(), table.size())
			|| !ReadAt(file, start + 16 + tableLength, names.data(), names.size()))
			return;
		for (std::uint32_t index = 0; index < entries; ++index)
		{
			const auto* entry = table.data() + index * 16;
			const auto flags = Read32(entry + 4);
			if (flags == 0x7FFFFF00) // Directory.
				continue;
			const auto nameOffset = Read32(entry) & 0xFFFF;
			if (nameOffset >= names.size())
				continue;
			const auto begin = names.begin() + nameOffset;
			const auto end = std::find(begin, names.end(), 0);
			if (end == names.end() || end - begin > 256)
				continue;
			const auto name = Lower(std::string(begin, end));
			if ((flags & 0x80000000) != 0) // Resource entry.
			{
				AddModelName(name, models, peds);
				continue;
			}
			if (!name.ends_with(".rpf"))
				continue;
			const auto compressedSize = (Read32(entry + 2) & 0xFFFFFF);
			const auto offset = std::uint64_t(Read32(entry + 4) >> 8) * 512;
			const auto childLength = Read32(entry + 8);
			const auto encrypted = Read32(entry + 12);
			// Embedded RPFs are normally uncompressed binary entries.
			if (compressedSize != 0 || encrypted != 0 || offset < 16 + tableLength + namesLength
				|| offset > length || childLength > length - offset)
				continue;
			ScanArchive(file, start + offset, childLength, models, peds, depth + 1);
		}
	}
}

static void ScanPacks(const std::filesystem::path& packs, const std::filesystem::path& originalPacks,
	ModelNames& models, bool peds)
{
	std::error_code error;
	for (std::filesystem::directory_iterator pack(packs, error), end; !error && pack != end; pack.increment(error))
	{
		if (!pack->is_directory(error))
			continue;
		// A copied official DLC/patch pack is not a user add-on pack.
		if (!peds && std::filesystem::exists(originalPacks / pack->path().filename(), error))
			continue;
		const auto packName = Lower(pack->path().filename().string());
		if (!peds && packName.starts_with("patchday"))
			continue;
		for (std::filesystem::recursive_directory_iterator entry(pack->path(), error), filesEnd;
			!error && entry != filesEnd; entry.increment(error))
		{
			if (!entry->is_regular_file(error))
				continue;
			const auto filename = Lower(entry->path().filename().string());
			AddModelName(filename, models, peds);
			if (filename.ends_with(".rpf"))
			{
				const auto length = entry->file_size(error);
				if (error)
					break;
				std::ifstream archive(entry->path(), std::ios::binary);
				if (archive)
					ScanArchive(archive, 0, length, models, peds);
			}
		}
		// An unreadable pack must not prevent scanning the next pack.
		error.clear();
	}
}

std::vector<std::string> FindAddonVehicleModelNames(const std::filesystem::path& gameRoot)
{
	const auto relativePacks = std::filesystem::path("update") / "x64" / "dlcpacks";
	ModelNames models;
	ScanPacks(gameRoot / "mods" / relativePacks, gameRoot / relativePacks, models, false);
	std::vector<std::string> result(models.begin(), models.end());
	std::sort(result.begin(), result.end());
	return result;
}

std::vector<std::string> FindAddonPedModelNames(const std::filesystem::path& gameRoot)
{
	const auto relativePacks = std::filesystem::path("update") / "x64" / "dlcpacks";
	ModelNames models;
	ScanPacks(gameRoot / "mods" / relativePacks, {}, models, true);
	ScanPacks(gameRoot / relativePacks, {}, models, true);
	std::vector<std::string> result(models.begin(), models.end());
	std::sort(result.begin(), result.end());
	return result;
}
