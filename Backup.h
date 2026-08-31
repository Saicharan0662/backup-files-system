#pragma once

#include <string>

// Immutable snapshot of a file's content at a point in time.
class Backup {
private:
    std::string content;

public:
    explicit Backup(const std::string& content);

    const std::string& getContent() const;
};
