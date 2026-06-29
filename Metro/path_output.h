#ifndef PATH_OUTPUT_H
#define PATH_OUTPUT_H

#include <vector>
#include <string>

class Graph;
class StationManager;

// Build human-readable path detail from a raw station-ID path
void buildPathDetail(Graph* graph, StationManager* sm,
                     const std::vector<int>& path,
                     std::vector<std::string>& detail,
                     int& totalTime, int& transfers);

// Collect K-shortest path candidates by blocking each edge in the best path
void collectTimeCandidates(Graph* graph, StationManager* sm,
                           int start, int end,
                           const std::vector<int>& bestPath,
                           std::vector<std::vector<int>>& paths,
                           std::vector<int>& times,
                           std::vector<std::vector<std::string>>& details);

void collectTransferCandidates(Graph* graph, StationManager* sm,
                               int start, int end,
                               const std::vector<int>& bestPath,
                               std::vector<std::vector<int>>& paths,
                               std::vector<int>& transfers,
                               std::vector<int>& times,
                               std::vector<std::vector<std::string>>& details);

#endif
