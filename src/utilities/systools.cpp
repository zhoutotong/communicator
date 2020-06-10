#include "systools.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <unistd.h>

namespace awe
{
namespace systools
{

// 通过 pid 获取进程名称
int getNameByPid(const pid_t &pid, awe::AString &name)
{
    std::stringstream ss;
    ss << "/proc/" << (pid) << "/status";

    // 如果文件不存在说明线程已经结束，返回错误
    if(0 != access(ss.str().c_str(), F_OK))
    {
        return -1;
    }

    // 打开状态文件
    std::ifstream file;
    
    file.open(ss.str());

    if(!file.is_open())
    {
        // 文件打开失败，则判断失败
        return -1;
    }

    // 遍历所有行找到名称行，并读取出来
    const size_t len = 512;
    char line[len];
    while( true )
    {
        file.getline(line, len);
        if(file.rdstate() == std::ifstream::eofbit) break;
        std::string nameLine(line);

        // 移除不必要的空格
        nameLine.erase(std::remove_if(nameLine.begin(), nameLine.end(), ::isspace), nameLine.end());

        // 查找 Name 字段
        const std::string nameLabel = "Name:";
        int cutLen = nameLine.find_first_of(nameLabel, 0);
        if(cutLen != -1)
        {
            name = nameLine.substr(cutLen + nameLabel.size(), nameLine.size());
            return 0;
        }   
    }
    return -1;
}


} // namespace systools    
} // namespace awe