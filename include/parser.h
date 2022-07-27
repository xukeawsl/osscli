#pragma once

#include "executor.h"

#include <memory>
#include <string>

class OssParser {
public:
    OssParser() : executor(std::make_unique<OssExecutor>()) {}
    ~OssParser() = default;

    // 解析配置文件
    bool ParseConfigFile(const std::string& filename);

    // 解析命令行参数, 返回一个 OssExecutor 对象, 为空说明解析失败
    std::unique_ptr<OssExecutor> ParseCommandLine(int ac, char** av);

private:
    std::unique_ptr<OssExecutor> executor;
};