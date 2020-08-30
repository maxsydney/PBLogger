#include <iostream>
#include <string>
#include <stdio.h>
#include "args/args.hxx"
#include "connection.h"
#include "logger.h"

int main(int argc, char* argv[])
{
    args::ArgumentParser parser("Pissbot data logger. Log distillation run over websockets");
    args::HelpFlag help(parser, "help", "Display the help menu", {'h', "help"});
    args::Positional<std::string> logPath(parser, "logPath", "Path to logger output file");
    args::Positional<std::string> IP(parser, "IP", "Pissbot server ip address");

    try
    {
        parser.ParseCLI(argc, argv);
    }
    catch (args::Help)
    {
        std::cout << parser;
        return 0;
    }
    catch (args::ParseError e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }
    
    WsLogger logger(logPath.Get(), IP.Get(), LogType::Default);
    logger.start();

    printf("Logging complete. Quitting\n");
    return 0;
}