#ifndef __PROTOCOL_UTIL_HPP__
#define __PROTOCOL_UTIL_HPP__

#include <iostream>
#include <string>
#include <strings.h>
#include <sstream>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "Log.hpp"

#define OK 200
#define NOT_FOUND 404

#define WEB_ROOT "wwwroot"
#define HOME_PAGE "index.html"

class Request{
    public:
        std::string rq_line;
        std::string rq_head;
        std::string blank;
        std::string rq_text;
    private:
        std::string method;
        std::string uri;
        std::string version;
        bool cgi; //method=POST, GET-> uri(?)

        std::string path;
        std::string param;
    public:
        Request():blank("\n"), cgi(false), path(WEB_ROOT)
        {}
        void RequestLineParse()
        {
            stringstream ss(rq_line);
            ss >> method >> uri >> version;
        }
        void UriParse()
        {
            std::size_t pos_ = uri.find('?');
            if(std::string::npos != pos_){
                cgi = true;
                path += uri.substr(0, pos);
                param = uri.substr(pos+1);
            }
            else{
                path += uri;
            }
            if(path[path.size() -1] == '/'){
                path += HOME_PAGE;
            }
        }
        bool IsMethodLegal()
        {
            if(strcasecmp(method.c_str(),"GET") == 0 ||\
                    cgi = (strcasecmp(method.c_str(),"POST") == 0)){
                return true;
            }

            return false;
        }
        ~Request()
        {}
};

class Response{
    private:
        std::string rsp_line;
        std::string rsp_head;
        std::string blank;
        std::string rsp_text;
    public:
        int code;
    public:
        Response():blank("\n"), code(OK)
        {}

        ~Response()
        {}
};

class Connect{
    private:
        int sock;
    public:
        Connect(int sock_):sock(sock_)
        {}
        int RecvOneLine(std::string &line_)
        {
            char c = 'X';
            while(c != '\n'){
                ssize_t s = recv(sock, &c, 1, 0);
                if(s > 0){
                    if( c == '\r' ){
                        recv(sock, &c, 1, MSG_PEEK);
                        if( c== '\n' ){
                            recv(sock, &c, 1, 0);
                        }
                        else{
                            c = '\n';
                        }
                    }

                    line_.push_back(c);
                }
                else{
                    break;
                }
            }
            return line_.size();
        }
        ~Connect()
        {
            if(sock >= 0){
                close(sock);
            }
        }
};

class Entry{
    public:
        static void *HandlerRequest(void *arg_)
        {
            int sock_ = *(int*)arg_;
            delete (int*)arg_;
            Connect *conn_ = new Connect(sock_);
            Request *rq_ = new Request();
            Response *rsp_ = new Response();

            int &code_ = rsp_->code;

            conn_->RecvOneLine(rq_->rq_line);
            rq_->RequestLineParse();
            if( !rq_->IsMethodLegal() ){
                code_ = NOT_FOUND;
                goto end;
            }

            rq_->UriParse();


end:
            if(code != OK){
                //HandlerError(sock_);
            }
            delete conn_;
            delete rq_;
            delete rsp_;
        }
};

#endif






