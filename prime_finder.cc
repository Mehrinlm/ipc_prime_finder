#include <vector>
#include <iostream>

using namespace std;

//finds prime numbers using Sieve of Eratosthenes algorithm
vector<int> calc_primes(const int min, const int max);
void send_primes(vector<int> primes);

int main(int argc, char *argv[]){
	cout << "Min number: ";
    int min;
	cin >> min;
	cout << "Min number: ";
    int max;
	cin >> max;
	
    vector<int> primes = calc_primes(min, max);
	
	//Send across wire
	send_primes(primes);
	
	
	cout << "Sending:";
	//This prints out primes to screen
    for(int i = 0; i < primes.size(); i++) {
        if(primes[i] != 0){
			cout << " " << primes[i];
		}	
    }
	cout << endl;
    return 0;
}

/**
 * Sends primes across wire to other program
 * @param primes Vector of integers that represent primes (only send ones
 * 					that are not zero)
 */
 void send_primes(vector<int> primes){
	//do stuff
 }

/** 
 *	Does one iteration and returns all the values remaining in 
 *	the range that have not been ruled out.
 */
vector<int> calc_primes(const int min, const int max) {
    vector<int> primes;

    // fill vector with candidates
    for(int i = min; i < max; i++) {
        primes.push_back(i);
    }

	//get the value
	int v = primes[0];

	if (v!=0) {
			//remove all multiples of the value
			int x = v;
			while(x < primes.size()) {
					primes[x]=0;
					x = x+v;
			}
	}
    return primes;
}