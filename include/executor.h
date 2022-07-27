#pragma once

#include <vector>
#include <string>

class OssExecutor {
public:
    enum class Type : int { Create, Head, Get, Put, Delete, None };

    OssExecutor();
    explicit OssExecutor(Type t);

    ~OssExecutor();

    OssExecutor(const OssExecutor&) = delete;
    OssExecutor& operator=(const OssExecutor&) = delete;

    bool Execute();

    inline void setType(Type t) { type = t; }

    inline void setBucketName(std::string&& b) { bucket_name = std::move(b); }

    inline void setEndpoint(std::string&& e) { endpoint = std::move(e); }

    inline void setAccessKeyId(std::string&& ki) { access_key_id = std::move(ki); }

    inline void setAccessKeySecret(std::string&& ks) { access_key_secret = std::move(ks); }

    inline void setInputFiles(std::vector<std::string>&& in) { input_files = std::move(in); }

    inline Type getType() const { return type; }

    inline std::string getBucketName() const { return bucket_name; }

    inline std::string getEndpoint() const { return endpoint; }

    inline std::string getAccessKeyId() const { return access_key_id; }

    inline std::string getAccessKeySecret() const { return access_key_secret; }

private:
    // 1 GB 以上的文件采用分片传输 MultiUpload
    static constexpr int64_t UploadLimitSize = 1024 * 1024 * 1024;

    bool Create();

    bool Head();

    bool Get();

    bool Put();

    bool Delete();

private:
    Type type;
    std::string bucket_name;
    std::string endpoint;
    std::string access_key_id;
    std::string access_key_secret;
    std::vector<std::string> input_files;
};

