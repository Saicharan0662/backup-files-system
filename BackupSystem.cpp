#include "BackupSystem.h"

#include <iostream>
#include <utility>

BackupSystem::~BackupSystem() {
    // Nothing here persists between runs -- the system holds everything in
    // memory. Clearing the maps runs every unique_ptr<File> destructor (which
    // deletes the File) and destroys every Backup, so the process exits clean.
    backups.clear();
    files.clear();
    std::cout << "BackupSystem: released all files and backups on exit.\n";
}

File* BackupSystem::addFile(std::unique_ptr<File> file) {
    if (!file) {
        return nullptr;
    }

    const std::string name = file->getName();

    // Handle duplicate file names: a name is a unique key in `files`, so if one
    // is already registered we keep it (and its backup history) and reject the
    // newcomer, returning the file that actually stays in the system.
    auto existing = files.find(name);
    if (existing != files.end()) {
        std::cout << "addFile: \"" << name
                  << "\" already exists; keeping the registered file.\n";
        return existing->second.get();
    }

    File* observer = file.get();
    files.emplace(name, std::move(file));
    return observer;
}

void BackupSystem::createBackup(const std::string& fileName) {
    createBackup(fileName, std::time(nullptr));
}

void BackupSystem::createBackup(const std::string& fileName, std::time_t timestamp) {
    auto fileIt = files.find(fileName);
    if (fileIt == files.end()) {
        std::cout << "createBackup: no file named \"" << fileName << "\".\n";
        return;
    }

    const std::string& content = fileIt->second->getContent();

    // `backups[fileName]` creates the per-file history entry if the key is new,
    // otherwise returns the existing inner map. Then: if there is no snapshot at
    // `timestamp` yet, insert one; if there already is, update its value in place.
    auto& history = backups[fileName];
    auto snapshot = history.find(timestamp);
    if (snapshot == history.end()) {
        history.insert(std::make_pair(timestamp, Backup(content)));
    } else {
        snapshot->second = Backup(content);
    }

    std::cout << "createBackup: snapshot of \"" << fileName << "\" taken at "
              << timestamp << ".\n";
}

void BackupSystem::listBackups(const std::string& fileName) const {
    std::cout << "Backups for \"" << fileName << "\":";

    auto histIt = backups.find(fileName);
    if (histIt == backups.end() || histIt->second.empty()) {
        std::cout << " (none)\n";
        return;
    }

    std::cout << "\n";
    for (const auto& snapshot : histIt->second) {
        std::cout << "  - " << snapshot.first << "\n";
    }
}

void BackupSystem::restore(const std::string& fileName, std::time_t timestamp) {
    auto fileIt = files.find(fileName);
    if (fileIt == files.end()) {
        std::cout << "restore: no file named \"" << fileName << "\".\n";
        return;
    }

    auto histIt = backups.find(fileName);
    if (histIt == backups.end()) {
        std::cout << "restore: \"" << fileName << "\" has no backups.\n";
        return;
    }

    auto snapshot = histIt->second.find(timestamp);
    if (snapshot == histIt->second.end()) {
        std::cout << "restore: \"" << fileName << "\" has no backup at "
                  << timestamp << ".\n";
        return;
    }

    fileIt->second->updateContent(snapshot->second.getContent());
    std::cout << "restore: \"" << fileName << "\" rolled back to the snapshot at "
              << timestamp << ".\n";
}
