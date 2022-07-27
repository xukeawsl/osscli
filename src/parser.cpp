#include "parser.h"
#include "program_options.hpp"
#include "json.hpp"

#include <fstream>
#include <iostream>

namespace po = boost::program_options;

// 字符串转为全小写
static std::string strToLower(std::string str) {
    for (auto& ch : str) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = ch - 'A' + 'a';
        }
    }
    return str;
}

bool OssParser::ParseConfigFile(const std::string& filename) {
    std::ifstream f(filename);
    nlohmann::json data = nlohmann::json::parse(f);
    std::string endpoint =          data["Endpoint"].get<std::string>();
    std::string access_key_id =     data["AccessKeyId"].get<std::string>();
    std::string access_key_secret = data["AccessKeySecret"].get<std::string>();
    if (endpoint.empty() || access_key_id.empty() || access_key_secret.empty()) {
        return false;
    }
    executor->setEndpoint(std::move(endpoint));
    executor->setAccessKeyId(std::move(access_key_id));
    executor->setAccessKeySecret(std::move(access_key_secret));
    return true;
}

std::unique_ptr<OssExecutor> OssParser::ParseCommandLine(int ac, char** av) {
    po::options_description desc("Options are");
    desc.add_options()
        ("help,h", "Display usage information (this message)")
        ("bucket,b", po::value<std::string>(), "OSS Bucket Name")
        ("input-files,f", po::value<std::vector<std::string>>(), "Input Files")
    ;

    po::positional_options_description p;
    p.add("input-files", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(ac, av).
            options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << "Usage: osscli [options] method [input-files...]" << std::endl;
        std::cout << desc << "\n";
        return std::move(executor);
    }

    if (vm.count("bucket")) {
        std::string BucketName = vm["bucket"].as<std::string>();
        executor->setBucketName(std::move(BucketName));
    } else {
        return nullptr;
    }

    // input-files : method [names...]
    if (vm.count("input-files")) {
        auto files = vm["input-files"].as<std::vector<std::string>>();
        // method
        std::string method = strToLower(files[0]);
        files.erase(files.begin());
        if (method == "create") {
            executor->setType(OssExecutor::Type::Create);
        } else if (method == "head") {
            executor->setType(OssExecutor::Type::Head);
        } else if (method == "get") {
            executor->setType(OssExecutor::Type::Get);
        } else if (method == "put") {
            executor->setType(OssExecutor::Type::Put);
        } else if (method == "delete") {
            executor->setType(OssExecutor::Type::Delete);
        }
        executor->setInputFiles(std::move(files));
    } else {
        return nullptr;
    }

    return std::move(executor);
}