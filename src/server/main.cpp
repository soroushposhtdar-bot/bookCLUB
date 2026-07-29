// src/server/main.cpp
// Headless entry point for BookClubServer (no dashboard UI).
#include "src/server/ServerApplication.h"

int main(int argc, char *argv[])
{
    bookclub::server::ServerApplication app;
    return app.run(argc, argv);
}
