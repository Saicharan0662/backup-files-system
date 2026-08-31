#pragma once

#include <string>

// Abstract interface for any file that can be backed up.
class File {
public:
    virtual ~File() = default;

    virtual const std::string& getName() const = 0;
    virtual const std::string& getFileType() const = 0;
    virtual const std::string& getContent() const = 0;

    virtual void updateContent(const std::string& newContent) = 0;
};
