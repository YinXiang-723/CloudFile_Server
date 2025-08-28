/**
 * ApiBatchOperation.cpp
 * 批量操作API实现
 */

#include "ApiBatchOperation.h"
#include "ApiCommon.h"
#include "DBPool.h"
#include "CachePool.h"
#include "Logging.h"
#include "json/json.h"
#include <vector>
#include <map>

// 批量删除文件
int batchDeleteFiles(const std::string &user, const Json::Value &file_md5s, std::string &str_json)
{
    int ret = 0;
    int success_count = 0;
    int fail_count = 0;
    Json::Value result_array(Json::arrayValue);

    CDBManager *pDBManager = CDBManager::getInstance();
    CDBConn *pDBConn = pDBManager->GetDBConn("tuchuang_slave");
    AUTO_REL_DBCONN(pDBManager, pDBConn);

    CacheManager *pCacheManager = CacheManager::getInstance();
    CacheConn *pCacheConn = pCacheManager->GetCacheConn("token");
    AUTO_REL_CACHECONN(pCacheManager, pCacheConn);

    // 遍历所有文件，逐个删除
    for (unsigned int i = 0; i < file_md5s.size(); i++)
    {
        std::string file_md5 = file_md5s[i].asString();
        std::string result_str;
        int delete_ret = -1;

        // 检查文件是否存在且属于该用户
        char sql_cmd[SQL_MAX_LEN] = {0};
        sprintf(sql_cmd, "SELECT id, file_name FROM user_file_list WHERE user='%s' AND md5='%s'", user.c_str(), file_md5.c_str());
        CResultSet *pResultSet = pDBConn->ExecuteQuery(sql_cmd);
        if (!pResultSet || !pResultSet->Next())
        {
            LOG_ERROR << "file not found or not belong to user: " << file_md5;
            delete_ret = -2; // 文件不存在或不属于该用户
        }
        else
        {
            int file_id = pResultSet->GetInt("id");
            std::string file_name = pResultSet->GetString("file_name");
            delete pResultSet;

            // 删除用户文件列表中的记录
            sprintf(sql_cmd, "DELETE FROM user_file_list WHERE id=%d", file_id);
            if (!pDBConn->ExecuteUpdate(sql_cmd))
            {
                LOG_ERROR << "delete file from user_file_list failed: " << sql_cmd;
                delete_ret = -3; // 删除失败
            }
            else
            {
                // 更新用户文件计数
                if(CacheDecrCount(pCacheConn, string(user)) < 0)
                {
                    LOG_ERROR << "decrement user file count failed for user: " << user;
                    // 不影响删除操作的结果
                }

                delete_ret = 0; // 删除成功
            }
        }

        Json::Value result;
        result["file_md5"] = file_md5;
        result["success"] = (delete_ret == 0);
        result["message"] = (delete_ret == 0) ? "success" : "failed";
        result_array.append(result);

        if (delete_ret == 0)
        {
            success_count++;
        }
        else
        {
            fail_count++;
        }
    }

    // 返回批量操作结果
    Json::Value root;
    root["code"] = (fail_count == 0) ? 0 : -1;
    root["success_count"] = success_count;
    root["fail_count"] = fail_count;
    root["results"] = result_array;
    Json::FastWriter writer;
    str_json = writer.write(root);

    return ret;
}

// 批量分享文件
int batchShareFiles(const std::string &user, const Json::Value &file_md5s, std::string &str_json)
{
    int ret = 0;
    int success_count = 0;
    int fail_count = 0;
    Json::Value result_array(Json::arrayValue);

    CDBManager *pDBManager = CDBManager::getInstance();
    CDBConn *pDBConn = pDBManager->GetDBConn("tuchuang_slave");
    AUTO_REL_DBCONN(pDBManager, pDBConn);

    // 遍历所有文件，逐个分享
    for (unsigned int i = 0; i < file_md5s.size(); i++)
    {
        std::string file_md5 = file_md5s[i].asString();
        int share_ret = -1;

        // 检查文件是否存在且属于该用户
        char sql_cmd[SQL_MAX_LEN] = {0};
        sprintf(sql_cmd, "SELECT id, file_name FROM user_file_list WHERE user='%s' AND md5='%s'", user.c_str(), file_md5.c_str());
        CResultSet *pResultSet = pDBConn->ExecuteQuery(sql_cmd);
        if (!pResultSet || !pResultSet->Next())
        {
            LOG_ERROR << "file not found or not belong to user: " << file_md5;
            share_ret = -2; // 文件不存在或不属于该用户
        }
        else
        {
            int file_id = pResultSet->GetInt("id");
            std::string file_name = pResultSet->GetString("file_name");
            delete pResultSet;

            // 检查文件是否已经分享
            sprintf(sql_cmd, "SELECT id FROM share_file_list WHERE user='%s' AND md5='%s'", user.c_str(), file_md5.c_str());
            pResultSet = pDBConn->ExecuteQuery(sql_cmd);
            if (pResultSet && pResultSet->Next())
            {
                LOG_ERROR << "file already shared: " << file_md5;
                share_ret = -3; // 文件已经分享
            }
            else
            {
                delete pResultSet;

                // 添加到分享文件列表
                sprintf(sql_cmd, "INSERT INTO share_file_list (user, md5, file_name) VALUES ('%s', '%s', '%s')", 
                        user.c_str(), file_md5.c_str(), file_name.c_str());
                if (!pDBConn->ExecuteCreate(sql_cmd))
                {
                    LOG_ERROR << "add file to share list failed: " << sql_cmd;
                    share_ret = -4; // 添加到分享列表失败
                }
                else
                {
                    // 更新用户文件列表中的共享状态
                    sprintf(sql_cmd, "UPDATE user_file_list SET shared_status=1 WHERE id=%d", file_id);
                    if (!pDBConn->ExecuteUpdate(sql_cmd))
                    {
                        LOG_ERROR << "update file shared status failed: " << sql_cmd;
                        // 不影响分享操作的结果
                    }

                    share_ret = 0; // 分享成功
                }
            }
        }

        Json::Value result;
        result["file_md5"] = file_md5;
        result["success"] = (share_ret == 0);
        result["message"] = (share_ret == 0) ? "success" : "failed";
        result_array.append(result);

        if (share_ret == 0)
        {
            success_count++;
        }
        else
        {
            fail_count++;
        }
    }

    // 返回批量操作结果
    Json::Value root;
    root["code"] = (fail_count == 0) ? 0 : -1;
    root["success_count"] = success_count;
    root["fail_count"] = fail_count;
    root["results"] = result_array;
    Json::FastWriter writer;
    str_json = writer.write(root);

    return ret;
}

// 批量上传文件到指定文件夹
int batchUploadFilesToFolder(const std::string &user, const Json::Value &files, int folder_id, std::string &str_json)
{
    int ret = 0;
    int success_count = 0;
    int fail_count = 0;
    Json::Value result_array(Json::arrayValue);

    // 遍历所有文件，逐个上传
    for (unsigned int i = 0; i < files.size(); i++)
    {
        Json::Value file = files[i];
        std::string file_name = file["file_name"].asString();
        std::string file_content = file["file_content"].asString(); // 这里应该是文件的Base64编码内容
        std::string file_md5 = file["file_md5"].asString(); // 这里应该是文件的MD5值

        Json::Value result;
        result["file_name"] = file_name;
        result["success"] = false;
        result["message"] = "not implemented";

        // TODO: 实现文件上传逻辑，这里只是示例
        // 实际实现应该调用文件上传的相关函数

        result_array.append(result);

        // 暂时都算作失败
        fail_count++;
    }

    // 返回批量操作结果
    Json::Value root;
    root["code"] = (fail_count == 0) ? 0 : -1;
    root["success_count"] = success_count;
    root["fail_count"] = fail_count;
    root["results"] = result_array;
    Json::FastWriter writer;
    str_json = writer.write(root);

    return ret;
}

// 初始化批量操作API
int ApiBatchOperationInit()
{
    // 可以在这里进行一些初始化操作
    return 0;
}
