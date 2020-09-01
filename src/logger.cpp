#include <fstream>
#include <sstream>
#include <boost/filesystem.hpp>
#include "logger.h"

using json = nlohmann::json;
static bool run = false;

// This is not really ideal. Find a better method
void quitHandler(int s){
    printf("Receieved ctrl+c signal\n");
    run = false;
}

WsLogger::WsLogger(const std::string& logPath, const std::string& url, LogType logType)
    : _logPath(logPath), _logType(logType)
{
    if (_connectToServer(url) == LogRet::Failure)
    {   
        // Error printed in _connectToServer
        _configured = false;
        return;
    }

    // Check if directory exists
    boost::filesystem::path p(logPath);
    boost::filesystem::path dir = p.parent_path();
    if (boost::filesystem::exists(dir) == false)
    {
        printf("ERROR: Directory %s does not exist\n", dir.c_str());
        _configured = false;
        return;
    }

    // Check if file already exists
    if (std::ifstream(logPath))
    {
        // File already exists
        printf("ERROR: File %s already exists. Please choose a new file\n", logPath.c_str());
        _configured = false;
        return;
    }

    // Setup ctrl c handler
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = quitHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    // Setup logging
    if (logType == LogType::Default)
    {
        _setupDefaultLogging();
    }
    else
    {
        printf("ERROR: Invalid log type\n");
        _configured = false;
        return;
    }

    _configured = true;
}

WsLogger::~WsLogger(void)
{
    if (_configured && _writeLogs)
    {
        // Write logs to file
        printf("Wrote logs to %s\n", _logPath.c_str());

        std::ofstream f;
        f.open(_logPath, std::ios_base::app);

        for (size_t i = 0; i < _nRows; i++)
        {
            std::stringstream line;
            for (std::pair<std::string, std::vector<double>> val : _data)
            {
            f << val.second.at(i) << ", ";
            }
            f << std::endl;
        }

        f.close();
    }
}

void WsLogger::start(void)
{
    // Run the logger
    if (_configured == false)
    {
        printf("ERROR: Logger was not configured correctly. Quitting\n");
        return;
    }

    ConnectionMetadata::ptr metadata = _ws.get_metadata(_conID);
    run = true;
    _writeLogs = true;
    while (run)
    {
        if (metadata->isConnected() == false) 
        {
            double runTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - _connTime).count();
            if (runTime > timeout_s)
            {
                printf("Unable to receive data from server. Quitting\n");
                _writeLogs = false;
                run = false;
            }
        }
        if (metadata->getNumMessages() >= 10)
        {
            std::vector<std::string> msgs = metadata->readMessageQueue();

            for (const std::string msg : msgs)
            {
                // Parse JSON and push data to logs
                json data = json::parse(msg);
                std::string type = data["type"].get<std::string>();
                if (type == "data")
                {
                    // This is a data message, add to logs
                    _logData(data);
                }
                // TODO: Handle commands and messages
            }
        }
    }
}

double WsLogger::_getDoubleSafe(const json& data, const std::string& key)
{
    if (data.contains(key))
    {
        if (data[key].is_number())
        {
            return data[key].get<double>();
        }
    }

    return NAN;
}

void WsLogger::_logData(const json& data)
{
    // Store in data structure and save. This is the preferred method of logging
    for (auto& val : _data)
    {
        _data[val.first].push_back(_getDoubleSafe(data, val.first));

    }

    _nRows += 1;
}

LogRet WsLogger::_connectToServer(const std::string& url)
{
    // Attempt to connect to PB server
    _conID = _ws.connect(url);
    if (_conID == -1) 
    {
        printf("ERROR: Unable to connect to server at: %s\n", url.c_str());
        return LogRet::Failure;
    }

    printf("Got connection to server %s at: %s\n", _ws.get_metadata(_conID)->get_serverName().c_str(), url.c_str());
    _connTime = std::chrono::system_clock::now();
    return LogRet::Success;
}

LogRet WsLogger::_setupDefaultLogging(void)
{
    // Configure logger to log default parameters
    _data["T_head"] = std::vector<double>();
    _data["T_reflux"] = std::vector<double>();
    _data["T_prod"] = std::vector<double>();
    _data["T_radiator"] = std::vector<double>();
    // _data["T_reflux"] = std::vector<double>();       // TODO: Double up in server message here. Fix server side and replace this
    _data["setpoint"] = std::vector<double>();
    _data["P_gain"] = std::vector<double>();
    _data["I_gain"] = std::vector<double>();
    _data["D_gain"] = std::vector<double>();
    _data["LPFCutoff"] = std::vector<double>();
    _data["uptime"] = std::vector<double>();
    _data["flowrate"] = std::vector<double>();
    _data["boilerConc"] = std::vector<double>();
    _data["vapourConc"] = std::vector<double>();

    return LogRet::Success;
}