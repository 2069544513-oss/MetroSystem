#ifndef PATHFINDER_H
#define PATHFINDER_H

#include <vector>
#include <string>

class Graph;
class StationManager;

class Pathfinder {
public:
    Pathfinder(Graph* g, StationManager* sm);

    Graph* getGraph() const { return graph; }
    StationManager* getStationManager() const { return stationManager; }

    std::vector<int> shortestPath(int start, int end,
        int& totalTime,
        std::vector<std::string>& detail);

    std::vector<int> leastTransferPath(int start, int end,
        int& transfers,
        int& totalTime,
        std::vector<std::string>& detail);

    std::vector<std::vector<int>> kShortestPaths(int start, int end, int K,
        std::vector<int>& times,
        std::vector<std::vector<std::string>>& details);

    std::vector<std::vector<int>> kLeastTransferPaths(int start, int end, int K,
        std::vector<int>& transfers,
        std::vector<int>& times,
        std::vector<std::vector<std::string>>& details);

private:
    Graph* graph;
    StationManager* stationManager;

    std::vector<int> dijkstra(int start, int end, int& totalTime);
    std::vector<int> dijkstraByTransfer(int start, int end,
        int& transfers, int& totalTime);
};

#endif
