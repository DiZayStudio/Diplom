#include "db.h"
#include <iostream>
#include <string.h>
#include <mutex>

std::mutex mtxDb;
DataBase::DataBase(){}

void DataBase::SetConnection(std::unique_ptr<pqxx::connection> in_c)
{
	c_ = std::move(in_c);
}

void DataBase::DeleteTable() {
	pqxx::work tx{ *c_ };

	tx.exec("DROP table DocumentsWords");
	tx.exec("DROP table Documents");
	tx.exec("DROP table Words");

	tx.commit();
}

void DataBase::CreateTable() {
	pqxx::work tx{ *c_ };

	tx.exec("CREATE TABLE IF NOT EXISTS Documents (id SERIAL PRIMARY KEY, protocol VARCHAR(100) NOT NULL, hostName VARCHAR(250) NOT NULL, query VARCHAR(250) NOT NULL); ");
	tx.exec("CREATE TABLE IF NOT EXISTS Words (id SERIAL PRIMARY KEY, word VARCHAR(33) NOT NULL); ");
	tx.exec("CREATE TABLE IF NOT EXISTS DocumentsWords (docLink_id integer NOT NULL REFERENCES Documents (id), word_id integer NOT NULL REFERENCES Words (id), count integer NOT NULL); ");

	tx.commit();
}

void DataBase::InsertData(const std::map<std::string, int>& words, const Link& link) {
	
	pqxx::work tx{ *c_ };
	std::string query;

	std::lock_guard<std::mutex> lock(mtxDb);
	// сохраняем ссылку
	query = "INSERT INTO Documents VALUES ( nextval('documents_id_seq'::regclass), "
		"'" + (link.protocol) + "', '" + link.hostName + "', '" + link.query + "') RETURNING id";

	int id_dokument = tx.query_value<int>(query);
	// проверка наличия данных в даблице
	int count = 0;
	int id_word = 0;

	for (const auto element : words) {		
		query = "SELECT count(id) FROM words WHERE word='" + element.first + "'";
		count = tx.query_value<int>(query);
		
		// если таблица не пустая, поиск слова 	
		if (count != 0) {
			query = "SELECT id FROM words WHERE word='" + element.first + "'";
			id_word = tx.query_value<int>(query);
		} else {
			query = "INSERT INTO Words VALUES ( nextval('words_id_seq'::regclass), '" + element.first + "') RETURNING id";
			id_word = tx.query_value<int>(query);
		}

		query = "INSERT INTO documentswords(doclink_id, word_id, count) "
				"VALUES ('" + std::to_string(id_dokument)+"', '"+std::to_string(id_word)+"', '"+std::to_string(element.second)+ "') ";
		// загрузить индекс слова
		tx.exec(query);
	}
	tx.commit();
};

void DataBase::ClearTable(const std::string tableName) {
	pqxx::work tx{ *c_ };
	tx.exec("DELETE from " + tableName);
	tx.commit();
}

bool DataBase::SearchLink(const Link& link) {
	pqxx::work tx{ *c_ };
	int count = tx.query_value<int>("SELECT COUNT(*) FROM documents "
						"WHERE protocol='" + link.protocol + "' AND hostName='" + link.hostName + "' AND query='" + link.query + "'");
	if (count == 0) {
		return true;
	}
	else {
		return false;
	}
	
}
	