#include <chrono>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "BackupSystem.h"
#include "TextFile.h"

// Driver code: exercises the full flow with more than one file.
int main() {
    std::cout << "-------- This is a Backup System for Files --------\n\n";

    BackupSystem backupSystem;

    // --- Register live files ------------------------------------------------
    File* notes =
        backupSystem.addFile(std::make_unique<TextFile>("notes.txt", "text", "notes: v1"));
    File* todo =
        backupSystem.addFile(std::make_unique<TextFile>("todo.txt", "text", "todo: v1"));

    // Duplicate file name: addFile keeps the existing file and returns it.
    File* dup =
        backupSystem.addFile(std::make_unique<TextFile>("notes.txt", "text", "notes: impostor"));
    std::cout << "duplicate addFile(\"notes.txt\") returned the existing file: "
              << std::boolalpha << (dup == notes) << "\n\n";

    // --- Back up notes.txt across two edits -------------------------------
    std::time_t notesV1At = std::time(nullptr);
    backupSystem.createBackup("notes.txt");

    std::this_thread::sleep_for(std::chrono::seconds(1));  // demo only: keep timestamps distinct
    notes->updateContent("notes: v2");
    backupSystem.createBackup("notes.txt");

    // --- Back up todo.txt -------------------------------------------------
    todo->updateContent("todo: v2");
    backupSystem.createBackup("todo.txt");

    // --- Inspect histories ----------------------------------------------
    std::cout << "\n";
    backupSystem.listBackups("notes.txt");
    backupSystem.listBackups("todo.txt");
    backupSystem.listBackups("missing.txt");

    // --- Restore notes.txt to its first snapshot ------------------------
    std::cout << "\nnotes.txt before restore: " << notes->getContent() << "\n";
    backupSystem.restore("notes.txt", notesV1At);
    std::cout << "notes.txt after restore:  " << notes->getContent() << "\n";

    // Restore against a timestamp with no snapshot.
    backupSystem.restore("notes.txt", notesV1At - 100);

    return 0;
}
