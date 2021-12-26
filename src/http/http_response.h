#ifndef WEB_SERVER_HTTP_RESPONSE_H
#define WEB_SERVER_HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "../buffer/buffer.h"

class HttpResponse
{
public:
    HttpResponse();
    ~HttpResponse();

    void init(const std::string &srcDir, std::string &path, bool isKeepAlive = false, int code = -1);

    void make_response(Buffer &buff);

    void unmap_file();

    char *File();

    size_t file_len() const;

    void error_content(Buffer &buff, std::string message);

    int code() const { return code_; };

private:
    void add_state_line_(Buffer &buff);

    void add_header_(Buffer &buff);

    void add_content_(Buffer &buff);

    void error_html();

    std::string get_file_type_();

    int code_;
    bool isKeepAlive_;

    std::string path_;
    std::string srcDir_;

    char *mmFile_;
    struct stat mmFileStat_;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;
};

#endif
