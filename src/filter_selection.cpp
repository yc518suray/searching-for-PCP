// usage: ./filter_selection [path to order-pairs-found] [order] [compress]

#include<cstdio>
#include<string>
#include<fstream>
#include<cmath>
#include<vector>

using namespace std;

/*---------- declarations ----------*/

vector<int> get_abs(vector<int> a);
vector<int> get_reverse(vector<int> a);
void output_seq(vector<int> a);

/*---------- main ----------*/

int main(int argc, char ** argv)
{
	string infname = argv[1];
	int order = stoi(argv[2]);
	int compress = stoi(argv[3]);
	int LEN = order / compress;

	char outfname[25];
	sprintf(outfname, "%d-pairs-found-filtered", order);

	ifstream infile(infname);
	ofstream outfile(outfname);
	if(!infile)
	{
		printf("cannot open input file\n");
		exit(1);
	}
	
	string str;
	vector<int> a(LEN, 0);
	vector<int> b(LEN, 0);
	while(infile>>str)
	{
		a[0] = stoi(str);
		for(int i = 1; i < LEN; i++)
		{
			infile>>str;
			a[i] = stoi(str);
		}
		for(int i = 0; i < LEN; i++)
		{
			infile>>str;
			b[i] = stoi(str);
		}

		/*----- filter -----*/

		bool pass = false;
		int suma = 0;
		int sumb = 0;
		vector<int> a_abs = get_abs(a);
		vector<int> b_abs = get_abs(b);
		vector<int> a_abs_reverse = get_reverse(a_abs);
		
		for(int i = 0; i < LEN; i++)
		{
			suma += a_abs[i];
			sumb += b_abs[i];
			int match_cnt = 0;
			int match_cnt_r = 0;
			int cnt_zero_four = 0;
			int cnt_zero_four_r = 0;
			for(int j = 0; j < LEN; j++)
			{
				if(a_abs[(i + j) % LEN] == b_abs[j])
				{
					match_cnt++;
				}
				else if((a_abs[(i + j) % LEN] == 0 && b_abs[j] == 4) ||\
					    (a_abs[(i + j) % LEN] == 4 && b_abs[j] == 0))
				{
					cnt_zero_four++;
				}
				else;

				if(a_abs_reverse[(i + j) % LEN] == b_abs[j])
				{
					match_cnt_r++;
				}
				else if((a_abs_reverse[(i + j) % LEN] == 0 && b_abs[j] == 4) ||\
					    (a_abs_reverse[(i + j) % LEN] == 4 && b_abs[j] == 0))
				{
					cnt_zero_four_r++;
				}
				else;
			}
			bool cond1 = (match_cnt >= LEN - 1);
			bool cond2 = (match_cnt_r >= LEN - 1);
			bool cond3 = ((suma * suma) + (sumb * sumb) == 2 * order) && (i == LEN - 1);
			bool cond4 = (cnt_zero_four <= 3 && match_cnt == LEN - cnt_zero_four) ||\
						 (cnt_zero_four_r <= 3 && match_cnt_r == LEN - cnt_zero_four_r);
			if(cond1 || cond2 || cond3 || cond4)
			{
				pass = true;
				break;
			}
		}
		
		/*----- write filtered -----*/
		
		if(pass)
		{
			for(int i = 0; i < LEN; i++) outfile<<a[i]<<" ";
			outfile<<" ";
			for(int i = 0; i < LEN; i++) outfile<<b[i]<<" ";
			outfile<<endl;
		}
	}

	infile.close();
	outfile.close();

	return 0;
}

/*---------- function definitions ----------*/

vector<int> get_abs(vector<int> a)
{
	size_t L = a.size();
	vector<int> x(L, 0);
	for(size_t i = 0; i < L; i++) x[i] = abs(a[i]);
	
	return x;
}

vector<int> get_reverse(vector<int> a)
{
	size_t L = a.size();
	vector<int> x(L, 0);
	for(size_t i = 0; i < L; i++) x[i] = a[L - 1 - i];
	return x;
}

void output_seq(vector<int> a)
{
	size_t L = a.size();
	for(size_t i = 0; i < L; i++) printf("%d ", a[i]);
	printf("\n");
}
