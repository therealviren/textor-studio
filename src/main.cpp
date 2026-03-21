#include "editor.hpp"
#include "version.hpp"
#include <iostream>
#include <string>

int main(int argc, char **argv) {
    if (argc == 1) {
        std::cout << ts::NAME << " by " << ts::AUTHOR << " (" << ts::VERSION << ")\n\n";
        std::cout << "Usage:\n";
        std::cout << "tse [file_path]    open or create file\n";
        std::cout << "tse help           show help\n";
        std::cout << "tse credits        show credits\n";
        std::cout << "tse version        show version\n";
        return 0;
    }
    std::string arg = argv[1];
    if (arg == "--help" || arg == "help") {
        std::cout << "Textor Studio (tse) help\n";
        std::cout << "Controls inside editor:\n";
        std::cout << "^O Write Out   ^F Where Is   ^K Cut\n";
        std::cout << "^X Exit        ^R Read File  ^\\ Replace   ^U Paste\n";
        std::cout << "^Z Undo        ^Y Redo\n";
        return 0;
    }
    if (arg == "credits") {
        std::cout << "created by " << ts::AUTHOR << "\n";
        return 0;
    }
    if (arg == "version") {
        std::cout << ts::NAME << " version " << ts::VERSION << "\n";
        return 0;
    }
    std::string path = arg;
    Editor editor;
    editor.open(path);
    editor.run();
    return 0;
}
