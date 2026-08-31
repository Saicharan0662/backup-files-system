#include <ctime>
#include <iostream>
#include <memory>
#include <string>

#include "BackupSystem.h"
#include "TextFile.h"

// Driver code: shows the intended flow of the application.
// Method bodies are not implemented yet, so this will not link until they are.
int main() {
    std::cout << "-------- This is a Backup System for Files --------\n\n";

    BackupSystem backupSystem;

    // 1. Register a live file. Keep the observer pointer so we can edit it.
    File* notes =
        backupSystem.addFile(std::make_unique<TextFile>("notes.txt", "text", "first draft"));

    // 2. Take the first backup -- snapshots the current content into BackupSystem.
    std::time_t firstBackupAt = std::time(nullptr);
    backupSystem.createBackup("notes.txt");
    std::cout << "Created backup #1 for notes.txt\n";

    // 3. Edit the file through the File itself, then back it up again.
    //    createBackup reads the file's current content and records it.
    notes->updateContent("second draft");
    backupSystem.createBackup("notes.txt");
    std::cout << "Created backup #2 for notes.txt\n\n";

    // 4. List every backup the file has.
    std::cout << "Listing backups:\n";
    backupSystem.listBackups("notes.txt");
    std::cout << "\n";

    // 5. Restore the file to its first backup.
    std::cout << "Restoring notes.txt to backup #1...\n";
    backupSystem.restore("notes.txt", firstBackupAt);
    std::cout << "notes.txt content after restore: " << notes->getContent() << "\n";

    return 0;
}
