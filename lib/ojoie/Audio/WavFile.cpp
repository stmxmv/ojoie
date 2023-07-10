//
// Created by Aleudillonam on 7/30/2022.
//

#include "Core/Exception.hpp"
#include "Utility/Log.h"
#include "Audio/WavFile.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <memory>
#include <format>
#include <filesystem>

namespace AN {


bool WavFile::init(const char *filePath) {

    class FileError : public Exception {
    public:
#ifdef __cpp_lib_source_location
        FileError(const char *msg, const std::source_location location = std::source_location::current()) noexcept
            : Exception(std::format("FileError: {}", msg).c_str(), location) {}
#else
        explicit FileError(const char *msg) noexcept
            : Exception(std::format("WavFileError: {}", msg).c_str()) {}
#endif
    };

//    /// on windows need to convert utf-8 to utf-16, or the ifstream will convert it by ansi which is wrong
//#ifdef _WIN32
//    std::wstring wFilePath;
//    wFilePath.resize(MultiByteToWideChar(CP_UTF8, 0, filePath, (int)strlen(filePath) + 1, nullptr, 0));
//    MultiByteToWideChar(CP_UTF8, 0, filePath, (int)strlen(filePath) + 1, wFilePath.data(), (int)wFilePath.size());
//
//#define filePath wFilePath.c_str()
//#define u8FilePath filePath
//#else
//#define u8FilePath filePath
//#endif

    /// with c++ 20 just cast to char8_t seems to solve the problem ！
    if (!std::filesystem::exists((const char8_t *)filePath) || std::filesystem::is_directory((const char8_t *)filePath)) {
        ANLog("Wav File not found at %s", filePath);
        return false;
    }
    
    try {
        {
            file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            try {
                file.open((const char8_t *)filePath, std::ios::binary);
            } catch (const std::exception &e) {
                throw FileError(std::format("error opening file: {}", e.what()).c_str());
            }

//#undef filePath
//#undef u8FilePath

            file.read((char *)&riff, sizeof riff);

            if (riff.chuckID != 'FFIR') {
                throw FileError("bad fourCC");
            }

            if (riff.chuckSize + 8 <= 44) {
                throw FileError("file too small");
            }

            if (riff.format != 'EVAW') {
                throw FileError("format not WAVE");
            }
        }

        unsigned int fileSize = riff.chuckSize + 8;
        //look for 'fmt ' chunk id
        bool bFilledFormat = false;
        for (unsigned int i = 12; i < fileSize;) {

            file.seekg(i, std::ios::beg);
            int subChuckID;
            file.read((char *)&subChuckID, sizeof subChuckID);
            if (subChuckID == ' tmf') {
                file.seekg(4, std::ios::cur); // skip the sub chuck size
                file.read((char *)&format_chuck, sizeof format_chuck);
                bFilledFormat = true;
                break;
            }
            // chunk size + size entry size + chunk id entry size + word padding
            int sub_chuck_size;
            file.read((char *)&sub_chuck_size, sizeof sub_chuck_size);
            i += (sub_chuck_size + 9) & 0xFFFFFFFE;
        }

        if (!bFilledFormat) {
            throw FileError("fmt chunk not found");
        }

        //look for 'data' chunk id
        bool bFoundData = false;
        for (unsigned int i = 12; i < fileSize;) {
            file.seekg(i, std::ios::beg);
            unsigned int subChuckID, sub_chuck_size;
            file.read((char *)&subChuckID, sizeof subChuckID);
            file.read((char *)&sub_chuck_size, sizeof sub_chuck_size);

            if (subChuckID == 'atad') {
                dataOffset = i + 8;
                dataSize = sub_chuck_size;
                bFoundData = true;
                break;
            }

            // chunk size + size entry size + chunk id entry size + word padding
            i += (sub_chuck_size + 9) & 0xFFFFFFFE;
        }
        if (!bFoundData) {
            throw FileError("data chunk not found");
        }
    } catch (const Exception &e) {

        ANLog("%s", e.what());
        return false;

    } catch (const std::exception &e) {
        throw FileError(e.what());
    }

    return true;
}

bool WavFile::read(unsigned char *out) {
    file.seekg(dataOffset, std::ios::beg);
    return (bool)file.read((char *)out, dataSize);
}

unsigned int WavFile::readChuck(unsigned char *out, unsigned int chuck_size) {
    if (currentPos >= dataSize) {
        return 0;
    }

    /// align
    chuck_size = chuck_size - (chuck_size % format_chuck.block_align);
    
    unsigned int read_size = chuck_size;
    if (currentPos + chuck_size > dataSize) {
        read_size = dataSize - currentPos;
    }
    file.seekg(currentPos + dataOffset, std::ios::beg);
    currentPos += read_size;

    if (file.read((char *)out, read_size)) {
        return read_size;
    }
    return 0;
}

void WavFile::rewind() {
    currentPos = 0;
}




}