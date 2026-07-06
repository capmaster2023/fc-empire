#include <jni.h>
#include <string>
#include <mutex>
#include "match_engine.h"

using fcempire::MatchEngine;

static MatchEngine gEngine;
static std::mutex gMutex;

static std::string j2s(JNIEnv* env, jstring s) {
    if (!s) return {};
    const char* c = env->GetStringUTFChars(s, nullptr);
    std::string out = c ? c : "";
    if (c) env->ReleaseStringUTFChars(s, c);
    return out;
}

static jstring s2j(JNIEnv* env, const std::string& s) {
    return env->NewStringUTF(s.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_capmaster2023_footballempire_MatchEnginePlugin_nativeStartMatch(JNIEnv* env, jobject, jstring json) {
    std::lock_guard<std::mutex> lock(gMutex);
    return s2j(env, gEngine.startMatch(j2s(env, json)));
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_capmaster2023_footballempire_MatchEnginePlugin_nativeTick(JNIEnv* env, jobject, jdouble deltaMinutes) {
    std::lock_guard<std::mutex> lock(gMutex);
    return s2j(env, gEngine.tick(deltaMinutes));
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_capmaster2023_footballempire_MatchEnginePlugin_nativePause(JNIEnv* env, jobject) {
    std::lock_guard<std::mutex> lock(gMutex);
    return s2j(env, gEngine.pause());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_capmaster2023_footballempire_MatchEnginePlugin_nativeResume(JNIEnv* env, jobject) {
    std::lock_guard<std::mutex> lock(gMutex);
    return s2j(env, gEngine.resume());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_capmaster2023_footballempire_MatchEnginePlugin_nativeSetSpeed(JNIEnv* env, jobject, jdouble speed) {
    std::lock_guard<std::mutex> lock(gMutex);
    return s2j(env, gEngine.setSpeed(speed));
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_capmaster2023_footballempire_MatchEnginePlugin_nativeUpdateTactics(JNIEnv* env, jobject, jstring teamId, jstring json) {
    std::lock_guard<std::mutex> lock(gMutex);
    return s2j(env, gEngine.updateTactics(j2s(env, teamId), j2s(env, json)));
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_capmaster2023_footballempire_MatchEnginePlugin_nativeGetMatchState(JNIEnv* env, jobject) {
    std::lock_guard<std::mutex> lock(gMutex);
    return s2j(env, gEngine.getMatchState());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_capmaster2023_footballempire_MatchEnginePlugin_nativeStopMatch(JNIEnv* env, jobject) {
    std::lock_guard<std::mutex> lock(gMutex);
    return s2j(env, gEngine.stopMatch());
}
