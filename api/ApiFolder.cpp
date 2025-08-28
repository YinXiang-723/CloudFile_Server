/**
 * ApiFolder.cpp
 * 文件夹管理API实现
 */

#include "ApiFolder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#include "fdfs_client.h"
#include "ApiCommon.h"
#include "DBPool.h"
#include "CachePool.h"
#include "Logging.h"
#include "json/json.h"
#include "util.h"

// 解析文件夹相关JSON数据
int decodeFolderJson(const std::string &str_json, int &user_id, std::string &folder_name, int &parent_id)
{
    bool res;
    Json::Value root;
    Json::Reader jsonReader;
    res = jsonReader.parse(str_json, root);
    if (!res)
    {
        LOG_ERROR << "parse folder json failed";
        return -1;
    }

    // 用户ID
    if (root["user_id"].isNull())
    {
        LOG_ERROR << "user_id null";
        return -1;
    }
    user_id = root["user_id"].asInt();

    // 文件夹名称
    if (root["folder_name"].isNull())
    {
        LOG_ERROR << "folder_name null";
        return -1;
    }
    folder_name = root["folder_name"].asString();

    // 父文件夹ID
    if (root["parent_id"].isNull())
    {
        LOG_ERROR << "parent_id null";
        return -1;
    }
    parent_id = root["parent_id"].asInt();

    return 0;
}

// 封装文件夹操作的JSON响应
int encodeFolderJson(int ret, std::string &str_json)
{
    Json::Value root;
    root["code"] = ret;
    Json::FastWriter writer;
    str_json = writer.write(root);
    return 0;
}

// 获取用户文件夹列表
int getUserFolders(const std::string &user, std::string &str_json)
{
    int ret = 0;
    int user_id = 0;
    CDBManager *pDBManager = CDBManager::getInstance();
    CDBConn *pDBConn = pDBManager->GetDBConn("tuchuang_slave");
    AUTO_REL_DBCONN(pDBManager, pDBConn);
    
    // 提前初始化所有变量，避免goto跨越初始化
    char sql_cmd[SQL_MAX_LEN] = {0};
    CResultSet *pResultSet = NULL;
    Json::Value root;
    Json::Value folder_tree;
    Json::Value root_folder;
    std::map<int, Json::Value> folder_map;
    Json::FastWriter writer;

    // 获取用户ID
    sprintf(sql_cmd, "SELECT id FROM user_info WHERE user_name='%s'", user.c_str());
    pResultSet = pDBConn->ExecuteQuery(sql_cmd);
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

    // 获取用户的文件夹列表
    sprintf(sql_cmd, "SELECT id, parent_id, folder_name, create_time, update_time FROM folders WHERE user_id=%d AND is_deleted=0 ORDER BY parent_id, folder_name", user_id);
    pResultSet = pDBConn->ExecuteQuery(sql_cmd);
    if (!pResultSet)
    {
        LOG_ERROR << "query folders failed";
        ret = -1;
        goto END;
    }
    
    // 创建根节点
    root_folder["id"] = 0;
    root_folder["parent_id"] = -1; // 根节点的父ID设为-1
    root_folder["folder_name"] = "我的文件";
    root_folder["create_time"] = "";
    root_folder["update_time"] = "";
    root_folder["children"] = Json::Value(Json::arrayValue);
    folder_tree.append(root_folder);
    
    
    // 添加所有文件夹到映射
    while (pResultSet->Next())
    {
        int folder_id = pResultSet->GetInt("id");
        int parent_id = pResultSet->GetInt("parent_id");
        
        Json::Value folder;
        folder["id"] = folder_id;
        folder["parent_id"] = parent_id;
        folder["folder_name"] = pResultSet->GetString("folder_name");
        folder["create_time"] = pResultSet->GetString("create_time");
        folder["update_time"] = pResultSet->GetString("update_time");
        folder["children"] = Json::Value(Json::arrayValue);
        
        folder_map[folder_id] = folder;
    }
    delete pResultSet;
    
    // 构建树形结构
    for (std::map<int, Json::Value>::iterator it = folder_map.begin(); it != folder_map.end(); ++it)
    {
        int folder_id = it->first;
        Json::Value &folder = it->second;
        int parent_id = folder["parent_id"].asInt();
        
        // 查找父节点
        if (parent_id == 0)
        {
            // 如果是根文件夹，添加到根节点
            folder_tree[0]["children"].append(folder);
        }
        else
        {
            // 查找父节点
            std::map<int, Json::Value>::iterator parent_it = folder_map.find(parent_id);
            if (parent_it != folder_map.end())
            {
                // 添加到父节点的children中
                parent_it->second["children"].append(folder);
            }
        }
    }
    
    root["code"] = 0;
    root["folders"] = folder_tree;
    
    str_json = writer.write(root);

END:
    return ret;
}

// 创建文件夹
int createFolder(const std::string &user, const std::string &folder_name, int parent_id, std::string &str_json)
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

    // 检查父文件夹是否存在
    if (parent_id > 0)
    {
        sprintf(sql_cmd, "SELECT id FROM folders WHERE id=%d AND user_id=%d AND is_deleted=0", parent_id, user_id);
        pResultSet = pDBConn->ExecuteQuery(sql_cmd);
        if (!pResultSet || !pResultSet->Next())
        {
            LOG_ERROR << "parent folder not found or not belong to user: " << parent_id;
            ret = -2; // 父文件夹不存在或不属于该用户
            goto END;
        }
        delete pResultSet;
    }

    // 检查同名文件夹是否已存在
    sprintf(sql_cmd, "SELECT id FROM folders WHERE user_id=%d AND parent_id=%d AND folder_name='%s' AND is_deleted=0", 
            user_id, parent_id, folder_name.c_str());
    pResultSet = pDBConn->ExecuteQuery(sql_cmd);
    if (pResultSet && pResultSet->Next())
    {
        LOG_ERROR << "folder already exists: " << folder_name;
        ret = -3; // 同名文件夹已存在
        goto END;
    }
    delete pResultSet;

    // 创建新文件夹
    sprintf(sql_cmd, "INSERT INTO folders (user_id, parent_id, folder_name) VALUES (%d, %d, '%s')", 
            user_id, parent_id, folder_name.c_str());
    if (!pDBConn->ExecuteCreate(sql_cmd))
    {
        LOG_ERROR << "create folder failed: " << sql_cmd;
        ret = -1;
        goto END;
    }

    // 获取新创建的文件夹ID
    int new_folder_id = pDBConn->GetInsertId();
    LOG_INFO << "create folder success: " << new_folder_id;

    // 返回成功响应
    Json::Value root;
    root["code"] = 0;
    root["folder_id"] = new_folder_id;
    Json::FastWriter writer;
    str_json = writer.write(root);

END:
    return ret;
}

// 更新文件夹名称
int updateFolderName(const std::string &user, int folder_id, const std::string &new_name, std::string &str_json)
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

    // 检查文件夹是否存在且属于该用户
    sprintf(sql_cmd, "SELECT id FROM folders WHERE id=%d AND user_id=%d AND is_deleted=0", folder_id, user_id);
    pResultSet = pDBConn->ExecuteQuery(sql_cmd);
    if (!pResultSet || !pResultSet->Next())
    {
        LOG_ERROR << "folder not found or not belong to user: " << folder_id;
        ret = -2; // 文件夹不存在或不属于该用户
        goto END;
    }
    delete pResultSet;

    // 检查同名文件夹是否已存在（排除当前文件夹）
    sprintf(sql_cmd, "SELECT id FROM folders WHERE user_id=%d AND parent_id=(SELECT parent_id FROM folders WHERE id=%d) AND folder_name='%s' AND id!=%d AND is_deleted=0", 
            user_id, folder_id, new_name.c_str(), folder_id);
    pResultSet = pDBConn->ExecuteQuery(sql_cmd);
    if (pResultSet && pResultSet->Next())
    {
        LOG_ERROR << "folder with same name already exists: " << new_name;
        ret = -3; // 同名文件夹已存在
        goto END;
    }
    delete pResultSet;

    // 更新文件夹名称
    sprintf(sql_cmd, "UPDATE folders SET folder_name='%s' WHERE id=%d", new_name.c_str(), folder_id);
    if (!pDBConn->ExecuteUpdate(sql_cmd))
    {
        LOG_ERROR << "update folder name failed: " << sql_cmd;
        ret = -1;
        goto END;
    }

    LOG_INFO << "update folder name success: " << folder_id << " -> " << new_name;

    // 返回成功响应
    Json::Value root;
    root["code"] = 0;
    Json::FastWriter writer;
    str_json = writer.write(root);

END:
    return ret;
}

// 删除文件夹
int deleteFolder(const std::string &user, int folder_id, std::string &str_json)
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

    // 检查文件夹是否存在且属于该用户
    sprintf(sql_cmd, "SELECT id, parent_id FROM folders WHERE id=%d AND user_id=%d AND is_deleted=0", folder_id, user_id);
    pResultSet = pDBConn->ExecuteQuery(sql_cmd);
    if (!pResultSet || !pResultSet->Next())
    {
        LOG_ERROR << "folder not found or not belong to user: " << folder_id;
        ret = -2; // 文件夹不存在或不属于该用户
        goto END;
    }
    int parent_id = pResultSet->GetInt("parent_id");
    delete pResultSet;

    // 检查是否为根文件夹
    if (parent_id == 0)
    {
        LOG_ERROR << "cannot delete root folder: " << folder_id;
        ret = -3; // 不能删除根文件夹
        goto END;
    }

    // 检查文件夹是否包含子文件夹
    sprintf(sql_cmd, "SELECT id FROM folders WHERE parent_id=%d AND is_deleted=0", folder_id);
    pResultSet = pDBConn->ExecuteQuery(sql_cmd);
    if (pResultSet && pResultSet->Next())
    {
        LOG_ERROR << "folder contains subfolders, cannot delete: " << folder_id;
        ret = -4; // 文件夹包含子文件夹
        goto END;
    }
    delete pResultSet;

    // 检查文件夹是否包含文件
    sprintf(sql_cmd, "SELECT id FROM user_file_list WHERE folder_id=%d", folder_id);
    pResultSet = pDBConn->ExecuteQuery(sql_cmd);
    if (pResultSet && pResultSet->Next())
    {
        LOG_ERROR << "folder contains files, cannot delete: " << folder_id;
        ret = -5; // 文件夹包含文件
        goto END;
    }
    delete pResultSet;

    // 软删除文件夹
    sprintf(sql_cmd, "UPDATE folders SET is_deleted=1 WHERE id=%d", folder_id);
    if (!pDBConn->ExecuteUpdate(sql_cmd))
    {
        LOG_ERROR << "delete folder failed: " << sql_cmd;
        ret = -1;
        goto END;
    }

    LOG_INFO << "delete folder success: " << folder_id;

    // 返回成功响应
    Json::Value root;
    root["code"] = 0;
    Json::FastWriter writer;
    str_json = writer.write(root);

END:
    return ret;
}

// 移动文件夹
int moveFolder(const std::string &user, int folder_id, int new_parent_id, std::string &str_json)
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

    // 检查文件夹是否存在且属于该用户
    sprintf(sql_cmd, "SELECT id, parent_id FROM folders WHERE id=%d AND user_id=%d AND is_deleted=0", folder_id, user_id);
    pResultSet = pDBConn->ExecuteQuery(sql_cmd);
    if (!pResultSet || !pResultSet->Next())
    {
        LOG_ERROR << "folder not found or not belong to user: " << folder_id;
        ret = -2; // 文件夹不存在或不属于该用户
        goto END;
    }
    int old_parent_id = pResultSet->GetInt("parent_id");
    delete pResultSet;

    // 如果目标父文件夹与当前相同，则无需移动
    if (old_parent_id == new_parent_id)
    {
        LOG_INFO << "folder already in target parent: " << folder_id;
        ret = -3; // 文件夹已在目标父文件夹中
        goto END;
    }

    // 检查目标父文件夹是否存在且属于该用户
    if (new_parent_id > 0)
    {
        sprintf(sql_cmd, "SELECT id FROM folders WHERE id=%d AND user_id=%d AND is_deleted=0", new_parent_id, user_id);
        pResultSet = pDBConn->ExecuteQuery(sql_cmd);
        if (!pResultSet || !pResultSet->Next())
        {
            LOG_ERROR << "target parent folder not found or not belong to user: " << new_parent_id;
            ret = -4; // 目标父文件夹不存在或不属于该用户
            goto END;
        }
        delete pResultSet;
    }

    // 检查移动是否会造成循环引用（将文件夹移动到自己的子文件夹中）
    if (new_parent_id > 0)
    {
        int check_id = new_parent_id;
        while (check_id > 0)
        {
            sprintf(sql_cmd, "SELECT parent_id FROM folders WHERE id=%d AND is_deleted=0", check_id);
            pResultSet = pDBConn->ExecuteQuery(sql_cmd);
            if (!pResultSet || !pResultSet->Next())
            {
                break;
            }
            int check_parent_id = pResultSet->GetInt("parent_id");
            delete pResultSet;

            if (check_parent_id == folder_id)
            {
                LOG_ERROR << "cannot move folder to its own subfolder: " << folder_id << " -> " << new_parent_id;
                ret = -5; // 不能将文件夹移动到自己的子文件夹中
                goto END;
            }

            check_id = check_parent_id;
        }
    }

    // 移动文件夹
    sprintf(sql_cmd, "UPDATE folders SET parent_id=%d WHERE id=%d", new_parent_id, folder_id);
    if (!pDBConn->ExecuteUpdate(sql_cmd))
    {
        LOG_ERROR << "move folder failed: " << sql_cmd;
        ret = -1;
        goto END;
    }

    LOG_INFO << "move folder success: " << folder_id << " from " << old_parent_id << " to " << new_parent_id;

    // 返回成功响应
    Json::Value root;
    root["code"] = 0;
    Json::FastWriter writer;
    str_json = writer.write(root);

END:
    return ret;
}

// 获取文件夹中的文件列表
int getFolderFiles(const std::string &user, int folder_id, std::string &str_json)
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

    // 检查文件夹是否存在且属于该用户
    sprintf(sql_cmd, "SELECT id FROM folders WHERE id=%d AND user_id=%d AND is_deleted=0", folder_id, user_id);
    pResultSet = pDBConn->ExecuteQuery(sql_cmd);
    if (!pResultSet || !pResultSet->Next())
    {
        LOG_ERROR << "folder not found or not belong to user: " << folder_id;
        ret = -2; // 文件夹不存在或不属于该用户
        goto END;
    }
    delete pResultSet;

    // 获取文件夹中的文件列表
    sprintf(sql_cmd, "SELECT fl.id, fl.file_name, fl.create_time, fl.pv, fi.size, fi.type "
                     "FROM user_file_list fl "
                     "LEFT JOIN file_info fi ON fl.md5 = fi.md5 "
                     "WHERE fl.user='%s' AND fl.folder_id=%d ORDER BY fl.create_time DESC", user.c_str(), folder_id);
    pResultSet = pDBConn->ExecuteQuery(sql_cmd);
    if (!pResultSet)
    {
        LOG_ERROR << "query folder files failed";
        ret = -1;
        goto END;
    }

    Json::Value root;
    Json::Value file_array(Json::arrayValue);
    while (pResultSet->Next())
    {
        Json::Value file;
        file["id"] = pResultSet->GetInt("id");
        file["file_name"] = pResultSet->GetString("file_name");
        file["create_time"] = pResultSet->GetString("create_time");
        file["pv"] = pResultSet->GetInt("pv");
        file["size"] = pResultSet->GetString("size");
        file["type"] = pResultSet->GetString("type");
        file_array.append(file);
    }
    root["code"] = 0;
    root["files"] = file_array;
    delete pResultSet;

    Json::FastWriter writer;
    str_json = writer.write(root);

END:
    return ret;
}

// 初始化文件夹管理API
int ApiFolderInit()
{
    // 可以在这里进行一些初始化操作
    return 0;
}
