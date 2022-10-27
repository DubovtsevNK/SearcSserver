# SearchServer
Про
Сервер поиска документов, позволяющий находить документы с ранжированием их в соответствии с запросом пользователя.

Основные функции сервера:
- AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings) - добавляет документ в базу данных

- RemoveDocument(int document_id) - удаляет документ из базы данных

- FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) - Поиск документы в соответствии с запросом(rawquery). Дополнительный параметр поиска (doc)
- MatchDocument(std::string_view raw_query, int document_id) - Определяет слова в документе, которые соответствуют запросу пользователя.

RemoveDocument, FindTopDocuments, MatchDocument могут выполняться в последовательном или параллельном режиме.
