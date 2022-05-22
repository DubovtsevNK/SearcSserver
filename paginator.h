#pragma once

#include <vector>
#include <iostream>

template <class It>
class IteratorRange
{
public:
    IteratorRange(It begin, It end) :
        beginIt(begin),endIt(end), lenght(distance(begin,end))
    {

    }

    auto begin() const
    {
        return beginIt;
    }
    It end() const
    {
        return endIt;
    }
    size_t Size() const
    {
        return lenght;
    }
private:
    It beginIt;
    It endIt;
    size_t lenght;
};

template< class It>
class Paginator
{
public:
   Paginator(It begin, It end, size_t len)
    {
        size_t all_Len = std::distance(begin,end);
        if(all_Len > len)
        {
            It tmpp_gBegin = begin;
            It tmpp_gEnd = begin;
            while(all_Len > len)
            {
                std::advance(tmpp_gEnd, len);
                vec_Page.push_back({tmpp_gBegin,tmpp_gEnd});
                std::advance(tmpp_gBegin, len);
                all_Len = std::distance(tmpp_gEnd,end);
            }
            vec_Page.push_back({tmpp_gBegin,end});
        }
        else
        {
            vec_Page.push_back({begin,end});
        }

    }

    auto begin() const {
        return vec_Page.begin();
    }

    auto end() const {
        return vec_Page.end();
    }

    size_t size() const {
        return vec_Page.size();
    }
private:
    std::vector<IteratorRange<It>> vec_Page;


};


template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template <typename It>
std::ostream& operator<<(std::ostream& out, const IteratorRange<It>& range) {
    for (It it = range.begin(); it != range.end(); ++it) {
        out << *it;
    }
    return out;
}

