#ifndef SRC_LOGGER_H
#define SRC_LOGGER_H

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "connection.h"

enum class LogType { None, Default };
enum class LogRet { Success, Failure };

class WsLogger
{
    public:
        // TODO: Can't use dependency injection here because WebsocketEndpoint is not copyable.
        WsLogger(void) = default;
        WsLogger(const std::string& logPath, const std::string& url, LogType logType=LogType::Default);
        ~WsLogger(void);     // TODO: Write logs to file when logger goes out of scope

        void start(void);

    private:

        LogRet _setupDefaultLogging(void);
        LogRet _connectToServer(const std::string& url);

        void _logData(const nlohmann::json& data);
        void _logCmd(const nlohmann::json& cmd);
        void _logMsg(const nlohmann::json& msg);

        double _getDoubleSafe(const nlohmann::json& data, const std::string& key);

        // Data, log messages and command data structures for logging
        const std::string _logPath;
        WebsocketEndpoint _ws;
        int _conID = 0;

        bool _run = false;
        int _nRows = 0;
        bool _configured = false;

        // Map containing time series data
        std::unordered_map<std::string, std::vector<double>> _data;

        LogType _logType = LogType::None;
};

#endif // SRC_LOGGER_H