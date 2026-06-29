#include "pathfinder.h"
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
        auto [d, u] = pq.top(); pq.pop();
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

void Pathfinder::buildPathDetail(const vector<int>& path,
    vector<string>& detail, int& totalTime, int& transfers) {
    detail.clear();
    totalTime = 0;
    transfers = 0;

    if (path.size() < 2) return;

    string currentLine = "";
    int segmentTime = 0;
    int segStart = path[0];

    for (size_t i = 0; i < path.size() - 1; i++) {
        int u = path[i], v = path[i + 1];

        int bestEi = -1, bestTime = INT_MAX;
        auto it = graph->adj.find(u);
        if (it != graph->adj.end()) {
            for (int ei : it->second) {
                if (graph->edges[ei].to == v && graph->edges[ei].time < bestTime) {
                    bestTime = graph->edges[ei].time;
                    bestEi = ei;
                }
            }
        }

        if (bestEi == -1) continue;

        const Edge& e = graph->edges[bestEi];
        totalTime += e.time;

        if (e.isTransfer) {
            // Output previous segment
            if (segmentTime > 0 && currentLine != "") {
                Station* fromS = stationManager->getStationById(segStart);
                Station* toS = stationManager->getStationById(u);
                char buf[256];
                snprintf(buf, sizeof(buf),
                    "  乘坐 %s: %s → %s (用时 %d 分钟)",
                    currentLine.c_str(),
                    fromS ? fromS->name.c_str() : "?",
                    toS ? toS->name.c_str() : "?",
                    segmentTime);
                detail.push_back(buf);
            }

            transfers++;
            Station* sAt = stationManager->getStationById(u);
            char buf[256];
            snprintf(buf, sizeof(buf),
                "  [换乘] 在 %s 站换乘",
                sAt ? sAt->name.c_str() : "?");
            detail.push_back(buf);

            currentLine = "";
            segmentTime = 0;
            segStart = v;
        } else {
            if (currentLine == "") {
                currentLine = e.line;
                segStart = u;
            }
            segmentTime += e.time;
        }
    }

    // Output last segment
    if (segmentTime > 0 && currentLine != "") {
        int last = path.back();
        Station* fromS = stationManager->getStationById(segStart);
        Station* toS = stationManager->getStationById(last);
        char buf[256];
        snprintf(buf, sizeof(buf),
            "  乘坐 %s: %s → %s (用时 %d 分钟)",
            currentLine.c_str(),
            fromS ? fromS->name.c_str() : "?",
            toS ? toS->name.c_str() : "?",
            segmentTime);
        detail.push_back(buf);
    }
}

vector<int> Pathfinder::shortestPath(int start, int end,
    int& totalTime, vector<string>& detail) {
    vector<int> path = dijkstra(start, end, totalTime);
    if (!path.empty()) {
        int tr;
        buildPathDetail(path, detail, totalTime, tr);
    }
    return path;
}

vector<int> Pathfinder::leastTransferPath(int start, int end,
    int& transfers, int& totalTime, vector<string>& detail) {
    vector<int> path = dijkstraByTransfer(start, end, transfers, totalTime);
    if (!path.empty()) {
        buildPathDetail(path, detail, totalTime, transfers);
    }
    return path;
}

vector<vector<int>> Pathfinder::kShortestPaths(int start, int end, int K,
    vector<int>& times, vector<vector<string>>& details) {
    vector<vector<int>> result;
    times.clear();
    details.clear();

    if (K <= 0) return result;

    int t;
    vector<int> p = dijkstra(start, end, t);
    if (p.empty()) return result;

    result.push_back(p);
    times.push_back(t);

    vector<string> d;
    int tr;
    buildPathDetail(p, d, t, tr);
    details.push_back(d);

    // Generate alternatives by removing one edge at a time from the best path
    for (int k = 1; k < K && k < 5; k++) {
        if (k - 1 >= (int)p.size() - 1) break;

        // Bypass one intermediate node
        int bypassIdx = k - 1;
        if (bypassIdx >= (int)p.size() - 2) break;

        int avoidNode = p[bypassIdx + 1];
        map<int, int> dist, parent;
        set<int> vis;
        priority_queue<pair<int, int>,
            vector<pair<int, int>>,
            greater<pair<int, int>>> pq;

        dist[start] = 0;
        pq.push({0, start});

        while (!pq.empty()) {
            auto [d, u] = pq.top(); pq.pop();
            if (vis.count(u)) continue;
            vis.insert(u);
            if (u == end) break;

            auto it = graph->adj.find(u);
            if (it == graph->adj.end()) continue;

            for (int ei : it->second) {
                const Edge& e = graph->edges[ei];
                int v = e.to;
                if (v == avoidNode && u == p[bypassIdx]) continue;
                if (vis.count(v)) continue;
                if (graph->stationOpenMap && graph->stationOpenMap->count(v)
                    && !graph->stationOpenMap->at(v)) continue;

                int nd = d + e.time;
                if (!dist.count(v) || nd < dist[v]) {
                    dist[v] = nd;
                    parent[v] = u;
                    pq.push({nd, v});
                }
            }
        }

        if (dist.count(end)) {
            vector<int> altPath;
            for (int cur = end; cur != start; cur = parent[cur])
                altPath.push_back(cur);
            altPath.push_back(start);
            reverse(altPath.begin(), altPath.end());

            bool dup = false;
            for (auto& r : result) {
                if (r == altPath) { dup = true; break; }
            }
            if (!dup) {
                result.push_back(altPath);
                times.push_back(dist[end]);
                vector<string> dd;
                int trr;
                buildPathDetail(altPath, dd, dist[end], trr);
                details.push_back(dd);
            }
        }
    }

    return result;
}

vector<vector<int>> Pathfinder::kLeastTransferPaths(int start, int end, int K,
    vector<int>& transfers, vector<int>& times,
    vector<vector<string>>& details) {
    vector<vector<int>> result;
    transfers.clear();
    times.clear();
    details.clear();

    if (K <= 0) return result;

    int tr, t;
    vector<int> p = dijkstraByTransfer(start, end, tr, t);
    if (p.empty()) return result;

    result.push_back(p);
    transfers.push_back(tr);
    times.push_back(t);

    vector<string> d;
    buildPathDetail(p, d, t, tr);
    details.push_back(d);

    // Generate alternatives by bypassing nodes
    for (int k = 1; k < K && k < 5; k++) {
        if (k - 1 >= (int)p.size() - 2) break;

        int avoidNode = p[k];
        map<int, int> dist, trans, parent;
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
                if (v == avoidNode) continue;
                if (vis.count(v)) continue;
                if (graph->stationOpenMap && graph->stationOpenMap->count(v)
                    && !graph->stationOpenMap->at(v)) continue;

                int nt = trans[u] + (e.isTransfer ? 1 : 0);
                int nd = dist[u] + e.time;

                if (!trans.count(v) || nt < trans[v] ||
                    (nt == trans[v] && nd < dist[v])) {
                    trans[v] = nt;
                    dist[v] = nd;
                    parent[v] = u;
                    pq.push({nt * 100000 + nd, v});
                }
            }
        }

        if (trans.count(end)) {
            vector<int> altPath;
            for (int cur = end; cur != start; cur = parent[cur])
                altPath.push_back(cur);
            altPath.push_back(start);
            reverse(altPath.begin(), altPath.end());

            bool dup = false;
            for (auto& r : result) {
                if (r == altPath) { dup = true; break; }
            }
            if (!dup) {
                result.push_back(altPath);
                transfers.push_back(trans[end]);
                times.push_back(dist[end]);
                vector<string> dd;
                int trr;
                buildPathDetail(altPath, dd, dist[end], trr);
                details.push_back(dd);
            }
        }
    }

    return result;
}
