/**
 * ApiFolderHandler.h
 * 文件夹管理API处理
 */

#ifndef _API_FOLDER_HANDLER_H_
#define _API_FOLDER_HANDLER_H_

#include <string>

// 处理获取用户文件夹列表请求
int handleGetUserFolders(const std::string &url, const std::string &post_data, std::string &str_json);

// 处理创建文件夹请求
int handleCreateFolder(const std::string &url, const std::string &post_data, std::string &str_json);

// 处理更新文件夹名称请求
int handleUpdateFolderName(const std::string &url, const std::string &post_data, std::string &str_json);

// 处理删除文件夹请求
int handleDeleteFolder(const std::string &url, const std::string &post_data, std::string &str_json);

// 处理移动文件夹请求
int handleMoveFolder(const std::string &url, const std::string &post_data, std::string &str_json);

// 处理获取文件夹中的文件列表请求
int handleGetFolderFiles(const std::string &url, const std::string &post_data, std::string &str_json);

#endif // _API_FOLDER_HANDLER_H_
