#include <windows.h>
#include <BaseTsd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#define CACHE_SIZE 4      
#define BLOCK_SIZE 4096   // Размер сектора

typedef struct CacheEntry {
    int fileOffset;
    char* data;
    int dirty;
    struct CacheEntry *next;
} CacheEntry;

typedef struct {
    HANDLE hFile;
    CacheEntry *cacheHead;
    CacheEntry *cacheTail;
    int cacheCount;
} FileDescriptor;

FileDescriptor *openFiles[256] = {NULL};

#ifdef _MSC_VER
  #define DLL_EXPORT __declspec(dllexport)
#else
  #define DLL_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

DLL_EXPORT int lab2_open(const char *path);
DLL_EXPORT int lab2_close(int fd);
DLL_EXPORT SSIZE_T lab2_read(int fd, void *buf, size_t count);
DLL_EXPORT SSIZE_T lab2_write(int fd, const void *buf, size_t count);
DLL_EXPORT long lab2_lseek(int fd, long offset, int whence);
DLL_EXPORT int lab2_fsync(int fd);

DLL_EXPORT int lab2_open(const char *path) {
    HANDLE hFile = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, 
                               OPEN_ALWAYS, FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
       return -1;
    }
    
    for (int i = 0; i < 256; i++) {
        if (openFiles[i] == NULL) {
            openFiles[i] = (FileDescriptor*)malloc(sizeof(FileDescriptor));
            openFiles[i]->hFile = hFile;
            openFiles[i]->cacheHead = NULL;
            openFiles[i]->cacheTail = NULL;
            openFiles[i]->cacheCount = 0;
            return i;
        }
    }
    CloseHandle(hFile); 
    return -1;
}

CacheEntry* findCacheEntry(FileDescriptor *fd, int fileOffset) {
    CacheEntry *current = fd->cacheHead;
    while (current) {
        if (current->fileOffset == fileOffset) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

DLL_EXPORT int lab2_close(int fd) {
    if (fd < 0 || fd >= 256 || openFiles[fd] == NULL)
        return -1;
    
    FileDescriptor *file = openFiles[fd];
    CacheEntry *current = file->cacheHead;
    while (current) {
        if (current->dirty) {
            DWORD bytesWritten;
            LARGE_INTEGER li;
            li.QuadPart = current->fileOffset;
            SetFilePointerEx(file->hFile, li, NULL, FILE_BEGIN);
            WriteFile(file->hFile, current->data, BLOCK_SIZE, &bytesWritten, NULL);
        }
        CacheEntry *next = current->next;
        VirtualFree(current->data, 0, MEM_RELEASE);
        free(current);
        current = next;
    }
    CloseHandle(file->hFile);
    free(file);
    openFiles[fd] = NULL;
    return 0;
}

DLL_EXPORT SSIZE_T lab2_read(int fd, void *buf, size_t count) {
    if (fd < 0 || fd >= 256 || openFiles[fd] == NULL)
        return -1;
    
    FileDescriptor *file = openFiles[fd];
    int fileOffset = lab2_lseek(fd, 0, SEEK_CUR); 

    CacheEntry *entry = findCacheEntry(file, fileOffset);
    if (entry) {
        memcpy(buf, entry->data, count); 
        return count;
    }
    
    // Выделяем выровненный буфер через VirtualAlloc (под NO_BUFFERING требования Windows)
    char *alignedBuffer = (char*)VirtualAlloc(NULL, BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!alignedBuffer)
        return -1;

    DWORD bytesRead;
    LARGE_INTEGER li;
    li.QuadPart = fileOffset;
    SetFilePointerEx(file->hFile, li, NULL, FILE_BEGIN);
    if (!ReadFile(file->hFile, alignedBuffer, BLOCK_SIZE, &bytesRead, NULL)) {
        VirtualFree(alignedBuffer, 0, MEM_RELEASE);
        return -1;
    }
    
    CacheEntry *newEntry = (CacheEntry*)malloc(sizeof(CacheEntry));
    newEntry->fileOffset = fileOffset;
    newEntry->data = alignedBuffer;
    newEntry->dirty = 0;
    newEntry->next = NULL;
    
    if (file->cacheTail)
        file->cacheTail->next = newEntry;
    file->cacheTail = newEntry;
    if (!file->cacheHead)
        file->cacheHead = newEntry;
    file->cacheCount++;
    
    if (file->cacheCount > CACHE_SIZE) {
        CacheEntry *old = file->cacheHead;
        file->cacheHead = old->next;
        VirtualFree(old->data, 0, MEM_RELEASE);
        free(old);
        file->cacheCount--;
    }
    
    memcpy(buf, alignedBuffer, count);
    return bytesRead;
}

DLL_EXPORT SSIZE_T lab2_write(int fd, const void *buf, size_t count) {
    if (fd < 0 || fd >= 256 || openFiles[fd] == NULL)
        return -1;
    
    FileDescriptor *file = openFiles[fd];
    int fileOffset = lab2_lseek(fd, 0, SEEK_CUR);

    CacheEntry *entry = findCacheEntry(file, fileOffset);
    if (entry) {
        memcpy(entry->data, buf, count); 
        entry->dirty = 1;
        return count;
    }
    
    char *alignedBuffer = (char*)VirtualAlloc(NULL, BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!alignedBuffer)
        return -1;
    memcpy(alignedBuffer, buf, count);
    
    CacheEntry *newEntry = (CacheEntry*)malloc(sizeof(CacheEntry));
    newEntry->fileOffset = fileOffset;
    newEntry->data = alignedBuffer;
    newEntry->dirty = 1;
    newEntry->next = NULL;
    
    if (file->cacheTail)
        file->cacheTail->next = newEntry;
    file->cacheTail = newEntry;
    if (!file->cacheHead)
        file->cacheHead = newEntry;
    file->cacheCount++;
    
    if (file->cacheCount > CACHE_SIZE) {
        CacheEntry *old = file->cacheHead;
        file->cacheHead = old->next;
        if (old->dirty) {
            DWORD bytesWritten;
            LARGE_INTEGER li;
            li.QuadPart = old->fileOffset;
            SetFilePointerEx(file->hFile, li, NULL, FILE_BEGIN);
            WriteFile(file->hFile, old->data, BLOCK_SIZE, &bytesWritten, NULL);
        }
        VirtualFree(old->data, 0, MEM_RELEASE);
        free(old);
        file->cacheCount--;
    }
    
    return count;
}

DLL_EXPORT long lab2_lseek(int fd, long offset, int whence) {
    if (fd < 0 || fd >= 256 || openFiles[fd] == NULL)
        return -1;
    LARGE_INTEGER li;
    li.QuadPart = offset;
    return SetFilePointerEx(openFiles[fd]->hFile, li, NULL, whence) ? li.QuadPart : -1;
}

DLL_EXPORT int lab2_fsync(int fd) {
    if (fd < 0 || fd >= 256 || openFiles[fd] == NULL)
        return -1;
    return FlushFileBuffers(openFiles[fd]->hFile) ? 0 : -1;
}

#ifdef __cplusplus
}
#endif
