#ifndef REKHCOM_SAVE_FILE_H
#define REKHCOM_SAVE_FILE_H
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#endif
#include <filesystem>
#include <vector>
#include <string>
namespace RekhcomSave {
/* Same-directory staging; no successful backup means caller must not call. */
inline bool Replace(const std::filesystem::path& target,const std::vector<unsigned char>& data) {
    std::filesystem::path temp;
    static unsigned serial=0;
#ifdef _WIN32
    HANDLE file=INVALID_HANDLE_VALUE;
    for(int attempt=0;attempt<16&&file==INVALID_HANDLE_VALUE;attempt++) {
        temp=target;temp+=L".import-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(++serial);
        file=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file==INVALID_HANDLE_VALUE&&GetLastError()!=ERROR_FILE_EXISTS)return false;
    }
    if(file==INVALID_HANDLE_VALUE)return false;
    DWORD written=0;
    bool ok=WriteFile(file,data.data(),static_cast<DWORD>(data.size()),&written,nullptr)
            &&written==data.size()&&FlushFileBuffers(file);
    if(!CloseHandle(file))ok=false;
    if(ok)ok=MoveFileExW(temp.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if(!ok)DeleteFileW(temp.c_str());
    return ok;
#else
    /* the same on Android/POSIX: a new file next to it, written and synced, then renamed over the old one */
    int fd=-1;
    for(int attempt=0;attempt<16&&fd<0;attempt++) {
        temp=target;temp+=".import-"+std::to_string(getpid())+"-"+std::to_string(++serial);
        fd=open(temp.c_str(),O_WRONLY|O_CREAT|O_EXCL,0644);
        if(fd<0&&errno!=EEXIST)return false;
    }
    if(fd<0)return false;
    size_t done=0;
    while(done<data.size()) {
        ssize_t n=write(fd,data.data()+done,data.size()-done);
        if(n<=0)break;
        done+=(size_t)n;
    }
    bool ok=done==data.size()&&fsync(fd)==0;
    if(close(fd)!=0)ok=false;
    if(ok)ok=std::rename(temp.c_str(),target.c_str())==0;
    if(!ok)std::remove(temp.c_str());
    return ok;
#endif
}
}
#endif
