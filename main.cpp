#include "search_server.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>


#include "test_example_functions.h"
#include "search_server.h"

using namespace std;

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(const bool t, const string& t_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t == false) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << t_str <<") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl((expr), #expr , __FILE__, __FUNCTION__, __LINE__, ""s) /* реализовать самостоятельно */

#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr , __FILE__, __FUNCTION__, __LINE__, hint)/* реализовать самостоятельно */

template <class T>
void RunTestImpl(T &t, const string t_str) {
    t();
    cerr<<t_str<<" OK"<<endl;
}

#define RUN_TEST(func) RunTestImpl(func,#func)  // напишите недостающий код


// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server (" "s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }
}
void TestAddDocument()
{
    const int doc_id = 0;
    const std::string document_text = "My moms love cat and dogs"s;
    const vector<int> doc_ratings = { 1, 2, 3, 4, 5, 5 };
    const DocumentStatus status = DocumentStatus::ACTUAL;
    const string querry = "moms"s;
    SearchServer server ("none"s);

    ASSERT_EQUAL(server.GetDocumentCount(), 0);
    server.AddDocument(doc_id, document_text, status, doc_ratings);

    ASSERT_EQUAL (server.GetDocumentCount(), 1);
    {
        // Проверка на рейтинг
        const int AVG = 3;

        auto doc = server.FindTopDocuments(querry,status);

        ASSERT_EQUAL(doc.size(), 1);

        ASSERT_EQUAL(doc[0].id, doc_id);

        ASSERT_EQUAL(doc[0].rating, AVG);
    }
    {
        //Проверка на статус

        //const auto [words,docStatus] = server.MatchDocument(querry,doc_id);
        //ASSERT_EQUAL ( docStatus, status);

    }

}

void TestSetStopWord()
{
    const int doc_id = 0;
    const std::string document_text = "My moms love cat and dogs"s;
    const vector<int> doc_ratings = { 1, 2, 3, 4, 5, 5 };
    const DocumentStatus status = DocumentStatus::ACTUAL;
    const string querry = "moms"s;
    SearchServer server ("moms"s);
    server.AddDocument(doc_id, document_text, status, doc_ratings);
    auto findDoc = server.FindTopDocuments(querry);
    ASSERT_EQUAL( findDoc.size(), 0);
}

void TestMinusWordinDocument()
{
    const int doc_id = 0;
    const std::string document_text = "My moms love cat and dogs"s;
    const vector<int> doc_ratings = { 1, 2, 3, 4, 5, 5 };
    const DocumentStatus status = DocumentStatus::ACTUAL;
    const string querry = "moms -cat"s;
    SearchServer server("none"s);
    server.AddDocument(doc_id, document_text, status, doc_ratings);
    auto findDoc = server.FindTopDocuments(querry);
    ASSERT_EQUAL( findDoc.size(), 0);
}
void TestMatchMinusWords()
{
    const int doc_id = 0;
    const std::string document_text = "My moms love cat and dogs"s;
    const vector<int> doc_ratings = { 1, 2, 3, 4, 5, 5 };
    const DocumentStatus status = DocumentStatus::ACTUAL;
    const string querry = "moms -cat"s;
    SearchServer server("none"s);
    server.AddDocument(doc_id, document_text, status, doc_ratings);
    auto [words, statusdoc] = server.MatchDocument(querry,doc_id);
    ASSERT_EQUAL( words.size(), 0);

}

void FindDocStatus()
{
    const int doc_id = 0;
    const std::string document_text = "My moms love cat and dogs"s;
    const vector<int> doc_ratings = { 1, 2, 3, 4, 5, 5 };
    const string querry = "moms"s;
    {
    SearchServer server("none"s);
    const DocumentStatus status = DocumentStatus::ACTUAL;
    server.AddDocument(doc_id, document_text, status, doc_ratings);
    auto doc = server.FindTopDocuments(querry,status);
    ASSERT_EQUAL(doc.size(), 1);
    ASSERT_EQUAL(doc[0].id, 0);
    }
    {
    SearchServer server("none"s);
    const DocumentStatus status = DocumentStatus::BANNED;
    server.AddDocument(doc_id, document_text, status, doc_ratings);
    auto doc = server.FindTopDocuments(querry,status);
    ASSERT_EQUAL(doc.size(), 1);
    ASSERT_EQUAL(doc[0].id, 0);
    }
    {
    SearchServer server("none"s);
    const DocumentStatus status = DocumentStatus::IRRELEVANT;
    server.AddDocument(doc_id, document_text, status, doc_ratings);
    auto doc = server.FindTopDocuments(querry,status);
    ASSERT_EQUAL(doc.size(), 1);
    ASSERT_EQUAL(doc[0].id, 0);
    }
    {
    SearchServer server("none"s);
    const DocumentStatus status = DocumentStatus::REMOVED;
    server.AddDocument(doc_id, document_text, status, doc_ratings);
    auto doc = server.FindTopDocuments(querry,status);
    ASSERT_EQUAL(doc.size(), 1);
    ASSERT_EQUAL(doc[0].id, 0);
    }
}

void TestUSersPredicate()
{
    const DocumentStatus status = DocumentStatus::ACTUAL;

    const int doc_id0 = 0;
    const std::string document_text0 = "My moms love cat and dogs"s;
    const vector<int> doc_ratings0 = { 1, 2, 3, 4, 5, 5 };
    const int AVG0 = 3;

    const int doc_id1 = 1;
    const std::string document_text1 = "Two elephant eat my socks"s;
    const vector<int> doc_ratings1 = { 8, 5, 4};
    const int AVG1 = 5;

    const int doc_id2 = 2;
    const std::string document_text2 = "My brother sleep in the bedroom"s;
    const vector<int> doc_ratings2 = { 7, 1 , 2 , 7};
    const int AVG2 = 4;

    SearchServer server("none"s);

    server.AddDocument(doc_id0,document_text0,status,doc_ratings0);
    server.AddDocument(doc_id1,document_text1,status,doc_ratings1);
    server.AddDocument(doc_id2,document_text2,status,doc_ratings2);

    {
        const string querry = "love"s;
        auto findDoc = server.FindTopDocuments(querry,[doc_id0,status,AVG0](int id, DocumentStatus st, int rating)
        {
            return id == doc_id0 && st == status && rating == AVG0;
        });

        ASSERT_EQUAL(findDoc.size(), 1);
        ASSERT_EQUAL(findDoc[0].id, doc_id0);
        ASSERT_EQUAL(findDoc[0].rating, AVG0);
    }
    {
        const string querry = "eat"s;
        auto findDoc = server.FindTopDocuments(querry,[=](int id, DocumentStatus st, int rating)
        {
            return id == doc_id1 and st == status and rating == AVG1;
        });

        ASSERT_EQUAL(findDoc.size(), 1);
        ASSERT_EQUAL(findDoc[0].id, doc_id1);
        ASSERT_EQUAL(findDoc[0].rating, AVG1);
    }
    {
        const string querry = "sleep"s;
        auto findDoc = server.FindTopDocuments(querry,[=](int id, DocumentStatus st, int rating)
        {
            return id == doc_id2 and st == status and rating == AVG2;
        });

        ASSERT_EQUAL(findDoc.size(), 1);
        ASSERT_EQUAL(findDoc[0].id, doc_id2);
        ASSERT_EQUAL(findDoc[0].rating, AVG2);
    }


}
/*
        Разместите код остальных тестов здесь
        */

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestSetStopWord);
    RUN_TEST(TestMinusWordinDocument);
    RUN_TEST(TestMatchMinusWords);
    RUN_TEST(FindDocStatus);
    RUN_TEST(TestUSersPredicate);
    // Не забудьте вызывать остальные тесты здесь
}

int main() {
    using namespace std;

    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,             //1  2-word
            "funny pet with curly hair"s,           //2  2-word
            "funny pet and not very nasty rat"s,    //3  0-word( stop words)
            "pet with rat and rat and rat"s,        //4  1-word
            "nasty rat with curly hair"s,           //5  2-word
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const string query = "curly and funny rat rat -not"s;

    {
        const auto [words, status] = search_server.MatchDocument(query, 1);
        cout << words.size() << " words for document 1"s << endl;
        // 1 words for document 1
    }

    {
        const auto [words, status] = search_server.MatchDocument(execution::seq, query, 2);
        cout << words.size() << " words for document 2"s << endl;
        // 2 words for document 2
    }

    {
        const auto [words, status] = search_server.MatchDocument(execution::par, query, 3);
        cout << words.size() << " words for document 3"s << endl;
        // 0 words for document 3
    }

        {
            const auto [words, status] = search_server.MatchDocument(execution::par, query, 4);
            cout << words.size() << " words for document 4"s << endl;
            // 0 words for document 4
        }

            {
                const auto [words, status] = search_server.MatchDocument(execution::par, query, 5);
                cout << words.size() << " words for document 5"s << endl;
                // 0 words for document 5
            }




    return 0;
}

