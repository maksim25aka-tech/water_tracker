// water_tracker.cpp
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <map>
#include <variant>

using namespace std;

struct WaterEntry {
    int id;
    string timestamp;
    int amount;
    string note;

    WaterEntry(int id, int amount, const string& note = "", const string& timestamp = "")
        : id(id), amount(amount), note(note) {
        if (timestamp.empty()) {
            time_t now = time(nullptr);
            char buf[20];
            tm* tm_info = localtime(&now);
            strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", tm_info);
            this->timestamp = string(buf);
        } else {
            this->timestamp = timestamp;
        }
    }
};

class WaterTracker {
private:
    vector<WaterEntry> entries;
    int nextId = 1;
    int dailyGoal = 2000;

public:
    WaterEntry addEntry(int amount, const string& note = "") {
        if (amount <= 0) throw invalid_argument("Объём должен быть положительным");
        WaterEntry entry(nextId, amount, note);
        entries.push_back(entry);
        nextId++;
        return entry;
    }

    WaterEntry* findEntry(int id) {
        auto it = find_if(entries.begin(), entries.end(), [id](const WaterEntry& e) { return e.id == id; });
        return it != entries.end() ? &(*it) : nullptr;
    }

    bool deleteEntry(int id) {
        auto it = find_if(entries.begin(), entries.end(), [id](const WaterEntry& e) { return e.id == id; });
        if (it == entries.end()) return false;
        entries.erase(it);
        return true;
    }

    vector<WaterEntry> getTodayEntries() const {
        time_t now = time(nullptr);
        tm* tm_now = localtime(&now);
        tm today_start = *tm_now;
        today_start.tm_hour = 0;
        today_start.tm_min = 0;
        today_start.tm_sec = 0;
        time_t start = mktime(&today_start);
        vector<WaterEntry> result;
        for (const auto& e : entries) {
            tm tm_e;
            strptime(e.timestamp.c_str(), "%Y-%m-%dT%H:%M:%S", &tm_e);
            time_t t = mktime(&tm_e);
            if (t >= start) result.push_back(e);
        }
        return result;
    }

    vector<WaterEntry> getWeekEntries() const {
        time_t now = time(nullptr);
        time_t weekAgo = now - 7 * 24 * 3600;
        vector<WaterEntry> result;
        for (const auto& e : entries) {
            tm tm_e;
            strptime(e.timestamp.c_str(), "%Y-%m-%dT%H:%M:%S", &tm_e);
            time_t t = mktime(&tm_e);
            if (t >= weekAgo) result.push_back(e);
        }
        return result;
    }

    map<string, variant<int, double>> getStats() const {
        auto today = getTodayEntries();
        auto week = getWeekEntries();
        int todayTotal = 0;
        for (const auto& e : today) todayTotal += e.amount;
        int weekTotal = 0;
        for (const auto& e : week) weekTotal += e.amount;
        double avgDaily = weekTotal / 7.0;
        double progress = dailyGoal > 0 ? (double)todayTotal / dailyGoal * 100 : 0;
        if (progress > 100) progress = 100;
        map<string, variant<int, double>> stats;
        stats["today_total"] = todayTotal;
        stats["today_count"] = (int)today.size();
        stats["week_total"] = weekTotal;
        stats["week_count"] = (int)week.size();
        stats["avg_daily"] = avgDaily;
        stats["daily_goal"] = dailyGoal;
        stats["progress"] = progress;
        return stats;
    }

    void setDailyGoal(int goal) {
        if (goal <= 0) throw invalid_argument("Цель должна быть положительной");
        dailyGoal = goal;
    }

    void saveToFile(const string& filename = "water_data.txt") {
        ofstream out(filename);
        if (!out) return;
        out << dailyGoal << '\n';
        for (const auto& e : entries) {
            out << e.id << '|'
                << e.timestamp << '|'
                << e.amount << '|'
                << e.note << '\n';
        }
    }

    void loadFromFile(const string& filename = "water_data.txt") {
        ifstream in(filename);
        if (!in) return;
        entries.clear();
        string line;
        if (getline(in, line)) {
            dailyGoal = stoi(line);
        }
        while (getline(in, line)) {
            stringstream ss(line);
            string idStr, timestamp, amountStr, note;
            getline(ss, idStr, '|');
            getline(ss, timestamp, '|');
            getline(ss, amountStr, '|');
            getline(ss, note, '|');
            int id = stoi(idStr);
            int amount = stoi(amountStr);
            entries.emplace_back(id, amount, note, timestamp);
            if (id >= nextId) nextId = id + 1;
        }
    }

    const vector<WaterEntry>& getEntries() const { return entries; }
};

string readString(const string& prompt) {
    cout << prompt;
    string input;
    getline(cin, input);
    return input;
}

int readInt(const string& prompt) {
    while (true) {
        cout << prompt;
        string input;
        getline(cin, input);
        try {
            return stoi(input);
        } catch (...) {
            cout << "Введите число.\n";
        }
    }
}

void printEntry(const WaterEntry& entry) {
    cout << "#" << entry.id << " - " << entry.timestamp << " - " << entry.amount << " мл\n";
    if (!entry.note.empty()) cout << "   Заметка: " << entry.note << "\n";
}

int main() {
    WaterTracker tracker;
    tracker.loadFromFile();

    while (true) {
        cout << "\n===== ТРЕКЕР ВОДНОГО БАЛАНСА (C++) =====" << endl;
        cout << "1. Добавить запись\n";
        cout << "2. Просмотреть записи за сегодня\n";
        cout << "3. Просмотреть записи за неделю\n";
        cout << "4. Показать статистику\n";
        cout << "5. Установить дневную цель\n";
        cout << "6. Удалить запись\n";
        cout << "7. Сохранить в файл\n";
        cout << "8. Загрузить из файла\n";
        cout << "0. Выход\n";
        string choice = readString("Выберите действие: ");

        if (choice == "0") break;

        if (choice == "1") {
            int amount = readInt("Объём (мл): ");
            if (amount <= 0) {
                cout << "Объём должен быть положительным.\n";
                continue;
            }
            string note = readString("Заметка (необязательно): ");
            try {
                WaterEntry entry = tracker.addEntry(amount, note);
                cout << "Запись добавлена с ID " << entry.id << "\n";
            } catch (const exception& e) {
                cout << e.what() << "\n";
            }
        } else if (choice == "2") {
            auto entries = tracker.getTodayEntries();
            if (entries.empty()) {
                cout << "За сегодня записей нет.\n";
            } else {
                cout << "\nЗаписи за сегодня (" << entries.size() << "):\n";
                for (const auto& e : entries) printEntry(e);
            }
        } else if (choice == "3") {
            auto entries = tracker.getWeekEntries();
            if (entries.empty()) {
                cout << "За неделю записей нет.\n";
            } else {
                cout << "\nЗаписи за последние 7 дней (" << entries.size() << "):\n";
                for (const auto& e : entries) printEntry(e);
            }
        } else if (choice == "4") {
            auto stats = tracker.getStats();
            cout << "\n=== СТАТИСТИКА ===\n";
            cout << "Дневная цель: " << get<int>(stats["daily_goal"]) << " мл\n";
            cout << "Сегодня выпито: " << get<int>(stats["today_total"]) << " мл (" << get<int>(stats["today_count"]) << " записей)\n";
            cout << "Прогресс: " << fixed << setprecision(1) << get<double>(stats["progress"]) << "%\n";
            cout << "За неделю: " << get<int>(stats["week_total"]) << " мл (" << get<int>(stats["week_count"]) << " записей)\n";
            cout << "Среднее за день (за неделю): " << fixed << setprecision(1) << get<double>(stats["avg_daily"]) << " мл\n";
        } else if (choice == "5") {
            int goal = readInt("Новая дневная цель (мл): ");
            try {
                tracker.setDailyGoal(goal);
                cout << "Цель установлена: " << goal << " мл\n";
            } catch (const exception& e) {
                cout << e.what() << "\n";
            }
        } else if (choice == "6") {
            int id = readInt("Введите ID записи для удаления: ");
            if (tracker.deleteEntry(id)) {
                cout << "Запись удалена.\n";
            } else {
                cout << "Запись не найдена.\n";
            }
        } else if (choice == "7") {
            tracker.saveToFile();
            cout << "Сохранено.\n";
        } else if (choice == "8") {
            tracker.loadFromFile();
            cout << "Загружено.\n";
        } else {
            cout << "Неизвестная команда.\n";
        }
    }
    return 0;
}
