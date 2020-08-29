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

std::ostream & operator<< (std::ostream & out, ConnectionMetadata const & data) 
{
    out << "> URI: " << data.m_uri << "\n"
        << "> Status: " << data.m_status << "\n"
        << "> Remote Server: " << (data.m_server.empty() ? "None Specified" : data.m_server) << "\n"
        << "> Error/close reason: " << (data.m_error_reason.empty() ? "N/A" : data.m_error_reason);
 
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

    m_endpoint.connect(con);

    return new_id;
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