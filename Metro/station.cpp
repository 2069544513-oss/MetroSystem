#include "station.h"
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace std;

static vector<string> splitCSV(const string& line) {
    vector<string> res;
    string cur;
    for (char c : line) {
        if (c == '\r') continue;
        if (c == ',') { res.push_back(cur); cur.clear(); }
        else cur += c;
    }
    res.push_back(cur);
    return res;
}

bool StationManager::loadStations(const string& file) {
    ifstream fin(file);
    if (!fin.is_open()) return false;

    string line;
    getline(fin, line); // skip header

    while (getline(fin, line)) {
        auto f = splitCSV(line);
        if (f.size() < 4) continue;

        Station s;
        s.id = stoi(f[0]);
        s.name = f[1];
        s.line = f[2];
        s.open = (f[3] == "开启");

        stations.push_back(s);
        originStatus[s.id] = s.open;
        openMap[s.id] = s.open;
    }
    return true;
}

void StationManager::syncOpenMap() {
    for (auto& s : stations)
        openMap[s.id] = s.open;
}

void StationManager::updateStatusFromCSV(const string& file) {
    ifstream fin(file);
    if (!fin.is_open()) return;

    string line;
    getline(fin, line);

    while (getline(fin, line)) {
        auto f = splitCSV(line);
        if (f.size() < 2) continue;
        int id = stoi(f[0]);
        bool open = (f[1] == "开启" || f[1] == "1");
        setStationStatus(id, open);
    }
}

void StationManager::setStationStatus(int id, bool open) {
    for (auto& s : stations) {
        if (s.id == id) {
            s.open = open;
            openMap[id] = open;
            return;
        }
    }
}

void StationManager::resetStatus() {
    for (auto& s : stations) {
        if (originStatus.find(s.id) != originStatus.end()) {
            s.open = originStatus[s.id];
            openMap[s.id] = s.open;
        }
    }
}

Station* StationManager::getStationById(int id) {
    for (auto& s : stations)
        if (s.id == id) return &s;
    return nullptr;
}

vector<Station*> StationManager::searchStations(const string& keyword) {
    vector<Station*> result;
    for (auto& s : stations)
        if (s.name.find(keyword) != string::npos)
            result.push_back(&s);
    return result;
}

vector<Station*> StationManager::getStationsByLine(const string& line) {
    vector<Station*> result;
    for (auto& s : stations)
        if (s.line == line)
            result.push_back(&s);
    return result;
}

bool StationManager::isOpen(int id) const {
    for (auto& s : stations)
        if (s.id == id) return s.open;
    return false;
}

void StationManager::showClosedStations() const {
    cout << "\n===== 当前关闭站点 =====\n";
    bool found = false;
    for (auto& s : stations) {
        if (!s.open) {
            cout << "  [" << s.id << "] " << s.name << " (" << s.line << ")\n";
            found = true;
        }
    }
    if (!found) cout << "  所有站点均处于开启状态。\n";
}

void StationManager::showLineInfo(const string& line) const {
    cout << "\n===== " << line << " 站点信息 =====\n";
    bool found = false;
    for (auto& s : stations) {
        if (s.line == line) {
            cout << "  [" << s.id << "] " << s.name
                 << (s.open ? " [开启]" : " [关闭]") << "\n";
            found = true;
        }
    }
    if (!found) cout << "  未找到该线路的站点。\n";
}
