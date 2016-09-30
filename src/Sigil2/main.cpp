#include "Sigil.hpp"
#include <iostream>

void prettyPrintSigil2()
{
    std::string title =
        "    ______    _           _  __   _____   \n"
        "  .' ____ \\  (_)         (_)[  | / ___ `. \n"
        "  | (___ \\_| __   .--./) __  | ||_/___) | \n"
        "   _.____`. [  | / /'`\\;[  | | | .'____.' \n"
        "  | \\____) | | | \\ \\._// | | | |/ /_____  \n"
        "   \\______.'[___].',__` [___|___]_______| \n"
        "                 ( ( __))                 \n"
        "                                          \n";
    std::cerr << title;
}

int main(int argc, char *argv[])
{
    prettyPrintSigil2();

    Sigil::instance().parseOptions(argc, argv);
    Sigil::instance().generateEvents();
}
