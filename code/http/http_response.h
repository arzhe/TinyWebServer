#ifndef WEB_SERVER_HTTP_HTTP_RESPONSE_H
#define WEB_SERVER_HTTP_HTTP_RESPONSE_H

#include <unordered_map>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "../log/log.h"
#include "../buffer/buffer.h"

class HttpResponse {
private:
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;

    int code;
    bool is_keep_alive;

    std::string path;
    std::string src_dir;

    char* mm_file;
    struct stat mm_file_stat;

public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string& src_dir, std::string& path, bool is_keep_alive = false, int code = -1);
    void MakeResponse(Buffer& buff);
    void UnmapFile();
    char* File();
    size_t FileLen() const;
    void ErrorContent(Buffer& buff, std::string message);
    int Code() const { return code; }

private:
    void AddStateLine(Buffer& buff);
    void AddHeader(Buffer& buff);
    void AddContent(Buffer& buff);

    void ErrorHtml();
    std::string GetFileType();
};

#endif
