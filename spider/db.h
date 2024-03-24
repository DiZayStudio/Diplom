#pragma once
#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include "link.h"

class DataBase
{
public:
    DataBase();
    void SetConnection(std::unique_ptr<pqxx::connection> in_c);

    DataBase(const DataBase&) = delete;
    DataBase& operator=(const DataBase&) = delete;

    void CreateTable();
    void DeleteTable();
    void InsertData(const std::map<std::string, int>& words, const Link& link);
    void ClearTable(const std::string tableName);
    bool SearchLink(const Link& link);

private:  

    std::unique_ptr<pqxx::connection> c_;
};
