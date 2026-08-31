// Unit tests for the backup system.
//
// Build:
//   g++ -std=c++14 -Wall -Wextra -o run-tests \
//       test_backup_system.cpp BackupSystem.cpp TextFile.cpp Backup.cpp
//   ./run-tests
//
// No test framework -- a handful of macros and a cout-capture helper are all
// that is needed. main() runs every test and exits non-zero if any check fails.

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "BackupSystem.h"
#include "TextFile.h"

// --------------------------------------------------------------------------
// Tiny test harness
// --------------------------------------------------------------------------
static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        ++g_checks;                                                       \
        if (!(cond)) {                                                    \
            ++g_failures;                                                 \
            std::cout << "    [FAIL] line " << __LINE__                    \
                      << ": CHECK(" #cond ")\n";                          \
        }                                                                 \
    } while (0)

#define CHECK_EQ(actual, expected)                                        \
    do {                                                                  \
        ++g_checks;                                                       \
        auto _a = (actual);                                               \
        auto _e = (expected);                                             \
        if (!(_a == _e)) {                                                \
            ++g_failures;                                                 \
            std::cout << "    [FAIL] line " << __LINE__                    \
                      << ": CHECK_EQ(" #actual ", " #expected ")"         \
                      << "  got [" << _a << "] want [" << _e << "]\n";     \
        }                                                                 \
    } while (0)

#define RUN(fn)                                                           \
    do {                                                                  \
        std::cout << "- " #fn "\n";                                       \
        fn();                                                             \
    } while (0)

// Redirects std::cout into a buffer for the lifetime of the object, so a test
// can assert on what a void, print-only method wrote. Restores cout on destruction.
class CoutCapture {
public:
    CoutCapture() : previous_(std::cout.rdbuf(buffer_.rdbuf())) {}
    ~CoutCapture() { std::cout.rdbuf(previous_); }
    std::string str() const { return buffer_.str(); }

private:
    std::ostringstream buffer_;
    std::streambuf* previous_;
};

static bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

static std::unique_ptr<TextFile> makeTextFile(const std::string& name,
                                              const std::string& content) {
    return std::unique_ptr<TextFile>(new TextFile(name, "text", content));
}

// --------------------------------------------------------------------------
// Core behaviour
// --------------------------------------------------------------------------

// Creating several backups and listing them: every timestamp shows up, and the
// list is printed in ascending time order even when inserted out of order.
void test_list_multiple_backups_in_order() {
    BackupSystem sys;
    TextFile* doc = static_cast<TextFile*>(sys.addFile(makeTextFile("doc.txt", "v1")));

    sys.createBackup("doc.txt", 1000);
    doc->updateContent("v2");
    sys.createBackup("doc.txt", 3000);
    doc->updateContent("v3");
    sys.createBackup("doc.txt", 2000);  // out of order on purpose

    std::string out;
    {
        CoutCapture cap;
        sys.listBackups("doc.txt");
        out = cap.str();
    }

    CHECK(contains(out, "doc.txt"));
    CHECK(contains(out, "1000"));
    CHECK(contains(out, "2000"));
    CHECK(contains(out, "3000"));

    const size_t p1000 = out.find("1000");
    const size_t p2000 = out.find("2000");
    const size_t p3000 = out.find("3000");
    CHECK(p1000 < p2000);
    CHECK(p2000 < p3000);
}

// Restoring to a valid timestamp rewrites the live file's content, and the two
// backups genuinely captured different content (round-trip proves it).
void test_restore_valid_timestamp_and_content_differs_between_backups() {
    BackupSystem sys;
    TextFile* doc = static_cast<TextFile*>(sys.addFile(makeTextFile("doc.txt", "first")));

    sys.createBackup("doc.txt", 100);
    doc->updateContent("second");
    sys.createBackup("doc.txt", 200);

    CHECK_EQ(doc->getContent(), std::string("second"));

    sys.restore("doc.txt", 100);
    CHECK_EQ(doc->getContent(), std::string("first"));

    sys.restore("doc.txt", 200);
    CHECK_EQ(doc->getContent(), std::string("second"));

    // The snapshot at 100 and the snapshot at 200 hold different content.
    CHECK(std::string("first") != std::string("second"));
}

// Restoring to a timestamp with no snapshot is a no-op and reports the miss.
void test_restore_unknown_timestamp_is_noop() {
    BackupSystem sys;
    TextFile* doc = static_cast<TextFile*>(sys.addFile(makeTextFile("doc.txt", "orig")));

    sys.createBackup("doc.txt", 100);
    doc->updateContent("changed");

    std::string out;
    {
        CoutCapture cap;
        sys.restore("doc.txt", 999);
        out = cap.str();
    }

    CHECK(contains(out, "no backup at"));
    CHECK_EQ(doc->getContent(), std::string("changed"));  // untouched
}

// A second backup at the same timestamp updates the existing snapshot rather
// than adding a new entry (create-or-update semantics).
void test_same_timestamp_updates_snapshot() {
    BackupSystem sys;
    TextFile* doc = static_cast<TextFile*>(sys.addFile(makeTextFile("doc.txt", "A")));

    sys.createBackup("doc.txt", 500);
    doc->updateContent("B");
    sys.createBackup("doc.txt", 500);  // same timestamp -> overwrite

    std::string out;
    {
        CoutCapture cap;
        sys.listBackups("doc.txt");
        out = cap.str();
    }
    const size_t firstHit = out.find("500");
    CHECK(firstHit != std::string::npos);
    CHECK(out.find("500", firstHit + 1) == std::string::npos);  // only one entry

    doc->updateContent("C");
    sys.restore("doc.txt", 500);
    CHECK_EQ(doc->getContent(), std::string("B"));  // the updated snapshot
}

// --------------------------------------------------------------------------
// Edge cases
// --------------------------------------------------------------------------

// Empty file name: no file is ever registered under "", so every op declines.
void test_empty_file_name() {
    BackupSystem sys;
    std::string out;

    {
        CoutCapture cap;
        sys.createBackup("");
        out = cap.str();
    }
    CHECK(contains(out, "no file named \"\""));

    {
        CoutCapture cap;
        sys.restore("", 100);
        out = cap.str();
    }
    CHECK(contains(out, "no file named \"\""));

    {
        CoutCapture cap;
        sys.listBackups("");
        out = cap.str();
    }
    CHECK(contains(out, "(none)"));
}

// Operations naming a file that was never added.
void test_unknown_file() {
    BackupSystem sys;
    std::string out;

    {
        CoutCapture cap;
        sys.createBackup("ghost.txt");
        out = cap.str();
    }
    CHECK(contains(out, "no file named \"ghost.txt\""));

    {
        CoutCapture cap;
        sys.restore("ghost.txt", 1);
        out = cap.str();
    }
    CHECK(contains(out, "no file named \"ghost.txt\""));

    {
        CoutCapture cap;
        sys.listBackups("ghost.txt");
        out = cap.str();
    }
    CHECK(contains(out, "(none)"));
}

// File exists but has no backups yet: restore declines, content is untouched.
void test_restore_when_no_backups() {
    BackupSystem sys;
    TextFile* doc = static_cast<TextFile*>(sys.addFile(makeTextFile("doc.txt", "orig")));

    std::string out;
    {
        CoutCapture cap;
        sys.restore("doc.txt", 100);
        out = cap.str();
    }
    CHECK(contains(out, "has no backups"));
    CHECK_EQ(doc->getContent(), std::string("orig"));
}

// listBackups on a registered file that has not been backed up prints "(none)".
void test_list_when_no_backups_for_known_file() {
    BackupSystem sys;
    sys.addFile(makeTextFile("doc.txt", "x"));

    std::string out;
    {
        CoutCapture cap;
        sys.listBackups("doc.txt");
        out = cap.str();
    }
    CHECK(contains(out, "doc.txt"));
    CHECK(contains(out, "(none)"));
}

// Duplicate file name: the first file stays, the impostor is rejected, and
// addFile returns the file that is actually registered.
void test_duplicate_file_name() {
    BackupSystem sys;
    File* first = sys.addFile(makeTextFile("dup.txt", "one"));

    File* second = nullptr;
    std::string out;
    {
        CoutCapture cap;
        second = sys.addFile(makeTextFile("dup.txt", "two"));
        out = cap.str();
    }

    CHECK(contains(out, "already exists"));
    CHECK(first == second);
    CHECK_EQ(static_cast<TextFile*>(first)->getContent(), std::string("one"));
}

// addFile(nullptr) is rejected and returns nullptr.
void test_add_null_file() {
    BackupSystem sys;
    CHECK(sys.addFile(nullptr) == nullptr);
}

// Backups are tracked per file: one file's history never leaks into another's.
void test_backups_are_isolated_per_file() {
    BackupSystem sys;
    sys.addFile(makeTextFile("a.txt", "a1"));
    sys.addFile(makeTextFile("b.txt", "b1"));

    sys.createBackup("a.txt", 10);
    sys.createBackup("a.txt", 20);
    sys.createBackup("b.txt", 30);

    std::string outA;
    {
        CoutCapture cap;
        sys.listBackups("a.txt");
        outA = cap.str();
    }
    CHECK(contains(outA, "10"));
    CHECK(contains(outA, "20"));
    CHECK(!contains(outA, "30"));

    std::string outB;
    {
        CoutCapture cap;
        sys.listBackups("b.txt");
        outB = cap.str();
    }
    CHECK(contains(outB, "30"));
    CHECK(!contains(outB, "10"));
}

// --------------------------------------------------------------------------
int main() {
    std::cout << "Running BackupSystem unit tests...\n\n";

    RUN(test_list_multiple_backups_in_order);
    RUN(test_restore_valid_timestamp_and_content_differs_between_backups);
    RUN(test_restore_unknown_timestamp_is_noop);
    RUN(test_same_timestamp_updates_snapshot);
    RUN(test_empty_file_name);
    RUN(test_unknown_file);
    RUN(test_restore_when_no_backups);
    RUN(test_list_when_no_backups_for_known_file);
    RUN(test_duplicate_file_name);
    RUN(test_add_null_file);
    RUN(test_backups_are_isolated_per_file);

    std::cout << "\n"
              << (g_checks - g_failures) << "/" << g_checks << " checks passed.\n";
    if (g_failures > 0) {
        std::cout << g_failures << " CHECK(S) FAILED\n";
        return 1;
    }
    std::cout << "ALL TESTS PASSED\n";
    return 0;
}
