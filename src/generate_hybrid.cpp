#include<stdio.h>
#include<stdlib.h>
#include<vector>
#include<set>
#include<array>
#include<time.h>
#include"../lib/orderly_equivalence.h"
#include"fftw3.h"
#include"../lib/array.h"
#include"../lib/decomps.h"
#include"../lib/fourier.h"
#include<tgmath.h>
#include<algorithm>
#include<omp.h>

using namespace std;

void writeSeq(FILE * out, vector<int> seq);

//Su: actually it's norm squared
double norm(vector<double> dft) {
    return dft[0] * dft[0] + dft[1] * dft[1];
}

void printArray(vector<int> seq) {
    for(unsigned int i = 0; i < seq.size(); i++) {
        printf("%d ", seq[i]);
    }
    printf("\n");
}

int rowsum(vector<int> seq) {
    int sum = 0;
    for(unsigned int i = 0; i < seq.size(); i++) {
        sum = sum + seq[i];
    }
    return sum;
}

bool nextBranch(vector<int>& seq, unsigned int len, set<int> alphabet);

template<class BidirIt>
bool nextPermutation(BidirIt first, BidirIt last, set<int> alphabet);

/*----------main----------*/

int main(int argc, char ** argv) {

    int ORDER = stoi(argv[1]);
    int COMPRESS = stoi(argv[2]);
    int LEN = ORDER / COMPRESS;
	const int NUM_PARA_THREADS = stoi(argv[3]);

	//Su: FFTW plan initialization for each thread
	fftw_complex * in[NUM_PARA_THREADS];
	fftw_complex * out[NUM_PARA_THREADS];
	fftw_plan plan[NUM_PARA_THREADS];
	for(int i = 0; i < NUM_PARA_THREADS; i++)
	{
		in[i] = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * LEN);
    	out[i] = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * LEN);
    	plan[i] = fftw_plan_dft_1d(LEN, in[i], out[i], FFTW_FORWARD, FFTW_MEASURE);
	}

	//Su: OpenMP settings
	omp_set_dynamic(0);	
	omp_set_num_threads(NUM_PARA_THREADS);

	//Su: output file settings
	char fname[100];
	FILE * outa [NUM_PARA_THREADS];
	FILE * outb [NUM_PARA_THREADS];
	for(int i = 0; i < NUM_PARA_THREADS; i++)
	{
    	sprintf(fname, "results/%d/%d-unique-filtered-a_%d-%d", ORDER, ORDER, 1, i);
    	outa[i] = fopen(fname, "w");
    	sprintf(fname, "results/%d/%d-unique-filtered-b_%d-%d", ORDER, ORDER, 1, i);
    	outb[i] = fopen(fname, "w");
	}
    
	//Su: generation settings
	unsigned long long int count = 0;

    std::set<int> alphabet;

    if(COMPRESS % 2 == 0) {
        for(int i = 0; i <= COMPRESS; i += 2) {
            alphabet.insert(i);
            alphabet.insert(-i);
        }
    } else {
        for(int i = 1; i <= COMPRESS; i += 2) {
            alphabet.insert(i);
            alphabet.insert(-i);
        }
    }

    vector<int> seq;
	set<vector<int>> generatorsA;
	set<vector<int>> generatorsB;
	#pragma omp parallel sections
	{
		#pragma omp section
		{
			generatorsA = constructGenerators(0, LEN);
		}	
		#pragma omp section
		{
			generatorsB = constructGenerators(1, LEN);
		}
	}

	//Su: flag = 0 --> decimation equivalents are generated
	//Su: flag = 1 --> decimation equivalents NOT generated

	bool stop_branching = false;
	vector<vector<int>> combinations = getCombinations(LEN - LEN / 2, alphabet);
	
	//Su: start generation
	#pragma omp parallel
	{
	int tid = 0;
	#pragma omp single
	{
	int nthreads = omp_get_num_threads();
	printf("nthreads = %d\n", nthreads);
	
	//Su: seq is of size 0 at the beginning
    while(!stop_branching && nextBranch(seq, LEN / 2, alphabet)) {

		//Su: orderly generation, corrected
        while(!partialCanonical(seq)) {
            if(!nextBranch(seq, seq.size(), alphabet)) {
				stop_branching = true;
				break;
            }
        }
	
		//Su: integer partition, parallelized
        if(!stop_branching && (int)seq.size() == LEN / 2) {   

			//finish the constructions
            std::vector<std::vector<int>> rowcombo;
            for(std::vector<int> combo : combinations) {
                int sum = rowsum(combo);
                if(sum == decomps[ORDER][0][0] - rowsum(seq) ||\
				   sum == decomps[ORDER][0][1] - rowsum(seq)) {
                    rowcombo.push_back(combo);
                }
            }

			#pragma omp taskloop grainsize(1) firstprivate(seq) shared(count)
            for(size_t i = 0; i < rowcombo.size(); i++) {
				tid = omp_get_thread_num();
				
				vector<int> tail = rowcombo[i];
                sort(tail.begin(), tail.end());
                vector<int> newseq = seq;
                newseq.insert(newseq.end(), tail.begin(), tail.end());

				do
                {
					//Su: not lexicographically minimal
                    if(newseq.back() == *alphabet.begin()) {
                        continue;
                    }

                    if(rowsum(newseq) == decomps[ORDER][0][0]) {
						vector<vector<double>> outdft = dft(newseq, in[tid], out[tid], plan[tid]);
                        if(dftfilter(outdft, LEN, ORDER) && isCanonical(newseq, generatorsA)) {
							count++;
                            for(int i = 0; i < LEN / 2; i++) {
                                fprintf(outa[tid], "%d", (int)rint(norm(outdft[i])));
                            }
                            fprintf(outa[tid], " ");
                            writeSeq(outa[tid], newseq);
                            fprintf(outa[tid], "\n");
                        }
                    }

                    if(rowsum(newseq) == decomps[ORDER][0][1]) {
                        vector<vector<double>> outdft= dft(newseq, in[tid], out[tid], plan[tid]);
                        if(dftfilter(outdft, LEN, ORDER) && isCanonical(newseq, generatorsB)) {
							count++;
                            for(int i = 0; i < LEN / 2; i++) {
                                fprintf(outb[tid], "%d",\
										ORDER * 2 - (int)rint(norm(outdft[i])));
                            }
                            fprintf(outb[tid], " ");
                            writeSeq(outb[tid], newseq);
                            fprintf(outb[tid], "\n");
                        }
                    }
				} while(next_permutation(newseq.begin() + LEN / 2, newseq.end()));
            }
			#pragma omp taskwait
        }
    } //Su: end of while loop for integer partition part
	} //Su: end of omp single
	} //Su: end of omp parallel
	
	//Su: close output file
	for(int i = 0; i < NUM_PARA_THREADS; i++)
	{
		fclose(outa[i]);
		fclose(outb[i]);
	} 
	
	//Su: free FFTW memories
	for(int i = 0; i < NUM_PARA_THREADS; i++)
	{
		fftw_free(in[i]);
    	fftw_free(out[i]);
    	fftw_destroy_plan(plan[i]);
	}

	printf("%llu\n", count);
	
	return 0;  
}

/*----------function definitions----------*/

template<class BidirIt>
bool nextPermutation(BidirIt first, BidirIt last, set<int> alphabet) {
    int min = *std::min_element(alphabet.begin(), alphabet.end());
    int max = *std::max_element(alphabet.begin(), alphabet.end());

    last = last - 1;

    auto curr = last;

    if(*curr != max) {

        *curr = *curr + 2;
        return true;

    } else if(*curr == max) {

        while(curr != first - 1) {
            if(*curr != max) {
                *curr = *curr + 2;
                curr++;
                while(curr != last + 1) {
                    *curr = min;
                    curr++;
                }
                return true;
            }
            curr--;
        }

        curr++;
        while(curr != last) {
            *curr = min;
        }

        return false;

    }

    return false;
}

bool nextBranch(vector<int>& seq, unsigned int len, set<int> alphabet) {

	//Su: elements are stored in set with ascending order
	//	  min = minimum in the set, max = maximum in the set

    int max = *alphabet.rbegin();
    int min = *alphabet.begin();

        if(seq.size() == len) {
            while(seq.size() != 0 && seq[seq.size() - 1] == max) {
                seq.pop_back();
            }
            if(seq.size() == 0) {
				//Su: no, don't go to next branch
                return false;
            }
            int next = seq.back() + 2;
            seq.pop_back();
            seq.push_back(next);
        }
		//Su: the minimum letter in alphabet is inserted to seq
		else {
            seq.push_back(min);
        }
    
	//Su: yes, next branch please
    return true;

}


void writeSeq(FILE * out, vector<int> seq) {
    for(unsigned int i = 0; i < seq.size(); i++) {
        fprintf(out, "%d ", seq[i]);
    }
}

int classIsGenerated(vector<set<vector<int>>>& classes, vector<int>& seq) {
    for(set<vector<int>> map : classes) {
        if(map.find(seq) != map.end()) {
            return 1;
        }
    }
    return 0;
}
