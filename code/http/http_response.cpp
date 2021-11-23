#include "./http_response.h"

const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const std::unordered_map<int, std::string> HttpResponse::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

HttpResponse::HttpResponse() {
    code = -1;
    path = src_dir = "";
    is_keep_alive = false;
    mm_file = nullptr;
    mm_file_stat = {0};
}

HttpResponse::~HttpResponse() {
    UnmapFile();
}

void HttpResponse::Init(const std::string& src_dir, std::string& path, bool is_keep_alive, int code) {
    assert(src_dir != "");
    if(mm_file) { UnmapFile(); }
    this->code = code;
    this->is_keep_alive = is_keep_alive;
    this->path = path;
    this->src_dir = src_dir;
    this->mm_file = nullptr;
    this->mm_file_stat = {0};
}

void HttpResponse::MakeResponse(Buffer& buff) {
    if(stat((src_dir + path).data(), &mm_file_stat) < 0 || S_ISDIR(mm_file_stat.st_mode)) {
        code = 404;
    }
    else if(!(mm_file_stat.st_mode & S_IROTH)) {
        code = 403;
    }
    else if(code == -1) {
        code = 200;
    }

    ErrorHtml();
    AddStateLine(buff);
    AddHeader(buff);
    AddContent(buff);
}

void HttpResponse::UnmapFile() {
    if(mm_file) {
        munmap(mm_file, mm_file_stat.st_size);
        mm_file = nullptr;
    }
}

char* HttpResponse::File() {
    return mm_file;
}

size_t HttpResponse::FileLen() const {
    return static_cast<size_t>(mm_file_stat.st_size);
}

void HttpResponse::ErrorHtml() {
    if(CODE_PATH.count(code) == 1) {
        path = CODE_PATH.find(code)->second;
        stat((src_dir + path).data(), &mm_file_stat);
    }
}

void HttpResponse::AddStateLine(Buffer& buff) {
    std::string status;
    if(CODE_STATUS.count(code) == 1) {
        status = CODE_STATUS.find(code)->second;
    }
    else {
        code = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff.Append("HTTP/1.1 " + std::to_string(code) + " " + status + "\r\n");
}

void HttpResponse::AddHeader(Buffer& buff) {
    buff.Append("Connection: ");
    if(is_keep_alive) {
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=6, timeout=120\r\n");
    }
    else {
        buff.Append("close\r\n");
    }
    buff.Append("AddContent-type: " + GetFileType() + "\r\n");
}

void HttpResponse::AddContent(Buffer& buff) {
    int src_fd = open((src_dir + path).data(), O_RDONLY);
    if(src_fd < 0) {
        ErrorContent(buff, "File NotFound!");
        return;
    }

    LOG_DEBUG("file path %s", (src_dir + path).data());
    char* mm_ret = (char*)mmap(0, mm_file_stat.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
    if(*mm_ret == -1) {
        ErrorContent(buff, "File NotFound!");
        return;
    }
    mm_file = mm_ret;
    close(src_fd);
    buff.Append("Content-length: " + std::to_string(mm_file_stat.st_size) + "\r\n\r\n");
}

std::string HttpResponse::GetFileType() {
    std::string::size_type idx = path.find_last_of('.');
    if(idx == std::string::npos) {
        return "text/plain";
    }
    std::string suffix = path.substr(idx);
    if(SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

void HttpResponse::ErrorContent(Buffer& buff, std::string message) {
    std::string body;
    std::string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code) == 1) {
        status = CODE_STATUS.find(code)->second;
    }
    else {
        status = "Bad Request";
    }
    body += std::to_string(code) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>WebServerCpp</em></body></html>";

    buff.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}
