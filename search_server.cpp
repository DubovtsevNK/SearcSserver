#include "search_server.h"
#include <set>

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id");
    }
    std::vector<std::string_view> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (std::string_view& word : words) {
        std::string s_word (word);
        if (words_in_docs_.count(s_word) == 0) {
            words_in_docs_[s_word].first = s_word;
            std::string_view sv_word{ words_in_docs_.at(s_word).first };
            words_in_docs_.at(s_word).second = sv_word;
        }
        word_to_document_freqs_[words_in_docs_.at(s_word).second][document_id] += inv_word_count;
        document_and_word[document_id][words_in_docs_.at(s_word).second] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq, raw_query, [status](int document_id, DocumentStatus statusp, int rating) { return statusp == status; });
}
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}



const std::map< std::string_view, double> &SearchServer::GetWordFrequencies(int document_id) const
{
    static const std::map<std::string_view, double> empty_map;
    auto it = document_and_word.find(document_id);

    if(it != document_and_word.end())
    {
        return it->second;
    }
    return empty_map ;
}

std::tuple<std::vector< std::string_view>, DocumentStatus> SearchServer::MatchDocument( std::string_view raw_query, int document_id) const
{
    return MatchDocument(std::execution::seq, raw_query, document_id);
}

std::tuple<std::vector< std::string_view>, DocumentStatus>  SearchServer::MatchDocument
(const std::execution::parallel_policy &,  std::string_view raw_query, int document_id) const
{
    if (document_ids_.count(document_id) == 0)
    {
        throw  std::out_of_range("document_id is invalid");
    }
    const auto query = ParseQuery(raw_query);

    std::vector<std::string_view> matched_words;

    auto comp = [this, document_id](const auto val)
    {
        return word_to_document_freqs_.find(val) != word_to_document_freqs_.end() and word_to_document_freqs_.at(val).count(document_id) ;
    };

    if(std::any_of(query.minus_words.begin(), query.minus_words.end(),comp))
    {
      return {matched_words, documents_.at(document_id).status};
    }
   matched_words.resize(query.plus_words.size());
   std::copy_if(query.plus_words.begin(),query.plus_words.end(),matched_words.begin(),comp);

   sort(matched_words.begin(),matched_words.end());
   auto f = std::unique(matched_words.begin(),matched_words.end());
   matched_words.erase(f, matched_words.end());

   return {matched_words,documents_.at(document_id).status };

}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument
(const std::execution::sequenced_policy &,  std::string_view raw_query, int document_id) const
{
    if (document_ids_.count(document_id) == 0)
    {
        throw  std::out_of_range("document_id is invalid");
    }

    const auto query = ParseQuery(raw_query);

    std::vector< std::string_view> matched_words;

    for (const  auto& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    for (const  auto& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }

    sort(matched_words.begin(),matched_words.end());
    auto f = std::unique(matched_words.begin(),matched_words.end());
    matched_words.erase(f, matched_words.end());


    return {matched_words, documents_.at(document_id).status};
}



void SearchServer::RemoveDocument(int document_id)
{

    RemoveDocument( std::execution::seq, document_id);

}

void SearchServer::RemoveDocument(const __pstl::execution::sequenced_policy &, int document_id)
{
    if (document_ids_.count(document_id) == 1) {
        for (auto [word, freq] : GetWordFrequencies(document_id)) {
            word_to_document_freqs_[word].erase(document_id);
            if (word_to_document_freqs_.count(word) == 1 && word_to_document_freqs_.at(word).size() == 0) {
               std::string s_word{ word };
                words_in_docs_.erase(s_word);
            }
        }
        document_ids_.erase(document_id);
        documents_.erase(document_id);
        document_and_word.erase(document_id);
    }
    return;

}

void SearchServer::RemoveDocument(const __pstl::execution::parallel_policy &, int document_id)
{
    using namespace std;
    if (document_and_word.count(document_id) == 0) {
            return;
        }


        const auto& word_freqs = document_and_word.at(document_id);
        vector<string_view> words(word_freqs.size());
        transform(
            execution::par,
            word_freqs.begin(), word_freqs.end(),
            words.begin(),
            [](const auto& item) { return item.first; }
        );
        for_each(
            execution::par,
            words.begin(), words.end(),
            [this, document_id](string_view word) {
                word_to_document_freqs_.at(word).erase(document_id);
            });


        document_ids_.erase(document_id);
        documents_.erase(document_id);
        document_and_word.erase(document_id);

}

bool SearchServer::IsStopWord( std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord( std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop( std::string_view text) const
{
    std::vector<std::string_view> words;
    for (const auto word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word " + static_cast<std::string>(word) + " is invalid");
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(),ratings.end(), 0);

    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty");
    }
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
        throw  std::invalid_argument("Query word " + static_cast<std::string>(text) + " is invalid");
    }

    return {text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text) const {
    SearchServer::Query result;
    for (const auto& word : SplitIntoWords(text))
    {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop)
        {
            if (query_word.is_minus)
            {
                result.minus_words.push_back(query_word.data);
            }
            else
            {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    return result;
}
