#include <vector>
#include <map>
#include <utility>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>
#include <limits>
#include <fstream>
#include <sstream>

using namespace std;

//given a vector of ids(integers) returns a map of counts of consecutive pairs.
// returns: {(1,3): 3, (2,5): 4, ..}
map<pair<int,int>, int> get_stats(
    const vector<int> &ids,
    map<pair<int,int>, int> *counts = nullptr
){
    map<pair<int,int>,int> local_counts;
    map<pair<int,int>, int>& ref_counts = counts ? *counts : local_counts;
    // if(counts){
    //     map<pair<int,int>,int> &ref_counts = *counts;
    // }else{
    //     map<pair<int,int>,int> &ref_counts = local_counts;
    // }
    
    for( size_t i = 0; i+1 < ids.size(); i++){
        pair<int, int> p = {ids[i], ids[i+1]};
        ref_counts[p]++;
    }
    return ref_counts;
}

//function that performs token mergers. Given, (tok_1, tok_2) in V it merges it to 
// tok_3 not in V, if the pair satisfies a certain condition.
// given ids of the sentence and a pair which we want to merge to a new id
// this function performs the mergers.
//eg ids = [1,2,3,1,2], pair = (1,2), idx = 4 -> [4,3,4]
vector<int> merge(
    const vector<int> &ids,
    const pair<int, int> &pair,
    int idx
){
    vector<int> newids;
    size_t i = 0;

    while( i < ids.size()){
        if( i < ids.size() - 1 && ids[i] == pair.first && ids[i+1] == pair.second){
            newids.push_back(idx);
            i+=2;
        }else{
            newids.push_back(ids[i]);
            i+=1;
        }
    }
    return newids;
}


string render_token(const vector<unsigned char>& token){
    //not training for control sequences as of now
    return std::string(token.begin(), token.end());
}



class BasicTokenizer{
    public:
        map<pair<int,int>, int> merges;
        map<int,vector<unsigned char>> vocab;

        BasicTokenizer(){
            // initialise with the bytes from 0 to 255.
            for(int i = 0; i < 256; i++){
                vocab[i] = {(unsigned char) i};
            }
        }

        void train(
            const string& text,
            int vocab_size,
            bool verbose = false
        ){
            vector<int> ids;
            ids.reserve(text.size());

            for(char c: text){
                ids.push_back(static_cast<unsigned char>(c));
            }
            //iteratively merge the most common pairs
            int num_merges = vocab_size - 256;

            for(int i = 0; i < num_merges; i++){
                
                auto stats = get_stats(ids); //gets the pairs in the current sequence of integers

                if(stats.empty()) break;//no more pairs to merge

                //mint a new token
                int idx = 256 + i;

                auto max_it = max_element(stats.begin(), stats.end(),
                        [](const auto& a, const auto &b){
                            return a.second < b.second; //picks (1,2): 3 over (2,1): 1
                        });
                
                pair<int, int> pair = max_it->first; //get the top ranked pair
                ids = merge(ids,pair, idx); //get the new list of ids after you perform the merges 

                this->merges[pair] = idx; //the merges map stores the combined pair and the new token assigned to it.
                vector<unsigned char> new_token = vocab[pair.first]; //say, new_token = {'h', 'e'}
                //say, vocab[pair.second] = {'l', 'l', 'o'}
                new_token.insert(new_token.end(), vocab[pair.second].begin(), vocab[pair.second].end());
                //now after insertion, new_token = {'h', 'e', 'l', 'l', 'o'}
                this->vocab[idx] = new_token;
                

                if (verbose) {
                    cout << "Merge " << i + 1 << "/" << num_merges << ": ("
                         << pair.first << "," << pair.second << ") -> " << idx
                         << " (" << stats[pair] << " occurrences)" << endl;
                }



            }
        }


        
        string decode(const vector<int> &ids){
            vector<unsigned char> text_bytes;

            for(int id: ids){
                const auto &token = vocab.at(id); //gets the token situated at vocab[id]
                text_bytes.insert(text_bytes.end(), token.begin(), token.end()); //inserts the token at the end
            }
            //returns the decoded string.
            return string(text_bytes.begin(), text_bytes.end());
        }



        vector<int> encode(string &text){

            vector<int> ids;
            //reserve the kength of text worth of space for tokens
            ids.reserve(text.size());

            for(char c: text){
                ids.push_back(static_cast<unsigned char>(c));
            }

            while(ids.size() >= 2){
                //get the pair statistics
                auto stats = get_stats(ids);

                pair<int, int> best_pair;

                int min_rank = numeric_limits<int>::max();

                for(const auto &stat_pair : stats){
                    auto it = merges.find(stat_pair.first);
                    if(it != merges.end()){
                        min_rank = it->second;
                        best_pair = it->first;
                    }
                }
                // If no known merges are found in the sequence, we're done
                if (min_rank == numeric_limits<int>::max()) {
                    break;
                }

                // Otherwise, merge the best pair
                int idx = merges.at(best_pair);
                ids = merge(ids, best_pair, idx);
            }
            return ids;

        }





    
    
    
    };

int main() {
    BasicTokenizer tokenizer;

    // 2. train
    string text = "MY name is Rajdeep Aher";
    int vocab_size = 260; // 256 bytes + 4 merges
    cout << "--- Training ---" << endl;
    tokenizer.train(text, vocab_size, true);


    cout << "\n--- Learned Merges ---" << endl;
    for(const auto& pair : tokenizer.merges) {
        cout << "(" << pair.first.first << ", " << pair.first.second << ") -> " << pair.second << endl;
    }

    //encode
    string test_string = "abracadabra";
    cout << "\n--- Encoding '" << test_string << "' ---" << endl;
    vector<int> encoded = tokenizer.encode(test_string);
    
    cout << "Encoded IDs: ";
    for(int id : encoded) {
        cout << id << " ";
    }
    cout << endl;

    //decode
    cout << "\n--- Decoding IDs ---" << endl;
    string decoded = tokenizer.decode(encoded);
    cout << "Decoded string: " << decoded << endl;

    //verify
    if (test_string == decoded) {
        cout << "\nSuccess! The decoded string matches the original." << endl;
    } else {
        cout << "\nFailure. Strings do not match." << endl;
    }

    return 0;
}