#!/usr/bin/env bash
set -e

cat > android/settings.gradle <<'SETTINGS'
pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}

dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.PREFER_PROJECT)
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}

rootProject.name = 'Football Empire'
include ':app'
include ':capacitor-android'
project(':capacitor-android').projectDir = new File('../node_modules/@capacitor/android/capacitor')
include ':capacitor-cordova-android-plugins'
project(':capacitor-cordova-android-plugins').projectDir = new File('./capacitor-cordova-android-plugins/')
SETTINGS

cat > android/build.gradle <<'GRADLE'
plugins {
    id 'com.android.application' version '8.7.3' apply false
    id 'com.android.library' version '8.7.3' apply false
}

ext {
    compileSdkVersion = 35
    minSdkVersion = 23
    targetSdkVersion = 35
    androidxAppCompatVersion = '1.7.0'
    cordovaAndroidVersion = '10.1.1'
}
GRADLE

python3 - <<'PY'
from pathlib import Path
for p in [Path('android/app/capacitor.build.gradle'), Path('android/capacitor-cordova-android-plugins/build.gradle')]:
    if p.exists():
        s = p.read_text()
        s = s.replace('JavaVersion.VERSION_21', 'JavaVersion.VERSION_17')
        p.write_text(s)
PY

mkdir -p .github/workflows
cat > .github/workflows/build-apk.yml <<'WORKFLOW'
name: Build APK

on:
  push:
    branches: [ main ]
  workflow_dispatch:

jobs:
  build:
    runs-on: ubuntu-latest

    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Setup Node.js
        uses: actions/setup-node@v4
        with:
          node-version: 20

      - name: Setup Java 21
        uses: actions/setup-java@v4
        with:
          distribution: temurin
          java-version: 21

      - name: Setup Android SDK
        uses: android-actions/setup-android@v3

      - name: Install Android SDK packages
        run: |
          yes | sdkmanager --licenses >/dev/null || true
          sdkmanager "platforms;android-35" "build-tools;35.0.0" "ndk;26.1.10909125" "cmake;3.22.1"

      - name: Setup Gradle 8.10.2
        uses: gradle/actions/setup-gradle@v4
        with:
          gradle-version: 8.10.2

      - name: Install dependencies
        run: |
          rm -rf node_modules package-lock.json
          npm cache clean --force
          npm config set registry https://registry.npmjs.org/
          npm install --legacy-peer-deps --no-audit --no-fund --package-lock=false

      - name: Sync Capacitor web assets
        run: |
          npx cap sync android || npx cap copy android
          mkdir -p android/app/src/main/assets/public
          rsync -a --delete www/ android/app/src/main/assets/public/
          cp capacitor.config.json android/app/src/main/assets/capacitor.config.json

      - name: Build debug APK
        run: gradle -p android assembleDebug --stacktrace

      - name: Upload APK
        uses: actions/upload-artifact@v4
        with:
          name: fc-empire-native-dat-debug-apk
          path: android/app/build/outputs/apk/debug/*.apk
WORKFLOW

git add android/settings.gradle android/build.gradle android/app/capacitor.build.gradle android/capacitor-cordova-android-plugins/build.gradle .github/workflows/build-apk.yml
git commit -m "Fix Gradle repository mode for Capacitor Android" || true
git push
