// TIVIDY web server: the REST API from docs/web-api.md plus the static front-end in ../web.
//
// This file only moves text between HTTP and TividyService. All workflow behaviour is in
// the engine; nothing here knows what a task, a stage or an industry is.

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "third_party/httplib.h"
#include "third_party/json.hpp"
#include "TividyService.h"

typedef nlohmann::json Json;

namespace {

void send(httplib::Response& res, const std::string& body, int status = 200) {
    res.status = status;
    res.set_header("Cache-Control", "no-store");
    res.set_content(body, "application/json");
}

void sendError(httplib::Response& res, int status, const std::string& message) {
    Json j;
    j["ok"] = false;
    j["message"] = message;
    j["events"] = Json::array();
    send(res, j.dump(), status);
}

/// Parses the request body as a JSON object; an empty body counts as {}.
bool parseBody(const httplib::Request& req, httplib::Response& res, Json& out) {
    if (req.body.empty()) {
        out = Json::object();
        return true;
    }
    out = Json::parse(req.body, nullptr, false);
    if (out.is_discarded() || !out.is_object()) {
        sendError(res, 400, "The request body must be a JSON object.");
        return false;
    }
    return true;
}

std::string text(const Json& body, const char* key) {
    Json::const_iterator found = body.find(key);
    return found != body.end() && found->is_string() ? found->get<std::string>() : std::string();
}

int number(const Json& body, const char* key, int fallback) {
    Json::const_iterator found = body.find(key);
    return found != body.end() && found->is_number_integer() ? found->get<int>() : fallback;
}

}  // namespace

int main(int argc, char** argv) {
    int port = 8080;
    std::string webDir = "../web";
    std::string logFile = "tividy-log.jsonl";   //everything is saved here and replayed on start
    bool fresh = false;
    bool defaults = true;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) port = std::atoi(argv[++i]);
        else if (arg == "--web" && i + 1 < argc) webDir = argv[++i];
        else if (arg == "--log" && i + 1 < argc) logFile = argv[++i];
        else if (arg == "--no-save") logFile.clear();
        else if (arg == "--fresh") fresh = true;
        else if (arg == "--no-defaults") defaults = false;
        else {
            std::cerr << "Usage: tividy_web [--port N] [--web DIR] [--log FILE] [--fresh] [--no-save] [--no-defaults]\n";
            return 1;
        }
    }

    TividyService service;
    if (!logFile.empty()) {
        if (fresh) std::remove(logFile.c_str());
        int replayed = service.useLog(logFile);
        std::cout << "Saving to " << logFile << " (" << replayed << " earlier change(s) restored).\n";
    }
    //like a main(): with nothing open, start with the default scenarios (3 support, 3 admissions)
    if (defaults && Json::parse(service.state()).value("loaded", false) == false) service.loadDefaults();
    httplib::Server server;

    server.Get("/api/industries", [&](const httplib::Request&, httplib::Response& res) {
        send(res, service.industries());
    });

    server.Post("/api/industry", [&](const httplib::Request& req, httplib::Response& res) {
        Json body;
        if (!parseBody(req, res, body)) return;
        if (body.find("industryId") == body.end() || !body["industryId"].is_number_integer()) {
            sendError(res, 400, "industryId (integer) is required.");
            return;
        }
        std::string overrides;
        Json::const_iterator data = body.find("data");
        if (data != body.end()) overrides = data->dump();
        send(res, service.loadIndustry(body["industryId"].get<int>(), number(body, "caseIndex", 0), overrides));
    });

    server.Post("/api/workflow", [&](const httplib::Request& req, httplib::Response& res) {
        Json body;
        if (!parseBody(req, res, body)) return;
        send(res, service.createWorkflow(body.dump()));
    });

    server.Get("/api/state", [&](const httplib::Request&, httplib::Response& res) {
        send(res, service.state());
    });

    server.Post("/api/action", [&](const httplib::Request& req, httplib::Response& res) {
        Json body;
        if (!parseBody(req, res, body)) return;
        std::string action = text(body, "action");
        std::string taskId = text(body, "taskId");
        if (action.empty() || taskId.empty()) {
            sendError(res, 400, "action and taskId are required.");
            return;
        }
        send(res, service.act(action, taskId, text(body, "participantName"), text(body, "caseId")));
    });

    server.Post("/api/undo", [&](const httplib::Request&, httplib::Response& res) { send(res, service.undo()); });
    server.Post("/api/redo", [&](const httplib::Request&, httplib::Response& res) { send(res, service.redo()); });

    server.Post("/api/rollback", [&](const httplib::Request& req, httplib::Response& res) {
        Json body;
        if (!parseBody(req, res, body)) return;
        send(res, service.rollback(number(body, "marker", 0)));
    });

    server.Post("/api/case", [&](const httplib::Request& req, httplib::Response& res) {
        Json body;
        if (!parseBody(req, res, body)) return;
        send(res, service.switchCase(number(body, "index", -1)));
    });

    server.Post("/api/reset", [&](const httplib::Request&, httplib::Response& res) {
        send(res, service.reset());
    });
    server.Post("/api/defaults", [&](const httplib::Request&, httplib::Response& res) {
        send(res, service.resetToDefaults());
    });
    server.Post("/api/review", [&](const httplib::Request&, httplib::Response& res) {
        send(res, service.review());
    });
    server.Post("/api/batch", [&](const httplib::Request& req, httplib::Response& res) {
        Json body;
        if (!parseBody(req, res, body)) return;
        send(res, service.startBatch(number(body, "industryId", 1), number(body, "count", 15), number(body, "slots", 8),
                                     text(body, "policy")));
    });
    server.Post("/api/batch/tick", [&](const httplib::Request& req, httplib::Response& res) {
        Json body;
        if (!parseBody(req, res, body)) return;
        send(res, service.tick(number(body, "ticks", 1)));
    });
    server.Post("/api/batch/stop", [&](const httplib::Request&, httplib::Response& res) {
        send(res, service.stopBatch());
    });

    server.Get("/api/overview", [&](const httplib::Request&, httplib::Response& res) {
        send(res, service.overview());
    });

    server.Get("/api/report/progress", [&](const httplib::Request&, httplib::Response& res) {
        send(res, service.report("progress"));
    });
    server.Get("/api/report/stalled", [&](const httplib::Request&, httplib::Response& res) {
        send(res, service.report("stalled"));
    });
    server.Get("/api/report/escalations", [&](const httplib::Request&, httplib::Response& res) {
        send(res, service.report("escalations"));
    });

    if (!server.set_mount_point("/", webDir)) {
        std::cerr << "Warning: front-end folder '" << webDir << "' not found; only the API is served.\n";
    }

    std::cout << "TIVIDY web UI: http://localhost:" << port << "/\n" << std::flush;
    if (!server.listen("0.0.0.0", port)) {
        std::cerr << "Could not listen on port " << port << " (is it already in use?).\n";
        return 1;
    }
    return 0;
}
