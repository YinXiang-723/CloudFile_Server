/**
 * ApiFileMove.cpp
 * 文件移动API实现
 */

#include "ApiFileMove.h"
#include "ApiCommon.h"
#include "DBPool.h"
#include "Logging.h"
#include "json/json.h"

// 移动文件到指定文件夹
int moveFileToFolder(const std::string &user, const std::string &file_md5, int folder_id, std::string &str_json)
{
    int ret = 0;
    int user_id = 0;
    CDBManager *pDBManager = CDBManager::getInstance();
    CDBConn *pDBConn = pDBManager->GetDBConn("tuchuang_slave");
    AUTO_REL_DBCONN(pDBManager, pDBConn);

    // 获取用户ID
    char sql_cmd[SQL_MAX_LEN] = {0};
    sprintf(sql_cmd, "SELECT id FROM user_info WHERE user_name='%s'", user.c_str());
    CResultSet *pResultSet = pDBConn->ExecuteQuery(sql_cmd);
    if (pResultSet && pResultSet->Next())
    {
        user_id = pResultSet->GetInt("id");
    }
    else
    {
        LOG_ERROR << "user not found: " << user;
        ret = -1;
        goto END;
    }
    delete pResultSet;

    // 检查文件是否存在且属于该用户
    sprintf(sql_cmd, "SELECT id, file_name FROM user_file_list WHERE user='%s' AND md5='%s'", user.c_str(), file_md5.c_str());
    pResultSet = pDBConn->ExecuteQuery(sql_cmd);
    if (!pResultSet || !pResultSet->Next())
    {
        LOG_ERROR << "file not found or not belong to user: " << file_md5;
        ret = -2; // 文件不存在或不属于该用户
        goto END;
    }
    delete pResultSet;

    // 检查文件夹是否存在且属于该用户
    if (folder_id > 0)
    {
        sprintf(sql_cmd, "SELECT id FROM folders WHERE id=%d AND user_id=%d AND is_deleted=0", folder_id, user_id);
        pResultSet = pDBConn->ExecuteQuery(sql_cmd);
        if (!pResultSet || !pResultSet->Next())
        {
            LOG_ERROR << "folder not found or not belong to user: " << folder_id;
            ret = -3; // 文件夹不存在或不属于该用户
            goto END;
        }
        delete pResultSet;
    }

    // 移动文件到指定文件夹
    if (folder_id > 0)
    {
        sprintf(sql_cmd, "UPDATE user_file_list SET folder_id=%d WHERE user='%s' AND md5='%s'", folder_id, user.c_str(), file_md5.c_str());
    }
    else
    {
        sprintf(sql_cmd, "UPDATE user_file_list SET folder_id=NULL WHERE user='%s' AND md5='%s'", user.c_str(), file_md5.c_str());
    }
    if (!pDBConn->ExecuteUpdate(sql_cmd))
    {
        LOG_ERROR << "move file to folder failed: " << sql_cmd;
        ret = -1;
        goto END;
    }

    LOG_INFO << "move file to folder success: " << file_md5 << " -> " << folder_id;

    // 返回成功响应
    Json::Value root;
    root["code"] = 0;
    Json::FastWriter writer;
    str_json = writer.write(root);

END:
    return ret;
}

// 批量移动文件到指定文件夹
int batchMoveFilesToFolder(const std::string &user, const Json::Value &file_md5s, int folder_id, std::string &str_json)
{
    int ret = 0;
    int success_count = 0;
    int fail_count = 0;
    Json::Value result_array(Json::arrayValue);

    // 遍历所有文件，逐个移动
    for (unsigned int i = 0; i < file_md5s.size(); i++)
    {
        std::string file_md5 = file_md5s[i].asString();
        std::string result_str;
        int move_ret = moveFileToFolder(user, file_md5, folder_id, result_str);

        Json::Value result;
        result["file_md5"] = file_md5;
        result["success"] = (move_ret == 0);
        result["message"] = (move_ret == 0) ? "success" : "failed";
        result_array.append(result);

        if (move_ret == 0)
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

// 初始化文件移动API
int ApiFileMoveInit()
{
    // 可以在这里进行一些初始化操作
    return 0;
}
