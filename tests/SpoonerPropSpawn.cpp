#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>
#include <pugixml/src/pugixml.hpp>

using DWORD = unsigned long;
using Hash = DWORD;
constexpr Hash ringHash = 0x74cb5845;
constexpr int GTA_MAX_ENTITIES = 100;
// Replay the reported rejection with deterministic native responses. The recovery
// case simulates MP map registration succeeding; it verifies the real spawn flow,
// not whether the installed game supplies an object archetype for this drawable.
static std::uint64_t tick, mapsReadyAt, modelReadyAt;
static bool mapsPending, mapsMounted, mapsMissing, creationFails, cameraActive, online, knownLabel;
static Hash requestedHash;
static int mpLoads, waits, creations, invalidCreations, invalidActions, dimensionsBeforeMount, releases;
static bool prioritySet;
bool NETWORK_IS_IN_SESSION() { return online; }
static std::string lastError;
static const std::string ringName = "apa_mp_apa_icon_mk_ring";
std::uint64_t GetTickMs() { return tick; }
bool IS_MODEL_IN_CDIMAGE(Hash hash) { return hash == 1 || (hash == ringHash && mapsMounted); }
bool HAS_MODEL_LOADED(Hash hash) { return requestedHash == hash && tick >= modelReadyAt; }
void REQUEST_MODEL(Hash hash) { requestedHash = hash; }
void SET_MODEL_AS_NO_LONGER_NEEDED(Hash) { requestedHash = 0; ++releases; }
void SET_INSTANCE_PRIORITY_MODE(bool enabled) { prioritySet = enabled; }
void ON_ENTER_MP() { ++mpLoads; mapsPending = prioritySet; }
void WAIT(int)
{
    tick += 16;
    ++waits;
    if (mapsPending && !mapsMissing && tick >= mapsReadyAt) mapsMounted = true;
}
Hash GET_HASH_KEY(const std::string& name)
{
    std::uint32_t hash = 0;
    for (unsigned char c : name) {
        if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        hash += c; hash += hash << 10; hash ^= hash >> 6;
    }
    hash += hash << 3; hash ^= hash >> 11; hash += hash << 15;
    return hash;
}
struct Vector3 { float x = 0, y = 0, z = 0; Vector3(float a = 0, float b = 0, float c = 0) : x(a), y(b), z(c) {} };
struct Vector2 { Vector2(float, float) {} };
namespace GTAmodel {
    struct ModelDimensions { Vector3 Dim1, Dim2; };
    class Model {
    public:
        Hash hash;
        Model(Hash value) : hash(value) {}
        bool IsInCdImage() const;
        bool Load(DWORD timeout) const;
        void Unload() const;
        void LoadCollision(int) const {}
        Vector3 Dim1() const { return {}; }
        ModelDimensions Dimensions() const {
            if (!IS_MODEL_IN_CDIMAGE(hash)) ++dimensionsBeforeMount;
            return {};
        }
    };
}
using namespace GTAmodel;
class GTAentity {
    int value;
    void Action() const { if (!value) ++invalidActions; }
public:
    GTAentity(int handle = 0) : value(handle) {}
    int Handle() const { return value; }
    bool Exists() const { return value != 0; }
    Vector3 Dim1() const { return {}; }
    Vector3 GetOffsetInWorldCoords(float, float, float) const { return {}; }
    Vector3 Rotation_get() const { return {}; }
    bool IsInAir() const { return false; }
    void Position_set(Vector3) const { Action(); }
    void Rotation_set(Vector3) const { Action(); }
    void PlaceOnGround() const { Action(); }
    void Dynamic_set(bool) const { Action(); }
    void FreezePosition(bool) const { Action(); }
    void LodDistance_set(int) const { Action(); }
    void MissionEntity_set(bool) const { Action(); }
    void SetInvincible(bool) const { Action(); }
    void SetExplosionProof(bool) const { Action(); }
    void SetMeleeProof(bool) const { Action(); }
    void IsCollisionEnabled_set(bool) const { Action(); }
    int Type() const { return 3; }
};
using GTAprop = GTAentity;
class GTAped : public GTAentity {
public:
    using GTAentity::GTAentity;
    bool IsInVehicle() const { return false; }
    GTAentity CurrentVehicle() const { return 1; }
};
int PLAYER_PED_ID() { return 1; }
int CREATE_OBJECT(Hash hash, float, float, float, int, int, bool)
{
    if (!IS_MODEL_IN_CDIMAGE(hash) || !HAS_MODEL_LOADED(hash)) { ++invalidCreations; return 0; }
    ++creations;
    return creationFails ? 0 : 42;
}
void PLACE_OBJECT_ON_GROUND_PROPERLY(int handle) { if (!handle) ++invalidActions; }
int OBJ_TO_NET(int handle) { if (!handle) ++invalidActions; return handle; }
void SET_NETWORK_ID_CAN_MIGRATE(int handle, bool) { if (!handle) ++invalidActions; }
namespace Game::Print {
    void PrintBottomLeft(const std::string& text) { lastError = text; }
    void PrintBottomCentre(const std::string& text) { lastError = text; }
}
std::string get_prop_model_label(const Model& model) { return model.hash == ringHash && knownLabel ? ringName : ""; }
std::string int_to_hexstring(Hash, bool) { return "0x0"; }
namespace World {
    float GetGroundHeight(Vector3) { return 0; }
    GTAprop CreateProp(Model, Vector3, bool, bool);
    GTAprop CreateProp(Model, Vector3, const Vector3&, bool, bool);
}
namespace sub::Spooner {
    enum class EntityType { PROP = 3 };
    struct SpoonerEntity { GTAentity Handle; EntityType Type{}; std::string HashName; bool Dynamic = false; };
    namespace Databases { std::vector<SpoonerEntity> EntityDb; }
    namespace Settings { bool bSpawnDynamicProps = false, bSpawnInvincibleEntities = false; }
    namespace SpoonerMode {
        struct Camera {
            bool IsActive() const { return cameraActive; }
            Vector3 RaycastForCoord(Vector2, int, float, float) const { return {}; }
            Vector3 Rotation_get() const { return {}; }
        } spoonerModeCamera;
    }
    namespace EntityManagement {
        void AddEntityToDb(const SpoonerEntity& entity) { if (entity.Handle.Exists()) Databases::EntityDb.push_back(entity); }
    }
}
#include "SpoonerPropSpawnProduction.inl"

static void Check(bool condition, const char* message)
{
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
static void Reset()
{
    tick = 1000; mapsReadyAt = 1064; modelReadyAt = 1128;
    mapsPending = mapsMounted = mapsMissing = creationFails = cameraActive = prioritySet = online = false;
    knownLabel = true;
    requestedHash = 0;
    mpLoads = waits = creations = invalidCreations = invalidActions = dimensionsBeforeMount = releases = 0;
    lastError.clear();
    sub::Spooner::Databases::EntityDb.clear();
}
int main()
{
    using namespace sub::Spooner;
    pugi::xml_document doc;
    Check(bool(doc.load_string("<PropModel modelName=\"apa_mp_apa_icon_mk_ring\" modelHash=\"0x74cb5845\" />")), "user XML parses");
    auto node = doc.child("PropModel");
    Model model(node.attribute("modelHash").as_uint());
    Check(model.hash == ringHash && model.hash == GET_HASH_KEY(node.attribute("modelName").as_string()), "saved name and hexadecimal hash agree");
    Reset();
    auto entity = EntityManagement::AddProp(model, node.attribute("modelName").as_string(), true);
    std::cout << "User model: handle=" << entity.Handle.Handle() << ", MP loads=" << mpLoads << ", error=" << lastError << '\n';
    Check(entity.Handle.Exists(), "the unavailable MP ring must be retried after loading MP content");
    Check(lastError.empty() && creations == 1 && Databases::EntityDb.size() == 1, "recovered object spawns and enters Spooner without an error");
    Check(dimensionsBeforeMount == 0 && invalidCreations == 0 && invalidActions == 0, "model and entity operations wait for successful loading and creation");
    Check(releases == 1, "spawn releases the model once after object setup");

    Reset();
    knownLabel = false;
    cameraActive = true;
    entity = EntityManagement::AddProp(model, ringName, false);
    Check(entity.Handle.Exists() && mpLoads == 1, "a custom favourite can recover without a PropList entry in camera mode");
    Check(releases == 0 && HAS_MODEL_LOADED(ringHash), "unloadModel=false keeps the requested model loaded");

    Reset();
    entity = EntityManagement::AddProp(model, "", true);
    Check(entity.Handle.Exists(), "a hash-only input resolves the name through the model list");

    Reset();
    mapsMounted = true;
    requestedHash = ringHash;
    modelReadyAt = 0;
    Check(EntityManagement::AddProp(model, ringName, true).Handle.Exists(), "an already-loaded MP object spawns normally");
    Check(mpLoads == 0 && waits == 0, "an available MP model does not reload maps or wait");

    Reset();
    Check(EntityManagement::AddProp(Model(1), "stock_prop", true).Handle.Exists(), "a normal Story Mode prop still spawns");
    Check(mpLoads == 0 && invalidCreations == 0, "normal props do not change the map mode");

    Reset();
    Check(!EntityManagement::AddProp(Model(0), "not_a_model", true).Handle.Exists(), "an unknown model still fails");
    Check(mpLoads == 0 && creations == 0 && waits == 0, "unknown names do not load maps or request invalid models");
    Check(lastError.find("unavailable") != std::string::npos && lastError.find("NO SUGGESTED LOCATIONS") == std::string::npos, "unavailable models get a clear error without an empty location hint");

    Reset();
    Check(!EntityManagement::AddProp(Model(123), ringName, true).Handle.Exists(), "a mismatched saved hash still fails");
    Check(mpLoads == 0 && waits == 0, "a mismatched name and hash does not change maps");

    Reset();
    mapsMissing = true;
    Check(!EntityManagement::AddProp(model, ringName, true).Handle.Exists(), "an MP model still missing after mounting fails safely");
    Check(mpLoads == 1 && tick >= 4000 && tick <= 4016, "map registration retry has a three-second limit");
    Check(creations == 0 && invalidActions == 0 && Databases::EntityDb.empty(), "missing DLC does not create or register an empty entity");

    Reset();
    online = true;
    Check(!EntityManagement::AddProp(model, ringName, true).Handle.Exists(), "unavailable online model still fails");
    Check(mpLoads == 0 && waits == 0, "recovery does not switch maps in an online session");

    Reset();
    modelReadyAt = 100000;
    Check(!EntityManagement::AddProp(Model(1), "stock_prop", true).Handle.Exists(), "model load timeout returns failure");
    Check(tick >= 4000 && tick <= 4016 && creations == 0 && invalidActions == 0, "timeout stops before creation and object setup");
    Check(releases == 1 && requestedHash == 0 && lastError.find("load in time") != std::string::npos, "timeout releases the pending request and reports the cause");

    Reset();
    creationFails = true;
    Check(!EntityManagement::AddProp(Model(1), "stock_prop", true).Handle.Exists(), "native object creation failure is reported");
    Check(invalidActions == 0 && releases == 1 && Databases::EntityDb.empty(), "creation failure stops object operations and releases the model");
    Check(lastError.find("Unable to create") != std::string::npos, "creation failure has its own error");

    Reset();
    Databases::EntityDb.resize(GTA_MAX_ENTITIES);
    Check(!EntityManagement::AddProp(model, ringName, true).Handle.Exists(), "the Spooner entity limit still applies");
    Check(mpLoads == 0 && waits == 0 && requestedHash == 0, "the entity limit is checked before map and model loading");

    Reset();
    modelReadyAt = 100000;
    Check(!World::CreateProp(Model(1), {}, false, false).Exists(), "direct prop creation stops on streaming timeout");
    Check(creations == 0 && invalidCreations == 0, "direct callers do not create an unloaded model");

    Reset();
    creationFails = true;
    Check(!World::CreateProp(Model(1), {}, Vector3(), false, true).Exists(), "direct rotated prop creation returns an empty handle on failure");
    Check(invalidActions == 0, "direct callers do not position or rotate an empty handle");
    std::cout << "MP recovery, favourites, stock props, missing models, online sessions, timeouts, creation failures, and entity limit checks passed.\n";
}
