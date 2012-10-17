#include <vector>
#include <iostream>

using namespace std;

//finds prime numbers using Sieve of Eratosthenes algorithm
vector<int> calc_primes(const int min, const int max);

int main(int argc, char *argv[])
{
	cout << "Min number: ";
    int min;
	cin >> min;
	cout << "Min number: ";
    int max;
	cin >> max;
	
    vector<int> primes = calc_primes(min, max);

    for(int i = 0; i < primes.size(); i++)
    {
        if(primes[i] != 0)
                cout<<primes[i]<<endl;
    }

    return 0;
}

vector<int> calc_primes(const int min, const int max)
{
    vector<int> primes;

    // fill vector with candidates
    for(int i = min; i < max; i++)
    {
        primes.push_back(i);
    }

    // for each value in the vector...
    for(int i = 0; i < primes.size(); i++)
    {
        //get the value
        int v = primes[i];

        if (v!=0) {
                //remove all multiples of the value
                int x = i+v;
                while(x < primes.size()) {
                        primes[x]=0;
                        x = x+v;
                }
        }
    }
    return primes;
}