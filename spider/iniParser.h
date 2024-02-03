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

        conf.host = pt.get<std::string>("spider.host", "en.wikipedia.org");
        conf.startPage = pt.get<std::string>("spider.startPage", "/wiki/Main_Page");
        conf.depth = pt.get<int>("spider.depth", 1);

        conf.dbHost = pt.get<std::string>("db.host", "pg3.sweb.ru");
        conf.dbPort = pt.get<std::string>("db.port", "5432");
        conf.dbName = pt.get<std::string>("db.name", "samozaschi");
        conf.dbUser = pt.get<std::string>("db.user", "samozaschi");
        conf.dbPass = pt.get<std::string>("db.pass", "GQ5B57BNY$MY56Gw");
    }
};