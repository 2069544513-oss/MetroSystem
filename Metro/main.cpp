#include <iostream>
#include <string>
#include "station.h"
#include "graph.h"
#include "pathfinder.h"
#include "menu.h"

using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    StationManager stationManager;
    Graph graph;
    Pathfinder pathfinder(&graph, &stationManager);

    // ===== 1. 数据加载 =====
    if (!stationManager.loadStations("../Data/Station.csv")) {
        // Fallback for running from different working directories
        if (!stationManager.loadStations("Data/Station.csv")) {
            cout << "Station.csv 加载失败，请确认数据文件路径。\n";
            return 0;
        }
    }

    if (!graph.loadEdges("../Data/Edge.csv")) {
        if (!graph.loadEdges("Data/Edge.csv")) {
            cout << "Edge.csv 加载失败，请确认数据文件路径。\n";
            return 0;
        }
    }

    graph.setStationManager(&stationManager);

    cout << "数据加载完成: " << stationManager.stations.size()
         << " 个站点, " << graph.edges.size() << " 条边。\n";

    // ===== 2. 菜单系统 =====
    Menu menu(&stationManager, &graph, &pathfinder);

    // ===== 3. 主循环 =====
    while (true) {
        cout << "\n==============================\n";
        cout << "      地铁路径规划系统\n";
        cout << "==============================\n";
        cout << "1. 线路站点信息 / 运营状态管理\n";
        cout << "2. 所需时间最短路径规划\n";
        cout << "3. 所需换乘次数最少路径规划\n";
        cout << "4. 退出系统\n";
        cout << "请输入选项编号：";

        int op;
        if (!(cin >> op)) break;

        switch (op) {
        case 1:
            menu.stationMenu();
            break;

        case 2:
            menu.timeMenu();
            break;

        case 3:
            menu.transferMenu();
            break;

        case 4:
            cout << "系统已退出。\n";
            return 0;

        default:
            cout << "无效输入，请重新选择。\n";
            break;
        }
    }

    return 0;
}
