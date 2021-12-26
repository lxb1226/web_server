/**
 * @file http_request.h
 * @author heyjude (1944303766@qq.com)
 * @brief
 * @version 0.1
 * @date 2021-12-06
 *
 * @copyright Copyright (c) 2021
 *
 */

#ifndef WEB_SERVER_HTTP_REQUEST_H
#define WEB_SERVER_HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <errno.h>
#include <cstring>
#include <algorithm>
#include <regex>

#include "../buffer/buffer.h"

class http_request
{
public:
    // 解析HTTP请求状态
    enum class CHECK_STATE : int
    {
        CHECK_REQUEST_LINE = 0,
        CHECK_HEADERS,
        CHECK_CONTENT,
        CHECK_FINISH
    };

    // HTTP1.1请求方法，目前仅支持GET
    enum class METHOD : int
    {
        GET = 0,
        HEAD,
        POST,
        PUT,
        DELETE,
        CONNECT,
        OPTIONS,
        TRACE,
        PATCH,
        UNKNOWN,
    };

    // 服务器处理HTTP请求时的可能结果
    enum class HTTP_CODE : int
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };

    // 行的读取状态
    enum class LINE_STATUS : int
    {
        LINE_OK = 0,
        LINE_OPEN,
        LINE_BAD
    };

    http_request() { init(); }
    ~http_request() = default;

    void init();

    bool parse(Buffer &buff);

    std::string path() const;

    std::string &path();

    std::string method() const;

    std::string version() const;

    std::string get_post(const std::string &key) const;

    std::string get_post(const char *key) const;

    bool is_keep_alive() const;

private:
    bool parse_request_line(const std::string &line);

    void parse_headers(const std::string &line);

    void parse_body(const std::string &line);

    void parse_path_();

    void parse_post_();

    void parse_from_url_encoded_();

    static bool user_verify(const std::string &name, const std::string &pwd, bool isLogin);

    CHECK_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;

    static int ConvertHex(char ch);
};

#endif
