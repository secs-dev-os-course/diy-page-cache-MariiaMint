#include <algorithm>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <windows.h>
#include "fifo_cache.h"


namespace {
    void SortData(int fd, int iterations, int block_size) {
        const int SECTOR_SIZE = 4096;
        int aligned_size =
            ((block_size * sizeof(int) + SECTOR_SIZE - 1) / SECTOR_SIZE) *
            SECTOR_SIZE;
        std::vector<int> data(aligned_size / sizeof(int), 0);
    
        if (fd == -1) {
            std::cerr << "Error opening file: " << fd << std::endl;
            return;
        }
        // Чтение 
        if (lab2_read(fd, data.data(), aligned_size) == -1) {
            std::cerr << "Error reading from file" << std::endl;
            lab2_close(fd);
            return;
        }
        // Сортировка 
        for (int i = 0; i < iterations; ++i) {
            std::sort(data.begin(), data.end());
        }
        // Запись 
        if (lab2_write(fd, data.data(), aligned_size) == -1) {
            std::cerr << "Error writing to file" << std::endl;
        }
    
        lab2_fsync(fd);
        lab2_close(fd);
    }


    void SortData_SystemCache(HANDLE hFile, int iterations, int block_size) {
        const int SECTOR_SIZE = 4096;
        int aligned_size =
            ((block_size * sizeof(int) + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;
        std::vector<int> data(aligned_size / sizeof(int), 0);
    
        
        // Чтение
        DWORD bytesRead;
        if (!ReadFile(hFile, data.data(), aligned_size, &bytesRead, NULL)) {
            std::cerr << "Error reading from file: " << GetLastError() << std::endl;
            CloseHandle(hFile);
            return;
        }
        // Сортировка
        for (int i = 0; i < iterations; ++i) {
            std::sort(data.begin(), data.end());
        }
        // Запись
        DWORD bytesWritten;
        SetFilePointer(hFile, 0, NULL, FILE_BEGIN); // Перемещаем указатель в начало
        if (!WriteFile(hFile, data.data(), aligned_size, &bytesWritten, NULL)) {
            std::cerr << "Error writing to file: " << GetLastError() << std::endl;
        }
    
        FlushFileBuffers(hFile);
        CloseHandle(hFile);
    }
    

    void SortData_NoSystemCache(HANDLE hFile, int iterations, int block_size) {
        const int SECTOR_SIZE = 4096;
        int aligned_size =
            ((block_size * sizeof(int) + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;
        std::vector<int> data(aligned_size / sizeof(int), 0);

        DWORD bytesRead;
        if (!ReadFile(hFile, data.data(), aligned_size, &bytesRead, NULL)) {
            std::cerr << "Error reading from file: " << GetLastError() << std::endl;
            CloseHandle(hFile);
            return;
        }

        for (int i = 0; i < iterations; ++i) {
            std::sort(data.begin(), data.end());
        }

        DWORD bytesWritten;
        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
        if (!WriteFile(hFile, data.data(), aligned_size, &bytesWritten, NULL)) {
            std::cerr << "Error writing to file: " << GetLastError() << std::endl;
        }

        FlushFileBuffers(hFile);
        CloseHandle(hFile);
    }

} // namespace

int main(int argc, char *argv[]) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0]
              << " <iterations> <block_size> <file_path>\n";
    return 1;
  }

  int iterations = std::stoi(argv[1]);
  int block_size = std::stoi(argv[2]);
  const char *file_path = argv[3];

  int fd = lab2_open(file_path);
  SortData(fd, iterations, block_size);

//   HANDLE hFile = CreateFileA(file_path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
//     OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
//   if (hFile == INVALID_HANDLE_VALUE) {
//     std::cerr << "Error opening file: " << GetLastError() << std::endl;
//     return 1;
//   }
//   SortData_NoSystemCache(hFile, iterations, block_size);


//   HANDLE hFile = CreateFileA(file_path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
//     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
//   if (hFile == INVALID_HANDLE_VALUE) {
//     std::cerr << "Error opening file: " << GetLastError() << std::endl;
//     return 1;
//   }
//   SortData_SystemCache(hFile, iterations, block_size);

  return 0;
}
