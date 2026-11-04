#include <iostream>
#include <cstdlib>

#include <httplib.h>

#include "simulation_api.h"
#include "simulation_store.h"

int main()
{
    const char* configuredPath = std::getenv("CLINICAL_SIMULATOR_DB");
    const std::string databasePath = configuredPath ? configuredPath : "data/clinical_simulator.db";
    if (!initializeStore(databasePath)) {
        std::cerr << "Could not initialize SQLite database: " << databasePath << std::endl;
        return 1;
    }
    httplib::Server server;

    server.Get("/", [](const httplib::Request&, httplib::Response& response) {
        response.set_content(
            R"({
  "name": "Clinical Simulator API",
  "instructions": "Start a simulation first and use the returned simulation_id in the other routes.",
  "routes": [
    {
      "method": "GET",
      "path": "/api/v1/cases"
    },
    {
      "method": "GET",
      "path": "/api/v1/cases/{id}"
    },
    {
      "method": "GET",
      "path": "/api/v1/health"
    },
    {
      "method": "POST",
      "path": "/api/v1/simulations",
      "body": {
        "time_limit_minutes": 30,
        "case_id": 1
      }
    },
    {
      "method": "GET",
      "path": "/api/v1/simulations"
    },
    {
      "method": "GET",
      "path": "/api/v1/simulations/{id}"
    },
    {
      "method": "POST",
      "path": "/api/v1/simulations/{id}/interactions",
      "body": {
        "student_text": "Administer oxygen and monitor saturation.",
        "elapsed_minutes": 2
      }
    },
    {
      "method": "POST",
      "path": "/api/v1/simulations/{id}/finish"
    },
    {
      "method": "GET",
      "path": "/api/v1/simulations/{id}/result"
    }
  ]
})",
            "application/json"
        );
    });

    server.Get("/api/v1/health", [](const httplib::Request&, httplib::Response& response) {
        response.set_content(R"({"status":"ok"})", "application/json");
    });

    server.Post("/api/v1/simulations", postSimulations);
    server.Get("/api/v1/cases", getCases);
    server.Get(R"(/api/v1/cases/(\d+))", getCaseById);
    server.Get("/api/v1/simulations", getSimulations);
    server.Get(R"(/api/v1/simulations/(\d+))", getSimulationById);
    server.Get(R"(/api/v1/simulations/(\d+)/result)", getResult);
    server.Post(
        R"(/api/v1/simulations/(\d+)/interactions)",
        postInteractions
    );
    server.Post(
        R"(/api/v1/simulations/(\d+)/finish)",
        postFinishSimulation
    );

    std::cout << "Listening on http://127.0.0.1:8080" << std::endl;
    if (!server.listen("127.0.0.1", 8080)) {
        std::cerr << "Could not start server on 127.0.0.1:8080." << std::endl;
        closeStore();
        return 1;
    }
    closeStore();
    return 0;
}
