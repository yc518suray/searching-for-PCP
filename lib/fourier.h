#include"fftw3.h"
#include"golay.h"
#include<array>
#include<set>
#include<vector>
using namespace std;

vector<vector<double>> dft(vector<int> seq, fftw_complex * in, fftw_complex * out, fftw_plan p);
bool dftfilter(vector<vector<double>> seqdft, int len, int ORDER);
int dftfilterpair(fftw_complex *dftA, fftw_complex *dftB, int len);
std::set<int> spectrumthree(int len, int ORDER);
