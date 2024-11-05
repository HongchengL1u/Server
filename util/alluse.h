#ifndef SERVER_UTIL_ALLUSE_H
#define SERVER_UTIL_ALLUSE_H
#include <glog/logging.h>

#define LOG_INFO LOG(INFO)
#define LOG_ERROR LOG(ERROR)
#define LOG_FATAL LOG(FATAL)


void glog_init(const char* argv_0)
{
    google::InitGoogleLogging(argv_0);  
    google::SetLogDestination(google::GLOG_FATAL, "./log/log_fatal_"); // 设置 google::FATAL 级别的日志存储路径和文件名前缀
    google::SetLogDestination(google::GLOG_ERROR, "./log/log_error_"); //设置 google::ERROR 级别的日志存储路径和文件名前缀
    google::SetLogDestination(google::GLOG_WARNING, "./log/log_warning_"); //设置 google::WARNING 级别的日志存储路径和文件名前缀
    google::SetLogDestination(google::GLOG_INFO, "./log/log_info_"); //设置 google::INFO 级别的日志存储路径和文件名前缀
}
#endif