#pragma once
#include <android/asset_manager.h>
#ifndef FCEMPIRE_HAS_SQLITE
#define FCEMPIRE_HAS_SQLITE 1
#endif
#if FCEMPIRE_HAS_SQLITE
#include <sqlite3.h>
#else
typedef struct sqlite3 sqlite3;
#endif
#include <cstdint>
#include <string>
#include <vector>

struct StringHit {
    size_t pos;
    std::string value;
};

struct DatHeader {
    bool valid = false;
    uint32_t declaredRecords = 0;
};

class DatParser {
public:
    static std::string importAll(AAssetManager* mgr, const std::string& sqlitePath);
    static std::string inspect(AAssetManager* mgr, const std::string& assetPath, int maxStrings);

private:
    static DatHeader header(const std::vector<uint8_t>& data);
    static std::vector<StringHit> scanStrings(const std::vector<uint8_t>& data, size_t limit = 0, uint32_t maxLen = 96);
    static int continentIdForCountry(const std::string& name);
    static bool isCode(const std::string& s);
    static std::string sqlText(const std::string& s);
    static void exec(sqlite3* db, const std::string& sql);
    static void insertDebugFile(sqlite3* db, const std::string& file, size_t bytes, uint32_t records, size_t strings, const std::string& status);
    static void insertDebugStrings(sqlite3* db, const std::string& file, const std::vector<StringHit>& hits, int maxRows);
    static void importContinents(sqlite3* db, const std::vector<uint8_t>& data);
    static void importNations(sqlite3* db, const std::vector<uint8_t>& data);
    static void importCompetitions(sqlite3* db, const std::vector<uint8_t>& data);
    static void importClubs(sqlite3* db, const std::vector<uint8_t>& data);
};
