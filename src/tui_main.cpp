#include "TUI.h"

/// Interactive entry point: runs the TUI on std::cin / std::cout.
/// Build and run with:   make run_tui
int main() {
    TUI tui(std::cin, std::cout);
    return tui.run();
}
