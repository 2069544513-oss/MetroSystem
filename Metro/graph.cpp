#include "graph.h"
#include "station.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

using namespace std;

static vector<string> splitCSV(const string& line) {
    vector<string> res;
    string cur;
    for (char c : line) {
        if (c == '\r') continue;
        if (c == ',') { res.push_back(cur); cur.clear(); }
        else cur += c;
    }
    res.push_back(cur);
    return res;
}

bool Graph::loadEdges(const string& path) {
    ifstream file(path);
    if (!file.is_open()) return false;

    string line;
    getline(file, line); // header

    while (getline(file, line)) {
        auto f = splitCSV(line);
        if (f.size() < 5) continue;

        Edge e;
        e.from = stoi(f[0]);
        e.to = stoi(f[1]);
        e.line = f[2];
        e.direction = f[3];
        e.time = stoi(f[4]);
        e.isTransfer = (e.line == "换乘");
        e.transferTime = e.isTransfer ? e.time : 0;

        edges.push_back(e);
        adj[e.from].push_back((int)edges.size() - 1);
    }
    return true;
}

void Graph::setStationManager(StationManager* sm) {
    stationOpenMap = &sm->getOpenMap();
}
