#include "simulation_store.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <unordered_map>

using json = nlohmann::json;

std::unordered_map<std::string, json> simulations;
int nextId = 1;

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
    response.erase("interaction_count");
    return response;
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
            (name == "Oxygen saturation" && value <= 85.0);

        vitalSign["is_critical"] = critical;

        if (critical) {
            criticalState = true;
        }
    }

    return criticalState;
}

void closeSimulation(
    json& simulation,
    const std::string& status,
    const std::string& outcome
)
{
    simulation["status"] = status;
    simulation["result"] = {
        {"outcome", outcome},
        {"elapsed_time_minutes", simulation["elapsed_time_minutes"]},
        {"interaction_count", simulation["interaction_count"]}
    };
}

nlohmann::json startSimulation(int timeLimit)
{
    if (timeLimit <= 0) {
        return makeError(
            400,
            "invalid_time_limit",
            "time_limit_minutes must be greater than zero."
        );
    }

    int id = nextId;
    nextId++;

    json simulation = {
        {"simulation_id", id},
        {"status", "running"},
        {"elapsed_time_minutes", 0},
        {"time_limit_minutes", timeLimit},
        {"interaction_count", 0},
        {"patient", {
            {"symptoms", {
                {
                    {"name", "Chest pain"},
                    {"intensity", 8.0},
                    {"severity", "HIGH"}
                },
                {
                    {"name", "Shortness of breath"},
                    {"intensity", 5.0},
                    {"severity", "MODERATE"}
                }
            }},
            {"vital_signs", {
                {
                    {"name", "Heart rate"},
                    {"value", 110.0},
                    {"unit", "bpm"},
                    {"is_critical", false}
                },
                {
                    {"name", "Oxygen saturation"},
                    {"value", 90.0},
                    {"unit", "%"},
                    {"is_critical", false}
                }
            }}
        }},
        {"result", nullptr}
    };

    simulations[std::to_string(id)] = simulation;
    return buildResponse(simulation);
}

nlohmann::json registerInteraction(
    const std::string& id,
    const std::string& studentText,
    int minutes
)
{
    if (!simulations.contains(id)) {
        return makeError(404, "simulation_not_found", "Simulation not found.");
    }

    json& simulation = simulations[id];

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

    bool treatedBreathing =
        normalizedText.find("oxygen") != std::string::npos ||
        normalizedText.find("saturation") != std::string::npos;
    bool treatedPain =
        normalizedText.find("analges") != std::string::npos ||
        normalizedText.find("relieve pain") != std::string::npos;
    bool correctAction = treatedBreathing || treatedPain;

    if (treatedBreathing) {
        updateSymptom(simulation, "Shortness of breath", -3.0);
        updateVitalSign(simulation, "Oxygen saturation", 2.0);
    }
    if (treatedPain) {
        updateSymptom(simulation, "Chest pain", -5.0);
    }

    updateSymptom(simulation, "Chest pain", 0.1 * minutes);
    updateSymptom(simulation, "Shortness of breath", 0.1 * minutes);
    updateVitalSign(simulation, "Heart rate", 0.2 * minutes);
    updateVitalSign(simulation, "Oxygen saturation", -0.1 * minutes);

    int currentTime = simulation["elapsed_time_minutes"];
    simulation["elapsed_time_minutes"] = currentTime + minutes;

    int interactionCount = simulation["interaction_count"];
    simulation["interaction_count"] = interactionCount + 1;

    if (hasCriticalState(simulation)) {
        closeSimulation(simulation, "critical", "Critical deterioration");
    }
    else if (
        simulation["elapsed_time_minutes"].get<int>() >=
        simulation["time_limit_minutes"].get<int>()
    ) {
        closeSimulation(simulation, "time_expired", "Time expired");
    }

    json response = buildResponse(simulation);
    response["classification"] = correctAction ? "correct" : "incorrect";
    response["patient_response"] = correctAction
        ? "I feel better after your intervention."
        : "My condition did not improve after that intervention.";

    return response;
}

nlohmann::json finishSimulation(
    const std::string& id,
    const std::string& outcome
)
{
    if (!simulations.contains(id)) {
        return makeError(404, "simulation_not_found", "Simulation not found.");
    }

    json& simulation = simulations[id];

    if (simulation["status"] != "running") {
        return makeError(409, "simulation_finished", "The simulation has already finished.");
    }
    if (isBlank(outcome)) {
        return makeError(400, "invalid_outcome", "outcome cannot be empty.");
    }

    closeSimulation(simulation, "finished", outcome);
    return buildResponse(simulation);
}
