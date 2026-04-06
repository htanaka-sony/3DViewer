#ifndef MEMORYMAPPEDFILE_H
#define MEMORYMAPPEDFILE_H

#include <windows.h>
#include <algorithm>
#include <iostream>
#include <new>    // std::nothrow
#include <string>

class MemoryMappedFile {
public:
    MemoryMappedFile(const std::wstring& filename, bool nouse_memory_mapping = true)
        : hFile(INVALID_HANDLE_VALUE)
        , hMapping(NULL)
        , mapView(NULL)
        , buffer(nullptr)
        , fileSize(0)
        , useMemoryMapping(false)
        , valid(false)
        , m_path(filename)
        , m_nouse_memory_mapping(nouse_memory_mapping)
    {
    }

    MemoryMappedFile()
        : hFile(INVALID_HANDLE_VALUE)
        , hMapping(NULL)
        , mapView(NULL)
        , buffer(nullptr)
        , fileSize(0)
        , useMemoryMapping(false)
        , valid(false)
        , m_path(L"")
    {
    }

    ~MemoryMappedFile() { closeFile(); }

    bool isValid() const { return valid; }

    float getFloatAt(size_t offset)
    {
        if (!isValid()) return 0.0f;
        if (offset + sizeof(float) > fileSize) return 0.0f;

        if (useMemoryMapping) {
            return *reinterpret_cast<float*>(static_cast<char*>(mapView) + offset);
        }
        else {
            return *reinterpret_cast<float*>(buffer + offset);
        }
    }

    float* basePointer()
    {
        if (useMemoryMapping) {
            return reinterpret_cast<float*>(static_cast<char*>(mapView));
        }
        else {
            return reinterpret_cast<float*>(buffer);
        }
    }

    char* getPointer(size_t offset)
    {
        if (!isValid()) return nullptr;
        if (offset > fileSize) return nullptr;

        if (useMemoryMapping) {
            return static_cast<char*>(mapView) + offset;
        }
        else {
            return buffer + offset;
        }
    }

    short getShortAt(size_t offset)
    {
        if (!isValid()) return 0;
        if (offset + sizeof(short) > fileSize) return 0;

        if (useMemoryMapping) {
            return *reinterpret_cast<short*>(static_cast<char*>(mapView) + offset);
        }
        else {
            return *reinterpret_cast<short*>(buffer + offset);
        }
    }

    char getCharAt(size_t offset)
    {
        if (!isValid()) return 0;
        if (offset + sizeof(char) > fileSize) return 0;

        if (useMemoryMapping) {
            return *(static_cast<char*>(mapView) + offset);
        }
        else {
            return *(buffer + offset);
        }
    }

    const std::wstring& filePath() const { return m_path; }

    bool openFile() { return openFile(m_path); }

private:
    HANDLE             hFile;
    HANDLE             hMapping;
    void*              mapView;
    char*              buffer;
    size_t             fileSize;
    bool               useMemoryMapping;
    bool               valid;
    bool               m_nouse_memory_mapping;
    const std::wstring m_path;

    bool openFile(const std::wstring& filename)
    {
        hFile = CreateFileW(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                            NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            valid = false;
            return false;
        }

        LARGE_INTEGER fileSize_li;
        if (!GetFileSizeEx(hFile, &fileSize_li)) {
            CloseHandle(hFile);
            valid = false;
            return false;
        }
        fileSize = static_cast<size_t>(fileSize_li.QuadPart);

        /// 暫定 - MemoryMappedと言いつつ使わない
        /// 形状内に限定するときにデータ書き換えをするので（MemoryMappedは書き換え不可）
        /// データ書き換えない別の手法()も思いついたがとりあえず現状はこれで。。。
        if (!m_nouse_memory_mapping) {
            hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
            if (hMapping) {
                mapView = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
                if (mapView) {
                    useMemoryMapping = true;
                    valid            = true;
                    CloseHandle(hFile);
                    hFile = INVALID_HANDLE_VALUE;
                    return true;
                }
            }
        }

        // メモリマッピング失敗時
        useMemoryMapping = false;
        if (hMapping) {
            CloseHandle(hMapping);
            hMapping = NULL;
        }

        buffer = new (std::nothrow) char[fileSize];
        if (!buffer) {
            CloseHandle(hFile);
            hFile = INVALID_HANDLE_VALUE;
            valid = false;
            return false;
        }

        const size_t MAX_CHUNK_SIZE = 0xFFFFFFFF;    // DWORD最大値（4GB）
        size_t       totalRead      = 0;
        DWORD        bytesRead      = 0;
        BOOL         result         = TRUE;

        while (totalRead < fileSize) {
            size_t chunkSize = MAX_CHUNK_SIZE < fileSize - totalRead ? MAX_CHUNK_SIZE : fileSize - totalRead;
            result           = ReadFile(hFile, buffer + totalRead, static_cast<DWORD>(chunkSize), &bytesRead, NULL);
            if (!result || bytesRead == 0) {
                delete[] buffer;
                buffer = nullptr;
                valid  = false;
                return false;
            }
            totalRead += bytesRead;
        }
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
        /*
        DWORD bytesRead = 0;
        BOOL  result    = ReadFile(hFile, buffer, static_cast<DWORD>(fileSize), &bytesRead, NULL);
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
        if (!result || bytesRead != fileSize) {
            delete[] buffer;
            buffer = nullptr;
            valid  = false;
            return false;
        }
        */
        valid = true;
        return true;
    }

public:
    void closeFile()
    {
        if (mapView) {
            UnmapViewOfFile(mapView);
            mapView = NULL;
        }
        if (hMapping) {
            CloseHandle(hMapping);
            hMapping = NULL;
        }
        if (buffer) {
            delete[] buffer;
            buffer = nullptr;
        }
        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(hFile);
            hFile = INVALID_HANDLE_VALUE;
        }
    }
};

#endif    // MEMORYMAPPEDFILE_H
