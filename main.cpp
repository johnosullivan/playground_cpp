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

bool dispatcher_bool = true;



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

/*
 * Will take two connections and check if they are the same 
 */
bool connections_equal(websocketpp::connection_hdl t, websocketpp::connection_hdl u)
{
    return !t.owner_before(u) && !u.owner_before(t);
}




class Console {
    private:
        Console() = default;

        inline static void innerPrint(std::ostream &stream) {}

        template<typename Head, typename... Tail>
        inline static void innerPrint(std::ostream &stream, Head const head, Tail const ...tail) {
            stream << head;
            innerPrint(stream, tail...);
        }

    public:
        template<typename Head, typename... Tail>
        inline static void print(Head const head, Tail const ...tail) {
            std::stringbuf buffer;
            std::ostream stream(&buffer);
            innerPrint(stream, head, tail...);
            std::cout << buffer.str();
        }

        template<typename Head, typename... Tail>
        inline static void println(Head const head, Tail const ...tail) {
            print(head, tail..., "\n");
        }
};




class Session {

    public:
        server *thread_server;

        Session(server *_thread_server) : session_thread() { 
            thread_server = _thread_server;
            uuid = sole::uuid4().str();
            Console::println("Session(", thread_server, ")");
        }

        ~Session(){
            Console::println("~Session(", uuid, ")");
            terminate_session();
        }

        void start(){
            Console::println("Session.start(", thread_server, ")");
            Console::println("  =====> ", uuid);
            session_thread = std::thread(&Session::thread_main,this);
            in_progress = true;
        }

        void terminate_session() {
            stop_thread = true;
            if(session_thread.joinable()) {
                session_thread.join();
            }
        }

        void thread_main(){
            while(!stop_thread){
                //Console::println("session_msg_handler_connections_size: ", connections.size());
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }

        bool get_progress() {
            return in_progress;
        }

        std::string get_id() {
            return uuid;
        }

        void add_connection(websocketpp::connection_hdl hdl) {
            std::cout << "session_thread_add_connection_hdl: " << hdl.lock().get() << std::endl;
            connections.insert(hdl);
        }

        void send_frame(std::string data, websocketpp::connection_hdl hdl) {
            connection_set::iterator it;
            for (it = connections.begin(); it != connections.end(); ++it) {
                if (!connections_equal(*it,hdl)) {
                    thread_server->send(hdl, data, websocketpp::frame::opcode::text);
                }
            }
        }

     private:
        connection_set connections;
        bool stop_thread = false;
        bool in_progress = false;
        std::string uuid;
        std::thread session_thread;
        int temp_count;
};

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
            // con->set_message_handler(bind(&custom_on_msg,std::ref(s),::_1,::_2));

            std::cout << "Upgrading connection to custom handler" << std::endl;
        }
    } catch (websocketpp::exception const & e) { } catch (...) { }
}

/*
 * Will fire as new connections become opened
 */
void on_open(server* s, websocketpp::connection_hdl hdl) {
    /* Adds the connection to the set of all connections */
    ready_connections.insert(hdl);

    //server::connection_ptr con = s->get_con_from_hdl(hdl);

    s->send(hdl, "Connected", websocketpp::frame::opcode::text);
}

/*
 * Will fire as old connections are closed
 */
void on_close(server* s, websocketpp::connection_hdl hdl) {
    ready_connections.erase(hdl);
}

bool status = true;


void dispatcher_thread(server& server_ref) 
{   
    while (dispatcher_bool) { 
        int count = ready_connections.size();

        Console::println("dispatcher_thread_count: ", count);

        /*if (count == 1) {
            connection_set::iterator it;

            Session * current_session = new Session(&server_ref);

            for (it = ready_connections.begin(); it != ready_connections.end(); ++it) {
                server::connection_ptr con = server_ref.get_con_from_hdl(*it);

                std::cout << "dispatcher_thread_connection_ptr: " << con << std::endl;
                
                current_session->add_connection(*it);
            }

            current_session->start();

            ready_connections.clear();
        }*/

        std::this_thread::sleep_for (std::chrono::seconds(10));
    } 
} 

std::set<Session*> global_sessions_set;
std::map<std::string,Session*> test;

bool status_test = true;



















int main() {
    std::cout << "Starting Playground++ Version " << VERSION_NAME << std::endl;

    /* Started websockets */
    server ws_server;

    std::thread dispatcher(dispatcher_thread, std::ref(ws_server));

    std::cout << &ws_server << std::endl;
    
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
