#include "DatParser.h"
#include <jni.h>
#include <android/asset_manager_jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_com_capmaster2023_footballempire_DatDbPlugin_nativeImportDat(JNIEnv* env, jobject, jobject assetManager, jstring sqlitePath) {
    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
    const char* pathChars = env->GetStringUTFChars(sqlitePath, nullptr);
    std::string path = pathChars ? pathChars : "";
    if (pathChars) env->ReleaseStringUTFChars(sqlitePath, pathChars);
    std::string out = DatParser::importAll(mgr, path);
    return env->NewStringUTF(out.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_capmaster2023_footballempire_DatDbPlugin_nativeInspectDat(JNIEnv* env, jobject, jobject assetManager, jstring assetPath, jint maxStrings) {
    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
    const char* p = env->GetStringUTFChars(assetPath, nullptr);
    std::string path = p ? p : "";
    if (p) env->ReleaseStringUTFChars(assetPath, p);
    std::string out = DatParser::inspect(mgr, path, static_cast<int>(maxStrings));
    return env->NewStringUTF(out.c_str());
}
