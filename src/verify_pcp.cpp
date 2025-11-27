#include<iostream>
#include<string>
#include<fstream>

using namespace std;

/*----------declarations----------*/
bool notPCP(int * seqA, int * seqB, int L);

int main(int argc, char * argv[])
{
	/*----------initial----------*/
	ifstream infile(argv[1]);
	int LEN = stoi(argv[2]);

	/*----------verify----------*/
	int nline = 0;
	int seq1[LEN];
	int seq2[LEN];
	bool allPCP = true;
	string str;

	while(infile>>str)
	{
		seq1[0] = stoi(str);
		nline++;
		for(int i = 1; i < LEN; i++)
		{
			infile>>str;
			seq1[i] = stoi(str);
		}
		for(int i = 0; i < LEN; i++)
		{
			infile>>str;
			seq2[i] = stoi(str);
		}
		if(notPCP(seq1, seq2, LEN))
		{
			allPCP = false;
			cout<<"Line "<<nline<<" is not PCP."<<endl;
		}
	}
	
	if(allPCP) cout<<"All pairs are PCP."<<endl;

	infile.close();
	return 0;
}

/*----------function definitions----------*/
bool notPCP(int * seqA, int * seqB, int L)
{
	int corr_sum_A, corr_sum_B;
	for(int u = 1; u <= L/2; u++)
	{
		corr_sum_A = 0;
		corr_sum_B = 0;
		for(int i = 0; i < L; i++)
		{
			corr_sum_A += ((seqA[(i + u) % L] == seqA[i])? 1: -1);
			corr_sum_B += ((seqB[(i + u) % L] == seqB[i])? 1: -1);
		}
		if((corr_sum_A + corr_sum_B) != 0) return true;
	}
	return false;
}
