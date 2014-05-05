//  (C) Copyright Jeremy W. Murphy 2013.
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/algorithm/integer_sort.hpp>

#include <boost/lexical_cast.hpp>

#include <chrono>
#include <iostream>
#include <vector>
#include <functional>
#include <algorithm>
#include <string>
#include <locale>
#include <iomanip>
#include <typeinfo>
#include <cmath>
#include <random>

using namespace std;

using boost::lexical_cast;
using namespace boost::algorithm;


template <typename T>
struct foo
{
    T key;
    double d;
    foo() : key(T()), d(double()) {}
    foo(T x) : key(x), d(double()) {}

    operator T() const
    {
        return key;
    }
    
    
    friend
    bool operator<(foo const &a, foo const b)
    {
        return a.key < b.key;
    }
    
    
    friend
    bool operator==(foo const &a, foo const &b)
    {
        return a.key == b.key;
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


template <typename F, typename I>
chrono::duration<double, nano> time_sort(F sort, I first, I last, char unsigned logn, unsigned max_logn)
{
    auto const reps = 1ul << (max_logn - logn);
    vector<typename iterator_traits<I>::value_type> buffer(first, last);
    chrono::duration<double, nano> elapsed;
    for(long unsigned i = 0; i < reps; i++)
    {
        auto const start = chrono::high_resolution_clock::now();
        sort(begin(buffer), end(buffer));
        elapsed += chrono::high_resolution_clock::now() - start;
        copy(first, last, begin(buffer));
    }
    return elapsed / reps;
}


template <typename T, class Distribution>
void test(Distribution dist, unsigned const seed = 0, unsigned const max2 = 26)
{
    typedef foo<T> value_type;
    typedef transformation::implicit<T> converter;
    typedef typename vector<value_type>::iterator v_iterator;
    typedef typename vector<value_type>::const_iterator v_const_iterator;

    cout << "=== Test (seed = " << seed << ", T = " << typeid(T).name() << "). ===" << endl;
    mt19937 rng(seed);
    converter const conv;

    cout << "log2(n)\tt/n (ns)\tc\tx sort\tx stable_sort\n";
    cout << "-------------------------------------------------------------\n";
    for(unsigned p = 1; p <= max2; p++)
    {
        size_t const n(1ul << p);
        vector<value_type> A;
        A.reserve(n);
        generate_n(back_inserter(A), n, bind(dist, rng));
        vector<value_type> B(A);
        auto const  _min = min_element(begin(A), end(A)), 
                    _max = max_element(begin(A), end(A));
        cout << setw(7) << p << "\t" << flush;
        vector<value_type> X(A);
        auto const f1 = bind(static_cast<v_iterator(*)(v_const_iterator, v_const_iterator, v_iterator, converter, T, T, unsigned)>(stable_radix_sort<v_const_iterator, v_iterator, converter>), placeholders::_1, placeholders::_2, begin(B), conv, conv(*_min), conv(*_max), 8u);
        auto const f2 = bind(sort<v_iterator>, placeholders::_1, placeholders::_2);
        auto const f3 = bind(stable_sort<v_iterator>, placeholders::_1, placeholders::_2);
        auto const t1 = time_sort(f1, begin(A), end(A), p, max2);
        auto const t2 = time_sort(f2, begin(A), end(A), p, max2);
        auto const t3 = time_sort(f3, begin(A), end(A), p, max2);

        cout.precision(2);
        cout.setf(ios::fixed);
        cout << setw(8) << t1.count() / n << "\t";
        cout << t1.count() / n / log2(n) << "\t";
        cout.precision(1);
        cout << t2.count() / t1.count() << "\t";
        cout << t3.count() / t1.count();
        cout << endl;
    }
}



int main(int argc, char **argv)
{
    // Configure cout to my liking.
    ios_base::sync_with_stdio(false);
    locale::global(locale(""));
    cout.imbue(locale());
    
    unsigned const  seed(argc < 2 ? time(NULL) : lexical_cast<unsigned>(argv[1])),
                    max_logn(argc < 3 ? 24 : stoul(argv[2]));

    cout << "\n*** poisson_distribution, mean = numeric_limits<T>::max() / 2" << endl;
    test<unsigned char>(poisson_distribution<unsigned char>(numeric_limits<unsigned char>::max() / 2), seed, max_logn);
    test<unsigned short>(poisson_distribution<unsigned short>(numeric_limits<unsigned short>::max() / 2), seed, max_logn);
    test<unsigned int>(poisson_distribution<unsigned int>(numeric_limits<unsigned int>::max() / 2), seed, max_logn);
    test<unsigned long>(poisson_distribution<unsigned long>(numeric_limits<unsigned long>::max() / 2), seed, max_logn);

    cout << "\n*** uniform_int_distribution, min = 0, k = numeric_limits<T>::max()" << endl;
    test<unsigned char>(uniform_int_distribution<unsigned char>(0, numeric_limits<unsigned char>::max()), seed, max_logn);
    test<unsigned short>(uniform_int_distribution<unsigned short>(0, numeric_limits<unsigned short>::max()), seed, max_logn);
    test<unsigned int>(uniform_int_distribution<unsigned int>(0, numeric_limits<unsigned int>::max()), seed, max_logn);
    test<unsigned long>(uniform_int_distribution<unsigned long>(0, numeric_limits<unsigned long>::max()), seed, max_logn);
    return 0;
}
