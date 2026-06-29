#ifndef MENU_H
#define MENU_H

#include <string>
#include <vector>

class StationManager;
class Graph;
class Pathfinder;

class Menu {
public:
    Menu(StationManager* sm, Graph* g, Pathfinder* pf);

    void stationMenu();
    void timeMenu();
    void transferMenu();

private:
    StationManager* stationManager;
    Graph* graph;
    Pathfinder* pathfinder;

    // Sub-functions for station menu
    void updateStationStatusByCSV();
    void manualUpdateStationStatus();
    void showClosedStations();
    void resetAllStations();
    void showLineInfo();
    void analyzeAffectedArea();
    void queryStation();

    // Sub-functions for path menus
    void shortestTimePath();
    void kShortestTimePaths();
    void minTransferPath();
    void kMinTransferPaths();

    // Helpers
    int getChoice(int minV, int maxV);
    int readStation(const std::string& prompt);
    void printStationSequence(const std::vector<int>& path);
    void pause();
};

#endif
