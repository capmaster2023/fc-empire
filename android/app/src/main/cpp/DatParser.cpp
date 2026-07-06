#include "DatParser.h"
#include "DatReader.h"
#include <android/log.h>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <set>

#define LOG_TAG "DatParser"
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

static std::string jsonEscape(const std::string& in) {
    std::string out;
    out.reserve(in.size() + 8);
    for (unsigned char c : in) {
        if (c == '\\') out += "\\\\";
        else if (c == '"') out += "\\\"";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c >= 32) out += static_cast<char>(c);
    }
    return out;
}

static uint32_t le32(const std::vector<uint8_t>& d, size_t p) {
    if (p + 4 > d.size()) return 0;
    return static_cast<uint32_t>(d[p]) |
           (static_cast<uint32_t>(d[p + 1]) << 8) |
           (static_cast<uint32_t>(d[p + 2]) << 16) |
           (static_cast<uint32_t>(d[p + 3]) << 24);
}

DatHeader DatParser::header(const std::vector<uint8_t>& data) {
    DatHeader h;
    if (data.size() >= 12 && data[2] == 't' && data[3] == 'a' && data[4] == 'd' && data[5] == '.') {
        h.valid = true;
        h.declaredRecords = le32(data, 8);
    }
    return h;
}

bool DatParser::isCode(const std::string& s) {
    if (s.size() < 2 || s.size() > 6) return false;
    int upper = 0;
    for (unsigned char c : s) {
        if (std::isupper(c) || std::isdigit(c) || c == '&') upper++;
        else return false;
    }
    return upper >= 2;
}

std::vector<StringHit> DatParser::scanStrings(const std::vector<uint8_t>& data, size_t limit, uint32_t maxLen) {
    std::vector<StringHit> out;
    const size_t end = limit > 0 ? std::min(limit, data.size()) : data.size();
    size_t i = 0;
    while (i + 4 <= end) {
        uint32_t len = le32(data, i);
        if (len > 0 && len <= maxLen && i + 4 + len <= data.size()) {
            const uint8_t* p = data.data() + i + 4;
            bool ok = true;
            int alpha = 0;
            for (uint32_t k = 0; k < len; ++k) {
                unsigned char c = p[k];
                if (c < 32 || c == 127) { ok = false; break; }
                if (std::isalpha(c)) alpha++;
            }
            if (ok && alpha > 0) {
                out.push_back({i, std::string(reinterpret_cast<const char*>(p), len)});
                i += 4 + len;
                if (i < data.size() && (data[i] == 0x00 || data[i] == 0xff)) i++;
                continue;
            }
        }
        ++i;
    }
    return out;
}

int DatParser::continentIdForCountry(const std::string& name) {
    static const std::set<std::string> africa = {"Algeria","Angola","Benin","Botswana","Burkina Faso","Burundi","Cameroon","Cape Verde","Central African Rep.","Chad","Comoros","Congo","DR Congo","Côte d'Ivoire","Ivory Coast","Djibouti","Egypt","Equatorial Guinea","Eritrea","Ethiopia","Gabon","Gambia","Ghana","Guinea","Guinea-Bissau","Kenya","Lesotho","Liberia","Libya","Madagascar","Malawi","Mali","Mauritania","Mauritius","Morocco","Mozambique","Namibia","Niger","Nigeria","Rwanda","Senegal","Seychelles","Sierra Leone","Somalia","South Africa","Sudan","Tanzania","Togo","Tunisia","Uganda","Zambia","Zimbabwe"};
    static const std::set<std::string> europe = {"Albania","Andorra","Armenia","Austria","Azerbaijan","Belarus","Belgium","Bosnia and Herzegovina","Bulgaria","Croatia","Cyprus","Czechia","Denmark","England","Estonia","Faroe Islands","Finland","France","Georgia","Germany","Greece","Hungary","Iceland","Israel","Italy","Latvia","Lithuania","Luxembourg","Malta","Moldova","Montenegro","Netherlands","North Macedonia","Northern Ireland","Norway","Poland","Portugal","Republic of Ireland","Romania","Russia","Scotland","Serbia","Slovakia","Slovenia","Spain","Sweden","Switzerland","Turkey","Ukraine","Wales"};
    static const std::set<std::string> asia = {"Australia","Bahrain","China","India","Indonesia","Iran","Iraq","Japan","Jordan","Korea Republic","South Korea","Kuwait","Malaysia","Oman","Qatar","Saudi Arabia","Singapore","Thailand","United Arab Emirates","Vietnam"};
    static const std::set<std::string> nam = {"Canada","United States","Mexico","Costa Rica","Haiti","Honduras","Jamaica","Panama","Trinidad and Tobago"};
    static const std::set<std::string> oce = {"New Zealand","Fiji","Papua New Guinea","Tahiti","Samoa"};
    static const std::set<std::string> sam = {"Argentina","Bolivia","Brazil","Chile","Colombia","Ecuador","Paraguay","Peru","Uruguay","Venezuela"};
    if (africa.count(name)) return 0;
    if (asia.count(name)) return 1;
    if (europe.count(name)) return 2;
    if (nam.count(name)) return 3;
    if (oce.count(name)) return 4;
    if (sam.count(name)) return 5;
    return 6;
}

#if FCEMPIRE_HAS_SQLITE
std::string DatParser::sqlText(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "''";
        else if (static_cast<unsigned char>(c) >= 32 || c == '\t') out += c;
    }
    out += "'";
    return out;
}

void DatParser::exec(sqlite3* db, const std::string& sql) {
    char* err = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        LOGW("sqlite exec failed: %s", err ? err : "?");
        sqlite3_free(err);
    }
}

void DatParser::insertDebugFile(sqlite3* db, const std::string& file, size_t bytes, uint32_t records, size_t strings, const std::string& status) {
    exec(db, "INSERT OR REPLACE INTO debug_dat_files(file,bytes,declared_records,extracted_strings,status) VALUES(" + sqlText(file) + "," + std::to_string(bytes) + "," + std::to_string(records) + "," + std::to_string(strings) + "," + sqlText(status) + ")");
}

void DatParser::insertDebugStrings(sqlite3* db, const std::string& file, const std::vector<StringHit>& hits, int maxRows) {
    exec(db, "DELETE FROM debug_strings WHERE file=" + sqlText(file));
    int n = std::min<int>(maxRows, static_cast<int>(hits.size()));
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db, "INSERT INTO debug_strings(file,idx,value,pos) VALUES(?,?,?,?)", -1, &st, nullptr);
    for (int i = 0; i < n; ++i) {
        sqlite3_bind_text(st, 1, file.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(st, 2, i);
        sqlite3_bind_text(st, 3, hits[i].value.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(st, 4, static_cast<sqlite3_int64>(hits[i].pos));
        sqlite3_step(st);
        sqlite3_reset(st);
        sqlite3_clear_bindings(st);
    }
    sqlite3_finalize(st);
}

void DatParser::importContinents(sqlite3* db, const std::vector<uint8_t>& data) {
    auto hits = scanStrings(data, 0, 64);
    exec(db, "DELETE FROM continents");
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO continents(id,name) VALUES(?,?)", -1, &st, nullptr);
    int id = 0;
    for (size_t i = 0; i < hits.size();) {
        std::string name = hits[i].value;
        if (!isCode(name)) {
            sqlite3_bind_int(st, 1, id++);
            sqlite3_bind_text(st, 2, name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(st);
            sqlite3_reset(st);
            sqlite3_clear_bindings(st);
            i += 3;
        } else {
            ++i;
        }
    }
    sqlite3_finalize(st);
}

void DatParser::importNations(sqlite3* db, const std::vector<uint8_t>& data) {
    auto hits = scanStrings(data, 0, 96);
    exec(db, "DELETE FROM nations");
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO nations(id,name,continent_id,code) VALUES(?,?,?,?)", -1, &st, nullptr);
    int id = 0;
    for (size_t i = 0; i < hits.size();) {
        std::string name = hits[i].value;
        if (isCode(name)) { ++i; continue; }
        std::string code;
        size_t j = i + 1;
        if (j < hits.size() && !isCode(hits[j].value)) j++;
        if (j < hits.size() && isCode(hits[j].value)) code = hits[j].value;
        sqlite3_bind_int(st, 1, id++);
        sqlite3_bind_text(st, 2, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(st, 3, continentIdForCountry(name));
        sqlite3_bind_text(st, 4, code.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(st);
        sqlite3_reset(st);
        sqlite3_clear_bindings(st);
        i = code.empty() ? i + 1 : j + 1;
    }
    sqlite3_finalize(st);
}

void DatParser::importCompetitions(sqlite3* db, const std::vector<uint8_t>& data) {
    auto hits = scanStrings(data, 0, 128);
    exec(db, "DELETE FROM competitions");
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO competitions(id,name,nation_id,level,reputation,code) VALUES(?,?,?,?,?,?)", -1, &st, nullptr);
    int id = 0;
    for (size_t i = 0; i < hits.size() && id < 20000;) {
        std::string name = hits[i].value;
        if (isCode(name)) { ++i; continue; }
        std::string code;
        size_t j = i + 1;
        if (j < hits.size() && !isCode(hits[j].value)) j++;
        if (j < hits.size() && isCode(hits[j].value)) code = hits[j].value;
        sqlite3_bind_int(st, 1, id++);
        sqlite3_bind_text(st, 2, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(st, 3, 0);
        sqlite3_bind_int(st, 4, 1);
        sqlite3_bind_double(st, 5, 0.0);
        sqlite3_bind_text(st, 6, code.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(st);
        sqlite3_reset(st);
        sqlite3_clear_bindings(st);
        i = code.empty() ? i + 1 : j + 1;
    }
    sqlite3_finalize(st);
}

void DatParser::importClubs(sqlite3* db, const std::vector<uint8_t>& data) {
    auto hits = scanStrings(data, 0, 128);
    exec(db, "DELETE FROM clubs");
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO clubs(id,name,nation_id,competition_id,reputation,budget,stadium,code) VALUES(?,?,?,?,?,?,?,?)", -1, &st, nullptr);
    int id = 0;
    for (size_t i = 0; i < hits.size() && id < 60000;) {
        std::string name = hits[i].value;
        if (isCode(name)) { ++i; continue; }
        std::string code;
        size_t j = i + 1;
        if (j < hits.size() && !isCode(hits[j].value)) j++;
        if (j < hits.size() && isCode(hits[j].value)) code = hits[j].value;
        sqlite3_bind_int(st, 1, id++);
        sqlite3_bind_text(st, 2, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(st, 3, 0);
        sqlite3_bind_int(st, 4, 0);
        sqlite3_bind_double(st, 5, 0.0);
        sqlite3_bind_int64(st, 6, 0);
        sqlite3_bind_text(st, 7, "", -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(st, 8, code.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(st);
        sqlite3_reset(st);
        sqlite3_clear_bindings(st);
        i = code.empty() ? i + 1 : j + 1;
    }
    sqlite3_finalize(st);
}

std::string DatParser::importAll(AAssetManager* mgr, const std::string& sqlitePath) {
    sqlite3* db = nullptr;
    if (sqlite3_open(sqlitePath.c_str(), &db) != SQLITE_OK) return "{\"ok\":false,\"error\":\"sqlite_open failed\"}";
    exec(db, "PRAGMA journal_mode=WAL");
    exec(db, "PRAGMA synchronous=NORMAL");
    exec(db, "BEGIN");

    const std::vector<std::string> files = {"continent.dat","nation.dat","competition.dat","club.dat","players.dat","people.dat","player_history.dat","starting_contracts.dat"};
    std::ostringstream json;
    json << "{\"ok\":true,\"sqliteNative\":true,\"files\":[";
    bool first = true;
    for (const auto& f : files) {
        std::vector<uint8_t> bytes;
        std::string err;
        if (!DatReader::readAsset(mgr, "db/" + f, bytes, err)) {
            insertDebugFile(db, f, 0, 0, 0, err);
            continue;
        }
        auto h = header(bytes);
        auto hits = scanStrings(bytes, (f == "players.dat" || f == "people.dat" || f == "player_history.dat" || f == "starting_contracts.dat") ? 512 * 1024 : 0, 128);
        insertDebugFile(db, f, bytes.size(), h.declaredRecords, hits.size(), h.valid ? "ok" : "magic_inconnue");
        insertDebugStrings(db, f, hits, 250);
        try {
            if (f == "continent.dat") importContinents(db, bytes);
            else if (f == "nation.dat") importNations(db, bytes);
            else if (f == "competition.dat") importCompetitions(db, bytes);
            else if (f == "club.dat") importClubs(db, bytes);
        } catch (...) {
            LOGW("import tolerant failed for %s", f.c_str());
        }
        if (!first) json << ",";
        first = false;
        json << "{\"file\":\"" << f << "\",\"bytes\":" << bytes.size() << ",\"declaredRecords\":" << h.declaredRecords << ",\"strings\":" << hits.size() << "}";
    }
    exec(db, "COMMIT");
    exec(db, "CREATE INDEX IF NOT EXISTS countries_by_continent ON nations(continent_id, name)");
    exec(db, "CREATE INDEX IF NOT EXISTS competitions_by_country ON competitions(nation_id, level, reputation)");
    exec(db, "CREATE INDEX IF NOT EXISTS clubs_by_competition ON clubs(competition_id, reputation)");
    exec(db, "CREATE INDEX IF NOT EXISTS players_by_club ON players(club_id, rating)");
    exec(db, "CREATE INDEX IF NOT EXISTS search_players_by_name ON players(name)");
    sqlite3_close(db);
    json << "],\"note\":\"Import natif tolerant. Les fichiers non compris restent inspectables dans debug_strings.\"}";
    return json.str();
}
#else
std::string DatParser::importAll(AAssetManager* mgr, const std::string& sqlitePath) {
    (void)sqlitePath;
    const std::vector<std::string> files = {"continent.dat","nation.dat","competition.dat","club.dat","players.dat","people.dat","player_history.dat","starting_contracts.dat"};
    std::ostringstream json;
    json << "{\"ok\":true,\"sqliteNative\":false,\"files\":[";
    bool first = true;
    for (const auto& f : files) {
        std::vector<uint8_t> bytes;
        std::string err;
        if (!DatReader::readAsset(mgr, "db/" + f, bytes, err)) continue;
        auto h = header(bytes);
        auto hits = scanStrings(bytes, (f == "players.dat" || f == "people.dat") ? 512 * 1024 : 0, 128);
        if (!first) json << ",";
        first = false;
        json << "{\"file\":\"" << f << "\",\"bytes\":" << bytes.size() << ",\"declaredRecords\":" << h.declaredRecords << ",\"strings\":" << hits.size() << "}";
    }
    json << "],\"note\":\"SQLite NDK indisponible; cache SQLite Android conserve cote Java.\"}";
    return json.str();
}
#endif

std::string DatParser::inspect(AAssetManager* mgr, const std::string& assetPath, int maxStrings) {
    std::vector<uint8_t> bytes;
    std::string err;
    if (!DatReader::readAsset(mgr, assetPath, bytes, err)) return "{\"ok\":false,\"error\":\"" + jsonEscape(err) + "\"}";
    auto h = header(bytes);
    auto hits = scanStrings(bytes, 0, 128);
    std::ostringstream json;
    json << "{\"ok\":true,\"bytes\":" << bytes.size() << ",\"declaredRecords\":" << h.declaredRecords << ",\"strings\":[";
    int n = std::min<int>(maxStrings, static_cast<int>(hits.size()));
    for (int i = 0; i < n; ++i) {
        if (i) json << ",";
        json << "{\"pos\":" << hits[i].pos << ",\"value\":\"" << jsonEscape(hits[i].value) << "\"}";
    }
    json << "]}";
    return json.str();
}
