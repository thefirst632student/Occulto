#include "Global.h"

static std::unordered_set<std::string> targetNames;

static inline void Trim(std::string& value)
{
    while (!value.empty() && (value.back() == '\r' || value.back() == '\n' || value.back() == ' ' || value.back() == '\t'))
        value.pop_back();

    size_t start = 0;
    while (start < value.size() && (value[start] == ' ' || value[start] == '\t'))
        ++start;

    if (start > 0)
        value.erase(0, start);

    // UTF-8 BOM, if present.
    if (value.size() >= 3
        && static_cast<unsigned char>(value[0]) == 0xEF
        && static_cast<unsigned char>(value[1]) == 0xBB
        && static_cast<unsigned char>(value[2]) == 0xBF)
        value.erase(0, 3);
}

bool Config::Read(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return false;

    targetNames.clear();

    std::string line;
    while (std::getline(file, line))
    {
        Trim(line);
        if (line.empty())
            continue;
        if (line.rfind("#", 0) == 0 || line.rfind("//", 0) == 0)
            continue;
        targetNames.insert(line);
    }

    file.close();
    printf("Config targets: %zu\n", targetNames.size());
    return true;
}

bool Config::ShouldProtect(const std::string& name)
{
    return targetNames.find(name) != targetNames.end();
}
