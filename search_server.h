#pragma once

#include <stdexcept>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <execution>
#include "concurentmap.h"

#include "string_processing.h"
#include "document.h"


const int MAX_RESULT_DOCUMENT_COUNT = 5;



class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
    {
        if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw std::invalid_argument("Some of stop words are invalid");
        }
    }

    explicit SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text)){ }

    explicit SearchServer(const std::string_view stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text)){ }

    void AddDocument(int document_id,  std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    void RemoveDocument(int document_id);

    void RemoveDocument(const std::execution::parallel_policy &, int document_id);

    void RemoveDocument(const std::execution::sequenced_policy &, int document_id);

    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;

    template <typename Policy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const Policy exec_policy, std::string_view raw_query, DocumentPredicate document_predicate) const;

    template <typename Policy>
    std::vector<Document> FindTopDocuments(const Policy policy, std::string_view raw_query, DocumentStatus status) const;

    template <typename Policy>
    std::vector<Document> FindTopDocuments(const Policy policy, std::string_view raw_query) const;

    int GetDocumentCount() const;

    std::set<int>::const_iterator begin() const
    {
        return document_ids_.begin();
    }

    std::set<int>::const_iterator end() const
    {
        return document_ids_.end();
    }

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    std::tuple< std::vector< std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy &, std::string_view raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy &, std::string_view raw_query,int document_id) const;




private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    std::map<std::string, std::pair<std::string, std::string_view>> words_in_docs_;
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    std::map<int,std::map<std::string_view, double>> document_and_word;




    bool IsStopWord(std::string_view word) const;

    static bool IsValidWord(const std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;


    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(std::string_view text) const;


    double ComputeWordInverseDocumentFreq(std::string_view text) const {
        return  log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(text).size());
    }
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
    template <typename Policy,typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Policy policy,const Query& query, DocumentPredicate document_predicate) const;
};

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename Policy,typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const  Policy policy,std::string_view raw_query, DocumentPredicate document_predicate) const {
     auto query = ParseQuery(raw_query);

     sort(query.minus_words.begin(),query.minus_words.end());
     auto f = std::unique(query.minus_words.begin(),query.minus_words.end());
     query.minus_words.erase(f, query.minus_words.end());

     sort(query.plus_words.begin(),query.plus_words.end());
     auto f1 = std::unique(query.plus_words.begin(),query.plus_words.end());
     query.plus_words.erase(f1, query.plus_words.end());


    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    std::sort(policy, matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
            return lhs.rating > rhs.rating;
        } else {
            return lhs.relevance > rhs.relevance;
        }
    });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename Policy>
std::vector<Document> SearchServer::FindTopDocuments(const  Policy policy, std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}
template <typename Policy>
std::vector<Document> SearchServer::FindTopDocuments(const  Policy policy, std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus statusp, int rating) { return statusp == status; });
}
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    FindAllDocuments(std::execution::seq,query, document_predicate);
}



template <typename Policy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Policy policy,const Query& query, DocumentPredicate document_predicate) const {

    ConcurrentMap<int, double> document_to_relevance(16);
    std::for_each(policy, query.plus_words.begin(), query.plus_words.end(),[this, &document_to_relevance, &document_predicate](auto word)
    {
        if (word_to_document_freqs_.count(word) == 0) {
            return ;
        }
         const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
         for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
             const auto& document_data = documents_.at(document_id);
             if (document_predicate(document_id, document_data.status, document_data.rating)) {
                 document_to_relevance[document_id].ref_to_value +=  static_cast<double>(term_freq * inverse_document_freq);
             }
         }

    });
    std::for_each(policy, query.minus_words.begin(), query.minus_words.end(),[this, &document_to_relevance](auto word)
    {
        if (word_to_document_freqs_.count(word) == 0) {
            return;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }

    });

    std::vector<Document> matched_documents;
    auto map_one_result = document_to_relevance.BuildOrdinaryMap();
    for (const auto [document_id, relevance] : map_one_result) {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}



