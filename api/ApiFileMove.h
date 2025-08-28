/**
 * ApiFileMove.h
 * 文件移动API接口
 */

#ifndef _API_FILE_MOVE_H_
#define _API_FILE_MOVE_H_

#include <string>
#include <json/json.h>

// 移动文件到指定文件夹
int moveFileToFolder(const std::string &user, const std::string &file_md5, int folder_id, std::string &str_json);

// 批量移动文件到指定文件夹
int batchMoveFilesToFolder(const std::string &user, const Json::Value &file_md5s, int folder_id, std::string &str_json);

// 初始化文件移动API
int ApiFileMoveInit();

#endif // _API_FILE_MOVE_H_
