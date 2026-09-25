#include "simulation_api.h"

#include <nlohmann/json.hpp>

#include "simulation_store.h"

using json = nlohmann::json;

bool readJson(
    const httplib::Request& request,
    httplib::Response& response,
    json& data
)
{
    try {
        data = json::parse(request.body);
    }
    catch (const json::parse_error&) {
        response.status = 400;
        response.set_content(
            R"({"error":{"code":"invalid_json","message":"Invalid JSON."}})",
            "application/json"
        );
        return false;
    }

    if (!data.is_object()) {
        response.status = 400;
        response.set_content(
            R"({"error":{"code":"invalid_json","message":"The request body must be a JSON object."}})",
            "application/json"
        );
        return false;
    }

    return true;
}

void sendResponse(
    json data,
    int successStatus,
    httplib::Response& response
)
{
    response.status = data.value("_http_status", successStatus);
    data.erase("_http_status");
    response.set_content(data.dump(), "application/json");
}

void postSimulations(
    const httplib::Request& request,
    httplib::Response& response
)
{
    json data;

    if (!readJson(request, response, data)) {
        return;
    }

    if (
        !data.contains("time_limit_minutes") ||
        !data["time_limit_minutes"].is_number_integer()
    ) {
        response.status = 400;
        response.set_content(
            R"({"error":{"code":"invalid_field","message":"time_limit_minutes must be an integer."}})",
            "application/json"
        );
        return;
    }

    int timeLimit = data["time_limit_minutes"];
    json result = startSimulation(timeLimit);
    sendResponse(result, 201, response);
}

void postInteractions(
    const httplib::Request& request,
    httplib::Response& response
)
{
    json data;

    if (!readJson(request, response, data)) {
        return;
    }

    if (
        !data.contains("student_text") ||
        !data["student_text"].is_string() ||
        !data.contains("elapsed_minutes") ||
        !data["elapsed_minutes"].is_number_integer()
    ) {
        response.status = 400;
        response.set_content(
            R"({"error":{"code":"invalid_field","message":"student_text and elapsed_minutes are required."}})",
            "application/json"
        );
        return;
    }

    std::string id = request.matches[1];
    std::string studentText = data["student_text"];
    int minutes = data["elapsed_minutes"];

    json result = registerInteraction(id, studentText, minutes);
    sendResponse(result, 200, response);
}

void postFinishSimulation(
    const httplib::Request& request,
    httplib::Response& response
)
{
    json data;

    if (!readJson(request, response, data)) {
        return;
    }

    if (!data.contains("outcome") || !data["outcome"].is_string()) {
        response.status = 400;
        response.set_content(
            R"({"error":{"code":"invalid_field","message":"outcome is required."}})",
            "application/json"
        );
        return;
    }

    std::string id = request.matches[1];
    std::string outcome = data["outcome"];

    json result = finishSimulation(id, outcome);
    sendResponse(result, 200, response);
}
