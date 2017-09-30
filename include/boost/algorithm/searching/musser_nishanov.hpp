/* 
   Copyright (c) David R. Musser & Gor V. Nishanov 1997.
   Copyright (c) Jeremy W. Murphy 2016.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org
*/

#ifndef BOOST_ALGORITHM_SEARCH_MUSSER_NISHANOV_HPP
#define BOOST_ALGORITHM_SEARCH_MUSSER_NISHANOV_HPP

#include <boost/algorithm/searching/detail/mn_traits.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>
#include <boost/function.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/or.hpp>
#include <boost/next_prior.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>

#include <iterator>
#include <utility>
#include <vector>


namespace boost { namespace algorithm {

namespace detail {
    
/**
 * @brief Accelerated Linear search.
 * 
 * Accelerated Linear (AL) search by Musser & Nishanov.
 * 
 */
template <typename PatIter, typename CorpusIter>
class accelerated_linear
{
    BOOST_STATIC_ASSERT (( boost::is_same<
    typename std::iterator_traits<PatIter>::value_type, 
    typename std::iterator_traits<CorpusIter>::value_type>::value ));
public:    
    typedef typename std::iterator_traits<PatIter>::difference_type pattern_difference_type;
    typedef typename std::iterator_traits<CorpusIter>::difference_type corpus_difference_type;
    
protected:
    PatIter pat_first, pat_last;
    std::vector<corpus_difference_type> next_;
    pattern_difference_type k_pattern_length;

private:
    void compute_next()
    {
        BOOST_ASSERT(k_pattern_length > 0);
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

public:
    accelerated_linear(PatIter pat_first, PatIter pat_last) : pat_first(pat_first), pat_last(pat_last), k_pattern_length(std::distance(pat_first, pat_last))
    {
        if (k_pattern_length > 0)
            compute_next();
    }
    
    std::pair<CorpusIter, CorpusIter> operator()(CorpusIter corpus_first, CorpusIter corpus_last) const
    {
        BOOST_ASSERT(std::distance(pat_first, pat_last) == k_pattern_length);
        BOOST_ASSERT(size_t(k_pattern_length) == next_.size());

        using std::find;
        using std::make_pair;
        
        if (pat_first == pat_last)
            return make_pair(corpus_first, corpus_first);            
        
        if (next_.size() == 1)
        {
            CorpusIter const result = find(corpus_first, corpus_last, *pat_first);
            return result == corpus_last ? make_pair(corpus_last, corpus_last) : make_pair(result, boost::next(result));
        }
        
        PatIter p1 = pat_first;
        ++p1;
        
        while (corpus_first != corpus_last)
        {
            corpus_first = find(corpus_first, corpus_last, *pat_first);
            if (corpus_first == corpus_last)
                return make_pair(corpus_last, corpus_last);
            CorpusIter hold = corpus_first;
            if (++corpus_first == corpus_last)
                return make_pair(corpus_last, corpus_last);
            PatIter p = p1;
            pattern_difference_type j = 1;
            while (*corpus_first == *p)
            {
                if (++p == pat_last)
                    return make_pair(hold, boost::next(hold, k_pattern_length));
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
                        return make_pair(hold, boost::next(hold, k_pattern_length));
                    }
                    if (corpus_first == corpus_last)
                        return make_pair(corpus_last, corpus_last);
                }
            }
        }
        return make_pair(corpus_last, corpus_last);
    }
    
    template <typename Range>
    std::pair<CorpusIter, CorpusIter> operator()(const Range &corpus) const
    {
        return (*this)(boost::begin(corpus), boost::end(corpus));
    }    
};


/**
 * Musser-Nishanov Hashed Accelerated Linear (HAL) search algorithm.
 */
template <typename PatIter, typename CorpusIter, typename Trait>
class hashed_accelerated_linear : private boost::algorithm::detail::accelerated_linear<PatIter, CorpusIter>
{
    typedef boost::algorithm::detail::accelerated_linear<PatIter, CorpusIter> AcceleratedLinear;
    
    using typename AcceleratedLinear::pattern_difference_type;
    using typename AcceleratedLinear::corpus_difference_type;
    using AcceleratedLinear::k_pattern_length;
    using AcceleratedLinear::pat_first;
    using AcceleratedLinear::pat_last;
    using AcceleratedLinear::next_;
    
    typedef boost::array<corpus_difference_type, Trait::hash_range_max> skip_container;
    typedef boost::function<std::pair<CorpusIter, CorpusIter>(CorpusIter, CorpusIter)> search_function;

    skip_container skip_;
    corpus_difference_type mismatch_shift;
    search_function search;
    
    std::pair<CorpusIter, CorpusIter> nul_pattern(CorpusIter corpus_first, CorpusIter) const
    {
        return std::make_pair(corpus_first, corpus_first);
    }    
    

    // These HAL(first, last) overloads are just to differentiate const and non-const.
   
    std::pair<CorpusIter, CorpusIter> HAL(CorpusIter corpus_first, CorpusIter corpus_last) const
    {
        // Make a copy so that the real HAL can modify it.
        skip_container skip(skip_);
        return HAL(corpus_first, corpus_last, skip.begin());
    }
    

    std::pair<CorpusIter, CorpusIter> HAL(CorpusIter corpus_first, CorpusIter corpus_last)
    {
        // Go ahead and modify the object.
        return HAL(corpus_first, corpus_last, skip_.begin());
    }
    
    
    //This is the real HAL algorithm.
    template <typename I>
    std::pair<CorpusIter, CorpusIter> HAL(CorpusIter corpus_first, CorpusIter corpus_last, I skip) const
    {
        BOOST_CONCEPT_ASSERT((boost::Mutable_RandomAccessIterator<I>));
        
        BOOST_ASSERT(pat_first != pat_last);
        BOOST_ASSERT(std::distance(pat_first, pat_last) == k_pattern_length);
        BOOST_ASSERT(size_t(k_pattern_length) == next_.size());

        using std::make_pair;
        
        corpus_difference_type const k_corpus_length = corpus_last - corpus_first;
        corpus_difference_type const adjustment = k_corpus_length + k_pattern_length;
        // NOTE: This assignment requires the skip iterator to be mutable, and
        // the implementation would be greatly simplified if a way around it could be found.
        skip[Trait::hash(pat_first + k_pattern_length - 1)] = k_corpus_length + 1;
        corpus_difference_type k = -k_corpus_length;
        for (;;)
        {
            k += k_pattern_length - 1;
            if (k >= 0) break;
            do   // this loop is hot for data read
            {
                corpus_difference_type const increment = skip[Trait::hash(corpus_last + k)];
                k += increment;
            }
            while (k < 0);
            if (k < k_pattern_length)
                return make_pair(corpus_last, corpus_last);
            k -= adjustment;
            
            BOOST_ASSERT(k < 0);
            if (corpus_last[k] != pat_first[0])
                k += mismatch_shift;
            else
            {
                pattern_difference_type j = 1;
                for (;;)
                {
                    ++k;
                    if (corpus_last[k] != pat_first[j])
                        break;
                    ++j;
                    if (j == k_pattern_length)
                        return std::make_pair(corpus_last + k - k_pattern_length + 1, corpus_last + k + 1);
                }
                
                if (mismatch_shift > j)
                    k += mismatch_shift - j;
                else
                    
                    for (;;)
                    {
                        j = next_[j];
                        if (j < 0)
                        {
                            ++k;
                            break;
                        }
                        if (j == 0)
                            break;
                        while (corpus_last[k] == pat_first[j])
                        {
                            ++k;
                            ++j;
                            if (j == k_pattern_length)
                            {
                                return make_pair(corpus_last + k - k_pattern_length, corpus_last + k);
                            }
                            if (k == 0)
                                return make_pair(corpus_last, corpus_last);
                        }
                    }
            }
        }
        return make_pair(corpus_last, corpus_last);
    }
    
    
    void compute_skip()
    {
        BOOST_ASSERT(next_.size() >= Trait::suffix_size);
        
        pattern_difference_type const m = next_.size();
        std::fill(skip_.begin(), skip_.end(), m - Trait::suffix_size + 1);
        for (pattern_difference_type j = Trait::suffix_size - 1; j < m - 1; ++j)
            skip_[Trait::hash(pat_first + j)] = m - 1 - j;
        mismatch_shift = skip_[Trait::hash(pat_first + m - 1)];
        skip_[Trait::hash(pat_first + m - 1)] = 0;
    }
    
    
    std::pair<CorpusIter, CorpusIter> AL(CorpusIter corpus_first, CorpusIter corpus_last) const
    {
        return AcceleratedLinear::operator()(corpus_first, corpus_last);
    }
    
    // Choose the const or non-const HAL search function.
    search_function HAL_function() const
    {
        return bind(static_cast<std::pair<CorpusIter, CorpusIter>(hashed_accelerated_linear::*)(CorpusIter, CorpusIter) const>(&hashed_accelerated_linear::HAL), this, _1, _2);
    }

    search_function HAL_function()
    {
        return bind(static_cast<std::pair<CorpusIter, CorpusIter>(hashed_accelerated_linear::*)(CorpusIter, CorpusIter)>(&hashed_accelerated_linear::HAL), this, _1, _2);
    }
    
public:
    using AcceleratedLinear::operator();

    hashed_accelerated_linear(PatIter pat_first, PatIter pat_last) : AcceleratedLinear(pat_first, pat_last)
    {
        if (k_pattern_length > 0)
        {
            if (k_pattern_length < Trait::suffix_size || k_pattern_length == 1)
                search = bind(&hashed_accelerated_linear::AL, this, _1, _2);
            else
            {
                search = HAL_function();
                compute_skip();
            }
        }
        else
            search = bind(&hashed_accelerated_linear::nul_pattern, this, _1, _2);
    }
    
    std::pair<CorpusIter, CorpusIter> operator()(CorpusIter corpus_first, CorpusIter corpus_last)
    {
        return search(corpus_first, corpus_last);
    }
};

} // namespace detail


/**
 * One class, two identities based on corpus iterator and the suffix size trait.
 */
template <typename PatIter, typename CorpusIter = PatIter, typename Trait = search_trait<typename std::iterator_traits<PatIter>::value_type>, typename Enable = void>
class musser_nishanov;


/**
 * Musser-Nishanov Accelerated Linear search algorithm.
 */
template <typename PatIter, typename CorpusIter, typename Trait>
class musser_nishanov<PatIter, CorpusIter, Trait, 
typename disable_if<
    typename boost::mpl::and_<
        boost::is_base_of<std::random_access_iterator_tag, typename std::iterator_traits<CorpusIter>::iterator_category>,
        boost::mpl::bool_<Trait::suffix_size>
    >::type 
>::type> : public boost::algorithm::detail::accelerated_linear<PatIter, CorpusIter>
{
    typedef boost::algorithm::detail::accelerated_linear<PatIter, CorpusIter> AcceleratedLinear;
    
public:
    musser_nishanov(PatIter pat_first, PatIter pat_last) : AcceleratedLinear(pat_first, pat_last) {}
};


/**
 * Musser-Nishanov Hashed Accelerated Linear search algorithm.
 */
template <typename PatIter, typename CorpusIter, typename Trait>
class musser_nishanov<PatIter, CorpusIter, Trait, 
typename enable_if<
    typename boost::mpl::and_<
        boost::is_base_of<std::random_access_iterator_tag, typename std::iterator_traits<CorpusIter>::iterator_category>,
        boost::mpl::bool_<Trait::suffix_size> 
    >::type 
>::type> : public boost::algorithm::detail::hashed_accelerated_linear<PatIter, CorpusIter, Trait>
{
    typedef boost::algorithm::detail::hashed_accelerated_linear<PatIter, CorpusIter, Trait> HashedAcceleratedLinear;
public:
    musser_nishanov(PatIter pat_first, PatIter pat_last) : HashedAcceleratedLinear(pat_first, pat_last) {}
};


template <typename PatIter, typename CorpusIter>
std::pair<CorpusIter, CorpusIter> musser_nishanov_search(CorpusIter corpus_first, CorpusIter corpus_last, PatIter pat_first, PatIter pat_last)
{
    musser_nishanov<PatIter, CorpusIter> mn(pat_first, pat_last);
    return mn(corpus_first, corpus_last);
}


template <typename PatternRange, typename CorpusIter>
std::pair<CorpusIter, CorpusIter> musser_nishanov_search(CorpusIter corpus_first, CorpusIter corpus_last, const PatternRange &pattern)
{
    typedef typename boost::range_iterator<const PatternRange>::type pattern_iterator;
    musser_nishanov<pattern_iterator, CorpusIter> mn(boost::begin(pattern), boost::end(pattern));
    return mn(corpus_first, corpus_last);
}


template <typename patIter, typename CorpusRange>
typename boost::disable_if_c<
    boost::is_same<CorpusRange, patIter>::value, 
    std::pair<typename boost::range_iterator<CorpusRange>::type, typename boost::range_iterator<CorpusRange>::type> >
::type
musser_nishanov_search ( CorpusRange &corpus, patIter pat_first, patIter pat_last )
{
    typedef typename boost::range_iterator<const CorpusRange>::type corpus_iterator;
    musser_nishanov<patIter, corpus_iterator> mn(pat_first, pat_last);
    return mn(boost::begin(corpus), boost::end(corpus));
}


template <typename PatternRange, typename CorpusRange>
std::pair<typename boost::range_iterator<CorpusRange>::type, typename boost::range_iterator<CorpusRange>::type>
musser_nishanov_search ( CorpusRange &corpus, const PatternRange &pattern )
{
    typedef typename boost::range_iterator<const PatternRange>::type pattern_iterator;
    typedef typename boost::range_iterator<const CorpusRange>::type corpus_iterator;
    musser_nishanov<pattern_iterator, corpus_iterator> mn(boost::begin(pattern), boost::end(pattern));
    return mn(boost::begin(corpus), boost::end(corpus));
}


//  Creator functions -- take a pattern range, return an object
template <typename Range>
musser_nishanov<typename boost::range_iterator<const Range>::type>
make_musser_nishanov(const Range &r) {
    typedef typename boost::range_iterator<const Range>::type pattern_iterator;
    return musser_nishanov<pattern_iterator>(boost::begin(r), boost::end(r));
}


// This overload permits specification of the corpus iterator type.
template <typename PatternRange, typename CorpusRange>
musser_nishanov<typename boost::range_iterator<const PatternRange>::type, typename boost::range_iterator<const CorpusRange>::type>
make_musser_nishanov(const PatternRange &r, const CorpusRange &) {
    typedef typename boost::range_iterator<const PatternRange>::type pattern_iterator;
    typedef typename boost::range_iterator<const CorpusRange>::type corpus_iterator;
    return musser_nishanov<pattern_iterator, corpus_iterator>(boost::begin(r), boost::end(r));
}


// This overload permits specification of corpus iterator and search trait class.
template <typename PatternRange, typename CorpusRange, typename Trait>
musser_nishanov<typename boost::range_iterator<const PatternRange>::type, typename boost::range_iterator<const CorpusRange>::type>
make_musser_nishanov(const PatternRange &r, const CorpusRange &) {
    typedef typename boost::range_iterator<const PatternRange>::type pattern_iterator;
    typedef typename boost::range_iterator<const CorpusRange>::type corpus_iterator;
    return musser_nishanov<pattern_iterator, corpus_iterator, Trait>(boost::begin(r), boost::end(r));
}

}} // namespace boost::algorithm

#endif
