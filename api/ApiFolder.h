/**
 * ApiFolder.h
 * 文件夹管理API接口
 */

#ifndef _API_FOLDER_H_
#define _API_FOLDER_H_

#include <string>
#include <json/json.h>

// 获取用户文件夹列表
int getUserFolders(const std::string &user, std::string &str_json);

// 创建文件夹
int createFolder(const std::string &user, const std::string &folder_name, int parent_id, std::string &str_json);

// 更新文件夹名称
int updateFolderName(const std::string &user, int folder_id, const std::string &new_name, std::string &str_json);

// 删除文件夹
int deleteFolder(const std::string &user, int folder_id, std::string &str_json);

// 移动文件夹
int moveFolder(const std::string &user, int folder_id, int new_parent_id, std::string &str_json);

// 获取文件夹中的文件列表
int getFolderFiles(const std::string &user, int folder_id, std::string &str_json);

// 初始化文件夹管理API
int ApiFolderInit();

#endif // _API_FOLDER_H_
