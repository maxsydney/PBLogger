#ifndef SRC_CONNECTION_H
#define SRC_CONNECTION_H

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
 
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

typedef websocketpp::client<websocketpp::config::asio_client> Client;

class ConnectionMetadata {
    public:
        typedef std::shared_ptr<ConnectionMetadata> ptr;
    
        ConnectionMetadata(int id, websocketpp::connection_hdl hdl, std::string uri)
            : m_id(id), m_hdl(hdl), m_status("Connecting"), m_uri(uri) , m_server("N/A")
        {}
    
        void on_open(Client* c, websocketpp::connection_hdl hdl);
        void on_fail(Client* c, websocketpp::connection_hdl hdl);
        void on_close(Client* c, websocketpp::connection_hdl hdl);
        void on_message(websocketpp::connection_hdl hdl, Client::message_ptr msg);

        std::vector<std::string> readMessageQueue(void);

        friend std::ostream & operator<< (std::ostream & out, ConnectionMetadata const& data);

        void record_sent_message(std::string message) { m_messages.push_back(">> " + message); }

        bool isOpen(void) const { return m_status == "Open"; }
        size_t getNumMessages(void) const { return m_messages.size(); }
        websocketpp::connection_hdl get_hdl() const { return m_hdl; }
        int get_id() const { return m_id; }
        std::string get_status() const { return m_status; }

    private:
        int m_id;
        websocketpp::connection_hdl m_hdl;
        std::string m_status;
        std::string m_uri;
        std::string m_server;
        std::string m_error_reason;
        std::vector<std::string> m_messages;
};
 
class WebsocketEndpoint {
    public:
        WebsocketEndpoint(void);
        WebsocketEndpoint(WebsocketEndpoint& other) = default;
        ~WebsocketEndpoint(void); 
    
        int connect(std::string const & uri);
        void send(int id, std::string message);
        void close(int id, websocketpp::close::status::value code);
        ConnectionMetadata::ptr get_metadata(int id) const;

    private:
        typedef std::map<int,ConnectionMetadata::ptr> con_list;
    
        Client m_endpoint;
        std::shared_ptr<websocketpp::lib::thread> m_thread;
    
        con_list m_connection_list;
        int m_next_id;
};

class Connection
{
    public:
        Connection(void) = default;
};

#endif // SRC_CONNECTION_H