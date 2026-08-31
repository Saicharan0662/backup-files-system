#pragma once

#include <ctime>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include "Backup.h"
#include "File.h"

// Manager class: owns the live files and their timestamped backups.
class BackupSystem {
private:
    std::unordered_map<std::string, std::unique_ptr<File>> files;
    std::unordered_map<std::string, std::map<std::time_t, Backup>> backups;

public:
    // Register a live file with the system and return an observer pointer to it,
    // so the caller can keep editing it via updateContent() before each backup.
    File* addFile(std::unique_ptr<File> file);

    // Prints the file name and every backup timestamp it has.
    void listBackups(const std::string& fileName) const;
    void restore(const std::string& fileName, std::time_t timestamp);
    void createBackup(const std::string& fileName);
};
