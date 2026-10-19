# Backup System for Files

A small, in-memory low-level design (LLD) of a file backup system, written in C++14.
You register files, take timestamped snapshots of their content, list a file's
snapshots, and restore a file to any earlier snapshot.

> **Authorship:** The design is entirely my own: the classes, their
> responsibilities, the relationships between them, the data structures, and the
> API. [Claude](https://claude.com/claude-code) only wrote the code that
> implements that design.

---

## Architecture

### Class diagram

```
            «interface»
       ┌────────────────────┐
       │        File        │
       ├────────────────────┤
       │ + getName()        │
       │ + getFileType()    │
       │ + getContent()     │
       │ + updateContent()  │
       └─────────▲──────────┘
                 │ implements
       ┌─────────┴──────────┐
       │      TextFile      │
       ├────────────────────┤
       │ - name             │
       │ - fileType         │
       │ - content          │
       └────────────────────┘

┌───────────────────────────────────────────────────────────────┐
│                         BackupSystem                          │
├───────────────────────────────────────────────────────────────┤
│ - files   : unordered_map<string, unique_ptr<File>>           │
│ - backups : unordered_map<string, map<time_t, Backup>>        │
├───────────────────────────────────────────────────────────────┤
│ + addFile(unique_ptr<File>) : File*                           │
│ + createBackup(fileName)                                      │
│ + createBackup(fileName, timestamp)                           │
│ + listBackups(fileName)                                       │
│ + restore(fileName, timestamp)                                │
└──────────────┬──────────────────────────────┬─────────────────┘
               │ owns 0..*                    │ owns 0..* per file
               ▼                              ▼
          ┌─────────┐                 ┌────────────────┐
          │  File   │                 │     Backup     │
          └─────────┘                 ├────────────────┤
                                      │ - content      │
                                      │ + getContent() │
                                      └────────────────┘
```

### Components

| Component | Files | Responsibility |
|---|---|---|
| `File` | `File.h` | Abstract interface for anything that can be backed up. It exposes the name, the file type and the content, plus `updateContent()` so the file can be edited or restored. |
| `TextFile` | `TextFile.h`, `TextFile.cpp` | A concrete `File` that stores its name, type and content as strings. |
| `Backup` | `Backup.h`, `Backup.cpp` | An immutable snapshot of a file's content at one point in time. |
| `BackupSystem` | `BackupSystem.h`, `BackupSystem.cpp` | The manager. It owns every live file and every file's backup history, and it handles create, list and restore. |
| Driver | `main.cpp` | Demo run of the full flow with two files. |
| Tests | `test_backup_system.cpp` | Unit tests that need no framework. |

### Data structures

```cpp
std::unordered_map<std::string, std::unique_ptr<File>>          files;
std::unordered_map<std::string, std::map<std::time_t, Backup>>  backups;
```

- **`files`** looks up a live file by name in O(1) on average. The file name is
  the unique key.
- **`backups`** maps each file name to its own history. The inner
  `std::map<time_t, Backup>` keeps snapshots sorted by timestamp, so
  `listBackups` prints them in order without extra sorting, and finding a
  snapshot by timestamp is O(log n).
- Histories are kept per file, so snapshots of one file never mix with
  snapshots of another.

### Ownership and lifetime

- `BackupSystem` takes ownership of every file through `std::unique_ptr<File>`.
- `addFile()` returns a raw **observer pointer** (`File*`). The caller can keep
  editing the file through `updateContent()` between backups, but it never owns
  or deletes the file.
- `Backup` objects are stored by value inside the history maps.
- The destructor clears both maps, which frees every file and backup. Nothing is
  saved to disk; the whole system lives in memory.

### Flow

```
addFile(TextFile)  ──►  files["notes.txt"]
        │
        ▼  caller edits via File*->updateContent()
createBackup("notes.txt")  ──►  backups["notes.txt"][now] = Backup(content)
        │
        ▼
listBackups("notes.txt")   ──►  prints the timestamps in sorted order
        │
        ▼
restore("notes.txt", t)    ──►  file->updateContent(backups["notes.txt"][t].content)
```

### Behaviour and edge cases

| Situation | Behaviour |
|---|---|
| `addFile` with a name that is already registered | The existing file and its history are kept, the new file is rejected, and a pointer to the existing file is returned. |
| `addFile(nullptr)` | Returns `nullptr`. |
| Two backups with the same timestamp | The snapshot at that timestamp is overwritten in place with the current content. |
| `createBackup` / `restore` on an unknown file | Prints a message and does nothing. |
| `restore` when the file has no backups, or none at that timestamp | Prints a message and leaves the file unchanged. |
| `listBackups` for a file with no history | Prints `(none)`. |

`createBackup(fileName, timestamp)` takes an explicit timestamp so that tests
are deterministic and historical snapshots can be imported. The one-argument
form uses `std::time(nullptr)`.

### Extensibility

To support a new kind of file (binary, image, and so on), add another class
that implements `File`. `BackupSystem` only depends on the `File` interface, so
it does not need to change.

---

## Build and run

Needs a C++14 compiler.

```sh
# Demo driver
g++ -std=c++14 -Wall -Wextra -o backup-system \
    main.cpp BackupSystem.cpp TextFile.cpp Backup.cpp
./backup-system

# Unit tests
g++ -std=c++14 -Wall -Wextra -o run-tests \
    test_backup_system.cpp BackupSystem.cpp TextFile.cpp Backup.cpp
./run-tests
```

The test binary exits with a non-zero code if any check fails.

## Project layout

```
.
├── File.h                  # abstract File interface
├── TextFile.h / .cpp       # concrete text file
├── Backup.h / .cpp         # immutable content snapshot
├── BackupSystem.h / .cpp   # manager: files + per-file backup histories
├── main.cpp                # demo driver
└── test_backup_system.cpp  # unit tests
```

## Credits

- **Design:** Sai Charan. All architecture and design decisions are my own.
- **Code:** Written by [Claude](https://claude.com/claude-code) from that design.
