#pragma once

#include <filesystem>
#include <string>
#include <vector>

// Discover vehicle assets in custom mods/update/x64/dlcpacks archives.
std::vector<std::string> FindAddonVehicleModelNames(const std::filesystem::path& gameRoot);

// Ped candidates from both DLC pack folders. Callers must check model type and availability.
std::vector<std::string> FindAddonPedModelNames(const std::filesystem::path& gameRoot);
