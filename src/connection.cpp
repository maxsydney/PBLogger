#include "connection.h"

void ConnectionMetadata::on_open(Client * c, websocketpp::connection_hdl hdl)
{
    m_status = "Open";
    
    Client::connection_ptr con = c->get_con_from_hdl(hdl);
    m_server = con->get_response_header("Server");
}

void ConnectionMetadata::on_fail(Client* c, websocketpp::connection_hdl hdl)
{
    m_status = "Failed";
    
    Client::connection_ptr con = c->get_con_from_hdl(hdl);
    m_server = con->get_response_header("Server");
    m_error_reason = con->get_ec().message();
}

void ConnectionMetadata::on_close(Client* c, websocketpp::connection_hdl hdl)
{
    m_status = "Closed";
    Client::connection_ptr con = c->get_con_from_hdl(hdl);
    std::stringstream s;
    s << "close code: " << con->get_remote_close_code() << " (" 
      << websocketpp::close::status::get_string(con->get_remote_close_code()) 
      << "), close reason: " << con->get_remote_close_reason();
    m_error_reason = s.str();
}

void ConnectionMetadata::on_message(websocketpp::connection_hdl hdl, Client::message_ptr msg) 
{
    if (_connected == false)
    {
        // Record connection once we have received some data
        _connected = true;
    }

    if (msg->get_opcode() == websocketpp::frame::opcode::text) {
        m_messages.push_back(msg->get_payload());
    } else {
        m_messages.push_back(websocketpp::utility::to_hex(msg->get_payload()));
    }
}

std::vector<std::string> ConnectionMetadata::readMessageQueue(void)
{
    printf("Writing out %zu messages\n", m_messages.size());
    std::vector<std::string> outMessages(m_messages);
    m_messages.clear();
    return outMessages;
}

std::ostream & operator<< (std::ostream & out, ConnectionMetadata const & data) 
{
    out << "> URI: " << data.m_uri << "\n"
        << "> Status: " << data.m_status << "\n"
        << "> Remote Server: " << (data.m_server.empty() ? "None Specified" : data.m_server) << "\n"
        << "> Error/close reason: " << (data.m_error_reason.empty() ? "N/A" : data.m_error_reason);

    out << "> Messages Processed: (" << data.m_messages.size() << ") \n";
 
    // std::vector<std::string>::const_iterator it;
    // for (it = data.m_messages.begin(); it != data.m_messages.end(); ++it) {
    //     out << *it << "\n";
    // }
 
    return out;
}

WebsocketEndpoint::WebsocketEndpoint(void) 
    : m_next_id(0) 
{
    m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
    m_endpoint.clear_error_channels(websocketpp::log::elevel::all);

    m_endpoint.init_asio();
    m_endpoint.start_perpetual();

    m_thread.reset(new websocketpp::lib::thread(&Client::run, &m_endpoint));
}

WebsocketEndpoint::~WebsocketEndpoint(void) 
{
    m_endpoint.stop_perpetual();
    
    for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
        if (it->second->get_status() != "Open") {
            // Only close open connections
            continue;
        }
        
        std::cout << "> Closing connection " << it->second->get_id() << std::endl;
        
        websocketpp::lib::error_code ec;
        m_endpoint.close(it->second->get_hdl(), websocketpp::close::status::going_away, "", ec);
        if (ec) {
            std::cout << "> Error closing connection " << it->second->get_id() << ": "  
                      << ec.message() << std::endl;
        }
    }
    printf("Closing endpoint\n");
    m_thread->join();
}

int WebsocketEndpoint::connect(std::string const& uri) 
{
    websocketpp::lib::error_code ec;

    Client::connection_ptr con = m_endpoint.get_connection(uri, ec);

    if (ec) {
        std::cout << "> Connect initialization error: " << ec.message() << std::endl;
        return -1;
    }

    int new_id = m_next_id++;
    ConnectionMetadata::ptr metadata_ptr(new ConnectionMetadata(new_id, con->get_handle(), uri));
    m_connection_list[new_id] = metadata_ptr;

    con->set_open_handler(websocketpp::lib::bind(
        &ConnectionMetadata::on_open,
        metadata_ptr,
        &m_endpoint,
        websocketpp::lib::placeholders::_1
    ));
    con->set_fail_handler(websocketpp::lib::bind(
        &ConnectionMetadata::on_fail,
        metadata_ptr,
        &m_endpoint,
        websocketpp::lib::placeholders::_1
    ));
    con->set_close_handler(websocketpp::lib::bind(
        &ConnectionMetadata::on_close,
        metadata_ptr,
        &m_endpoint,
        websocketpp::lib::placeholders::_1
    ));
    con->set_message_handler(websocketpp::lib::bind(
        &ConnectionMetadata::on_message,
        metadata_ptr,
        websocketpp::lib::placeholders::_1,
        websocketpp::lib::placeholders::_2
    ));

    m_endpoint.connect(con);

    return new_id;
}

void WebsocketEndpoint::send(int id, std::string message) 
{
    websocketpp::lib::error_code ec;
    
    con_list::iterator metadata_it = m_connection_list.find(id);
    if (metadata_it == m_connection_list.end()) {
        std::cout << "> No connection found with id " << id << std::endl;
        return;
    }
    
    m_endpoint.send(metadata_it->second->get_hdl(), message, websocketpp::frame::opcode::text, ec);
    if (ec) {
        std::cout << "> Error sending message: " << ec.message() << std::endl;
        return;
    }
    
    metadata_it->second->record_sent_message(message);
}

void WebsocketEndpoint::close(int id, websocketpp::close::status::value code) 
{
    websocketpp::lib::error_code ec;
    
    con_list::iterator metadata_it = m_connection_list.find(id);
    if (metadata_it == m_connection_list.end()) {
        std::cout << "> No connection found with id " << id << std::endl;
        return;
    }
    
    m_endpoint.close(metadata_it->second->get_hdl(), code, "", ec);
    if (ec) {
        std::cout << "> Error initiating close: " << ec.message() << std::endl;
    }
}

ConnectionMetadata::ptr WebsocketEndpoint::get_metadata(int id) const 
{
    con_list::const_iterator metadata_it = m_connection_list.find(id);
    if (metadata_it == m_connection_list.end()) {
        return ConnectionMetadata::ptr();
    } else {
        return metadata_it->second;
    }
}