#pragma once

#include <string>

#include "File.h"

class TextFile : public File {
private:
    std::string name;
    std::string fileType;
    std::string content;

public:
    TextFile(const std::string& name, const std::string& fileType, const std::string& content);

    const std::string& getName() const override;
    const std::string& getFileType() const override;
    const std::string& getContent() const override;

    void updateContent(const std::string& newContent) override;
};
