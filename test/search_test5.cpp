/* 
   Copyright (c) Marshall Clow 2010-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org
*/

#include <boost/algorithm/searching/boyer_moore.hpp>
#include <boost/algorithm/searching/boyer_moore_horspool.hpp>
#include <boost/algorithm/searching/knuth_morris_pratt.hpp>
#include <boost/algorithm/searching/musser_nishanov_HAL.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <ctime>        // for clock_t
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <vector>

#include <boost/filesystem.hpp>

typedef std::vector<char> vec;
#define NUM_TRIES   100

#define runOne(call, refDiff)   { \
    std::clock_t bTime, eTime;                              \
    bTime = std::clock ();                                  \
    for ( i = 0; i < NUM_TRIES; ++i ) {                     \
        res = boost::algorithm::call                        \
            ( haystack.begin (), haystack.end (),           \
                        needle.begin (), needle.end ());    \
        if ( res != exp ) {                                 \
            std::cout << "On run # " << i << " expected "   \
                << exp.first - haystack.begin () << " got "       \
                << res.first - haystack.begin () << std::endl;    \
            throw std::runtime_error                        \
                ( "Unexpected result from " #call );        \
            }                                               \
        }                                                   \
    eTime = std::clock ();                                  \
    printRes ( #call, eTime - bTime, refDiff ); }

#define runHALObject(obj, trait, refDiff) { \
    std::clock_t bTime, eTime;                              \
    bTime = std::clock ();                                  \
    boost::algorithm::obj<vec::const_iterator, vec::const_iterator, trait >             \
                s_o ( needle.begin (), needle.end ());      \
    for ( i = 0; i < NUM_TRIES; ++i ) {                     \
        res = s_o ( haystack.begin (), haystack.end ());    \
        if ( res != exp ) {                                 \
            std::cout << "On run # " << i << " expected "   \
            << exp.first - haystack.begin () << " got "           \
            << res.first - haystack.begin () << std::endl;        \
            throw std::runtime_error                        \
            ( "Unexpected result from " #obj " object" );   \
            }                                               \
        }                                                   \
    eTime = std::clock ();                                  \
    printRes ( #obj " object", eTime - bTime, refDiff ); }
    
    #define runObject(obj, refDiff) { \
    std::clock_t bTime, eTime;                              \
    bTime = std::clock ();                                  \
    boost::algorithm::obj <vec::const_iterator>             \
    s_o ( needle.begin (), needle.end ());      \
    for ( i = 0; i < NUM_TRIES; ++i ) {                     \
        res = s_o ( haystack.begin (), haystack.end ());    \
        if ( res != exp ) {                                 \
            std::cout << "On run # " << i << " expected "   \
            << exp.first - haystack.begin () << " got "           \
            << res.first - haystack.begin () << std::endl;        \
            throw std::runtime_error                        \
            ( "Unexpected result from " #obj " object" );   \
        }                                               \
    }                                                   \
    eTime = std::clock ();                                  \
    printRes ( #obj " object", eTime - bTime, refDiff ); }
    

namespace {

    vec ReadFromFile ( const char *name ) {
        std::ifstream in ( name, std::ios_base::binary | std::ios_base::in );
        vec retVal;
        std::string temp;
        /*
        std::istream_iterator<char, char> begin(in);
        std::istream_iterator<char, char> end;
        
        std::copy ( begin, end, std::back_inserter ( retVal ));
        */
        std::getline(in, temp);
        std::copy(temp.begin(), temp.end(), back_inserter(retVal));
        return retVal;
        }
    
    void printRes ( const char *prompt, unsigned long diff, unsigned long stdDiff ) {
        std::cout 
            << std::setw(34) << prompt << " "
            << std::setw(6)  << (  1.0 * diff) / CLOCKS_PER_SEC << " seconds\t"
            << std::setw(5)  << (100.0 * diff) / stdDiff << "% \t" 
            << std::setw(12) << diff;
        if ( diff > stdDiff ) 
            std::cout << " !!";
        std::cout << std::endl;
        }
    
    void check_one ( const vec &haystack, const vec &needle, int expected ) {
        std::size_t i;
        std::clock_t sTime;
        unsigned long stdDiff;
        
        std::pair<vec::const_iterator, vec::const_iterator> res;
        std::pair<vec::const_iterator, vec::const_iterator> exp;        // the expected result
        vec::const_iterator exp_start;
        
        if ( expected >= 0 )
            exp_start = haystack.begin () + expected;
        else if ( expected == -1 )
            exp_start = haystack.end ();      // we didn't find it!
        else if ( expected == -2 )
            exp_start = std::search ( haystack.begin (), haystack.end (), needle.begin (), needle.end ());
        else    
            throw std::logic_error ( "Expected must be -2, -1, or >= 0" );

		if ( expected == -1 )
			exp = std::make_pair(haystack.end(), haystack.end());
		else
			exp = std::make_pair(exp_start, exp_start + needle.size());
			
        std::cout << "Pattern is " << needle.size ()   << " entries long" << std::endl;
        std::cout << "Corpus  is " << haystack.size () << " entries long" << std::endl;

    //  First, the std library search
        sTime = std::clock ();
        for ( i = 0; i < NUM_TRIES; ++i ) {
            vec::const_iterator s_res = std::search ( haystack.begin (), haystack.end (), needle.begin (), needle.end ());
            if ( s_res != exp.first ) {
                std::cout << "On run # " << i << " expected " << exp.first - haystack.begin () << " got " << s_res - haystack.begin () << std::endl;
                throw std::runtime_error ( "Unexpected result from std::search" );
                }
            }
        stdDiff = std::clock () - sTime;
        printRes ( "std::search", stdDiff, stdDiff );

        runOne    ( boyer_moore_search,          stdDiff );
        runObject ( boyer_moore,                 stdDiff );
        runOne    ( boyer_moore_horspool_search, stdDiff );
        runObject ( boyer_moore_horspool,        stdDiff );
        runOne    ( knuth_morris_pratt_search,   stdDiff );
        runObject ( knuth_morris_pratt,          stdDiff );
        runOne    ( musser_nishanov_HAL_search,   stdDiff );
        runHALObject ( musser_nishanov_HAL, boost::algorithm::search_trait<typename std::iterator_traits<vec::const_iterator>::value_type>, stdDiff );
        runHALObject ( musser_nishanov_HAL, boost::algorithm::search_trait_dna2, stdDiff );
        runHALObject ( musser_nishanov_HAL, boost::algorithm::search_trait_dna3, stdDiff );
        runHALObject ( musser_nishanov_HAL, boost::algorithm::search_trait_dna4, stdDiff );
    }
    }

BOOST_AUTO_TEST_CASE( test_main )
{
    using namespace boost::filesystem;
    
    vec c1  = ReadFromFile ( "search_test_data/DNA/0001.corpus" );    
    std::cout << std::ios::fixed << std::setprecision(4);
    
    path data = path("search_test_data/DNA");
    for (directory_iterator dirent = directory_iterator(data); dirent != directory_iterator(); ++dirent)
    {
        if (dirent->path().extension() != ".pat")
            continue;
        unsigned m = atoi(dirent->path().stem().c_str());
        std::cout << "--- " << m << " ---" << std::endl;
        vec pat = ReadFromFile(dirent->path().c_str());
        check_one ( c1, pat, -1 );
    }    
}
