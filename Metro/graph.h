#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <map>
#include <string>

struct Edge {
    int from;
    int to;
    std::string line;       // "1号线", "2号线", or "换乘"
    std::string direction;
    int time;
    bool isTransfer;
    int transferTime;
};

class StationManager;

class Graph {
public:
    std::vector<Edge> edges;
    std::map<int, std::vector<int>> adj;

    const std::map<int, bool>* stationOpenMap = nullptr;

    bool loadEdges(const std::string& path);
    void setStationManager(StationManager* sm);

    const Edge& getEdge(int idx) const { return edges[idx]; }
};

#endif
