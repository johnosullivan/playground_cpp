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
            session_thread = std::thread(&Session::thread_main,this);
            in_progress = true;

            json session_details;
            session_details["uuid"] = uuid;
            session_details["playerCount"] = connections.size();
                                                                                                                                                                                                                                                                                                                                                                                                                                                             
            connection_set::iterator it;
            std::string data = session_details.dump();
            for (it = connections.begin(); it != connections.end(); ++it) {
                thread_server->send(*it, data, websocketpp::frame::opcode::text);
            }
        }

        void terminate_session() {
            stop_thread = true;
            if(session_thread.joinable()) {
                session_thread.join();
            }
        }

        void thread_main(){
            while(!stop_thread){
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
            connections.insert(hdl);
        }

        void send_frame(std::string rawData, json data, websocketpp::connection_hdl hdl) {
            if (data.find("done") != data.end()) {
                // there is an entry with key "foo"
                doneCount = doneCount + 1;
                if (doneCount == connections.size()) {
                    in_progress = false;
                    std::cout << "Done: " << uuid << std::endl; 
                }
            }

            connection_set::iterator it;
            for (it = connections.begin(); it != connections.end(); ++it) {
                if (!connections_equal(*it, hdl)) {
                    thread_server->send(*it, rawData, websocketpp::frame::opcode::text);
                }
            }
        }

        void on_message(server& s, websocketpp::connection_hdl hdl, message_ptr msg) {
            std::cout << "HDL: " << hdl.lock().get() << "SESSION PAYLOAD: " << msg->get_payload() << std::endl;

        }

     private:
        connection_set connections;
        bool stop_thread = false;
        bool in_progress = false;
        std::string uuid;
        std::thread session_thread;

        int doneCount = 0;
};


std::map<std::string,Session*> global_sessions_map;

/*
 * Will fisre as messages are coming in via the ws connection
 */
void on_message(server& s, websocketpp::connection_hdl hdl, message_ptr msg) {
    std::cout << "HDL: " << hdl.lock().get() << " PAYLOAD: " << msg->get_payload() << std::endl;
    try {
        
        /*auto data = json::parse(msg->get_payload());
        
        std::string key = data["session_id"];

        global_sessions_map[key]->send_frame(msg->get_payload(),data ,hdl);*/

    } catch (websocketpp::exception const & e) { 
        std::cout << e.what() << std::endl;
    } catch (...) { 
        std::cout << "Error" << std::endl;
    }
}

/*
 * Will fire as new connections become opened
 */
void on_open(server* s, websocketpp::connection_hdl hdl) {
    ready_connections.insert(hdl); 
}

/*
 * Will fire as connections are closed
 */
void on_close(server* s, websocketpp::connection_hdl hdl) {
    ready_connections.erase(hdl);
}


void dispatcher_thread(server& server_ref) 
{   
    while (dispatcher_bool) { 
        int count = ready_connections.size();

        if (count == 2) {

            connection_set::iterator it;

            Session * current_session = new Session(&server_ref);

            for (it = ready_connections.begin(); it != ready_connections.end(); ++it) {
                server::connection_ptr con = server_ref.get_con_from_hdl(*it);

                std::cout << "dispatcher_thread_connection_ptr: " << con << std::endl;
                
                current_session->add_connection(*it);

                //con->set_message_handler(bind(&current_session->on_message,std::ref(server_ref),::_1,::_2));
                //con->set_message_handler(bind(current_session->on_message,std::ref(server_ref),::_1,::_2));
                //con->set_message_handler(bind(&Session::on_message,std::ref(server_ref),::_1,::_2));
                //con->set_message_handler(bind(&Session::on_message,current_session,::_1,::_2));
            }

            Console::println("current_session: ", current_session->get_id());

            global_sessions_map[current_session->get_id()] = current_session;

            current_session->start();

            ready_connections.clear();
            
        }

        std::this_thread::sleep_for (std::chrono::seconds(10));
    } 
} 

int main() {
    std::cout << "Starting Playground++ Version " << VERSION_NAME << std::endl;

    /* Started websockets */
    server ws_server;

    //std::thread dispatcher(dispatcher_thread, std::ref(ws_server));

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
