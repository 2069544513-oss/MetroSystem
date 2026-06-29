#include "path_output.h"
#include "graph.h"
#include "station.h"
#include <queue>
#include <map>
#include <set>
#include <algorithm>
#include <climits>

using namespace std;

void buildPathDetail(Graph* graph, StationManager* sm,
                     const vector<int>& path,
                     vector<string>& detail,
                     int& totalTime, int& transfers) {
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
            if (segmentTime > 0 && currentLine != "") {
                Station* fromS = sm->getStationById(segStart);
                Station* toS = sm->getStationById(u);
                char buf[256];
                snprintf(buf, sizeof(buf),
                    "  乘坐 %s: %s -> %s (用时 %d 分钟)",
                    currentLine.c_str(),
                    fromS ? fromS->name.c_str() : "?",
                    toS ? toS->name.c_str() : "?",
                    segmentTime);
                detail.push_back(buf);
            }

            transfers++;
            Station* sAt = sm->getStationById(u);
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

    if (segmentTime > 0 && currentLine != "") {
        int last = path.back();
        Station* fromS = sm->getStationById(segStart);
        Station* toS = sm->getStationById(last);
        char buf[256];
        snprintf(buf, sizeof(buf),
            "  乘坐 %s: %s -> %s (用时 %d 分钟)",
            currentLine.c_str(),
            fromS ? fromS->name.c_str() : "?",
            toS ? toS->name.c_str() : "?",
            segmentTime);
        detail.push_back(buf);
    }
}

void collectTimeCandidates(Graph* graph, StationManager* sm,
    int start, int end, const vector<int>& bestPath,
    vector<vector<int>>& paths, vector<int>& times,
    vector<vector<string>>& details) {
    for (size_t i = 0; i < bestPath.size() - 1 && paths.size() < 15; i++) {
        int blockU = bestPath[i];
        int blockV = bestPath[i + 1];

        map<int, int> dist, parent;
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
                if (u == blockU && v == blockV) continue;
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
            vector<int> alt;
            for (int cur = end; cur != start; cur = parent[cur])
                alt.push_back(cur);
            alt.push_back(start);
            reverse(alt.begin(), alt.end());

            bool dup = false;
            for (auto& p : paths) { if (p == alt) { dup = true; break; } }
            if (!dup) {
                paths.push_back(alt);
                times.push_back(dist[end]);
                vector<string> d;
                int dummy;
                buildPathDetail(graph, sm, alt, d, dummy, dummy);
                details.push_back(d);
            }
        }
    }
}

void collectTransferCandidates(Graph* graph, StationManager* sm,
    int start, int end, const vector<int>& bestPath,
    vector<vector<int>>& paths, vector<int>& transfers,
    vector<int>& times, vector<vector<string>>& details) {
    for (size_t i = 0; i < bestPath.size() - 1 && paths.size() < 15; i++) {
        int blockU = bestPath[i];
        int blockV = bestPath[i + 1];

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
                if (u == blockU && v == blockV) continue;
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
            vector<int> alt;
            for (int cur = end; cur != start; cur = parent[cur])
                alt.push_back(cur);
            alt.push_back(start);
            reverse(alt.begin(), alt.end());

            bool dup = false;
            for (auto& p : paths) { if (p == alt) { dup = true; break; } }
            if (!dup) {
                paths.push_back(alt);
                transfers.push_back(trans[end]);
                times.push_back(dist[end]);
                vector<string> d;
                int dummy;
                buildPathDetail(graph, sm, alt, d, dummy, dummy);
                details.push_back(d);
            }
        }
    }
}
