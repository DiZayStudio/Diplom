#pragma once
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "struct.h"

class IniParser
{
public:

    void parse(const std::string& filename, Configure& conf)
    {
        boost::property_tree::ptree pt;
        boost::property_tree::ini_parser::read_ini(filename, pt);

        conf.host = pt.get<std::string>("server.host", "0.0.0.0");
        conf.port = pt.get<int>("server.port", 8080);

        conf.dbHost = pt.get<std::string>("db.host", "pg3.sweb.ru");
        conf.dbPort = pt.get<std::string>("db.port", "5432");
        conf.dbName = pt.get<std::string>("db.name", "samozaschi");
        conf.dbUser = pt.get<std::string>("db.user", "samozaschi");
        conf.dbPass = pt.get<std::string>("db.pass", "GQ5B57BNY$MY56Gw");
    }
};