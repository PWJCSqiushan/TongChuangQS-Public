#pragma once

#include <string>
#include <utility>

namespace qs {

/**
 * 业务操作的统一返回值：成功/失败 + 面向用户的中文说明。
 * 所有"会修改数据"的接口都返回 ResultQS，界面层直接把 message 展示给用户，
 * 这样同一套业务接口可以同时服务于控制台和图形界面。
 */
class ResultQS {
public:
    static ResultQS ok(std::string message = "操作成功") { return ResultQS(true, std::move(message)); }
    static ResultQS fail(std::string message) { return ResultQS(false, std::move(message)); }

    bool isOk() const noexcept { return ok_; }
    explicit operator bool() const noexcept { return ok_; }
    const std::string& message() const noexcept { return message_; }

private:
    ResultQS(bool ok, std::string message) : ok_(ok), message_(std::move(message)) {}
    bool ok_;
    std::string message_;
};

}  // namespace qs
