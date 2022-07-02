#ifdef _WIN32

#define NOMINMAX
#include <Windows.h>
#include <memoryapi.h>

void* getDataPtr(const std::string& filePath)
{
    HANDLE hFile;
    HANDLE hMap;

    OFSTRUCT buffer;
    hFile = CreateFile(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    hMap = CreateFileMapping(
                hFile,
                NULL,                          // Mapping attributes
                PAGE_READONLY,                 // Protection flags
                0,                             // MaximumSizeHigh
                0,                             // MaximumSizeLow
                NULL);                         // Name

    void* lpBasePtr = MapViewOfFile(
                hMap,
                FILE_MAP_READ,         // dwDesiredAccess
                0,                     // dwFileOffsetHigh
                0,                     // dwFileOffsetLow
                0);                    // dwNumberOfBytesToMap

    return lpBasePtr;
}

void* alloc_large_pages(size_t size)
{
	const size_t adjusted_size = size - (size % 2097152ULL) + 2097152ULL;
	
	return VirtualAlloc(NULL, adjusted_size, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
}	

#include "large_page.hpp"

#else
	
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <sys/stat.h>

void* alloc_large_pages(size_t size)
{
	const size_t adjusted_size = size - (size % 2097152ULL) + 2097152ULL;
	
	return mmap(0, adjusted_size, (PROT_READ | PROT_WRITE), (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB), 0, 0);
}

void* getDataPtr(const std::string& filePath)
{
  int fd = open(filePath.c_str(), O_RDONLY);

  struct stat sb;
  fstat( fd, &sb );

  void* buf = mmap(0, sb.st_size, PROT_READ, MAP_PRIVATE|MAP_POPULATE, fd, 0);
  
  return buf;
}
#endif