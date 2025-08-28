/**
 * ApiFileMoveHandler.cpp
 * 文件移动API处理实现
 */

#include "ApiFileMoveHandler.h"
#include "ApiFileMove.h"
#include "ApiCommon.h"
#include <json/json.h>

// 处理移动文件到指定文件夹请求
int handleMoveFileToFolder(const std::string &url, const std::string &post_data, std::string &str_json)
{
    // 解析JSON数据
    Json::Value root;
    Json::Reader jsonReader;
    if (!jsonReader.parse(post_data, root))
    {
        LOG_ERROR << "parse move file json failed";
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

    // 获取文件MD5和文件夹ID
    std::string file_md5 = root["file_md5"].asString();
    int folder_id = root["folder_id"].asInt();

    // 调用移动文件到文件夹函数
    return moveFileToFolder(user, file_md5, folder_id, str_json);
}

// 处理批量移动文件到指定文件夹请求
int handleBatchMoveFilesToFolder(const std::string &url, const std::string &post_data, std::string &str_json)
{
    // 解析JSON数据
    Json::Value root;
    Json::Reader jsonReader;
    if (!jsonReader.parse(post_data, root))
    {
        LOG_ERROR << "parse batch move files json failed";
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

    // 获取文件MD5数组和文件夹ID
    Json::Value file_md5s = root["file_md5s"];
    if (!file_md5s.isArray() || file_md5s.empty())
    {
        LOG_ERROR << "file_md5s is empty or not an array";
        return -3;
    }

    int folder_id = root["folder_id"].asInt();

    // 调用批量移动文件到文件夹函数
    return batchMoveFilesToFolder(user, file_md5s, folder_id, str_json);
}
