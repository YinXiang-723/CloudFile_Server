/**
 * ApiBatchOperationHandler.cpp
 * 批量操作API处理实现
 */

#include "ApiBatchOperationHandler.h"
#include "ApiBatchOperation.h"
#include "ApiCommon.h"
#include <json/json.h>

// 处理批量删除文件请求
int handleBatchDeleteFiles(const std::string &url, const std::string &post_data, std::string &str_json)
{
    // 解析JSON数据
    Json::Value root;
    Json::Reader jsonReader;
    if (!jsonReader.parse(post_data, root))
    {
        LOG_ERROR << "parse batch delete files json failed";
        return -1;
    }

    // 验证用户token
    std::string user = root["user"].asString();
    std::string token = root["token"].asString();
    if (!VerifyToken(user, token))
    {
        LOG_ERROR << "token invalid for user: " << user;
        return -2; // token无效
    }

    // 获取文件MD5数组
    Json::Value file_md5s = root["file_md5s"];
    if (!file_md5s.isArray() || file_md5s.empty())
    {
        LOG_ERROR << "file_md5s is empty or not an array";
        return -3;
    }

    // 调用批量删除文件函数
    return batchDeleteFiles(user, file_md5s, str_json);
}

// 处理批量分享文件请求
int handleBatchShareFiles(const std::string &url, const std::string &post_data, std::string &str_json)
{
    // 解析JSON数据
    Json::Value root;
    Json::Reader jsonReader;
    if (!jsonReader.parse(post_data, root))
    {
        LOG_ERROR << "parse batch share files json failed";
        return -1;
    }

    // 验证用户token
    std::string user = root["user"].asString();
    std::string token = root["token"].asString();
    if (!VerifyToken(user, token))
    {
        LOG_ERROR << "token invalid for user: " << user;
        return -2; // token无效
    }

    // 获取文件MD5数组
    Json::Value file_md5s = root["file_md5s"];
    if (!file_md5s.isArray() || file_md5s.empty())
    {
        LOG_ERROR << "file_md5s is empty or not an array";
        return -3;
    }

    // 调用批量分享文件函数
    return batchShareFiles(user, file_md5s, str_json);
}

// 处理批量上传文件到指定文件夹请求
int handleBatchUploadFilesToFolder(const std::string &url, const std::string &post_data, std::string &str_json)
{
    // 解析JSON数据
    Json::Value root;
    Json::Reader jsonReader;
    if (!jsonReader.parse(post_data, root))
    {
        LOG_ERROR << "parse batch upload files json failed";
        return -1;
    }

    // 验证用户token
    std::string user = root["user"].asString();
    std::string token = root["token"].asString();
    if (!VerifyToken(user, token))
    {
        LOG_ERROR << "token invalid for user: " << user;
        return -2; // token无效
    }

    // 获取文件数组和文件夹ID
    Json::Value files = root["files"];
    if (!files.isArray() || files.empty())
    {
        LOG_ERROR << "files is empty or not an array";
        return -3;
    }

    int folder_id = root["folder_id"].asInt();

    // 调用批量上传文件到文件夹函数
    return batchUploadFilesToFolder(user, files, folder_id, str_json);
}
