#include "DatReader.h"
#include <android/asset_manager.h>
#include <cstring>
#include <utility>

DatReader::DatReader() = default;
DatReader::DatReader(std::vector<uint8_t> bytes) : data_(std::move(bytes)) {}

bool DatReader::readAsset(AAssetManager* mgr, const std::string& assetPath, std::vector<uint8_t>& out, std::string& error) {
    out.clear();
    if (!mgr) { error = "AAssetManager null"; return false; }
    AAsset* asset = AAssetManager_open(mgr, assetPath.c_str(), AASSET_MODE_BUFFER);
    if (!asset) { error = "asset introuvable: " + assetPath; return false; }
    const off_t len = AAsset_getLength(asset);
    if (len < 0) { AAsset_close(asset); error = "taille asset invalide"; return false; }
    out.resize(static_cast<size_t>(len));
    size_t readTotal = 0;
    while (readTotal < out.size()) {
        int r = AAsset_read(asset, out.data() + readTotal, out.size() - readTotal);
        if (r <= 0) break;
        readTotal += static_cast<size_t>(r);
    }
    AAsset_close(asset);
    if (readTotal != out.size()) {
        out.resize(readTotal);
        error = "lecture partielle: " + assetPath;
        return false;
    }
    return true;
}

bool DatReader::eof() const { return pos_ >= data_.size(); }
size_t DatReader::pos() const { return pos_; }
size_t DatReader::size() const { return data_.size(); }
bool DatReader::seek(size_t p) { if (p > data_.size()) return false; pos_ = p; return true; }
bool DatReader::skip(size_t n) { return seek(pos_ + n); }

bool DatReader::readU8(uint8_t& v) {
    if (pos_ + 1 > data_.size()) return false;
    v = data_[pos_++]; return true;
}
bool DatReader::readU16(uint16_t& v) {
    if (pos_ + 2 > data_.size()) return false;
    v = static_cast<uint16_t>(data_[pos_] | (data_[pos_ + 1] << 8)); pos_ += 2; return true;
}
bool DatReader::readU32(uint32_t& v) {
    if (pos_ + 4 > data_.size()) return false;
    v = static_cast<uint32_t>(data_[pos_]) | (static_cast<uint32_t>(data_[pos_ + 1]) << 8) |
        (static_cast<uint32_t>(data_[pos_ + 2]) << 16) | (static_cast<uint32_t>(data_[pos_ + 3]) << 24);
    pos_ += 4; return true;
}
bool DatReader::readI32(int32_t& v) { uint32_t u; if (!readU32(u)) return false; std::memcpy(&v, &u, sizeof(v)); return true; }
bool DatReader::readFloat(float& v) { uint32_t u; if (!readU32(u)) return false; std::memcpy(&v, &u, sizeof(v)); return true; }
bool DatReader::readBlock(size_t n, std::vector<uint8_t>& out) {
    if (pos_ + n > data_.size()) return false;
    out.assign(data_.begin() + static_cast<long>(pos_), data_.begin() + static_cast<long>(pos_ + n));
    pos_ += n; return true;
}
bool DatReader::readString(std::string& out, uint32_t maxLen) {
    uint32_t len = 0;
    const size_t start = pos_;
    if (!readU32(len)) return false;
    if (len > maxLen || pos_ + len > data_.size()) { pos_ = start; return false; }
    out.assign(reinterpret_cast<const char*>(data_.data() + pos_), len);
    pos_ += len;
    return true;
}
