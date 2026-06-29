#include "menu.h"
#include "station.h"
#include "graph.h"
#include "pathfinder.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <set>
#include <map>
#include <queue>
#include <algorithm>

using namespace std;

Menu::Menu(StationManager* sm, Graph* g, Pathfinder* pf)
    : stationManager(sm), graph(g), pathfinder(pf) {}

int Menu::getChoice(int minV, int maxV) {
    int choice;
    while (true) {
        cout << "请输入选项编号：";
        if (cin >> choice && choice >= minV && choice <= maxV) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return choice;
        }
        cout << "输入无效，请重新输入！\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

void Menu::pause() {
    cout << "\n按回车键继续...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

int Menu::readStation(const string& prompt) {
    while (true) {
        cout << prompt;
        string input;
        getline(cin, input);
        if (input.empty()) continue;

        // 尝试作为纯数字ID解析
        bool isNumeric = true;
        for (char c : input) {
            if (!isdigit((unsigned char)c)) { isNumeric = false; break; }
        }

        if (isNumeric) {
            int id = stoi(input);
            if (stationManager->getStationById(id))
                return id;
            cout << "未找到站点ID: " << id << "，请重新输入。\n";
            continue;
        }

        // 按名称搜索
        auto results = stationManager->searchStations(input);
        if (results.empty()) {
            cout << "未找到包含 \"" << input << "\" 的站点，请重新输入。\n";
            continue;
        }

        if (results.size() == 1) {
            cout << "  → " << results[0]->name
                 << " (" << results[0]->line << ")\n";
            return results[0]->id;
        }

        // 多个结果，让用户选择
        cout << "找到 " << results.size() << " 个匹配站点:\n";
        for (size_t i = 0; i < results.size(); i++) {
            cout << "  " << (i + 1) << ". [" << results[i]->id << "] "
                 << results[i]->name << " (" << results[i]->line << ")"
                 << (results[i]->open ? "" : " [关闭]") << "\n";
        }
        cout << "请选择序号（1-" << results.size() << "）：";
        int choice;
        if (cin >> choice && choice >= 1 && choice <= (int)results.size()) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return results[choice - 1]->id;
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

// ==================== Station Menu ====================

void Menu::stationMenu() {
    while (true) {
        cout << "\n==== 线路站点信息/运营状态管理 ====\n";
        cout << "1. 从 CSV 文件批量更新站点开启/关闭状态\n";
        cout << "2. 手动更新站点开启/关闭状态\n";
        cout << "3. 显示当前关闭站点\n";
        cout << "4. 恢复所有站点至初始状态\n";
        cout << "5. 显示线路站点信息\n";
        cout << "6. 关闭站点影响范围分析\n";
        cout << "7. 站点查询\n";
        cout << "8. 返回上级菜单\n";

        int c = getChoice(1, 8);
        switch (c) {
        case 1: updateStationStatusByCSV(); break;
        case 2: manualUpdateStationStatus(); break;
        case 3: showClosedStations(); break;
        case 4: resetAllStations(); break;
        case 5: showLineInfo(); break;
        case 6: analyzeAffectedArea(); break;
        case 7: queryStation(); break;
        case 8: return;
        }
    }
}

void Menu::updateStationStatusByCSV() {
    cout << "\n请输入CSV文件路径（格式: 站点ID,状态）：";
    string path;
    getline(cin, path);
    if (path.empty()) {
        cout << "路径不能为空。\n";
        return;
    }
    stationManager->updateStatusFromCSV(path);
    cout << "状态更新完成！\n";
    showClosedStations();
}

void Menu::manualUpdateStationStatus() {
    int id = readStation("请输入站点名称或ID：");

    Station* s = stationManager->getStationById(id);
    if (!s) {
        cout << "未找到该站点。\n";
        return;
    }

    cout << "站点: " << s->name << " (" << s->line << ")\n";
    cout << "当前状态: " << (s->open ? "开启" : "关闭") << "\n";
    cout << "请输入新状态 (1=开启, 0=关闭)：";
    int status;
    cin >> status;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    stationManager->setStationStatus(id, status == 1);
    cout << "状态已更新。\n";
}

void Menu::showClosedStations() {
    stationManager->showClosedStations();
}

void Menu::resetAllStations() {
    stationManager->resetStatus();
    cout << "所有站点已恢复至初始状态。\n";
}

void Menu::showLineInfo() {
    cout << "\n请输入线路名称（如: 1号线, 2号线, 磁悬浮线...）：";
    string line;
    getline(cin, line);
    if (line.empty()) {
        cout << "线路名不能为空。\n";
        return;
    }
    stationManager->showLineInfo(line);
}

void Menu::analyzeAffectedArea() {
    cout << "\n分析关闭站点对路网连通性的影响...\n";

    // Collect closed stations
    vector<int> closedIds;
    for (auto& s : stationManager->stations) {
        if (!s.open) closedIds.push_back(s.id);
    }

    if (closedIds.empty()) {
        cout << "当前没有关闭的站点，路网完全连通。\n";
        return;
    }

    // For each closed station, find which adjacent stations lose connectivity
    map<int, set<int>> affectedBy; // closed station -> affected adjacent stations

    for (int cid : closedIds) {
        auto it = graph->adj.find(cid);
        if (it == graph->adj.end()) continue;

        for (int ei : it->second) {
            int neighbor = graph->edges[ei].to;
            if (stationManager->isOpen(neighbor)) {
                affectedBy[cid].insert(neighbor);
            }
        }
    }

    cout << "\n===== 关闭站点影响分析 =====\n";
    for (int cid : closedIds) {
        Station* cs = stationManager->getStationById(cid);
        cout << "\n关闭站点: [" << cid << "] "
             << (cs ? cs->name : "?") << " ("
             << (cs ? cs->line : "?") << ")\n";

        if (affectedBy[cid].empty()) {
            cout << "  该站点无相邻可达站点。\n";
        } else {
            cout << "  受影响的相邻站点 (" << affectedBy[cid].size() << " 个):\n";
            for (int nid : affectedBy[cid]) {
                Station* ns = stationManager->getStationById(nid);
                cout << "    - [" << nid << "] "
                     << (ns ? ns->name : "?") << " ("
                     << (ns ? ns->line : "?") << ")\n";
            }
        }
    }

    // Count unreachable stations (BFS ignoring closed stations)
    set<int> allOpen;
    for (auto& s : stationManager->stations)
        if (s.open) allOpen.insert(s.id);

    if (allOpen.size() > 1) {
        int start = *allOpen.begin();
        set<int> visited;
        queue<int> q;
        q.push(start);
        visited.insert(start);

        while (!q.empty()) {
            int u = q.front(); q.pop();
            auto it = graph->adj.find(u);
            if (it == graph->adj.end()) continue;
            for (int ei : it->second) {
                int v = graph->edges[ei].to;
                if (!visited.count(v) && stationManager->isOpen(v)) {
                    visited.insert(v);
                    q.push(v);
                }
            }
        }

        vector<int> unreachable;
        for (int id : allOpen)
            if (!visited.count(id)) unreachable.push_back(id);

        if (!unreachable.empty()) {
            cout << "\n===== 不可达站点 (因关闭站点导致路网分割) =====\n";
            cout << "共 " << unreachable.size() << " 个站点无法从起始站点到达:\n";
            for (int id : unreachable) {
                Station* s = stationManager->getStationById(id);
                cout << "  [" << id << "] "
                     << (s ? s->name : "?") << " ("
                     << (s ? s->line : "?") << ")\n";
            }
        } else {
            cout << "\n路网仍然连通，所有开启站点均可到达。\n";
        }
    }
}

void Menu::queryStation() {
    cout << "\n请输入站点名称关键词：";
    string keyword;
    getline(cin, keyword);

    if (keyword.empty()) {
        cout << "关键词不能为空。\n";
        return;
    }

    auto results = stationManager->searchStations(keyword);
    if (results.empty()) {
        cout << "未找到包含 \"" << keyword << "\" 的站点。\n";
        return;
    }

    cout << "\n找到 " << results.size() << " 个站点:\n";
    for (auto* s : results) {
        cout << "  [" << s->id << "] " << s->name
             << " - " << s->line
             << " (" << (s->open ? "开启" : "关闭") << ")\n";
    }
}

void Menu::printStationSequence(const vector<int>& path) {
    for (size_t i = 0; i < path.size(); i++) {
        Station* s = stationManager->getStationById(path[i]);
        if (!s) continue;

        // 跳过与前一个同名站（换乘站）
        if (i > 0) {
            Station* prev = stationManager->getStationById(path[i - 1]);
            if (prev && prev->name == s->name) {
                // 更新上一行的线路信息
                continue;
            }
        }

        cout << "  " << (i + 1) << ". [" << s->id << "] "
             << s->name << " (" << s->line << ")";

        // 如果下一个站点同名（换乘），显示换乘线路
        if (i + 1 < path.size()) {
            Station* next = stationManager->getStationById(path[i + 1]);
            if (next && next->name == s->name) {
                cout << " -> (" << next->line << ")";
            }
        }
        cout << "\n";
    }
}

// ==================== Time Menu ====================

void Menu::timeMenu() {
    while (true) {
        cout << "\n==== 所需时间最短路径规划 ====\n";
        cout << "1. 单条所需时间最短路径\n";
        cout << "2. 3条所需时间最短路径\n";
        cout << "3. 返回上级菜单\n";

        int c = getChoice(1, 3);
        switch (c) {
        case 1: shortestTimePath(); break;
        case 2: kShortestTimePaths(); break;
        case 3: return;
        }
    }
}

void Menu::shortestTimePath() {
    int start = readStation("请输入起始站名称或ID：");
    int end = readStation("请输入终点站名称或ID：");

    Station* ss = stationManager->getStationById(start);
    Station* es = stationManager->getStationById(end);

    if (!ss || !es) {
        cout << "站点ID无效。\n";
        return;
    }

    if (!ss->open) {
        cout << "起始站 [" << ss->name << "] 当前处于关闭状态。\n";
        return;
    }
    if (!es->open) {
        cout << "终点站 [" << es->name << "] 当前处于关闭状态。\n";
        return;
    }

    cout << "\n计算 " << ss->name << " → " << es->name << " 的最短时间路径...\n";

    int totalTime;
    vector<string> detail;
    vector<int> path = pathfinder->shortestPath(start, end, totalTime, detail);

    if (path.empty()) {
        cout << "未找到可达路径。\n";
        return;
    }

    cout << "\n===== 最短时间路径 =====\n";
    cout << "总耗时: " << totalTime << " 分钟\n";
    cout << "经过站点数: " << path.size() << "\n";
    cout << "\n路径详情:\n";
    for (auto& d : detail) cout << d << "\n";

    cout << "\n站点序列:\n";
    for (size_t i = 0; i < path.size(); i++) {
        Station* s = stationManager->getStationById(path[i]);
        if (s) cout << "  " << (i + 1) << ". [" << s->id << "] "
                     << s->name << " (" << s->line << ")\n";
    }
}

void Menu::kShortestTimePaths() {
    int start = readStation("请输入起始站名称或ID：");
    int end = readStation("请输入终点站名称或ID：");

    Station* ss = stationManager->getStationById(start);
    Station* es = stationManager->getStationById(end);

    if (!ss || !es) {
        cout << "站点ID无效。\n";
        return;
    }
    if (!ss->open || !es->open) {
        cout << "起始站或终点站处于关闭状态。\n";
        return;
    }

    cout << "\n计算 " << ss->name << " → " << es->name << " 的3条最短时间路径...\n";

    vector<int> times;
    vector<vector<string>> details;
    auto paths = pathfinder->kShortestPaths(start, end, 3, times, details);

    if (paths.empty()) {
        cout << "未找到可达路径。\n";
        return;
    }

    for (size_t i = 0; i < paths.size(); i++) {
        cout << "\n===== 路径 " << (i + 1) << " =====";
        cout << "  总耗时: " << times[i] << " 分钟\n";
        cout << "  经过站点数: " << paths[i].size() << "\n";
        if (!details[i].empty()) {
            cout << "  路径详情:\n";
            for (auto& d : details[i]) cout << "    " << d << "\n";
        }
        cout << "  站点序列: ";
        {
            string prevName = "";
            for (size_t j = 0; j < paths[i].size(); j++) {
                Station* s = stationManager->getStationById(paths[i][j]);
                if (!s) continue;
                if (s->name == prevName) continue;
                if (!prevName.empty()) cout << " -> ";
                cout << s->name;
                prevName = s->name;
            }
        }
        cout << "\n";
    }
}

// ==================== Transfer Menu ====================

void Menu::transferMenu() {
    while (true) {
        cout << "\n==== 所需换乘次数最少路径规划 ====\n";
        cout << "1. 单条最少换乘路径\n";
        cout << "2. 3条最少换乘路径\n";
        cout << "3. 返回上级菜单\n";

        int c = getChoice(1, 3);
        switch (c) {
        case 1: minTransferPath(); break;
        case 2: kMinTransferPaths(); break;
        case 3: return;
        }
    }
}

void Menu::minTransferPath() {
    int start = readStation("请输入起始站名称或ID：");
    int end = readStation("请输入终点站名称或ID：");

    Station* ss = stationManager->getStationById(start);
    Station* es = stationManager->getStationById(end);

    if (!ss || !es) {
        cout << "站点ID无效。\n";
        return;
    }
    if (!ss->open || !es->open) {
        cout << "起始站或终点站处于关闭状态。\n";
        return;
    }

    cout << "\n计算 " << ss->name << " → " << es->name << " 的最少换乘路径...\n";

    int transfers, totalTime;
    vector<string> detail;
    vector<int> path = pathfinder->leastTransferPath(start, end, transfers, totalTime, detail);

    if (path.empty()) {
        cout << "未找到可达路径。\n";
        return;
    }

    cout << "\n===== 最少换乘路径 =====\n";
    cout << "换乘次数: " << transfers << "\n";
    cout << "总耗时: " << totalTime << " 分钟\n";
    cout << "经过站点数: " << path.size() << "\n";
    cout << "\n路径详情:\n";
    for (auto& d : detail) cout << d << "\n";

    cout << "\n站点序列:\n";
    for (size_t i = 0; i < path.size(); i++) {
        Station* s = stationManager->getStationById(path[i]);
        if (s) cout << "  " << (i + 1) << ". [" << s->id << "] "
                     << s->name << " (" << s->line << ")\n";
    }
}

void Menu::kMinTransferPaths() {
    int start = readStation("请输入起始站名称或ID：");
    int end = readStation("请输入终点站名称或ID：");

    Station* ss = stationManager->getStationById(start);
    Station* es = stationManager->getStationById(end);

    if (!ss || !es) {
        cout << "站点ID无效。\n";
        return;
    }
    if (!ss->open || !es->open) {
        cout << "起始站或终点站处于关闭状态。\n";
        return;
    }

    cout << "\n计算 " << ss->name << " → " << es->name << " 的3条最少换乘路径...\n";

    vector<int> transfers, times;
    vector<vector<string>> details;
    auto paths = pathfinder->kLeastTransferPaths(start, end, 3, transfers, times, details);

    if (paths.empty()) {
        cout << "未找到可达路径。\n";
        return;
    }

    for (size_t i = 0; i < paths.size(); i++) {
        cout << "\n===== 路径 " << (i + 1) << " =====";
        cout << "  换乘次数: " << transfers[i] << "\n";
        cout << "  总耗时: " << times[i] << " 分钟\n";
        cout << "  经过站点数: " << paths[i].size() << "\n";
        if (!details[i].empty()) {
            cout << "  路径详情:\n";
            for (auto& d : details[i]) cout << "    " << d << "\n";
        }
        cout << "  站点序列: ";
        {
            string prevName = "";
            for (size_t j = 0; j < paths[i].size(); j++) {
                Station* s = stationManager->getStationById(paths[i][j]);
                if (!s) continue;
                if (s->name == prevName) continue;
                if (!prevName.empty()) cout << " -> ";
                cout << s->name;
                prevName = s->name;
            }
        }
        cout << "\n";
    }
}
