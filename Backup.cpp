#include "Backup.h"

Backup::Backup(const std::string& content) : content(content) {}

const std::string& Backup::getContent() const {
    return content;
}
