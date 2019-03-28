#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>
#include <optional>
#include <functional>
#include <sstream>
#include <set>
#include <thread> 

std::string VERSION_NAME = "v0.1.0";

/*
 * Included the JSON for Modern C++
 * MIT: https://github.com/nlohmann/json
 */
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/*
 * Incldued websocketpp for all the websockets (RFC6455) 
 * BSD: https://github.com/zaphoyd/websocketpp 
 */
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "sole.hpp"

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

typedef std::set<websocketpp::connection_hdl,std::owner_less<websocketpp::connection_hdl>> connection_set;
typedef websocketpp::server<websocketpp::config::asio> server;
typedef server::message_ptr message_ptr;
connection_set connections;


class Session {
    std::string uuid;
    connection_set players;

  public:
    Session();
    std::string get_id() {
        return uuid;
    }
    void set_players(connection_set _players) {
        players = _players;
    }
};

Session::Session () {
    /* Creates the unique uuid for a given session */
    uuid = sole::uuid4().str();
}

/*
 * Helper functions for data transformations 
 */
std::optional<int> string_to_int(std::string rawValue, int base) {
    /* alloc size_type */
    std::string::size_type size;
    /* try/catch stoi */
    try {
        /* return optional with the correct value */
        return std::stoi(rawValue, &size, base);
    } catch (...) { }
    /* return optional with no value */
    return { };
}

std::optional<double> string_to_double(std::string rawValue) {
    /* try/catch stoi */
    try {
        /* return optional with the correct value */
        return std::atof(rawValue.c_str());
    } catch (...) { }
    /* return optional with no value */
    return { };
}

std::optional<std::vector<std::string>> string_to_vector(std::string rawValue) {
    /* creates a stringstream and result vector  */
    std::vector<std::string> result;
    std::stringstream stream(rawValue);
    /* while to iter over stream */
    while(stream.good()) {
        /* grabs the substr and add to vector */
        std::string substr;
        getline(stream, substr, ',');
        result.push_back(substr);
    }
    /* returns the vector wrapped in optional */
    return result;
}

void on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg) {
    std::cout << "HDL: " << hdl.lock().get() << " PAYLOAD: " << msg->get_payload() << std::endl;

    try {
        //s->send(hdl, msg->get_payload(), msg->get_opcode());
        json payload = json::parse(msg->get_payload());
        //std::cout << "payload: " << payload["session_id"] << std::endl;

        /*for (it = connections.begin(); it != connections.end(); ++it) {
            s->send(*it, msg->get_payload(), msg->get_opcode());
        }*/
    } catch (websocketpp::exception const & e) {
        std::cout << "ERROR: " << "(" << e.what() << ")" << std::endl;
    } catch (...) {
        std::cout << "You done fucked up somewhere, johnny boi" << std::endl;
    }
}

void on_open(server* s, websocketpp::connection_hdl hdl) {
    std::cout << "OPEN: " << hdl.lock().get() << std::endl;
    connections.insert(hdl);
}

void on_close(server* s, websocketpp::connection_hdl hdl) {
    std::cout << "CLOSE: " << hdl.lock().get() << std::endl;
    connections.erase(hdl);
}

void dispatcher_thread(server &server_ref) 
{ 
    while (true) { 
        std::cout << connections.size() << std::endl;
        connection_set::iterator it;

        for (it = connections.begin(); it != connections.end(); ++it) {
            server_ref.send(*it, sole::uuid4().str(), websocketpp::frame::opcode::text);
        }

        std::this_thread::sleep_for (std::chrono::seconds(1));
    } 
} 

int main() {
    std::cout << "Starting Playground++ Version ";
    std::cout << VERSION_NAME << std::endl;

    Session session1;
    std::cout << session1.get_id() << std::endl;

    /* Started websockets */
    server ws_server;

    //std::thread tp(dispatcher_thread, std::ref(ws_server));

    try {
        ws_server.set_access_channels(websocketpp::log::alevel::all);

        ws_server.clear_access_channels(websocketpp::log::alevel::frame_payload);

        ws_server.init_asio();

        ws_server.set_message_handler(bind(&on_message,&ws_server,::_1,::_2));

        ws_server.set_open_handler(bind(&on_open,&ws_server,::_1));

        ws_server.set_close_handler(bind(&on_close,&ws_server,::_1));

        ws_server.listen(9000);

        ws_server.start_accept();

        ws_server.run();
    } catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    } catch (...) { }
    
    return 0; 
}
