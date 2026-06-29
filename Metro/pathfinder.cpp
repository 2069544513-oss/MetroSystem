#include "pathfinder.h"
#include "path_output.h"
#include "graph.h"
#include "station.h"
#include <queue>
#include <map>
#include <set>
#include <algorithm>
#include <climits>

using namespace std;

Pathfinder::Pathfinder(Graph* g, StationManager* sm)
    : graph(g), stationManager(sm) {}

vector<int> Pathfinder::dijkstra(int start, int end, int& totalTime) {
    totalTime = -1;
    map<int, int> dist, parent, usedEdge;
    set<int> vis;

    priority_queue<pair<int, int>,
        vector<pair<int, int>>,
        greater<pair<int, int>>> pq;

    dist[start] = 0;
    pq.push({0, start});

    while (!pq.empty()) {
        int d = pq.top().first; int u = pq.top().second; pq.pop();
        if (vis.count(u)) continue;
        vis.insert(u);
        if (u == end) break;

        auto it = graph->adj.find(u);
        if (it == graph->adj.end()) continue;

        for (int ei : it->second) {
            const Edge& e = graph->edges[ei];
            int v = e.to;
            if (vis.count(v)) continue;

            if (graph->stationOpenMap && !graph->stationOpenMap->count(v)) continue;
            if (graph->stationOpenMap && graph->stationOpenMap->at(v) == false) continue;

            int nd = d + e.time;
            if (!dist.count(v) || nd < dist[v]) {
                dist[v] = nd;
                parent[v] = u;
                usedEdge[v] = ei;
                pq.push({nd, v});
            }
        }
    }

    if (!dist.count(end)) return {};

    totalTime = dist[end];
    vector<int> path;
    for (int cur = end; cur != start; cur = parent[cur])
        path.push_back(cur);
    path.push_back(start);
    reverse(path.begin(), path.end());
    return path;
}

vector<int> Pathfinder::dijkstraByTransfer(int start, int end,
    int& transfers, int& totalTime) {
    transfers = -1;
    totalTime = -1;

    map<int, int> dist, trans, parent, usedEdge;
    set<int> vis;

    priority_queue<pair<int, int>,
        vector<pair<int, int>>,
        greater<pair<int, int>>> pq;

    dist[start] = 0;
    trans[start] = 0;
    pq.push({0, start});

    while (!pq.empty()) {
        int u = pq.top().second; pq.pop();
        if (vis.count(u)) continue;
        vis.insert(u);
        if (u == end) break;

        auto it = graph->adj.find(u);
        if (it == graph->adj.end()) continue;

        for (int ei : it->second) {
            const Edge& e = graph->edges[ei];
            int v = e.to;
            if (vis.count(v)) continue;

            if (graph->stationOpenMap && !graph->stationOpenMap->count(v)) continue;
            if (graph->stationOpenMap && graph->stationOpenMap->at(v) == false) continue;

            int nt = trans[u] + (e.isTransfer ? 1 : 0);
            int nd = dist[u] + e.time;

            if (!trans.count(v) || nt < trans[v] ||
                (nt == trans[v] && nd < dist[v])) {
                trans[v] = nt;
                dist[v] = nd;
                parent[v] = u;
                usedEdge[v] = ei;
                pq.push({nt * 100000 + nd, v});
            }
        }
    }

    if (!trans.count(end)) return {};

    transfers = trans[end];
    totalTime = dist[end];

    vector<int> path;
    for (int cur = end; cur != start; cur = parent[cur])
        path.push_back(cur);
    path.push_back(start);
    reverse(path.begin(), path.end());
    return path;
}

vector<int> Pathfinder::shortestPath(int start, int end,
    int& totalTime, vector<string>& detail) {
    vector<int> path = dijkstra(start, end, totalTime);
    if (!path.empty()) {
        int tr;
        buildPathDetail(graph, stationManager, path, detail, totalTime, tr);
    }
    return path;
}

vector<int> Pathfinder::leastTransferPath(int start, int end,
    int& transfers, int& totalTime, vector<string>& detail) {
    vector<int> path = dijkstraByTransfer(start, end, transfers, totalTime);
    if (!path.empty()) {
        buildPathDetail(graph, stationManager, path, detail, totalTime, transfers);
    }
    return path;
}

vector<vector<int>> Pathfinder::kShortestPaths(int start, int end, int K,
    vector<int>& times, vector<vector<string>>& details) {
    times.clear();
    details.clear();

    if (K <= 0) return {};

    int t;
    vector<int> bestPath = dijkstra(start, end, t);
    if (bestPath.empty()) return {};

    vector<vector<int>> paths = {bestPath};
    vector<int> tvec = {t};
    vector<vector<string>> dvec;
    {
        vector<string> dd;
        int dummy;
        buildPathDetail(graph, stationManager, bestPath, dd, dummy, dummy);
        dvec.push_back(dd);
    }

    collectTimeCandidates(graph, stationManager, start, end,
                          bestPath, paths, tvec, dvec);

    // Sort by time ascending
    vector<size_t> idx(paths.size());
    for (size_t i = 0; i < idx.size(); i++) idx[i] = i;
    sort(idx.begin(), idx.end(), [&](size_t a, size_t b) {
        if (tvec[a] != tvec[b]) return tvec[a] < tvec[b];
        return paths[a].size() < paths[b].size();
    });

    vector<vector<int>> result;
    for (size_t i = 0; i < idx.size() && (int)i < K; i++) {
        result.push_back(paths[idx[i]]);
        times.push_back(tvec[idx[i]]);
        details.push_back(dvec[idx[i]]);
    }
    return result;
}

vector<vector<int>> Pathfinder::kLeastTransferPaths(int start, int end, int K,
    vector<int>& transfers, vector<int>& times,
    vector<vector<string>>& details) {
    transfers.clear();
    times.clear();
    details.clear();

    if (K <= 0) return {};

    int tr, t;
    vector<int> bestPath = dijkstraByTransfer(start, end, tr, t);
    if (bestPath.empty()) return {};

    vector<vector<int>> paths = {bestPath};
    vector<int> trvec = {tr};
    vector<int> tvec = {t};
    vector<vector<string>> dvec;
    {
        vector<string> dd;
        int dummy;
        buildPathDetail(graph, stationManager, bestPath, dd, dummy, dummy);
        dvec.push_back(dd);
    }

    collectTransferCandidates(graph, stationManager, start, end,
                              bestPath, paths, trvec, tvec, dvec);

    // Sort by transfers, then time
    vector<size_t> idx(paths.size());
    for (size_t i = 0; i < idx.size(); i++) idx[i] = i;
    sort(idx.begin(), idx.end(), [&](size_t a, size_t b) {
        if (trvec[a] != trvec[b]) return trvec[a] < trvec[b];
        return tvec[a] < tvec[b];
    });

    vector<vector<int>> result;
    for (size_t i = 0; i < idx.size() && (int)i < K; i++) {
        result.push_back(paths[idx[i]]);
        transfers.push_back(trvec[idx[i]]);
        times.push_back(tvec[idx[i]]);
        details.push_back(dvec[idx[i]]);
    }
    return result;
}
