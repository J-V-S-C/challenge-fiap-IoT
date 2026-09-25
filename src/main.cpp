#include <iostream>

#include <httplib.h>

#include "simulation_api.h"

int main()
{
    httplib::Server server;

    server.Get("/", [](const httplib::Request&, httplib::Response& response) {
        response.set_content(
            R"({
  "name": "Clinical Simulator API",
  "instructions": "Start a simulation first and use the returned simulation_id in the other routes.",
  "routes": [
    {
      "method": "POST",
      "path": "/api/v1/simulations",
      "body": {
        "time_limit_minutes": 30
      }
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
      "path": "/api/v1/simulations/{id}/finish",
      "body": {
        "outcome": "Care completed"
      }
    }
  ]
})",
            "application/json"
        );
    });

    server.Post("/api/v1/simulations", postSimulations);
    server.Post(
        R"(/api/v1/simulations/(\d+)/interactions)",
        postInteractions
    );
    server.Post(
        R"(/api/v1/simulations/(\d+)/finish)",
        postFinishSimulation
    );

    std::cout << "Clinical simulator started on port 8080." << std::endl;
    server.listen("0.0.0.0", 8080);

    return 0;
}
