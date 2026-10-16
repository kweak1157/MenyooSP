#include <cmath>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>
#include <pugixml/src/pugixml.hpp>
#include "../Solution/source/Submenus/Spooner/STSTask.h"
#include "../Solution/source/Submenus/Spooner/Submenus_TaskSequence.h"
#include "../Solution/source/Submenus/Spooner/SpoonerTaskSequence.h"
using Vehicle = int;

struct Vector3 {
    float x, y, z;
    Vector3(float a = 0, float b = 0, float c = 0) : x(a), y(b), z(c) {}
    float DistanceTo(const Vector3& b) const { return std::sqrt((x-b.x)*(x-b.x) + (y-b.y)*(y-b.y) + (z-b.z)*(z-b.z)); }
};
static Vector3 vehiclePosition;
static int parkingActions, immediateStops;
struct Model {
    int kind = 0;
    bool IsHeli() const { return kind == 1; }
    bool IsPlane() const { return kind == 2; }
    bool IsBoat() const { return kind == 3; }
};
struct GTAentity { int value; int Handle() const { return value; } };
struct GTAvehicle {
    int value = 20, driver = 10;
    ::Model model;
    bool Exists() const { return value != 0; }
    int Handle() const { return value; }
    GTAentity Driver_get() const { return {driver}; }
    const ::Model& Model() const { return model; }
    Vector3 Position_get() const { return vehiclePosition; }
    void StopImmediately() { ++immediateStops; }
};
static int clearedTasks;
static int cruiseChanges;
struct Tasks { void ClearAll() { ++clearedTasks; } };
class GTAped : public GTAentity {
public:
    GTAped() : GTAentity{10} {}
    GTAped(const GTAentity& e) : GTAentity(e) {}
    inline static bool inVehicle = true;
    inline static GTAvehicle vehicle;
    inline static float maxSpeed = 0;
    bool IsInVehicle() const { return inVehicle; }
    GTAvehicle CurrentVehicle() const { return vehicle; }
    void MaxDrivingSpeed_set(float speed) { maxSpeed = speed; }
    void DrivingSpeed_set(float) { ++cruiseChanges; }
    Tasks Task() const { return {}; }
};
namespace DrivingStyle { constexpr int Normal = 786603; }
struct Leg { int kind, vehicle; Vector3 point; float speed; int style; float stopRange; };
static std::vector<Leg> legs;
static std::uint64_t now;
static bool requireContinuousDrive;
std::uint64_t GetTickMs() { return now; }
static void Check(bool result, const char* message) {
    if (!result) { std::cerr << message << '\n'; std::exit(1); }
}
void TASK_VEHICLE_TEMP_ACTION(int ped, int vehicle, int action, int duration) {
    Check(ped == 10 && vehicle == 20 && action == 6 && duration == -1, "Final point must apply an indefinite parking brake");
    ++parkingActions;
}
static void Record(int kind, int ped, int vehicle, float x, float y, float z, float speed, int style = 0, float range = 0) {
    Check(ped == 10, "The current driver must receive the point task directly");
    legs.push_back({kind, vehicle, {x, y, z}, speed, style, range});
}
void TASK_VEHICLE_MISSION_COORS_TARGET(int ped, int vehicle, float x, float y, float z, int mission, float speed, int style, float radius, float, bool) {
    if (requireContinuousDrive) Check(mission == 14, "Moving steering targets must use continuous driving instead of drive-and-stop missions");
    Check(mission == 4 || mission == 14, "Route leg must be a go-to mission");
    Record(0, ped, vehicle, x, y, z, speed, style, radius);
}
void TASK_HELI_MISSION(int ped, int vehicle, int, int, float x, float y, float z, int, float speed, float, float, int, int, int, int) {
    Record(1, ped, vehicle, x, y, z, speed);
}
void TASK_PLANE_MISSION(int ped, int vehicle, int, int, float x, float y, float z, int, float speed, float, float, int, int, bool) {
    Record(2, ped, vehicle, x, y, z, speed);
}
void TASK_BOAT_MISSION(int ped, int vehicle, int, int, float x, float y, float z, int, float speed, int, float, int) {
    Record(3, ped, vehicle, x, y, z, speed);
}
namespace sub::Spooner {
    STSTask::STSTask() : type(STSTaskType::Nothing), submenu(nullptr), duration(1000), durationAfterLife(-2), isLoopedTask(false) {}
    void STSTask::RunP(GTAped&) {}
    void STSTask::TickP(GTAped&) {}
    void STSTask::EndP(GTAped&) {}
    void STSTask::LoadTargetingDressing(Entity, Entity) {}
    void Submenus::Sub_TaskSequence::DriveFollowRoute() {}
    namespace STSTasks {
#include "VehicleRouteDeclaration.inl"
    }
}
#include "VehicleRouteProduction.inl"
using namespace sub::Spooner;
using STSTasks::DriveFollowRoute;
static bool SamePoint(const Vector3& a, const Vector3& b) { return a.DistanceTo(b) == 0; }
static bool OnSegment(const Vector3& point, const Vector3& a, const Vector3& b) {
    const float dx = b.x-a.x, dy = b.y-a.y, dz = b.z-a.z;
    const float length = dx*dx+dy*dy+dz*dz;
    if (length < 0.001f) return point.DistanceTo(b) < 0.001f;
    const float t = ((point.x-a.x)*dx+(point.y-a.y)*dy+(point.z-a.z)*dz)/length;
    return t >= 0 && t <= 1.001f && point.DistanceTo({a.x+dx*t, a.y+dy*t, a.z+dz*t}) < 0.001f;
}
static void ResetRecording() { legs.clear(); clearedTasks = 0; cruiseChanges = 0; parkingActions = 0; immediateStops = 0; vehiclePosition = {}; GTAped::vehicle = {}; GTAped::inVehicle = true; now = 0; }

int main() {
    static_assert(static_cast<int>(STSTaskType::FollowRoute) == 7);
    static_assert(static_cast<int>(STSTaskType::RemoveBlip) == 52);
    static_assert(static_cast<int>(STSTaskType::DriveFollowRoute) == 53);
    ResetRecording();
    DriveFollowRoute task;
    GTAped driver;
    {
        DriveFollowRoute steadyRoute;
        steadyRoute.route = {{1000, 0, 0}};
        steadyRoute.speedInKmph = 36;
        requireContinuousDrive = true;
        steadyRoute.RunP(driver);
        for (int i = 1; i <= 20; ++i) {
            now = std::uint64_t(i * 100);
            vehiclePosition = {float(i), (i % 2 == 0 ? 1.1f : 0.9f), 0};
            steadyRoute.TickP(driver);
            Check(driver.maxSpeed >= 9.5f, "Small normal tracking deviations must not repeatedly cut cruise speed");
        }
        Check(legs.size() <= 7, "Straight-line cruising must not restart the driving task every update");
        Check(cruiseChanges > 0, "Cruise updates must reuse the current driving task");
        requireContinuousDrive = false;
        ResetRecording();
    }
    {
        DriveFollowRoute denseRoute;
        denseRoute.speedInKmph = 72;
        for (int i = 1; i <= 5; ++i) denseRoute.route.emplace_back(float(i * 10), 0.0f, 0.0f);
        denseRoute.RunP(driver);
        Check(driver.maxSpeed > 10 && driver.maxSpeed < 20, "Final-stop planning must look beyond closely spaced straight checkpoints");
        float previousSpeed = driver.maxSpeed;
        for (int i = 1; i <= 90; ++i) {
            now = std::uint64_t(i * 100);
            vehiclePosition = {float(i) * 0.5f, 0, 0};
            denseRoute.TickP(driver);
            Check(driver.maxSpeed <= previousSpeed + 0.01f, "Straight checkpoints must not cause brake/accelerate cycles on the final approach");
            Check(previousSpeed - driver.maxSpeed <= 0.301f, "Braking commands must ramp gradually");
            previousSpeed = driver.maxSpeed;
        }
        Check(parkingActions == 0, "Dense routes must not park before their final point");
        vehiclePosition = {50, 0, 0};
        denseRoute.TickP(driver);
        Check(parkingActions == 1 && denseRoute.StopsSequence(), "Smooth braking must retain final parking");
        ResetRecording();
        DriveFollowRoute gentleRoute;
        gentleRoute.route = {{100, 0, 0}, {200, 25, 0}};
        gentleRoute.speedInKmph = 72;
        gentleRoute.RunP(driver);
        Check(driver.maxSpeed == 20, "Gentle bends must not receive sharp-corner speed cuts");
        ResetRecording();
    }
    Check(task.submenu == Submenus::Sub_TaskSequence::DriveFollowRoute, "Missing route editor");
    Check(!task.isLoopedTask, "Starting the task every frame would reset route progress");
    task.RunP(driver);
    Check(legs.empty(), "Empty route must not issue a driving task");
    for (int i = 0; i < 12; ++i) task.route.emplace_back(float(10 + i * 8), float(i * -3), 0.0f);
    task.route.push_back(task.route.back());
    task.speedInKmph = 36;
    task.drivingStyle = 2883621;
    task.duration = 120000;
    driver.inVehicle = false;
    task.RunP(driver);
    Check(legs.empty(), "Ped on foot must not be assigned driving tasks");
    driver.inVehicle = true;
    driver.vehicle.driver = 99;
    task.RunP(driver);
    Check(legs.empty(), "Passenger must not be assigned driving tasks");
    driver.vehicle.driver = driver.Handle();
    driver.vehicle.value = 0;
    task.RunP(driver);
    Check(legs.empty(), "Missing vehicle must not be used");
    for (int kind = 0; kind < 4; ++kind) {
        ResetRecording();
        driver.vehicle.model.kind = kind;
        task.RunP(driver);
        Check(legs.size() == 1 && OnSegment(legs.back().point, {}, task.route[0]), "Only the first unreached segment may be commanded");
        Check(driver.maxSpeed == 10.0f, "Speed cap must use metres per second");
        for (size_t i = 0; i < 12; ++i) {
            Check(legs.size() == i + 1 && OnSegment(legs.back().point, i == 0 ? Vector3() : task.route[i-1], task.route[i]), "Only the current recorded segment may be followed");
            Check(legs.back().kind == kind && legs.back().vehicle == 20 && legs.back().speed > 0 && legs.back().speed <= 10, "Incorrect vehicle mission or speed");
            if (kind == 0) {
                Check((legs.back().style & 16777216) != 0, "Recorded routes must target the points directly instead of following the road network");
                Check((legs.back().style & (262144 | 4194304 | 1073741824)) == 0, "Road shortcuts must not replace the recorded route");
                Check(legs.back().stopRange < 2, "Native must not stop outside the waypoint arrival radius");
                Check((legs.back().style & 128) == (task.drivingStyle & 128), "User driving preferences must survive");
            }
            // Reaching a nearby road node is not reaching the saved coordinate.
            vehiclePosition = {task.route[i].x, task.route[i].y + 6, task.route[i].z};
            for (int frame = 0; frame < 10; ++frame) task.TickP(driver);
            Check(legs.size() == i + 1, "A missed position must neither advance nor restart the leg");
            vehiclePosition = {task.route[i].x, task.route[i].y, task.route[i].z + 8};
            task.TickP(driver);
            Check(legs.size() == i + 1, "A point on a different elevation must not count as reached");
            vehiclePosition = task.route[i];
            task.TickP(driver);
        }
        Check(legs.size() == 12, "Repeated final position must not restart driving");
        Check(parkingActions == 1 && immediateStops == 1, "Reaching the final point must stop and park the vehicle");
        task.TickP(driver);
        Check(legs.size() == 12, "Completed route must stay complete until the next sequence run");
        task.EndP(driver);
        Check(clearedTasks == 2, "Arrival and explicit end must clear the driving/parking actions");
    }
    // Long segments must recover to their saved line, not diagonally cut toward its end.
    ResetRecording();
    DriveFollowRoute trackingRoute;
    trackingRoute.route = {{100, 0, 0}, {100, 100, 0}};
    trackingRoute.speedInKmph = 72;
    trackingRoute.drivingStyle = 1074528293;
    trackingRoute.RunP(driver);
    Check(legs.back().point.x > 0 && legs.back().point.x <= 25 && legs.back().point.y == 0, "Long segments must use a nearby target on the recorded line");
    Check(legs.back().speed == 20, "Clear straight stretches must respect the requested speed");
    const float straightSpeed = legs.back().speed;
    now = 1000;
    vehiclePosition = {40, 6, 0};
    trackingRoute.TickP(driver);
    Check(legs.size() == 2 && legs.back().point.x >= 40 && legs.back().point.x <= 52 && legs.back().point.y == 0, "Drift must pull the driver back to the original segment promptly");
    Check(legs.back().speed < straightSpeed, "Drift recovery must reduce speed");
    Check((legs.back().style & (4 | 8 | 16 | 32 | 256 | 2048 | 262144 | 524288)) == 0, "Strict tracking must disable swerving, wandering and lane changes");
    Check((legs.back().style & 3) == 3, "Driver should stop for cars and peds instead of taking a detour");
    now = 1001;
    vehiclePosition = {41, 6, 0};
    trackingRoute.TickP(driver);
    Check(legs.size() == 2, "Steering updates must be rate limited");
    now = 1400;
    vehiclePosition = {100, 50, 0};
    trackingRoute.TickP(driver);
    Check(SamePoint(legs.back().point, {100, 0, 0}) && !trackingRoute.StopsSequence(), "Being near a later segment must return to the unreached corner, not skip it");
    now = 1800;
    vehiclePosition = {98, 0, 0};
    trackingRoute.TickP(driver);
    Check(legs.back().point.y == 0 && legs.back().point.x == 100 && driver.maxSpeed < straightSpeed, "Driver must slow before a sharp bend without skipping its recorded corner");
    const auto commandCount = legs.size();
    now = 2000;
    trackingRoute.TickP(driver);
    Check(legs.size() == commandCount, "Identical steering commands must not restart driving repeatedly");
    vehiclePosition = {100, 0, 0};
    trackingRoute.TickP(driver);
    Check(legs.back().point.x == 100 && legs.back().point.y > 0 && legs.back().point.y <= 25, "Reaching a corner must start following the next segment");
    now = 6000;
    vehiclePosition = {100, 97, 0};
    trackingRoute.TickP(driver);
    const auto finalApproachCommands = legs.size();
    for (int i = 0; i < 40; ++i) { now += 100; trackingRoute.TickP(driver); }
    Check(driver.maxSpeed < 3 && parkingActions == 0, "Final approach must brake smoothly while still completing the route");
    Check(legs.size() == finalApproachCommands, "Slowing on the final approach must change cruise speed without restarting the driving mission");
    vehiclePosition = {100, 100, 0};
    trackingRoute.TickP(driver);
    Check(parkingActions == 1 && trackingRoute.StopsSequence(), "Improved tracking must retain final parking");
    // Exercise the real Spooner scheduler, including frame updates and repeat/start.
    ResetRecording();
    SpoonerTaskSequence sequence;
    auto route = std::make_unique<DriveFollowRoute>();
    route->route = {{10, 0, 0}, {10, 20, 0}, {30, 20, 0}};
    sequence.AddTask(std::move(route));
    sequence.Start();
    sequence.Tick(&driver);
    Check(legs.size() == 1 && OnSegment(legs.back().point, {}, {10, 0, 0}), "Scheduler must start the first segment");
    now = 100;
    vehiclePosition = {10, 0, 0};
    sequence.Tick(&driver);
    Check(legs.size() == 2 && OnSegment(legs.back().point, {10, 0, 0}, {10, 20, 0}), "Scheduler must update arrival without restarting RunP");
    now = 200;
    vehiclePosition = {10, 10, 0};
    sequence.Tick(&driver);
    Check(legs.size() == 2 && OnSegment(legs.back().point, {10, 0, 0}, {10, 20, 0}), "Scheduler must keep driving the current segment between steering updates");
    sequence.Reset(false);
    vehiclePosition = {};
    sequence.Start();
    sequence.Tick(&driver);
    Check(legs.size() == 3 && OnSegment(legs.back().point, {}, {10, 0, 0}), "Restarting the sequence must restart the route");
    now = 40201;
    sequence.Tick(&driver);
    Check(clearedTasks == 1, "Allocated duration must end the active driving mission");
    ResetRecording();
    SpoonerTaskSequence parkingSequence;
    auto parkingRoute = std::make_unique<DriveFollowRoute>();
    parkingRoute->route = {{10, 0, 0}, {20, 0, 0}};
    parkingSequence.AddTask(std::move(parkingRoute));
    parkingSequence.Start();
    parkingSequence.Tick(&driver);
    vehiclePosition = {10, 0, 0};
    parkingSequence.Tick(&driver);
    Check(parkingActions == 0, "Intermediate points must not park the vehicle");
    vehiclePosition = {20, 0, 0};
    parkingSequence.Tick(&driver);
    Check(parkingActions == 1 && immediateStops == 1 && clearedTasks == 1, "Final arrival must replace driving with a stationary parking action");
    Check(!parkingSequence.IsActive(), "Completed route must stop its sequence");
    now = 200000;
    for (int frame = 0; frame < 10; ++frame) parkingSequence.Tick(&driver);
    Check(legs.size() == 2 && parkingActions == 1 && clearedTasks == 1, "Expired timers must not repeat the route or release the parking brake");
    parkingSequence.Start();
    parkingSequence.Tick(&driver);
    Check(parkingSequence.IsActive() && legs.size() == 3 && OnSegment(legs.back().point, {20, 0, 0}, {10, 0, 0}) && clearedTasks == 2, "Manual restart must release parking and start the route again");
    ResetRecording();
    SpoonerTaskSequence singlePointSequence;
    auto singlePointRoute = std::make_unique<DriveFollowRoute>();
    singlePointRoute->route = {{0, 0, 0}};
    singlePointSequence.AddTask(std::move(singlePointRoute));
    singlePointSequence.Start();
    singlePointSequence.Tick(&driver);
    Check(!singlePointSequence.IsActive() && legs.empty() && parkingActions == 1 && immediateStops == 1, "A route already at its only point must park immediately");
    // Old saved tasks keep IsLoopedTask=false; the independent frame hook must work.
    pugi::xml_document doc;
    auto node = doc.append_child("Task");
    task.GetXmlNode(node);
    DriveFollowRoute restored;
    restored.route.emplace_back(999.0f, 999.0f, 999.0f);
    restored.ImportXmlNode(node);
    restored.ImportXmlNode(node);
    Check(restored.route.size() == task.route.size(), "Loading must replace the previous route");
    Check(restored.type == task.type && restored.duration == task.duration && restored.speedInKmph == 36 && restored.drivingStyle == task.drivingStyle, "Task settings must survive save and load");
    DriveFollowRoute copied;
    copied.Assign(&restored);
    for (size_t i = 0; i < task.route.size(); ++i) {
        Check(SamePoint(restored.route[i], task.route[i]) && SamePoint(copied.route[i], task.route[i]), "Save/load and copying must preserve route order");
    }
    restored.route.clear();
    Check(copied.route.size() == task.route.size() && copied.speedInKmph == task.speedInKmph && copied.drivingStyle == task.drivingStyle && copied.duration == task.duration, "Copied task must own its route and settings");
    ResetRecording();
    copied.RunP(driver);
    Check(OnSegment(legs.back().point, {}, copied.route[0]), "Copied task must start on its first segment");
    driver.vehicle.value = 21;
    copied.TickP(driver);
    Check(legs.size() == 2 && legs.back().vehicle == 21, "Switching vehicles must reissue the current leg");
    driver.inVehicle = false;
    copied.TickP(driver);
    driver.inVehicle = true;
    copied.TickP(driver);
    Check(legs.size() == 3 && OnSegment(legs.back().point, {}, copied.route[0]), "Resuming driving must retain the unreached segment");
    pugi::xml_document emptyDoc;
    auto emptyNode = emptyDoc.append_child("Task");
    restored.ImportXmlNode(emptyNode);
    Check(restored.route.empty() && restored.speedInKmph == 10 && restored.drivingStyle == DrivingStyle::Normal, "Missing route settings must use defaults");
    std::cout << "Vehicle route checks passed: steady cruising, bounded task restarts, smooth braking across checkpoints, line tracking, final parking and persistence.\n";
}
