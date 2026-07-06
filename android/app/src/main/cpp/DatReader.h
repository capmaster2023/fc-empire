#pragma once
#include <android/asset_manager.h>
#include <cstdint>
#include <string>
#include <vector>

class DatReader {
public:
    DatReader();
    explicit DatReader(std::vector<uint8_t> bytes);

    static bool readAsset(AAssetManager* mgr, const std::string& assetPath, std::vector<uint8_t>& out, std::string& error);

    bool eof() const;
    size_t pos() const;
    size_t size() const;
    bool seek(size_t p);
    bool skip(size_t n);

    bool readU8(uint8_t& v);
    bool readU16(uint16_t& v);
    bool readU32(uint32_t& v);
    bool readI32(int32_t& v);
    bool readFloat(float& v);
    bool readBlock(size_t n, std::vector<uint8_t>& out);
    bool readString(std::string& out, uint32_t maxLen = 4096);

    const std::vector<uint8_t>& bytes() const { return data_; }

private:
    std::vector<uint8_t> data_;
    size_t pos_ = 0;
};
