package com.capmaster2023.footballempire;

import android.content.Context;
import android.content.res.AssetManager;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

import com.getcapacitor.JSArray;
import com.getcapacitor.JSObject;
import com.getcapacitor.Plugin;
import com.getcapacitor.PluginCall;
import com.getcapacitor.PluginMethod;
import com.getcapacitor.annotation.CapacitorPlugin;

import org.json.JSONException;

import java.io.File;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

@CapacitorPlugin(name = "DatDb")
public class DatDbPlugin extends Plugin {
    private static final String TAG = "DatDbPlugin";
    private static final ExecutorService EXECUTOR = Executors.newSingleThreadExecutor();
    private NativeCache cache;

    static {
        try {
            System.loadLibrary("datdb");
        } catch (Throwable t) {
            Log.e(TAG, "Impossible de charger libdatdb.so", t);
        }
    }

    private native String nativeImportDat(AssetManager assetManager, String sqlitePath);
    private native String nativeInspectDat(AssetManager assetManager, String assetPath, int maxStrings);

    @Override
    public void load() {
        cache = new NativeCache(getContext());
    }

    @PluginMethod
    public void loadDatabase(PluginCall call) {
        EXECUTOR.execute(() -> {
            JSObject ret = new JSObject();
            try {
                SQLiteDatabase db = cache.getWritableDatabase();
                cache.ensureSchema(db);
                File dbFile = getContext().getDatabasePath(NativeCache.DB_NAME);
                String nativeJson = nativeImportDat(getContext().getAssets(), dbFile.getAbsolutePath());
                ret.put("ok", true);
                ret.put("cache", dbFile.getAbsolutePath());
                ret.put("native", nativeJson == null ? "{}" : nativeJson);
                ret.put("continents", cache.count("continents"));
                ret.put("nations", cache.count("nations"));
                ret.put("competitions", cache.count("competitions"));
                ret.put("clubs", cache.count("clubs"));
                ret.put("players", cache.count("players"));
                call.resolve(ret);
            } catch (Throwable t) {
                Log.e(TAG, "loadDatabase failed", t);
                ret.put("ok", false);
                ret.put("error", t.getMessage());
                call.resolve(ret);
            }
        });
    }

    @PluginMethod
    public void inspectFile(PluginCall call) {
        String file = call.getString("file", "continent.dat");
        int max = call.getInt("maxStrings", 120);
        EXECUTOR.execute(() -> {
            JSObject ret = new JSObject();
            try {
                ret.put("ok", true);
                ret.put("file", file);
                ret.put("result", nativeInspectDat(getContext().getAssets(), "db/" + file, max));
            } catch (Throwable t) {
                ret.put("ok", false);
                ret.put("error", t.getMessage());
            }
            call.resolve(ret);
        });
    }

    @PluginMethod public void getContinents(PluginCall call) { resolveRows(call, "continents", null, null, "name ASC", 200, 0); }

    @PluginMethod
    public void getCountriesByContinent(PluginCall call) {
        long id = readId(call, "continentId", "id");
        resolveRows(call, "nations", "continent_id=?", new String[]{String.valueOf(id)}, "name ASC", 500, 0);
    }

    @PluginMethod
    public void getCompetitionsByCountry(PluginCall call) {
        long id = readId(call, "countryId", "nationId");
        resolveRows(call, "competitions", "nation_id=?", new String[]{String.valueOf(id)}, "level ASC, reputation DESC, name ASC", 500, 0);
    }

    @PluginMethod
    public void getClubsByCompetition(PluginCall call) {
        long id = readId(call, "competitionId", "id");
        resolveRows(call, "clubs", "competition_id=?", new String[]{String.valueOf(id)}, "reputation DESC, name ASC", call.getInt("limit", 500), call.getInt("offset", 0));
    }

    @PluginMethod
    public void getPlayersByClub(PluginCall call) {
        long id = readId(call, "clubId", "id");
        resolveRows(call, "players", "club_id=?", new String[]{String.valueOf(id)}, "rating DESC, potential DESC, name ASC", call.getInt("limit", 80), call.getInt("offset", 0));
    }

    @PluginMethod
    public void searchPlayers(PluginCall call) {
        String q = call.getString("query", "").trim();
        int limit = call.getInt("limit", 80);
        int offset = call.getInt("offset", 0);
        resolveRows(call, "players", "name LIKE ?", new String[]{"%" + q + "%"}, "rating DESC, name ASC", limit, offset);
    }

    @PluginMethod public void getPlayer(PluginCall call) { resolveOne(call, "players", readId(call, "playerId", "id")); }
    @PluginMethod public void getClub(PluginCall call) { resolveOne(call, "clubs", readId(call, "clubId", "id")); }
    @PluginMethod public void getCompetition(PluginCall call) { resolveOne(call, "competitions", readId(call, "competitionId", "id")); }
    @PluginMethod public void getNation(PluginCall call) { resolveOne(call, "nations", readId(call, "nationId", "id")); }

    private long readId(PluginCall call, String primary, String fallback) {
        Object v = call.getData().has(primary) ? call.getData().opt(primary) : call.getData().opt(fallback);
        if (v instanceof Number) return ((Number) v).longValue();
        try { return Long.parseLong(String.valueOf(v)); } catch (Exception e) { return 0L; }
    }

    private void resolveOne(PluginCall call, String table, long id) {
        resolveRows(call, table, "id=?", new String[]{String.valueOf(id)}, null, 1, 0);
    }

    private void resolveRows(PluginCall call, String table, String where, String[] args, String order, int limit, int offset) {
        EXECUTOR.execute(() -> {
            JSObject ret = new JSObject();
            try {
                SQLiteDatabase db = cache.getReadableDatabase();
                JSArray rows = new JSArray();
                Cursor c = db.query(table, null, where, args, null, null, order, Math.max(1, Math.min(limit, 1000)) + " OFFSET " + Math.max(0, offset));
                try {
                    while (c.moveToNext()) rows.put(cursorToObject(c));
                } finally {
                    c.close();
                }
                ret.put("ok", true);
                ret.put("rows", rows);
                ret.put("count", rows.length());
                call.resolve(ret);
            } catch (Throwable t) {
                Log.e(TAG, "query failed: " + table, t);
                ret.put("ok", false);
                ret.put("error", t.getMessage());
                try { ret.put("rows", new JSArray()); } catch (Exception ignored) {}
                call.resolve(ret);
            }
        });
    }

    private JSObject cursorToObject(Cursor c) throws JSONException {
        JSObject o = new JSObject();
        for (int i = 0; i < c.getColumnCount(); i++) {
            String name = c.getColumnName(i);
            switch (c.getType(i)) {
                case Cursor.FIELD_TYPE_INTEGER: o.put(name, c.getLong(i)); break;
                case Cursor.FIELD_TYPE_FLOAT: o.put(name, c.getDouble(i)); break;
                case Cursor.FIELD_TYPE_NULL: o.put(name, null); break;
                default: o.put(name, c.getString(i)); break;
            }
        }
        return o;
    }

    public static class NativeCache extends SQLiteOpenHelper {
        public static final String DB_NAME = "fc_empire_native_dat.db";
        public NativeCache(Context context) { super(context, DB_NAME, null, 1); }
        @Override public void onCreate(SQLiteDatabase db) { ensureSchema(db); }
        @Override public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) { ensureSchema(db); }

        public void ensureSchema(SQLiteDatabase db) {
            db.execSQL("CREATE TABLE IF NOT EXISTS continents(id INTEGER PRIMARY KEY, name TEXT NOT NULL UNIQUE)");
            db.execSQL("CREATE TABLE IF NOT EXISTS nations(id INTEGER PRIMARY KEY, name TEXT NOT NULL, continent_id INTEGER DEFAULT 6, code TEXT)");
            db.execSQL("CREATE TABLE IF NOT EXISTS competitions(id INTEGER PRIMARY KEY, name TEXT NOT NULL, nation_id INTEGER DEFAULT 0, level INTEGER DEFAULT 1, reputation REAL DEFAULT 0, code TEXT)");
            db.execSQL("CREATE TABLE IF NOT EXISTS clubs(id INTEGER PRIMARY KEY, name TEXT NOT NULL, nation_id INTEGER DEFAULT 0, competition_id INTEGER DEFAULT 0, reputation REAL DEFAULT 0, budget INTEGER DEFAULT 0, stadium TEXT, code TEXT)");
            db.execSQL("CREATE TABLE IF NOT EXISTS people(id INTEGER PRIMARY KEY, first_name TEXT, last_name TEXT, birth_date TEXT, nationality_id INTEGER DEFAULT 0)");
            db.execSQL("CREATE TABLE IF NOT EXISTS players(id INTEGER PRIMARY KEY, person_id INTEGER DEFAULT 0, name TEXT, age INTEGER DEFAULT 0, nationality_id INTEGER DEFAULT 0, club_id INTEGER DEFAULT 0, position TEXT, rating INTEGER DEFAULT 0, potential INTEGER DEFAULT 0, value INTEGER DEFAULT 0, wage INTEGER DEFAULT 0)");
            db.execSQL("CREATE TABLE IF NOT EXISTS debug_dat_files(file TEXT PRIMARY KEY, bytes INTEGER, declared_records INTEGER, extracted_strings INTEGER, status TEXT)");
            db.execSQL("CREATE TABLE IF NOT EXISTS debug_strings(file TEXT, idx INTEGER, value TEXT, pos INTEGER)");
            db.execSQL("CREATE INDEX IF NOT EXISTS countries_by_continent ON nations(continent_id, name)");
            db.execSQL("CREATE INDEX IF NOT EXISTS competitions_by_country ON competitions(nation_id, level, reputation)");
            db.execSQL("CREATE INDEX IF NOT EXISTS clubs_by_competition ON clubs(competition_id, reputation)");
            db.execSQL("CREATE INDEX IF NOT EXISTS players_by_club ON players(club_id, rating)");
            db.execSQL("CREATE INDEX IF NOT EXISTS search_players_by_name ON players(name)");
        }

        public int count(String table) {
            SQLiteDatabase db = getReadableDatabase();
            Cursor c = db.rawQuery(String.format(Locale.US, "SELECT COUNT(*) FROM %s", table), null);
            try { return c.moveToFirst() ? c.getInt(0) : 0; }
            finally { c.close(); }
        }
    }
}
