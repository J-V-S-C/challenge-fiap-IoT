#ifndef SIMULATION_STORE_H
#define SIMULATION_STORE_H

#include <string>

#include <nlohmann/json.hpp>

nlohmann::json startSimulation(int timeLimit);

nlohmann::json registerInteraction(
    const std::string& id,
    const std::string& studentText,
    int minutes
);

nlohmann::json finishSimulation(
    const std::string& id,
    const std::string& outcome
);

#endif

