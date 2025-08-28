/**
 * ApiFolderHandler.cpp
 * 文件夹管理API处理实现
 */

#include "ApiFolderHandler.h"
#include "ApiFolder.h"
#include "ApiCommon.h"
#include <json/json.h>

// 处理获取用户文件夹列表请求
int handleGetUserFolders(const std::string &url, const std::string &post_data, std::string &str_json)
{
    // 解析JSON数据
    Json::Value root;
    Json::Reader jsonReader;
    if (!jsonReader.parse(post_data, root))
    {
        LOG_ERROR << "parse get folders json failed";
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

    // 调用获取用户文件夹列表函数
    return getUserFolders(user, str_json);
}

// 处理创建文件夹请求
int handleCreateFolder(const std::string &url, const std::string &post_data, std::string &str_json)
{
    // 解析JSON数据
    Json::Value root;
    Json::Reader jsonReader;
    if (!jsonReader.parse(post_data, root))
    {
        LOG_ERROR << "parse create folder json failed";
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

    // 获取文件夹名称和父文件夹ID
    std::string folder_name = root["folder_name"].asString();
    int parent_id = root["parent_id"].isNull() ? 0 : root["parent_id"].asInt();

    // 调用创建文件夹函数
    return createFolder(user, folder_name, parent_id, str_json);
}

// 处理更新文件夹名称请求
int handleUpdateFolderName(const std::string &url, const std::string &post_data, std::string &str_json)
{
    // 解析JSON数据
    Json::Value root;
    Json::Reader jsonReader;
    if (!jsonReader.parse(post_data, root))
    {
        LOG_ERROR << "parse update folder name json failed";
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

    // 获取文件夹ID和新名称
    int folder_id = root["folder_id"].asInt();
    std::string new_name = root["new_name"].asString();

    // 调用更新文件夹名称函数
    return updateFolderName(user, folder_id, new_name, str_json);
}

// 处理删除文件夹请求
int handleDeleteFolder(const std::string &url, const std::string &post_data, std::string &str_json)
{
    // 解析JSON数据
    Json::Value root;
    Json::Reader jsonReader;
    if (!jsonReader.parse(post_data, root))
    {
        LOG_ERROR << "parse delete folder json failed";
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

    // 获取文件夹ID
    int folder_id = root["folder_id"].asInt();

    // 调用删除文件夹函数
    return deleteFolder(user, folder_id, str_json);
}

// 处理移动文件夹请求
int handleMoveFolder(const std::string &url, const std::string &post_data, std::string &str_json)
{
    // 解析JSON数据
    Json::Value root;
    Json::Reader jsonReader;
    if (!jsonReader.parse(post_data, root))
    {
        LOG_ERROR << "parse move folder json failed";
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

    // 获取文件夹ID和新父文件夹ID
    int folder_id = root["folder_id"].asInt();
    int new_parent_id = root["new_parent_id"].asInt();

    // 调用移动文件夹函数
    return moveFolder(user, folder_id, new_parent_id, str_json);
}

// 处理获取文件夹中的文件列表请求
int handleGetFolderFiles(const std::string &url, const std::string &post_data, std::string &str_json)
{
    // 解析JSON数据
    Json::Value root;
    Json::Reader jsonReader;
    if (!jsonReader.parse(post_data, root))
    {
        LOG_ERROR << "parse get folder files json failed";
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

    // 获取文件夹ID
    int folder_id = root["folder_id"].asInt();
    if (folder_id <= 0)
    {
        LOG_ERROR << "folder_id invalid: " << folder_id;
        return -1;
    }

    // 调用获取文件夹中的文件列表函数
    return getFolderFiles(user, folder_id, str_json);
}
