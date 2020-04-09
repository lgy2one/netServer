/******************************************************
 *Copyright 2020, Li GuoYan.
 *
 *Author: Li GuoYan
 *
 *httpsession类
 *
 *200：请求被正常处理
 *204：请求被受理但没有资源可以返回
 *206：客户端只是请求资源的一部分，服务器只对请求的部分资源执行GET方法，相应报文中通过Content-Range指定范围的资源。
 *
 *301：永久性重定向
 *302：临时重定向
 *303：与302状态码有相似功能，只是它希望客户端在请求一个URI的时候，能通过GET方法重定向到另一个URI上
 *304：发送附带条件的请求时，条件不满足时返回，与重定向无关
 *307：临时重定向，与302类似，只是强制要求使用POST方法
 *
 *400：请求报文语法有误，服务器无法识别
 *401：请求需要认证
 *403：请求的对应资源禁止被访问
 *404：服务器无法找到对应资源
 *
 *500：服务器内部错误
 *503：服务器正忙
*******************************************************/

#include "HttpSession.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>

HttpSession::HttpSession()
    : praseresult_(false),
    keepalive_(true)
{

}

HttpSession::~HttpSession()
{

}

bool HttpSession::PraseHttpRequest(std::string &msg, HttpRequestContext &httprequestcontext)
{
    //std::cout << "HttpServer::HttpParser" << std::endl;
	std::string crlf("\r\n"), crlfcrlf("\r\n\r\n");
	size_t prev = 0, next = 0, pos_colon;
	std::string key, value;

    bool praseresult = false;
    //以下解析可以改成状态机，解决一次收Http报文不完整问题
	//prase http request line
	if ((next = msg.find(crlf, prev)) != std::string::npos)
	{
		std::string first_line(msg.substr(prev, next - prev));
		prev = next;
		std::stringstream sstream(first_line);
		sstream >> (httprequestcontext.method);
		sstream >> (httprequestcontext.url);
		sstream >> (httprequestcontext.version);
	}
	else
	{
        std::cout << "msg" << msg << std::endl;
		std::cout << "Error in httpPraser: http_request_line isn't complete!" << std::endl;
        praseresult = false;
        msg.clear();
        return praseresult;
        //可以临时存起来，凑齐了再解析
	}
    //prase http request header
	size_t pos_crlfcrlf = 0;
	if (( pos_crlfcrlf = msg.find(crlfcrlf, prev)) != std::string::npos)
	{
		while (prev != pos_crlfcrlf)
        {
            next = msg.find(crlf, prev + 2);
            pos_colon = msg.find(":", prev + 2);
            key = msg.substr(prev + 2, pos_colon - prev-2);
            value = msg.substr(pos_colon + 2, next-pos_colon-2);
            prev = next;
            httprequestcontext.header.insert(std::pair<std::string, std::string>(key, value));
        }
	}
    else
    {
        std::cout << "Error in httpPraser: http_request_header isn't complete!" << std::endl;
        praseresult = false;
        msg.clear();
        return praseresult;
    }
    //prase http request body
	httprequestcontext.body = msg.substr(pos_crlfcrlf + 4);
    praseresult = true;
    msg.clear();
    return praseresult;
}

void HttpSession::HttpProcess(const HttpRequestContext &httprequestcontext, std::string &responsecontext)
{
    std::string responsebody;
    std::string errormsg;
    std::string path;
    std::string querystring;
    std::string filetype;

    if("GET" == httprequestcontext.method)
    {
        ;
    }
    else if("POST" == httprequestcontext.method)
    {
        ;
    }
    else
    {
        std::cout << "HttpServer::HttpParser" << std::endl;
        errormsg = "Method Not Implemented";
        HttpError(501, "Method Not Implemented", httprequestcontext, responsecontext);
        return;
    }

    size_t pos = httprequestcontext.url.find("?");
    if(pos != std::string::npos)
    {
        path = httprequestcontext.url.substr(0, pos);
        querystring = httprequestcontext.url.substr(pos+1);
    }
    else
    {
        path = httprequestcontext.url;
    }

    //keepalive判断处理
    std::map<std::string, std::string>::const_iterator iter = httprequestcontext.header.find("Connection");
    if(iter != httprequestcontext.header.end())
    {
        keepalive_ = (iter->second == "Keep-Alive");
    }
    else
    {
        if(httprequestcontext.version == "HTTP/1.1")
        {
            keepalive_ = true;//HTTP/1.1默认长连接
        }
        else
        {
            keepalive_ = false;//HTTP/1.0默认短连接
        }
    }


    if("/" == path)
    {
        path = "/lgy2one.html";
    }
    else if("/hello" == path)
    {
        //Webbench 测试用
        std::string filetype("text/html");
        responsebody = ("hello world");
        responsecontext += httprequestcontext.version + " 200 OK\r\n";
        responsecontext += "Li GuoYan's NetServer/0.1\r\n";
        responsecontext += "Content-Type: " + filetype + "; charset=utf-8\r\n";
        if(iter != httprequestcontext.header.end())
        {
            responsecontext += "Connection: " + iter->second + "\r\n";
        }
        responsecontext += "Content-Length: " + std::to_string(responsebody.size()) + "\r\n";
        responsecontext += "\r\n";
        responsecontext += responsebody;
        return;
    }
    else
    {
        int strSize = path.size();
        for(int i = 0; i < strSize;i++){
            if(path.at(i) == '.'){
                filetype.append(path, i + 1, strSize - i - 1);
                break;
            }
        }
    }

    path.insert(0,".");
    std::ifstream fp(path);
    if(fp.fail())
    {
        HttpError(404, "Not Found", httprequestcontext, responsecontext);
        return;
    }
    else
    {
        while(!fp.eof())
        {
            std::string buffer;
            getline(fp, buffer, '#');

            if(!fp.eof()){
                buffer.append("#");
            }
            responsebody.append(buffer);
        }
        fp.close();
    }

    /*Only supported js,css,html currently 3.5*/
    if(filetype == "js"){
        filetype = "text/JavaScript";
    }
    else{
        filetype.insert(0,"text/");
    }

    responsecontext += httprequestcontext.version + " 200 OK\r\n";
    responsecontext += "Server: Li GuoYan's NetServer/0.1\r\n";
    responsecontext += "Content-Type: " + filetype + "; charset=utf-8\r\n";
    if(iter != httprequestcontext.header.end())
    {
        responsecontext += "Connection: " + iter->second + "\r\n";
    }
    responsecontext += "Content-Length: " + std::to_string(responsebody.size()) + "\r\n";
    responsecontext += "\r\n";
    responsecontext += responsebody;
}

void HttpSession::HttpError(const int err_num, const std::string short_msg, const HttpRequestContext &httprequestcontext, std::string &responsecontext)
{
    //这里string创建销毁应该会耗时间
    std::string responsebody;
    responsebody += "<html><title>出错了</title>";
    responsebody += "<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"></head>";
    responsebody += "<style>body{background-color:#f;font-size:14px;}h1{font-size:60px;color:#eeetext-align:center;padding-top:30px;font-weight:normal;}</style>";
    responsebody += "<body bgcolor=\"ffffff\"><h1>";
    responsebody += std::to_string(err_num) + " " + short_msg;
    responsebody += "</h1><hr><em> Li GuoYan's NetServer</em>\n</body></html>";

    std::string httpversion;
    if(httprequestcontext.version.empty())
    {
        httpversion = "HTTP/1.1";
    }
    else
    {
        httpversion = httprequestcontext.version;
    }

    responsecontext += httpversion + " " + std::to_string(err_num) + " " + short_msg + "\r\n";
    responsecontext += "Server: Li GuoYan's NetServer/0.1\r\n";
    responsecontext += "Content-Type: text/html\r\n";
    responsecontext += "Connection: Keep-Alive\r\n";
    responsecontext += "Content-Length: " + std::to_string(responsebody.size()) + "\r\n";
    responsecontext += "\r\n";
    responsecontext += responsebody;
}

