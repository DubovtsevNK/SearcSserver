#pragma once

#include "document.h"
#include "search_server.h"

#include <vector>
#include <string>
#include <deque>




class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server): server(search_server){
    }
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;
private:
    struct QueryResult {
        unsigned long  time_query;
        bool empty;
    };
    const  SearchServer &server;
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    unsigned long  time = 0;

    void AddDeque ( bool empty);


};
template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    auto resultQuery = server.FindTopDocuments(raw_query,document_predicate);
    AddDeque(resultQuery.empty());
    return resultQuery;
}

