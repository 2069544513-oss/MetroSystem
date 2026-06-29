#ifndef STATION_H
#define STATION_H

#include <string>
#include <vector>
#include <map>
#include <iostream>

struct Station {
    int id;
    std::string name;
    std::string line;
    bool open;
};

class StationManager {
public:
    std::vector<Station> stations;
    std::map<int, bool> originStatus;
    std::map<int, bool> openMap;

    bool loadStations(const std::string& file);

    void updateStatusFromCSV(const std::string& file);
    void resetStatus();
    void setStationStatus(int id, bool open);

    Station* getStationById(int id);
    std::vector<Station*> searchStations(const std::string& keyword);
    std::vector<Station*> getStationsByLine(const std::string& line);

    bool isOpen(int id) const;
    void showClosedStations() const;
    void showLineInfo(const std::string& line) const;

    const std::map<int, bool>& getOpenMap() const { return openMap; }

private:
    void syncOpenMap();
};

#endif
