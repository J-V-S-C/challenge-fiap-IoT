#ifndef SIMULATION_API_H
#define SIMULATION_API_H

#include <httplib.h>

void postSimulations(
    const httplib::Request& request,
    httplib::Response& response
);

void postInteractions(
    const httplib::Request& request,
    httplib::Response& response
);

void postFinishSimulation(
    const httplib::Request& request,
    httplib::Response& response
);

#endif

