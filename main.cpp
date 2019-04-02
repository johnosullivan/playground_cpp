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
            Console::println("session_server_ptr: ", thread_server);
        }

        ~Session(){
            Console::println("~Session");
            terminate_session();
        }

        void session_msg_handler(server &s, websocketpp::connection_hdl hdl, server::message_ptr msg) {
            std::cout << "session_msg_handler: " << msg.get() << std::endl;

            /*connection_set::iterator it;
            for (it = connections.begin(); it != connections.end(); ++it) {
                if (!connections_equal(*it,hdl)) {
                    s.send(*it, msg->get_payload(), msg->get_opcode());
                }
            }*/
        }

        void start(){
            Console::println("session_start");
            session_thread = std::thread(&Session::thread_main,this);
            in_progress = true;
        }

        void terminate_session() {
            Console::println("terminate_session");
            stop_thread = true;
            if(session_thread.joinable()) {
                session_thread.join();
            }
        }

        void add_connection(websocketpp::connection_hdl hdl) {
            //std::cout << "session_thread_add_connection_hdl: " << hdl.lock().get() << std::endl;
            //connections.insert(hdl);
        }

         void thread_main(){
            while(!stop_thread){
                Console::println("session_msg_handler_connections_size: ");
                //std::cout << "session_msg_handler_connections_size: " << connections.size() << std::endl;
                
                temp_count++;

                if (temp_count == 3) {
                    in_progress = false;
                }
                
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }

        bool get_progress() {
            return in_progress;
        }

        std::string get_id() {
            return uuid;
        }

     private:
        bool stop_thread = false;
        bool in_progress = false;
        std::string uuid;
        std::thread session_thread;
        int temp_count;
};

void custom_on_msg(server & s, websocketpp::connection_hdl hdl, server::message_ptr msg) {
        std::cout << "Message sent to custom handler" << std::endl;
}

/*
 * Will fire as messages are coming in via the ws connection
 */
void on_message(server& s, websocketpp::connection_hdl hdl, message_ptr msg) {

    //std::cout << "on_message" << std::ref(s) << std::endl;

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
            con->set_message_handler(bind(&custom_on_msg,std::ref(s),::_1,::_2));

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
 
void dispatcher_thread(server& server_ref) 
{   
    while (true) { 
        int count = ready_connections.size();

        Console::println("dispatcher_thread_count");

        if (count == 1) {
            //mtx_main_thread.lock();

            //std::cout << "dispatcher_thread_server_ptr: " << std::ref(server_ref) << std::endl;

            //Session current_session = Session(&server_ref);

            //connection_set::iterator it;

            /* for (it = ready_connections.begin(); it != ready_connections.end(); ++it) {
                server::connection_ptr con = server_ref.get_con_from_hdl(*it);

                std::cout << "dispatcher_thread_connection_ptr: " << con << std::endl;
                
                //server_ref.send(*it, std::to_string((int)2019), websocketpp::frame::opcode::text);
                
                current_session.add_connection(*it);

                //con->set_message_handler(bind(current_session.session_msg_handler,std::ref(server_ref),::_1,::_2));

                //con->set_message_handler(bind(&custom_on_msg,std::ref(server_ref),::_1,::_2));
            }*/

            //current_session.start();

            //sessions_vector.push_back(current_session);

            //sessions_vector.push_back(current_session);

            //ready_connections.clear();

            //sessions_vector.push_back(current_session);

            //mtx_main_thread.unlock();
        }
        std::this_thread::sleep_for (std::chrono::seconds(10));
    } 
} 

std::set<Session*> global_sessions_set;

bool status_test = true;

void sessions_clean_up_thread() 
{   
    while (status_test) { 
        std::cout << "sessions_clean_up_thread_count: " << global_sessions_set.size() << std::endl;

        std::set<Session*>::iterator it;

        for (it = global_sessions_set.begin(); it != global_sessions_set.end(); ++it) {
            Session * session_temp = *it;

            std::cout << "session_temp_ptr: " << (*it) << std::endl;

            if ((*it)->get_progress() == false) {
                //global_sessions_set.erase(session_temp);
                delete (*it);
            }
        }

        std::this_thread::sleep_for (std::chrono::seconds(1));
    } 
} 

int main() {
    std::cout << "Starting Playground++ Version " << VERSION_NAME << std::endl;

    std::thread clean_sessions(sessions_clean_up_thread);

    std::this_thread::sleep_for (std::chrono::seconds(5));

    /* Started websockets */
    server ws_server;

    Session * current_session = new Session(&ws_server);
    current_session->start();

    std::cout << "current_session_ptr: " << current_session << std::endl;

    global_sessions_set.insert(current_session);


    std::this_thread::sleep_for (std::chrono::seconds(10));

    status_test = false;
    clean_sessions.join();

    /*std::thread tp(dispatcher_thread, std::ref(ws_server));

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
    } catch (...) { }*/
    
    return 0; 
}
