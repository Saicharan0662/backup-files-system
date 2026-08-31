#include "TextFile.h"

TextFile::TextFile(const std::string& name, const std::string& fileType, const std::string& content)
    : name(name), fileType(fileType), content(content) {}

const std::string& TextFile::getName() const {
    return name;
}

const std::string& TextFile::getFileType() const {
    return fileType;
}

const std::string& TextFile::getContent() const {
    return content;
}

void TextFile::updateContent(const std::string& newContent) {
    content = newContent;
}
