#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using Hash = unsigned long;
using DWORD = Hash;
using UINT = unsigned int;
static Hash currentModel = 1;
static int playerHandle = 11;
enum class EntityType { ALL, PED, VEHICLE, PROP };
enum class PedType { Human, Animal };
enum class VehicleSeat { Driver };
struct s_Weapon_Components_Tint {};
struct Model
{
	Hash hash;
	Model(Hash value = 0) : hash(value) {}
	bool IsInCdImage() const { return true; }
	bool Load(int) const { return true; }
	void Unload() const {}
};
namespace GTAmodel { using Model = ::Model; }
struct GTAentity
{
	int value = 0;
	GTAentity(int handle = 0) : value(handle) {}
	int Handle() const { return value; }
	bool Equals(const GTAentity& other) const { return value == other.value; }
	bool Exists() const { return value != 0; }
	int Type() const { return static_cast<int>(EntityType::PED); }
	::Model Model() const { return ::Model(currentModel); }
	bool IsPositionFrozen() const { return false; }
	bool IsCollisionEnabled_get() const { return true; }
	void AttachTo(GTAentity, int, bool, int, int) {}
};
struct GTAvehicle : GTAentity { using GTAentity::GTAentity; };
struct GTAped : GTAentity
{
	using GTAentity::GTAentity;
	void StoreWeaponsInArray(std::vector<s_Weapon_Components_Tint>&) {}
	Hash Weapon_get() const { return 0; }
	bool IsInVehicle() const { return false; }
	GTAvehicle CurrentVehicle() const { return {}; }
	VehicleSeat CurrentVehicleSeat_get() const { return VehicleSeat::Driver; }
	bool IsCollisionEnabled_get() const { return true; }
	void IsCollisionEnabled_set(bool) {}
	void SetIntoVehicle(GTAvehicle, VehicleSeat) {}
	::PedType PedType() const { return ::PedType::Human; }
	void GiveWeaponsFromArray(const std::vector<s_Weapon_Components_Tint>&) {}
	void Weapon_set(Hash) {}
};
static bool bit_infinite_ammo = false;
int PLAYER_ID() { return 0; }
int PLAYER_PED_ID() { return playerHandle; }
void SET_PLAYER_MODEL(int, Hash hash) { currentModel = hash; playerHandle = 12; }
void SET_PED_RANDOM_COMPONENT_VARIATION(int, int) {}
bool HAS_ANIM_DICT_LOADED(const char*) { return true; }
void REQUEST_ANIM_DICT(const char*) {}
bool IS_WEAPON_VALID(Hash) { return false; }
void SET_PED_INFINITE_AMMO_CLIP(int, bool) {}
Hash GET_HASH_KEY(const std::string& name) { return name == "player_zero" ? 1 : name == "customhero" ? 2 : 3; }
std::string int_to_hexstring(Hash hash, bool) { return hash == 2 ? "0x2" : "0x3"; }
std::vector<std::pair<std::string, std::string>> g_pedModels{ { "player_zero", "Michael" } };
std::string get_vehicle_model_label(const Model&, bool) { return "Vehicle"; }
std::string get_prop_model_label(const Model&) { return "Object"; }

namespace sub
{
	namespace PedDamageTextures_catind { std::unordered_map<int, int> vPedsAndDamagePacks; }
	namespace PedDecals_catind { std::unordered_map<int, int> vPedsAndDecals; }
	namespace Spooner
	{
		struct SpoonerEntity
		{
			GTAentity Handle;
			EntityType Type = EntityType::PED;
			std::string HashName;
			struct { bool isAttached = false; int boneIndex = 0, offset = 0, rotation = 0; } AttachmentArgs;
			struct { std::string dict, name; } LastAnimation;
			struct { void Reset() {} } TaskSequence;
			bool Dynamic = true, IsStill = false;
		};
		namespace Databases { std::vector<SpoonerEntity> EntityDb; }
		SpoonerEntity SelectedEntity;
		namespace EntityManagement
		{
			int GetEntityIndexInDb(const GTAentity& entity)
			{
				for (int i = 0; i < static_cast<int>(Databases::EntityDb.size()); ++i)
					if (Databases::EntityDb[i].Handle.Equals(entity)) return i;
				return -1;
			}
			void GetEntityThisEntityIsAttachedTo(GTAentity&, GTAentity&) {}
		}
	}
}

#include "PedLabelsProduction.inl"

static void Check(bool condition, const char* message)
{
	if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

int main()
{
#if HAS_ADDON_PED_NAMES
	RegisterAddonPedModelName(2, "customhero", "Custom Hero");
#endif
	using namespace sub::Spooner;
	SpoonerEntity player;
	player.Handle = playerHandle;
	player.HashName = "Michael";
	Databases::EntityDb.push_back(player);
	SelectedEntity = player;
	sub::ChangeModel_(Model(2));
	auto selected = SpoonerMode::GetEntityPtrValue(Databases::EntityDb[0].Handle);
	std::cout << "Database label: [" << Databases::EntityDb[0].HashName << "]; right-click label: [" << selected.HashName << "]\n";
	Check(Databases::EntityDb[0].HashName == "Custom Hero", "Add-on player name is blank in the entity database after changing model.");
	Check(selected.HashName == "Custom Hero", "Right-click selection must retain the add-on name.");
	Check(get_ped_model_label(Model(2), false) == "customhero", "Raw add-on model name must remain available.");
	Check(get_ped_model_label(Model(1), true) == "Michael", "Stock ped captions must be preserved.");
	GTAentity externalPed(99);
	Check(SpoonerMode::GetEntityPtrValue(externalPed).HashName == "Custom Hero", "Right-clicking a ped outside the database must resolve its add-on name.");
	Databases::EntityDb[0].HashName.clear();
	Check(SpoonerMode::GetEntityPtrValue(Databases::EntityDb[0].Handle).HashName == "Custom Hero", "An existing blank database label must be repaired.");
	Databases::EntityDb[0].HashName = "0x2";
	EntityManagement::RestorePedName(Databases::EntityDb[0]);
	Check(Databases::EntityDb[0].HashName == "Custom Hero", "A fallback hash must recover the discovered name.");
	Databases::EntityDb[0].HashName = "My Hero";
	EntityManagement::RestorePedName(Databases::EntityDb[0]);
	Check(Databases::EntityDb[0].HashName == "My Hero", "User entity names must be preserved.");
	sub::ChangeModel_(Model(3));
	Check(Databases::EntityDb[0].HashName == "0x3", "An undiscovered model must retain its fallback instead of a blank label.");
#if HAS_ADDON_PED_NAMES
	RegisterAddonPedModelName(4, "customvillain", "");
	Check(get_ped_model_label(Model(4), true) == "customvillain", "A missing caption must use the installed model name.");
#endif
	std::cout << "Ped database, right-click selection and model label checks passed.\n";
}
