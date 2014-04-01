//  (C) Copyright Jeremy W. Murphy 2013.
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/algorithm/integer_sort.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/random.hpp>
#include <boost/timer/timer.hpp>
#include <boost/bind.hpp>

#include <iostream>
#include <vector>
#include <functional>
#include <algorithm>
#include <string>
#include <locale>
#include <iomanip>
#include <typeinfo>
#include <cmath>

using boost::lexical_cast;
using namespace boost::random;
using namespace boost::algorithm;


template <typename T>
struct foo
{
    static mt19937 rng;
    T key;
    std::string s;
    foo() : key(T()), s(std::string()) {}
    foo(T const &key, std::string const &s) : key(key), s(s) {}
    foo(T const &key, char const *s) : key(key), s(std::string(s)) {}
    foo(T const &key) : key(key), s("satellite") {}
    /*
    // Although this particular foo is slower, it is better for testing.
    foo() : key(T()), d(rng()) {}
    foo(T const &key) : key(key), d(rng()) {}
    U d;
    */
    
    
    friend
    bool operator<(foo const &a, foo const b)
    {
        return a.key < b.key;
    }
    
    
    friend
    bool operator==(foo const &a, foo const &b)
    {
        return a.key == b.key && a.s == b.s;
    }
    
    
    struct foo_key
    {
        typedef T result_type;
        
        T operator()(foo const &x) const
        {
            return x.key;
        }
    };
};

template <typename T>
mt19937 foo<T>::rng = mt19937();

template <typename T, class Distribution>
void test(Distribution dist, unsigned const seed = 0, unsigned const max2 = 26)
{
    // typedef typename std::vector<T>::iterator iterator;
    typedef T value_type;
    typedef identity<T> converter;
    typedef typename std::vector<value_type>::const_iterator const_iterator;

    std::cout << "=== Test (seed = " << seed << ", T = " << typeid(T).name() << "). ===" << std::endl;
    mt19937 rng(seed);
    converter const conv;

    std::cout << "log2(n)\tt/n (ns)\tc\tx sort\tx stable_sort\n";
    std::cout << "-------------------------------------------------------------\n";
    for(unsigned p = 16; p <= max2; p++)
    {
        std::size_t const n(1ul << p);
        std::vector<value_type> A;
        A.reserve(n);
        std::generate_n(std::back_inserter(A), n, boost::bind(dist, rng));
        std::vector<value_type> B(A);
        const_iterator const    _min = std::min_element(A.begin(), A.end()), 
                                _max = std::max_element(A.begin(), A.end());
        std::cout << std::setw(7) << p << "\t";
        std::cout.flush();
        std::vector<value_type> X(A);
        boost::timer::cpu_timer timer;
        stable_radix_sort(A.begin(), A.end(), B.begin(), conv, conv(*_min), conv(*_max));
        boost::timer::cpu_times const t1 = timer.elapsed();
        std::sort(X.begin(), X.end());
        boost::timer::cpu_times const t2 = timer.elapsed();
        std::stable_sort(A.begin(), A.end());
        boost::timer::cpu_times const t3 = timer.elapsed();
        timer.stop();
        boost::timer::nanosecond_type const     t_radix((t1.system + t1.user));
        if(t_radix != 0)
        {
            boost::timer::nanosecond_type const t_sort(t2.system + t2.user - t_radix),
                                                t_stable_sort(t3.system + t3.user - t2.system - t2.user);
            std::cout.precision(2);
            std::cout.setf(std::ios::fixed);
            std::cout << std::setw(8) << t_radix / static_cast<double>(n) << "\t";
            std::cout << t_radix / n / log2(n) << "\t";
            std::cout.precision(1);
            std::cout << static_cast<double>(t_sort) / t_radix << "\t";
            std::cout << static_cast<double>(t_stable_sort) / t_radix;
        }
        std::cout << std::endl;
    }
}



int main(int argc, char **argv)
{
    // Configure cout to my liking.
    std::ios_base::sync_with_stdio(false);
    std::locale::global(std::locale(""));
    std::cout.imbue(std::locale());
    
    unsigned const seed(argc < 2 ? std::time(NULL) : lexical_cast<unsigned>(argv[1]));
    
    std::cout << "\n*** poisson_distribution, mean = std::numeric_limits<T>::max() / 2" << std::endl;
    test<unsigned char>(poisson_distribution<unsigned char>(std::numeric_limits<unsigned char>::max() / 2), seed, 28);
    test<unsigned short>(poisson_distribution<unsigned short>(std::numeric_limits<unsigned short>::max() / 2), seed, 28);
    test<unsigned int>(poisson_distribution<unsigned int>(std::numeric_limits<unsigned int>::max() / 2), seed);
    test<unsigned long>(poisson_distribution<unsigned long>(std::numeric_limits<unsigned long>::max() / 2), seed);
    
    std::cout << "\n*** uniform_int_distribution, min = 0, k = std::numeric_limits<T>::max()" << std::endl;
    test<unsigned char>(uniform_int_distribution<unsigned char>(0, std::numeric_limits<unsigned char>::max()), seed, 28);
    test<unsigned short>(uniform_int_distribution<unsigned short>(0, std::numeric_limits<unsigned short>::max()), seed, 28);
    test<unsigned int>(uniform_int_distribution<unsigned int>(0, std::numeric_limits<unsigned int>::max()), seed);
    test<unsigned long>(uniform_int_distribution<unsigned long>(0, std::numeric_limits<unsigned long>::max()), seed);
    return 0;
}
