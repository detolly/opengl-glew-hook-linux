
#include <cstdio>
#include <cstring>
#include <string>

#include <dlfcn.h>

FILE* wal_fp{ nullptr };
int wal_fd{ 0 };

int (*old_open)(const char*, int, mode_t) = nullptr;
#undef open
extern "C" int open(const char* pathname, int flags, mode_t mode)
{
    if (old_open == nullptr)
        old_open = (decltype(old_open))dlsym(RTLD_NEXT, "open");

    auto ret = old_open(pathname, flags, mode);

    std::string fn = pathname;

    if (fn.find(".WAL") != std::string::npos) {
        //printf("open(): %s 0x%04x 0x%04x\n", pathname, flags, mode);
        wal_fd = ret;
    }

    return ret;
    
}

int (*old_open64)(const char*, int) = nullptr;
#undef open64
extern "C" int open64(const char* pathname, int flags, mode_t mode)
{
    if (old_open64 == nullptr)
        old_open64 = (decltype(old_open64))dlsym(RTLD_NEXT, "open64");
	
    auto ret = old_open64(pathname, flags);

    const std::string_view fn = pathname;

    if (fn.find(".WAL") != std::string_view::npos) {
        //printf("open64(): %s 0x%04x 0x%04x\n", pathname, flags, mode);
        wal_fd = ret;
    }

    return ret;
}

FILE* (*old_fopen)(const char *filename, const char *mode);
#undef fopen
extern "C" FILE* fopen(const char* filename, const char* mode)
{
    if (old_fopen == nullptr)
        old_fopen = (decltype(old_fopen))dlsym(RTLD_NEXT, "fopen");
    
    auto ret = old_fopen(filename, mode);

    const std::string_view fn = filename;

    if (fn.find(".WAL") != std::string_view::npos) {
        printf("fopen(f, m): %s %s\n", filename, mode);
        wal_fp = ret;
    }

    return ret;
}

size_t (*old_read)(int, void*, size_t) = nullptr;
#undef read
extern "C" ssize_t read(int fd, void *buf, size_t count)
{
    if (old_read == nullptr)
        old_read = (decltype(old_read))dlsym(RTLD_NEXT, "read");

    auto ret = old_read(fd, buf, count);

    // if (fd == wal_fd)
    //     printf("read: %8p 0x%zx\n", buf, count);

    return ret;
}

size_t (*old_fread)(void* ptr, size_t size, size_t nmemb, FILE* stream);
#undef fread
extern "C" size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    if (old_fread == nullptr)
        old_fread = (decltype(old_fread))dlsym(RTLD_NEXT, "fread");
    
    auto ret = old_fread(ptr, size, nmemb, stream);

    // if (stream == wal_fp)
    //     printf("fread(): %8p 0x%zx 0x%zx %p\n", ptr, size, nmemb, stream);

    return ret;
}

off_t (*old_lseek)(int, off_t, int) = nullptr;
#undef read
extern "C" ssize_t lseek(int fd, off_t offset, int whence)
{
    if (old_lseek == nullptr)
        old_lseek = (decltype(old_lseek))dlsym(RTLD_NEXT, "lseek");

    auto ret = old_lseek(fd, offset, whence);

    // if (fd == wal_fd)
    //     printf("lseek: 0x%08zx %d\n", offset, whence);

    return ret;
}
