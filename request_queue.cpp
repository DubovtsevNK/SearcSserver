#include "request_queue.h"


#include <algorithm>


std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query, DocumentStatus status)
{
    auto resultQuery = server.FindTopDocuments(raw_query,status);// напишите реализацию
    AddDeque(resultQuery.empty());
    return resultQuery;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query)
{
    auto resultQuery = server.FindTopDocuments(raw_query);
    AddDeque(resultQuery.empty());
    return resultQuery;
}

int RequestQueue::GetNoResultRequests() const

{
    return std::count_if(requests_.begin(), requests_.end(), [](const QueryResult &res)
    {
        return res.empty == true;
    });
}

void RequestQueue::AddDeque(bool empty)
{
    time ++;

    while(time - requests_.front().time_query >= min_in_day_)
    {
        requests_.pop_front();
    }

    requests_.push_back({time, empty});
}



