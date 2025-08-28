/**
 * ApiFileMoveHandler.h
 * 文件移动API处理
 */

#ifndef _API_FILE_MOVE_HANDLER_H_
#define _API_FILE_MOVE_HANDLER_H_

#include <string>

// 处理移动文件到指定文件夹请求
int handleMoveFileToFolder(const std::string &url, const std::string &post_data, std::string &str_json);

// 处理批量移动文件到指定文件夹请求
int handleBatchMoveFilesToFolder(const std::string &url, const std::string &post_data, std::string &str_json);

#endif // _API_FILE_MOVE_HANDLER_H_
