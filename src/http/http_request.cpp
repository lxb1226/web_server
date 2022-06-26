#include "http_request.h"

using namespace std;

const unordered_set<string> http_request::DEFAULT_HTML{
    "/index",
    "/register",
    "/login",
    "/welcome",
    "/video",
    "/picture",
};

const unordered_map<string, int> http_request::DEFAULT_HTML_TAG{
    {"/register.html", 0},
    {"/login.html", 1},
};

void http_request::init()
{
    method_ = path_ = version_ = body_ = "";
    state_ = CHECK_STATE::CHECK_REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool http_request::is_keep_alive() const
{
    if (header_.count("Connection") == 1)
    {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

// 解析http_request请求
bool http_request::parse(Buffer &buff)
{
    const char CRLF[] = "\r\n";
    if (buff.ReadableBytes() <= 0)
    {
        return false;
    }

    while (buff.ReadableBytes() && state_ != CHECK_STATE::CHECK_FINISH)
    {
        const char *lineEnd = search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);
        string line(buff.Peek(), lineEnd);
        switch (state_)
        {
        case CHECK_STATE::CHECK_REQUEST_LINE:
            // 解析请求行
            if (!parse_request_line(line))
                return false;
            parse_path_();
            break;
        case CHECK_STATE::CHECK_HEADERS:
            parse_headers(line);
            if (buff.ReadableBytes() <= 2)
                state_ = CHECK_STATE::CHECK_FINISH;
            break;
        case CHECK_STATE::CHECK_CONTENT:
            parse_body(line);
            break;
        default:
            break;
        }
    }
}

bool http_request::parse_request_line(const std::string &line)
{
    regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch subMatch;
    if (regex_match(line, subMatch, pattern))
    {
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = CHECK_STATE::CHECK_HEADERS;
        return true;
    }

    return false;
}

void http_request::parse_headers(const std::string &line)
{
    regex pattern("^([^:]*): ?(.*)$");
    smatch subMatch;
    if (regex_match(line, subMatch, pattern))
    {
        header_[subMatch[1]] = subMatch[2];
    }
    else
    {
        state_ = CHECK_STATE::CHECK_CONTENT;
    }
}

void http_request::parse_body(const std::string &line)
{
    body_ = line;
    parse_post_();
    state_ = CHECK_STATE::CHECK_FINISH;
}

void http_request::parse_path_()
{
    if (path_ == "/")
    {
        path_ = "/index.html";
    }
    else
    {
        for (auto &item : DEFAULT_HTML)
        {
            if (item == path_)
            {
                path_ += ".html";
                break;
            }
        }
    }
}

void http_request::parse_post_()
{
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded")
    {
        parse_from_url_encoded_();
        if (DEFAULT_HTML_TAG.count(path_))
        {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            if (tag == 0 || tag == 1)
            {
                bool isLogin = (tag == 1);
                if (user_verify(post_["username"], post_["password"], isLogin))
                {
                    path_ = "/welcome.html";
                }
                else
                {
                    path_ = "/error.html";
                }
            }
        }
    }
}

void http_request::parse_from_url_encoded_()
{
    if (body_.size() == 0)
    {
        return;
    }

    string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for (; i < n; ++i)
    {
        char ch = body_[i];
        switch (ch)
        {
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            body_[i] = ' ';
            break;
        case '%':
            num = ConvertHex(body_[i + 1]) * 16 + ConvertHex(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            break;
        default:
            break;
        }
    }
}

std::string http_request::path() const
{
    return path_;
}

std::string &http_request::path()
{
    return path_;
}

std::string http_request::method() const
{
    return method_;
}

std::string http_request::version() const
{
    return version_;
}

std::string http_request::get_post(const std::string &key) const
{
    assert(key != "");
    if (post_.count(key) == 1)
    {
        return post_.find(key)->second;
    }
    return "";
}

std::string http_request::get_post(const char *key) const
{
    assert(key != nullptr);
    if (post_.count(key) == 1)
    {
        return post_.find(key)->second;
    }
    return "";
}

int http_request::ConvertHex(char ch)
{
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    return ch;
}

bool http_request::user_verify(const std::string &name, const std::string &pwd, bool isLogin)
{
    if (name == "" || pwd == "")
        return false;
    if (isLogin)
    {
        if (name == "username" && pwd == "password")
            return true;
        else
            return false;
    }

    return true;
}
