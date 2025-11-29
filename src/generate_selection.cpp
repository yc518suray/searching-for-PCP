// GENERATE_SELECTION_CPP
// usage:
// ./generate_selection.cpp [# of total columns] [# of lines to read per execution] [# of processes]

#include<cstdio>
#include<fstream>
#include<string>
#include<vector>
#include<random>
#include<algorithm>

using namespace std;

int main(int argc, char ** argv)
{
	int num_col = stoi(argv[1]);
	int num_select = stoi(argv[2]);
	int num_procs = stoi(argv[3]);

	int num_col_proc = num_col / num_procs; // number of total columns, per process

	vector<int> array(num_col_proc);
	for(int i = 0; i < num_col_proc; i++) array[i] = i + 1;

	// random permutation of array
	int seed = 42; // random seed, can be changed
	std::shuffle(array.begin(), array.end(), std::default_random_engine(seed));

	// output random selections
	int cnt = 0;
	char fname[25];
	sprintf(fname, "rd_select.out");
	FILE * outfile;
	outfile = fopen(fname, "w");
	for(int i = 0; i < num_col_proc; i++)
	{
		cnt++;
		fprintf(outfile, "%d ", array[i]);
		if(cnt == num_select)
		{
			cnt = 0;
			fprintf(outfile, "%d\n", -1);
		}
	}
	fclose(outfile);

	return 0;
}
