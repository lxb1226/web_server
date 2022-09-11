//
// Created by heyjude on 2022/7/16.
//

#ifndef WEB_SERVER_STATUS_H
#define WEB_SERVER_STATUS_H

#include <string>

static const int STATUS_OK = 0;                                 /** 正常流程返回值 */
static const int STATUS_ERR = -1;                               /** 异常流程返回值 */
static const char *STATUS_ERROR_INFO_CONNECTOR = " && ";        /** 多异常信息连接符号 */

class Status {
public:
    explicit Status() = default;

    explicit Status(const std::string &errorInfo) {
        this->error_code_ = STATUS_ERR;    // 默认的error code信息
        this->error_info_ = errorInfo;
    }

    explicit Status(int errorCode, const std::string &errorInfo) {
        this->error_code_ = errorCode;
        this->error_info_ = errorInfo;
    }

    Status(const Status &status) {
        this->error_code_ = status.error_code_;
        this->error_info_ = status.error_info_;
    }

    Status(const Status &&status) noexcept {
        this->error_code_ = status.error_code_;
        this->error_info_ = status.error_info_;
    }

    Status &operator=(const Status &status) = default;

    Status &operator+=(const Status &cur) {
        if (this->isOK() && cur.isOK()) {
            return (*this);
        }

        error_info_ = this->isOK()
                      ? cur.error_info_
                      : (cur.isOK()
                         ? error_info_
                         : (error_info_ + STATUS_ERROR_INFO_CONNECTOR + cur.error_info_));
        error_code_ = STATUS_ERR;

        return (*this);
    }

    void setStatus(const std::string &info) {
        error_code_ = STATUS_ERR;
        error_info_ = info;
    }

    void setStatus(int code, const std::string &info) {
        error_code_ = code;
        error_info_ = info;
    }

    [[nodiscard]] int getCode() const {
        return this->error_code_;
    }

    [[nodiscard]] const std::string &getInfo() const {
        return this->error_info_;
    }

    /**
     * 判断当前状态是否可行
     * @return
     */
    [[nodiscard]] bool isOK() const {
        return STATUS_OK == error_code_;
    }

    /**
     * 判断当前状态是否可行
     * @return
     */
    [[nodiscard]] bool isErr() const {
        return error_code_ < STATUS_OK;    // 约定异常信息，均为负值
    }

private:
    int error_code_{STATUS_OK};                   // 错误码信息
    std::string error_info_;                         // 错误信息描述
};


#endif //WEB_SERVER_STATUS_H
