#include "simulation_store.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <mutex>
#include <unordered_map>

using json = nlohmann::json;

std::unordered_map<std::string, json> simulations;
int nextId = 1;
std::mutex simulationsMutex;
sqlite3* database = nullptr;

json caseCatalog()
{
    return json::array({
        {
            {"case_id", 1}, {"title", "Chest pain"},
            {"description", "Fictional patient with chest pain and shortness of breath."},
            {"patient", {
                {"symptoms", json::array({
                    {{"name", "Chest pain"}, {"intensity", 8.0}, {"severity", "HIGH"}},
                    {{"name", "Shortness of breath"}, {"intensity", 5.0}, {"severity", "MODERATE"}}
                })},
                {"vital_signs", json::array({
                    {{"name", "Heart rate"}, {"value", 110.0}, {"unit", "bpm"}, {"is_critical", false}},
                    {{"name", "Oxygen saturation"}, {"value", 90.0}, {"unit", "%"}, {"is_critical", false}}
                })}
            }}
        },
        {
            {"case_id", 2}, {"title", "Dehydration"},
            {"description", "Fictional patient with thirst and dizziness."},
            {"patient", {
                {"symptoms", json::array({
                    {{"name", "Thirst"}, {"intensity", 7.0}, {"severity", "HIGH"}},
                    {{"name", "Dizziness"}, {"intensity", 5.0}, {"severity", "MODERATE"}}
                })},
                {"vital_signs", json::array({
                    {{"name", "Heart rate"}, {"value", 105.0}, {"unit", "bpm"}, {"is_critical", false}},
                    {{"name", "Systolic blood pressure"}, {"value", 100.0}, {"unit", "mmHg"}, {"is_critical", false}}
                })}
            }}
        }
    });
}

bool saveSimulation(const json& simulation)
{
    const std::string serialized = simulation.dump();
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database,
            "INSERT OR REPLACE INTO simulations(id, state_json) VALUES (?, ?)",
            -1, &statement, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(statement, 1, simulation["simulation_id"]);
    sqlite3_bind_text(statement, 2, serialized.c_str(), -1, SQLITE_TRANSIENT);
    const bool saved = sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return saved;
}

bool initializeStore(const std::string& path)
{
    std::lock_guard<std::mutex> lock(simulationsMutex);
    try {
        const auto parent = std::filesystem::path(path).parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent);
    } catch (const std::filesystem::filesystem_error&) { return false; }
    if (sqlite3_open(path.c_str(), &database) != SQLITE_OK) {
        sqlite3_close(database);
        database = nullptr;
        return false;
    }
    sqlite3_busy_timeout(database, 3000);
    if (sqlite3_exec(database,
        "CREATE TABLE IF NOT EXISTS simulations(id INTEGER PRIMARY KEY, state_json TEXT NOT NULL)",
        nullptr, nullptr, nullptr) != SQLITE_OK) return false;
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database, "SELECT id, state_json FROM simulations", -1,
            &statement, nullptr) != SQLITE_OK) return false;
    simulations.clear();
    int code;
    while ((code = sqlite3_step(statement)) == SQLITE_ROW) {
        const int id = sqlite3_column_int(statement, 0);
        try {
            simulations[std::to_string(id)] = json::parse(
                reinterpret_cast<const char*>(sqlite3_column_text(statement, 1)));
        } catch (const json::exception&) {
            sqlite3_finalize(statement);
            return false;
        }
        nextId = std::max(nextId, id + 1);
    }
    sqlite3_finalize(statement);
    return code == SQLITE_DONE;
}

void closeStore()
{
    std::lock_guard<std::mutex> lock(simulationsMutex);
    sqlite3_close(database);
    database = nullptr;
}

double roundValue(double value)
{
    return std::round(value * 100.0) / 100.0;
}

bool isBlank(const std::string& text)
{
    return text.empty() || std::all_of(
        text.begin(),
        text.end(),
        [](unsigned char character) {
            return std::isspace(character);
        }
    );
}

json makeError(int status, const std::string& code, const std::string& message)
{
    return {
        {"_http_status", status},
        {"error", {
            {"code", code},
            {"message", message}
        }}
    };
}

std::string getSeverity(double intensity)
{
    if (intensity <= 3.0) {
        return "LOW";
    }
    if (intensity <= 6.0) {
        return "MODERATE";
    }
    return "HIGH";
}

json buildResponse(const json& simulation)
{
    json response = simulation;
    return response;
}

json listCases()
{
    return caseCatalog();
}

json getCase(const std::string& id)
{
    for (const auto& item : caseCatalog()) {
        if (std::to_string(item["case_id"].get<int>()) == id) return item;
    }
    return makeError(404, "case_not_found", "Case not found.");
}

json listSimulations()
{
    std::lock_guard<std::mutex> lock(simulationsMutex);
    json result = json::array();
    for (const auto& [id, state] : simulations) {
        result.push_back({
            {"simulation_id", state["simulation_id"]}, {"case_id", state["case_id"]},
            {"status", state["status"]},
            {"elapsed_time_minutes", state["elapsed_time_minutes"]}
        });
    }
    std::sort(result.begin(), result.end(), [](const json& a, const json& b) {
        return a["simulation_id"].get<int>() > b["simulation_id"].get<int>();
    });
    return result;
}

void updateSymptom(json& simulation, const std::string& name, double change)
{
    for (json& symptom : simulation["patient"]["symptoms"]) {
        if (symptom["name"] == name) {
            double intensity = symptom["intensity"];
            intensity = std::clamp(intensity + change, 0.0, 10.0);
            symptom["intensity"] = roundValue(intensity);
            symptom["severity"] = getSeverity(intensity);
        }
    }
}

void updateVitalSign(json& simulation, const std::string& name, double change)
{
    for (json& vitalSign : simulation["patient"]["vital_signs"]) {
        if (vitalSign["name"] == name) {
            double value = vitalSign["value"];
            vitalSign["value"] = roundValue(value + change);
        }
    }
}

bool hasCriticalState(json& simulation)
{
    bool criticalState = false;

    for (json& vitalSign : simulation["patient"]["vital_signs"]) {
        std::string name = vitalSign["name"];
        double value = vitalSign["value"];

        bool critical =
            (name == "Heart rate" && value >= 140.0) ||
            (name == "Oxygen saturation" && value <= 85.0) ||
            (name == "Systolic blood pressure" && value <= 80.0);

        vitalSign["is_critical"] = critical;

        if (critical) {
            criticalState = true;
        }
    }

    return criticalState;
}

void closeSimulation(
    json& simulation,
    const std::string& status
)
{
    simulation["status"] = status;
    const int correct = simulation["correct_actions"];
    const int total = simulation["interaction_count"];
    const std::string outcome = status == "critical" ? "Critical deterioration" :
        status == "time_expired" ? "Time expired" :
        correct > 0 ? "Care completed with effective actions" :
        "Care completed without effective actions";
    simulation["result"] = {
        {"outcome", outcome},
        {"elapsed_time_minutes", simulation["elapsed_time_minutes"]},
        {"interaction_count", total},
        {"correct_actions", correct},
        {"score_percent", total == 0 ? 0 : 100 * correct / total}
    };
}

nlohmann::json startSimulation(int timeLimit, int caseId)
{
    if (timeLimit <= 0) {
        return makeError(
            400,
            "invalid_time_limit",
            "time_limit_minutes must be greater than zero."
        );
    }

    json selected;
    for (const auto& item : caseCatalog()) {
        if (item["case_id"] == caseId) selected = item;
    }
    if (selected.is_null()) return makeError(404, "case_not_found", "Case not found.");

    std::lock_guard<std::mutex> lock(simulationsMutex);
    int id = nextId;

    json simulation = {
        {"simulation_id", id},
        {"case_id", caseId},
        {"case_title", selected["title"]},
        {"status", "running"},
        {"elapsed_time_minutes", 0},
        {"time_limit_minutes", timeLimit},
        {"interaction_count", 0},
        {"correct_actions", 0},
        {"patient", selected["patient"]},
        {"history", json::array()},
        {"result", nullptr}
    };

    if (!saveSimulation(simulation)) return makeError(500, "storage_error", "Could not save simulation.");
    nextId++;
    simulations[std::to_string(id)] = simulation;
    return buildResponse(simulation);
}

nlohmann::json registerInteraction(
    const std::string& id,
    const std::string& studentText,
    int minutes
)
{
    std::lock_guard<std::mutex> lock(simulationsMutex);
    if (!simulations.contains(id)) {
        return makeError(404, "simulation_not_found", "Simulation not found.");
    }

    json simulation = simulations[id];

    if (simulation["status"] != "running") {
        return makeError(409, "simulation_finished", "The simulation has already finished.");
    }
    if (isBlank(studentText)) {
        return makeError(400, "invalid_student_text", "student_text cannot be empty.");
    }
    if (minutes <= 0) {
        return makeError(
            400,
            "invalid_elapsed_minutes",
            "elapsed_minutes must be greater than zero."
        );
    }

    std::string normalizedText = studentText;
    std::transform(
        normalizedText.begin(),
        normalizedText.end(),
        normalizedText.begin(),
        [](unsigned char character) {
            return std::tolower(character);
        }
    );

    bool correctAction = false;
    if (simulation["case_id"] == 1) {
        bool treatedBreathing =
            normalizedText.find("oxygen") != std::string::npos ||
            normalizedText.find("saturation") != std::string::npos;
        bool treatedPain =
            normalizedText.find("analges") != std::string::npos ||
            normalizedText.find("relieve pain") != std::string::npos;
        correctAction = treatedBreathing || treatedPain;
        if (treatedBreathing) {
            updateSymptom(simulation, "Shortness of breath", -3.0);
            updateVitalSign(simulation, "Oxygen saturation", 2.0);
        }
        if (treatedPain) updateSymptom(simulation, "Chest pain", -5.0);
        updateSymptom(simulation, "Chest pain", 0.1 * minutes);
        updateSymptom(simulation, "Shortness of breath", 0.1 * minutes);
        updateVitalSign(simulation, "Oxygen saturation", -0.1 * minutes);
    } else {
        correctAction = normalizedText.find("water") != std::string::npos ||
            normalizedText.find("hydrate") != std::string::npos ||
            normalizedText.find("fluid") != std::string::npos;
        if (correctAction) {
            updateSymptom(simulation, "Thirst", -4.0);
            updateSymptom(simulation, "Dizziness", -2.0);
            updateVitalSign(simulation, "Systolic blood pressure", 3.0);
            updateVitalSign(simulation, "Heart rate", -4.0);
        }
        updateSymptom(simulation, "Thirst", 0.1 * minutes);
        updateSymptom(simulation, "Dizziness", 0.1 * minutes);
        updateVitalSign(simulation, "Systolic blood pressure", -0.3 * minutes);
    }
    updateVitalSign(simulation, "Heart rate", 0.2 * minutes);

    const int currentTime = simulation["elapsed_time_minutes"];
    const int timeLimit = simulation["time_limit_minutes"];
    simulation["elapsed_time_minutes"] =
        static_cast<int>(std::min(static_cast<std::int64_t>(currentTime) + minutes,
                                  static_cast<std::int64_t>(timeLimit)));

    int interactionCount = simulation["interaction_count"];
    simulation["interaction_count"] = interactionCount + 1;
    if (correctAction) simulation["correct_actions"] = simulation["correct_actions"].get<int>() + 1;

    const std::string patientResponse = correctAction
        ? "I feel better after your intervention."
        : "My condition did not improve after that intervention.";
    simulation["history"].push_back({
        {"student_text", studentText}, {"elapsed_minutes", minutes},
        {"classification", correctAction ? "correct" : "incorrect"},
        {"patient_response", patientResponse},
        {"simulation_time_minutes", simulation["elapsed_time_minutes"]}
    });

    if (hasCriticalState(simulation)) {
        closeSimulation(simulation, "critical");
    }
    else if (
        simulation["elapsed_time_minutes"].get<int>() >=
        simulation["time_limit_minutes"].get<int>()
    ) {
        closeSimulation(simulation, "time_expired");
    }

    if (!saveSimulation(simulation)) return makeError(500, "storage_error", "Could not save interaction.");
    simulations[id] = simulation;

    json response = buildResponse(simulation);
    response["classification"] = correctAction ? "correct" : "incorrect";
    response["patient_response"] = patientResponse;

    return response;
}

nlohmann::json finishSimulation(
    const std::string& id
)
{
    std::lock_guard<std::mutex> lock(simulationsMutex);
    if (!simulations.contains(id)) {
        return makeError(404, "simulation_not_found", "Simulation not found.");
    }

    json simulation = simulations[id];

    if (simulation["status"] != "running") {
        return makeError(409, "simulation_finished", "The simulation has already finished.");
    }
    closeSimulation(simulation, "finished");
    if (!saveSimulation(simulation)) return makeError(500, "storage_error", "Could not save result.");
    simulations[id] = simulation;
    return buildResponse(simulation);
}

nlohmann::json getSimulation(const std::string& id)
{
    std::lock_guard<std::mutex> lock(simulationsMutex);
    const auto found = simulations.find(id);
    if (found == simulations.end()) {
        return makeError(404, "simulation_not_found", "Simulation not found.");
    }
    return buildResponse(found->second);
}

nlohmann::json getSimulationResult(const std::string& id)
{
    std::lock_guard<std::mutex> lock(simulationsMutex);
    const auto found = simulations.find(id);
    if (found == simulations.end()) {
        return makeError(404, "simulation_not_found", "Simulation not found.");
    }
    if (found->second["status"] == "running") {
        return makeError(409, "simulation_running", "The simulation is still running.");
    }
    return found->second["result"];
}
