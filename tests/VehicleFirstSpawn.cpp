#include <cstdint>
#include <cstdlib>
#include <iostream>

using DWORD = unsigned long;
using Hash = DWORD;
using Vehicle = int;
using Ped = int;
using INT = int;
constexpr int SEAT_DRIVER = -1;
struct Vector3 { float x = 0, y = 0, z = 0; };

// Streaming completes only when the script yields, as it does in the game.
static std::uint64_t tick;
static std::uint64_t readyAt;
static bool requested;
static bool loaded;
static int requests;
static int waits;
static int creations;
static int invalidCreations;
static int deletions;
static bool inVehicle;
static bool creationFails;
std::uint64_t GetTickMs() { return tick; }
bool HAS_MODEL_LOADED(Hash) { return loaded; }
void REQUEST_MODEL(Hash) { requested = true; ++requests; }
void WAIT(int milliseconds)
{
    tick += milliseconds > 0 ? milliseconds : 16;
    ++waits;
    if (requested && tick >= readyAt) loaded = true;
}
void SET_MODEL_AS_NO_LONGER_NEEDED(Hash) { requested = false; }

namespace GTAmodel {
    class Model {
    public:
        Hash hash;
        Model(Hash value) : hash(value) {}
        void Load() const;
        bool Load(DWORD timeOut) const;
        void LoadAndWait() const;
        void Unload() const;
        Vector3 Dim1() const { return {}; }
        Vector3 Dim2() const { return {}; }
    };
}
using GTAmodel::Model;
class GTAentity {
    int handle;
public:
    GTAentity(int value) : handle(value) {}
    operator int() const { return handle; }
    int Handle() const { return handle; }
    int GetHandle() const { return handle; }
    bool RequestControl() const { return true; }
};
class GTAvehicle : public GTAentity {
public:
    GTAvehicle(int value) : GTAentity(value) {}
    int FirstFreeSeat(int seat) const { return seat; }
    void RadioStation_set(int) {}
    void CloseAllDoors(bool) {}
};
class GTAped : public GTAentity {
public:
    GTAped(int value) : GTAentity(value) {}
    bool IsInVehicle() const { return inVehicle; }
    GTAvehicle CurrentVehicle() const { return 7; }
    Vector3 GetOffsetInWorldCoords(Vector3 value) const { return value; }
    float Heading_get() const { return 0; }
};
namespace PTFX { void trigger_ptfx_1(const char*, const char*, int, Vector3, Vector3, float) {} }
namespace Game { void RequestControlOfId(int) {} }

int GET_PLAYER_RADIO_STATION_INDEX() { return 0; }
Vector3 GET_ENTITY_VELOCITY(int) { return {}; }
bool GET_IS_VEHICLE_ENGINE_RUNNING(int) { return true; }
Hash GET_ENTITY_MODEL(int) { return 1; }
Vector3 GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(int, int, float, int) { return {}; }
int CREATE_VEHICLE(Hash, float, float, float, float, int, int, int)
{
    if (!loaded) { ++invalidCreations; return 0; }
    ++creations;
    return creationFails ? 0 : 42;
}
void SET_ENTITY_COLLISION(int, bool, bool) {}
void SET_ENTITY_ALPHA(int, int, bool) {}
void SET_VEHICLE_HAS_STRONG_AXLES(int, int) {}
void SET_VEHICLE_DIRT_LEVEL(int, float) {}
void SET_VEHICLE_ENVEFF_SCALE(int, float) {}
void SET_ENTITY_AS_MISSION_ENTITY(int, int, int) {}
int VEH_TO_NET(int value) { return value; }
bool DOES_ENTITY_EXIST(int value) { return value != 0; }
void SET_ENTITY_VELOCITY(int, float, float, float) {}
void FREEZE_ENTITY_POSITION(int, bool) {}
void SET_VEHICLE_ENGINE_ON(int, bool, bool, int) {}
void RESET_ENTITY_ALPHA(int) {}
int GET_VEHICLE_MAX_NUMBER_OF_PASSENGERS(int) { return 0; }
int GET_PED_IN_VEHICLE_SEAT(int, int, int) { return 0; }
void SET_PED_INTO_VEHICLE(int, int, int) {}
void TASK_LEAVE_ANY_VEHICLE(int, int, int) {}
void CLEAR_PED_TASKS_IMMEDIATELY(int) {}
void SET_ENTITY_COORDS(int, float, float, float, int, int, int, int) {}
void DELETE_VEHICLE(int* value) { ++deletions; *value = 0; }
void SET_VEHICLE_IS_STOLEN(int, bool) {}

#include "VehicleFirstSpawnProduction.inl"

static void Check(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}
static void Reset(std::uint64_t delay)
{
    tick = 1000;
    readyAt = tick + delay;
    requested = loaded = inVehicle = creationFails = false;
    requests = waits = creations = invalidCreations = deletions = 0;
}

int main()
{
    Model model(1);
    Reset(64);
    model.Load(); // Same request made by the saved-vehicle XML loader.
    const int firstVehicle = FuncSpawnVehicle_(model, 1, true, true);
    std::cout << "First attempt: streaming waits=" << waits << ", vehicle creations=" << creations << '\n';
    WAIT(100); // The saved-vehicle loader yields during customization.
    const int secondVehicle = FuncSpawnVehicle_(model, 1, true, true);
    std::cout << "Saved vehicle attempts: first=" << firstVehicle << ", second=" << secondVehicle << '\n';
    Check(firstVehicle != 0, "saved vehicle must spawn on the first attempt with an unloaded model");
    Check(secondVehicle != 0, "repeat spawn still works");
    Check(creations == 2 && invalidCreations == 0, "each attempt creates one vehicle after loading");

    Reset(64);
    Check(FuncSpawnVehicle_(model, 1, false, false) != 0, "cold spawn also works without a prior model request");

    Reset(0);
    loaded = true;
    Check(FuncSpawnVehicle_(model, 1, false, false) != 0, "already-loaded vehicle spawns");
    Check(waits == 0, "already-loaded model does not delay spawning");

    Reset(10000);
    inVehicle = true;
    Check(FuncSpawnVehicle_(model, 1, true, true) == 0, "unavailable model returns failure");
    Check(tick >= 4000 && tick <= 4016, "spawn stops at its three-second loading timeout");
    Check(creations == 0 && deletions == 0, "timeout keeps the existing vehicle");

    Reset(0);
    loaded = inVehicle = creationFails = true;
    Check(FuncSpawnVehicle_(model, 1, true, true) == 0, "creation failure returns no vehicle");
    Check(deletions == 0 && waits == 0, "creation failure keeps the existing vehicle");

    Reset(32);
    Check(!model.Load(0), "zero timeout makes a non-blocking request");
    Check(requests == 1 && waits == 0, "zero timeout does not yield");

    Reset(32);
    model.LoadAndWait();
    Check(loaded, "LoadAndWait waits for streaming to finish");
    std::cout << "Cold spawn, warm spawn, timeout, zero-timeout, and LoadAndWait checks passed.\n";
}
