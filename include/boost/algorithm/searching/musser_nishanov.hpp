#ifndef BOOST_ALGORITHM_SEARCH_MUSSER_NISHANOV_HPP
#define BOOST_ALGORITHM_SEARCH_MUSSER_NISHANOV_HPP

#include <boost/algorithm/searching/detail/musser_nishanov_HAL.hpp>
#include <boost/algorithm/searching/detail/mn_traits.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/next_prior.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/utility/enable_if.hpp>

#include <iterator>
#include <utility>
#include <vector>


namespace boost { namespace algorithm {

/**
 * One class, two identities.
 */
template <typename PatIter, typename CorpusIter = PatIter, typename Trait = search_trait<typename std::iterator_traits<PatIter>::value_type>, typename Enable = void>
class musser_nishanov;

/**
 * Musser-Nishanov Accelerated Linear search algorithm.
 */
template <typename PatIter, typename CorpusIter, typename Trait>
class musser_nishanov<PatIter, CorpusIter, Trait, 
typename disable_if<is_base_of<std::random_access_iterator_tag, typename std::iterator_traits<CorpusIter>::iterator_category> >::type>
{
    BOOST_STATIC_ASSERT (( boost::is_same<
    typename std::iterator_traits<PatIter>::value_type, 
    typename std::iterator_traits<CorpusIter>::value_type>::value ));
    
    typedef typename std::iterator_traits<PatIter>::difference_type pattern_difference_type;
    typedef typename std::iterator_traits<CorpusIter>::difference_type corpus_difference_type;
    
    PatIter pat_first, pat_last;
    std::vector<corpus_difference_type> next_;
    pattern_difference_type k_pattern_length;
    
    void compute_next()
    {
        pattern_difference_type j = 0, t = -1;
        next_.reserve(k_pattern_length);
        next_.push_back(-1);
        while (j < k_pattern_length - 1)
        {
            while (t >= 0 && pat_first[j] != pat_first[t])
                t = next_[t];
            ++j;
            ++t;
            next_.push_back(pat_first[j] == pat_first[t] ? next_[t] : t);
        }
    }

    std::pair<CorpusIter, CorpusIter> AL(CorpusIter corpus_first, CorpusIter corpus_last) const
    {
        using std::find;
        using std::make_pair;
        
        if (pat_first == pat_last)
            return make_pair(corpus_first, corpus_first);            
        
        PatIter p1;

        if (next_.size() == 1)
        {
            CorpusIter const result = find(corpus_first, corpus_last, *pat_first);
            return result == corpus_last ? make_pair(corpus_last, corpus_last) : make_pair(result, next(result));
        }
        p1 = pat_first;
        ++p1;
        while (corpus_first != corpus_last)
        {
            corpus_first = find(corpus_first, corpus_last, *pat_first);
            if (corpus_first == corpus_last)
                return make_pair(corpus_last, corpus_last);
            PatIter p = p1;
            pattern_difference_type j = 1;
            CorpusIter hold = corpus_first;
            if (++corpus_first == corpus_last)
                return make_pair(corpus_last, corpus_last);
            while (*corpus_first == *p)
            {
                if (++p == pat_last)
                    return make_pair(hold, next(hold, k_pattern_length));
                if (++corpus_first == corpus_last)
                    return make_pair(corpus_last, corpus_last);
                ++j;
            }
            
            for (;;)
            {
                j = next_[j];
                if (j < 0)
                {
                    ++corpus_first;
                    break;
                }
                if (j == 0)
                    break;
                p = pat_first + j;
                while (*corpus_first == *p)
                {
                    ++corpus_first;
                    ++p;
                    ++j;
                    if (p == pat_last)
                    {
                        CorpusIter succesor = hold;
                        std::advance(succesor, next_.size());
                        while (succesor != corpus_first)
                            ++succesor, ++hold;
                        return make_pair(hold, next(hold, k_pattern_length));
                    }
                    if (corpus_first == corpus_last)
                        return make_pair(corpus_last, corpus_last);
                }
            }
        }
        return make_pair(corpus_last, corpus_last);
    }
    
public:
    musser_nishanov(PatIter pat_first, PatIter pat_last) : pat_first(pat_first), pat_last(pat_last), k_pattern_length(std::distance(pat_first, pat_last))
    {
        if (k_pattern_length > 0)
            compute_next();
    }

    /**
     * Run the search object on a corpus with forward or bidirectional iterators.
     */
    std::pair<CorpusIter, CorpusIter>
    operator()(CorpusIter corpus_first, CorpusIter corpus_last) const
    {
        return AL(corpus_first, corpus_last);
    }
};


/**
 * Musser-Nishanov Hashed Accelerated Linear search algorithm.
 */
template <typename PatIter, typename CorpusIter, typename Trait>
class musser_nishanov<PatIter, CorpusIter, Trait, 
typename enable_if<is_base_of<std::random_access_iterator_tag, typename std::iterator_traits<CorpusIter>::iterator_category> >::type>
{
    BOOST_STATIC_ASSERT (( boost::is_same<
    typename std::iterator_traits<PatIter>::value_type, 
    typename std::iterator_traits<CorpusIter>::value_type>::value ));
    
    typedef typename std::iterator_traits<PatIter>::difference_type pattern_difference_type;
    typedef typename std::iterator_traits<CorpusIter>::difference_type corpus_difference_type;

    PatIter pat_first, pat_last;
    std::vector<corpus_difference_type> next;
    boost::array<corpus_difference_type, Trait::hash_range_max> skip;
    pattern_difference_type k_pattern_length;
    corpus_difference_type mismatch_shift;
    boost::function<std::pair<CorpusIter, CorpusIter>(CorpusIter, CorpusIter)> search;
    
    /**
     * Called the first time a search object is run on a corpus with random-access iterators.
     * This means that the skip table is only calculated if it is required.
     */
    std::pair<CorpusIter, CorpusIter> HAL_initialize(CorpusIter corpus_first, CorpusIter corpus_last)
    {
        search = bind(&musser_nishanov::HAL, *this, _1, _2);
        compute_skip();
        return HAL(corpus_first, corpus_last);
    }
    
    std::pair<CorpusIter, CorpusIter> HAL(CorpusIter corpus_first, CorpusIter corpus_last)
    {
        return std::make_pair(corpus_first, corpus_last);
    }
    
    std::pair<CorpusIter, CorpusIter> AL(CorpusIter corpus_first, CorpusIter corpus_last)
    {
        return std::make_pair(corpus_first, corpus_last);
    }

    void compute_next()
    {
        pattern_difference_type j = 0, t = -1;
        next.reserve(k_pattern_length);
        next.push_back(-1);
        while (j < k_pattern_length - 1)
        {
            while (t >= 0 && pat_first[j] != pat_first[t])
                t = next[t];
            ++j;
            ++t;
            next.push_back(pat_first[j] == pat_first[t] ? next[t] : t);
        }
    }
    
    void compute_skip()
    {
        pattern_difference_type const m = next.size();
        std::fill(skip.begin(), skip.end(), m - Trait::suffix_size + 1);
        for (pattern_difference_type j = Trait::suffix_size - 1; j < m - 1; ++j)
        {
            // unsigned char const index = Trait::hash(pat_first + j);
            skip[Trait::hash(pat_first + j)] = m - 1 - j;
        }
        mismatch_shift = skip[Trait::hash(pat_first + m - 1)];
        skip[Trait::hash(pat_first + m - 1)] = 0;
    }
    
public:
    musser_nishanov(PatIter pat_first, PatIter pat_last) : pat_first(pat_first), pat_last(pat_last), k_pattern_length(std::distance(pat_first, pat_last))
    {
        if (Trait::suffix_size == 0 || k_pattern_length < Trait::suffix_size)
            search = bind(&musser_nishanov::AL, *this, _1, _2);
        else
            search = bind(&musser_nishanov::HAL_initialize, *this, _1, _2);
        if (k_pattern_length > 0)
            compute_next();
    }
    
    /**
     * Run the search object on a corpus with random-access iterators.
     */
    template <typename I>
    typename enable_if<is_base_of<std::random_access_iterator_tag, typename std::iterator_traits<I>::iterator_category>, std::pair<I, I> >::type
    operator()(I corpus_first, I corpus_last)
    {
        return search(corpus_first, corpus_last);
    }
    
};

}} // namespace boost::algorithm

#endif
