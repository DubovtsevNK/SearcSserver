#include "remove_duplicates.h"


void RemoveDuplicates(SearchServer& search_server)
{
    std::vector<int> delete_documents;
    std::set<std::set<std::string>> words_in_docs;
    for (const int &document_id : search_server) {
          const auto &words = search_server.GetWordFrequencies(document_id);
          std::set<std::string> words_one_doc;

          for(const auto& [word, freak] : words)
          {
              words_one_doc.insert(word);
          }

          if(words_in_docs.count(words_one_doc) > 0)
          {
              delete_documents.push_back(document_id);
          }
          else
          {
              words_in_docs.insert(words_one_doc);
          }
    }
    for( int del: delete_documents )
    {
        std::cout << "Found duplicate document id " << del << std::endl;
        search_server.RemoveDocument(del);
    }
}
