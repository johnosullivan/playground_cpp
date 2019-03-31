#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>
#include <optional>
#include <functional>
#include <sstream>
#include <set>
#include <thread> 
#include <mutex> 

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

const std::string connection_confirm_frame = "CONNECTION_CONFIRM_FRAME";

connection_set ready_connections;
std::map<std::string,connection_set> sessions;


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

















class Session {
    public:
        bool stop_thread = false;
        server *thread_server;

        std::set<server::connection_ptr> connections;

        Session(server *_thread_server) : session_thread() { 
            thread_server = _thread_server;
            std::cout << "session_server_ptr: " << thread_server << std::endl;
        }

        ~Session(){
            stop_thread = true;
            if(session_thread.joinable()) {
                session_thread.join();
            }
        }

        void session_msg_handler(server &s, websocketpp::connection_hdl hdl, server::message_ptr msg) {
            std::cout << "Message sent to custom handler:" << msg.get() << std::endl;
        }
        void start(){
            session_thread = std::thread(&Session::thread_main,this);
        }

        void add_connection(server::connection_ptr con) {

            std::cout << "session_thread_connection_ptr: " << con << std::endl;
            //con->set_message_handler(bind(&Session::session_msg_handler,std::ref(thread_server),::_1,::_2));
        }

        void thread_main(){
            while(!stop_thread){
                // Do something useful, e.g:


                std::cout << "s" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }

    private:
        std::thread session_thread;
        std::string uuid;
        connection_set players;
};























/*
 * Will take two connections and check if they are the same 
 */
bool connections_equal(websocketpp::connection_hdl t, websocketpp::connection_hdl u)
{
    return !t.owner_before(u) && !u.owner_before(t);
}

void custom_on_msg(server & s, websocketpp::connection_hdl hdl, server::message_ptr msg) {
        std::cout << "Message sent to custom handler" << std::endl;
}

/*
 * Will fire as messages are coming in via the ws connection
 */
void on_message(server& s, websocketpp::connection_hdl hdl, message_ptr msg) {
    std::cout << "HDL: " << hdl.lock().get() << " PAYLOAD: " << msg->get_payload() << std::endl;
    try {
        //s->send(hdl, msg->get_payload(), msg->get_opcode()); // Basic Echo Example
        /*json payload = json::parse(msg->get_payload());
        std::string id = payload["session_id"];

        connection_set xx = sessions[id];
        connection_set::iterator it;
        for (it = xx.begin(); it != xx.end(); ++it) {
            if (!connections_equal(*it,hdl)) {
                s->send(*it, msg->get_payload(), msg->get_opcode());
            }
        }*/
        if (msg->get_payload() == "upgrade") {
            // Upgrade our connection_hdl to a full connection_ptr
            server::connection_ptr con = s.get_con_from_hdl(hdl);

            // Change the on message handler for this connection only to
            // custom_on_mesage

            Session current_session = Session(s);

            con->set_message_handler(bind(current_session.session_msg_handler,std::ref(s),::_1,::_2));

            std::cout << "Upgrading connection to custom handler" << std::endl;
        }
    } catch (websocketpp::exception const & e) { } catch (...) { }
}

std::optional<std::string> generate_connection_link(websocketpp::connection_hdl hdl) {
    json link_response;
    link_response["id"] = sole::uuid4().str();
    return link_response.dump();
}

/*
 * Will fire as new connections become opened
 */
void on_open(server* s, websocketpp::connection_hdl hdl) {
    /* Adds the connection to the set of all connections */
    ready_connections.insert(hdl);

    //server::connection_ptr con = s->get_con_from_hdl(hdl);

    std::optional<std::string> link_optional = generate_connection_link(hdl);
    if (link_optional.has_value()) {
        //s->send(hdl, link_optional.value(), websocketpp::frame::opcode::text);
    }
}

/*
 * Will fire as old connections are closed
 */
void on_close(server* s, websocketpp::connection_hdl hdl) {
    ready_connections.erase(hdl);
}

bool status = true;

class count_down { 
    public: 
        void operator()(server &server_ref, connection_set sessions_players) 
        {         
                for(int i = 0; i < 10; i++)
                {
                    connection_set::iterator it;

                    for (it = sessions_players.begin(); it != sessions_players.end(); ++it) {
                        server_ref.send(*it, std::to_string((int)i), websocketpp::frame::opcode::text);
                    }

                    std::this_thread::sleep_for (std::chrono::seconds(1));
                }
        } 
}; 


std::mutex mtx_main_thread;  
std::vector<Session> sessions_vector;

void dispatcher_thread(server *server_ref) 
{ 
    while (true) { 
        int count = ready_connections.size();
        if (count == 1) {
            mtx_main_thread.lock();

            std::cout << "dispatcher_thread_server_ptr: " << server_ref << std::endl;

            Session current_session = Session(server_ref);

            connection_set::iterator it;

            for (it = ready_connections.begin(); it != ready_connections.end(); ++it) {
                server::connection_ptr con = server_ref->get_con_from_hdl(*it);
                
                con->set_message_handler(bind(&custom_on_msg,server_ref,::_1,::_2));
                //current_session.add_connection(con);
            }

            ready_connections.clear();

            //sessions_vector.push_back(current_session);

            mtx_main_thread.unlock();
        }
        std::this_thread::sleep_for (std::chrono::seconds(1));
    } 
} 

int main() {
    std::cout << "Starting Playground++ Version ";
    std::cout << VERSION_NAME << std::endl;

    std::this_thread::sleep_for (std::chrono::seconds(5));


    /* Started websockets */
    server ws_server;

   //std::thread tp(dispatcher_thread, &ws_server);
    
    try {
        ws_server.set_access_channels(websocketpp::log::alevel::all);

        ws_server.clear_access_channels(websocketpp::log::alevel::frame_payload);

        ws_server.init_asio();

        ws_server.set_message_handler(bind(&on_message,std::ref(ws_server),::_1,::_2));

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
