#include <iostream>
#include <string>
#include <stdio.h>
#include "args/args.hxx"
#include "connection.h"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

bool run;

void my_handler(int s){
    run = false;
}

int main(int argc, char* argv[])
{
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);


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

    // Hack
    WebsocketEndpoint endpoint;
    printf("Attempting to connect to: %s\n", args::get(IP).c_str());
    int id = endpoint.connect(args::get(IP));
    if (id != -1) {
        std::cout << "> Created connection with id " << id << std::endl;
    }

    run = true;
    while (run)
    {

    }


    ConnectionMetadata::ptr metadata = endpoint.get_metadata(0);
    if (metadata) {
        std::cout << *metadata << std::endl;
    } else {
        std::cout << "> Unknown connection id " << id << std::endl;
    }

    // Establish a connection

    // Initialise logger with connection

    // Log until done

    return 0;
}