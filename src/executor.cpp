#include "executor.h"

#include "alibabacloud/oss/OssClient.h"

#include <fstream>
#include <iomanip>

namespace OSS = AlibabaCloud::OSS;

// 获取文件大小
int64_t getFileSize(const std::string& filename) {
    std::fstream f(filename, std::ios::in | std::ios::binary);
    f.seekg(0, f.end);
    int64_t size = f.tellg();
    f.close();
    return size;
}

// 打印错误信息
template <typename OutCome>
static void PrintErrorMessage(OutCome* outcome) {
    std::cout << "[Error]" << std::endl;
    std::cout << "code      :" << outcome->error().Code() << std::endl;
    std::cout << "message   :" << outcome->error().Message() << std::endl;
    std::cout << "requestId :" << outcome->error().RequestId() << std::endl;
}

// 打印头部信息
static void PrintHeadMessage(OSS::ListObjectOutcome* outcome) {
    size_t left = 0, mid = 0, right = 0;
    for (const auto& object : outcome->result().ObjectSummarys()) {
            left  = std::max(left, object.Key().size() + 4);
            mid   = std::max(mid , std::to_string(object.Size()).size() + 4);
            right = std::max(right, object.LastModified().size() + 4);
    }      

    std::cout << std::setw(left) << std::left << "name" << 
                 std::setw(mid) << std::left << "size" <<
                 std::setw(right) << std::left << "lastModifiedTime" << std::endl;
    std::cout << std::setw(left) << std::left << "----" << 
                 std::setw(mid) << std::left << "----" <<
                 std::setw(right) << std::left << "----------------" << std::endl;

    for (const auto& object : outcome->result().ObjectSummarys()) {
        std::cout << std::setw(left) << std::left << object.Key() << 
                     std::setw(mid) << std::left << object.Size() <<
                     std::setw(right) << std::left << object.LastModified() << std::endl;
    }
}

OssExecutor::OssExecutor()
    : type(Type::None) {
    OSS::InitializeSdk();
}

OssExecutor::OssExecutor(Type t) 
    : OssExecutor() {
    this->setType(t);
}

OssExecutor::~OssExecutor() {
    OSS::ShutdownSdk();
}

bool OssExecutor::Execute() {
    switch (type)
    {
    case Type::Create:  return Create();
    case Type::Head:    return Head();
    case Type::Get:     return Get();
    case Type::Put:     return Put();
    case Type::Delete:  return Delete();
    case Type::Ping:    return Ping();
    case Type::Find:    return CommonPrefix();
    default:
        break;
    }
    return false;
}

bool OssExecutor::Create() {
    OSS::ClientConfiguration conf;
    OSS::OssClient client(getEndpoint().c_str(), getAccessKeyId().c_str(),
                          getAccessKeySecret().c_str(), conf);
    OSS::CreateBucketRequest request(getBucketName().c_str(), OSS::StorageClass::Standard,
                                     OSS::CannedAccessControlList::Private);

    auto outcome = client.CreateBucket(request);
    if (!outcome.isSuccess()) {
        PrintErrorMessage(&outcome);
        return false;
    } else {
        std::cout << "[Success] Create Bucket : " << getBucketName() << std::endl;
    }
    return true;
}

bool OssExecutor::Head() {
    OSS::ClientConfiguration conf;
    OSS::OssClient client(getEndpoint().c_str(), getAccessKeyId().c_str(),
                          getAccessKeySecret().c_str(), conf);
    /* 列举文件。*/
    OSS::ListObjectsRequest request(getBucketName().c_str());
    auto outcome = client.ListObjects(request);

    if (!outcome.isSuccess()) {    
        /* 异常处理。*/
        PrintErrorMessage(&outcome);
        return false;  
    }
    else {
        PrintHeadMessage(&outcome);
    }
    return true;
}

bool OssExecutor::Get() {
    OSS::ClientConfiguration conf;
    OSS::OssClient client(getEndpoint().c_str(), getAccessKeyId().c_str(),
                          getAccessKeySecret().c_str(), conf);
    if (input_files.size() != 2) return false;
    
    OSS::GetObjectRequest request(getBucketName().c_str(), input_files[0].c_str());
    request.setResponseStreamFactory([&]{ 
        return std::make_shared<std::fstream>(input_files[1].c_str(), std::ios_base::out | std::ios_base::in | std::ios_base::trunc| std::ios_base::binary);
        });

    auto outcome = client.GetObject(request);
    if (!outcome.isSuccess()) {
        PrintErrorMessage(&outcome);
        return false;
    } else {
        std::cout << "[Success] Get Object : " << input_files[0].c_str() << std::endl;
    }
    return true;
}

bool OssExecutor::Put() {
    OSS::ClientConfiguration conf;
    OSS::OssClient client(getEndpoint().c_str(), getAccessKeyId().c_str(),
                          getAccessKeySecret().c_str(), conf);
    if (input_files.size() != 2) return false;

    auto fileSize = getFileSize(input_files[1]);

    if (fileSize < UploadLimitSize) {   // normal upload
        // (BucketName, ObjectName, LocalName)
        auto outcome = client.PutObject(getBucketName().c_str(), 
                                        input_files[0].c_str(), input_files[1].c_str());
        if (!outcome.isSuccess()) {
            PrintErrorMessage(&outcome);
            return false;
        } else {
            std::cout << "[Success] Put Object : " << input_files[0].c_str() << std::endl;
        }
    } else {    // multipart upload
        OSS::InitiateMultipartUploadRequest initUploadRequest(getBucketName().c_str(), input_files[0].c_str());
        auto uploadIdResult = client.InitiateMultipartUpload(initUploadRequest);
        auto uploadId = uploadIdResult.result().UploadId();
        std::string fileToUpload = input_files[1];
        int64_t partSize = 1024 * 1024;
        OSS::PartList partETagList;
        int partCount = static_cast<int>(fileSize / partSize);
        // 计算分片个数
        if (fileSize % partSize != 0) {
            ++partCount;
        }

        // 对每一个分片进行上传
        for (int i = 1; i <= partCount; i++) {
            auto skipBytes = partSize * (i - 1);
            auto size = (partSize < fileSize - skipBytes) ? partSize : (fileSize - skipBytes);
            std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(fileToUpload, std::ios::in|std::ios::binary);
            content->seekg(skipBytes, std::ios::beg);

            OSS::UploadPartRequest uploadPartRequest(getBucketName().c_str(), input_files[0].c_str(), 
                                                     content);
            uploadPartRequest.setContentLength(size);
            uploadPartRequest.setUploadId(uploadId);
            uploadPartRequest.setPartNumber(i);
            auto uploadPartOutcome = client.UploadPart(uploadPartRequest);
            if (uploadPartOutcome.isSuccess()) {
                OSS::Part part(i, uploadPartOutcome.result().ETag());
                partETagList.emplace_back(part);
            } else {
                PrintErrorMessage(&uploadPartOutcome);
            }
        }

        // 完成分片上传
        OSS::CompleteMultipartUploadRequest request(getBucketName().c_str(), input_files[0].c_str());
        request.setUploadId(uploadId);
        request.setPartList(partETagList);
        request.setAcl(OSS::CannedAccessControlList::Private);

        auto outcome = client.CompleteMultipartUpload(request);
        if (!outcome.isSuccess()) {
            PrintErrorMessage(&outcome);
            return false;
        } else {
            std::cout << "[Success] MultipartUpload Object : " << input_files[0].c_str() << std::endl;
        }
    }
    return true;
}

bool OssExecutor::Delete() {
    OSS::ClientConfiguration conf;
    OSS::OssClient client(getEndpoint().c_str(), getAccessKeyId().c_str(),
                          getAccessKeySecret().c_str(), conf);
    
    for (const auto& input_file : input_files) {
        OSS::DeleteObjectRequest request(getBucketName().c_str(), input_file.c_str());

        auto outcome = client.DeleteObject(request);
        if (!outcome.isSuccess()) {
            PrintErrorMessage(&outcome);
            return false;
        } else {
            std::cout << "[Success] Delete Object : " << input_file << std::endl;
        }
    }

    return true;
}

bool OssExecutor::Ping() {
    OSS::ClientConfiguration conf;
    OSS::OssClient client(getEndpoint().c_str(), getAccessKeyId().c_str(),
                          getAccessKeySecret().c_str(), conf);

    for (const auto& input_file : input_files) {
        
        auto outcome = client.DoesObjectExist(getBucketName().c_str(), input_file.c_str());
        if (!outcome) {
            std::cout << "[Error] The Object (" << input_file << ") does not exists!" << std::endl; 
        } else {
            std::cout << "[Success] The Object (" << input_file << ") exists!" << std::endl;
        }
    }

    return true;
}

bool OssExecutor::CommonPrefix() {
    OSS::ClientConfiguration conf;
    OSS::OssClient client(getEndpoint().c_str(), getAccessKeyId().c_str(),
                          getAccessKeySecret().c_str(), conf);
    if (input_files.size() != 1) return false;

    std::string keyPrefix = input_files[0];
    std::string nextMarker = "";
    bool isTruncated = false;
    do {
        OSS::ListObjectsRequest request(getBucketName().c_str());
        request.setPrefix(keyPrefix);
        request.setMarker(nextMarker);
        auto outcome = client.ListObjects(request);

        if (!outcome.isSuccess()) {
            PrintErrorMessage(&outcome);
            return false;
        }

        PrintHeadMessage(&outcome);
        nextMarker = outcome.result().NextMarker();
        isTruncated = outcome.result().IsTruncated();
    } while (isTruncated);

    return true;
}
