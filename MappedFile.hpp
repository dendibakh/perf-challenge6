#ifndef WORDCOUNT_MAPPEDFILE_HPP
#define WORDCOUNT_MAPPEDFILE_HPP

#include "DetectOS.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

#ifdef ON_LINUX
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef ON_WINDOWS
#include <windows.h>

#include <fileapi.h>
#include <memoryapi.h>
#include <winbase.h>
#endif

class MappedFile
{
public:
    MappedFile() : m_content{nullptr}, m_size{0} {}
    explicit inline MappedFile(const std::string& path);
    MappedFile(const MappedFile&)            = delete;
    MappedFile& operator=(const MappedFile&) = delete;
    inline MappedFile(MappedFile&& other) noexcept;
    inline MappedFile& operator=(MappedFile&& other) noexcept;
    ~MappedFile()
    {
        if (m_content)
            dtorImpl();
    }

    [[nodiscard]] auto getContents() const -> std::string_view { return {m_content, m_size}; }

private:
    inline void dtorImpl() noexcept;

    char*       m_content;
    std::size_t m_size;

#ifdef ON_WINDOWS
    HANDLE m_file_handle;
    HANDLE m_mapping_handle;
#endif
};

#ifdef ON_LINUX
MappedFile::MappedFile(const std::string& path)
{
    const int fd = open(path.c_str(), O_RDONLY | O_LARGEFILE);
    if (fd == -1)
        throw std::runtime_error{"Could not open the specified file"};
    struct stat sb
    {};
    if (fstat(fd, &sb))
        throw std::runtime_error{"Could not obtain file info"};
    m_size = static_cast< std::size_t >(sb.st_size);

    void* const mapped_addr = mmap(NULL, m_size, PROT_READ, MAP_PRIVATE, fd, 0u); // NOLINT it's a C API...
    if (mapped_addr == MAP_FAILED)
        throw std::runtime_error{"Could not mmap file"};

    m_content = static_cast< char* >(mapped_addr);

    if (close(fd))
        throw std::runtime_error{"Could not close file after mmap"};
}

MappedFile::MappedFile(MappedFile&& other) noexcept : m_content{other.m_content}, m_size{other.m_size}
{
    other.m_content = nullptr;
}

MappedFile& MappedFile::operator=(MappedFile&& other) noexcept
{
    if (this != &other)
    {
        dtorImpl();
        m_content       = other.m_content;
        m_size          = other.m_size;
        other.m_content = nullptr;
    }
    return *this;
}

void MappedFile::dtorImpl() noexcept
{
    munmap(m_content, m_size); // Ignore error - dtor shouldn't throw + we can't meaningfully handle this error anyway
}
#endif

#ifdef ON_WINDOWS
MappedFile::MappedFile(const std::string& path)
{
    m_file_handle =
        CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (m_file_handle == INVALID_HANDLE_VALUE)
        throw std::runtime_error{"Could not open file"};

    LARGE_INTEGER size{};
    if (not GetFileSizeEx(m_file_handle, &size))
    {
        CloseHandle(m_file_handle);
        throw std::runtime_error{"Could not obtain file size"};
    }
    m_size = size.QuadPart;

    m_mapping_handle = CreateFileMappingA(m_file_handle, NULL, PAGE_READONLY, 0, 0, NULL);
    if (m_mapping_handle == NULL)
    {
        CloseHandle(m_file_handle);
        throw std::runtime_error{"Could not create file mapping"};
    }

    const auto map_content = MapViewOfFile(m_mapping_handle, FILE_MAP_READ, 0, 0, 0);
    if (map_content == NULL)
    {
        CloseHandle(m_mapping_handle);
        CloseHandle(m_file_handle);
        throw std::runtime_error{"Could not map file into process address space"};
    }
    m_content = static_cast< char* >(map_content);
}

MappedFile::MappedFile(MappedFile&& other) noexcept
    : m_content{other.m_content},
      m_size{other.m_size},
      m_file_handle{other.m_file_handle},
      m_mapping_handle{other.m_mapping_handle}
{
    other.m_content = nullptr;
}

MappedFile& MappedFile::operator=(MappedFile&& other) noexcept
{
    if (this != &other)
    {
        dtorImpl();
        m_content        = other.m_content;
        m_size           = other.m_size;
        m_file_handle    = other.m_file_handle;
        m_mapping_handle = other.m_mapping_handle;
        other.m_content  = nullptr;
    }
    return *this;
}

void MappedFile::dtorImpl() noexcept
{
    UnmapViewOfFile(m_content);
    CloseHandle(m_mapping_handle);
    CloseHandle(m_file_handle);
}
#endif
#endif // WORDCOUNT_MAPPEDFILE_HPP
