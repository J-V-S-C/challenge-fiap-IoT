#ifndef SIMULATION_STORE_H
#define SIMULATION_STORE_H

#include <string>

#include <nlohmann/json.hpp>

bool initializeStore(const std::string& path);
void closeStore();

nlohmann::json listCases();
nlohmann::json getCase(const std::string& id);
nlohmann::json listSimulations();
nlohmann::json startSimulation(int timeLimit, int caseId);

nlohmann::json registerInteraction(
    const std::string& id,
    const std::string& studentText,
    int minutes
);

nlohmann::json finishSimulation(
    const std::string& id
);

nlohmann::json getSimulation(const std::string& id);

nlohmann::json getSimulationResult(const std::string& id);

#endif

