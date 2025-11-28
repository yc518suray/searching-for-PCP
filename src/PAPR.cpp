#include <iostream>
#include <vector>
#include <cmath>
#include <complex>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <algorithm>
#include <stdexcept>

#include"../lib/fourier.h"
#include"../lib/golay.h"
#include "fftw3.h"

double calculate_papr_linear(const std::vector<std::complex<double>>& time_domain_sequence) {
    if (time_domain_sequence.empty()) {
        return 0.0;
    }

    double max_power = 0.0;
    double total_power = 0.0;
    size_t N = time_domain_sequence.size();
    
    for (const auto& sample : time_domain_sequence) {
        //P_n = |x[n]|^2 = real^2 + imag^2
        double instantaneous_power = std::norm(sample); 
        
        max_power = std::max(max_power, instantaneous_power);
        total_power += instantaneous_power;
    }

    double average_power = total_power / N;

    if (average_power == 0.0) {
        return 0.0; 
    }
    
    // PAPR = P_peak / P_avg
    return max_power / average_power;
}


int main(int argc, char ** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <ORDER> <LEN>" << std::endl;
        return 1;
    }

	int oversampling = 30;

    int ORDER = std::stoi(argv[1]);
    int LEN = std::stoi(argv[2]);
    int N = LEN; // original sequence length
	int Noversample = N * oversampling;

    fftw_complex *in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * Noversample);
    fftw_complex *out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * Noversample);
    
    fftw_plan p_ifft = fftw_plan_dft_1d(Noversample, in, out, FFTW_BACKWARD, FFTW_ESTIMATE);

    char fname[100];
    std::sprintf(fname, "results/%d-unique-pairs-found", ORDER);

    std::ifstream infile(fname);
    if (!infile.is_open()) {
        std::cerr << "Error: Could not open file " << fname << std::endl;
        fftw_destroy_plan(p_ifft);
        fftw_free(in);
        fftw_free(out);
        return 1;
    }

    std::string line;
    int pair_count = 0;
    
    std::cout << "---    Golay Pair PAPR    ---\n";
    std::cout << "Pair |  PAPR (A)  |  PAPR (B)\n";
    std::cout << "--------------------------------\n";
    std::cout << std::fixed << std::setprecision(6); // increase output precision

    while (std::getline(infile, line)) {
    	pair_count++;

    	// 1. use stringstream to parse the line
        std::stringstream ss(line);
        int value;
        std::vector<int> full_sequence;

        while (ss >> value) {
            full_sequence.push_back(value);
        }

        // 2. check length
        if ((int)full_sequence.size() != 2 * N) {
            // error handling
			std::cout<<"something is wrong with line "<<pair_count<<std::endl;
            continue;
        }

        // 3. seperate A and B
		std::vector<int> seqa(Noversample, 0);
		copy(full_sequence.begin(), full_sequence.begin() + N, seqa.begin());
        std::vector<int> seqb(Noversample, 0);
		copy(full_sequence.begin() + N, full_sequence.end(), seqb.begin());

		//Su: PAPR of sequence A
        vector<vector<double>> xA_fft = dft(seqa, in, out, p_ifft); 
        std::vector<std::complex<double>> xA_time_domain;

		for(int i = 0; i < Noversample; ++i) {
            // IFFT output devided by N normalization
            xA_time_domain.emplace_back(xA_fft[i][0] / Noversample, xA_fft[i][1] / Noversample); 
        }
        double paprA_linear = calculate_papr_linear(xA_time_domain);

		//Su: PAPR of sequence B
        vector<vector<double>> xB_fft = dft(seqb, in, out, p_ifft); 
        std::vector<std::complex<double>> xB_time_domain;
        
		for(int i = 0; i < Noversample; ++i) {
            xB_time_domain.emplace_back(xB_fft[i][0] / Noversample, xB_fft[i][1] / Noversample); 
        }
        double paprB_linear = calculate_papr_linear(xB_time_domain);

        // output results
        std::cout << std::setw(4) << pair_count << " | "
                  << std::setw(10) << paprA_linear << " | "
                  << std::setw(10) << paprB_linear << std::endl;
    }

    //clean fftw resources
    fftw_destroy_plan(p_ifft);
    fftw_free(in);
    fftw_free(out);

    std::cout << "\ndone, excuted " << pair_count << " pairs." << std::endl;
    
    return 0;
}

