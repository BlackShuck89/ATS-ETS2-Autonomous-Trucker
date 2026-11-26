#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

#include "scssdk.h"
#include "scssdk_telemetry.h"
#include "common/scssdk_telemetry_truck_common_channels.h"

namespace {

std::ofstream capture;
std::mutex capture_mutex;
std::string active_game_id;

std::string json_escape(const std::string& input) {
    std::ostringstream out;
    for (const unsigned char ch : input) {
        switch (ch) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (ch < 0x20) {
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<unsigned>(ch) << std::dec;
            } else {
                out << static_cast<char>(ch);
            }
        }
    }
    return out.str();
}

std::string utc_now() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &time);
#else
    gmtime_r(&time, &utc);
#endif
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count() % 1000;
    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << millis << 'Z';
    return out.str();
}

std::filesystem::path capture_directory() {
    if (const char* override_dir = std::getenv("ATS_ETS2_PROBE_DIR")) {
        if (*override_dir) return std::filesystem::path(override_dir);
    }
#ifdef _WIN32
    if (const char* profile = std::getenv("USERPROFILE")) {
        if (*profile) {
            return std::filesystem::path(profile) /
                "Documents" / "ATS-ETS2-Autonomous-Trucker" / "captures";
        }
    }
#endif
    return std::filesystem::current_path() / "autonomous-trucker-captures";
}

std::string value_type_name(const scs_value_type_t type) {
    switch (type) {
    case SCS_VALUE_TYPE_bool: return "bool";
    case SCS_VALUE_TYPE_s32: return "s32";
    case SCS_VALUE_TYPE_u32: return "u32";
    case SCS_VALUE_TYPE_u64: return "u64";
    case SCS_VALUE_TYPE_float: return "float";
    case SCS_VALUE_TYPE_double: return "double";
    case SCS_VALUE_TYPE_fvector: return "fvector";
    case SCS_VALUE_TYPE_dvector: return "dvector";
    case SCS_VALUE_TYPE_euler: return "euler";
    case SCS_VALUE_TYPE_fplacement: return "fplacement";
    case SCS_VALUE_TYPE_dplacement: return "dplacement";
    case SCS_VALUE_TYPE_string: return "string";
    default: return "unknown";
    }
}

void write_value(std::ostream& out, const scs_value_t& value) {
    switch (value.type) {
    case SCS_VALUE_TYPE_bool:
        out << (value.value_bool.value ? "true" : "false");
        break;
    case SCS_VALUE_TYPE_s32:
        out << value.value_s32.value;
        break;
    case SCS_VALUE_TYPE_u32:
        out << value.value_u32.value;
        break;
    case SCS_VALUE_TYPE_u64:
        out << value.value_u64.value;
        break;
    case SCS_VALUE_TYPE_float:
        out << std::setprecision(9) << value.value_float.value;
        break;
    case SCS_VALUE_TYPE_double:
        out << std::setprecision(12) << value.value_double.value;
        break;
    case SCS_VALUE_TYPE_string:
        out << '"' << json_escape(value.value_string.value ? value.value_string.value : "") << '"';
        break;
    case SCS_VALUE_TYPE_fvector:
        out << "{\"x\":" << value.value_fvector.x
            << ",\"y\":" << value.value_fvector.y
            << ",\"z\":" << value.value_fvector.z << '}';
        break;
    case SCS_VALUE_TYPE_dvector:
        out << "{\"x\":" << value.value_dvector.x
            << ",\"y\":" << value.value_dvector.y
            << ",\"z\":" << value.value_dvector.z << '}';
        break;
    case SCS_VALUE_TYPE_euler:
        out << "{\"heading\":" << value.value_euler.heading
            << ",\"pitch\":" << value.value_euler.pitch
            << ",\"roll\":" << value.value_euler.roll << '}';
        break;
    case SCS_VALUE_TYPE_fplacement:
        out << "{\"position\":{\"x\":" << value.value_fplacement.position.x
            << ",\"y\":" << value.value_fplacement.position.y
            << ",\"z\":" << value.value_fplacement.position.z
            << "},\"orientation\":{\"heading\":" << value.value_fplacement.orientation.heading
            << ",\"pitch\":" << value.value_fplacement.orientation.pitch
            << ",\"roll\":" << value.value_fplacement.orientation.roll << "}}";
        break;
    case SCS_VALUE_TYPE_dplacement:
        out << "{\"position\":{\"x\":" << value.value_dplacement.position.x
            << ",\"y\":" << value.value_dplacement.position.y
            << ",\"z\":" << value.value_dplacement.position.z
            << "},\"orientation\":{\"heading\":" << value.value_dplacement.orientation.heading
            << ",\"pitch\":" << value.value_dplacement.orientation.pitch
            << ",\"roll\":" << value.value_dplacement.orientation.roll << "}}";
        break;
    default:
        out << "null";
        break;
    }
}

SCSAPI_VOID channel_callback(
    const scs_string_t name,
    const scs_u32_t index,
    const scs_value_t* const value,
    const scs_context_t) {
    if (!name || !value) return;
    std::lock_guard<std::mutex> lock(capture_mutex);
    if (!capture.is_open()) return;

    capture << "{\"kind\":\"telemetry\",\"utc\":\"" << utc_now()
            << "\",\"game_id\":\"" << json_escape(active_game_id)
            << "\",\"channel\":\"" << json_escape(name)
            << "\",\"index\":";
    if (index == SCS_U32_NIL) capture << "null";
    else capture << index;
    capture << ",\"type\":\"" << value_type_name(value->type) << "\",\"value\":";
    write_value(capture, *value);
    capture << "}\n";
}

bool register_channel(
    const scs_telemetry_register_for_channel_t register_for_channel,
    const scs_string_t name,
    const scs_value_type_t type) {
    if (!register_for_channel || !name) return false;
    const scs_result_t result = register_for_channel(
        name,
        SCS_U32_NIL,
        type,
        SCS_TELEMETRY_CHANNEL_FLAG_none,
        channel_callback,
        nullptr);
    // Individual channels may be unavailable in a particular game/API version.
    return result == SCS_RESULT_ok ||
           result == SCS_RESULT_not_found ||
           result == SCS_RESULT_unsupported_type;
}

template <typename T>
void register_typed(
    const scs_telemetry_register_for_channel_t register_for_channel,
    const scs_string_t name,
    const scs_value_type_t type) {
    register_channel(register_for_channel, name, type);
}

} // namespace

extern "C" __declspec(dllexport) SCSAPI_RESULT scs_telemetry_init(
    const scs_u32_t version,
    const scs_telemetry_init_params_t* const params) {
    if (SCS_GET_MAJOR_VERSION(version) != 1) return SCS_RESULT_unsupported;
    if (!params) return SCS_RESULT_invalid_parameter;

    const auto* init = reinterpret_cast<const scs_telemetry_init_params_v100_t*>(params);
    active_game_id = init->common.game_id ? init->common.game_id : "unknown";

    try {
        const auto directory = capture_directory();
        std::filesystem::create_directories(directory);
        const auto filename = active_game_id + "-" +
            std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) +
            ".jsonl";
        capture.open(directory / filename, std::ios::out | std::ios::app);
    } catch (...) {
        return SCS_RESULT_generic_error;
    }
    if (!capture.is_open()) return SCS_RESULT_generic_error;

    capture << "{\"kind\":\"session\",\"game_id\":\"" << json_escape(active_game_id)
            << "\",\"game_name\":\""
            << json_escape(init->common.game_name ? init->common.game_name : "")
            << "\",\"sdk_api_version\":" << version
            << ",\"note\":\"Read-only telemetry capture; no controls are sent.\"}\n";

    const auto reg = init->register_for_channel;
    register_typed<void>(reg, "game.time", SCS_VALUE_TYPE_u32);
    register_typed<void>(reg, SCS_TELEMETRY_TRUCK_CHANNEL_world_placement, SCS_VALUE_TYPE_dplacement);
    register_typed<void>(reg, SCS_TELEMETRY_TRUCK_CHANNEL_speed, SCS_VALUE_TYPE_float);
    register_typed<void>(reg, SCS_TELEMETRY_TRUCK_CHANNEL_input_steering, SCS_VALUE_TYPE_float);
    register_typed<void>(reg, SCS_TELEMETRY_TRUCK_CHANNEL_input_throttle, SCS_VALUE_TYPE_float);
    register_typed<void>(reg, SCS_TELEMETRY_TRUCK_CHANNEL_input_brake, SCS_VALUE_TYPE_float);
    register_typed<void>(reg, "truck.navigation.distance", SCS_VALUE_TYPE_float);
    register_typed<void>(reg, "truck.navigation.time", SCS_VALUE_TYPE_float);
    register_typed<void>(reg, "truck.navigation.speed.limit", SCS_VALUE_TYPE_float);
    register_typed<void>(reg, SCS_TELEMETRY_TRUCK_CHANNEL_cruise_control, SCS_VALUE_TYPE_float);
    register_typed<void>(reg, SCS_TELEMETRY_TRUCK_CHANNEL_wipers, SCS_VALUE_TYPE_bool);
    register_typed<void>(reg, SCS_TELEMETRY_TRUCK_CHANNEL_lblinker, SCS_VALUE_TYPE_bool);
    register_typed<void>(reg, SCS_TELEMETRY_TRUCK_CHANNEL_rblinker, SCS_VALUE_TYPE_bool);
    register_typed<void>(reg, SCS_TELEMETRY_TRUCK_CHANNEL_light_parking, SCS_VALUE_TYPE_bool);
    register_typed<void>(reg, SCS_TELEMETRY_TRUCK_CHANNEL_light_low_beam, SCS_VALUE_TYPE_bool);
    register_typed<void>(reg, SCS_TELEMETRY_TRUCK_CHANNEL_light_high_beam, SCS_VALUE_TYPE_bool);
    register_typed<void>(reg, "truck.light.hazard", SCS_VALUE_TYPE_bool);

    capture.flush();
    return SCS_RESULT_ok;
}

extern "C" __declspec(dllexport) SCSAPI_VOID scs_telemetry_shutdown() {
    std::lock_guard<std::mutex> lock(capture_mutex);
    if (capture.is_open()) {
        capture.flush();
        capture.close();
    }
}
