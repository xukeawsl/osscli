# 阿里云 OSS 对象存储客户端----osscli


## 安装 SDK 依赖
```bash
sudo apt-get install libcurl4-openssl-dev libssl-dev
```

## Linux 下构建

```bash
mkdir build
cd build
cmake ..
make
```

## 配置 OSS 用户账户
```json
{
    "Endpoint" : "your-endpoint",
    "AccessKeyId" : "your-access-key-id",
    "AccessKeySecret" : "your-access-key-secret"
}
```

## 支持功能

* 查看帮助
```bash
./osscli --help
```
* 创建 `Bucket`
```bash
./osscli --bucket "new-bucket-name" create
```
* 获取 `Object`
```bash
./osscli --bucket "bucket-name" get "object-name" "save-to-name"
```
* 上传 `Object`
```bash
# 超过 1GB 的文件通过 MultiPartUpload 来上传
./osscli --bucket "bucket-name" put "object-name" "loacl-name"
```
* 删除 `Object`
```bash
./osscli --bucket "bucket-name" delete "object-name1" "object-name2" ... "object-nameN"
```
* 查看 `Oject` 是否存在
```bash
./osscli --bucket "bucket-name" ping "object-name1" "object-name2" ... "object-nameN"
```
* 根据前缀查找 `Object`
```
./osscli --bucket "bucket-name" find "prefix-name"
```

依赖

* `aliyun-oss-cpp-sdk`
* `boost.program_options`
* `nlohmann`
