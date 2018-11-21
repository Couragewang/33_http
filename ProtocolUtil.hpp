#ifndef __PROTOCOL_UTIL_HPP__
#define __PROTOCOL_UTIL_HPP__

#include <iostream>
#include <string>
#include <strings.h>
#include <sstream>
#include <unordered_map>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "Log.hpp"

#define OK 200
#define NOT_FOUND 404

#define WEB_ROOT "wwwroot"
#define HOME_PAGE "index.html"

#define HTTP_VERSION "http/1.0"

class ProtocolUtil{
    public:
        static void MakeKV(std::unordered_map<std::string, std::string> &kv_,\
                std::string &str_)
        {
            std::size_t pos = str_.find(": ");
            if(std::string::npos == pos){
                return;
            }

            std::string k_ = str_.substr(0, pos);
            std::string v_ = str_.substr(pos+2);

            kv_.insert(make_pair(k_, v_));
        }
        static std::string IntToString(int &code)
        {
            stringstream ss;
            ss << code;
            return ss.str();
        }
        static std::string CodeToDesc(int code)
        {
            switch(code){
                case 200:
                    return "OK";
                case 404:
                    return "NOT FOUND";
                default:
                    return "UNKNOW";
            }
        }
};

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

        int resource_size;
        std::unordered_map<std::string, std::string> head_kv;
        int content_length;
    public:
        Request():blank("\n"), cgi(false), path(WEB_ROOT), resource_size(0),content_length(-1);
        {}
        std::string &GetParam()
        {
            return param;
        }
        int GetResourceSize()
        {
            return resource_size;
        }
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
        bool RequestHeadParse()
        {
            int start = 0;
            while(start < rq_head.size()){
                std::size_t pos = rq_head.find('\n', start);
                if(std::string::npos == pos){
                    break;
                }

                std::string sub_string_ = rq_head.substr(start, pos - start);
                if(!sub_string_.empty()){
                    ProtocolUtil::MakeKV(head_kv, sub_string_);
                }
                else{
                    break;
                }
                start = pos + 1;
            }
        }
        int GetContentLength()
        {
            std::string cl_ = head_kv["Content-Length"];
            if(!cl_.empty()){
                std::stringstream ss(cl_);
                ss >> content_length;
            }
            return content_length;
        }
        bool IsMethodLegal()
        {
            if(strcasecmp(method.c_str(),"GET") == 0 ||\
                    cgi = (strcasecmp(method.c_str(),"POST") == 0)){
                return true;
            }

            return false;
        }
        bool IsPathLegal()
        {
            struct stat st;
            if(stat(path.c_str(), &st) < 0){
                LOG(WARNING, "path not found!");
                return false;
            }

            if(S_ISDIR(st.st_mode)){
                path += "/";
                path += HOME_PAGE;
            }
            else{
                if((st.st_mode & S_IXUSR ) ||\
                   (st.st_mode & S_IXGRP) ||\
                   (st.st_mode & S_IXOTH)){
                    cgi = true;
                }
            }

            resource_size = st.st_size;
            return true;
        }
        bool IsNeedRecvText()
        {
            if(strcasecmp(method.c_str(), "POST")){
                return true;
            }
            return false;
        }
        bool IsCgi()
        {
            return cgi;
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
        void MakeStatusLine()
        {
            rsp_line = HTTP_VERSION;
            rsp_line += " ";
            rsp_line += ProtocolUtil::IntToString(code);
            rsp_line += " ";
            rsp_line += ProtocolUtil::CodeToDesc(code);
        }
        
        void MakeResponseHead(Request *&rq_)
        {
            rsp_head = "Content-Length: ";
            rsp_head += ProtocolUtil::IntToString(rq_->GetResourceSize());
            rsp_head += "\n";

        }
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
        void RecvRequestHead(std::string &head_)
        {
            head_ = "";
            std::string line_;
            while(line_ != "\n"){
                line_ = "";
                RecvOneLine(line_);
                head_ += line_;
            }
        }
        void RecvRequestText(std::string &text_, int len_, std::string &param_)
        {
            char c_;
            int i_ = 0;
            while( i_ < len_){
                recv(sock, &c_, 1, 0);
                text_.push_back(c);
            }

            param_ = text_;
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
        static int ProcessNonCgi(Connect *&conn_, Request *&rq_, Response *&rsp_)
        {
            int &code_ = rsp_->code;
            rsp_->MakeStatusLine();
            rsp_->MakeResponseHead(rq_);
        }
        static int  PorcessResponse(Connect *&conn_, Request *&rq_, Response *&rsp_)
        {
            if(rq_->IsCgi()){
                //ProcessCgi();
            }else{
                ProcessNonCgi(conn_, rq_, rsp_);
            }
        }
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

            if( !rq_->IsPathLegal() ){
                code_ = NOT_FOUND;
                goto end;
            }

            LOG(INFO, "request path is OK!");

            conn_->RecvRequestHead(rq_->rq_head);
            if(rq_->RequestHeadParse()){
                LOG(INFO, "parse head done")
            }else{
                code = NOT_FOUND;
                goto end;
            }

            if(rq_->IsNeedRecvText()){
                conn_->RecvRequestText(rq_->rq_text, rq_->GetContentLength(),\
                        rq_->GetParam());
            }

            PorcessResponse(conn_, rq_, rsp_);
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






