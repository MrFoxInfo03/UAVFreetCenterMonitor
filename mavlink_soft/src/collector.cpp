#include <modules/nats_conn.h>

#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/telemetry/telemetry.h>

#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>
#include <chrono>
#include <vector>
#include <map>
#include <mutex>
#include <deque>
#include <sstream>
#include <iomanip>
#include <cmath>

using namespace mavsdk;
using namespace std::chrono;

std::atomic<bool> running(true);
std::mutex data_mutex;

struct DroneTelemetry {
    Telemetry::Position position {0, 0, 0, 0};
    Telemetry::Battery battery{0, 0, false, false, 0, 0};
    Telemetry::FlightMode flight_mode{Telemetry::FlightMode::Unknown};
    Telemetry::VelocityNed velocity{0, 0, 0};
    std::deque<Telemetry::Position> flight_path; //Flight history
};

std::map<uint64_t, DroneTelemetry> drones_data;
NatsConnection nats; //NATS Connection

void signal_handler(int signum)
{
    running = false;
}

//Convert Flight mode to String
std::string flight_mode_to_string(Telemetry::FlightMode mode)
{
    switch(mode) {
        case Telemetry::FlightMode::Ready: return "Ready";
        case Telemetry::FlightMode::Takeoff: return "Takeoff";
        case Telemetry::FlightMode::Hold: return "Hold";
        case Telemetry::FlightMode::Mission: return "Mission";
        case Telemetry::FlightMode::ReturnToLaunch: return "RTL";
        case Telemetry::FlightMode::Land: return "Land";
        case Telemetry::FlightMode::Offboard: return "Offboard";
        case Telemetry::FlightMode::FollowMe: return "FollowMe";
        default: return "Unknown";
    }
}

//Serialization telemetry to JSON
std::string telemetry_to_json(uint64_t uuid, const DroneTelemetry& data)
{
    double speed = std::sqrt(
        data.velocity.north_m_s * data.velocity.north_m_s +
        data.velocity.east_m_s * data.velocity.east_m_s + 
        data.velocity.down_m_s * data.velocity.down_m_s
    );

    std::ostringstream json;
    json << std::fixed << std::setprecision(7);
    json << "{"
         << "\"uuid\":" << uuid << ","
         << "\"position\":{"
         << "\"lat\":" << data.position.latitude_deg << ","
         << "\"lon\":" << data.position.longitude_deg << ","
         << "\"alt\":" << data.position.absolute_altitude_m
         << "},"
         << "\"battery\": {"
         << "\"percent\":" << (data.battery.remaining_percent * 100)
         << "},"
         << "\"flight_mode\":\"" << flight_mode_to_string(data.flight_mode) << "\","
         << "\"speed\":" << speed << ","
         << "\"path_points\":" << data.flight_path.size()
         << "}";
         
         return json.str();
}

//Publish to NATS
void publish_to_nats(uint64_t uuid, const DroneTelemetry& data)
{
    std::string subject = "drones.telemetry." + std::to_string(uuid);
    std::string json = telemetry_to_json(uuid, data);
    nats.Publish(subject, json);
}

//Callback for published on telemetry
void on_position_update(uint64_t uuid, Telemetry::Position pos)
{
    std::lock_guard<std::mutex> lock(data_mutex);
    drones_data[uuid].position = pos;
    drones_data[uuid].flight_path.push_back(pos);

    if (drones_data[uuid].flight_path.size() > 10000) {
        drones_data[uuid].flight_path.pop_front();
    }
    
    publish_to_nats(uuid, drones_data[uuid]);
}

void on_battery_update(uint64_t uuid, Telemetry::Battery bat)
{
    std::lock_guard<std::mutex> lock(data_mutex);
    drones_data[uuid].battery = bat;
    publish_to_nats(uuid, drones_data[uuid]);
}

void on_flight_mode_update(uint64_t uuid, Telemetry::FlightMode mode)
{
    std::lock_guard<std::mutex> lock(data_mutex);
    drones_data[uuid].flight_mode = mode;
    publish_to_nats(uuid, drones_data[uuid]);
}

void on_velocity_update(uint64_t uuid, Telemetry::VelocityNed vel)
{
    std::lock_guard<std::mutex> lock(data_mutex);
    drones_data[uuid].velocity = vel;
    publish_to_nats(uuid, drones_data[uuid]);
}

void telemetry_collector(Mavsdk& mavsdk)
{
    std::cout << "Waiting for drones to connect...\n";

    while(mavsdk.systems().empty() && running) {
        std::this_thread::sleep_for(milliseconds(100));
    }

    if(!running) return;

    std::cout << "Found " << mavsdk.systems().size() << " drone(s). Subscribing...\n";

    std::vector<std::shared_ptr<Telemetry>> telemetry_plugins;

    for(auto system : mavsdk.systems()) {
        uint64_t uuid = reinterpret_cast<uint64_t>(system.get());

        auto telemetry = std::make_shared<Telemetry>(system);
        telemetry_plugins.push_back(telemetry);

        telemetry->subscribe_position([uuid](Telemetry::Position pos) {
            on_position_update(uuid, pos);
        });

        telemetry->subscribe_battery([uuid](Telemetry::Battery bat) {
            on_battery_update(uuid, bat);
        });

        telemetry->subscribe_flight_mode([uuid](Telemetry::FlightMode mode) {
            on_flight_mode_update(uuid, mode);
        });

        telemetry->subscribe_velocity_ned([uuid](Telemetry::VelocityNed vel) {
            on_velocity_update(uuid, vel);
        });

        std::cout << "Subscribed to drone " << uuid << "\n";
    }

    while(running) {
        std::this_thread::sleep_for(seconds(1));
    }

    std::cout << "\nCollector thread stopped.\n";
}

int main()
{
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    if(!nats.Connect()) {
        std::cerr << "Failed to connect to NATS. Exiting.\n";
        return 1;
    }

    Mavsdk mavsdk{Mavsdk::Configuration{ComponentType::GroundStation}};

    //Connection to a port MAVLink system
    auto conn_result = mavsdk.add_any_connection("udp://:14540");

    if (conn_result != ConnectionResult::Success) {
        std::cerr << "Connection failed: " << conn_result << '\n';
        return 1;
    }
    std::cout << "Connected. Press Ctrl+C to exit.\n";

    std::thread t([&mavsdk]() {
        telemetry_collector(mavsdk);
    });

    t.join();

    std::cout << "Shutting down...\n";
    return 0;
}